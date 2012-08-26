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


#ifndef SYS_CONF_H_
#define SYS_CONF_H_

#include "io.h"
#include "pc_comm_api.h"


#define SIZEOF_ARRAY(x) sizeof(x)/sizeof(x[0])


typedef struct {
    input_calib_data_t calib;
    char name[IO_NAME_LEN];
    uint8_t mapping;  //logical input => physical input
} input_conf_t;

typedef struct {
    output_calib_data_t calib;
    char name[IO_NAME_LEN];
    uint8_t max_speed; //maximum speed in SPEED_COEF/2^16 per refresh period
} output_conf_t;


typedef struct {
    uint32_t compatibility_magic;

    uint8_t input_nb;
    input_conf_t in_conf[MAX_IN_NB];

    uint8_t output_nb;
    output_conf_t out_conf[MAX_SERVO_NB];

    uint32_t servo_active_bm;
} system_conf_t;



/**
 * Create and initialize an empty configuration
 */
void init_sys_conf();

/**
 * Load configuration from the user flash
 */
void load_sys_conf();


/**
 * Copy system configuration to the given structure
 */
void sys_conf_copy(system_conf_t *sys_conf);


/**
 * Set an input calibration structure
 */
void sys_conf_set_input_calib(uint8_t input_nb, const input_calib_data_t *calib);

/**
 * Reset all inputs calibration structure for automatic calibration
 */
void sys_conf_reset_input_calib();

#endif /* SYS_CONF_H_ */
