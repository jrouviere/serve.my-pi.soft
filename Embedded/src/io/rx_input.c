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


#include "board.h"
#include "tc.h"
#include "gpio.h"
#include "interrupt.h"
#include "system.h"
#include "trace.h"
#include "rx_input.h"
#include "avr32_interrupt.h"


#define INACTIVITY_TIMEOUT_TICK         ((rctime_t) US_TO_TC4_TICK(25000))
#define FRAME_START_PULSE_VALUE         ((rctime_t) US_TO_TC4_TICK(3000))
#define DEFAULT_PULSE_VALUE             ((rctime_t) US_TO_TC4_TICK(1500))
#define AVR32_GPIO_GROUP                2

//FIFO are implemented with a simple circular buffer,
//the easiest way to do that in software is to use a binary mask
//must be a power of two
#define FIFO_SIZE                       (1 << 6)
#define FIFO_MASK                       (FIFO_SIZE-1)

//macro to increment a FIFO index, with wrap-around
#define INC_FIFO_INDEX(idx)             do { idx = ((idx+1) & FIFO_MASK); } while(0)

#define RX_IRQ_LEVEL                    2

#define SYSTEM_TC_CHANNEL               1

#define MAX_PPM_CHANNEL                 16



typedef struct
{
    uint32_t gpio_mask;                     //!< mask used for direct register comparison
    bool prev_level;                        //!< 0 = low, 1 = high

    rctime_t last_toggle_time;              //!< absolute time of gpio modification

    int index;                              //!< current index in the ppm frame

    bool active;                            //!< any activity lately on this gpio?
    uint16_t pulse_tick[MAX_PPM_CHANNEL];   //!< last measured pulse width in timer ticks
} ppm_input_t;



typedef struct
{
    uint32_t value; /*TODO: useless for ppm*/
    rctime_t time;
} irq_fifo_data_t;


static ppm_input_t ppm_input;

static uint32_t global_gpio_mask = 0;

static irq_fifo_data_t irq_fifo[FIFO_SIZE];
static int irq_fifo_idx = 1;
static int pop_fifo_idx = 0;

static volatile avr32_gpio_port_t *gpio_port = &AVR32_GPIO.port[1];
static volatile avr32_tc_t *tc = (&AVR32_TC);


const io_class_t ppm_input_class = {
    .name = "RC PPM input",

    .init_module = ppm_input_init,
    .init_channel = NULL,
    .cleanup_channel = NULL,
    .cleanup_module = NULL,

    .pre = rx_ppm_input_process_input,
    .get = ppm_input_get_value,
    .set = NULL,
    .post = NULL,
};



#define timer_get_value() ((rctime_t)(tc->channel[SYSTEM_TC_CHANNEL].cv & 0xFFFF))


void timer_init()
{
    //free running 16 bits timer, used to measure pulse width
    static const tc_waveform_opt_t WAVEFORM_OPT =
    {
      .channel  = SYSTEM_TC_CHANNEL,                 // Channel selection.

      .bswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOB.
      .beevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOB.
      .bcpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOB.
      .bcpb     = TC_EVT_EFFECT_NOOP,                // RB compare effect on TIOB.

      .aswtrg   = TC_EVT_EFFECT_NOOP,                // Software trigger effect on TIOA.
      .aeevt    = TC_EVT_EFFECT_NOOP,                // External event effect on TIOA.
      .acpc     = TC_EVT_EFFECT_NOOP,                // RC compare effect on TIOA: toggle.
      .acpa     = TC_EVT_EFFECT_NOOP,                // RA compare effect on TIOA: toggle (other possibilities are none, set and clear).

      .wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,// Waveform selection: Up mode with automatic trigger(reset) on RC compare.
      .enetrg   = FALSE,                             // External event trigger enable.
      .eevt     = 0,                                 // External event selection.
      .eevtedg  = TC_SEL_NO_EDGE,                    // External event edge selection.
      .cpcdis   = FALSE,                             // Counter disable when RC compare.
      .cpcstop  = FALSE,                             // Counter clock stopped with RC compare.

      .burst    = FALSE,                             // Burst signal selection.
      .clki     = FALSE,                             // Clock inversion.
      .tcclks   = TC_CLOCK_SOURCE_TC4                // Internal source clock 4, connected to fPBA / 32.
    };

    tc_init_waveform(tc, &WAVEFORM_OPT);
    tc_start(tc, SYSTEM_TC_CHANNEL);
}




