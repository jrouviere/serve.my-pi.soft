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
#include <stdarg.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "usb_standard_request.h"

#include "trace.h"
#include "pc_comm.h"

#include "usb_drv.h"


//In debug mode we enable the full-featured snprintf trace function
#ifdef TRACE_DEBUG

#define TRACE_BUFFER_SHIFT          10 // 9 is 512B, 10 is 1 kB, 11 is 2 kB...
#define TRACE_BUFFER_SIZE           (1<<TRACE_BUFFER_SHIFT)
#define TRACE_BUFFER_MASK           (TRACE_BUFFER_SIZE-1)

//The buffer is aligned, so we can use a simple mask for the ring buffer logic
static char trace_buffer[TRACE_BUFFER_SIZE]  __attribute__ ((aligned (TRACE_BUFFER_SIZE)));

//current read/write cursor
static char *read_ptr = trace_buffer;
static char *write_ptr = trace_buffer;
//pointer to the end of the ring buffer (used to cut memcpy in two parts)
static const char *end_ptr = (trace_buffer + TRACE_BUFFER_SIZE);

static xSemaphoreHandle trace_mutex;


#endif


static bool avr32_usb_dev_master_send(const char *buffer, int size)
{
    if(!Is_device_enumerated())
    {
        return false;
    }
    if(!Is_usb_write_enabled(MASTER_TX_EP))
    {
        return false;
    }

    while(size)
    {
        Usb_reset_endpoint_fifo_access(MASTER_TX_EP);
        size = usb_write_ep_txpacket(MASTER_TX_EP, buffer, size, (const void**)&buffer);
        Usb_ack_in_ready_send(MASTER_TX_EP);
    }
    return true;
}

//In debug mode we enable the full-featured snprintf trace function
#ifdef TRACE_DEBUG

static uint32_t count_data()
{
    if(write_ptr < read_ptr)
    {
        return TRACE_BUFFER_SIZE - (read_ptr - write_ptr);
    }
    else
    {
        return (write_ptr - read_ptr);
    }
}

/* increment the pointer on the ring buffer,
 * buf is the base of the ring,
 * ptr is the pointer navigating on the ring
 * inc is the number of cell to increment
 */
#define INC_PTR(buf, ptr, inc) (char*)((uint32_t)(buf) | ((uint32_t)(ptr+inc) & TRACE_BUFFER_MASK))
#define TMP_BUF_SIZE    128

// see header for documentation
void trace(const char *format, ...)
{
    static char buf[TMP_BUF_SIZE];
    uint32_t size;
    uint32_t i=0;

    va_list args;
    va_start(args, format);

    /*we use a static temp buffer, and a global ring buffer,
    so we need to protect against simultaneous writers*/
    if(xSemaphoreTake(trace_mutex, portMAX_DELAY))
    {
        size = vsnprintf(buf, TMP_BUF_SIZE, format, args);

        //add the formatted string to the ring buffer
        while((i < size) && (INC_PTR(trace_buffer, write_ptr, 1) != read_ptr))
        {
            /* special case: buffer is nearly full, mark the truncature
             * with a fancy symbol "[..]" and stop
             */
            if(INC_PTR(trace_buffer, write_ptr, 5+1) == read_ptr)
            {
                *write_ptr = '[';
                write_ptr = INC_PTR(trace_buffer, write_ptr, 1);
                *write_ptr = '.';
                write_ptr = INC_PTR(trace_buffer, write_ptr, 1);
                *write_ptr = '.';
                write_ptr = INC_PTR(trace_buffer, write_ptr, 1);
                *write_ptr = ']';
                write_ptr = INC_PTR(trace_buffer, write_ptr, 1);
                *write_ptr = '\n';
                write_ptr = INC_PTR(trace_buffer, write_ptr, 1);
                break;
            }
            *write_ptr = buf[i];
            i++;
            write_ptr = INC_PTR(trace_buffer, write_ptr, 1);
        }

        xSemaphoreGive(trace_mutex);
    }


    va_end(args);

    //TODO post sem
}

void trace_task(void *arg)
{
    vTaskDelay(1000);

    for(;;)
    {
        uint32_t size;

        //TODO: wait sem
        uint32_t count = count_data();
        size = MIN(count, (end_ptr - read_ptr));
        size = MIN(size, MAX_PACKET_SIZE);
        if(size>0 && avr32_usb_dev_master_send(read_ptr, size))
        {
            read_ptr = INC_PTR(trace_buffer, read_ptr, size);
        }
        else
        {
            vTaskDelay(10);
        }
    }
}

void trace_init(void)
{
    //Create trace task
    trace_mutex = xSemaphoreCreateMutex();
    if(trace_mutex == NULL)
    {
        FATAL_ERROR();
    }

    xTaskCreate(trace_task,
                configTSK_TRACE_NAME,
                configTSK_TRACE_STACK_SIZE,
                NULL,
                configTSK_TRACE_PRIORITY,
                NULL);

}

#else

// see header for documentation
void trace(const char *log, ...)
{
    //push the buffer without formatting
    avr32_usb_dev_master_send(log, strlen(log));
}

void trace_init(void)
{
}

#endif

