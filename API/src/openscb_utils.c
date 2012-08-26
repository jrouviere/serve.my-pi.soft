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

#include "arch.h"
#include "openscb_utils.h"

#include <string.h>



static int scb_parse_dsp16_packet(const pccomm_packet_t *packet,
        uint8_t module, uint8_t command, float *results, int nb);

static int scb_check_packet(const pccomm_packet_t *packet,
        uint8_t module, uint8_t command);

int scb_send_float_as_dsp16_array(openscb_dev dev,
        uint8_t module, uint8_t command, float *data, int nb)
{
    int ret;
    int ind = 0;

    while(ind < nb)
    {
        _dsp16_t conv_data[MAX_PAYLOAD_SIZE/sizeof(_dsp16_t)];
        pccomm_packet_t packet;

        int elt_per_packet = MIN(MAX_PAYLOAD_SIZE/sizeof(_dsp16_t), nb-ind);
        scb_convert_float_dsp16_BE_array(data, conv_data, elt_per_packet);

        ind = scb_fragment_data(module, command, ind, conv_data, sizeof(_dsp16_t), nb, &packet);
        ret = scb_send_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
        if(ret < 0)
        {
            return ret;
        }
    }
    return 0;
}

int scb_send_frame(openscb_dev dev, uint8_t module, uint8_t command, uint8_t index,
    uint32_t duration, const float *position, const bool *active, uint8_t nb)
{
    int pos_nb = MIN(MAX_OUT_NB, nb);

    pccomm_packet_t packet;
    packet.header.module = module;
    packet.header.command = command;
    packet.header.index = index;
    packet.header.size = sizeof(pccomm_msg_header_t) + sizeof(frame_t);

    frame_t *frame = (frame_t*)packet.data;
    frame->active_bm = scb_bool_to_bitmask(active, pos_nb);
    frame->duration = BE32(duration);
    scb_convert_float_dsp16_BE_array(position, frame->position, pos_nb);

    return scb_send_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
}


int scb_fragment_and_send_data(openscb_dev dev, const void *data, uint8_t nb,
        int element_size, uint8_t module, uint8_t command)
{
    int ret;
    int ind = 0;

    while(ind < nb)
    {
        pccomm_packet_t packet;

        ind = scb_fragment_data(module, command, ind, data, element_size, nb,
                &packet);
        ret = scb_send_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
        if(ret < 0)
        {
            return ret;
        }
    }

    return 0;
}

int scb_send_request(openscb_dev dev, uint8_t module, uint8_t command)
{
    pccomm_packet_t packet;
    scb_build_packet(module, command, 0, NULL, 0, &packet);
    return scb_send_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
}

int scb_send_request_index(openscb_dev dev, uint8_t module, uint8_t command, uint8_t index)
{
    pccomm_packet_t packet;
    scb_build_packet(module, command, index, NULL, 0, &packet);
    return scb_send_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
}

int scb_send_string(openscb_dev dev, uint8_t module, uint8_t command,
        uint8_t index, const char *str)
{
    pccomm_packet_t packet;
    int str_sz = strlen(str);
    int size = MIN(str_sz+1, MAX_PAYLOAD_SIZE); //add 1 for '\0'
    scb_build_packet(module, command, index, str, size, &packet);
    return scb_send_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
}

