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


#ifndef OPENSCB_ADVANCED_H_
#define OPENSCB_ADVANCED_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "pc_comm_api.h"
#include "openscb_dev.h"


typedef char io_name_t[IO_NAME_LEN];

/**
 * Switch the board into a different mode (calibration, normal)
 *
 * \param dev handle to openscb device
 * \param mode new mode of the board
 * \return <0 on error
 */
int scb_set_core_mode(openscb_dev dev, uint8_t mode);

/**
 * Get mapping of input from the board
 *
 * \param dev handle to openscb device
 * \param mapping[out] number of the logical input linked to each physical input
 * \param nb maximum size of mapping array
 * \return <0 on error
 */
int scb_get_input_mapping(openscb_dev dev, uint8_t *mapping, uint8_t nb);

/**
 * Set mapping of input on the board
 *
 * \param dev handle to openscb device
 * \param mapping number of the logical input linked to each physical input
 * \param nb size of mapping array
 * \return <0 on error
 */
int scb_set_input_mapping(openscb_dev dev, uint8_t *mapping, uint8_t nb);


/**
 * Get all output names
 *
 * \param dev handle to openscb device
 * \param names array to store name from the board
 * \param nb size of names array
 * \return <0 on error
 */
int scb_get_output_name(openscb_dev dev, io_name_t *names, uint8_t nb);

/**
 * Set name for all output
 *
 * \param dev handle to openscb device
 * \param names array of names to put on each output
 * \param nb size of names array
 * \return <0 on error
 */
int scb_set_output_name(openscb_dev dev, io_name_t *names, uint8_t nb);


/**
 * Start a sequence store procedure, you then need to add frame
 * one by one using scb_upload_sequence_frame function.
 *
 * \param dev handle to openscb device
 * \param slot_id in what flash space the sequence will be saved
 * \param frame_nb number of frame that are part of the sequence
 * \return <0 on error
 */
int scb_upload_sequence_start(openscb_dev dev, uint8_t slot_id, uint16_t frame_nb);

/**
 * Send frame to be save in flash, you should have called
 * scb_upload_sequence_start before this function.
 * When all frame has been send, you can call scb_upload_sequence_end to
 * finish the job.
 *
 * \param dev handle to openscb device
 * \note see scb_load_frame for parameters definition
 * \return <0 on error
 */
int scb_upload_sequence_frame(openscb_dev dev, uint8_t index, uint32_t duration,
        const float *position, const bool *active, uint8_t nb);

/**
 * Terminate the sequence store procedure.
 *
 * \param dev handle to openscb device
 * \param seq_desc sequence description (might be truncated upon sending)
 * \return <0 on error
 */
int scb_upload_sequence_end(openscb_dev dev, const char *seq_desc);


/**
 * Get header of one flash slot
 *
 * \param dev handle to openscb device
 * \param slot_id slot to load
 * \param slot_header[out] result
 *
 */
int scb_get_flash_slot(openscb_dev dev, uint8_t slot_id, slot_header_t *slot_header);

/**
 * Get all header of available flash slots
 *
 * \param dev handle to openscb device
 * \param slot_header[out] result
 * \param nb number of element in slot_header array
 */
int scb_get_flash_overview(openscb_dev dev, slot_header_t *slot_header, uint8_t nb);


/**
 * Get current input calibration
 *
 * \param dev handle to openscb device
 * \param calib[out] list of input calibration
 * \param nb number of calibration structure to get from the board
 * \return <0 on error
 */
int scb_get_in_calib_values(openscb_dev dev, input_calib_data_t *calib, uint8_t nb);


/**
 * Get current output calibration
 *
 * \param dev handle to openscb device
 * \param calib[out] list of output calibration
 * \param nb number of calibration structure to get from the board
 * \return <0 on error
 */
int scb_get_out_calib_values(openscb_dev dev, output_calib_data_t *calib, uint8_t nb);

/**
 * Set output calibration
 *
 * \param dev handle to openscb device
 * \param calib list of output calibration
 * \param nb number of calibration structure in calib array
 * \return <0 on error
 */
int scb_set_out_calib_values(openscb_dev dev, output_calib_data_t *calib,
        uint8_t nb);
/**
 * Set raw value on one output (used for calibration)
 *
 * \param dev handle to openscb device
 * \param out_id output number
 * \param value raw value to apply to the output
 * \return <0 on error
 */
int scb_set_out_calib_raw(openscb_dev dev, uint8_t out_id, _dsp16_t value);

/**
 * Force the board to restart into bootloader mode
 *
 * \param dev handle to openscb device
 * \return <0 on error
 */
int scb_req_restart_to_bootloader(openscb_dev dev);


/**
 * Save system configuration
 *
 * \param dev handle to openscb device
 * \return <0 on error
 */
int scb_save_sys_conf(openscb_dev dev);


#ifdef __cplusplus
}
#endif

#endif /* OPENSCB_ADVANCED_H_ */


