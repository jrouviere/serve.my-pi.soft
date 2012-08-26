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

#include "openscb.h"

#include "openscb_utils.h"
#include "pc_comm_const.h"
#include "arch.h"

#include <string.h>
#include <math.h>



float scb_degrees_to_goal(float degrees)
{
    return degrees / 180.0f;
}

float scb_goal_to_degrees(float goal)
{
    return goal * 180.0f;
}

float scb_radian_to_goal(float radian)
{
    return radian / M_PI;
}

float scb_goal_to_radian(float goal)
{
    return goal * M_PI;
}


int scb_set_one_output_goal(openscb_dev dev, uint8_t out_id, float output)
{
    pccomm_packet_t packet;
    _dsp16_t conv_data;
    conv_data = scb_convert_float_dsp16_BE(output);

    scb_build_packet(PCCOM_API_CTRL, API_SET_OUTPUT_GOAL, out_id, &conv_data,
            sizeof(_dsp16_t), &packet);
    return scb_send_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
}


int scb_set_output_goal(openscb_dev dev, float *outputs, int nb)
{
    return scb_send_float_as_dsp16_array(dev, PCCOM_API_CTRL, API_SET_OUTPUT_GOAL, outputs, nb);
}


int scb_set_out_speed(openscb_dev dev, uint8_t *speeds, int nb)
{
    return scb_fragment_and_send_data(dev, speeds, nb, sizeof(uint8_t),
            PCCOM_SYS_CONF, SET_OUTPUT_MAX_SPEED);
}

int scb_get_input_nb(openscb_dev dev, uint8_t *nb)
{
    return scb_request_and_defragment(dev, nb, 1, sizeof(uint8_t),
            PCCOM_CORE, REQ_INPUT_NB);
}

int scb_get_input_values(openscb_dev dev, float *inputs, uint8_t nb)
{
    return scb_request_and_parse_dsp16(dev, inputs, nb, PCCOM_CORE, REQ_INPUT_VALUE);
}

int scb_get_output_nb(openscb_dev dev, uint8_t *nb)
{
    return scb_request_and_defragment(dev, nb, 1, sizeof(uint8_t),
            PCCOM_CORE, REQ_OUTPUT_NB);
}


int scb_get_output_values(openscb_dev dev, float *outputs, uint8_t nb)
{
    return scb_request_and_parse_dsp16(dev, outputs, nb, PCCOM_CORE, REQ_OUTPUT_VALUE);
}


int scb_get_output_goal(openscb_dev dev, float *goal, uint8_t nb)
{
    return scb_request_and_parse_dsp16(dev, goal, nb, PCCOM_API_CTRL, API_REQ_OUTPUT_GOAL);
}


int scb_get_output_speed(openscb_dev dev, uint8_t *speeds, uint8_t nb)
{
    return scb_request_and_defragment(dev, speeds, nb, sizeof(uint8_t),
            PCCOM_SYS_CONF, REQ_OUTPUT_MAX_SPEED);
}


int scb_set_out_controlled(openscb_dev dev, const bool *controlled, uint8_t nb)
{
    int pos_nb = MIN(MAX_OUT_NB, nb);
    pccomm_packet_t packet;

    uint32_t active_bm = scb_bool_to_bitmask(controlled, pos_nb);

    scb_build_packet(PCCOM_API_CTRL, API_SET_CONTROL_BM, 0, &active_bm,
            sizeof(active_bm), &packet);

    return scb_send_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
}


int scb_get_out_enabled(openscb_dev dev, bool *enabled, uint8_t nb)
{
    int ret;
    uint32_t *enabled_bm;
    pccomm_packet_t packet;

    ret = scb_send_request(dev, PCCOM_SYS_CONF, REQ_OUTPUT_ACTIVE_BM);
    if(ret < 0)
    {
        return ret;
    }

    ret = scb_get_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
    if(ret < 0)
    {
        return ret;
    }

    enabled_bm = (uint32_t *)packet.data;
    scb_bitmask_to_bool(*enabled_bm, enabled, nb);
    return 0;
}


int scb_set_out_enabled(openscb_dev dev, const bool *enabled, uint8_t nb)
{
    int pos_nb = MIN(MAX_OUT_NB, nb);
    pccomm_packet_t packet;

    uint32_t enabled_bm = scb_bool_to_bitmask(enabled, pos_nb);

    scb_build_packet(PCCOM_SYS_CONF, SET_OUTPUT_ACTIVE_BM, 0, &enabled_bm,
            sizeof(enabled_bm), &packet);

    return scb_send_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
}




int scb_load_frame(openscb_dev dev, uint32_t duration, const float *position,
        const bool *active, uint8_t nb)
{
    return scb_send_frame(dev, PCCOM_POS_CTRL, POS_CTRL_LOAD_FRAME, 0,
            duration, position, active, nb);
}


int scb_disable_frame(openscb_dev dev)
{
    return scb_send_request(dev, PCCOM_POS_CTRL, POS_CTRL_DISABLE);
}


int scb_play_sequence_slot(openscb_dev dev, uint8_t slot_id)
{
    return scb_send_request_index(dev, PCCOM_SEQ_CTRL, SEQ_CTRL_PLAY_SLOT, slot_id);
}



int scb_check_firmware_version(openscb_dev dev, bool *compatible, char *version, int size)
{
    int ret;
    int len;
    pccomm_packet_t packet;

    /*init value in case of premature return*/
    if(size >0)
    {
        *version = '\0';
    }
    *compatible = false;

    ret = scb_request_and_get_reply(dev, &packet, PCCOM_SYSTEM, REQ_SOFT_VERSION, 0);
    if(ret < 0)
    {
        return ret;
    }

    len = MIN(size, packet.header.size-sizeof(packet.header));
    memcpy(version, packet.data, len);
    version[len] = '\0';

    *compatible = (strcmp(OPENSCB_SOFT_VERSION, version) == 0);

    return 0;
}


