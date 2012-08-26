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
 * \file calibration.h
 * \brief Application calibration mode
 *
 * Main calibration procedure and helper functions to use calibration
 * data.
 */


#ifndef CORE_CALIBRATION_H_
#define CORE_CALIBRATION_H_

#include "rc_utils.h"
#include "sys_conf.h"


/**
 * Module initialization
 */
void core_calib_init(void);

/**
 * Initialize output calibration structure with default value
 */
void output_calib_init_default(output_calib_data_t *out_calib);

/**
 * Initialize input calibration structure with default value
 */
void input_calib_init_default(input_calib_data_t *in_calib);


/**
 * Initialize input calibration values with incorrect value so they
 * can be calibrated with automatic bound detection.
 */
void input_calib_reset_for_calib(input_calib_data_t *in_calib);

/**
 * compute rc_value_t from a raw input using calibration data
 *
 * \param calib calibration structure
 * \param raw value to convert
 * \return value transformed in a more convenient data format
 */
rc_value_t input_calib_raw_to_rc(const input_calib_data_t *calib, rc_raw_t raw);

/**
 * compute raw output from a rc_value_t data
 *
 * \param calib calibration structure
 * \param rc value to convert
 * \return value in raw format
 */
rc_raw_t output_calib_rc_to_raw(const output_calib_data_t *calib, rc_value_t rc);


/**
 * Function to be called from main core loop to calibrate inputs
 *
 * \param sys_conf core system configuration
 */
void input_calib_core_run(const system_conf_t *sys_conf);

/**
 * Function to call from core to help user calibrate
 *
 * \param sys_conf core system configuration
 */
void output_calib_core_run(const system_conf_t *sys_conf);

#endif /* CORE_CALIBRATION_H_ */
