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
 * \file spi_driver.h
 * \brief SPI bus driver
 */

#ifndef SPI_DRIVER_H_
#define SPI_DRIVER_H_


/*--------------------------------------------------------------------------------------------------
 *  TYPE DEFINITIONS
 *------------------------------------------------------------------------------------------------*/
/** SPI device handler definition */
typedef void * spi_driver_hdl_t;


/*--------------------------------------------------------------------------------------------------
 *  MACRO DEFINITIONS
 *------------------------------------------------------------------------------------------------*/
/**
 * \defgroup CS_reg_options Chip Select Register options definitions
 * @{
 */

/** SPI controller Chip Select Register BITS field offset */
#define SPI_DRIVER_REG_CS_BITS_OFFSET           (4u)
/** SPI controller Chip Select Register BITS field mask */
#define SPI_DRIVER_REG_CS_BITS_MASK             (0x000000F0u)
/** SPI controller Chip Select Register BITS field value for 8 bits transfers */
#define SPI_DRIVER_REG_CS_BITS_8                (0x00000000u)
/** SPI controller Chip Select Register BITS field value for 16 bits transfers */
#define SPI_DRIVER_REG_CS_BITS_16               (0x00000080u)

/** SPI controller Chip Select Register CPOL field offset */
#define SPI_DRIVER_REG_CS_CPOL_OFFSET           (0u)
/** SPI controller Chip Select Register CPOL field mask */
#define SPI_DRIVER_REG_CS_CPOL_MASK             (0x00000001u)
/** SPI controller Chip Select Register CPOL field value */
#define SPI_DRIVER_REG_CS_CPOL                  (0x00000001u)

/** SPI controller Chip Select Register NCPHA field offset */
#define SPI_DRIVER_REG_CS_NCPHA_OFFSET          (1u)
/** SPI controller Chip Select Register NCPHA field mask */
#define SPI_DRIVER_REG_CS_NCPHA_MASK            (0x00000002u)
/** SPI controller Chip Select Register NCPHA field value */
#define SPI_DRIVER_REG_CS_NCPHA                 (0x00000002u)

/** SPI controller Chip Select Register DLYBS field offset */
#define SPI_DRIVER_REG_CS_DLYBS_OFFSET          (16u)
/** SPI controller Chip Select Register DLYBS field mask */
#define SPI_DRIVER_REG_CS_DLYBS_MASK            (0x00FF0000u)

/** SPI controller Chip Select Register DLYBCT field offset */
#define SPI_DRIVER_REG_CS_DLYBCT_OFFSET         (24u)
/** SPI controller Chip Select Register DLYBCT field mask */
#define SPI_DRIVER_REG_CS_DLYBCT_MASK           (0xFF000000u)
/** @} */


/*--------------------------------------------------------------------------------------------------
 *  EXTERNAL API PROTOTYPES
 *------------------------------------------------------------------------------------------------*/
/**
 * Initialize the SPI driver:
 *   - assign MISO, MOSI and SCK I/O to the SPI controller
 *   - setup and enable the SPI controller
 *   - setup both DMA channels for RX and TX data
 *   - create general multitasking objects
 *
 * \return SPI driver error code
 */
int spi_driver_init(void);


/**
 * Assign a device to a given chip select channel and configure its dedicated SPI settings.
 *
 * \param[in] cs Chip Select channel correspond to the device being configured
 * \param[in] freq Communication frequency for this device (baudrate)
 * \param[in] options SPI Chip Select Register options for this device (\ref CS_reg_options)
 * \param[out] handler_ptr Pointer to a SPI handler used for every access to this device
 * \return SPI driver error code
 */
int spi_driver_configure_device(uint8_t cs, uint32_t freq, uint32_t options,
                                spi_driver_hdl_t * handler_ptr);


/**
 * Start a transfer to the SPI device pointed by the given handler. SPI transfers are full-duplex
 * hence the need to give both a transmit buffer and a receive buffer. However, the SPI driver
 * is able to handle rx-only or tx-only transfers, or even partly-tx or partly-rx transfers.\n
 * - TX-only transfer example:
 * \code
 * // Receive 28 bytes without taking care of what is being sent
 * err = spi_driver_xfer(device_hdl, NULL, 0, rx_buffer, 28, 0);
 * \endcode
 * - RX-only transfer example:
 * \code
 * // Send 12 bytes without taking care of what is being received
 * err = spi_driver_xfer(device_hdl, tx_buffer, 12, NULL, 0, 0);
 * \endcode
 * - Partly-RX/Partly-TX transfer example:
 * \code
 * // Send 5 bytes without taking care of the received data
 * // Then read 30 bytes without taking care of the sent data
 * err = spi_driver_xfer(device_hdl, tx_buffer, 5, rx_buffer, 30, 5);
 * \endcode
 *
 * \param[in] handler_ptr Pointer to the SPI handler corresponding to the target device
 * \param[in] tx_buf Buffer for data transmission
 * \param[in] tx_size Size of the transmit buffer
 * \param[out] rx_buf Buffer for data reception
 * \param[in] rx_size Size of the reception buffer
 *                    (in bytes or half-words depending device configuration)
 * \param[in] rx_offset Offset count before filling rx_buf with received data
 *                      (in bytes or half-words depending device configuration)
 * \return SPI driver error code
 */
int spi_driver_xfer(spi_driver_hdl_t handler_ptr, uint8_t * tx_buf, uint32_t tx_size,
                    uint8_t * rx_buf, uint32_t rx_size, uint32_t rx_offset);

#endif /* SPI_DRIVER_H_ */
