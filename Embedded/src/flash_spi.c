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
 * \file    flash_spi.c
 * \brief   SST25 series SPI flash driver
 *
 * Define a basic set of function for accessing the on-board flash SPI device.
 */

#include "board.h"
#include "FreeRTOS.h"

#include "flash_spi.h"
#include "spi_driver.h"

#include "trace.h"
#include <string.h>
#include "task.h"


/*--------------------------------------------------------------------------------------------------
 *  MACRO DEFINITIONS
 *------------------------------------------------------------------------------------------------*/
/* Flash commands */
#define COMMAND_READ                    (0x03)  /**< Read memory command */
#define COMMAND_SECTOR_ERASE            (0x20)  /**< 4KB sector erase command */
#define COMMAND_32K_BLOCK_ERASE         (0x52)  /**< 32KB block erase command */
#define COMMAND_64K_BLOCK_ERASE         (0xD8)  /**< 64kB block erase command */
#define COMMAND_CHIP_ERASE              (0x60)  /**< Full chip erase command */
#define COMMAND_BYTE_PROGRAM            (0x02)  /**< Single byte program command */
#define COMMAND_WORD_PROGRAM            (0xAD)  /**< Auto address increment word program command */
#define COMMAND_READ_STATUS             (0x05)  /**< Status register read command */
#define COMMAND_WRITE_STATUS_ENABLE     (0x50)  /**< Status register write enable command */
#define COMMAND_WRITE_STATUS            (0x01)  /**< Status register write command */
#define COMMAND_WRITE_ENABLE            (0x06)  /**< Device write enable command */
#define COMMAND_WRITE_DISABLE           (0x04)  /**< Device write disable command */
#define COMMAND_READ_ID                 (0x90)  /**< Device ID read command */

/* Flash status register */
#define STATUS_REG_BUSY                 (0x01)  /**< Status register: Busy bit */
#define STATUS_REG_WRITE_ENABLED        (0x02)  /**< Status register: Write enabled bit */
#define STATUS_REG_BLOCK_PROTECT_0      (0x04)  /**< Status register: Block protect 0 bit */
#define STATUS_REG_BLOCK_PROTECT_1      (0x08)  /**< Status register: Block protect 1 bit */
#define STATUS_REG_BLOCK_PROTECT_2      (0x10)  /**< Status register: Block protect 2 bit */
#define STATUS_REG_BLOCK_PROTECT_3      (0x20)  /**< Status register: Block protect 3 bit */
#define STATUS_REG_AUTO_ADDRESS_INC     (0x40)  /**< Status register: Auto address increment bit */
#define STATUS_REG_BLOCK_PROTECT_LOCK   (0x80)  /**< Status register: Block protect lock-down bit */

/* SST25 4MBit device definitions */
#define SST25_40_SECTOR_SIZE            (0x1000u)   /**< SST25VF040 sector size */
#define SST25_40_SECTOR_NB              (128u)      /**< SST25VF040 number of sectors */
#define SST25_40_CHIP_SIZE              (0x80000u)  /**< SST25VF040 chip size */

/** SPI Link baudrate */
#define SPI_OPTION_BAUDRATE             (1000000u)
/** SPI Link CS corresponding to the onboard SST25 flash device */
#define SPI_OPTION_CS_NB                (1u)

/** Max time to wait for the flash device to be ready (in OS ticks) */
#define FLASH_SPI_MAX_WAIT_READY_TIME   (50u)


/*--------------------------------------------------------------------------------------------------
 *  TYPE DEFINITIONS
 *------------------------------------------------------------------------------------------------*/

/** Flash device descriptor structure */
typedef struct
{
    spi_driver_hdl_t spi_handler_ptr;   /**< SPI driver handler corresponding to the device */
    uint32_t sector_size;               /**< Sector size in bytes */
    uint32_t sector_nb;                 /**< Number of sectors */
    uint32_t chip_size;                 /**< Device size in bytes */
} flash_spi_device_t;


/*--------------------------------------------------------------------------------------------------
 *  GLOBAL VARIABLES
 *------------------------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------------------------
 *  LOCAL FUNCTIONS PROTOTYPES
 *------------------------------------------------------------------------------------------------*/
static int flash_spi_write_enable(flash_spi_device_t * device_ptr);
static int flash_spi_write_disable(flash_spi_device_t * device_ptr);
static int flash_spi_unlock(flash_spi_device_t * device_ptr);
static int flash_spi_read_status(flash_spi_device_t * device_ptr, uint8_t * status_reg_ptr);
static int flash_spi_wait_till_ready(flash_spi_device_t * device_ptr);


