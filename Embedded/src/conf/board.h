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


#ifndef _OPENSCB_BOARD_H_
#define _OPENSCB_BOARD_H_

#include "compiler.h"
#include <stdint.h>


/*! \name Oscillator Definitions
 */
//! @{

// RCOsc has no custom calibration by default. Set the following definition to
// the appropriate value if a custom RCOsc calibration has been applied to your
// part.
#define FOSC32          32768                                 //!< Osc32 frequency: Hz.
#define OSC32_STARTUP   AVR32_PM_OSCCTRL32_STARTUP_8192_RCOSC //!< Osc32 startup time: RCOsc periods.

#define FOSC0           12000000                              //!< Osc0 frequency: Hz.
#define OSC0_STARTUP    AVR32_PM_OSCCTRL0_STARTUP_2048_RCOSC  //!< Osc0 startup time: RCOsc periods.


//! @}

#define USER_PAGE_ORIG    0x80800000

#define APPLI_CPU_SPEED   60000000
#define APPLI_PBA_SPEED   60000000

/*! \name USB Definitions
 */
//! @{

//! Multiplexed pin used for USB_ID: AVR32_USBB_USB_ID_x_x.
//! To be selected according to the AVR32_USBB_USB_ID_x_x_PIN and
//! AVR32_USBB_USB_ID_x_x_FUNCTION definitions from <avr32/uc3bxxxx.h>.
#define USB_ID                      AVR32_USBB_USB_ID_0_0

//! Multiplexed pin used for USB_VBOF: AVR32_USBB_USB_VBOF_x_x.
//! To be selected according to the AVR32_USBB_USB_VBOF_x_x_PIN and
//! AVR32_USBB_USB_VBOF_x_x_FUNCTION definitions from <avr32/uc3bxxxx.h>.
#define USB_VBOF                    AVR32_USBB_USB_VBOF_0_0

//! Active level of the USB_VBOF output pin.
#define USB_VBOF_ACTIVE_LEVEL       LOW

//! USB overcurrent detection pin.
#define USB_OVERCURRENT_DETECT_PIN  AVR32_PIN_PA20

//! @}


/*! \name GPIO Definitions
 */
//! @{

#define SCB_PIN_LED_ERROR   AVR32_PIN_PB11
#define PPM_INPUT_PIN       AVR32_PIN_PB10

static const uint8_t IO_BB_PINS[] = {
        AVR32_PIN_PA22,
        AVR32_PIN_PA21,
        AVR32_PIN_PA20,
        AVR32_PIN_PA16,
        AVR32_PIN_PA15,

        AVR32_PIN_PA14,
        AVR32_PIN_PA12,
        AVR32_PIN_PA11,
        AVR32_PIN_PA10,
        AVR32_PIN_PA09,

        AVR32_PIN_PA31,
        AVR32_PIN_PA30,
        AVR32_PIN_PA08,
        AVR32_PIN_PA07,
        AVR32_PIN_PA06,

        AVR32_PIN_PA05,
        AVR32_PIN_PA04,
        AVR32_PIN_PA03,
        AVR32_PIN_TMS,
        AVR32_PIN_TDO,

        AVR32_PIN_TDI,
        AVR32_PIN_PA27,
        AVR32_PIN_PA26,
        AVR32_PIN_PA25,
 };

#define IO_BB_MAX_NB    (sizeof(IO_BB_PINS)/sizeof(uint8_t))

//! @}


/*! \name DMA Definitions
 */
//! @{
#define DMA_CHANNEL_SPI_RX     (0)
#define DMA_CHANNEL_SPI_TX     (1)
//! @}


/**
 * \defgroup error_codes Error codes definitions
 * @{
 */
/**
 * \defgroup error_codes_common General error codes definitions
 * @{
 */
/** No error */
#define ERROR_NONE                                      (0)
/** Memory allocation error */
#define ERROR_OS_COULD_NOT_ALLOCATE_REQUIRED_MEMORY     errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY
/** No task to run error */
#define ERROR_OS_NO_TASK_TO_RUN                         errNO_TASK_TO_RUN
/** Queue block request error */
#define ERROR_OS_QUEUE_BLOCKED                          errQUEUE_BLOCKED
/**  */
#define ERROR_OS_QUEUE_YIELD                            errQUEUE_YIELD
/** OS Queue empty error */
#define ERROR_OS_QUEUE_EMPTY                            errQUEUE_EMPTY
/** OS Queue full error */
#define ERROR_OS_QUEUE_FULL                             errQUEUE_FULL
/** Parameter range error */
#define ERROR_BAD_PARAMETER                             (-7)

/**
 * \defgroup error_codes_spi_driver SPI Driver error codes definitions
 * @{
 */
/** SPI driver not initialized properly error */
#define ERROR_SPI_DRIVER_NOT_INITIALIZED    (-100)
/** SPI driver operation ended with a timeout */
#define ERROR_SPI_DRIVER_TIMEOUT            (-101)
/** A DMA error was raised during the SPI transfer */
#define ERROR_SPI_DRIVER_DMA                (-102)
/** The flash device could not be unlocked */
#define ERROR_FLASH_DRIVER_DEVICE_LOCKED    (-200)
/** The flash device could not be unlocked */
#define ERROR_FLASH_DRIVER_DEVICE_TIMEOUT   (-201)
/** @} */
/** @} */

#endif  // _OPENSCB_BOARD_H_
