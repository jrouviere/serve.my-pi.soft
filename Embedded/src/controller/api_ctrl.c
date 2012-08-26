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

/*
 * This is a special controller that turn the board into a simple USB
 * controlled servo controller. The PC use the API to communicate
 * with the board.
 */

#include "api_ctrl.h"

#include "frame_ctrl.h"
#include "sequence.h"

#include "rc_utils.h"
#include "core.h"
#include "board.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "pc_comm.h"

//0 means no speed/accel limit
typedef struct {
    rc_value_t goal;
} sp_out_t;


static sp_out_t sp_outs[MAX_OUT_NB];
static xSemaphoreHandle api_ctrl_mutex;
static uint32_t controlled_bm;


static void set_output_goal(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(api_ctrl_mutex, portMAX_DELAY))
    {
        PC_COMM_RX_MEMBERS(packet, sp_outs, goal, MAX_OUT_NB);
        xSemaphoreGive(api_ctrl_mutex);
    }
}

static void req_output_goal(pccomm_packet_t *packet)
{
    pccomm_msg_header_t *header = &packet->header;
    if(xSemaphoreTake(api_ctrl_mutex, portMAX_DELAY))
    {
        PC_COMM_SEND_MEMBERS(header, sp_outs, goal, MAX_OUT_NB);
        xSemaphoreGive(api_ctrl_mutex);
    }
}

static void set_control_bm(pccomm_packet_t *packet)
{
    uint32_t *bitmask = (uint32_t*)(packet->data);
    controlled_bm = *bitmask;
}

static void req_control_bm(pccomm_packet_t *packet)
{
    pccomm_msg_header_t *header = &packet->header;
    pc_comm_send_packet(header->module, header->command, 0, &controlled_bm,
            sizeof(controlled_bm));
}

static const pc_comm_rx_callback rx_callbacks[] =
{
    [API_SET_OUTPUT_GOAL] = set_output_goal,
    [API_REQ_OUTPUT_GOAL] = req_output_goal,

    [API_SET_CONTROL_BM] = set_control_bm,
    [API_REQ_CONTROL_BM] = req_control_bm,
};

static pccom_callbacks comm_callbacks =
{
    .callback_nb = SIZEOF_ARRAY(rx_callbacks),
    .callbacks = rx_callbacks
};

void api_ctrl_init()
{
    if(api_ctrl_mutex == NULL)
    {
        api_ctrl_mutex = xSemaphoreCreateMutex();
        if(api_ctrl_mutex == NULL)
        {
            FATAL_ERROR();
        }
    }

    frame_ctrl_api_init();
    seq_ctrl_api_init();

    pc_comm_register_module_callback(PCCOM_API_CTRL, comm_callbacks);
}

void api_ctrl_update(core_output_t *outputs)
{
    if(xSemaphoreTake(api_ctrl_mutex, portMAX_DELAY))
    {
        int i;
        for(i=0; i<MAX_OUT_NB; i++)
        {
            if(controlled_bm & (1 << i))
            {
                outputs[i].value = sp_outs[i].goal;
                outputs[i].active = true;
            }
        }
        xSemaphoreGive(api_ctrl_mutex);
    }

    frame_ctrl_api_update(outputs);
    seq_ctrl_api_update(outputs);
}