/*--------------------------------------------------------------------------------------------------
 *  EXTERNAL API FUNCTIONS
 *------------------------------------------------------------------------------------------------*/
int flash_spi_init(flash_spi_hdl_t * device_hdl)
{
    int err;
    static flash_spi_device_t onboard_sst25_device = {spi_handler_ptr: NULL,
                                                      sector_size: SST25_40_SECTOR_SIZE,
                                                      sector_nb: SST25_40_SECTOR_NB,
                                                      chip_size: SST25_40_CHIP_SIZE};

    err = spi_driver_configure_device(SPI_OPTION_CS_NB, SPI_OPTION_BAUDRATE,
                                      SPI_DRIVER_REG_CS_BITS_8 | SPI_DRIVER_REG_CS_NCPHA,
                                      &(onboard_sst25_device.spi_handler_ptr));

    if (err != ERROR_NONE)
    {
        *device_hdl = NULL;
        return err;
    }

    *device_hdl = (flash_spi_hdl_t *) &onboard_sst25_device;

    /* At boot-up, flash is write-protected.
     * In order to be able to modify its content, we need to disable this protection */
    err = flash_spi_unlock(&onboard_sst25_device);
    if (err != ERROR_NONE)
    {
        *device_hdl = NULL;
        return err;
    }

    return ERROR_NONE;
}

int flash_spi_read(flash_spi_hdl_t device_hdl, uint8_t * buffer_ptr, uint32_t size, uint32_t address)
{
    flash_spi_device_t * device_ptr = (flash_spi_device_t *) device_hdl;
    uint8_t tx_buf[4];
    int err = ERROR_NONE;

    /* Check parameters */
    if ((address >= device_ptr->chip_size) ||
        ((device_ptr->chip_size - address) < size))
    {
        return ERROR_BAD_PARAMETER;
    }


    /* Clear buffers */
    memset((uint8_t *) buffer_ptr, 0x00, size);

    /* Prepare tx buffer */
    tx_buf[0] = COMMAND_READ;
    tx_buf[1] = (address & 0x00FF0000) >> 16;
    tx_buf[2] = (address & 0x0000FF00) >> 8;
    tx_buf[3] = (address & 0x000000FF);

    /* Launch SPI action */
    err = spi_driver_xfer(device_ptr->spi_handler_ptr, tx_buf, 4u, buffer_ptr, size, 4u);

    return err;
}

int flash_spi_write(flash_spi_hdl_t device_hdl, const uint8_t * buffer_ptr, uint32_t size,
                    uint32_t address)
{
    flash_spi_device_t * device_ptr = (flash_spi_device_t *) device_hdl;
    uint8_t tx_buf[6];
    int err = ERROR_NONE;

    /* Check parameters */
    if ((address >= device_ptr->chip_size) ||
        ((device_ptr->chip_size - address) < size))
    {
        return ERROR_BAD_PARAMETER;
    }

    if (size > 1u)
    {
        /* Execute a write enable command first */
        err = flash_spi_write_enable(device_ptr);
        if (err != ERROR_NONE)
        {
            return err;
        }

        /* Prepare initial auto-increment write sequence */
        tx_buf[0] = COMMAND_WORD_PROGRAM;
        tx_buf[1] = (address & 0x00FF0000) >> 16;
        tx_buf[2] = (address & 0x0000FF00) >> 8;
        tx_buf[3] = (address & 0x000000FF);
        tx_buf[4] = (*buffer_ptr);
        buffer_ptr++;
        tx_buf[5] = (*buffer_ptr);
        buffer_ptr++;
        size -= 2;
        address += 2;

        /* Launch SPI action */
        err = spi_driver_xfer(device_ptr->spi_handler_ptr, tx_buf, 6u, NULL, 0u, 6u);
        if (ERROR_NONE != err)
        {
            return err;
        }

        while(size > 1u)
        {
            err = flash_spi_wait_till_ready(device_ptr);
            if (ERROR_NONE != err)
            {
                return err;
            }
            tx_buf[1] = (*buffer_ptr);
            buffer_ptr++;
            tx_buf[2] = (*buffer_ptr);
            buffer_ptr++;
            size -= 2;
            address += 2;

            err = spi_driver_xfer(device_ptr->spi_handler_ptr, tx_buf, 3u, NULL, 0u, 3u);
            if (ERROR_NONE != err)
            {
                return err;
            }
        }

        (void) flash_spi_wait_till_ready(device_ptr);
        flash_spi_write_disable(device_ptr);
    }

    if (size == 1u)
    {
        /* Execute a write enable command first */
        err = flash_spi_write_enable(device_ptr);
        if (ERROR_NONE != err)
        {
            return err;
        }

        /* Prepare one byte write sequence */
        tx_buf[0] = COMMAND_BYTE_PROGRAM;
        tx_buf[1] = (address & 0x00FF0000) >> 16;
        tx_buf[2] = (address & 0x0000FF00) >> 8;
        tx_buf[3] = (address & 0x000000FF);
        tx_buf[4] = (*buffer_ptr);

        /* Launch SPI action */
        err = spi_driver_xfer(device_ptr->spi_handler_ptr, tx_buf, 5u, NULL, 0u, 5u);
        if (ERROR_NONE != err)
        {
            return err;
        }
    }

    return err;
}

