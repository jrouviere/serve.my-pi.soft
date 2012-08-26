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

#ifndef API_CTRL_H_
#define API_CTRL_H_

#include "out_ctrl.h"

/**
 * Initialize API controller.
 * The API controller is used to apply value to each output
 * of the system. It can be used to test connection with the GUI
 * or to control the board from a remote program.
 */
void api_ctrl_init(void);


/**
 * Get value from API controller.
 */
void api_ctrl_update(core_output_t *outputs);

#endif /* API_CTRL_H_ */
