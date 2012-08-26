/*
 *  This file is part of OpenSCB project <http://openscb.org>.
 *  Copyright (C) 2011  Opendrain
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
 * \file spi_driver.c
 * \brief SPI bus driver
 */

#include "board.h"
#include "FreeRTOS.h"

#include <avr32/io.h>
#include <gpio.h>
#include <string.h>

#include "task.h"
#include "semphr.h"
#include "spi_driver.h"
#include "trace.h"
#include "avr32_interrupt.h"


/*--------------------------------------------------------------------------------------------------
 *  MACRO DEFINITIONS
 *------------------------------------------------------------------------------------------------*/

/** SPI driver number of support devices */
#define SPI_DRIVER_CS_NUMBER                (2u)
/** SPI controller base frequency reference */
#define SPI_DRIVER_CPU_FREQ                 (FOSC0)
/** SPI controller maximum supported baudrate */
#define SPI_DRIVER_BAUDRATE_MAX             (SPI_DRIVER_CPU_FREQ)
/** SPI controller minimum supported baudrate */
#define SPI_DRIVER_BAUDRATE_MIN             ((SPI_DRIVER_CPU_FREQ+255u)/255u)
/** SPI baudrate computation macro */
#define SPI_DRIVER_BAUDRATE_COMPUTE(x)      ((SPI_DRIVER_CPU_FREQ+((x)/2))/(x))

/** SPI controller Chip Select Register CSNAAT field offset */
#define SPI_DRIVER_REG_CS_CSNAAT_OFFSET     (2u)
/** SPI controller Chip Select Register CSNAAT field mask */
#define SPI_DRIVER_REG_CS_CSNAAT_MASK       (0x00000004u)
/** SPI controller Chip Select Register CSNAAT field value */
#define SPI_DRIVER_REG_CS_CSNAAT            (0x00000004u)

/** SPI controller Chip Select Register CSAAT field offset */
#define SPI_DRIVER_REG_CS_CSAAT_OFFSET      (3u)
/** SPI controller Chip Select Register CSAAT field mask */
#define SPI_DRIVER_REG_CS_CSAAT_MASK        (0x00000008u)
/** SPI controller Chip Select Register CSAAT field value */
#define SPI_DRIVER_REG_CS_CSAAT             (0x00000008u)

/** SPI controller no Chip Select assertion definition */
#define SPI_DRIVER_PCS_NONE                 (0x0fu)
/** SPI controller Chip Select 0 assertion definition */
#define SPI_DRIVER_PCS_CS0                  (0x00u)
/** SPI controller Chip Select 1 assertion definition */
#define SPI_DRIVER_PCS_CS1                  (0x01u)
/** SPI controller Chip Select 2 assertion definition */
#define SPI_DRIVER_PCS_CS2                  (0x03u)
/** SPI controller Chip Select 3 assertion definition */
#define SPI_DRIVER_PCS_CS3                  (0x07u)

/** SPI Controller options: mode fault disabled */
#define SPI_DRIVER_OPTIONS_MR_MODFDIS           (1u)
/** SPI Controller options: 6 clock cycles between CS deassertion to another CS assertion */
#define SPI_DRIVER_OPTIONS_MR_DLYBCS            (6u)
/** SPI Controller options: fixed peripheral select enabled */
#define SPI_DRIVER_OPTIONS_MR_PS                (0u)
/** SPI Controller options: local loopback disabled */
#define SPI_DRIVER_OPTIONS_MR_LLB               (0u)
/** SPI Controller options: chip select mux decode disabled */
#define SPI_DRIVER_OPTIONS_MR_PCSDEC            (0u)
/** SPI Controller options: master mode */
#define SPI_DRIVER_OPTIONS_MR_MSTR              (1u)

/** SPI driver transfer timeout (in system tick time) */
#define SPI_DRIVER_XFER_TIMEOUT                 (portMAX_DELAY)

