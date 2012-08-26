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
#include "core.h"
#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "embedded_api.h"

#include <math.h>

//in m
#define OA  Q(0.012)
#define AB  Q(0.045)
#define BC  Q(0.065)


#define SQR(a) (dsp16_op_mul(a,a))
static bool compute_ik(dsp16_t x, dsp16_t y, dsp16_t z, dsp16_t *theta, dsp16_t *beta, dsp16_t *alpha)
{
    dsp16_t r = dsp16_op_sqrt(dsp16_op_mul(x,x) + dsp16_op_mul(y,y));
    dsp16_t u = r-OA;

    if((SQR(x)+SQR(y)+SQR(z)) > SQR(OA+AB+BC))
    {
        return false;
    }

    *theta = dsp16_op_atan2f_emu(y,x);
    dsp16_t cos_beta = dsp16_op_div((SQR(u)+SQR(z)-SQR(AB)-SQR(BC)), 2*dsp16_op_mul(AB, BC));
    *beta = dsp16_op_acos( cos_beta );
    *alpha = dsp16_op_atan2f_emu(-z,u) - dsp16_op_acos( dsp16_op_div( (AB+dsp16_op_mul(BC, cos_beta)), dsp16_op_sqrt(SQR(z)+SQR(u)) ) );
    return true;
}


typedef struct {
    dsp16_t x;
    dsp16_t y;
    dsp16_t theta;

    /*computed later*/
    dsp16_t touch_x;
    dsp16_t touch_y;
    matrix4 matrix;

    /*corresponding servos*/
    uint8_t out_no[3];
} hexapod_leg_t;



/*
 * Move a leg to a given position u,v,z in hexapod basis.
 */
static bool process_leg(const matrix4 mat_body, const hexapod_leg_t *leg, core_output_t *outs,
                        dsp16_t move_u, dsp16_t move_v, dsp16_t move_z, dsp16_t rot)
{
    matrix4 mat_leg;
    matrix4 inv_leg;


    matrix4_multiply(mat_body, leg->matrix, mat_leg);

    matrix4_inverse_homogeneous(mat_leg, inv_leg);

    //natural point of leg
    dsp16_t x = leg->touch_x + mul16(move_u, Q(0.03));
    dsp16_t y = leg->touch_y + mul16(move_v, Q(0.03));

    dsp16_t cos_t = cos16(rot);
    dsp16_t sin_t = sin16(rot);

    //apply rotation
    vector3 p1 = {  mul16(x, cos_t) - mul16(y, sin_t),
                    mul16(y, cos_t) + mul16(x, sin_t),
                    mul16(move_z, Q(0.02))};


    matrix4_tranform_vector(inv_leg, p1, p1);

    dsp16_t theta, beta, alpha;
    if(compute_ik(p1[0], p1[1], p1[2], &theta, &beta, &alpha))
    {
        outs[ leg->out_no[0] ].value = theta;
        outs[ leg->out_no[1] ].value = alpha;
        outs[ leg->out_no[2] ].value = DSP16_PI_2 - beta;

        outs[ leg->out_no[0] ].active = true;
        outs[ leg->out_no[1] ].active = true;
        outs[ leg->out_no[2] ].active = true;
        return true;
    }

    outs[ leg->out_no[0] ].active = false;
    outs[ leg->out_no[1] ].active = false;
    outs[ leg->out_no[2] ].active = false;
    return false;
}


/*FIXME: opposite angle for all legs?*/
static hexapod_leg_t legs[6] =
{
        {.x=Q(0.036f), .y=Q(0.000f), .theta=DEG_TO_RCVALUE(0.0f), .out_no={0,1,2}},
        {.x=Q(-0.028f), .y=Q(-0.047f), .theta=DEG_TO_RCVALUE(-140.0f), .out_no={7,8,9}},
        {.x=Q(-0.028f), .y=Q(0.047f), .theta=DEG_TO_RCVALUE(140.0f), .out_no={16,17,18}},

        {.x=Q(-0.036f), .y=Q(0.000f), .theta=DEG_TO_RCVALUE(180.0f), .out_no={10,11,12}},
        {.x=Q(0.028f), .y=Q(-0.047f), .theta=DEG_TO_RCVALUE(-40.0f), .out_no={3,4,5}},
        {.x=Q(0.028f), .y=Q(0.047f), .theta=DEG_TO_RCVALUE(40.0f), .out_no={13,14,15}},
};


