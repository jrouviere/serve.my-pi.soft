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


#ifndef SEQUENCE_H_
#define SEQUENCE_H_

#include <stdint.h>
#include "frame_ctrl.h"

typedef struct {
    uint16_t frame_nb;
} seq_header_t;


typedef struct {
    uint8_t loaded_slot_id;    /*flash slot where sequence is currently played*/
    uint16_t seq_cur;
    uint16_t seq_item_nb;

    frame_control_t frm_ctrl;
} seq_ctrl_data_t;


/**
 * Initialize the API sequence controller.
 * The API sequence controller is used to interact with sequence
 * stored in user flash.
 */
void seq_ctrl_api_init();

/**
 * Get output from API sequence controller.
 */
void seq_ctrl_api_update(core_output_t *outputs);


#endif /* SEQUENCE_H_ */
