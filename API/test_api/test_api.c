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
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <usb.h>

#include "pc_comm_api.h"
#include "pc_comm_const.h"
#include "openscb_usb.h"
#include "openscb.h"



int main(void)
{
    int ret;

    openscb_dev dev = usb_connect_first_board();

    if(dev)
    {
        bool compat;
        char version[64];
        ret = scb_check_firmware_version(dev, &compat, version, 64);
        printf("Version: %s, compatible: %s\n", version, compat?"OK":"No");

        uint8_t f_speed[] = {1, 2, 3, 4, 5, 0, 0, 7, 8, 9};
        scb_set_out_speed(dev, f_speed, 10);

        float f_outs[] = {-1.0, -0.75, -0.5, -0.25, 0, 0.25, 0.5, 0.75, 1.0, 0.8};
        scb_set_output_goal(dev, f_outs, 10);

        uint8_t nb=0;
        ret = scb_get_output_nb(dev, &nb);
        printf("ret: %d\n", ret);
        ret = scb_get_output_values(dev, f_outs, nb);
        printf("ret: %d\n", ret);

        printf("received: [%d] ", nb);

        int i;
        for(i=0; i<nb; i++) printf("%.2f ", f_outs[i]);

        printf("\n");

        scb_close_board(dev);
    }


    printf("Done\n");


    return 0;
}

