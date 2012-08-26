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

#ifndef TRACE_H_
#define TRACE_H_

#define TRACE_DEBUG

#include "gpio.h"

//! Prints a debug line, new line will be added automatically
//! printf-like argument will only be displayed if TRACE_DEBUG is defined
#define TRACE(a, ...) do {              \
    trace(a, ## __VA_ARGS__); \
} while(0)


//! Helper function that prints a message with printf-like format
//! \note you shouldn't call that function directly, use TRACE macro instead
void trace(const char * format, ...);


//! Initialize module
void trace_init(void);

/**
 * Blink a led to signal the user something went wrong in the initialization
 * TODO: get what has been done for exception, maybe generate a NMI here
 */
#define FATAL_ERROR() do {                  \
    gpio_local_set_gpio_pin(SCB_PIN_LED_ERROR); \
} while(1)

#define ASSERT(must_be_true) if(!(must_be_true)) FATAL_ERROR()

#endif /* TRACE_H_ */
