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

#ifndef PC_COMM_CONST_H_
#define PC_COMM_CONST_H_

#ifdef __cplusplus
extern "C" {
#endif


/** \name OpenSCB communication constant values*/
/// @{

/**
 * List of the current modules that use PC communication
 */
typedef enum {
    PCCOM_TRACE,

    PCCOM_SYSTEM,

    PCCOM_CORE,
    PCCOM_CORE_CALIBRATION,
    PCCOM_SYS_CONF,

    PCCOM_API_CTRL,
    PCCOM_API_IO,

    PCCOM_POS_CTRL,
    PCCOM_SEQ_CTRL,

    PCCOM_USER_FLASH,

    PCCOM_MODULE_NB
} PCCOM_MODULE;



enum {
    TRACE_LOG,
    TRACE_MEMORY_DUMP,
};

enum {
    RESTART_BOOTLOADER,
    REQ_SOFT_VERSION,
};

enum {
    SET_CORE_MODE,

    REQ_INPUT_NB,
    REQ_OUTPUT_NB,

    REQ_INPUT_VALUE,
    REQ_OUTPUT_VALUE,

    REQ_INPUT_ACTIVE,
    REQ_OUTPUT_ACTIVE,
};

enum {
    SET_OUTPUT_CALIB_RAW,
    SET_INPUT_CALIB_CENTER,
};

enum {
    SAVE_SYS_CONF,
    LOAD_SYS_CONF,

    SET_INPUT_MAPPING,
    SET_OUTPUT_MAPPING,

    REQ_INPUT_MAPPING,
    REQ_OUTPUT_MAPPING,

    SET_INPUT_NAME,
    SET_OUTPUT_NAME,

    REQ_INPUT_NAME,
    REQ_OUTPUT_NAME,

    SET_OUTPUT_MAX_SPEED,
    REQ_OUTPUT_MAX_SPEED,

    REQ_INPUT_ACTIVE_BM,
    REQ_OUTPUT_ACTIVE_BM,

    SET_INPUT_ACTIVE_BM,
    SET_OUTPUT_ACTIVE_BM,

    REQ_INPUT_CALIB_VALUE,
    REQ_OUTPUT_CALIB_VALUE,

    SET_INPUT_CALIB_VALUE,
    SET_OUTPUT_CALIB_VALUE,
};

enum {
    API_SET_OUTPUT_GOAL,
    API_REQ_OUTPUT_GOAL,

    API_SET_CONTROL_BM,
    API_REQ_CONTROL_BM,
};

#if 0
enum {
    API_IO_SET_INPUT_VALUE,
    API_IO_SET_INPUT_ACTIVE,

    API_IO_REQ_OUTPUT_VALUE,
    API_IO_REQ_OUTPUT_ACTIVE,
};
#endif

enum {
    POS_CTRL_LOAD_FRAME,
    POS_CTRL_DISABLE,
};

enum {
    SEQ_CTRL_PLAY_SLOT,

    SEQ_CTRL_UPLOAD_SLOT_START,
    SEQ_CTRL_UPLOAD_SLOT_FRAME,
    SEQ_CTRL_UPLOAD_SLOT_END,

    SEQ_CTRL_DOWNLOAD_SLOT,
    SEQ_CTRL_DOWNLOAD_SLOT_FRAME,
};

enum {
    FLASH_REQ_OVERVIEW,
    FLASH_REQ_OVERVIEW_ALL,

    FLASH_SET_DESCRIPTION,
};


/// @}


#ifdef __cplusplus
}
#endif

#endif /* PC_COMM_CONST_H_ */
