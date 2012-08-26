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

#ifndef IO_H_
#define IO_H_


#include "rc_utils.h"


/** \name IO type definitions */
/// @{

/** Input type definitions */
typedef struct _core_input_t
{
    bool        active;
    rc_value_t  value;
} __attribute__((packed)) core_input_t;

/** Output type definitions */
typedef struct _core_output_t
{
    bool        active;
    rc_value_t  value;
} __attribute__((packed)) core_output_t;

/*input class callbacks*/
typedef void (*io_init_module_cb)(void);
typedef void (*io_init_channel_cb)(uint8_t channel_no);
typedef void (*io_cleanup_channel_cb)(uint8_t channel_no);
typedef void (*io_cleanup_module_cb)(void);
typedef void (*io_pre_cb)(void);
typedef void (*io_get_cb)(uint8_t channel_no, core_input_t *out_val);
typedef void (*io_set_cb)(uint8_t channel_no, const core_output_t *val);
typedef bool (*io_post_cb)(void);

/**IO class definition*/
typedef struct
{
    const char *name;

    io_init_module_cb init_module;
    io_init_channel_cb init_channel;
    io_cleanup_channel_cb cleanup_channel;
    io_cleanup_module_cb cleanup_module;

    io_pre_cb pre;
    io_get_cb get;
    io_set_cb set;
    io_post_cb post;
} io_class_t;


/// @}


#endif /* IO_H_ */
