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

	gpio_enable_module(USART_GPIO_MAP, sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));

	usart_init_rs232(RPI_USART, &USART_OPTIONS, APPLI_PBA_SPEED);
}
