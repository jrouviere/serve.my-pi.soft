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

#include "openscb_dev.h"
#include "openscb_dev_priv.h"


//see header
void scb_close_board(openscb_dev d)
{
    _openscb_dev *dev = (_openscb_dev *)d;
    dev->close(dev->data);
}

//see header
int scb_send_pure_raw_message(openscb_dev d, pccomm_packet_t *packet, int timeout)
{
    _openscb_dev *dev = (_openscb_dev *)d;
    return dev->send(dev->data, packet, timeout);
}

//see header
int scb_get_pure_raw_message(openscb_dev d, pccomm_packet_t *packet, int timeout)
{
    _openscb_dev *dev = (_openscb_dev *)d;
    return dev->get(dev->data, packet, timeout);
}

int scb_get_debug_message(openscb_dev d, char *data, int size, int timeout)
{
    _openscb_dev *dev = (_openscb_dev *)d;
    return dev->debug_get(dev->data, data, size, timeout);
}