/** DMA transfer interrupt priority level */
#define SPI_DRIVER_DMA_INTERRUPT_PRIORITY       (0u)
/** DMA transfer interrupt group for RX data */
#define SPI_DRIVER_DMA_INTERRUPT_GROUP          (3u)
/** DMA scratch buffers size */
#define SPI_DRIVER_SCRATCH_BUFFER_SIZE        (32u)
/** DMA buffer number */
#define SPI_DRIVER_DMA_BUFFER_NB                (2u)
/** Convenient macro to convert buffers size to a multiple of a device transfer size */
#define SPI_DRIVER_DMA_CONVERT_DMA_SIZE(dma_size)       ((dma_size == AVR32_PDCA_MR0_SIZE_BYTE)\
                                                                     ? (1)\
                                                                     : (2))
/** Convenient macro to init a buffer in a DMA management structure */
#define SPI_DRIVER_DMA_INIT_BUFFER(struct_name, buf_nb, _addr, _size, _chunk_size)     do\
        {\
            spi_driver_dma_##struct_name##_buffer_mgt.buffer_list[buf_nb].addr = _addr;\
            spi_driver_dma_##struct_name##_buffer_mgt.buffer_list[buf_nb].chunk_size = _chunk_size;\
            spi_driver_dma_##struct_name##_buffer_mgt.buffer_list[buf_nb].remaining = _size;\
        } while(0)


/*--------------------------------------------------------------------------------------------------
 *  TYPE DEFINITIONS
 *------------------------------------------------------------------------------------------------*/

/** SPI HW definitions: Chip Select register */
typedef struct
{
    uint32_t dlybct :8;     /**< Delay between consecutive transfers */
    uint32_t dlybs :8;      /**< Delay before SPCK */
    uint32_t scbr :8;       /**< Serial clock baudrate */
    uint32_t bits :4;       /**< Bits per transfer */
    uint32_t csaat :1;      /**< CS active after transfer */
    uint32_t csnaat :1;     /**< CS inactive after transfer */
    uint32_t ncpha :1;      /**< Clock phase */
    uint32_t cpol :1;       /**< Clock polarity */
} spi_driver_reg_cs_t;

/** SPI driver device structure */
typedef struct
{
    volatile spi_driver_reg_cs_t * reg_ptr;     /**< SPI controller HW chip select register */
    bool is_configured;                         /**< Flag stating if the device has been properly configured or not */
    uint8_t cs_nb;                              /**< Chip Select number in HW register format */
    uint32_t dma_size;                          /**< Size of DMA transfers in HW register format */
} spi_driver_device_t;


/** DMA buffer structure definition */
typedef struct
{
    uint32_t addr;          /**< Buffer start address */
    uint32_t chunk_size;    /**< Buffer chunk size (as multiple of DMA transfer size) */
    uint32_t remaining;     /**< Remaining size to be transfered (as multiple of DMA transfer size) */
} spi_driver_dma_buffer_t;

/** Buffer management structure for DMA transfers */
typedef struct
{
    spi_driver_dma_buffer_t buffer_list[SPI_DRIVER_DMA_BUFFER_NB];  /**< DMA buffer list */
    uint32_t buffer_count;      /**< Number of buffer in DMA buffer list */
    uint32_t load_count;        /**< Number of DMA loading to be done */
    uint8_t dma_transfer_size;  /**< Nb of bytes for each DMA transfer (1 or 2) */
} spi_driver_dma_mgt_t;


/*--------------------------------------------------------------------------------------------------
 *  GLOBAL VARIABLES
 *------------------------------------------------------------------------------------------------*/
/** Mutex used to protect the access to the SPI controller to one task at a time */
static xSemaphoreHandle spi_driver_controller_mutex = NULL;
/** Binary semaphore used for synchronization purposes with the end of the DMA transfer */
static xSemaphoreHandle spi_driver_dma_sem = NULL;
/** Scratch buffer for dumping or sending useless data */
static uint16_t spi_driver_scratch_buf[SPI_DRIVER_SCRATCH_BUFFER_SIZE];
/** Buffer management for DMA RX transfers */
static spi_driver_dma_mgt_t spi_driver_dma_rx_buffer_mgt;
/** Buffer management for DMA TX transfers */
static spi_driver_dma_mgt_t spi_driver_dma_tx_buffer_mgt;