/**
 * \brief On GPIO level change, store values to be handled later
 *
 * \note Cannot use FreeRTOS functions (even *FromISR ones) in this handler
 * because it runs at a higher level of interrupt priority.
 */
__attribute__((section(".exception")))
__attribute__((__interrupt__))
static void gpio_level_irq()
{
    //store first interesting value: time
    irq_fifo[irq_fifo_idx].time = timer_get_value();

    //clear all gpio interrupt flags now.
    //If something happens between now and the end of the interrupt,
    //we will have a new interrupt.
    gpio_port->ifrc = global_gpio_mask;

    //store second interesting value: port B level
    irq_fifo[irq_fifo_idx].value = gpio_port->pvr;

    //increment fifo index
    INC_FIFO_INDEX(irq_fifo_idx);
}

//! update ppm signal status from gpio state and time of last change
static void process_ppm(ppm_input_t *ppm, uint32_t gpio_level, rctime_t when)
{
    bool level = ((gpio_level & ppm->gpio_mask) != 0);

    //does the gpio level changed?
    if(level != ppm->prev_level)
    {
        //it's now low
        if(!level)
        {
            //compute pulse length, and check if it is higher than
            //the PPM start of frame detection threshold
            uint16_t pulse = when - ppm->last_toggle_time;
            if(pulse > FRAME_START_PULSE_VALUE)
            {
                ppm->index = 0;
            }
            else
            {
                ppm->pulse_tick[ppm->index] = pulse;
                ppm->index++;

                //defensive check: that should never happen
                if(ppm->index >= MAX_PPM_CHANNEL)
                {
                    ppm->index = 0;
                }
            }
            ppm->active = true;
            ppm->last_toggle_time = when;
        }

        ppm->prev_level = level;
    }
}

//! check if ppm signal has been active lately
//! if we invalidate the signal, all values will be set inactive
void check_ppm_activity(ppm_input_t *ppm, rctime_t when)
{
    if(ppm->active)
    {
        if(((rctime_t)(when - ppm->last_toggle_time)) > INACTIVITY_TIMEOUT_TICK)
        {
            ppm->active = false;
        }
    }
}

// see header for documentation
void rx_ppm_input_process_input()
{
    int irq_fifo_idx_copy;
    int pop_fifo_idx_p1;
    rctime_t current_time;
    uint32_t gpio_level;

    for(;;)
    {
        //make a copy of irq fifo index, disable interrupt to avoid race condition
        Disable_interrupt_level(RX_IRQ_LEVEL);
        irq_fifo_idx_copy = irq_fifo_idx;
        Enable_interrupt_level(RX_IRQ_LEVEL);

        //copy pop index and add 1 to compare it to copied irq index
        pop_fifo_idx_p1 = pop_fifo_idx;
        INC_FIFO_INDEX(pop_fifo_idx_p1);

        //if pop_idx+1 == irq_idx, that means fifo is empty and we can leave
        if(irq_fifo_idx_copy == pop_fifo_idx_p1)
        {
            break;
        }
        else
        {
            //pop fifo tail
            current_time = irq_fifo[pop_fifo_idx].time;
            gpio_level = irq_fifo[pop_fifo_idx].value;
            //increment index
            INC_FIFO_INDEX(pop_fifo_idx);

#if 0
            for(i=0; i<rx_input_nb; i++)
            {
                process_rx(&rx_input[i], gpio_level, current_time);
            }
#endif
            process_ppm(&ppm_input, gpio_level, current_time);
        }
    }

    //check for lack of activity on all active inputs
    current_time = timer_get_value();
#if 0
    for(i=0; i<rx_input_nb; i++)
    {
        check_rx_activity(&rx_input[i], current_time);
    }
#endif
    check_ppm_activity(&ppm_input, current_time);
}

// see header for documentation

void ppm_input_get_value(uint8_t channel_no, core_input_t *out_val)
{
    if(channel_no < MAX_PPM_CHANNEL)
    {
        out_val->value = ppm_input.pulse_tick[channel_no];
        out_val->active = ppm_input.active;
    }
}


// see header for documentation
void ppm_input_init()
{
    int pin = PPM_INPUT_PIN;
    timer_init();
    uint32_t mask = 1 << (pin & 0x1F);

    ppm_input.gpio_mask = mask;
    global_gpio_mask |= mask;

    ppm_input.prev_level = gpio_get_pin_value(pin);
    ppm_input.last_toggle_time = 0;
    ppm_input.active = false;
    ppm_input.index = 0;

    //register interrupt handler for all gpio event
    Disable_global_interrupt();
    interrupt_register_handler(AVR32_GPIO_GROUP, RX_IRQ_LEVEL, &gpio_level_irq);
    Enable_global_interrupt();

    //now interrupt can be enabled on ppm gpio
    gpio_configure_pin(pin, GPIO_DIR_INPUT | GPIO_PULL_UP | GPIO_INTERRUPT | GPIO_BOTHEDGES);
    gpio_enable_pin_interrupt(pin, GPIO_PIN_CHANGE);
}


