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


#ifndef OPENSCB_DEV_H_
#define OPENSCB_DEV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pc_comm_api.h"

#define DEFAULT_TIMEOUT     1000

/*opaque type*/
#define openscb_dev void*

/**
 * close a previously opened board
 */
void scb_close_board(openscb_dev dev);


/**
 * \return the number of byte actually received, or negative number on error/timeout
 */
int scb_get_pure_raw_message(openscb_dev dev, pccomm_packet_t *packet, int timeout);


/**
 * Hardware abstraction api, use send_raw_message instead
 */
int scb_send_pure_raw_message(openscb_dev dev, pccomm_packet_t *packet, int timeout);

#ifdef __cplusplus
}
#endif

#endif /* OPENSCB_DEV_H_ */