/*--------------------------------------------------------------------------------------------------
 *  LOCAL FUNCTIONS PROTOTYPES
 *------------------------------------------------------------------------------------------------*/
static spi_driver_device_t * spi_driver_init_cs0(void);
static spi_driver_device_t * spi_driver_init_cs1(void);
static void spi_driver_dma_it_handler(void);
static void spi_driver_dma_rx_it_handler(void);
static void spi_driver_dma_tx_it_handler(void);
static void spi_driver_dma_launch_transfer(void);


/*--------------------------------------------------------------------------------------------------
 *  EXTERNAL API FUNCTIONS
 *------------------------------------------------------------------------------------------------*/
int spi_driver_init(void)
{
    int err;
    gpio_map_t spi_io_map =
        {{ AVR32_SPI_SCK_0_1_PIN, AVR32_SPI_SCK_0_1_FUNCTION},
         { AVR32_SPI_MOSI_0_2_PIN, AVR32_SPI_MOSI_0_2_FUNCTION},
         { AVR32_SPI_MISO_0_2_PIN, AVR32_SPI_MISO_0_2_FUNCTION}};

    /* Assign I/Os to SPI */
    gpio_enable_module(spi_io_map, 3);

    /* Reset the SPI controller */
    AVR32_SPI.CR.swrst = 1;

    /* Mode register setup */
    AVR32_SPI.MR.modfdis = SPI_DRIVER_OPTIONS_MR_MODFDIS;
    AVR32_SPI.MR.dlybcs = SPI_DRIVER_OPTIONS_MR_DLYBCS;
    AVR32_SPI.MR.llb = SPI_DRIVER_OPTIONS_MR_LLB;
    AVR32_SPI.MR.mstr = SPI_DRIVER_OPTIONS_MR_MSTR;
    AVR32_SPI.MR.pcsdec = SPI_DRIVER_OPTIONS_MR_PCSDEC;
    AVR32_SPI.MR.ps = SPI_DRIVER_OPTIONS_MR_PS;

    /* Enable SPI controller */
    AVR32_SPI.CR.spien = 1;

    /* Activate SPI controller by asserting/deasserting a CS */
    AVR32_SPI.MR.pcs = SPI_DRIVER_PCS_NONE;
    AVR32_SPI.MR.pcs = SPI_DRIVER_PCS_CS1;
    AVR32_SPI.MR.pcs = SPI_DRIVER_PCS_NONE;

    /* Setup DMA channel register */
    AVR32_PDCA.channel[DMA_CHANNEL_SPI_TX].PSR.pid = AVR32_PDCA_PID_SPI_TX;
    AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].PSR.pid = AVR32_PDCA_PID_SPI_RX;

    /* Enable DMA transfers interrupts */
    Disable_global_interrupt();
    interrupt_register_handler(SPI_DRIVER_DMA_INTERRUPT_GROUP, SPI_DRIVER_DMA_INTERRUPT_PRIORITY,
                               &spi_driver_dma_it_handler);
    Enable_global_interrupt();

    /* Init scratch buffer for dumping or getting useless data */
    memset(spi_driver_scratch_buf, 0, sizeof(spi_driver_scratch_buf));

    /* Setup SPI controller mutex */
    spi_driver_controller_mutex = xSemaphoreCreateMutex();

    /* Create the event semaphore used to pend on DMA transfer completion */
    vSemaphoreCreateBinary(spi_driver_dma_sem);
    if (spi_driver_dma_sem == NULL)
    {
        err = ERROR_OS_COULD_NOT_ALLOCATE_REQUIRED_MEMORY;
    }
    else
    {
        err = xSemaphoreTake(spi_driver_dma_sem, 1);
        if (err == pdTRUE)
        {
            err = ERROR_NONE;
        }
    }

    return err;
}

