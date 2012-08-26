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

#include "frame_ctrl.h"

#include "FreeRTOS.h"
#include "task.h"
#include "board.h"

#include "core.h"
#include "pc_comm.h"


static void get_current_pos_frame(frame_t *frm)
{
    int i;

    frm->active_bm = 0;
    for(i=0; i<MAX_OUT_NB; i++)
    {
        const core_output_t *out = core_get_output(i);
        frm->position[i] = out->value;
        if(out->active)
        {
            frm->active_bm |= 1 << i;
        }
    }
}

void frame_ctrl_update(frame_control_t *frm_ctrl, core_output_t *outputs)
{
    int i;
    uint32_t now = xTaskGetTickCount() * portTICK_RATE_MS;
    uint32_t delta_t = now - frm_ctrl->start_time;

    for(i=0; i<MAX_OUT_NB; i++)
    {
        if(frm_ctrl->next.active_bm & (1 << i))
        {
            rc_value_t prev_pos = frm_ctrl->previous.position[i];
            rc_value_t next_pos = frm_ctrl->next.position[i];

            if(delta_t >= frm_ctrl->duration)
            {
                outputs[i].value = next_pos;
                frm_ctrl->done = true;
            }
            else
            {
                //formula: y0 + (y1-y0) * (t/T)
                outputs[i].value = prev_pos + (2 * dsp16_op_mul( next_pos/2 - prev_pos/2,
                                       RC_VALUE_MAX * delta_t / frm_ctrl->duration));
            }

            outputs[i].active = true;
        }
    }
}


void frame_ctrl_init(frame_control_t *frm_ctrl)
{
    frm_ctrl->start_time = 0;
    frm_ctrl->duration = 0;
    frm_ctrl->done = true;
    get_current_pos_frame(&frm_ctrl->previous);
    get_current_pos_frame(&frm_ctrl->next);
}

void frame_ctrl_load_frame(frame_control_t *frm_ctrl, const frame_t *frame)
{    
    uint32_t now = xTaskGetTickCount() * portTICK_RATE_MS;
    
    frm_ctrl->start_time = now;
    frm_ctrl->duration = frame->duration;
    frm_ctrl->done = false;
    
    get_current_pos_frame(&frm_ctrl->previous);
    memcpy(&frm_ctrl->next, frame, sizeof(frame_t));
}


bool frame_ctrl_done(frame_control_t *frm_ctrl)
{
    return frm_ctrl->done;
}


void frame_ctrl_disable(frame_control_t *frm_ctrl)
{
    frm_ctrl->next.active_bm = 0;
    frm_ctrl->done = true;
}






/*------------------------API----------------------------*/




static frame_control_t api_frm_ctrl;

static void load_frame(pccomm_packet_t *packet)
{
    frame_ctrl_load_frame(&api_frm_ctrl, (const frame_t *)packet->data);
}

static void disable_frame(pccomm_packet_t *packet)
{
    frame_ctrl_disable(&api_frm_ctrl);
}

static const pc_comm_rx_callback rx_callbacks[] =
{
    [POS_CTRL_LOAD_FRAME] = load_frame,
    [POS_CTRL_DISABLE] = disable_frame,
};

static pccom_callbacks comm_callbacks =
{
    .callback_nb = SIZEOF_ARRAY(rx_callbacks),
    .callbacks = rx_callbacks
};



void frame_ctrl_api_update(core_output_t *outputs)
{
    frame_ctrl_update(&api_frm_ctrl, outputs);
}

void frame_ctrl_api_init(void)
{
    pc_comm_register_module_callback(PCCOM_POS_CTRL, comm_callbacks);
}