#if 0 /*raw servo input, disable for now*/

/**
 * Cleanup rx_input module
 */
void rx_module_cleanup(void);


/**
 * Cleanup of rx_input module
 */
void rx_input_channel_cleanup();



/**
 * \brief Get the latest value of one Rx signal
 * \param signal_no number of the Rx signal you need to read
 * \param[out] value latest measured value in timer tick
 */
void rx_input_get_value(uint8_t channel_no, core_input_t *out_val);

/**
 * Initialize rx_input module
 */
void rx_module_init(void);


void rx_input_channel_init(uint8_t channel_no);


const io_class_t rx_input_class = {
    .name = "RC receiver input",

    .init_module = rx_module_init,
    .init_channel = rx_input_channel_init,
    .cleanup_channel = rx_input_channel_cleanup,
    .cleanup_module = rx_module_cleanup,

    .pre = rx_ppm_input_process_input,
    .get = rx_input_get_value,
    .set = NULL,
    .post = NULL,
};

typedef struct
{
    uint32_t gpio_mask;             //!< mask used for direct register comparison
    bool prev_level;                //!< 0 = low, 1 = high

    rctime_t last_toggle_time;       //!< absolute time of gpio modification

    bool active;                    //!< any activity lately on this gpio?
    uint16_t pulse_tick;            //!< last measured pulse width in timer ticks
} rx_input_t;

void rx_module_init()
{
    timer_init();
    rx_input = (rx_input_t*) malloc(rx_input_nb * sizeof(rx_input_t));
    global_gpio_mask = 0;

    //enable all port A gpio interrupts
    Disable_global_interrupt();
    interrupt_register_handler(AVR32_GPIO_GROUP, RX_IRQ_LEVEL, &gpio_level_irq);
    Enable_global_interrupt();
}

// see header for documentation
void rx_input_channel_init(uint8_t channel_no)
{
    uint32_t mask;

    int pin = IO_BB_PINS[channel_no];

    mask = 1 << (pin & 0x1F);

    gpio_configure_pin(pin, GPIO_DIR_INPUT | GPIO_PULL_UP | GPIO_INTERRUPT | GPIO_BOTHEDGES);
    rx_input[channel_no].gpio_mask = mask;
    rx_input[channel_no].prev_level = gpio_get_pin_value(pin);
    rx_input[channel_no].last_toggle_time = 0;
    rx_input[channel_no].active = false;
    rx_input[channel_no].pulse_tick = DEFAULT_PULSE_VALUE;
    global_gpio_mask |= mask;
    gpio_enable_pin_interrupt(pin, GPIO_PIN_CHANGE);
}

//! check if rx signal has been active lately
void check_rx_activity(rx_input_t *rx, rctime_t when)
{
    if(rx->active)
    {
        if(((rctime_t)(when - rx->last_toggle_time)) > INACTIVITY_TIMEOUT_TICK)
        {
            rx->active = false;
        }
    }
}


//! update rx signal status from gpio state and time of last change
static void process_rx(rx_input_t *rx, uint32_t gpio_level, rctime_t when)
{
    bool level = ((gpio_level & rx->gpio_mask) != 0);

    //does the gpio level changed?
    if(level != rx->prev_level)
    {
        //it's now high
        if(level)
        {
            rx->last_toggle_time = when;
        }
        //it's now low
        else
        {
            rx->pulse_tick = when - rx->last_toggle_time;
            //rx_input[i].last_toggle_time = now; //not really necessary
            rx->active = true;
        }

        rx->prev_level = level;
    }
}


// see header for documentation
void rx_input_get_value(uint8_t channel_no, core_input_t *out_val)
{
    if(channel_no < rx_input_nb)
    {
        out_val->value = rx_input[channel_no].pulse_tick;
        out_val->active = rx_input[channel_no].active;
    }
}

// see header for documentation
void rx_input_channel_cleanup(uint8_t channel_no)
{
    free(rx_input);
    rx_input_nb = 0;
}

void rx_module_cleanup()
{
}
#endif
