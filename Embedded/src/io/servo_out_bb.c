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

//see header for overview and documentation

#include "servo_out_bb.h"

#include "board.h"
#include "tc.h"
#include "gpio.h"
#include "trace.h"

#include "interrupt.h"
#include "system.h"

#include "avr32_interrupt.h"

#define AVR32_TC_GROUP              IRQ_TO_GROUP(AVR32_TC_IRQ0)
#define MAX_UINT16                  0xFFFF

//! timer channel used by the servo_out_bb module
#define SERVOBB_TC_CHANNEL          0

//! interrupt priority level used by the servo_out_bb module
//! (should be one of the highest to avoid jitter)
#define SERVOBB_TC_IRQ_LEVEL        3

//! send a pulse to each servos every N ms
#define REFRESH_PERIOD_IN_US        14000

//! protection against strange pulse values
#define MAX_SERVO_PULSE             US_TO_TC4_TICK(4000)
#define MIN_SERVO_PULSE             US_TO_TC4_TICK(100)

//! maximum number of servo queues
//! that correspond to the number of servo handled in parallel
#define MAX_QUEUE_NB                5

//! Absolute maximum servo that can be connected to the system
#define SERVO_MAX_NB                IO_BB_MAX_NB


#define TOGGLE_GPIO(step) do {                               \
AVR32_GPIO_LOCAL.port[0].ovrt = step->gpio_toggle;           \
} while(0)

#define GPIO_REG_SET(reg, pin)      reg |= 1 << ((pin) & 0x1F)
#define GPIO_REG_RESET(reg) do {                     \
        reg = 0;                                     \
} while(0)


#define IS_ACTIVE(no) (servo_active & (1 << (no)))
#define SET_ACTIVE(no) servo_active |= (1 << (no))
#define CLEAR_ACTIVE(no) servo_active &= ~(1 << (no))

#define IS_ENABLED(no) (servo_enabled & (1 << (no)))
#define SET_ENABLED(no) servo_enabled |= (1 << (no))
#define CLEAR_ENABLED(no) servo_enabled &= ~(1 << (no))


//! Structure to store all data concerning one servo
//! to optimize memory, two bitfield servo_active/servo_enable have been
//! extracted from this structure.
typedef struct {
    uint16_t timer_value;           //!< servo pulse width in timer tick
    uint16_t time_left;             //!< time left in the pulse (temp variable for algorithm)
} servo_t;

//! A servo queue is a list of servo that will be handled sequentially
typedef struct {

    uint8_t servo[SERVO_MAX_NB/MAX_QUEUE_NB+1];    //!< index of servos present in this queue
    uint8_t servo_nb;                              //!< number of servo in this queue

    uint8_t servo_index;                           //!< servo we are currently generating pulse (temp variable for algorithm)
} servo_queue_t;

//! structure that store orders for the IRQ handler
typedef struct
{
    uint32_t gpio_toggle;   //!< bitfield of gpio number to be toggled

    uint16_t time_to_next_step;         //!< timer value
} servo_step_t;


//! Module internal servo array
static uint32_t servo_active;
static uint32_t servo_enabled;
static servo_t servo [SERVO_MAX_NB];

//we might need up to one step per servo + 1 step for refresh period
static servo_step_t step_buffer1[SERVO_MAX_NB+1];  //!< double buffering, buffer 1
static servo_step_t step_buffer2[SERVO_MAX_NB+1];  //!< double buffering, buffer 2

//array of steps currently executed by the IRQ handler
static servo_step_t *current_step;
static servo_step_t *current_step_first;
static servo_step_t *current_step_last;

//next buffer the IRQ will be using (could be the same or the second one)
static servo_step_t *next_step_first;
static servo_step_t *next_step_last;


const io_class_t servobb_output_class = {
    .name = "RC servo output",

    .init_module = servobb_module_init,
    .init_channel = servobb_channel_init,
    .cleanup_channel = servobb_channel_cleanup,
    .cleanup_module = servobb_module_cleanup,

    .pre = NULL,
    .get = servobb_get_value,
    .set = servobb_set_value,
    .post = servobb_apply_values,
};


//! helper function: reset all servo queue structures
static void init_servo_queues(servo_queue_t *queue)
{
    int i;
    int q_nb = 0;

    for(i=0; i<MAX_QUEUE_NB; i++)
    {
        queue[i].servo_nb = 0;
    }

    for(i=0; i<SERVO_MAX_NB; i++)
    {
        if(IS_ENABLED(i))
        {
            queue[q_nb].servo[queue[q_nb].servo_nb] = i;
            queue[q_nb].servo_nb++;
            //reload timer values
            servo[i].time_left = servo[i].timer_value;
            //alternate queues to put our new servos in
            q_nb++;
            if(q_nb >= MAX_QUEUE_NB)
                q_nb = 0;
        }
    }

    for(i=0; i<MAX_QUEUE_NB; i++)
    {
        //reset servo index
        queue[i].servo_index = 0;
    }
}

