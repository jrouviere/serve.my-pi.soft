/*
 *  This file is part of OpenSCB project <http://openscb.org>.
 *  Copyright (C) 2010  Opendrain
 *
 *  OpenSCB software is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenSCB software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with OpenSCB software.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "core.h"

#include "board.h"
#include "FreeRTOS.h"
#include "task.h"

#include "flashc.h"
#include "wdt.h"

#include "trace.h"
#include "rc_utils.h"
#include "system.h"
#include "pc_comm.h"

#include "out_ctrl.h"
#include "api_ctrl.h"
#include "user_flash.h"

#include "appli/appli_ctrl.h"

#include "io/rx_input.h"
#include "io/servo_out_bb.h"

#include "controller/frame_ctrl.h"
#include "controller/sequence.h"

#include "calibration.h"


//don't need to go faster than inputs/outputs
#define CORE_PERIOD_MS          14

const io_class_t *input_type = &ppm_input_class;
const io_class_t *output_type = &servobb_output_class;

static system_conf_t sys_conf;

static core_input_t inputs[MAX_IN_NB];
static core_output_t outputs[MAX_OUT_NB];

static void core_ios_pre_processing();
static void core_ios_post_processing();

static void core_inputs_get_all(core_input_t *inputs);

static void core_outputs_set_all(core_output_t *outputs);


const system_conf_t *core_get_sys_conf(void)
{
    return &sys_conf;
}

const io_class_t *core_get_output_type(void)
{
    return output_type;
}

const io_class_t *core_get_input_type(void)
{
    return input_type;
}


static core_input_t * core_get_inputs(void)
{
    return inputs;
}

static core_output_t *core_get_outputs(void)
{
    return outputs;
}

const core_output_t *core_get_output(int i)
{
    if(i < sys_conf.output_nb)
    {
        return &outputs[i];
    }
    else
    {
        return NULL;
    }
}



static void core_ios_pre_processing()
{
    if(input_type->pre != NULL)
        input_type->pre();
    if(output_type->pre != NULL)
        output_type->pre();
}

static void core_ios_post_processing()
{
    if(input_type->post != NULL)
        input_type->post();
    if(output_type->post != NULL)
        output_type->post();
}

static void core_inputs_get_all(core_input_t *inputs)
{
    int i;

    const io_class_t *type = input_type;

    for(i=0; i<sys_conf.input_nb; i++)
    {
        uint8_t chn = sys_conf.in_conf[i].mapping;
        type->get(chn, &inputs[i]);

        inputs[i].value = input_calib_raw_to_rc(&sys_conf.in_conf[i].calib,
                inputs[i].value);
    }
}


static void core_outputs_set_all(core_output_t *outputs)
{
    int i;

    const io_class_t *type = output_type;

    for(i=0; i<MAX_SERVO_NB; i++)
    {
        uint8_t chn = i;

        bool enabled = sys_conf.servo_active_bm & (1 << i);

        if((chn < sys_conf.output_nb) && enabled)
        {
            core_output_t temp;
            temp.value = output_calib_rc_to_raw(&sys_conf.out_conf[i].calib,
                    outputs[chn].value);
            temp.active = outputs[chn].active;

            type->set(i, &temp);
        }
        else
        {
            core_output_t temp;
            temp.value = 0;
            temp.active = false;
            type->set(i, &temp);
        }
    }
}


typedef enum {
    NORMAL,
    OUTPUT_CALIB,
    INPUT_CALIB,

    MODE_NB
} MODE_T;

static MODE_T mode = NORMAL;

void core_change_mode(MODE_T m)
{
    if(m < MODE_NB)
    {
        mode = m;

        // Specific init for input calibration, reset all values
        if(m == INPUT_CALIB)
        {
            sys_conf_reset_input_calib();
        }
    }
}


void core_main_task(void *arg)
{
    /* OK, for the moment this init code is placed here because
    the flash driver requires the OS to be running.*/
    user_flash_init();
    load_sys_conf();

    const portTickType xDelay = TASK_DELAY_MS(CORE_PERIOD_MS);
    portTickType xLastWakeTime = xTaskGetTickCount();

    for(;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xDelay);

        //make a copy of sys_conf to avoid simultaneous access
        sys_conf_copy(&sys_conf);

        switch(mode)
        {
        case OUTPUT_CALIB:
            //set only one value to help calibrate one servo
            output_calib_core_run(&sys_conf);
            break;

        case INPUT_CALIB:
            //compute min max for all connected inputs
            input_calib_core_run(&sys_conf);
            break;

        default:
        case NORMAL:
            // inputs
            core_ios_pre_processing();
            core_inputs_get_all(inputs);
            out_ctrl_get_value(&sys_conf, inputs, outputs);
            core_outputs_set_all(outputs);
            break;
        }

        core_ios_post_processing();
    }
}


