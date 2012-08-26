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
 * \file rx_input.h
 * \brief Low level handling of RC receiver and PPM signal
 *
 * This module measure RC signal pulse width coming from a RC receiver or any
 * similar device. It can also measure pulse width of PPM signal which are
 * very similar.
 *
 * \note Functions are not thread-safe so they should not be called by
 * different threads to avoid race conditions.
 */


#ifndef RX_INPUT_H_
#define RX_INPUT_H_

#include "rc_utils.h"
#include "core.h"


/**
 * Initialize ppm_input module
 */
void ppm_input_init();

/**
 * \brief Get the latest value of one channel in the PPM signal
 * \param signal_no number of the channel you need to read
 * \param[out] value latest measured value in timer tick
 */
void ppm_input_get_value(uint8_t channel_no, core_input_t *out_val);


/**
 * \brief Process data from interrupt handler
 * This function is shared between rx_input and ppm_input functions.
 * \note This function must be called periodically to empty the internal
 * FIFO used by the interrupt handler.
 */
void rx_ppm_input_process_input();


extern const io_class_t ppm_input_class;

#endif /* RX_INPUT_H_ */
