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

#include "sys_conf.h"

#include "calibration.h"
#include "pc_comm.h"
#include "core.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "io/rx_input.h"
#include "io/servo_out_bb.h"


#define DEFAULT_CONF_SLOT               0
#define CURRENT_COMPATIBILITY_MAGIC     0xCAFE0001


static system_conf_t sys_conf;
static xSemaphoreHandle sysconf_mutex;


static void set_input_map(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        PC_COMM_RX_MEMBERS(packet, sys_conf.in_conf, mapping, sys_conf.input_nb);
        xSemaphoreGive(sysconf_mutex);
    }
}

static void req_input_map(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        pccomm_msg_header_t *header = &packet->header;
        PC_COMM_SEND_MEMBERS(header, sys_conf.in_conf, mapping, sys_conf.input_nb);
        xSemaphoreGive(sysconf_mutex);
    }
}

static void set_input_name(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        PC_COMM_RX_MEMBERS(packet, sys_conf.in_conf, name, sys_conf.input_nb);
        xSemaphoreGive(sysconf_mutex);
    }
}

static void set_output_name(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        PC_COMM_RX_MEMBERS(packet, sys_conf.out_conf, name, sys_conf.output_nb);
        xSemaphoreGive(sysconf_mutex);
    }
}

static void req_input_name(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        pccomm_msg_header_t *header = &packet->header;
        PC_COMM_SEND_MEMBERS(header, sys_conf.in_conf, name, sys_conf.input_nb);
        xSemaphoreGive(sysconf_mutex);
    }
}

static void req_output_name(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        pccomm_msg_header_t *header = &packet->header;
        PC_COMM_SEND_MEMBERS(header, sys_conf.out_conf, name, sys_conf.output_nb);
        xSemaphoreGive(sysconf_mutex);
    }
}


static void set_output_speed(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        PC_COMM_RX_MEMBERS(packet, sys_conf.out_conf, max_speed, sys_conf.output_nb);
        xSemaphoreGive(sysconf_mutex);
    }
}

static void req_output_speed(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        pccomm_msg_header_t *header = &packet->header;
        PC_COMM_SEND_MEMBERS(header, sys_conf.out_conf, max_speed, sys_conf.output_nb);
        xSemaphoreGive(sysconf_mutex);
    }
}


static void req_input_calib_value(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        pccomm_msg_header_t *header = &packet->header;
        PC_COMM_SEND_MEMBERS(header, sys_conf.in_conf, calib, sys_conf.input_nb);
        xSemaphoreGive(sysconf_mutex);
    }
}

static void req_output_calib_value(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        pccomm_msg_header_t *header = &packet->header;
        PC_COMM_SEND_MEMBERS(header, sys_conf.out_conf, calib, sys_conf.output_nb);
        xSemaphoreGive(sysconf_mutex);
    }
}

static void set_input_calib_value(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        PC_COMM_RX_MEMBERS(packet, sys_conf.in_conf, calib, sys_conf.input_nb);
        xSemaphoreGive(sysconf_mutex);
    }
}

static void set_output_calib_value(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        PC_COMM_RX_MEMBERS(packet, sys_conf.out_conf, calib, sys_conf.output_nb);
        xSemaphoreGive(sysconf_mutex);
    }
}

static void req_output_active_bm(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        pccomm_msg_header_t *header = &packet->header;
        pc_comm_send_packet(header->module, header->command, 0, &sys_conf.servo_active_bm,
                sizeof(sys_conf.servo_active_bm));
        xSemaphoreGive(sysconf_mutex);
    }
}

static void set_output_active_bm(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        uint32_t *bitmask = (uint32_t*)(packet->data);
        sys_conf.servo_active_bm = *bitmask;
        xSemaphoreGive(sysconf_mutex);
    }
}



static const pc_comm_rx_callback sys_conf_callbacks[] =
{
	/*
    [SAVE_SYS_CONF] = save_sys_conf,
    [LOAD_SYS_CONF] = req_load_sys_conf,
    */

    [SET_INPUT_MAPPING] = set_input_map,
    [REQ_INPUT_MAPPING] = req_input_map,

    [SET_INPUT_NAME] = set_input_name,
    [SET_OUTPUT_NAME] = set_output_name,

    [REQ_INPUT_NAME] = req_input_name,
    [REQ_OUTPUT_NAME] = req_output_name,

    [SET_OUTPUT_MAX_SPEED] = set_output_speed,
    [REQ_OUTPUT_MAX_SPEED] = req_output_speed,

    [REQ_OUTPUT_ACTIVE_BM] = req_output_active_bm,
    [SET_OUTPUT_ACTIVE_BM] = set_output_active_bm,

    [REQ_INPUT_CALIB_VALUE] = req_input_calib_value,
    [REQ_OUTPUT_CALIB_VALUE] = req_output_calib_value,
    [SET_INPUT_CALIB_VALUE] = set_input_calib_value,
    [SET_OUTPUT_CALIB_VALUE] = set_output_calib_value,
};

static pccom_callbacks comm_sys_conf_callbacks =
{
    .callback_nb = SIZEOF_ARRAY(sys_conf_callbacks),
    .callbacks = sys_conf_callbacks
};



void sys_conf_set_input_calib(uint8_t input_nb, const input_calib_data_t *calib)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        if(input_nb < MAX_IN_NB)
        {
            memcpy(&sys_conf.in_conf[input_nb].calib, calib, sizeof(input_calib_data_t));
        }
        xSemaphoreGive(sysconf_mutex);
    }
}

void sys_conf_reset_input_calib()
{
    int i;
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        for(i=0; i<MAX_IN_NB; i++)
        {
            input_calib_reset_for_calib(&sys_conf.in_conf[i].calib);
        }
        xSemaphoreGive(sysconf_mutex);
    }
}

void init_sys_conf()
{
    int i;

    //disable all servos
    sys_conf.compatibility_magic = CURRENT_COMPATIBILITY_MAGIC;
    sys_conf.servo_active_bm = 0x00000000;

    sys_conf.input_nb = MAX_IN_NB;
    for(i=0; i<sys_conf.input_nb; i++)
    {
        input_conf_t *in_conf = &sys_conf.in_conf[i];

        in_conf->mapping = i;
        in_conf->name[0] = 'p';
        in_conf->name[1] = 'p';
        in_conf->name[2] = 'm';
        in_conf->name[3] = (char)('0'+i/10);
        in_conf->name[4] = (char)('0'+i);
        in_conf->name[5] = '\0';
        input_calib_init_default(&in_conf->calib);
    }

    sys_conf.output_nb = MAX_OUT_NB;
    for(i=0; i<sys_conf.output_nb; i++)
    {
        output_conf_t *out_conf = &sys_conf.out_conf[i];

        out_conf->name[0] = (char)('A'+i);
        out_conf->name[1] = '\0';
        out_conf->max_speed = 0;
        output_calib_init_default(&out_conf->calib);
    }

    sysconf_mutex = xSemaphoreCreateMutex();
    if(sysconf_mutex == NULL)
    {
        FATAL_ERROR();
    }

    pc_comm_register_module_callback(PCCOM_SYS_CONF, comm_sys_conf_callbacks);
}


void sys_conf_copy(system_conf_t *copied_sys_conf)
{
    if(xSemaphoreTake(sysconf_mutex, portMAX_DELAY))
    {
        memcpy(copied_sys_conf, &sys_conf, sizeof(system_conf_t));
        xSemaphoreGive(sysconf_mutex);
    }
}