int spi_driver_configure_device(uint8_t cs, uint32_t freq, uint32_t options,
                                spi_driver_hdl_t * handler_ptr)
{
    spi_driver_device_t * device_ptr;

    /* Check parameters */
    if ((cs >= SPI_DRIVER_CS_NUMBER) || (NULL == handler_ptr) || (freq > SPI_DRIVER_BAUDRATE_MAX)
            || (freq < SPI_DRIVER_BAUDRATE_MIN))
    {
        return ERROR_BAD_PARAMETER;
    }
    else if (((options & SPI_DRIVER_REG_CS_BITS_MASK) != SPI_DRIVER_REG_CS_BITS_16) &&
            ((options & SPI_DRIVER_REG_CS_BITS_MASK) != SPI_DRIVER_REG_CS_BITS_8))
    {
        return ERROR_BAD_PARAMETER;
    }

    /* Retrieve the device descriptor structure corresponding
     * to the given chip select number */
    switch (cs)
    {
        case 0:
            device_ptr = spi_driver_init_cs0();
            break;
        case 1:
            device_ptr = spi_driver_init_cs1();
            break;
        default:
            device_ptr = NULL;
            break;
    }

    /* Check that the retrieved device descriptor structure
     * was initialized properly */
    if (NULL == device_ptr)
    {
        return ERROR_SPI_DRIVER_NOT_INITIALIZED;
    }

    /* Setup SPI controller for the corresponding device */
    device_ptr->reg_ptr->scbr = SPI_DRIVER_BAUDRATE_COMPUTE(freq);
    device_ptr->reg_ptr->bits = ((options & SPI_DRIVER_REG_CS_BITS_MASK)
            >> SPI_DRIVER_REG_CS_BITS_OFFSET);
    device_ptr->reg_ptr->dlybct = ((options & SPI_DRIVER_REG_CS_DLYBCT_MASK)
            >> SPI_DRIVER_REG_CS_DLYBCT_OFFSET);
    device_ptr->reg_ptr->dlybs = ((options & SPI_DRIVER_REG_CS_DLYBS_MASK)
            >> SPI_DRIVER_REG_CS_DLYBS_OFFSET);
    device_ptr->reg_ptr->ncpha = ((options & SPI_DRIVER_REG_CS_NCPHA_MASK)
            >> SPI_DRIVER_REG_CS_NCPHA_OFFSET);
    device_ptr->reg_ptr->cpol = ((options & SPI_DRIVER_REG_CS_CPOL_MASK)
            >> SPI_DRIVER_REG_CS_CPOL_OFFSET);

    /* Store transfer size parameter for DMA transfers */
    if ((options & SPI_DRIVER_REG_CS_BITS_MASK) == SPI_DRIVER_REG_CS_BITS_8)
    {
        device_ptr->dma_size = AVR32_PDCA_MR0_SIZE_BYTE;
    }
    else
    {
        device_ptr->dma_size = AVR32_PDCA_MR0_SIZE_HALF_WORD;
    }


    /* Declare the device as configured and return a pointer to the device structure */
    device_ptr->is_configured = TRUE;
    *handler_ptr = (void *) device_ptr;

    return ERROR_NONE;
}

