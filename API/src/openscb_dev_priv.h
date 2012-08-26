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

#ifndef OPENSCB_DEV_PRIV_H_
#define OPENSCB_DEV_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*close_fn)(void *);
typedef int (*get_fn)(void *, pccomm_packet_t *, int);
typedef int (*send_fn)(void *, pccomm_packet_t *, int);

typedef int (*get_debug_fn)(void *, char *, int, int);

typedef struct {
    close_fn close;
    get_fn get;
    send_fn send;

    get_debug_fn debug_get;

    void *data;
} _openscb_dev;

#ifdef __cplusplus
}
#endif

#endif /* OPENSCB_DEV_PRIV_H_ */
