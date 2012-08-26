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

#include "board.h"
#include "FreeRTOS.h"

#include "core.h"
#include "rc_utils.h"
#include "pc_comm.h"
#include "system.h"

#include "calibration.h"


#define DEFAULT_MIN_VALUE               US_TO_TC4_TICK(1050)
#define DEFAULT_MIDDLE_VALUE            US_TO_TC4_TICK(1520)
#define DEFAULT_MAX_VALUE               US_TO_TC4_TICK(1950)
#define DEFAULT_ANGLE180_VALUE          US_TO_TC4_TICK(1600)
#define DEFAULT_OUTPUT_RAW_VALUE        DEFAULT_MIDDLE_VALUE


void output_calib_init_default(output_calib_data_t *out_calib)
{
    out_calib->min = DEFAULT_MIN_VALUE;
    out_calib->max = DEFAULT_MAX_VALUE;
    out_calib->angle_180 = DEFAULT_ANGLE180_VALUE;
    out_calib->subtrim = DEFAULT_MIDDLE_VALUE;
}

void input_calib_init_default(input_calib_data_t *in_calib)
{
    in_calib->min =  DEFAULT_MIN_VALUE;
    in_calib->mid =  DEFAULT_MIDDLE_VALUE;
    in_calib->max =  DEFAULT_MAX_VALUE;
}

void input_calib_reset_for_calib(input_calib_data_t *in_calib)
{
    in_calib->min =  DEFAULT_MAX_VALUE;
    in_calib->mid =  DEFAULT_MIDDLE_VALUE;
    in_calib->max =  DEFAULT_MIN_VALUE;
}


rc_value_t input_calib_raw_to_rc(const input_calib_data_t *calib, rc_raw_t raw)
{
    if(raw <= calib->min)
        return RC_VALUE_MIN;

    if(raw >= calib->max)
        return RC_VALUE_MAX;

    /*compute distance to middle and apply corresponding ratio*/
    /*we consider inputs to be divided into two parts:
    - a linear function in negative values
    - another linear function in positive values */
    if(raw >= calib->mid)
    {
        return (((int32_t)raw - (int32_t)calib->mid) * 0x8000 / ((int32_t)calib->max - calib->mid));
    }
    else
    {
        return (((int32_t)raw - (int32_t)calib->mid) * 0x8000 / ((int32_t)calib->mid - calib->min));
    }
}


rc_raw_t output_calib_rc_to_raw(const output_calib_data_t *calib, rc_value_t rc)
{
    /*convert from angle to centered servo full range*/
    rc_raw_t raw = (rc_raw_t)((int32_t)rc * (int32_t)calib->angle_180 / 0x8000) + calib->subtrim;

    /*check valid range*/
    if(raw <= calib->min)
        return calib->min;
    else if(raw >= calib->max)
        return calib->max;

    return raw;
}


//---------------------------------------------------------
// CORE INPUT CALIBRATION
//---------------------------------------------------------
static bool store_center_values = false;

void input_calib_core_run(const system_conf_t *sys_conf)
{
    int i;

    core_input_t raw;
    const io_class_t *type = core_get_input_type();

    if(type->pre != NULL)
    {
        type->pre();
    }

    for(i=0; i<sys_conf->input_nb; i++)
    {
        uint8_t chn = sys_conf->in_conf[i].mapping;
        type->get(chn, &raw);

        if(raw.active)
        {
            input_calib_data_t calib;
            memcpy(&calib, &sys_conf->in_conf[i].calib, sizeof(calib));

            if(raw.value < calib.min)
            {
                calib.min = raw.value;
                sys_conf_set_input_calib(i, &calib);
            }

            if(raw.value > calib.max)
            {
                calib.max = raw.value;
                sys_conf_set_input_calib(i, &calib);
            }

            if(store_center_values)
            {
                calib.mid = raw.value;
            }
        }
    }
    store_center_values = false;
}

//---------------------------------------------------------
// CORE OUTPUT CALIBRATION
//---------------------------------------------------------

typedef struct {
    rc_raw_t raw;
    int out_no;
} core_calib_internal;
static core_calib_internal cal_data;


#define MAX_CALIB_DIFF  15

void output_calib_core_run(const system_conf_t *sys_conf)
{
    int i;
    core_output_t temp;
    const io_class_t *type = core_get_output_type();



    for(i=0; i<MAX_SERVO_NB; i++)
    {
        temp.value = 0;
        temp.active = false;
        type->set(i, &temp);
    }

    if(cal_data.out_no < MAX_SERVO_NB)
    {
        //get previous value so we can move slowly from the output
        core_input_t prev;
        rc_value_t diff, new_value;
        bool is_enabled = (sys_conf->servo_active_bm & (1 << cal_data.out_no)) != 0;

        type->get(cal_data.out_no, &prev);

        diff = cal_data.raw - prev.value;

        if(diff > MAX_CALIB_DIFF)
            new_value = prev.value + MAX_CALIB_DIFF;
        else if(diff < -MAX_CALIB_DIFF)
            new_value = prev.value - MAX_CALIB_DIFF;
        else
            new_value = prev.value + diff;

        temp.active = is_enabled;
        temp.value = new_value;
        type->set(cal_data.out_no, &temp);
    }
}

//---------------------------------------------------------
// COMMUNICATION WITH PC
//---------------------------------------------------------
static void set_output_calib_raw(pccomm_packet_t *packet)
{
    cal_data.out_no = packet->header.index;
    cal_data.raw = *((rc_raw_t*)packet->data);
}


static void set_input_calib_center(pccomm_packet_t *packet)
{
    store_center_values = true;
}


static const pc_comm_rx_callback rx_callbacks[] =
{
    [SET_OUTPUT_CALIB_RAW] = set_output_calib_raw,
    [SET_INPUT_CALIB_CENTER] = set_input_calib_center,
};

static pccom_callbacks comm_callbacks =
{
    .callback_nb = SIZEOF_ARRAY(rx_callbacks),
    .callbacks = rx_callbacks
};

void core_calib_init(void)
{
    cal_data.out_no = 0xFF;
    pc_comm_register_module_callback(PCCOM_CORE_CALIBRATION, comm_callbacks);
}
