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


#include <stdio.h>
#include <string.h>

#include "power_clocks_lib.h"
#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "flashc.h"

#include "pm.h"
#include "gpio.h"
#include "wdt.h"
#include "avr32_interrupt.h"
#include "usb_task.h"

#include "trace.h"
#include "system.h"
#include "pc_comm.h"
#include "core.h"
#include "system.h"

#include "spi_driver.h"
#include "flash_spi.h"


static void system_init(void)
{
    pcl_freq_param_t pcl_freq_param =
    {
        .cpu_f = APPLI_CPU_SPEED,
        .pba_f = APPLI_PBA_SPEED,
        .osc0_f = FOSC0,
        .osc0_startup = OSC0_STARTUP
    };

    Disable_global_interrupt();
    interrupt_init();
    Enable_global_interrupt();

    //Configure system clocks
    pcl_configure_clocks(&pcl_freq_param);
    pcl_configure_usb_clock();

    // Enable the local bus interface for GPIO.
    gpio_local_init();
    gpio_local_enable_pin_output_driver(SCB_PIN_LED_ERROR);

    // Restart bootloader if firmware is unstable
    check_buggy_firmware();

    usb_task_init();

    //start watchdog with a 2000ms period
    //the watchdog is cleared periodically by communication thread
    wdt_enable(2000000);
}

void main_task(void *arg)
{
    flash_spi_hdl_t flash_hdl;
    int err;
    flash_spi_status_t flash_status;
    static uint32_t rd_buf[4];
    static uint32_t wr_buf[4] = {0xDEADBEEF, 0x8BADF00D, 0xCAFED0D0, 0xDEADFA11};
    uint16_t id;

    memset(rd_buf, 0, sizeof(rd_buf));

    /* Initialize SPI driver */
    err = spi_driver_init();
    if (err != ERROR_NONE)
    {
        for(;;)
        {
            TRACE("SPI driver init error: %d\n", err);
            vTaskDelay(5000);
        }
    }

    /* Initialize Flash driver */
    err = flash_spi_init(&flash_hdl);
    if(err != ERROR_NONE)
    {
        for(;;)
        {
            TRACE("FLASH driver init error: %d\n", err);
            vTaskDelay(5000);
        }
    }

    /* Erase the first 4kB sector */
    err = flash_spi_erase_sector(flash_hdl, 0x00000000);
    if(err != ERROR_NONE)
    {
        for(;;)
        {
            TRACE("FLASH erase error: %d\n", err);
            vTaskDelay(5000);
        }
    }
    vTaskDelay(500u);

    /* Read test data bytes */
    err = flash_spi_read(flash_hdl, (uint8_t *) rd_buf, 16, 0x00000000);
    if (err != ERROR_NONE)
    {
        TRACE("FLASH read error: %d\n", err);
        vTaskDelay(TASK_DELAY_MS(200u));
    }
    else
    {
        TRACE("Flash read after erase: %08X %08X %08X %08X\n", rd_buf[0], rd_buf[1], rd_buf[2], rd_buf[3]);
        vTaskDelay(TASK_DELAY_MS(5000u));
    }

    /* Write some data bytes */
    /* 1. try an even number of bytes */
    err = flash_spi_write(flash_hdl,(uint8_t *) wr_buf, 8, 0x00000000);
    if(err != ERROR_NONE)
    {
        for(;;)
        {
            TRACE("FLASH write error: %d\n", err);
            vTaskDelay(5000);
        }
    }
    /* 2. try an odd by greater than 1 number of bytes */
    err = flash_spi_write(flash_hdl, ((uint8_t *) wr_buf)+8, 7, 0x00000008);
    if(err != ERROR_NONE)
    {
        for(;;)
        {
            TRACE("FLASH write error: %d\n", err);
            vTaskDelay(5000);
        }
    }
    /* 3. try a single byte */
    err = flash_spi_write(flash_hdl, ((uint8_t *) wr_buf)+15, 1, 0x0000000F);
    if(err != ERROR_NONE)
    {
        for(;;)
        {
            TRACE("FLASH write error: %d\n", err);
            vTaskDelay(5000);
        }
    }
    vTaskDelay(500u);

    /* Read test data bytes */
    err = flash_spi_read(flash_hdl, ((uint8_t *) rd_buf)+15, 1, 0x0000000F);
    if (err != ERROR_NONE)
    {
        for(;;)
        {
            TRACE("FLASH read error: %d\n", err);
            vTaskDelay(TASK_DELAY_MS(5000u));
        }
    }
    /* Read test data bytes */
    err = flash_spi_read(flash_hdl, ((uint8_t *) rd_buf+8), 7, 0x00000008);
    if (err != ERROR_NONE)
    {
        for(;;)
        {
            TRACE("FLASH read error: %d\n", err);
            vTaskDelay(TASK_DELAY_MS(5000u));
        }
    }
    /* Read test data bytes */
    err = flash_spi_read(flash_hdl, ((uint8_t *) rd_buf+0), 8, 0x00000000);
    if (err != ERROR_NONE)
    {
        for(;;)
        {
            TRACE("FLASH read error: %d\n", err);
            vTaskDelay(TASK_DELAY_MS(5000u));
        }
    }

    TRACE("Flash read after program: %08X %08X %08X %08X\n", rd_buf[0], rd_buf[1], rd_buf[2], rd_buf[3]);
    vTaskDelay(TASK_DELAY_MS(5000u));


    for(;;)
    {
        vTaskDelay(TASK_DELAY_MS(1000u));
        err = flash_spi_get_device_id(flash_hdl, &id);
        if (err != ERROR_NONE)
        {
            TRACE("Flash err: %d\n", err);
        }
        TRACE("Flash ID: 0x%04X\n", id);

        err = flash_spi_get_status(flash_hdl, &flash_status);
        if (err != ERROR_NONE)
        {
            TRACE("FLASH get status error: %d\n", err);
            vTaskDelay(TASK_DELAY_MS(200u));
        }
        else if (flash_status != FLASH_SPI_STATUS_READY)
        {
            TRACE("Flash status unexpected: %u\n", flash_status);
            vTaskDelay(TASK_DELAY_MS(200u));
        }

        err = flash_spi_read(flash_hdl, (uint8_t *) rd_buf, 16, 0x00000000);
        if (err != ERROR_NONE)
        {
            TRACE("FLASH read error: %d\n", err);
            vTaskDelay(TASK_DELAY_MS(200u));
        }
        else
        {
            TRACE("Flash read: %08X %08X %08X %08X\n", rd_buf[0], rd_buf[1], rd_buf[2], rd_buf[3]);
            vTaskDelay(TASK_DELAY_MS(5000u));
        }
    }
}


int main()
{
    trace_init();
    system_init();
    pc_comm_init();
    core_main_init();

    //Create core task
    /*xTaskCreate(main_task,
                configTSK_MAIN_NAME,
                configTSK_MAIN_STACK_SIZE,
                NULL,
                configTSK_MAIN_PRIORITY,
                NULL);*/

    //Start FreeRTOS scheduler
    vTaskStartScheduler();

    return 0;
}
