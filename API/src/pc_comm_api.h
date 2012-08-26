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

#ifndef PC_COMM_API_H_
#define PC_COMM_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>


/** System version
 */
//! \{

// This version number is shared between firmware and API
#define OPENSCB_SOFT_VERSION    "0.2"

//! \}

/** USB communication settings
 */
//! \{
#define OPENSCB_CLASS	0xFE
#define OPENSCB_VID		0X03EB
#define OPENSCB_PID		0XA380

#define DFU_BTLDR_CLASS 0xFE
#define DFU_BTLDR_VID   0x03EB
#define DFU_BTLDR_PID   0x2FF6

#define OPENSCB_SLAVE_EP_RCV 	0x81
#define OPENSCB_SLAVE_EP_SEND   0x02

#define OPENSCB_MASTER_EP_RCV   0x83
#define OPENSCB_MASTER_EP_SEND  0x04

#define OPENSCB_CFG		1
#define OPENSCB_INTERF  0

#define MAX_PACKET_SIZE     64
#define MAX_PAYLOAD_SIZE    (MAX_PACKET_SIZE-sizeof(pccomm_msg_header_t))

//! \}

/**
 * From Atmel DSP lib, copied here for convenience,
 * I renamed them so these macros don't interfere with DSP lib
 */
//! \{
#define _DSP16_QA	1
#define _DSP16_QB	15

typedef int16_t 	_dsp16_t;

#define _DSP_FP_MAX(a, b)    (((float) (1 << ((a)-1))) - _DSP_FP_RES(a, b))
#define _DSP_FP_MIN(a, b)    (-((float) (1 << ((a)-1))))

#define _DSP_Q_MAX(a, b)     ((int32_t) (((uint32_t) -1) >> (32 - ((a)+(b)-1))))
#define _DSP_Q_MIN(a, b)     ((int32_t) ((-1) << ((a)+(b)-1)))

#define _DSP_FP_RES(a, b)    (1./((unsigned) (1 << (b))))

#define _DSP_Q(a, b, fnum)   (((fnum) >= _DSP_FP_MAX(a, b) - _DSP_FP_RES(a, b))?\
                            _DSP_Q_MAX(a, b):\
                            (((fnum) <= _DSP_FP_MIN(a, b) + _DSP_FP_RES(a, b))?\
                            _DSP_Q_MIN(a, b):\
                            (((fnum)*(((unsigned) (1 << (b))))))))

#define _DSP16_Q(fnum)       ((_dsp16_t) _DSP_Q(_DSP16_QA, _DSP16_QB, fnum))

//! \}

#define MAX_SERVO_NB    24
#define MAX_OUT_NB      MAX_SERVO_NB
#define MAX_IN_NB       12

#define IO_NAME_LEN     20

/*for some reason the min/max from ASF are not working for me*/
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define DSP16_MIN         _DSP_Q_MIN(_DSP16_QA, _DSP16_QB)
#define DSP16_MAX         _DSP_Q_MAX(_DSP16_QA, _DSP16_QB)

#define RC_VALUE_MIN      DSP16_MIN
#define RC_VALUE_MAX      DSP16_MAX

#define FLOAT_TO_RCVALUE      _DSP16_Q
#define RCVALUE_TO_FLOAT(raw) ((raw / (float)RC_VALUE_MAX) * (_DSP_FP_MAX(_DSP16_QA, _DSP16_QB)))

/**
 * Convert an angle in radian [-M_PI .. M_PI]
 * to its fixed point representation [-1.0 .. 1.0[
 */
#define RAD_TO_RCVALUE(f) FLOAT_TO_RCVALUE(f/M_PI)

/**
 * Same thing for angle in degrees [-180 .. 180]
 * to fixed point representation [-1.0 .. 1.0[
 */
#define DEG_TO_RCVALUE(f) FLOAT_TO_RCVALUE(f/180.0f)


/** \name Communication message structure */
/// @{

/**
 * Packet header, this is the first data always received/sent
 * when communicating with the PC.
 */
typedef struct
__attribute__((__packed__))
{
    uint8_t module;     //!< module id
    uint8_t command;    //!< command code (module specific)
    uint8_t index;      //!< index of the message (if that makes sense for this command)
    uint8_t size;       //!< total packet size including header
} pccomm_msg_header_t;


/**
 * Structure to store a full packet.
 */
typedef struct
__attribute__((__packed__))
{
    pccomm_msg_header_t header;
    uint8_t data[MAX_PAYLOAD_SIZE];
} pccomm_packet_t;

/// @}


/** \name Calibration type definitions */
/// @{
/**
 * calibration structure for inputs
 */
typedef struct
__attribute__((packed))
{
    _dsp16_t min;           ///< minimum value
    _dsp16_t mid;           ///< middle value
    _dsp16_t max;           ///< maximum value
} input_calib_data_t;


/**
 * calibration structure for outputs
 */
typedef struct
__attribute__((packed))
{
    _dsp16_t min;           ///< minimum value
    _dsp16_t max;           ///< maximum value
    _dsp16_t angle_180;     ///< how much in rc_raw_t is a 180 degrees angle?
    _dsp16_t subtrim;       ///< user middle origin
} output_calib_data_t;

/// @}


/** \name Frame and sequence type definitions */
/// @{
/**
 * Frame structure for outputs
 */
typedef struct
__attribute__((packed))
{
    uint32_t duration;
    uint32_t active_bm;
    _dsp16_t position[MAX_OUT_NB];
} frame_t;

/// @}


/** \name User flash slot type and definitions */
/// @{

typedef enum {
    SLOT_UNINIT = 0xFFFFFFFF,       //flash is uninitialized
    SLOT_EMPTY = 0x4E4F4E45,        //'NONE', the slot is free

    SLOT_IO_SETTINGS = 0x53455555,  //'SETT', the slot contains calibration
                                    //data, default settings for IO, ...

    SLOT_OUT_SEQUENCE = 0x4F534551, //'OSEQ', output sequence

    SLOT_IN_SEQUENCE = 0x49534551,  //'ISEQ', virtual input sequence
} SLOT_TYPE;

#define SLOT_DESC_SIZE  52

/*The whole slot has to fit in a 4kB flash page*/
typedef struct
__attribute__((packed))
{
    uint32_t type;
    char description[SLOT_DESC_SIZE];

    uint32_t size;
} slot_header_t;

/// @}

#ifdef __cplusplus
}
#endif

#endif /* PC_COMM_API_H_ */
