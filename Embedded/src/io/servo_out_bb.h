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
 * \file    servo_out_bb.h
 * \brief   Low level software generation (bit-banging) of RC servo signal
 *
 * This module generate several servo signals (1-2 ms pulse). These code can be
 * used to control a servo, an ESC or any device that RC receiver can control.
 * This module only use a timer channel and GPIOs, it doesn't require any
 * particular hardware and can control a virtually unlimited number of outputs.
 *
 * \note Functions are not thread-safe so they should not be called by
 * different threads to avoid race conditions.
 */


#ifndef SERVO_OUT_BB_H_
#define SERVO_OUT_BB_H_

#include "rc_utils.h"
#include "core.h"

//! default servo pulse width
#define DEFAULT_OUTPUT_RAW_VALUE         US_TO_TC4_TICK(1520)


/**
 * Initialize servo_bb module
 */
void servobb_module_init(void);

/**
 * Configure a channel
 */
void servobb_channel_init(uint8_t channel_no);

/**
 * Unconfigure a channel
 */
void servobb_channel_cleanup(uint8_t channel_no);

/**
 * Cleanup servo_bb module
 */
void servobb_module_cleanup(void);

/**
 * Get pulse period set on this channel
 *
 * \param channel_no Servo number
 * \param[out] out_val Pulse width in timer tick + active state
 */
void servobb_get_value(uint8_t channel_no, core_input_t *out_val);

/**
 * Store desired pulse period in the corresponding structure.
 *
 * \note You need to call servobb_apply_values() for this new value to take effect.
 * \param channel_no Servo number
 * \param out Pulse width in timer tick + active state
 */
void servobb_set_value(uint8_t channel_no, const core_output_t *out);

/**
 * Apply values set with previous calls to servobb_set_value()
 *
 * The function choose one of the double buffer, and swap pointers atomically.
 * \return True if the operation was successful
 * \return False if the operation failed, you can't set new values if some new values are still pending
 */
bool servobb_apply_values();


extern const io_class_t servobb_output_class;



#endif /* SERVO_OUT_BB_H_ */
