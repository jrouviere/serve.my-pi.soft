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

#include "out_ctrl.h"

#include "core.h"
#include "api_ctrl.h"
#include "appli/appli_ctrl.h"

#define SPEED_COEF  8

static rc_value_t speed_limit(int out_no, rc_value_t goal)
{
    uint8_t speed = core_get_sys_conf()->out_conf[out_no].max_speed;
    rc_value_t new_pos;
    rc_value_t cur_pos;

    //get output current position
    cur_pos = core_get_output(out_no)->value;


    //compute new position,
    //0 means no speed limit
    if(speed == 0)
    {
        new_pos = goal;
    }
    else if(cur_pos < goal)
    {
        new_pos = MIN(cur_pos + SPEED_COEF*speed, goal);
    }
    else if(cur_pos > goal)
    {
        new_pos = MAX(cur_pos - SPEED_COEF*speed, goal);
    }
    else
    {
        new_pos = goal;
    }

    return new_pos;
}


void out_ctrl_get_value(const system_conf_t *sys_conf, const core_input_t *inputs, core_output_t *outputs)
{
    int i;
    core_output_t tmp[MAX_OUT_NB];

    for(i=0; i<MAX_OUT_NB; i++)
    {
        tmp[i].active = false;
    }

    /* Appli has priority over API, that's why it is called last,
     * there it can overwrite values from API*/
    api_ctrl_update(tmp);
    appli_update(inputs, tmp);

    /* Update core outputs, don't modify value if we don't need to,
     * that way we can remember last servo position to apply
     * correct speed control */
    for(i=0; i<sys_conf->output_nb; i++)
    {
        if(tmp[i].active)
        {
            outputs[i].value = speed_limit(i, tmp[i].value);
            outputs[i].active = true;
        }
        else
        {
            outputs[i].active = false;
        }
    }
}