//! helper function: compute the time needed to wait before the next servo event (pulse down)
static void find_next_time_left(servo_queue_t *queue, uint16_t *min_time_left)
{
    int i;
    for(i=0; i<MAX_QUEUE_NB; i++)
    {
        int ind = queue[i].servo_index;
        if(ind < queue[i].servo_nb)
        {
            uint8_t sv_no = queue[i].servo[ind];
            if(servo[sv_no].time_left < *min_time_left)
            {
                *min_time_left = servo[sv_no].time_left;
            }
        }
    }
}


/**
 * \brief TC interrupt handler, execute orders stored in the step structure
 *
 * \note Cannot use FreeRTOS functions (even *FromISR ones) in this handler
 * because it runs at a higher level of interrupt priority.
 */
__attribute__((section(".exception")))
__attribute__((__interrupt__))
static void servo_hard_tc_irq(void)
{
    //clear interrupt flag, prepare and start timer for next interrupt
    AVR32_TC.channel[SERVOBB_TC_CHANNEL].sr; //clear interrupt
    AVR32_TC.channel[SERVOBB_TC_CHANNEL].rc = current_step->time_to_next_step;
    AVR32_TC.channel[SERVOBB_TC_CHANNEL].ccr = AVR32_TC_SWTRG_MASK | AVR32_TC_CLKEN_MASK;

    TOGGLE_GPIO(current_step);

    current_step++;

    //this was the last step, switch to next steps set (could be the same or new ones)
    if(current_step > current_step_last)
    {
        current_step = next_step_first;
        current_step_first = next_step_first;
        current_step_last = next_step_last;
    }
}

/**
 * Generate steps structure for all servos in the system.
 *
 * This is the main function of the module, it computes all orders for the IRQ handler
 * it's using a temporary buffer to store the result without disturbing the IRQ handler.
 * Here we assume that the argument is an array of steps which contains enough elements,
 * this function returns the last initialized element of the array
 */
static servo_step_t *generate_servo_step(servo_step_t *new_step)
{
    int i;
    servo_queue_t queue[MAX_QUEUE_NB];

    //time left to the end of refresh frame
    int32_t refresh_period_needed = US_TO_TC4_TICK( REFRESH_PERIOD_IN_US );

    //reset all temporary values in servo queues
    init_servo_queues(queue);

    //we start all first servo pulses
    GPIO_REG_RESET(new_step->gpio_toggle);
    for(i=0; i<MAX_QUEUE_NB; i++)
    {
        if((queue[i].servo_nb > 0))
        {
            uint8_t sv_no = queue[i].servo[0];
            if(IS_ACTIVE(sv_no))
            {
                GPIO_REG_SET(new_step->gpio_toggle, IO_BB_PINS[sv_no]);
            }
        }
    }

    //find which of the current pulses finish first
    uint16_t min_time_left = MAX_UINT16;
    find_next_time_left(queue, &min_time_left);

    while(min_time_left != MAX_UINT16)
    {
        //complete the step by storing delay value to next step
        new_step->time_to_next_step = min_time_left;
        refresh_period_needed -= min_time_left;

        //init next step
        new_step++;
        GPIO_REG_RESET(new_step->gpio_toggle);

        for(i=0; i<MAX_QUEUE_NB; i++)
        {
            int ind = queue[i].servo_index;
            if(ind < queue[i].servo_nb)
            {
                uint8_t sv_no = queue[i].servo[ind];
                //decrement timer value for each servo
                servo[sv_no].time_left -= min_time_left;

                //if pulse is completed, stop it and switch to next servo
                if(servo[sv_no].time_left == 0)
                {
                    //stop current pulse
                    if(IS_ACTIVE(sv_no))
                    {
                        GPIO_REG_SET(new_step->gpio_toggle, IO_BB_PINS[sv_no]);
                    }
                    queue[i].servo_index++;

                    //start pulse on next servo (if any)
                    ind = queue[i].servo_index;
                    if(ind < queue[i].servo_nb)
                    {
                        uint8_t sv_no = queue[i].servo[ind];
                        if(IS_ACTIVE(sv_no))
                        {
                            GPIO_REG_SET(new_step->gpio_toggle, IO_BB_PINS[sv_no]);
                        }
                    }
                }
            }
        }

        min_time_left = MAX_UINT16;
        find_next_time_left(queue, &min_time_left);
    }

    /*while(refresh_period_needed > MAX_UINT16)
    {
        refresh_period_needed -= MAX_UINT16;
        new_step->time_to_next_step = MAX_UINT16;
        new_step++;
        GPIO_REG_RESET(new_step->gpio_toggle);
    }*/
    new_step->time_to_next_step = refresh_period_needed;

    //return the last step
    return new_step;
}

