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

#include "rc_utils.h"


//see header
dsp16_t sat_add16(dsp16_t a, dsp16_t b)
{
    dsp32_t tmp = (dsp32_t)a+(dsp32_t)b;
    if(tmp > DSP16_MAX)
        return DSP16_MAX;
    else if(tmp < DSP16_MIN)
        return DSP16_MIN;
    else
        return (dsp16_t)tmp;
}


//see header
rc_value_t execute_sequence(const sequence_point_t *pts, dsp16_t dt, rc_value_t amp)
{
    const sequence_point_t *prev = pts;
    const sequence_point_t *next = pts;

    //find the two points surrounding the current position in the timeline
    do
    {
        if(dt > pts->t)
        {
            prev = pts;
        }
        else
        {
            next = pts;
            break;
        }
        pts++;
    } while(pts->t!=-1 || pts->y!=-1);

    //original formula is : y0 + (y1-y0)*(t-t0)/(t1-t0)
    return 2*dsp16_op_mul(amp, ( prev->y/2 + dsp16_op_div(dsp16_op_mul((next->y/2-prev->y/2), dt-prev->t),  next->t-prev->t) ));
}

//see header
rc_value_t reverse_rc(rc_value_t in)
{
    return -in;
}

//see header
rc_value_t dead_zone(rc_value_t order, rc_value_t zone)
{
    if(dsp16_op_abs(order) < zone)
        return 0;
    else
    {
        //substract zone value to avoid having a step around deadzone
        if(order < 0)
            return order + zone;
        else
            return order - zone;
    }
}

//see header
rc_value_t integrate(rc_value_t current, rc_value_t order, dsp16_t k)
{
    int32_t result = current + dsp16_op_mul(order, k);

    /*take care of saturation*/
    if(result <= RC_VALUE_MIN)
        return RC_VALUE_MIN;
    else if(result >= RC_VALUE_MAX)
        return RC_VALUE_MAX;
    else
        return (rc_value_t) result;
}

//see header
rc_value_t slow_down(rc_value_t current, rc_value_t goal, dsp16_t k)
{
    if(dsp16_op_abs(goal-current) < k)
    {
        return goal;
    }

    if((goal-current) < 0)
        return current - k;
    else
        return current + k;
}

//see header
rc_value_t delta_ramp(rc_value_t y1, rc_value_t y0, dsp16_t t)
{
    if(t < DSP16_Q(0.5f))
    {
        //original formula is : y0 + (y1-y0)*t/(dT/2)
        //it has been tweaked a little to avoid overflow
        return 2*(y0/2 + 2*dsp16_op_mul((y1/2-y0/2),t) );
    }
    else
    {
        //y1 - (y1-y0)*(t-dT/2)/(dT/2)
        return 2*(y1/2 - 2*dsp16_op_mul((y1/2-y0/2),(t-DSP16_Q(0.5f))) );
    }
}