int spi_driver_xfer(spi_driver_hdl_t handler_ptr, uint8_t * tx_buf, uint32_t tx_size,
                    uint8_t * rx_buf, uint32_t rx_size, uint32_t rx_offset)
{
    spi_driver_device_t * device_ptr = ((spi_driver_device_t *) handler_ptr);
    int mutex_err;
    int err = ERROR_NONE;
    uint8_t dma_transfer_size = SPI_DRIVER_DMA_CONVERT_DMA_SIZE(device_ptr->dma_size);

    /* Take the hand over the SPI controller */
    mutex_err = xSemaphoreTake(spi_driver_controller_mutex, SPI_DRIVER_XFER_TIMEOUT);
    if (mutex_err != pdTRUE)
    {
        return mutex_err;
    }

    gpio_local_set_gpio_pin(SCB_PIN_LED_ERROR);

    /* Assert the CS corresponding to the given device */
    AVR32_SPI.MR.pcs = device_ptr->cs_nb;

    /* Prepare TX buffer management details */
    memset((uint8_t *) &spi_driver_dma_tx_buffer_mgt, 0u, sizeof(spi_driver_dma_tx_buffer_mgt));
    spi_driver_dma_tx_buffer_mgt.dma_transfer_size = dma_transfer_size;

    if (tx_size == 0u)
    {
        /* No TX data to be sent, first buffer is the scratch tx buffer */
        SPI_DRIVER_DMA_INIT_BUFFER(tx, 0, (uint32_t) spi_driver_scratch_buf,
                                   rx_size + rx_offset,
                                   SPI_DRIVER_SCRATCH_BUFFER_SIZE/dma_transfer_size);
        spi_driver_dma_tx_buffer_mgt.buffer_count = 1;
        /* Second buffer is unused */
    }
    else
    {
        /* First buffer is the given TX buffer */
        SPI_DRIVER_DMA_INIT_BUFFER(tx, 0, (uint32_t) tx_buf, tx_size, tx_size);
        spi_driver_dma_tx_buffer_mgt.buffer_count = 1;
        /* If RX total transfer size is bigger than the TX buffer,
         * append scratch data */
        if ((rx_size + rx_offset) > tx_size)
        {
            /* Second buffer is the scratch TX buffer */
            SPI_DRIVER_DMA_INIT_BUFFER(tx, 1, (uint32_t) spi_driver_scratch_buf,
                                       (rx_size + rx_offset) - tx_size,
                                       SPI_DRIVER_SCRATCH_BUFFER_SIZE/dma_transfer_size);
            spi_driver_dma_tx_buffer_mgt.buffer_count = 2;
        }
    }

    /* Prepare RX buffer management details */
    memset((uint8_t *) &spi_driver_dma_rx_buffer_mgt, 0u, sizeof(spi_driver_dma_rx_buffer_mgt));

    if (rx_size == 0u)
    {
        /* No RX data to be received, first buffer is the scratch RX buffer */
        SPI_DRIVER_DMA_INIT_BUFFER(rx, 0, (uint32_t) spi_driver_scratch_buf, tx_size,
                                   SPI_DRIVER_SCRATCH_BUFFER_SIZE/dma_transfer_size);
        spi_driver_dma_rx_buffer_mgt.buffer_count = 1;
        /* Second buffer is unused */
    }
    else
    {
        if (rx_offset > 0u)
        {
            /* First buffer is the scratch RX buffer */
            SPI_DRIVER_DMA_INIT_BUFFER(rx, 0, (uint32_t) spi_driver_scratch_buf, rx_offset,
                                       SPI_DRIVER_SCRATCH_BUFFER_SIZE/dma_transfer_size);
            /* Second buffer is the given RX buffer */
            SPI_DRIVER_DMA_INIT_BUFFER(rx, 1, (uint32_t) rx_buf, rx_size, rx_size);
            spi_driver_dma_rx_buffer_mgt.buffer_count = 2;
        }
        else
        {
            /* First buffer is the given RX buffer */
            SPI_DRIVER_DMA_INIT_BUFFER(rx, 0, (uint32_t) rx_buf, rx_size, rx_size);
            spi_driver_dma_rx_buffer_mgt.buffer_count = 1;
            if (rx_size < tx_size)
            {
                /* Second buffer is the scratch RX buffer */
                SPI_DRIVER_DMA_INIT_BUFFER(rx, 1, (uint32_t) spi_driver_scratch_buf,
                                           tx_size - rx_size,
                                           SPI_DRIVER_SCRATCH_BUFFER_SIZE/dma_transfer_size);
                spi_driver_dma_rx_buffer_mgt.buffer_count = 2;
            }
        }
    }

    /* Launch DMA transfer */
    spi_driver_dma_launch_transfer();
    AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].CR.ten = 1;
    AVR32_PDCA.channel[DMA_CHANNEL_SPI_TX].CR.ten = 1;

    AVR32_PDCA.channel[DMA_CHANNEL_SPI_TX].IER.terr = 1;
    AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].IER.terr = 1;
    AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].IER.trc = 1;

    /* Pend on DMA transfer end */
    err = xSemaphoreTake(spi_driver_dma_sem, SPI_DRIVER_XFER_TIMEOUT);

    /* Check DMA transfer flags and check that no error occurred */
    if (AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].ISR.terr == 1)
    {
        AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].CR.eclr = 1;
        err = ERROR_SPI_DRIVER_DMA;
    }
    else if (err != pdTRUE)
    {
        err = ERROR_SPI_DRIVER_TIMEOUT;
    }
    else
    {
        err = ERROR_NONE;
    }

    AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].CR.tdis = 1;
    AVR32_PDCA.channel[DMA_CHANNEL_SPI_TX].CR.tdis = 1;

    /* Deassert SPI chip select */
    AVR32_SPI.MR.pcs = SPI_DRIVER_PCS_CS0;

    gpio_local_clr_gpio_pin(SCB_PIN_LED_ERROR);

    /* Give back the hand over the SPI controller */
    mutex_err = xSemaphoreGive(spi_driver_controller_mutex);
    if (mutex_err != pdTRUE)
    {
        err = mutex_err;
    }

    return err;
}


