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

#include "openscb_advanced.h"

#include "openscb_utils.h"
#include "pc_comm_const.h"

#include <string.h>
#include "arch.h"



int scb_req_restart_to_bootloader(openscb_dev dev)
{
    return scb_send_request(dev, PCCOM_SYSTEM, RESTART_BOOTLOADER);
}


int scb_set_core_mode(openscb_dev dev, uint8_t mode)
{
    pccomm_packet_t packet;
    scb_build_packet(PCCOM_CORE, SET_CORE_MODE, mode, NULL, 0, &packet);
    return scb_send_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
}


int scb_in_calib_set_center(openscb_dev dev)
{
    return scb_send_request(dev, PCCOM_CORE_CALIBRATION, SET_INPUT_CALIB_CENTER);
}


int scb_get_in_calib_values(openscb_dev dev, input_calib_data_t *calib, uint8_t nb)
{
    return scb_request_and_defragment(dev, calib, nb, sizeof(input_calib_data_t),
            PCCOM_SYS_CONF, REQ_INPUT_CALIB_VALUE);
}


int scb_set_out_calib_values(openscb_dev dev, output_calib_data_t *calib,
        uint8_t nb)
{
    return scb_fragment_and_send_data(dev, calib, nb, sizeof(output_calib_data_t),
            PCCOM_SYS_CONF, SET_OUTPUT_CALIB_VALUE);
}


int scb_get_out_calib_values(openscb_dev dev, output_calib_data_t *calib, uint8_t nb)
{
    return scb_request_and_defragment(dev, calib, nb, sizeof(output_calib_data_t),
            PCCOM_SYS_CONF, REQ_OUTPUT_CALIB_VALUE);
}

int scb_set_out_calib_raw(openscb_dev dev, uint8_t out_id, _dsp16_t value)
{
    pccomm_packet_t packet;
    scb_build_packet(PCCOM_CORE_CALIBRATION, SET_OUTPUT_CALIB_RAW, out_id, &value,
            sizeof(value), &packet);

    return scb_send_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
}


int scb_get_input_mapping(openscb_dev dev, uint8_t *mapping, uint8_t nb)
{
    return scb_request_and_defragment(dev, mapping, nb, sizeof(uint8_t),
            PCCOM_SYS_CONF, REQ_INPUT_MAPPING);
}


int scb_set_input_mapping(openscb_dev dev, uint8_t *mapping, uint8_t nb)
{
    return scb_fragment_and_send_data(dev, mapping, nb, sizeof(uint8_t),
            PCCOM_SYS_CONF, SET_INPUT_MAPPING);
}


int scb_get_output_name(openscb_dev dev, io_name_t *names, uint8_t nb)
{
    return scb_request_and_defragment(dev, names, nb, sizeof(io_name_t),
            PCCOM_SYS_CONF, REQ_OUTPUT_NAME);
}


int scb_set_output_name(openscb_dev dev, io_name_t *names, uint8_t nb)
{
    return scb_fragment_and_send_data(dev, names, nb, sizeof(io_name_t),
            PCCOM_SYS_CONF, SET_OUTPUT_NAME);
}



int scb_upload_sequence_start(openscb_dev dev, uint8_t slot_id, uint16_t frame_nb)
{
    pccomm_packet_t packet;
    frame_nb = BE16(frame_nb);

    scb_build_packet(PCCOM_SEQ_CTRL, SEQ_CTRL_UPLOAD_SLOT_START, slot_id, &frame_nb, sizeof(frame_nb), &packet);
    return scb_send_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
}


int scb_upload_sequence_end(openscb_dev dev, const char *seq_desc)
{
    return scb_send_string(dev, PCCOM_SEQ_CTRL, SEQ_CTRL_UPLOAD_SLOT_END, 0, seq_desc);
}


int scb_upload_sequence_frame(openscb_dev dev, uint8_t index, uint32_t duration,
        const float *position, const bool *active, uint8_t nb)
{
    return scb_send_frame(dev, PCCOM_SEQ_CTRL, SEQ_CTRL_UPLOAD_SLOT_FRAME, index,
            duration, position, active, nb);
}

int scb_download_sequence(openscb_dev dev, uint8_t slot_id, frame_t *frame, uint8_t *nb)
{
    int ret;
    pccomm_packet_t packet;

    /*send a packet requesting to download slot*/
    ret = scb_send_request_index(dev, PCCOM_SEQ_CTRL, SEQ_CTRL_DOWNLOAD_SLOT, slot_id);

    /*get answer (== number of frame in sequence)*/
    ret = scb_get_reply(dev, &packet, PCCOM_SEQ_CTRL, SEQ_CTRL_DOWNLOAD_SLOT);
    if(ret < 0)
    {
        return ret;
    }

    /*then get all frame in native format*/
    uint16_t frame_nb = *((uint16_t*)packet.data);
    frame_nb = BE16(frame_nb);
    *nb = MIN(*nb, frame_nb);

    return scb_receive_and_defragment(dev, frame, *nb, sizeof(frame_t),
            PCCOM_SEQ_CTRL, SEQ_CTRL_DOWNLOAD_SLOT_FRAME);
}


int scb_store_settings_to_slot(openscb_dev dev, uint8_t slot_id)
{
    return scb_send_request_index(dev, PCCOM_SYS_CONF, SAVE_SYS_CONF, slot_id);
}


int scb_load_settings_from_slot(openscb_dev dev, uint8_t slot_id)
{
    return scb_send_request_index(dev, PCCOM_SYS_CONF, LOAD_SYS_CONF, slot_id);
}

int scb_get_flash_slot(openscb_dev dev, uint8_t slot_id, slot_header_t *slot_header)
{
    int ret;
    pccomm_packet_t packet;

    ret = scb_request_and_get_reply(dev, &packet,
            PCCOM_USER_FLASH, FLASH_REQ_OVERVIEW, slot_id);

    if(ret >= 0)
    {
        memcpy(slot_header, packet.data, sizeof(slot_header_t));
    }
    return ret;
}

int scb_get_flash_overview(openscb_dev dev, slot_header_t *slot_header,
        uint8_t nb)
{
    return scb_request_and_defragment(dev, slot_header, nb,
            sizeof(slot_header_t), PCCOM_USER_FLASH, FLASH_REQ_OVERVIEW_ALL);
}


int scb_set_flash_slot_description(openscb_dev dev, uint8_t slot_id,
        char *description)
{
    return scb_send_string(dev, PCCOM_USER_FLASH, FLASH_SET_DESCRIPTION,
            slot_id, description);
}