int flash_spi_erase_sector(flash_spi_hdl_t device_hdl, uint32_t address)
{
    flash_spi_device_t * device_ptr = (flash_spi_device_t *) device_hdl;
    uint8_t tx_buf[4];
    int err = ERROR_NONE;

    /* Check parameters */
    if (address >= device_ptr->chip_size)
    {
        return ERROR_BAD_PARAMETER;
    }

    /* Execute a write enable command first */
    err = flash_spi_write_enable(device_ptr);
    if (err != ERROR_NONE)
    {
        return err;
    }

    /* Prepare TX buffer */
    tx_buf[0] = COMMAND_SECTOR_ERASE;
    tx_buf[1] = (address & 0x00FF0000) >> 16;
    tx_buf[2] = (address & 0x0000FF00) >> 8;
    tx_buf[3] = (address & 0x000000FF);

    /* Launch SPI sector erase action */
    err = spi_driver_xfer(device_ptr->spi_handler_ptr, tx_buf, 4u, NULL, 0u, 4u);

    /* Wait till device is ready */
    err = flash_spi_wait_till_ready(device_ptr);
    if (err != ERROR_NONE)
    {
        return err;
    }

    /* Write disable */
    err = flash_spi_write_disable(device_ptr);

    return err;
}

int flash_spi_get_erase_range(flash_spi_hdl_t device_hdl, uint32_t address, uint32_t * start_ptr,
                              uint32_t * end_ptr)
{
    flash_spi_device_t * device_ptr = (flash_spi_device_t *) device_hdl;
    int err = ERROR_NONE;

    /* Check parameters */
    if (address >= device_ptr->chip_size)
    {
        return ERROR_BAD_PARAMETER;
    }

    *start_ptr = address - (address % device_ptr->sector_size);
    *end_ptr = *start_ptr + device_ptr->sector_size - 1;

    return err;
}

int flash_spi_get_status(flash_spi_hdl_t device_hdl, flash_spi_status_t * status_ptr)
{

    flash_spi_device_t * device_ptr = (flash_spi_device_t *) device_hdl;
    int err;
    uint8_t status_reg;

    /* Read status register */
    err = flash_spi_read_status(device_ptr, &status_reg);

    /* Analyze the returned status */
    if (err != ERROR_NONE)
    {
        *status_ptr = FLASH_SPI_STATUS_DEVICE_ERROR;
    }
    else if (status_reg == 0xff)
    {
        *status_ptr = FLASH_SPI_STATUS_DEVICE_ERROR;
    }
    else if ((status_reg & STATUS_REG_BUSY) == STATUS_REG_BUSY)
    {
        *status_ptr = FLASH_SPI_STATUS_BUSY;
    }
    else
    {
        *status_ptr = FLASH_SPI_STATUS_READY;
    }

    return err;
}

int flash_spi_get_device_id(flash_spi_hdl_t device_hdl, uint16_t * device_id_ptr)
{

    flash_spi_device_t * device_ptr = (flash_spi_device_t *) device_hdl;
    uint8_t tx_buf[6] = {COMMAND_READ_ID, 0x00, 0x00, 0x01};
    int err;

    /* Launch SPI action */
    err = spi_driver_xfer(device_ptr->spi_handler_ptr, tx_buf, 4u, (uint8_t *) device_id_ptr, 2u, 4u);

    return err;
}


/*--------------------------------------------------------------------------------------------------
 *  LOCAL FUNCTIONS
 *------------------------------------------------------------------------------------------------*/
