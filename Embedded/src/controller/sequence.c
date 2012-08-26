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

#include "sequence.h"
#include "user_flash.h"
#include "pc_comm.h"

#include "appli/embedded_api.h"

typedef struct {
    uint8_t storing_slot_id;   /*flash slot where sequence is being stored*/
    uint16_t store_frame_received;
    uint16_t store_frame_expected;
} api_store_data_t;


#define FRAME_OFFSET (sizeof(slot_header_t)+sizeof(seq_header_t))


static void load_flash_seq_header(uint8_t slot_id, seq_header_t *header);
static void load_flash_seq_frame(uint8_t slot_id, uint16_t idx, frame_t *frame);


void seq_ctrl_update(seq_ctrl_data_t *ctrl_data, core_output_t *outputs)
{
    if(frame_ctrl_done(&ctrl_data->frm_ctrl))
    {
        /*load next frame if it exists*/
        if(ctrl_data->seq_cur < ctrl_data->seq_item_nb)
        {
            frame_t tmp_frm;
            load_flash_seq_frame(ctrl_data->loaded_slot_id,
                    ctrl_data->seq_cur, &tmp_frm);
            frame_ctrl_load_frame(&ctrl_data->frm_ctrl, &tmp_frm);
        }

        ctrl_data->seq_cur++;
    }

    frame_ctrl_update(&ctrl_data->frm_ctrl, outputs);
}


void seq_ctrl_play_flash_slot(seq_ctrl_data_t *ctrl_data, uint8_t slot_id)
{
    seq_header_t header;

    if(flash_check_slot_type(slot_id, SLOT_OUT_SEQUENCE) != 0)
    {
        TRACE("Invalid slot type\n");
        return;
    }

    load_flash_seq_header(slot_id, &header);

    /*write down slot id, and current frame nb*/
    ctrl_data->loaded_slot_id = slot_id;
    ctrl_data->seq_item_nb = header.frame_nb;
    ctrl_data->seq_cur = 0;

    frame_ctrl_init(&ctrl_data->frm_ctrl);
}



/*-----------------------FLASH---------------------------*/


static void load_flash_seq_header(uint8_t slot_id, seq_header_t *header)
{
    flash_load_slot(slot_id, sizeof(slot_header_t), sizeof(seq_header_t),
            header);
}

static void load_flash_seq_frame(uint8_t slot_id, uint16_t idx, frame_t *frame)
{
    flash_load_slot(slot_id, FRAME_OFFSET + idx*sizeof(frame_t),
            sizeof(frame_t), frame);
}

static void save_seq_header(uint8_t slot_id, const seq_header_t *header)
{
    flash_store_slot(slot_id, sizeof(slot_header_t), sizeof(seq_header_t), header);
}

static void seq_ctrl_save_flash_slot_start(api_store_data_t *store_data, uint8_t slot_id, uint16_t frame_nb)
{
    seq_header_t seq_header;

    store_data->storing_slot_id = slot_id;
    store_data->store_frame_received = 0;
    store_data->store_frame_expected = frame_nb;

    if(flash_erase_slot(slot_id) != 0)
    {
        TRACE("Cannot erase flash slot\n");
        return;
    }

    seq_header.frame_nb = frame_nb;

    save_seq_header(slot_id, &seq_header);
}

static void seq_ctrl_save_flash_slot_end(api_store_data_t *store_data, const char *desc)
{
    //check the correct number of frame has been received
    if(store_data->store_frame_received == store_data->store_frame_expected)
    {
        flash_init_slot(store_data->storing_slot_id, SLOT_OUT_SEQUENCE, desc,
                store_data->store_frame_received * sizeof(frame_t) + sizeof(seq_header_t));
    }
    else
    {
        TRACE("Error during sequence save\n");
    }
}

static void seq_ctrl_save_flash_slot_frame(api_store_data_t *store_data, int idx, const frame_t *frame)
{
    //Check that we received expected frame
    if(idx == store_data->store_frame_received)
    {
        flash_store_slot(store_data->storing_slot_id, FRAME_OFFSET + idx*sizeof(frame_t),
                sizeof(frame_t), frame);

        store_data->store_frame_received++;
    }
}




/*-------------------------------------------------------*/
/*------------------------API----------------------------*/


static seq_ctrl_data_t api_ctrl_data;
static api_store_data_t store_data;

static void play_slot(pccomm_packet_t *packet)
{
    uint8_t slot_id = packet->header.index;
    seq_ctrl_play_flash_slot(&api_ctrl_data, slot_id);
}

static void save_slot_start(pccomm_packet_t *packet)
{
    uint8_t slot_id = packet->header.index;
    uint16_t *frame_nb = (uint16_t*)(packet->data);
    seq_ctrl_save_flash_slot_start(&store_data, slot_id, *frame_nb);
}

static void save_slot_frame(pccomm_packet_t *packet)
{
    seq_ctrl_save_flash_slot_frame(&store_data, packet->header.index,
            (const frame_t*)packet->data);
}

static void save_slot_end(pccomm_packet_t *packet)
{
    seq_ctrl_save_flash_slot_end(&store_data, (const char*)packet->data);
}

static void download_slot(pccomm_packet_t *packet)
{
    int i;
    seq_header_t header;
    uint8_t slot_id = packet->header.index;

    if(flash_check_slot_type(slot_id, SLOT_OUT_SEQUENCE) != 0)
    {
        TRACE("Invalid slot type\n");
        return;
    }

    load_flash_seq_header(slot_id, &header);
    memcpy(packet->data, &header, sizeof(header));
    packet->header.index = 0;
    packet->header.size = sizeof(packet->header) + sizeof(header);
    pc_comm_send_raw_packet(packet);

    for(i=0; i<header.frame_nb; i++)
    {
        flash_load_slot(slot_id, FRAME_OFFSET + i*sizeof(frame_t),
                sizeof(frame_t), packet->data);

        packet->header.index = i;
        packet->header.command = SEQ_CTRL_DOWNLOAD_SLOT_FRAME;
        packet->header.size = sizeof(packet->header) + sizeof(frame_t);
        pc_comm_send_raw_packet(packet);
    }
}



static const pc_comm_rx_callback rx_callbacks[] =
{
    [SEQ_CTRL_PLAY_SLOT] = play_slot,
    [SEQ_CTRL_UPLOAD_SLOT_START] = save_slot_start,
    [SEQ_CTRL_UPLOAD_SLOT_FRAME] = save_slot_frame,
    [SEQ_CTRL_UPLOAD_SLOT_END] = save_slot_end,

    [SEQ_CTRL_DOWNLOAD_SLOT] = download_slot,
};

static pccom_callbacks comm_callbacks =
{
    .callback_nb = SIZEOF_ARRAY(rx_callbacks),
    .callbacks = rx_callbacks
};


void seq_ctrl_api_update(core_output_t *outputs)
{
    seq_ctrl_update(&api_ctrl_data, outputs);
}

void seq_ctrl_api_init()
{
    pc_comm_register_module_callback(PCCOM_SEQ_CTRL, comm_callbacks);
}
