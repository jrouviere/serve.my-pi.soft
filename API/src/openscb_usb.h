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

#ifndef OPENSCB_USB_H_
#define OPENSCB_USB_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SCB_USE_LIBUSB
#include "openscb_dev.h"

/**
 * Get the number of board connected to USB
 */
int usb_get_openscb_nb(void);

/**
 * Connect to the first board found on USB.
 */
openscb_dev usb_connect_first_board(void);


#endif /*SCB_USE_LIBUSB*/

#ifdef __cplusplus
}
#endif

#endif /* OPENSCB_USB_H_ */