/*--------------------------------------------------------------------------------------------------
 *  LOCAL FUNCTIONS
 *------------------------------------------------------------------------------------------------*/
/**
 * Initialize the device descriptor structure corresponding to the SPI chip select 0.
 *
 * \return device descriptor structure pointer
 */
static spi_driver_device_t * spi_driver_init_cs0(void)
{
    static spi_driver_device_t spi_driver_device_cs0 =
        { reg_ptr: (spi_driver_reg_cs_t *) &AVR32_SPI.CSR0, is_configured: FALSE,
          cs_nb: SPI_DRIVER_PCS_CS0, dma_size: 0 };

    gpio_map_t spi_io_map =
        {{AVR32_SPI_NPCS_0_1_PIN, AVR32_SPI_NPCS_0_1_FUNCTION}};

    /* Assign CS I/O to SPI */
    gpio_enable_module(spi_io_map, 1);

    return &spi_driver_device_cs0;
}


/**
 * Initialize the device descriptor structure corresponding to the SPI chip select 1.
 *
 * \return device descriptor structure pointer
 */
static spi_driver_device_t * spi_driver_init_cs1(void)
{
    static spi_driver_device_t spi_driver_device_cs1 =
        { reg_ptr: (spi_driver_reg_cs_t *) &AVR32_SPI.CSR1, is_configured: FALSE,
          cs_nb: SPI_DRIVER_PCS_CS1, dma_size: 0 };

    gpio_map_t spi_io_map =
    {{AVR32_SPI_NPCS_1_1_PIN, AVR32_SPI_NPCS_1_1_FUNCTION}};

    /* Assign CS I/O to SPI */
    gpio_enable_module(spi_io_map, 1);

    return &spi_driver_device_cs1;
}


/**
 * Load DMA registers and, if necessary, DMA reload registers with the content of the buffers
 * specified in RX and TX DMA buffer management structures.
 * When the main DMA transfer is completed, DMA reload registers are used to begin a second
 * transfer.
 * If there is still non-empty buffers even after loading the DMA reload registers,
 * this function will activate the DMA reload interrupt.
 */
