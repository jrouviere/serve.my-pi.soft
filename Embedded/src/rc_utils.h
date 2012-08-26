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
 * \file    rc_utils.h
 * \brief   Internal representation and manipulation of RC values
 *
 * Define data type and several common helper functions to manipulate
 * application value. These functions should be used in C application to
 * implement input/output processing.
 */


#ifndef RC_UTILS_H_
#define RC_UTILS_H_

#include "dsp.h"
#include "trace.h"

#include "pc_comm_api.h"

/**
 * RC value are represented internally by a Q1.15 fixed point number.
 *
 * Q1.15 range are [-1 .. 0.99997] with a resolution of 0.00003. That's a very
 * efficient data type for our mcu, because some operation require to store
 * temporary values in a 32 bits number.
 * This range can be converted to/from a pulse in microseconds representing the
 * actual calibrated servo angle.
 * We consider that rc_value_t range [-1.0 .. 1.0] represent a full 360 degrees
 * rotation of the servo, so it's directly mapped to the [-180 .. 180] range in
 * degrees. Most servos won't be able to achieve this range, and will reach their
 * limits around -0.5 and 0.5.
 */
typedef dsp16_t rc_value_t;


/**
 * Raw value measured by the hardware
 *
 * The actual unit or meaning of the data doesn't really matter,
 * we only assume that rc_raw_t has a lower resolution than rc_value_t.
 */
typedef int16_t rc_raw_t;
#define RC_RAW_MIN        RC_VALUE_MIN
#define RC_RAW_MAX        RC_VALUE_MAX

/*for some reason the min/max from ASF are not working for me*/
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

/**
 * \name Shorter notation for convenience
 */
/// @{
#define cos16 dsp16_op_cos
#define sin16 dsp16_op_sin
#define mul16 dsp16_op_mul
#define Q DSP16_Q
/// @}


#define DSP16_PI            (DSP_Q_MAX(DSP16_QA, DSP16_QB))
#define DSP16_PI_2          (DSP16_Q(0.5)-1)
#define DSP16_PI_4          (DSP16_Q(0.25)-1)
#define DSP16_MINUS_PI      (DSP_Q_MIN(DSP16_QA, DSP16_QB))
#define DSP16_MINUS_PI_2    (DSP16_Q(-0.5))


typedef struct {
    dsp16_t t;
    rc_value_t y;
} sequence_point_t;

#define END_OF_SEQ          {.t=-1, .y=-1}





/**
 * Return the command corresponding to the current value
 * following a timing diagram defined by several points.
 *
 * \param pts list of diagram points
 * \param pts_sz size of the diagram points array
 * \param dt current timing in the diagram fixed point: [0..1]
 * \param amp amplitude to apply to the diagram
 */
rc_value_t execute_sequence(const sequence_point_t *pts, dsp16_t dt, rc_value_t amp);



/// @}

#endif /* RC_UTILS_H_ */

