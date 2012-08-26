

#ifndef AVR32_INTERRUPT_H_
#define AVR32_INTERRUPT_H_

#include "compiler.h"
#include "preprocessor.h"

#define AVR32_INTC_MAX_NUM_IRQS_PER_GRP             32

#define IRQ_TO_GROUP(irq_no) (irq_no/AVR32_INTC_MAX_NUM_IRQS_PER_GRP)

typedef void (*__int_handler)(void);

void interrupt_init(void);

//to avoid confusion with INTC_register_interrupt, parameters have been moved around
//I also changed irq number to irq group, because this function actually only handle groups
//your handler will have to find which interrupt occured by itself.
void interrupt_register_handler(int int_grp, uint32_t int_level, __int_handler handler);

#endif /* AVR32_INTERRUPT_H_ */
