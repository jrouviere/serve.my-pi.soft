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
 * \file embedded_api.h
 * \brief Embedded application helper functions
 *
 * This file list all functions that are useful to develop an embedded
 * application.
 *
 * Even if an embedded application can call any functions from the
 * firmware, use other functions with extra care.
 */

#include "frame_ctrl.h"
#include "sequence.h"
#include "rc_utils.h"


/**
 * \defgroup frame_ctrl Frame controller
 * @{
 */

/**
 * Initialize frame controller structure with default value.
 *
 * \param frm_ctrl frame controller internal data
 */
void frame_ctrl_init(frame_control_t *frm_ctrl);

/**
 * Move all servos to the position indicated by the frame.
 * Frames store both the position for each servos and a time to reach
 * that position. When you load a frame with this function the current
 * time is stored and subsequent call to frame_ctrl_update will move
 * each servo at a position between original position and position
 * specified by frame.
 *
 * \param frm_ctrl frame controller internal data
 */
void frame_ctrl_load_frame(frame_control_t *frm_ctrl, const frame_t *frame);

/**
 * Stop all servo control, even if translation to loaded frame is incomplete.
 *
 * \param frm_ctrl frame controller internal data
 */
void frame_ctrl_disable(frame_control_t *frm_ctrl);

/**
 * Return true if the servos have reached loaded frame.
 *
 * \param frm_ctrl frame controller internal data
 */
bool frame_ctrl_done(frame_control_t *frm_ctrl);

/**
 * Update output position with currently loaded frame.
 *
 * \param frm_ctrl frame controller internal data
 */
void frame_ctrl_update(frame_control_t *frm_ctrl, core_output_t *outputs);

/** @} */



/**
 * \defgroup seq_ctrl Sequence controller
 * @{
 */

/**
 * Load a sequence from the flash and start it.
 * This function will initialize ctrl_data structure and load sequence
 * from specified user flash slot.
 *
 * \param ctrl_data sequence controller internal data
 * \param slot_id flash slot that will be loaded
 */
void seq_ctrl_play_flash_slot(seq_ctrl_data_t *ctrl_data, uint8_t slot_id);


/**
 * Update output position with given sequence controller.
 * Once you have created a sequence controller, you can get its last
 * values with this function.
 *
 * \param ctrl_data sequence controller internal data
 * \param[out] outputs values to be updated
 */
void seq_ctrl_update(seq_ctrl_data_t *ctrl_data, core_output_t *outputs);

/** @} */



/**
 * \defgroup math_helpers Arithmetic helpers
 * @{
 */

/**
 * Saturating addition for fixed point
 *
 * A saturating addition never overflow/underflow:
 * max + 1 => max
 * min - 1 => min
 */
dsp16_t sat_add16(dsp16_t a, dsp16_t b);

/*Some test filter functions:*/

/**
 * Return the opposite angle
 */
rc_value_t reverse_rc(rc_value_t in);


/**
 * Output 0 around the middle.
 */
rc_value_t dead_zone(rc_value_t order, rc_value_t zone);

/**
 * Increment current value with the order at a factor of k.
 */
rc_value_t integrate(rc_value_t current, rc_value_t order, dsp16_t k);

/**
 * Slow down the output. Output slowly reach goal, at a speed factor k.
 */
rc_value_t slow_down(rc_value_t current, rc_value_t goal, dsp16_t k);


/**
 * Move servo back and forth.
 * The servo start from y0, go to y1, and come back to y0.
 * t indicate the current time of the movement "timeline"
 * t=0.0: begin, t=1.0: end.
 */
rc_value_t delta_ramp(rc_value_t y1, rc_value_t y0, dsp16_t t);


/**
 * Emulation of the atan2 function as it's not implemented in the dsplib
 * Rough approximation for the moment (< 3% error)
 */
dsp16_t dsp16_op_atan2f_emu(dsp16_t y, dsp16_t x);

/** @} */


/**
 * \defgroup matrix_helpers Matrix helpers
 * @{
 */

/* A 4D matrix, we reuse OpenGL format here,
 * but we use fixed point number instead of float.
 * m0  m4  m8  m12
 * m1  m5  m9  m13
 * m2  m6  m10 m14
 * m3  m7  m11 m15
 */
typedef dsp16_t matrix4[16];

/*
 * A 3D vector, in fixed point
 * v0 : x
 * v1 : y
 * v2 : z
 */
typedef dsp16_t vector3[3];

/**
 * Initialize m with identity matrix
 */
void matrix4_identity(matrix4 m);


/**
 * Initialize m with a transformation matrix defined by:
 * Translation of dx,dy,dz
 * then rotation of rz along the z axis
 * then rotation of rx along the x axis
 * then rotation of ry along the y axis
 * \note name is reversed because it correspond to matrix multiplication order
 */
void matrix4_rotation_yxz_translation(  dsp16_t rx, dsp16_t ry, dsp16_t rz,
                                        dsp16_t dx, dsp16_t dy, dsp16_t dz,
                                        matrix4 m);

/**
 * Compute inverse of homogeneous matrix m, and place the result into r
 * This function only work with homogeneous matrix that define a combination of
 * translation and/or rotation.
 */
void matrix4_inverse_homogeneous(matrix4 m, matrix4 r);

/**
 * Compute matrix multiplication m1*m2 and store the result into r
 */
void matrix4_multiply(const matrix4 m1, const matrix4 m2, matrix4 r);

/**
 * Transform vector v with matrix m and store the result into r
 */
void matrix4_tranform_vector(const matrix4 m, const vector3 v, vector3 r);


/**
 * Print the values of a matrix on the debug output
 */
void print_matrix4(const char *name, const matrix4 m);

/**
 * Print the values of a 3d vector on the debug output
 */
void print_vector3(const char *name, const vector3 v);

/** @} */