static void spi_driver_dma_launch_transfer(void)
{
    uint32_t load_count;
    uint32_t mgt_index;
    uint32_t i;
    uint32_t remaining_size;
    uint32_t chunk_size;
    spi_driver_dma_mgt_t * mgt_ptr[2] = {&spi_driver_dma_tx_buffer_mgt,
                                         &spi_driver_dma_rx_buffer_mgt};
    volatile avr32_pdca_channel_t * dma_ptr[2] = {&AVR32_PDCA.channel[DMA_CHANNEL_SPI_TX],
                                                  &AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX]};
    bool b_first_dma_setup;
    bool b_second_dma_setup;

    for (mgt_index = 0; mgt_index < 2; mgt_index++)
    {
        load_count = 0;
        b_first_dma_setup = FALSE;
        b_second_dma_setup = FALSE;
        for (i = 0; i < mgt_ptr[mgt_index]->buffer_count; i++)
        {
            /* Compute loading count */
            remaining_size = mgt_ptr[mgt_index]->buffer_list[i].remaining;
            chunk_size = mgt_ptr[mgt_index]->buffer_list[i].chunk_size;
            if (chunk_size != 0)
            {
                load_count += (remaining_size / chunk_size);
                load_count += ((remaining_size % chunk_size) ? 1 : 0);
            }

            /* If not already done, setup the first DMA transfer */
            if ((b_first_dma_setup == FALSE) && (mgt_ptr[mgt_index]->buffer_list[i].remaining > 0))
            {
                b_first_dma_setup = TRUE;
                /* Setup DMA memory address register */
                dma_ptr[mgt_index]->mar = mgt_ptr[mgt_index]->buffer_list[i].addr;
                if (mgt_ptr[mgt_index]->buffer_list[i].chunk_size
                        < mgt_ptr[mgt_index]->buffer_list[i].remaining)
                {
                    /* Setup DMA transfer counter register */
                    dma_ptr[mgt_index]->tcr = mgt_ptr[mgt_index]->buffer_list[i].chunk_size;
                    /* Update remaining transfer count */
                    mgt_ptr[mgt_index]->buffer_list[i].remaining -=
                            mgt_ptr[mgt_index]->buffer_list[i].chunk_size;
                }
                else
                {
                    /* Setup DMA transfer counter register */
                    dma_ptr[mgt_index]->tcr = mgt_ptr[mgt_index]->buffer_list[i].remaining;
                    /* Update remaining transfer count */
                    mgt_ptr[mgt_index]->buffer_list[i].remaining = 0u;
                }
            }

            /* If not already done, setup the second DMA transfer */
            if ((b_second_dma_setup == FALSE) && (mgt_ptr[mgt_index]->buffer_list[i].remaining > 0))
            {
                b_second_dma_setup = TRUE;
                /* Setup DMA memory address reload register */
                dma_ptr[mgt_index]->marr = mgt_ptr[mgt_index]->buffer_list[i].addr;
                if (mgt_ptr[mgt_index]->buffer_list[i].chunk_size
                        < mgt_ptr[mgt_index]->buffer_list[i].remaining)
                {
                    /* Setup DMA transfer counter reload register */
                    dma_ptr[mgt_index]->tcrr = mgt_ptr[mgt_index]->buffer_list[i].chunk_size;
                    /* Update remaining transfer count */
                    mgt_ptr[mgt_index]->buffer_list[i].remaining -=
                            mgt_ptr[mgt_index]->buffer_list[i].chunk_size;
                }
                else
                {
                    /* Setup DMA transfer counter reload register */
                    dma_ptr[mgt_index]->tcrr = mgt_ptr[mgt_index]->buffer_list[i].remaining;
                    /* Update remaining transfer count */
                    mgt_ptr[mgt_index]->buffer_list[i].remaining = 0u;
                }
            }
        }

        /* Store loading count */
        mgt_ptr[mgt_index]->load_count = load_count;

        /* Check number of loading necessary */
        if (mgt_ptr[mgt_index]->load_count > 2)
        {
            /* There are more than 2 loading necessary.
             * => Reload interrupt need to be managed */
            dma_ptr[mgt_index]->IER.rcz = 1;
        }
    }
}


/**
 * Reload the DMA address reload and DMA transfer count reload registers of the given DMA channel,
 * by finding the next not completed buffer in the given DMA buffer management structure.
 *
 * \param[out] dma_channel_ptr Pointer to the DMA channel hardware registers
 * \param[in,out] dma_mgt_ptr Pointer to the DMA buffer management structure
 */
