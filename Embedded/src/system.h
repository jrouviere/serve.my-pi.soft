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

#ifndef SYSTEM_H_
#define SYSTEM_H_

typedef uint16_t rctime_t;


//   rc = (FPBA / TIMER_SCALER) / freq
//=> rc = (FPBA / TIMER_SCALER) * (pulse_us/1e6)
//this operation should give the best rounding

//! TC3 clock source : fPBA/8
#define TC3_SCALER                8

//! macro to convert microsecond to timer tick TC3 clock source
//#define US_TO_TC3_TICK(time_in_us) (((APPLI_PBA_SPEED) / 1000000) * (time_in_us) / (TC3_SCALER))

//! macro to convert timer tick TC3 clock source to microsecond
//#define TC3_TICK_TO_US(time_in_tick) ((TC3_SCALER) * (time_in_tick) / ((APPLI_PBA_SPEED) / 1000000))


//! TC4 clock source : fPBA/32
#define TC4_SCALER                32

//! macro to convert microsecond to timer tick TC4 clock source
#define US_TO_TC4_TICK(time_in_us) (((APPLI_PBA_SPEED) / 1000000) * (time_in_us) / (TC4_SCALER))

//! macro to convert timer tick TC4 clock source to microsecond
#define TC4_TICK_TO_US(time_in_tick) ((TC4_SCALER) * (time_in_tick) / ((APPLI_PBA_SPEED) / 1000000))


/**
 * Reset the board into dfu bootloader mode
 */
void reset_to_bootloader();

/**
 * Check if the firmware has reset too many times
 */
void check_buggy_firmware();


#endif /* SYSTEM_H_ */