//! Initialize timer module to generate interrupt at precise timing
static void init_tc()
{
    // Options for waveform generation.
    static const tc_waveform_opt_t WAVEFORM_OPT =
    {
      .channel  = SERVOBB_TC_CHANNEL,                // Channel selection.

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
      .cpcstop  = TRUE,                              // Counter clock stopped with RC compare.

      .burst    = FALSE,                             // Burst signal selection.
      .clki     = FALSE,                             // Clock inversion.
      .tcclks   = TC_CLOCK_SOURCE_TC4                // Internal source clock 4, connected to fPBA / 32.
    };

    static const tc_interrupt_t TC_INTERRUPT =
    {
      .etrgs = 0,
      .ldrbs = 0,
      .ldras = 0,
      .cpcs  = 1,   //compare to rc
      .cpbs  = 0,
      .cpas  = 0,
      .lovrs = 0,
      .covfs = 0
    };

    // init tc interrupt
    Disable_global_interrupt();
    interrupt_register_handler(AVR32_TC_GROUP, SERVOBB_TC_IRQ_LEVEL, &servo_hard_tc_irq);
    Enable_global_interrupt();

    // init timer
    tc_init_waveform(&AVR32_TC, &WAVEFORM_OPT);         // Initialize the timer/counter waveform.
    tc_configure_interrupts(&AVR32_TC, SERVOBB_TC_CHANNEL, &TC_INTERRUPT);
}

void servobb_get_value(uint8_t channel_no, core_input_t *out_val)
{
    out_val->value = servo[channel_no].timer_value;
    out_val->active = IS_ACTIVE(channel_no);
}

// see header for documentation
void servobb_set_value(uint8_t channel_no, const core_output_t *out)
{
    //little protection against excessive pulse
    //which could produce strange results with the sequencing
    rc_raw_t pulse_tick = out->value;

    if(pulse_tick > MAX_SERVO_PULSE)
    {
        pulse_tick = MAX_SERVO_PULSE;
    }
    else if(pulse_tick < MIN_SERVO_PULSE)
    {
        pulse_tick = MIN_SERVO_PULSE;
    }

    if(channel_no < SERVO_MAX_NB)
    {
        if(out->active)
        {
            SET_ACTIVE(channel_no);
            servo[channel_no].timer_value = pulse_tick;
        }
        else
        {
            //here we don't set the value so it stays to the last
            //one actually sent to the servo
            CLEAR_ACTIVE(channel_no);
        }
    }
    else
    {
        TRACE("Incorrect servo number\n");
    }
}

// see header for documentation
bool servobb_apply_values()
{
    //select which buffer we will be using
    if(current_step_first == next_step_first)
    {
        servo_step_t *new_step, *new_step_last;
        if(current_step_first == step_buffer1)
        {
            new_step = step_buffer2;
        }
        else
        {
            new_step = step_buffer1;
        }

        //generate values, this can be long but interrupts are still enabled here
        new_step_last = generate_servo_step(new_step);

        //disable interrupt before swapping buffer to avoid concurrency
        //with irq handler
        Disable_interrupt_level(SERVOBB_TC_IRQ_LEVEL);
        next_step_first = new_step;
        next_step_last = new_step_last;
        Enable_interrupt_level(SERVOBB_TC_IRQ_LEVEL);

        return true;
    }
    else
    {
        return false;
    }
}




void servobb_module_init(void)
{
    int i;
    for(i=0; i<SERVO_MAX_NB; i++)
    {
        servo[i].timer_value = DEFAULT_OUTPUT_RAW_VALUE;
    }

    //initialize step variables
    current_step = step_buffer1;
    current_step_first = step_buffer1;
    current_step_last = generate_servo_step(step_buffer1);

    next_step_first = current_step_first;
    next_step_last = current_step_last;

    //init timer interrupt
    init_tc();

    //start pulse generation
    tc_write_rc(&AVR32_TC, SERVOBB_TC_CHANNEL, DEFAULT_OUTPUT_RAW_VALUE);
    tc_start(&AVR32_TC, SERVOBB_TC_CHANNEL);
}

// see header for documentation
void servobb_channel_init(uint8_t channel_no)
{
    if((channel_no >= SERVO_MAX_NB) ||
        channel_no >= SIZEOF_ARRAY(IO_BB_PINS))
    {
        TRACE("Incorrect servo number\n");
        return;
    }

    servo[channel_no].timer_value = DEFAULT_OUTPUT_RAW_VALUE;
    SET_ENABLED(channel_no);
    gpio_local_enable_pin_output_driver(IO_BB_PINS[channel_no]);
    gpio_local_clr_gpio_pin(IO_BB_PINS[channel_no]);
}

// see header for documentation
void servobb_channel_cleanup(uint8_t channel_no)
{
    if(channel_no < SERVO_MAX_NB && IS_ENABLED(channel_no))
    {
        CLEAR_ENABLED(channel_no);
        CLEAR_ACTIVE(channel_no);

        gpio_local_disable_pin_output_driver(IO_BB_PINS[channel_no]);
    }
    else
    {
        TRACE("servo was already disabled\n");
    }
}

void servobb_module_cleanup(void)
{
    tc_stop(&AVR32_TC, SERVOBB_TC_CHANNEL);
}

