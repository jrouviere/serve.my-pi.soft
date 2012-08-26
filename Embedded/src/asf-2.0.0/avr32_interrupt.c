
#include "board.h"
#include "avr32_interrupt.h"

// define _evba from exception.x
extern void _evba;

void interrupt_init(void)
{
    Set_system_register(AVR32_EVBA, (uint32_t)&_evba);
}

void interrupt_register_handler(int int_grp, uint32_t int_level, __int_handler handler)
{
    AVR32_INTC.ipr[int_grp] = (int_level << AVR32_INTC_IPR_INTLEVEL_OFFSET) | (((int)handler) - ((int)&_evba));
}