/**
 * Executes a Write Enable command to allow erasing or writing data to the flash device.
 *
 * \param[in] device_ptr Pointer to the Flash device handler corresponding to the target device
 *
 * \return Flash SPI error code
 */
static int flash_spi_write_enable(flash_spi_device_t * device_ptr)
{
    uint8_t tx_buf = COMMAND_WRITE_ENABLE;
    int err;

    /* Execute a write enable command */
    err = spi_driver_xfer(device_ptr->spi_handler_ptr, &tx_buf, 1, NULL, 0, 1);

    return err;
}

/**
 * Executes a Write Disable command to disallow any write operation to the flash device.
 *
 * \param[in] device_ptr Pointer to the Flash device handler corresponding to the target device
 *
 * \return Flash SPI error code
 */
static int flash_spi_write_disable(flash_spi_device_t * device_ptr)
{
    uint8_t tx_buf = COMMAND_WRITE_DISABLE;
    int err;

    /* Execute a write enable command */
    err = spi_driver_xfer(device_ptr->spi_handler_ptr, &tx_buf, 1, NULL, 0, 1);

    return err;
}

/**
 * Clear all block protect bits of the flash status register in order to be able to write or erase
 * any memory area of the device.
 *
 * \param[in] device_ptr Pointer to the Flash device handler corresponding to the target device
 *
 * \return Flash SPI error code
 */
static int flash_spi_unlock(flash_spi_device_t * device_ptr)
{
    uint8_t tx_buf[2];
    int err;
    uint8_t status_reg;

    /* Enable status reg to be written */
    tx_buf[0] = COMMAND_WRITE_STATUS_ENABLE;
    err = spi_driver_xfer(device_ptr->spi_handler_ptr, tx_buf, 1, NULL, 0, 1);
    if (ERROR_NONE != err)
    {
        return err;
    }

    /* Clear the status reg to unlock the device */
    tx_buf[0] = COMMAND_WRITE_STATUS;
    tx_buf[1] = 0x00;
    err = spi_driver_xfer(device_ptr->spi_handler_ptr, tx_buf, 2, NULL, 0, 2);

    /* Check that the device has been unlocked successfully */
    tx_buf[0] = COMMAND_WRITE_STATUS;
    tx_buf[1] = 0x00;
    err = flash_spi_read_status(device_ptr, &status_reg);
    if (ERROR_NONE != err)
    {
        return err;
    }
    else if ((status_reg & (STATUS_REG_BLOCK_PROTECT_0 +
                            STATUS_REG_BLOCK_PROTECT_1 +
                            STATUS_REG_BLOCK_PROTECT_2 +
                            STATUS_REG_BLOCK_PROTECT_LOCK)) != 0x00)
    {
        return ERROR_FLASH_DRIVER_DEVICE_LOCKED;
    }

    return err;
}

/**
 * Executes a Write Enable command to allow erasing or writing data to the flash device.
 *
 * \param[in] device_ptr Pointer to the Flash device handler corresponding to the target device
 * \param[out] status_reg_ptr Pointer to return the value of the status register of the flash device
 *
 * \return Flash SPI error code
 */
static int flash_spi_read_status(flash_spi_device_t * device_ptr, uint8_t * status_reg_ptr)
{

    uint8_t tx_buf = COMMAND_READ_STATUS;
    int err;

    /* Launch SPI action */
    err = spi_driver_xfer(device_ptr->spi_handler_ptr, &tx_buf, 1u, status_reg_ptr, 1u, 1u);

    return err;
}

/**
 * Poll the status register of the flash device until its busy bit is cleared or a defined time
 * has elapsed. If a timeout occured, an error will be returned.
 *
 * \param[in] device_ptr Pointer to the Flash device handler corresponding to the target device
 *
 * \return Flash SPI error code
 */
static int flash_spi_wait_till_ready(flash_spi_device_t * device_ptr)
{
    flash_spi_status_t status;
    int err;
    uint32_t start_time = xTaskGetTickCount();
    uint32_t current_time;

    do
    {
        err = flash_spi_get_status(device_ptr, &status);
        if (ERROR_NONE != err)
        {
            return err;
        }

        if (status == FLASH_SPI_STATUS_READY)
        {
            return ERROR_NONE;
        }
        current_time = xTaskGetTickCount();
    }
    while ((current_time - start_time) < FLASH_SPI_MAX_WAIT_READY_TIME);

    return ERROR_FLASH_DRIVER_DEVICE_TIMEOUT;
}
