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
 * \file api_io.h
 * \brief IO driven from the API
 *
 * This module define inputs that take their values from a remote controller
 * and outputs that are sent to it. This is useful for debugging, or to send
 * input to the board.
 * If you are looking for a very simple way to remote control the outputs of
 * the board, you might need to use the API controller.
 *
 */


#ifndef API_IO_H_
#define API_IO_H_

#include "rc_utils.h"
#include "core.h"

/**
 * Init api io module
 */
void api_io_init();

/**
 * Cleanup api io module
 */
void api_io_cleanup();

/**
 * Reset a channel
 */
void api_io_channel_init(uint8_t channel_no);

/**
 * Get an input value set by a remote host via the API
 */
void api_input_get_value(uint8_t channel_no, core_input_t *out_val);

/**
 * Set an output value, will be sent to a remote host via the API
 */
void api_output_set_value(uint8_t channel_no, const core_output_t *val);


extern const io_class_t api_io_class;


#endif /* API_IO_H_ */
