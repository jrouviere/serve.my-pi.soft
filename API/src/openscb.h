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


#ifndef OPENSCB_H_
#define OPENSCB_H_

#include "pc_comm_api.h"
#include "openscb_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * TODO: need a more convenient function to set several random outputs
 * maybe active + output, like what is used for loading frames
 *
 * TODO: need a function to find an output by name
 *
 * TODO: need a goal reached function (ie: bool is_servo_moving(int nb))
 *
 */


/**
 * Convert degrees to servo goal value
 *
 * \param degrees [-360..360]
 * \ret goal value [-1..1]
 */
float scb_degrees_to_goal(float degrees);

/**
 * Convert servo goal value to degrees
 *
 * \param goal value [-1..1]
 * \ret radian [-360..360]
 */
float scb_goal_to_degrees(float goal);

/**
 * Convert radian to servo goal value
 *
 * \param radian [-Pi..Pi]
 * \ret goal value [-1..1]
 */
float scb_radian_to_goal(float radian);

/**
 * Convert servo goal value to radian
 *
 * \param goal value [-1..1]
 * \ret radian [-Pi..Pi]
 */
float scb_goal_to_radian(float goal);


/**
 * Send goal value for one output of the board.
 * 
 * The logical output of the board will move to it's new goal position
 * according to configured maximum speed.
 *
 * \param dev handle to openscb device
 * \param out_id output number
 * \param output goal value, float format [-1..1]
 * \return <0 on error
 */
int scb_set_one_output_goal(openscb_dev dev, uint8_t out_id, float output);

/**
 * Send output goal values to the board
 * 
 * Set goal values for all output of the board, this does the same than
 * calling scb_set_one_output_goal several times except that only the
 * minimum amount of message is sent to the board.
 *
 * \param dev handle to openscb device
 * \param outputs array of output values, float format [-1..1]
 * \param nb number of element in the outputs array
 * \return <0 on error
 */
int scb_set_output_goal(openscb_dev dev, float *outputs, int nb);

/**
 * Send output max speed to the board
 *
 * \param dev handle to openscb device
 * \param speeds array of speed values, float format [-1..1]
 * \param nb number of element in the speed array
 * \return <0 on error
 */
int scb_set_out_speed(openscb_dev dev, uint8_t *speeds, int nb);

/**
 * Send output max acceleration values to the board
 *
 * \param dev handle to openscb device
 * \param accel array of accel values, in float format [-1..1]
 * \param nb number of element in the accel array
 * \return <0 on error
 */
int scb_set_out_accel(openscb_dev dev, uint8_t *accel, int nb);


/**
 * Get number of configured output
 *
 * \param dev handle to openscb device
 * \param nb[out] number returned by the board
 * \return <0 on error
 */
int scb_get_output_nb(openscb_dev dev, uint8_t *nb);

/**
 * Get current value of outputs
 *
 * \param dev handle to openscb device
 * \param outputs[out] list of output values
 * \param nb number of output to get from the board
 * (function will timeout if not all values can be read)
 * \return <0 on error
 */
int scb_get_output_values(openscb_dev dev, float *outputs, uint8_t nb);

/**
 * Get current value of max speed for each output
 *
 * \param dev handle to openscb device
 * \param speeds[out] list of speed values
 * \param nb number of values to get from the board
 * (function will timeout if not all values can be read)
 * \return <0 on error
 */
int scb_get_output_speed(openscb_dev dev, uint8_t *speeds, uint8_t nb);

/**
 * Get current value of acceleration for each output
 *
 * \param dev handle to openscb device
 * \param outputs[out] list of accel values
 * \param nb number of values to get from the board
 * (function will timeout if not all values can be read)
 * \return <0 on error
 */
int scb_get_output_accel(openscb_dev dev, uint8_t *accel, uint8_t nb);


/**
 * Load a frame on the board, and move servo to corresponding positions
 *
 * \param dev handle to openscb device
 * \param duration time until the frame is reached
 * \param position list of position value for each output
 * \param active whether each output is enabled or not
 * \param nb size of "position" and "active" array
 * \return <0 on error
 */
int scb_load_frame(openscb_dev dev, uint32_t duration, const float *position,
        const bool *active, uint8_t nb);

/**
 * Stop and cancel currently loaded frame (in move or not)
 *
 * \param dev handle to openscb device
 * \return <0 on error
 */
int scb_disable_frame(openscb_dev dev);


/**
 * Load the sequence from flash slot and play it
 *
 * \param dev handle to openscb device
 * \param slot_id what flash space need to be loaded
 * \return <0 on error
 */
int scb_play_sequence_slot(openscb_dev dev, uint8_t slot_id);


/**
 * Enable or disable output globally
 *
 * \param dev handle to openscb device
 * \param enabled does the output need to be enabled or disabled?
 * \param nb number of value in enabled array
 * \return <0 on error
 */
int scb_set_out_enabled(openscb_dev dev, const bool *enabled, uint8_t nb);

/**
 * Get global enable/disable state of output
 *
 * \param dev handle to openscb device
 * \param enabled is the output enabled?
 * \param nb number size of enabled array
 * \return <0 on error
 */
int scb_get_out_enabled(openscb_dev dev, bool *enabled, uint8_t nb);


/**
 * Enable or disable output control from API
 *
 * \param dev handle to openscb device
 * \param controlled does the output need to be controlled by api?
 * \param nb number of value in controlled array
 * \return <0 on error
 */
int scb_set_out_controlled(openscb_dev dev, const bool *controlled, uint8_t nb);


/**
 * Get the version of the firmware
 *
 * \param dev handle to openscb device
 * \param compatible[out] true if API and firmware are compatible
 * \param version[out] buffer to store the version string
 * \param size length of the buffer in byte
 * \return <0 on error
 */
int scb_check_firmware_version(openscb_dev dev, bool *compatible, char *version, int size);


#ifdef __cplusplus
}
#endif

#endif /* OPENSCB_H_ */