static void spi_driver_dma_reload(volatile avr32_pdca_channel_t * dma_channel_ptr,
                                  spi_driver_dma_mgt_t * dma_mgt_ptr)
{
    uint32_t i;
    bool b_reload_done = FALSE;

    dma_mgt_ptr->load_count--;

    for (i = 0; (i < dma_mgt_ptr->buffer_count) && (b_reload_done == FALSE); i++)
    {
        if (dma_mgt_ptr->buffer_list[i].remaining > 0)
        {
            b_reload_done = TRUE;
            /* Setup DMA memory address reload register */
            dma_channel_ptr->marr = dma_mgt_ptr->buffer_list[i].addr;
            if (dma_mgt_ptr->buffer_list[i].chunk_size < dma_mgt_ptr->buffer_list[i].remaining)
            {
                /* Setup DMA transfer counter reload register */
                dma_channel_ptr->tcrr = dma_mgt_ptr->buffer_list[i].chunk_size;
                /* Update remaining transfer count */
                dma_mgt_ptr->buffer_list[i].remaining -= dma_mgt_ptr->buffer_list[i].chunk_size;
            }
            else
            {
                /* Setup DMA transfer counter reload register */
                dma_channel_ptr->tcrr = dma_mgt_ptr->buffer_list[i].remaining;
                /* Update remaining transfer count */
                dma_mgt_ptr->buffer_list[i].remaining = 0;
            }
        }
    }

    /* Check number of loading necessary */
    if (dma_mgt_ptr->load_count > 2)
    {
        /* There are still more than 2 loading necessary.
         * => Reload interrupt need to be managed */
       dma_channel_ptr->IER.rcz = 1;
    }
}

__attribute__((section(".exception")))
__attribute__((__interrupt__))
/**
 * Global DMA interrupt service routine.
 * Depending the DMA channel that requested the interrupt, rx or tx dedicated interrupt handler
 * will be called.
 */
static void spi_driver_dma_it_handler(void)
{
    uint32_t irr = AVR32_INTC.irr[SPI_DRIVER_DMA_INTERRUPT_GROUP];

    if (irr & (1u << DMA_CHANNEL_SPI_RX))
    {
        spi_driver_dma_rx_it_handler();
    }

    if (irr & (1u << DMA_CHANNEL_SPI_TX))
    {
        spi_driver_dma_tx_it_handler();
    }
}


/**
 * Interrupt service routine dedicated to the RX DMA channel.
 *
 * \return device descriptor structure pointer
 */
static void spi_driver_dma_rx_it_handler(void)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    /* Check interrupt source */
    if (AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].ISR.terr == 1)
    {
        /* Transfer error interrupt */
        AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].IDR.rcz = 1;
        AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].IDR.trc = 1;
        AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].IDR.terr = 1;
        (void) xSemaphoreGiveFromISR(spi_driver_dma_sem, &xHigherPriorityTaskWoken);
    }
    else if (AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].ISR.trc == 1)
    {
        /* Transfer complete interrupt */
        AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].IDR.rcz = 1;
        AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].IDR.trc = 1;
        AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].IDR.terr = 1;
        (void) xSemaphoreGiveFromISR(spi_driver_dma_sem, &xHigherPriorityTaskWoken);
    }
    else if (AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].ISR.rcz == 1)
    {
        /* Transfer reload interrupt */
        AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX].IDR.rcz = 1;
        /* Reload complete interrupt */
        spi_driver_dma_reload(&AVR32_PDCA.channel[DMA_CHANNEL_SPI_RX],
                              &spi_driver_dma_rx_buffer_mgt);
    }
}


/**
 * Interrupt service routine dedicated to the TX DMA channel.
 *
 * \return device descriptor structure pointer
 */
static void spi_driver_dma_tx_it_handler(void)
{
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    /* Check interrupt source */
    if (AVR32_PDCA.channel[DMA_CHANNEL_SPI_TX].ISR.terr == 1)
    {
        /* Transfer error interrupt */
        AVR32_PDCA.channel[DMA_CHANNEL_SPI_TX].IDR.rcz = 1;
        AVR32_PDCA.channel[DMA_CHANNEL_SPI_TX].IDR.terr = 1;
        (void) xSemaphoreGiveFromISR(spi_driver_dma_sem, &xHigherPriorityTaskWoken);
    }
    else if (AVR32_PDCA.channel[DMA_CHANNEL_SPI_TX].ISR.rcz == 1)
    {
        /* Transfer reload interrupt */
        AVR32_PDCA.channel[DMA_CHANNEL_SPI_TX].IDR.rcz = 1;
        /* Reload complete interrupt */
        spi_driver_dma_reload(&AVR32_PDCA.channel[DMA_CHANNEL_SPI_TX],
                              &spi_driver_dma_tx_buffer_mgt);
    }
}
