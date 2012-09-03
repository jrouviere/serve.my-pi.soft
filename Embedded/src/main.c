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


int main()
{
    trace_init();
    system_init();
    pc_comm_init();
    core_main_init();

    //Start FreeRTOS scheduler
    vTaskStartScheduler();

    return 0;
}
