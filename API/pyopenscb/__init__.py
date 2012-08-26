#  This file is part of OpenSCB project <http://openscb.org>.
#  Copyright (C) 2010  Opendrain
#
#  OpenSCB software is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenSCB software is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with OpenSCB software.  If not, see <http://www.gnu.org/licenses/>.
"""
Python binding for the OpenSCB API library.
"""

import sys
import ctypes
from ctypes import *

#python binding version, should match version of API/firmware
OPENSCB_BINDING_VERSION = "0.2"

MAX_PAYLOAD_SIZE = 60
MAX_OUT_NB = 24
MAX_IN_NB = 12
SLOT_DESC_SIZE = 50
IO_NAME_LEN = 20
FLASH_SLOT_NB = 16

(
    MODE_NORMAL,
    MODE_OUTPUT_CALIB,
    MODE_INPUT_CALIB,
) = range(3)


class scb_msg_header(BigEndianStructure):
    _fields_ = [("module", c_uint8),
                ("command", c_uint8),
                ("index", c_uint8),
                ("size", c_uint8)]

class scb_packet(Structure):
    _fields_ = [("header", scb_msg_header),
                ("data", c_uint8 * MAX_PAYLOAD_SIZE)]


class constant_type(Structure):
    _fields_ = [("name", c_char_p),
                ("val", c_int) ]

class calib_input(BigEndianStructure):
    _fields_ = [("min", c_short),
                ("mid", c_short),
                ("max", c_short)]

class calib_output(BigEndianStructure):
    _fields_ = [("min", c_short),
                ("max", c_short),
                ("angle_180", c_short),
                ("subtrim", c_short)]

class calib_raw(BigEndianStructure):
    _fields_ = [("value", c_short)]

class slot_header(BigEndianStructure):
    _fields_ = [("type", c_uint32),
                ("desc", c_char * SLOT_DESC_SIZE),
                ("size", c_uint32)]


class seq_frame(BigEndianStructure):
    _fields_ = [("duration", c_uint32),
                ("active_bm", c_uint32),
                ("position", c_short * MAX_OUT_NB)]


class SlotHeader():
    SLOT_UNINIT = 0xFFFFFFFF
    SLOT_EMPTY = 0x4E4F4E45
    SLOT_IO_SETTINGS = 0x53455555
    SLOT_OUT_SEQUENCE = 0x4F534551
    SLOT_IN_SEQUENCE = 0x49534551

    def __init__(self, type, desc):
        self.type = type
        self.desc = desc

class OuputCalib():
    def __init__(self, min, max, angle_180, subtrim):
        self.min = min
        self.max = max
        self.angle_180 = angle_180
        self.subtrim = subtrim

    def __str__(self):
        return "%d, %d, %d, %d" % (self.min, self.max, self.angle_180,
                                   self.subtrim)

class InputCalib():
    def __init__(self, min, mid, max):
        self.min = min
        self.mid = mid
        self.max = max

    def __str__(self):
        return "%d, %d, %d" % (self.min, self.mid, self.max)



class DeviceNotFound(BaseException):
    pass

class TwoOrMoreBoard(BaseException):
    pass

class BoardInBootloaderMode(BaseException):
    pass

class ConnectionProblem(BaseException):
    def __init__(self, ret):
        BaseException.__init__(self)
        self.ret_code = ret

    def __str__(self):
        return "libusb connection problem: %d", self.ret_code