sequence_point_t timing_z[] = {
    {.t=Q(0.0f), .y=Q(-1.0f) },
    {.t=Q(0.1f), .y=Q(0.8f) },
    {.t=Q(0.5f), .y=Q(0.8f) },
    {.t=Q(0.6f), .y=Q(-1.0f) },
    {.t=Q(1.0f), .y=Q(-1.0f) },
    END_OF_SEQ
};

sequence_point_t timing_uv[] = {
    {.t=Q(0.0f), .y=Q(-1.0f) },
    {.t=Q(0.1f), .y=Q(-1.0f) },
    {.t=Q(0.5f), .y=Q(1.0f) },
    {.t=Q(0.6f), .y=Q(1.0f) },
    {.t=Q(1.0f), .y=Q(-1.0f) },
    END_OF_SEQ
};

sequence_point_t timing_speed[] = {
    {.t=Q(0.0f), .y=Q(0.4f) },
    {.t=Q(0.8f), .y=Q(0.9f) },
    {.t=Q(1.0f), .y=Q(0.9f) },
    END_OF_SEQ
};




static void standing(const hexapod_leg_t *legs, core_output_t *outs,
        rc_value_t u, rc_value_t v, rc_value_t z, rc_value_t alpha)
{
    int i;
    matrix4 mat_body;
    matrix4_rotation_yxz_translation(   v/4, -u/4, alpha/2,
                                        mul16(u, Q(0.025)),
                                        mul16(v, Q(0.025)),
                                        mul16(z, Q(0.025))+Q(0.050),
                                        mat_body);

    for(i=0; i<6; i++)
    {
        process_leg(mat_body, &legs[i], outs, 0, 0, 0, 0);
    }
}


static void four_leg_standing(const hexapod_leg_t *legs, core_output_t *outs,
        rc_value_t u, rc_value_t v, rc_value_t z, rc_value_t alpha)
{
    matrix4 mat_body;
    matrix4_rotation_yxz_translation(   -Q(0.15)+v/6, -u/6, alpha/3,
                                        mul16(u, Q(0.01)),
                                        mul16(v, Q(0.01)),
                                        mul16(z, Q(0.01))+Q(0.050),
                                        mat_body);


    process_leg(mat_body, &legs[0], outs, Q(0.1), Q(-0.7), Q(0.0), Q(0.0));
    process_leg(mat_body, &legs[2], outs, Q(0.0), Q(0.6), Q(0.0), Q(0.0));

    process_leg(mat_body, &legs[3], outs, Q(0.1), Q(-0.7), Q(0.0), Q(0.0));
    process_leg(mat_body, &legs[5], outs, Q(0.0), Q(0.6), Q(0.0), Q(0.0));

    outs[ legs[1].out_no[0] ].value = DSP16_PI_4+alpha;
    outs[ legs[1].out_no[1] ].value = -DSP16_PI_4+z+alpha;
    outs[ legs[1].out_no[2] ].value = DSP16_PI_2+alpha/2;
    outs[ legs[1].out_no[0] ].active = true;
    outs[ legs[1].out_no[1] ].active = true;
    outs[ legs[1].out_no[2] ].active = true;

    outs[ legs[4].out_no[0] ].value = -DSP16_PI_4+alpha;
    outs[ legs[4].out_no[1] ].value = -DSP16_PI_4+z-alpha;
    outs[ legs[4].out_no[2] ].value = DSP16_PI_2-alpha/2;
    outs[ legs[4].out_no[0] ].active = true;
    outs[ legs[4].out_no[1] ].active = true;
    outs[ legs[4].out_no[2] ].active = true;
}



