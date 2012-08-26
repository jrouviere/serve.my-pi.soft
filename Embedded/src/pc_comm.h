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

#ifndef PC_COMM_H_
#define PC_COMM_H_

#include <stdint.h>
#include <stdbool.h>

#include "pc_comm_api.h"
#include "pc_comm_const.h"


/**
 * USB packet reception callback prototype
 */
typedef void (*pc_comm_rx_callback)(pccomm_packet_t *packet);

/**
 * Structure used to declare module reception callbacks
 */
typedef struct
{
    int callback_nb;
    const pc_comm_rx_callback *callbacks;
} pccom_callbacks;



/**
 * Extract member of a structure for each structure in an array and send it to PC.
 * \note This function is thread safe but might block (busy-wait).
 * \sa PC_COMM_SEND_MEMBERS macro
 */
void pc_comm_send_member_of_struct_array(uint8_t module, uint8_t command,
        const void *array, int struct_size, int item_offset, int item_size,
        int item_nb);

/**
 * Update member of a structure in an array of structure with data comming from PC.
 * \note This function is thread safe but might block (busy-wait).
 * \sa PC_COMM_RX_MEMBERS macro
 */
void pc_comm_receive_member_of_struct_array(const pccomm_packet_t *packet,
        void *dest, int sizeof_container, int item_offset, int sizeof_item,
        int max_ind);

/**
 * Send a simple array of structure to PC (send the whole structure).
 * \note This function is thread safe but might block (busy-wait).
 */
void pc_comm_send_array(uint8_t module, uint8_t command, const void *data,
        uint8_t item_size, uint8_t item_nb);


/**
 * Receive a simple array of structure from PC.
 */
void pc_comm_receive_array(const pccomm_packet_t *packet, void *dest,
        uint8_t item_size, uint8_t item_nb);


/**
 * Send a zero terminated string to PC.
 * \note This function is thread safe but might block (busy-wait).
 */
bool pc_comm_send_string(uint8_t module, uint8_t command, uint8_t index,
        const char *str);

/**
 * Send a packet to PC, the data must be already formatted correctly.
 * \note This function is thread safe but might block (busy-wait).
 */
int pc_comm_send_packet(uint8_t module, uint8_t command, uint8_t index,
        const void *data, uint8_t size);


/**
 * Send a simple user-formatted packet to PC.
 *
 * \param packet The packet to transmit to PC.
 */
int pc_comm_send_raw_packet(pccomm_packet_t *packet);

/**
 *  As you can see, this macro is slightly complicated, but it's actually doing something complicated...
 *
 *  It's goal is to extract one of the member of a structure, for each structure in an array.
 *
 *  Let's say we have an array of structure containing 3 members A,B,C, if we have
 *  an array of 5 of these structures, memory would look like this:
 *  A1B1C1A2B2C2A3B3C3A4B4C4A5B5C5
 *
 *  The macro will extract and send member B of this array of structure:
 *  send(B1B2B3B4B5)
 *
 *  In a more concrete way:
 *  typedef struct {
 *      type_a A;
 *      type_b B;
 *      type_c C;
 *  } my_struct_t;
 *  my_struct_t my_array[42];
 *
 *  PC_COMM_SEND_MEMBERS(mod, com, my_struct_t, B, my_array, 42);
 *  would send the B field of all my_struct_t in my_array.
 *
 *  \sa pc_comm_send_member_of_struct_array
 *  \note the macro use the type of the array variable, it must be a pointer
 *  to the actual type of the array
 */
#define PC_COMM_SEND_MEMBERS(header, array, content_member, item_nb) do {        \
    const int item_offset = offsetof(typeof(*array), content_member);            \
    const int item_size = sizeof(((typeof(*array) *)0)->content_member);         \
    pc_comm_send_member_of_struct_array(header->module, header->command, array,  \
            sizeof(typeof(*array)), item_offset, item_size, item_nb);            \
} while(0)


/**
 * This macro basically does the opposite of PC_COMM_SEND_MEMBERS.
 *
 * Instead of taking data from an array of structure to send it to usb, it takes data from usb and
 * store it in an array of structure.
 *
 * \sa PC_COMM_RX_MEMBERS
 * \note the macro use the type of the array variable, it must be a pointer
 *  to the actual type of the array
 */
#define PC_COMM_RX_MEMBERS(packet, array, content_member, item_nb) do {     \
    const int item_offset = offsetof(typeof(*array), content_member);       \
    const int item_size = sizeof(((typeof(*array)*)0)->content_member);     \
    pc_comm_receive_member_of_struct_array(packet, (char*)array,            \
        sizeof(*array), item_offset, item_size, item_nb);           \
} while(0)



/**
 * Register a list of packet reception callbacks
 * This function is used by modules when they are interested in one or several
 * messages that can be sent by the PC. Data referenced in pccom_callbacks must be
 * defined statically because they won't be copied.
 */
void pc_comm_register_module_callback(uint8_t module, pccom_callbacks callbacks);


/**
 * Module initialization
 */
void pc_comm_init();


#endif /* PC_COMM_H_ */