class OpenSCBBoard():
    def __init__(self):
        try:
            if sys.platform == 'win32':
                self._lib = ctypes.CDLL("../../build/libopenscb_api.dll")
            else:
                self._lib = ctypes.CDLL("../../build/libopenscb_api.so")
        except:
            print "Cannot load OpenSCB API library"
            raise

        self._dev = None
        self._output_nb = None
        self._input_nb = None


    def get_debug_message(self, timeout=100):
        #for some reason, libusb 0.1 sometimes returns a timeout even if data
        #has been read. So we don't read the return value and return data
        #anyway. 
        if self.is_connected():
            c_msg = create_string_buffer(64)
            self._lib.scb_get_debug_message(self._dev, c_msg, 64, timeout)
            return c_msg.value
        return ""

    def connect(self):
        if self._dev:
            self.close()

        board_nb = self._lib.usb_get_openscb_nb()
        if board_nb > 1:
            raise TwoOrMoreBoard()

        if board_nb == 0:
            dfu_nb = self._lib.usb_get_dfuprog_nb()
            if dfu_nb > 0:
                raise BoardInBootloaderMode()

        self._dev = self._lib.usb_connect_first_board()

        if self._dev == 0:
            self._dev = None
            raise DeviceNotFound()

    def close(self):
        if self._dev:
            self._lib.scb_close_board(self._dev)
            self._dev = None

    def is_connected(self):
        return self._dev != None

    def _check_return_code(self, ret):
        if ret < 0:
            self.close()
            raise ConnectionProblem(ret)

    def _build_array(self, type, values):
        nb = c_ubyte(len(values))
        out = (type * nb.value)(*values)
        return nb, out

    def _get_in_array(self, type, function):
        if self.is_connected():
            nb = self.get_input_nb()
            inputs = (type * nb)()
            res = function(self._dev, inputs, c_ubyte(nb))
            self._check_return_code(res)
            return [o for o in inputs]

    def _get_out_array(self, type, function):
        if self.is_connected():
            nb = self.get_output_nb()
            outputs = (type * nb)()
            res = function(self._dev, outputs, c_ubyte(nb))
            self._check_return_code(res)
            return [o for o in outputs]

    def _set_out_array(self, type, values, function):
        if self.is_connected():
            nb, out = self._build_array(type, values)
            res = function(self._dev, out, nb)
            self._check_return_code(res)

    def check_firmware_version(self):
        if self.is_connected():
            version = create_string_buffer(64)
            compat = c_bool()
            res = self._lib.scb_check_firmware_version(self._dev,
                                        byref(compat), version, 64)
            self._check_return_code(res)

            if version.value != OPENSCB_BINDING_VERSION:
                compatible = False
            else:
                compatible = compat.value

            return compatible, version.value

    def restart_bootloader(self):
        if self.is_connected():
            res = self._lib.scb_req_restart_to_bootloader(self._dev)
            self._check_return_code(res)

    def get_input_nb(self):
        if self._input_nb is None:
            if self.is_connected():
                nb = c_ubyte()
                res = self._lib.scb_get_input_nb(self._dev, byref(nb))
                self._check_return_code(res)
                self._input_nb = nb.value
        return self._input_nb

    def get_input_values(self):
        return self._get_in_array(c_float, self._lib.scb_get_input_values)

    def get_output_nb(self):
        if self._output_nb is None:
            if self.is_connected():
                nb = c_ubyte()
                res = self._lib.scb_get_output_nb(self._dev, byref(nb))
                self._check_return_code(res)
                self._output_nb = nb.value
        return self._output_nb

    def get_output_values(self):
        return self._get_out_array(c_float, self._lib.scb_get_output_values)

    def get_output_goal(self):
        return self._get_out_array(c_float, self._lib.scb_get_output_goal)

    def get_output_speed(self):
        return self._get_out_array(c_ubyte, self._lib.scb_get_output_speed)

    def get_output_names(self):
        c_names = self._get_out_array(c_char * IO_NAME_LEN, self._lib.scb_get_output_name)
        return [str(name.value) for name in c_names]

    def get_out_enabled(self):
        return self._get_out_array(c_bool, self._lib.scb_get_out_enabled)

    def set_output_names(self, names):
        fixed_sz_names = [[i for i in n[:IO_NAME_LEN]] for n in names]
        c_names = [(c_char * IO_NAME_LEN)(*n) for n in fixed_sz_names]
        self._set_out_array(c_char * IO_NAME_LEN, c_names, self._lib.scb_set_output_name)


    def set_one_output_goal(self, out_no, output):
        if self.is_connected():
            no = c_ubyte(out_no)
            out = c_float(output)
            self._lib.scb_set_one_output_goal(self._dev, no, out)

    def set_output_goal(self, outputs):
        self._set_out_array(c_float, outputs, self._lib.scb_set_output_goal)

    def set_output_speed(self, speeds):
        self._set_out_array(c_ubyte, speeds, self._lib.scb_set_out_speed)

    def set_out_controlled(self, controlled):
        self._set_out_array(c_bool, controlled, self._lib.scb_set_out_controlled)

    def set_out_enabled(self, enabled):
        self._set_out_array(c_bool, enabled, self._lib.scb_set_out_enabled)

    def set_mode(self, mode):
        if self.is_connected():
            m = c_ubyte(mode)
            res = self._lib.scb_set_core_mode(self._dev, m)
            self._check_return_code(res)

    def save_sys_conf(self):
        if self.is_connected():
            res = self._lib.scb_save_sys_conf(self._dev)
            self._check_return_code(res)

    def in_calib_set_center(self):
        if self.is_connected():
            res = self._lib.scb_in_calib_set_center(self._dev)
            self._check_return_code(res)

    def get_in_calib_values(self):
        if self.is_connected():
            nb = self.get_input_nb()
            calib = (calib_input * nb)()
            res = self._lib.scb_get_in_calib_values(self._dev, calib, c_ubyte(nb))
            self._check_return_code(res)
            return [InputCalib(c.min, c.mid, c.max) for c in calib]

    def get_out_calib_values(self):
        if self.is_connected():
            nb = self.get_output_nb()
            calib = (calib_output * nb)()
            res = self._lib.scb_get_out_calib_values(self._dev, calib, c_ubyte(nb))
            self._check_return_code(res)
            return [OuputCalib(c.min, c.max, c.angle_180, c.subtrim) for c in calib]

    def set_out_calib_values(self, calib):
        if self.is_connected():
            nb = c_ubyte(len(calib))
            cal = (calib_output * nb.value)()
            for c, d in zip(cal, calib):
                c.min = d.min
                c.max = d.max
                c.angle_180 = d.angle_180
                c.subtrim = d.subtrim
            res = self._lib.scb_set_out_calib_values(self._dev, cal, nb)
            self._check_return_code(res)

    def set_out_calib_raw(self, no, value):
        if self.is_connected():
            c_no = c_ubyte(no)
            c_val = calib_raw(int(value))
            res = self._lib.scb_set_out_calib_raw(self._dev, c_no, c_val)
            self._check_return_code(res)

    def _load_frame(self, duration, pos_dic):
        """pos_dic = {0: 0.3, 5: 0.4, 12: -0.8}"""
        act = []
        pos = []
        for i in range(MAX_OUT_NB):
            if i in pos_dic:
                act.append(True)
                pos.append(pos_dic[i])
            else:
                act.append(False)
                pos.append(0.0)

        c_nb = c_ubyte(len(pos))
        c_duration = c_uint32(int(duration))
        c_pos = (c_float * c_nb.value)(*pos)
        c_act = (c_bool * c_nb.value)(*act)

        return c_duration, c_pos, c_act, c_nb

    def load_frame(self, duration, pos_dic):
        if self.is_connected():
            data = self._load_frame(duration, pos_dic)
            res = self._lib.scb_load_frame(self._dev, *data)
            self._check_return_code(res)

    def play_sequence(self, slot_id):
        if self.is_connected():
            c_id = c_uint32(int(slot_id))
            res = self._lib.scb_play_sequence_slot(self._dev, c_id)
            self._check_return_code(res)

    def upload_sequence_frame(self, idx, duration, pos_dic):
        if self.is_connected():
            c_idx = c_ubyte(idx)
            data = self._load_frame(duration, pos_dic)
            res = self._lib.scb_upload_sequence_frame(self._dev, c_idx, *data)
            self._check_return_code(res)

    def upload_sequence_start(self, slot_id, frame_nb):
        if self.is_connected():
            c_id = c_uint32(int(slot_id))
            c_nb = c_uint16(int(frame_nb))
            res = self._lib.scb_upload_sequence_start(self._dev, c_id, c_nb)
            self._check_return_code(res)

    def upload_sequence_end(self, seq_description):
        if self.is_connected():
            c_seq_desc = c_char_p(seq_description)
            res = self._lib.scb_upload_sequence_end(self._dev, c_seq_desc)
            self._check_return_code(res)

    def store_settings_to_slot(self, slot_id):
        if self.is_connected():
            c_id = c_uint32(int(slot_id))
            res = self._lib.scb_store_settings_to_slot(self._dev, c_id)
            self._check_return_code(res)

    def load_settings_from_slot(self, slot_id):
        if self.is_connected():
            c_id = c_uint32(int(slot_id))
            res = self._lib.scb_load_settings_from_slot(self._dev, c_id)
            self._check_return_code(res)

    def download_sequence(self, slot_id):
        if self.is_connected():
            frame_nb = 64
            c_id = c_uint32(int(slot_id))
            c_nb = c_uint8(int(frame_nb))
            c_frame = (seq_frame * frame_nb)()

            res = self._lib.scb_download_sequence(self._dev, c_id, c_frame, pointer(c_nb))
            self._check_return_code(res)

            #rebuild received frames in a pythonic format 
            result = []
            for i in range(c_nb.value):
                duration = c_frame[i].duration / 1000.0
                pos_dic = {}
                for j in range(MAX_OUT_NB):
                    if c_frame[i].active_bm & (1 << j):
                        pos_dic[j] = c_frame[i].position[j] / 32768.0
                result.append((duration, pos_dic))
            return result

    def disable_frame(self):
        if self.is_connected():
            res = self._lib.scb_disable_frame(self._dev)
            self._check_return_code(res)

    def get_flash_overview(self):
        if self.is_connected():
            c_nb = c_ubyte(FLASH_SLOT_NB)
            c_slots = (slot_header * FLASH_SLOT_NB)()
            res = self._lib.scb_get_flash_overview(self._dev, c_slots, c_nb)
            self._check_return_code(res)
            return [SlotHeader(s.type, s.desc) for s in c_slots]

    def get_flash_slot(self, slot_id):
        if self.is_connected():
            c_id = c_ubyte(slot_id)
            c_slot = (slot_header * 1)()
            res = self._lib.scb_get_flash_slot(self._dev, c_id, c_slot)
            self._check_return_code(res)
            return SlotHeader(c_slot[0].type, c_slot[0].desc)

    def set_flash_slot_description(self, slot_id, description):
        if self.is_connected():
            c_id = c_ubyte(slot_id)
            c_desc = c_char_p(description)
            res = self._lib.scb_set_flash_slot_description(self._dev, c_id, c_desc)
            self._check_return_code(res)