//---------------------------------------------------------
// COMMUNICATION WITH PC
//---------------------------------------------------------

static void set_core_mode(pccomm_packet_t *packet)
{
    core_change_mode(packet->header.index);
}

static void req_input_nb(pccomm_packet_t *packet)
{
    pccomm_msg_header_t *header = &packet->header;
    pc_comm_send_packet(header->module, header->command, 0, &sys_conf.input_nb,
            sizeof(sys_conf.input_nb));
}

static void req_output_nb(pccomm_packet_t *packet)
{
    pccomm_msg_header_t *header = &packet->header;
    pc_comm_send_packet(header->module, header->command, 0, &sys_conf.output_nb,
            sizeof(sys_conf.output_nb));
}

static void req_input_value(pccomm_packet_t *packet)
{
    pccomm_msg_header_t *header = &packet->header;
    PC_COMM_SEND_MEMBERS(header, core_get_inputs(), value, sys_conf.input_nb);
}

static void req_output_value(pccomm_packet_t *packet)
{
    pccomm_msg_header_t *header = &packet->header;
    PC_COMM_SEND_MEMBERS(header, core_get_outputs(), value, sys_conf.output_nb);
}

static void req_input_active(pccomm_packet_t *packet)
{
    pccomm_msg_header_t *header = &packet->header;
    PC_COMM_SEND_MEMBERS(header, core_get_inputs(), active, sys_conf.input_nb);
}

static void req_output_active(pccomm_packet_t *packet)
{
    pccomm_msg_header_t *header = &packet->header;
    PC_COMM_SEND_MEMBERS(header, core_get_outputs(), active, sys_conf.output_nb);
}


static void req_bootloader(pccomm_packet_t *packet)
{
    reset_to_bootloader();
}

static void req_soft_version(pccomm_packet_t *packet)
{
    pccomm_msg_header_t *header = &packet->header;
    pc_comm_send_string(header->module, header->command, 0, OPENSCB_SOFT_VERSION);
}


static const pc_comm_rx_callback core_callbacks[] =
{
    [SET_CORE_MODE] = set_core_mode,
    [REQ_INPUT_NB] = req_input_nb,
    [REQ_OUTPUT_NB] = req_output_nb,
    [REQ_INPUT_VALUE] = req_input_value,
    [REQ_OUTPUT_VALUE] = req_output_value,
    [REQ_INPUT_ACTIVE] = req_input_active,
    [REQ_OUTPUT_ACTIVE] = req_output_active,

};

static pccom_callbacks comm_core_callbacks =
{
    .callback_nb = SIZEOF_ARRAY(core_callbacks),
    .callbacks = core_callbacks
};

static const pc_comm_rx_callback sys_callbacks[] =
{
    [RESTART_BOOTLOADER] = req_bootloader,
    [REQ_SOFT_VERSION] = req_soft_version,
};

static pccom_callbacks comm_sys_callbacks =
{
    .callback_nb = SIZEOF_ARRAY(sys_callbacks),
    .callbacks = sys_callbacks
};

//---------------------------------------------------------
// MODULE INIT
//---------------------------------------------------------
void core_main_init()
{
    int i;

    init_sys_conf();

    input_type->init_module();
    for(i=0; i<MAX_IN_NB; i++)
    {
        inputs[i].active = false;
        inputs[i].value = 0;
    }

    output_type->init_module();
    for(i=0; i<MAX_OUT_NB; i++)
    {
        outputs[i].active = false;
        outputs[i].value = 0;
        output_type->init_channel(i);
    }

    pc_comm_register_module_callback(PCCOM_CORE, comm_core_callbacks);
    pc_comm_register_module_callback(PCCOM_SYSTEM, comm_sys_callbacks);

    core_calib_init();
    api_ctrl_init();
    appli_init();

    //Create core task
    xTaskCreate(core_main_task,
                configTSK_CORE_NAME,
                configTSK_CORE_STACK_SIZE,
                NULL,
                configTSK_CORE_PRIORITY,
                NULL);
}

