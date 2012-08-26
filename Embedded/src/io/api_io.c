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

#if 0 /*unused for the moment, will be reenabled later*/

#include "api_io.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "pc_comm.h"
#include "io.h"


static int io_ins_nb;
static core_input_t *io_ins;

static int io_outs_nb;
static core_output_t *io_outs;

static xSemaphoreHandle api_io_mutex;

const io_class_t api_io_class = {
    .name = "API IO",

    .init_module = api_io_init,
    .init_channel = NULL,
    .cleanup_channel = NULL,
    .cleanup_module = api_io_cleanup,

    .pre = NULL,
    .get = api_input_get_value,
    .set = api_output_set_value,
    .post = NULL,
};

void api_input_get_value(uint8_t channel_no, core_input_t *out_val)
{
    if(channel_no >= io_ins_nb)
    {
        out_val->active = false;
    }
    else
    {
        out_val->value = io_ins[channel_no].value;
        out_val->active = io_ins[channel_no].active;
    }
}

void api_output_set_value(uint8_t channel_no, const core_output_t *val)
{
    if(channel_no >= io_outs_nb)
    {
        return;
    }

    io_outs[channel_no].value = val->value;
    io_outs[channel_no].active = val->active;
}


static void api_msg_io_set_input_value(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(api_io_mutex, portMAX_DELAY))
    {
        PC_COMM_RX_MEMBERS(data, size, core_input_t, value, io_ins, io_ins_nb);
        xSemaphoreGive(api_io_mutex);
    }
}

static void api_msg_io_set_input_active(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(api_io_mutex, portMAX_DELAY))
    {
        PC_COMM_RX_MEMBERS(data, size, core_input_t, active, io_ins, io_ins_nb);
        xSemaphoreGive(api_io_mutex);
    }
}

static void api_msg_io_get_output_value(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(api_io_mutex, portMAX_DELAY))
    {
        pccomm_msg_header_t *header = data;
        PC_COMM_SEND_MEMBERS(header, io_outs, value, io_outs_nb);
        xSemaphoreGive(api_io_mutex);
    }
}

static void api_msg_io_get_output_active(pccomm_packet_t *packet)
{
    if(xSemaphoreTake(api_io_mutex, portMAX_DELAY))
    {
        pccomm_msg_header_t *header = data;
        PC_COMM_SEND_MEMBERS(header, io_outs, active, io_outs_nb);
        xSemaphoreGive(api_io_mutex);
    }
}



static pc_comm_rx_callback rx_callbacks[] =
{
    [API_IO_SET_INPUT_VALUE] = api_msg_io_set_input_value,
    [API_IO_SET_INPUT_ACTIVE] = api_msg_io_set_input_active,

    [API_IO_REQ_OUTPUT_VALUE] = api_msg_io_get_output_value,
    [API_IO_REQ_OUTPUT_ACTIVE] = api_msg_io_get_output_active,
};

static pccom_callbacks comm_callbacks =
{
    .callback_nb = SIZEOF_ARRAY(rx_callbacks),
    .callbacks = rx_callbacks
};

void api_io_init()
{
    int i;

    io_ins_nb = 8;
    io_ins = (core_input_t*) malloc(io_ins_nb * sizeof(core_input_t));
    for(i=0; i<io_ins_nb; i++)
    {
        io_ins[i].value = 0;
        io_ins[i].active = false;
    }

    io_outs_nb = 8;
    io_outs = (core_output_t*) malloc(io_outs_nb * sizeof(core_output_t));
    for(i=0; i<io_outs_nb; i++)
    {
        io_outs[i].value = 0;
        io_outs[i].active = false;
    }

    /*create mutex if necessary*/
    if(api_io_mutex == NULL)
    {
        api_io_mutex = xSemaphoreCreateMutex();
        if(api_io_mutex == NULL)
        {
            FATAL_ERROR();
        }
    }


    pc_comm_register_module_callback(PCCOM_API_IO, comm_callbacks);
}

void api_io_cleanup()
{
    if(io_ins)
    {
        free(io_ins);
        io_ins = NULL;
        io_ins_nb = 0;
    }

    if(io_outs)
    {
        free(io_outs);
        io_outs = NULL;
        io_outs_nb = 0;
    }
}

#endif
