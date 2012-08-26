/*
 * system.c
 *
 *  Created on: 30 oct. 2011
 *      Author: Julien
 */


#include "flashc.h"
#include "wdt.h"
#include "pm.h"
#include "gpio.h"
#include "board.h"

static uint8_t *flash_reset_nb = (void*)USER_PAGE_ORIG;

static void set_reset_nb(uint8_t nb)
{
    flashc_memcpy(flash_reset_nb, &nb, sizeof(flash_reset_nb), TRUE);
}


void reset_to_bootloader()
{
    //set fuse 31, on restart DFU bootloader will know we want to reprogram
    flashc_set_gp_fuse_bit(31, TRUE);

    //clean the reset counter for next restart and force the reset
    set_reset_nb(0);

    //reset mcu with watchdog
    wdt_reset_mcu();
}

void check_buggy_firmware()
{
    // Check wakeup cause: watch for watchdog and cpu error
    uint32_t cause = pm_get_reset_cause(&AVR32_PM);
    if(cause & 0x88)
    {
        if(*flash_reset_nb >= 1)
        {
            gpio_local_set_gpio_pin(SCB_PIN_LED_ERROR);
        }

        if(*flash_reset_nb >= 3)
        {
            // Enable bootloader on next restart, will be disabled after init
            flashc_set_gp_fuse_bit(31, TRUE);

            //clean the reset counter for next restart and force the reset
            set_reset_nb(0);

            wdt_reset_mcu();
        }
        else
        {
            set_reset_nb((*flash_reset_nb)+1);
        }
    }
    else
    {
        // Cold reset we can reset counter
        if(*flash_reset_nb != 0)
        {
            set_reset_nb(0);
        }
    }
}