import Queue

class OpenSCBSimu():

    def __init__(self):
        self._connected = False
        self._in_nb = MAX_IN_NB
        self._out_nb = MAX_OUT_NB

        self._goal = [0.0, ] * self._out_nb
        self._inputs = [0.0, ] * self._in_nb
        self._outputs = [0.0, ] * self._out_nb
        self._speeds = [0, ] * self._out_nb
        self._names = [a for a in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ']
        self._out_enabled = [False, ] * self._out_nb

        self._in_calib = [InputCalib(1000, 2000, 3000) for _ in range(self._in_nb)]
        self._out_calib = [OuputCalib(1000, 4000, 3000, 2500) for _ in range(self._out_nb)]
        self._flash_slot = [SlotHeader(SlotHeader.SLOT_UNINIT, "Simu flash") for _ in range(8)]

        self.__dbg_msg = Queue.Queue()

    def __log(self, txt):
        self.__dbg_msg.put(txt)

    def get_debug_message(self, timeout=100):
        try:
            return self.__dbg_msg.get(True, timeout)
        except Queue.Empty:
            return None

    def connect(self):
        self.__log("Board connected")
        self._connected = True

    def close(self):
        self.__log("Board disconnected")
        self._connected = False

    def is_connected(self):
        return self._connected

    def check_firmware_version(self):
        self.__log("Board simulation")
        return True, "Simu"

    def restart_bootloader(self):
        self.__log("Restarting to bootloader")

    def get_out_enabled(self):
        return self._out_enabled

    def set_out_controlled(self, controlled):
        self._out_controlled = controlled

    def set_out_enabled(self, enabled):
        self._out_enabled = enabled

    def get_input_nb(self):
        return self._in_nb

    def get_output_nb(self):
        return self._out_nb

    def get_input_values(self):
        return self._inputs

    def get_output_values(self):
        return self._outputs

    def get_output_speed(self):
        return self._speeds

    def get_output_names(self):
        return self._names

    def get_output_goal(self):
        return self._goal

    def set_output_names(self, names):
        self._names = names

    def set_output_goal(self, outputs):
        self._goal = outputs

    def set_one_output_goal(self, out_no, output):
        self._goal[out_no] = output

    def set_output_speed(self, speeds):
        self._speeds = speeds

    def set_mode(self, mode):
        self.__log("Setting board mode: %d" % mode)

    def save_sys_conf(self):
        self.__log("Saving system conf")


    def in_calib_set_center(self):
        pass

    def get_in_calib_values(self):
        return self._in_calib

    def get_out_calib_values(self):
        return self._out_calib

    def set_out_calib_values(self, calib):
        self._out_calib = calib

    def set_out_calib_raw(self, no, value):
        self.__log("Out %d raw: %d" % (no, value))

    def load_frame(self, duration, pos_dic):
        pass
    def disable_frame(self):
        pass

    def play_sequence(self, slot_id):
        self.__log("Playing sequence in slotid: %d" % slot_id)
    def upload_sequence_frame(self, idx, duration, pos_dic):
        self.__log("* Saving sequence frame: %d" % idx)
    def upload_sequence_start(self, slot_id, frame_nb):
        self.__log("Saving sequence start: %d" % slot_id)
        self._seq_slot = slot_id
    def upload_sequence_end(self, seq_description):
        self.__log("Saving sequence end")
        self._flash_slot[self._seq_slot].type = SlotHeader.SLOT_OUT_SEQUENCE
        self._flash_slot[self._seq_slot].desc = seq_description

    def download_sequence(self, slot_id):
        self.__log("Downloading sequence: %d" % slot_id)

    def store_settings_to_slot(self, slot_id):
        self.__log("Saving settings to slot")
        self._flash_slot[slot_id].type = SlotHeader.SLOT_IO_SETTINGS
        self._flash_slot[slot_id].desc = "Settings"

    def load_settings_from_slot(self, slot_id):
        self.__log("Loading settings from slot")


    def get_flash_overview(self):
        return self._flash_slot
    def get_flash_slot(self, slot_id):
        return self._flash_slot[slot_id]
    def set_flash_slot_description(self, slot_id, description):
        self._flash_slot[slot_id].desc = description


import time
if __name__ == '__main__':
    board = OpenSCBBoard()

    board.connect()

    print 'version =', board.check_firmware_version()

    slots = board.get_flash_overview()
    for s in slots:
        print s.type, s.desc


    nb = board.get_output_nb()
    board.set_out_controlled([True, ]*nb)
    board.set_output_speed([0, ] * nb)
    board.set_output_goal([0.8, ]*nb)
    board.set_out_enabled([True, ]*nb)

    time.sleep(2)

    v = board.get_output_values()
    v = [i + 0.1 for i in v]
    board.set_output_goal(v)
    v = board.get_output_values()
    print v

    v = board.get_out_calib_values()
    for c in v:
        print c
    board.set_out_calib_values(v)

    board.close()