static void tripod_walking(const hexapod_leg_t *legs, core_output_t *outs,
        rc_value_t u, rc_value_t v, rc_value_t z, rc_value_t alpha)
{
    static dsp16_t t;
    uint32_t speed;
    rc_value_t new_z;

    rc_value_t K_move = sat_add16(sat_add16(dsp16_op_abs(u), dsp16_op_abs(v)), dsp16_op_abs(alpha));

    if(K_move < FLOAT_TO_RCVALUE(0.05f))
    {
        u = 0;
        v = 0;
        alpha = 0;
        new_z = 0;
        speed = 0;
    }
    else
    {
        /*bound z to z from transmitter or moving factor*/
        /* new_z = MIN(dsp16_op_abs(z), K_move);*/
        new_z = z;
        speed = execute_sequence(timing_speed, K_move, Q(0.02));
    }

    t += speed;
    if(t<0)t=0;


    matrix4 mat_body;
    matrix4_rotation_yxz_translation(   -v/32, u/32, 0,
                                        0, 0, Q(0.050),
                                        mat_body);

    rc_value_t tripod1_u =  execute_sequence(timing_uv, t, u);
    rc_value_t tripod2_u = -tripod1_u;

    rc_value_t tripod1_v =  execute_sequence(timing_uv, t, v);
    rc_value_t tripod2_v = -tripod1_v;

    rc_value_t tripod1_z = execute_sequence(timing_z, t, -new_z/2);
    rc_value_t tripod2_z = -tripod1_z;

    rc_value_t tripod1_rot =  execute_sequence(timing_uv, t, alpha/16);
    rc_value_t tripod2_rot = -tripod1_rot;

    process_leg(mat_body, &legs[0], outs, tripod1_u, tripod1_v, tripod1_z, tripod1_rot);
    process_leg(mat_body, &legs[1], outs, tripod1_u, tripod1_v, tripod1_z, tripod1_rot);
    process_leg(mat_body, &legs[2], outs, tripod1_u, tripod1_v, tripod1_z, tripod1_rot);

    process_leg(mat_body, &legs[3], outs, tripod2_u, tripod2_v, tripod2_z, tripod2_rot);
    process_leg(mat_body, &legs[4], outs, tripod2_u, tripod2_v, tripod2_z, tripod2_rot);
    process_leg(mat_body, &legs[5], outs, tripod2_u, tripod2_v, tripod2_z, tripod2_rot);
}



typedef enum {
    MODE_STANDING,
    MODE_WALKING,
    MODE_4LEG_WALK,

    MODE_NB
} HEXAPOD_MODE;

static HEXAPOD_MODE mode;
static bool prev_level;


static void hexapod_update(const core_input_t *ins, core_output_t *outs)
{
    if(ins[0].active && ins[1].active && ins[2].active && ins[3].active)
    {
        //forward / backward
        rc_value_t u = ins[0].value;

        //left / right
        rc_value_t v = ins[1].value;

        //up / down
        rc_value_t z = ins[2].value;

        //rotation left/right
        rc_value_t alpha = ins[3].value;

        bool level = (ins[4].value > Q(0.5f));

        //handle mode switch
        if(level && (level != prev_level))
        {
            mode++;
            if(mode>=MODE_NB)
                mode=0;
        }
        prev_level = level;

        switch(mode)
        {
        case MODE_STANDING:
            {
                //standing
                standing(legs, outs, u,v,z,alpha);

                //turn head
                outs[19].value = alpha/4;
                outs[19].active = true;
            }
            break;
        case MODE_WALKING:
            {
                //walking
                tripod_walking(legs, outs, u,v,z,alpha);

                //turn head
                outs[19].value = alpha/4;
                outs[19].active = true;
            }
            break;
        case MODE_4LEG_WALK:
            {
                four_leg_standing(legs, outs, u,v,z,alpha);

                //turn head
                outs[19].value = alpha/2;
                outs[19].active = true;
            }
            break;
        case MODE_NB:
        default:
            break;
        };
    }
}


static void hexapod_init(void)
{
    int i;
    for(i=0; i<SIZEOF_ARRAY(legs); i++)
    {
        matrix4_rotation_yxz_translation(Q(0), Q(0), legs[i].theta,
                                         legs[i].x, legs[i].y, Q(0), legs[i].matrix);

        //get the 'natural' point
        matrix4 tmp;
        matrix4_rotation_yxz_translation(Q(0), Q(0), Q(0),
                                         OA+AB, Q(0), Q(0), tmp);

        matrix4_multiply(legs[i].matrix, tmp, tmp);

        //simply store the x,y translation corresponding to
        //the natural touch in world coordinate
        vector3 v = {0,0,0};
        matrix4_tranform_vector(tmp, v, v);
        legs[i].touch_x = v[0];
        legs[i].touch_y = v[1];
    }

    mode = MODE_WALKING;
}


void appli_init()
{
    hexapod_init();
}

void appli_update(const core_input_t *inputs, core_output_t *outputs)
{
    hexapod_update(inputs, outputs);
}
