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
#include "pc_comm.h"
#include "gpio.h"
#include "usart.h"
#include "sys_conf.h"

#include "FreeRTOS.h"
#include "task.h"

#include <limits.h>
#include <stdlib.h>


#define BUILD_CMD(a,b) ((a << 8U) | b)

#define CMD_ON		BUILD_CMD('O', 'N')
#define CMD_OF		BUILD_CMD('O', 'F')
#define CMD_SP		BUILD_CMD('S', 'P')
#define CMD_SS		BUILD_CMD('S', 'S')

/**
 * Get a message from uart
 * 
 * \param out_msg[out] buffer to store the message
 * \param max_len size of the buffer
 * \return size of message read from uart
 */
static int get_message(uint8_t *out_msg, int max_len)
{
	int i = 0;
	for(;;)
	{
		int rx = usart_getchar(RPI_USART);
		if(rx == USART_FAILURE)
		{
			//abort
			return 0;
		}
		else
		{
			out_msg[i] = rx;
			i++;
			
			//no more space in buffer: abort
			if(i >= max_len)
			{
				return 0;
			}
			
			//stop on carriage return char
			if(rx == '\n')
			{
				//null terminate the string
				out_msg[i] = '\0';
				return i+1;
			}
		}
	}
	return 0; //should never get there
}

static const char *skip_spaces(const char *msg)
{
	for(;;)
	{
		char c = *msg;
		
		if(c == '\0')
		{
			return NULL;
		}
		
		if( (c != ' ') && (c != ',') && (c != ';') )
		{
			return msg;
		}
		
		msg++;
	}
}

    
/**
 * The goal is to parse a string like this one:
 * "2, 3, 6, 9"
 */
static int parse_int_list(int32_t *out_list, int max_len, const char *msg)
{
	int i;
	
	for(i=0; (i<max_len) && (msg!=NULL); i++)
	{
		char *end;
		
		//get first int value from msg
		int32_t val = strtol(msg, &end, 10);
		if( (val == LONG_MAX) || (val == LONG_MIN) )
		{
			//not a valid number, abort! abort!
			return 0;
		}
		
		out_list[i] = val;
		
		//skip spaces, commas and semi-colons
		msg = skip_spaces(end);
	}
	
	return i;
}

static void handle_cmd_on(const char *data)
{
	int nb;
	int32_t list[IO_BB_MAX_NB];
	
	nb = parse_int_list(list, SIZEOF_ARRAY(list), data);
}

static void handle_cmd_of(const char *data)
{
	int nb;
	int32_t list[IO_BB_MAX_NB];
	
	nb = parse_int_list(list, SIZEOF_ARRAY(list), data);
}

/**
 * Read data from uart and handle to appropriate handler
 */
static void uart_task(void *args)
{
	static uint8_t msg[256];
	
    for(;;)
    {
		int sz = get_message(msg, sizeof(msg));
		if(sz > 2)
		{
			uint16_t cmd = BUILD_CMD(msg[0], msg[1]);
			
			switch(cmd) {
			case CMD_ON:
				handle_cmd_on((char*)(msg+2));
				break;
				
			case CMD_OF:
				handle_cmd_of((char*)(msg+2));
				break;
				
			case CMD_SP:
				break;
				
			case CMD_SS:
				break;
				
			default:
				break;
			};
		}
    }
}


void uart_init()
{
	static const gpio_map_t USART_GPIO_MAP =
	{
		{RPI_USART_RX_PIN, RPI_USART_RX_FUNCTION},
		{RPI_USART_TX_PIN, RPI_USART_TX_FUNCTION}
	};

	static const usart_options_t USART_OPTIONS =
	{
	.baudrate     = 57600,
	.charlength   = 8,
	.paritytype   = USART_NO_PARITY,
	.stopbits     = USART_1_STOPBIT,
	.channelmode  = USART_NORMAL_CHMODE
	};

	/* Init device */
	gpio_enable_module(USART_GPIO_MAP, sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));
	usart_init_rs232(RPI_USART, &USART_OPTIONS, APPLI_PBA_SPEED);
		

	/* Start uart task */
    xTaskCreate(uart_task,
                configTSK_UART_NAME,
                configTSK_UART_STACK_SIZE,
                NULL,
                configTSK_UART_PRIORITY,
                NULL);
}
