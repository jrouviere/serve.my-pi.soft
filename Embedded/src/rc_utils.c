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


/*rough atan2 approximation for fixed point (< 3% error)*/
dsp16_t dsp16_op_atan2f_emu(dsp16_t y, dsp16_t x)
{
    dsp16_t r, angle;
    dsp16_t coeff_1 = DSP16_PI_4;
    dsp16_t coeff_2 = 3*coeff_1;
    dsp16_t abs_y = dsp16_op_abs(y)+1;
    if(x>=0)
    {
        r = dsp16_op_div((x-abs_y), (x+abs_y));
        angle = coeff_1 - dsp16_op_mul(coeff_1, r);
    }
    else
    {
        r = dsp16_op_div((x+abs_y), (abs_y-x));
        angle = coeff_2 - dsp16_op_mul(coeff_1, r);
    }
    if (y < 0)
        return(-angle);
    else
        return(angle);
}


//helper, return a*b*c in dsp16_t
#define mul16_3(a,b,c) mul16(mul16(a,b),c)

//see header
void matrix4_identity(matrix4 m)
{
    m[0]=Q(1);  m[4]=Q(0);  m[8]=Q(0);  m[12]=Q(0);
    m[1]=Q(0);  m[5]=Q(1);  m[9]=Q(0);  m[13]=Q(0);
    m[2]=Q(0);  m[6]=Q(0);  m[10]=Q(1); m[14]=Q(0);
    m[3]=Q(0);  m[7]=Q(0);  m[11]=Q(0); m[15]=Q(1);
}

//see header
void matrix4_rotation_yxz_translation(dsp16_t rx, dsp16_t ry, dsp16_t rz, dsp16_t dx, dsp16_t dy, dsp16_t dz, matrix4 m)
{
    dsp16_t c1=cos16(ry); dsp16_t c2=cos16(rx); dsp16_t c3=cos16(rz);
    dsp16_t s1=sin16(ry); dsp16_t s2=sin16(rx); dsp16_t s3=sin16(rz);

    m[0]=mul16(c1,c3)+mul16_3(s1,s2,s3);    m[4]=mul16_3(c3,s1,s2)-mul16(c1,s3);    m[8]=mul16(c2,s1);  m[12]=dx;
    m[1]=mul16(c2,s3);                      m[5]=mul16(c2,c3);                      m[9]=-s2;           m[13]=dy;
    m[2]=mul16_3(c1,s2,s3)-mul16(c3,s1);    m[6]=mul16(s1,s3)+mul16_3(c1,c3,s2);    m[10]=mul16(c1,c2); m[14]=dz;
    m[3]=Q(0);                              m[7]=Q(0);                              m[11]=Q(0);         m[15]=Q(1);
}

//see header
void matrix4_inverse_homogeneous(matrix4 m, matrix4 r)
{
    //This is a specific formula for matrix that only involves rotation and translation
    dsp16_t dx = -( mul16(m[0],m[12]) + mul16(m[1],m[13]) + mul16(m[2],m[14]) );
    dsp16_t dy = -( mul16(m[4],m[12]) + mul16(m[5],m[13]) + mul16(m[6],m[14]) );
    dsp16_t dz = -( mul16(m[8],m[12]) + mul16(m[9],m[13]) + mul16(m[10],m[14]) );

    r[0]=m[0];  r[4]=m[1];  r[8]=m[2];      r[12]=dx;
    r[1]=m[4];  r[5]=m[5];  r[9]=m[6];      r[13]=dy;
    r[2]=m[8];  r[6]=m[9];  r[10]=m[10];    r[14]=dz;
    r[3]=Q(0);  r[7]=Q(0);  r[11]=Q(0);     r[15]=Q(1);
}

void matrix4_multiply(const matrix4 m1, const matrix4 m2, matrix4 r)
{
    //temp result to allow passing the same matrix in m1/m2 and r
    matrix4 tmp;
    tmp[0] = mul16(m1[0],m2[0]) + mul16(m1[4],m2[1]) + mul16(m1[8],m2[2]);
    tmp[1] = mul16(m1[1],m2[0]) + mul16(m1[5],m2[1]) + mul16(m1[9],m2[2]);
    tmp[2] = mul16(m1[2],m2[0]) + mul16(m1[6],m2[1]) + mul16(m1[10],m2[2]);
    tmp[3] = Q(0);

    tmp[4] = mul16(m1[0],m2[4]) + mul16(m1[4],m2[5]) + mul16(m1[8],m2[6]);
    tmp[5] = mul16(m1[1],m2[4]) + mul16(m1[5],m2[5]) + mul16(m1[9],m2[6]);
    tmp[6] = mul16(m1[2],m2[4]) + mul16(m1[6],m2[5]) + mul16(m1[10],m2[6]);
    tmp[7] = Q(0);

    tmp[8] = mul16(m1[0],m2[8]) + mul16(m1[4],m2[9]) + mul16(m1[8],m2[10]);
    tmp[9] = mul16(m1[1],m2[8]) + mul16(m1[5],m2[9]) + mul16(m1[9],m2[10]);
    tmp[10] = mul16(m1[2],m2[8]) + mul16(m1[6],m2[9]) + mul16(m1[10],m2[10]);
    tmp[11] = Q(0);

    tmp[12] = mul16(m1[0],m2[12]) + mul16(m1[4],m2[13]) + mul16(m1[8],m2[14]) + m1[12];
    tmp[13] = mul16(m1[1],m2[12]) + mul16(m1[5],m2[13]) + mul16(m1[9],m2[14]) + m1[13];
    tmp[14] = mul16(m1[2],m2[12]) + mul16(m1[6],m2[13]) + mul16(m1[10],m2[14]) + m1[14];
    tmp[15] = Q(1);

    memcpy(r, tmp, sizeof(tmp));
}


void matrix4_tranform_vector(const matrix4 m, const vector3 v, vector3 r)
{
    //temp result to allow passing the same vector in v and r
    vector3 tmp;

    tmp[0] = mul16(m[0],v[0]) + mul16(m[4],v[1]) + mul16(m[8],v[2]) + m[12];
    tmp[1] = mul16(m[1],v[0]) + mul16(m[5],v[1]) + mul16(m[9],v[2]) + m[13];
    tmp[2] = mul16(m[2],v[0]) + mul16(m[6],v[1]) + mul16(m[10],v[2]) + m[14];

    memcpy(r, tmp, sizeof(tmp));
}

void print_matrix4(const char *name, const matrix4 m)
{
    TRACE("%s: %04d %04d %04d %04d\n", name, m[0],m[4],m[8],m[12]);
    TRACE("%s: %04d %04d %04d %04d\n", name, m[1],m[5],m[9],m[13]);
    TRACE("%s: %04d %04d %04d %04d\n", name, m[2],m[6],m[10],m[14]);
    TRACE("%s: %04d %04d %04d %04d\n", name, m[3],m[7],m[11],m[15]);
}

void print_vector3(const char *name, const vector3 v)
{
    TRACE("%s: %04d %04d %04d\n", name, v[0],v[1],v[2]);
}
