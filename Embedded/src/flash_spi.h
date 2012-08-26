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
 * \file    flash_spi.h
 * \brief   SST25 series SPI flash driver
 *
 * Define a basic set of function for accessing the on-board flash SPI device.
 */

#ifndef FLASH_SPI_H_
#define FLASH_SPI_H_

#include <stdint.h>

/*--------------------------------------------------------------------------------------------------
 *  MACRO DEFINITIONS
 *------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------------------
 *  TYPE DEFINITIONS
 *------------------------------------------------------------------------------------------------*/
/** Flash device status definition */
typedef enum
{
    FLASH_SPI_STATUS_READY = 0u,
    FLASH_SPI_STATUS_BUSY = 1u,
    FLASH_SPI_STATUS_LOCKED = 2u,
    FLASH_SPI_STATUS_DEVICE_ERROR = 3u
} flash_spi_status_t;

/** Flash device handler definition */
typedef void * flash_spi_hdl_t;

/*--------------------------------------------------------------------------------------------------
 *  EXTERNAL API PROTOTYPES
 *------------------------------------------------------------------------------------------------*/

/**
 * Initialize the SPI driver:
 *   - Configure the SPI chip select line corresponding to the on-board flash device
 *   - Unlock the flash device (by default it is locked at boot-up)
 *
 * \param[out] device_hdl Pointer to a flash spi handler that is needed for every access to this device
 *
 * \return Flash SPI error code
 *
 * \warning The SPI driver must have been initialized prior to calling this function
 *          (-> \ref spi_driver_init)
 */
int flash_spi_init(flash_spi_hdl_t * device_hdl);


/**
 * Read data from the flash device
 *
 * \param[in] device_hdl Pointer to the Flash device handler corresponding to the target device
 * \param[out] buffer_ptr Buffer used to store the read data from the flash device
 * \param[in] size Number of data bytes to be read
 * \param[in] address Read start address
 *
 * \return Flash SPI error code
 */
int flash_spi_read(flash_spi_hdl_t device_hdl, uint8_t * buffer_ptr, uint32_t size, uint32_t address);


/**
 * Write data to the flash device
 *
 * \param[in] device_hdl Pointer to the Flash device handler corresponding to the target device
 * \param[in] buffer_ptr Buffer used to write data to the flash device
 * \param[in] size Number of data bytes to be written
 * \param[in] address Write start address
 *
 * \return Flash SPI error code
 */
int flash_spi_write(flash_spi_hdl_t device_hdl, const uint8_t * buffer_ptr, uint32_t size, uint32_t address);


/**
 * Erase a sector of the flash device
 *
 * \param[in] device_hdl Pointer to the Flash device handler corresponding to the target device
 * \param[in] address Address of the sector to be erased
 *
 * \return Flash SPI error code
 */
int flash_spi_erase_sector(flash_spi_hdl_t device_hdl, uint32_t address);


/**
 * Determine the real range of address that would be erased if a erase command was requested on
 * the given base address.
 *
 * \param[in] device_hdl Pointer to the Flash device handler corresponding to the target device
 * \param[in] address Base address to determine the erase range
 * \param[out] start_ptr Pointer to return the start address of the erase range
 * \param[out] end_ptr Pointer to return the end address of the erase range
 *
 * \return Flash SPI error code
 */
int flash_spi_get_erase_range(flash_spi_hdl_t device_hdl, uint32_t address, uint32_t * start_ptr,
                              uint32_t * end_ptr);


/**
 * Read the status of the flash device.
 *
 * \param[in] device_hdl Pointer to the Flash device handler corresponding to the target device
 * \param[out] status_ptr Pointer to return the status of the flash device
 *
 * \return Flash SPI error code
 */
int flash_spi_get_status(flash_spi_hdl_t device_hdl, flash_spi_status_t * status_ptr);


/**
 * Read data from the flash device
 *
 * \param[in] device_hdl Pointer to the Flash device handler corresponding to the target device
 * \param[out] device_id_ptr Pointer to return the device ID of the flash device
 *
 * \return Flash SPI error code
 */
int flash_spi_get_device_id(flash_spi_hdl_t device_hdl, uint16_t * device_id_ptr);

#endif /* FLASH_SPI_H_ */

