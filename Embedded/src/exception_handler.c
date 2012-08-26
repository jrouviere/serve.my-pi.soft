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
 * Default handler code for exceptions
 */

#include <stdint.h>
#include "board.h"
#include "gpio.h"
#include "wdt.h"

static void cycle_delay(uint32_t loops)
{
    uint32_t bclock, now;

    bclock = Get_system_register(AVR32_COUNT);
    do {
        now = Get_system_register(AVR32_COUNT);
    } while ((now - bclock) < loops);
}

/**
 * Get exception cause register value and blink the led n times, forever.
 */
void default_exception_handler()
{
    int k;
    //stop the watchdog so we can display our message
    wdt_disable();
    for(k=0;;k++)
    {
        int i;
        uint32_t ecr = Get_system_register( AVR32_ECR );

        for(i=0; i<ecr; i++)
        {
            gpio_local_set_gpio_pin(SCB_PIN_LED_ERROR);
            cycle_delay(APPLI_CPU_SPEED / 6);
            gpio_local_clr_gpio_pin(SCB_PIN_LED_ERROR);
            cycle_delay(APPLI_CPU_SPEED / 3);
        }
        cycle_delay(APPLI_CPU_SPEED);

        //enable the watchdog to reset the board
        if(k==1)
        {
            wdt_reenable();
        }
    }
}

