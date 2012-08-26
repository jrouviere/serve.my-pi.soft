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

#ifndef OPENSCB_UTILS_H_
#define OPENSCB_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "pc_comm_api.h"
#include "openscb_dev.h"



int scb_send_float_as_dsp16_array(openscb_dev dev,
        uint8_t module, uint8_t command, float *data, int nb);

int scb_fragment_and_send_data(openscb_dev dev, const void *data, uint8_t nb,
        int element_size, uint8_t module, uint8_t command);

int scb_send_request(openscb_dev dev, uint8_t module, uint8_t command);

int scb_send_request_index(openscb_dev dev, uint8_t module, uint8_t command, uint8_t index);

int scb_send_string(openscb_dev dev, uint8_t module, uint8_t command,
        uint8_t index, const char *str);

int scb_send_frame(openscb_dev dev, uint8_t module, uint8_t command, uint8_t index,
    uint32_t duration, const float *position, const bool *active, uint8_t nb);

/*generate control bitfield expected by the board*/
uint32_t scb_bool_to_bitmask(const bool *active, uint8_t nb);

/*generate control bitfield expected by the board*/
void scb_bitmask_to_bool(uint32_t bitmask, bool *array, uint8_t nb);

int scb_get_reply(openscb_dev dev, pccomm_packet_t *packet,
        uint8_t module, uint8_t command);

int scb_request_and_get_reply(openscb_dev dev, pccomm_packet_t *packet,
        uint8_t module, uint8_t command, uint8_t index);

int scb_request_and_parse_dsp16(openscb_dev dev, float *outputs, uint8_t nb,
        uint8_t module, uint8_t command);


int scb_receive_and_defragment(openscb_dev dev, void *out_data, uint8_t nb,
        int element_size, uint8_t module, uint8_t command);

int scb_request_and_defragment(openscb_dev dev, void *out_data, uint8_t nb,
        int element_size, uint8_t module, uint8_t command);


/**
 * Fit as much data into packet as possible starting from 'index'.
 * The basic usage of this function is to call it in a loop and send the packet
 * until it returns 'total_element_nb'.
 *
 * \param module module number to which the message is addressed
 * \param command command code, what the message is talking about
 * \param index index of the first element concerned by this packet (if relevant)
 * \param data stuff you want to transmit
 * \param element_size size of each element of data (should be sizeof(data[0]))
 * \param total_element_nb total number of element in your array
 * \param[out] out_packet the packet this function will fill in (you must have allocated it beforehand)
 *
 * \return first non-sent index
 */
int scb_fragment_data(uint8_t module, uint8_t command, uint8_t index,
        const void *data, int element_size, int total_element_nb,
        pccomm_packet_t *out_packet);

/**
 * Create a packet with appropriate values.
 *
 * \param module module number to which the message is addressed
 * \param command command code, what the message is talking about
 * \param index index of the first element concerned by this packet (if relevant)
 * \param data stuff you want to transmit
 * \param data_size quantity of data you want to transmit (in bytes)
 * \param[out] out_packet the packet this function will fill in (you must have allocated it beforehand)
 *
 * \return packet size
 */
int scb_build_packet(uint8_t module, uint8_t command, uint8_t index,
        const void *data, int data_size, pccomm_packet_t *out_packet);

/**
 * Does the contrary than scb_fragment_data, take a packet and fill an array from it.
 * It will use the 'index' field of the packet to put packet data into your array
 * at the right place.
 *
 * \param element_size size of each element of data (should be sizeof(data[0]))
 * \param total_element_nb total number of element in your array
 * \param packet the packet we are going to parse
 * \return number of extracted element
 */
int scb_defragment_data(void *data, int element_size, int total_element_nb, const pccomm_packet_t *packet);

/**
 * Convert a float to a big endian dsp16
 */
_dsp16_t scb_convert_float_dsp16_BE(float x);

/**
 * Convert a big endian dsp16 to a float
 */
float scb_convert_dsp16_BE_float(_dsp16_t x);

/**
 * Convert an array of float to an array of big endian dsp16.
 * Both array must be allocated with at least 'size' elements each.
 */
void scb_convert_float_dsp16_BE_array(const float *x, _dsp16_t *res, int size);

/**
 * Convert an array of big endian dsp16 to an array of float.
 * Both array must be allocated with at least 'size' elements each.
 */
void scb_convert_dsp16_BE_float_array(const _dsp16_t *x, float *res, int size);

#ifdef __cplusplus
}
#endif

#endif /* OPENSCB_UTILS_H_ */
