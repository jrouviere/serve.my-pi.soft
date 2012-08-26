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


#ifndef FRAME_CTRL_H_
#define FRAME_CTRL_H_

#include "out_ctrl.h"
#include "pc_comm_api.h"

typedef struct {
    frame_t previous;
    frame_t next;

    uint32_t duration;
    uint32_t start_time;

    bool done;
} frame_control_t;


/**
 * Initialize the API frame controller.
 * API frame controller is used to load a frame from API.
 */
void frame_ctrl_api_init();

/**
 * Get values from API frame controller.
 */
void frame_ctrl_api_update(core_output_t *outputs);



#endif /* FRAME_CTRL_H_ */
