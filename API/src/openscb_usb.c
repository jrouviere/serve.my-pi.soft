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

#ifdef SCB_USE_LIBUSB

#include "pc_comm_api.h"
#include "openscb_usb.h"
#include "openscb_dev_priv.h"

#include <usb.h>
#include <stdio.h>

/*for the moment only one static device structure
 *=> only one device loaded by one instance of the library*/
static _openscb_dev _dev;

/* Has to be called once at application startup */
static bool initialized = false;
static void usb_init_api()
{
    if(!initialized)
    {
        usb_init();
        initialized = true;
    }

    usb_find_busses();
    usb_find_devices();
}

//see header
static openscb_dev find_first_openscb(void)
{
    struct usb_bus *bus;
    struct usb_device *dev;

    for (bus = usb_get_busses(); bus; bus = bus->next)
    {
        for (dev = bus->devices; dev; dev = dev->next)
        {
            if((dev->descriptor.bDeviceClass == OPENSCB_CLASS)
                && (dev->descriptor.idVendor == OPENSCB_VID)
                && (dev->descriptor.idProduct == OPENSCB_PID))
            {
                return usb_open(dev);
            }
        }
    }
    return NULL;
}


static int usb_get_dev_count(uint8_t class, uint16_t vid, uint16_t pid)
{

    struct usb_bus *bus;
    struct usb_device *dev;
    int nb = 0;

    usb_init_api();

    for(bus=usb_get_busses(); bus; bus=bus->next)
    {
        for(dev=bus->devices; dev; dev=dev->next)
        {
            if((dev->descriptor.idVendor == vid)
                && (dev->descriptor.idProduct == pid))
            {
                nb++;
            }
        }
    }

    return nb;
}

//see header
int usb_get_openscb_nb(void)
{
    return usb_get_dev_count(OPENSCB_CLASS, OPENSCB_VID, OPENSCB_PID);
}

int usb_get_dfuprog_nb(void)
{
    return usb_get_dev_count(DFU_BTLDR_CLASS, DFU_BTLDR_VID, DFU_BTLDR_PID);
}


static void usb_dev_close(void *dev)
{
    usb_release_interface(dev, OPENSCB_INTERF);
    usb_close(dev);
}

static int usb_dev_send(void *dev, pccomm_packet_t *packet, int timeout)
{
    return usb_bulk_write(dev, OPENSCB_SLAVE_EP_SEND, (char*)packet, packet->header.size, timeout);
}

static int usb_dev_get(void *dev, pccomm_packet_t *packet, int timeout)
{
    int ret = usb_bulk_read(dev, OPENSCB_SLAVE_EP_RCV, (char*)packet, MAX_PACKET_SIZE, timeout);
    packet->header.size = ret;

    return ret;
}


static int usb_debug_get(void *dev, char *data, int size, int timeout)
{
    int ret = usb_bulk_read(dev, OPENSCB_MASTER_EP_RCV, data, size, timeout);

    return ret;
}


//see header
openscb_dev usb_connect_first_board()
{
    usb_init_api();

    struct usb_dev_handle *dev = find_first_openscb();
    if(dev == NULL)
    {
        return NULL;
    }

    if (usb_set_configuration(dev, OPENSCB_CFG) < 0)
    {
        printf("error setting config #%d: %s\n", OPENSCB_CFG, usb_strerror());
        usb_close(dev);
        return NULL;
    }

    if (usb_claim_interface(dev, OPENSCB_INTERF) < 0)
    {
        printf("error claiming interface #%d:\n%s\n", OPENSCB_INTERF, usb_strerror());
        usb_close(dev);
        return NULL;
    }

    /*setup the device structure and return it*/
    _dev.data = dev;
    _dev.close = usb_dev_close;
    _dev.send = usb_dev_send;
    _dev.get = usb_dev_get;
    _dev.debug_get = usb_debug_get;
    return (void*)&_dev;
}

#endif /*SCB_USE_LIBUSB*/

