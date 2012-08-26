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

#include "user_flash.h"

#include <stddef.h>
#include <string.h>

#include "spi_driver.h"
#include "flash_spi.h"
#include "sys_conf.h"
#include "pc_comm.h"

#include "FreeRTOS.h"
#include "task.h"


#define SLOT_NB     16
#define SLOT_SIZE   4096


static flash_spi_hdl_t flash_hdl;

static uint32_t get_slot_address(uint8_t slot_id)
{
    return slot_id * SLOT_SIZE;
}


int flash_erase_slot(uint8_t slot_id)
{
    int ret;
    ret = flash_spi_erase_sector(flash_hdl, get_slot_address(slot_id));

    if(ret != ERROR_NONE)
    {
        TRACE("flash erase failed [%d]: %d\n", slot_id, ret);
    }

    return 0;
}

void flash_init_slot(uint8_t slot_id, SLOT_TYPE type, const char *desc, uint32_t size)
{
    slot_header_t header;
    header.type = type;
    header.size = size;

    strncpy(header.description, desc, SLOT_DESC_SIZE);
    header.description[SLOT_DESC_SIZE-1] = '\0'; //force end of string

    flash_store_slot(slot_id, 0, sizeof(header), &header);
}

void flash_load_slot(uint8_t slot_id, uint32_t offset, uint32_t size, void *buffer)
{
    int ret;
    ret = flash_spi_read(flash_hdl, buffer, size, get_slot_address(slot_id)+offset);

    if(ret != ERROR_NONE)
    {
        TRACE("flash read failed [%d] @ %08X: %d\n", slot_id, offset, ret);
    }
}


void flash_store_slot(uint8_t slot_id, uint32_t offset, uint32_t size, const void *buffer)
{
    int ret;
    ret = flash_spi_write(flash_hdl, buffer, size, get_slot_address(slot_id)+offset);
    if(ret != ERROR_NONE)
    {
        TRACE("flash write failed [%d] @ %08X: %d\n", slot_id, offset, ret);
    }
}

int flash_check_slot_type(uint8_t slot_id, SLOT_TYPE type)
{
    SLOT_TYPE read_type;
    flash_load_slot(slot_id, offsetof(slot_header_t, type),
            sizeof(SLOT_TYPE), &read_type);

    if(type == read_type)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}


static void req_flash_overview(pccomm_packet_t *packet)
{
    int slot_id = packet->header.index;

    //reuse incoming packet memory
    packet->header.size = sizeof(packet->header) + sizeof(slot_header_t);

    flash_load_slot(slot_id, 0, sizeof(slot_header_t), packet->data);

    pc_comm_send_raw_packet(packet);
}

static void req_flash_overview_all(pccomm_packet_t *packet)
{
    int i;

    for(i=0; i<SLOT_NB; i++)
    {
        //reuse incoming packet memory
        packet->header.index = i;
        req_flash_overview(packet);
    }
}

static void set_flash_description(pccomm_packet_t *packet)
{
}

static const pc_comm_rx_callback rx_callbacks[] =
{
    [FLASH_REQ_OVERVIEW] = req_flash_overview,
    [FLASH_REQ_OVERVIEW_ALL] = req_flash_overview_all,

    [FLASH_SET_DESCRIPTION] = set_flash_description,
};

static pccom_callbacks comm_callbacks =
{
    .callback_nb = SIZEOF_ARRAY(rx_callbacks),
    .callbacks = rx_callbacks
};

void user_flash_init()
{
    int ret;

    ret = spi_driver_init();
    if(ret != ERROR_NONE)
    {
        TRACE("Spi driver init error: %d\n", ret);
        return;
    }


    ret = flash_spi_init(&flash_hdl);
    if(ret != ERROR_NONE)
    {
        TRACE("FLASH driver init error: %d\n", ret);
        return;
    }

    pc_comm_register_module_callback(PCCOM_USER_FLASH, comm_callbacks);
}

