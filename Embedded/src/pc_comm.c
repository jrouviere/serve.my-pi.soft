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


#include "pc_comm.h"

#include "pc_comm_usb.h"
#include "openscb_utils.h"
#include "usb_drv.h"
#include "wdt.h"

#include "board.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "task.h"
#include "trace.h"

#include <string.h>


static xSemaphoreHandle slave_send_mutex;
static pccom_callbacks registered_callbacks[PCCOM_MODULE_NB];


// see header for documentation
void pc_comm_send_array(uint8_t module, uint8_t command, const void *data,
        uint8_t item_size, uint8_t item_nb)
{
    pc_comm_send_member_of_struct_array(module, command, data, item_size,
            0, item_size, item_nb);
}

void pc_comm_send_member_of_struct_array(uint8_t module, uint8_t command,
        const void *array, int struct_size, int item_offset, int item_size, int item_nb)
{
    pccomm_packet_t packet;
    int ret;
    int i;

    uint8_t index = 0;
    openscb_dev dev = (openscb_dev)get_avr32_usb_dev();
    const uint8_t *data_in = ((const uint8_t *)array) + item_offset;

    if(xSemaphoreTake(slave_send_mutex, portMAX_DELAY))
    {
        while(index < item_nb)
        {
            uint8_t *data_out = (uint8_t *) packet.data;

            //compute nb of element that will go into this packet
            int elt_per_packet = MIN(MAX_PAYLOAD_SIZE/item_size, item_nb-index);

            packet.header.module = module;
            packet.header.command = command;
            packet.header.index = index;
            packet.header.size = sizeof(pccomm_msg_header_t) +
                    elt_per_packet*item_size;

            for(i=0; i<elt_per_packet; i++)
            {
                memcpy(data_out, data_in, item_size);
                data_in += struct_size;
                data_out += item_size;
            }

            ret = scb_send_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
            if(ret < 0)
            {
                break;
            }
            index += elt_per_packet;
        }

        xSemaphoreGive(slave_send_mutex);
    }
}


// see header for documentation
void pc_comm_receive_member_of_struct_array(const pccomm_packet_t *packet, void *dest,
        int sizeof_container, int item_offset, int sizeof_item, int max_ind)
{
    const pccomm_msg_header_t *header = &packet->header;
    const char *msg_data = (const char*)packet->data;
    const char *msg_end = ((const char*)packet) + header->size;
    int ind = header->index;

    while(msg_data < msg_end)
    {
        if(ind < max_ind)
        {
            char *d = dest;
            memcpy(d+item_offset+ind*sizeof_container, msg_data, sizeof_item);
        }
        msg_data += sizeof_item;
        ind++;
    }
}


void pc_comm_receive_array(const pccomm_packet_t *packet, void *dest,
        uint8_t item_size, uint8_t item_nb)
{
    pc_comm_receive_member_of_struct_array(packet, dest, item_size,
            0, item_size, item_nb);
}


// see header for documentation
bool pc_comm_send_string(uint8_t module, uint8_t command, uint8_t index,
        const char *str)
{
    int len = strlen(str);
    return pc_comm_send_packet(module, command, index, str, min(len, MAX_PAYLOAD_SIZE));
}

// see header for documentation
int pc_comm_send_packet(uint8_t module, uint8_t command, uint8_t index,
        const void *data, uint8_t data_size)
{
    pccomm_packet_t packet;

    if(scb_build_packet(module, command, index, data, data_size, &packet) > 0)
    {
        return pc_comm_send_raw_packet(&packet);
    }
    return -1;
}


// see header for documentation
int pc_comm_send_raw_packet(pccomm_packet_t *packet)
{
    int ret;
    openscb_dev dev = (openscb_dev)get_avr32_usb_dev();

    if(xSemaphoreTake(slave_send_mutex, portMAX_DELAY))
    {
        ret = scb_send_pure_raw_message(dev, packet, DEFAULT_TIMEOUT);
        xSemaphoreGive(slave_send_mutex);
        return ret;
    }
    return -1;
}


// see header for documentation
void pc_comm_register_module_callback(uint8_t module, pccom_callbacks callbacks)
{
    if(module < PCCOM_MODULE_NB)
    {
        registered_callbacks[module] = callbacks;
    }
    else
    {
        FATAL_ERROR();
    }
}

/**
 * Read data from usb and dispatch to registered handlers.
 */
static void comm_task(void *args)
{
    pccomm_packet_t packet;
    openscb_dev dev = (openscb_dev)get_avr32_usb_dev();

    for(;;)
    {
        int rx_size = scb_get_pure_raw_message(dev, &packet, 1);

        if(rx_size > 0)
        {
            //find the appropriate callback and pass the data
            pccomm_msg_header_t *hd = &packet.header;
            if(hd->module < PCCOM_MODULE_NB)
            {
                int nb = registered_callbacks[hd->module].callback_nb;
                if(hd->command < nb)
                {
                    pc_comm_rx_callback handler = registered_callbacks[hd->module].callbacks[hd->command];
                    if(handler)
                    {
                        handler(&packet);
                    }
                }
                else
                {
                    TRACE("Unrecognized command (%d, %d)\n", hd->module, hd->command);
                }
            }
            else
            {
                TRACE("Unrecognized module (%d, %d)\n", hd->module, hd->command);
            }
        }

        //clear the watchdog
        wdt_clear();
    }
}


// see header for documentation
void pc_comm_init()
{
    slave_send_mutex = xSemaphoreCreateMutex();
    if(slave_send_mutex == NULL)
    {
        FATAL_ERROR();
    }

    xTaskCreate(comm_task,
                configTSK_COMM_NAME,
                configTSK_COMM_STACK_SIZE,
                NULL,
                configTSK_COMM_PRIORITY,
                NULL);
}