int scb_check_packet(const pccomm_packet_t *packet,
        uint8_t module, uint8_t command)
{
    if(packet->header.module == module && packet->header.command == command)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/*generate control bitfield expected by the board*/
uint32_t scb_bool_to_bitmask(const bool *active, uint8_t nb)
{
    int i;
    uint32_t active_bm = 0;
    for(i=0; i<nb; i++)
    {
        if(active[i])
        {
            active_bm |= (1 << i);
        }
    }

    return BE32(active_bm);
}


/*generate control bitfield expected by the board*/
void scb_bitmask_to_bool(uint32_t bitmask, bool *array, uint8_t nb)
{
    int i;
    uint32_t active = BE32(bitmask);

    for(i=0; i<nb; i++)
    {
        if(active & (1 << i))
        {
            array[i] = true;
        }
        else
        {
            array[i] = false;
        }
    }
}

int scb_parse_dsp16_packet(const pccomm_packet_t *packet,
        uint8_t module, uint8_t command, float *results, int nb)
{
    if(scb_check_packet(packet, module, command) == 0)
    {
        int i;
        int element_size = sizeof(_dsp16_t);
        _dsp16_t *packet_data = (_dsp16_t*) packet->data;
        int elt_in_packet = (packet->header.size / element_size);
        int index = packet->header.index;
        int max = MIN(elt_in_packet, nb - index);

        for(i=0; i<max; i++)
        {
            results[i+index] = scb_convert_dsp16_BE_float(packet_data[i]);
        }

        return max;
    }
    else
    {
        return -1;
    }
}

int scb_get_reply(openscb_dev dev, pccomm_packet_t *packet,
        uint8_t module, uint8_t command)
{
    int ret;

    /* try to get the answer, if some random packets are received,
     * discard them. If the packet is loss, a timeout will occurs*/
    do {
        ret = scb_get_pure_raw_message(dev, packet, DEFAULT_TIMEOUT);
        if(ret < 0)
        {
            return ret;
        }
    } while (scb_check_packet(packet, module, command) < 0);

    return 0;
}

int scb_request_and_get_reply(openscb_dev dev, pccomm_packet_t *packet,
        uint8_t module, uint8_t command, uint8_t index)
{
    int ret;

    /*send the request*/
    ret = scb_send_request_index(dev, module, command, index);
    if(ret < 0)
    {
        return ret;
    }

    return scb_get_reply(dev, packet, module, command);
}


int scb_request_and_parse_dsp16(openscb_dev dev, float *outputs, uint8_t nb,
        uint8_t module, uint8_t command)
{
    int ret;
    pccomm_packet_t packet;
    uint8_t data_read = 0;

    ret = scb_send_request(dev, module, command);
    if(ret < 0)
    {
        return ret;
    }

    while(data_read < nb)
    {
        ret = scb_get_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
        if(ret < 0)
        {
            return ret;
        }

        ret = scb_parse_dsp16_packet(&packet, module, command, outputs, nb);
        if(ret < 0)
        {
            /*this might be an unrelated packet, try next one*/
            continue;
        }
        data_read += ret;
    }
    return 0;
}



int scb_receive_and_defragment(openscb_dev dev, void *out_data, uint8_t nb,
        int element_size, uint8_t module, uint8_t command)
{
    int ret;
    pccomm_packet_t packet;
    uint8_t data_read = 0;

    while(data_read < nb)
    {
        ret = scb_get_pure_raw_message(dev, &packet, DEFAULT_TIMEOUT);
        if(ret < 0)
        {
            return ret;
        }

        if(scb_check_packet(&packet, module, command) == 0)
        {
            ret = scb_defragment_data(out_data, element_size, nb, &packet);
        }
        else
        {
            continue; /*bogus packet, try next one*/
        }
        data_read += ret;
    }
    return 0;
}


int scb_request_and_defragment(openscb_dev dev, void *out_data, uint8_t nb,
        int element_size, uint8_t module, uint8_t command)
{
    int ret;

    ret = scb_send_request(dev, module, command);
    if(ret < 0)
    {
        return ret;
    }

    return scb_receive_and_defragment(dev, out_data, nb, element_size,
            module, command);
}


//see header
int scb_fragment_data(uint8_t module, uint8_t command, uint8_t index,
        const void *data, int element_size, int total_element_nb,
        pccomm_packet_t *out_packet)
{
    int elt_per_packet;

    //check maximum size
    if(element_size > MAX_PAYLOAD_SIZE)
    {
        return -2;
    }

    //move pointer to actual start of data
    data = ((uint8_t *)data) + index*element_size;

    //compute nb of element that will go into this packet
    elt_per_packet = MIN(MAX_PAYLOAD_SIZE/element_size, total_element_nb-index);

    if(scb_build_packet(module, command, index,
                 data, elt_per_packet*element_size, out_packet) < 0)
    {
        return -1;
    }

    return index + elt_per_packet;
}

//see header
int scb_build_packet(uint8_t module, uint8_t command, uint8_t index,
        const void *data, int data_size, pccomm_packet_t *out_packet)
{
    if(data_size > MAX_PAYLOAD_SIZE)
    {
        return -1;
    }

    out_packet->header.module = module;
    out_packet->header.command = command;
    out_packet->header.index = index;
    out_packet->header.size = sizeof(pccomm_msg_header_t)+data_size;

    memcpy(out_packet->data, data, data_size);

    return sizeof(pccomm_msg_header_t)+data_size;
}

//see header
int scb_defragment_data(void *data, int element_size, int total_element_nb, const pccomm_packet_t *packet)
{
    int buffer_available = (total_element_nb-packet->header.index)*element_size;
    int size = MIN(buffer_available, packet->header.size-sizeof(pccomm_msg_header_t));

    void *dest = ((uint8_t *)data) + packet->header.index*element_size;

    memcpy(dest, packet->data, size);
    return size / element_size;
}




//see header
_dsp16_t scb_convert_float_dsp16_BE(float x)
{
    _dsp16_t t = FLOAT_TO_RCVALUE(x);
    return BE16( t );
}

//see header
float scb_convert_dsp16_BE_float(_dsp16_t x)
{
    _dsp16_t t = BE16(x);
    return RCVALUE_TO_FLOAT( t );
}


//see header
void scb_convert_float_dsp16_BE_array(const float *x, _dsp16_t *res, int size)
{
    int i;
    for(i=0; i<size; i++)
    {
        res[i] = BE16( FLOAT_TO_RCVALUE(x[i]) );
    }
}

//see header
void scb_convert_dsp16_BE_float_array(const _dsp16_t *x, float *res, int size)
{
    int i;
    for(i=0; i<size; i++)
    {
        res[i] = RCVALUE_TO_FLOAT(BE16(x[i]));
    }
}
