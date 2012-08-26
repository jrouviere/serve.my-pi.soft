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
/**
 * \file core.h
 * \brief Application mode
 *
 * Main engine and mode helper functions.
 */


#ifndef CORE_H_
#define CORE_H_

#include "rc_utils.h"

#include "io.h"
#include "sys_conf.h"



/**
 * Get class of input used by core module (use with care)
 *
 */
const io_class_t *core_get_input_type(void);

/**
 * Get class of output used by core module (use with care)
 *
 */
const io_class_t *core_get_output_type(void);

/**
 * Get system configuration used by core.
 * This is a copy from actual system configuration so it
 * should be used in read-only.
 *
 */
const system_conf_t *core_get_sys_conf(void);


/**
 * Get current value/state of output
 *
 */
const core_output_t *core_get_output(int i);


/**
 * Module initialization
 */
void core_main_init();


#endif /* CORE_H_ */
