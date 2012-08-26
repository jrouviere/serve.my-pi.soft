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

#ifndef USER_FLASH_H_
#define USER_FLASH_H_

#include <stdint.h>
#include "pc_comm_api.h"



int flash_erase_slot(uint8_t slot_id);
void flash_init_slot(uint8_t slot_id, SLOT_TYPE type, const char *desc, uint32_t size);


void flash_load_slot(uint8_t slot_id, uint32_t offset, uint32_t size, void *buffer);
void flash_store_slot(uint8_t slot_id, uint32_t offset, uint32_t size, const void *buffer);


int flash_check_slot_type(uint8_t slot_id, SLOT_TYPE type);


void user_flash_init();


#endif /* USER_FLASH_H_ */
