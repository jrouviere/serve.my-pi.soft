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

#include "pc_comm_usb.h"
#include "openscb_dev_priv.h"

#include "usb_drv.h"

#include "board.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "task.h"


static void avr32_usb_dev_close(void *dev)
{
    /*unimplemented in the firmware*/
}

static int avr32_usb_dev_send(void *dev, pccomm_packet_t *packet, int timeout)
{
    //wait previous packet to be sent
    //TODO: implement timeout in the waiting loop
    while(!Is_usb_write_enabled(SLAVE_TX_EP));
    Usb_reset_endpoint_fifo_access(SLAVE_TX_EP);

    //write packet to endpoint buffer and signal it's ready
    usb_write_ep_txpacket(SLAVE_TX_EP, packet, packet->header.size, NULL);
    Usb_ack_in_ready_send(SLAVE_TX_EP);

    return true;
}

static int avr32_usb_dev_get(void *dev, pccomm_packet_t *packet, int timeout)
{
    if( Is_usb_out_received(SLAVE_RX_EP) )
    {
        Usb_reset_endpoint_fifo_access(SLAVE_RX_EP);
        uint32_t not_rx = usb_read_ep_rxpacket(SLAVE_RX_EP, packet, sizeof(pccomm_packet_t), NULL);

        Usb_ack_out_received_free(SLAVE_RX_EP);
        packet->header.size = sizeof(pccomm_packet_t)-not_rx;
        return packet->header.size;
    }
    else
    {
        portTickType xDelay = TASK_DELAY_MS(timeout);
        vTaskDelay(xDelay);
    }

    return 0;
}

static const _openscb_dev avr32_usb_dev =
{
    .close = avr32_usb_dev_close,
    .send = avr32_usb_dev_send,
    .get = avr32_usb_dev_get,
    .data = 0,
};

const openscb_dev get_avr32_usb_dev(void)
{
    return &avr32_usb_dev;
}
