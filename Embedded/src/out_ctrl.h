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

#ifndef OUT_CTRL_H_
#define OUT_CTRL_H_


#include "io.h"
#include "sys_conf.h"


/**
 * Gather output values from the different system modules
 * \param sys_conf system configuration
 * \param inputs list of inputs value (used for application)
 * \param outputs list of outputs value to be filled in
 */
void out_ctrl_get_value(const system_conf_t *sys_conf, const core_input_t *inputs, core_output_t *outputs);



#endif /* OUT_CTRL_H_ */
