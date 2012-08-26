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
Board communication
"""

from PyQt4 import QtGui, QtCore
from PyQt4.QtCore import QObject, pyqtSignal

import json

import sequence
import pyopenscb


class IncompatibleVersion(Exception):
    def __init__(self, ver_firm, ver_api):
        Exception.__init__(self)
        self.ver_firm = ver_firm
        self.ver_api = ver_api

class Board(QObject):
    boardConnected = pyqtSignal()
    boardDisconnected = pyqtSignal()
    positionUpdated = pyqtSignal()
    inputUpdated = pyqtSignal()
    settingsUpdated = pyqtSignal()
    enabledChanged = pyqtSignal()

    MAX_INPUT_NB = pyopenscb.MAX_IN_NB
    MAX_SERVO_NB = pyopenscb.MAX_OUT_NB

    def __init__(self, openscb_board):
        QObject.__init__(self)
        self._board = openscb_board
        self.input_nb = self.MAX_INPUT_NB
        self.output_nb = self.MAX_SERVO_NB
        self.out_values = None
        self.in_values = None
        self._generate_out_colors()

    def connect(self):
        self._board.connect()

        compat, vers = self._board.check_firmware_version()
        self.version = vers
        if not compat:
            raise IncompatibleVersion(vers, pyopenscb.OPENSCB_BINDING_VERSION)

        self.update_values()
        self.set_mode(pyopenscb.MODE_NORMAL)
        self.boardConnected.emit()

    def is_connected(self):
        return self._board.is_connected()

    def get_debug_message(self, timeout):
        return self._board.get_debug_message(timeout)

    def update_position(self):
        new_values = self._board.get_output_values()
        if new_values != self.out_values:
            self.out_values = new_values
            self.positionUpdated.emit()

        new_values = self._board.get_input_values()
        if new_values != self.in_values:
            self.in_values = new_values
            self.inputUpdated.emit()

    def update_values(self):
        self.input_nb = self._board.get_input_nb()
        self.output_nb = self._board.get_output_nb()

        self.sv_enabled = self._board.get_out_enabled()
        self.prev_sv_enabled = self.sv_enabled[:]
        self.out_api_controlled = [False, ] * self.output_nb

        self.in_values = self._board.get_input_values()

        self.out_name = self._board.get_output_names()
        self.out_values = self._board.get_output_values()
        self.goal_values = self._board.get_output_goal()
        self.out_speed = self._board.get_output_speed()

        self.in_calib = self._board.get_in_calib_values()
        self.out_calib = self._board.get_out_calib_values()

        self.flash_slots = self._board.get_flash_overview()

        self._sequence = []
        for slot_id, slot in enumerate(self.flash_slots[1:]):
            seq = sequence.Sequence()
            if slot.type == pyopenscb.SlotHeader.SLOT_OUT_SEQUENCE:
                frames = self.download_sequence(slot_id + 1)
                if frames:
                    seq.set_description(slot.desc)
                    time = 0
                    for frm in frames:
                        time += frm[0]
                        seq.add_frame(time, frm[1])

            self._sequence.append(seq)

    def get_enabled_outputs(self):
        return [i for i in range(self.output_nb) if self.sv_enabled[i]]

    def upload_setting_values(self):
        self._board.set_output_names(self.out_name)
        self._board.set_out_calib_values(self.out_calib)
        self._board.set_output_speed(self.out_speed)
        self._board.set_out_enabled(self.sv_enabled)

    def store_setting_values(self, slot_id):
        self._board.store_settings_to_slot(slot_id)

    def load_setting_values(self, slot_id):
        self._board.load_settings_from_slot(slot_id)

    def _generate_out_colors(self):
        nb = self.MAX_SERVO_NB
        prime = 17
        tab = [ (0.0 + i * (1.0 / nb)) for i in range(nb) ]
        self._out_color = []
        for i in range(nb):
            h = tab[(prime * i) % nb]
            self._out_color.append(QtGui.QColor.fromHslF(h, 0.2, 0.9))

    def import_config_from_json(self, json_data):
        values = json.loads(json_data)
        (name, max_speed, imp_calib, enabled) = values
        calib = [pyopenscb.calib_output(*c) for c in imp_calib]
        self.out_name = name
        self.out_speed = max_speed
        self.out_calib = calib
        self.sv_enabled = enabled
        self.settingsUpdated.emit()
        #update the board
        self.upload_setting_values()

    def _export_config_to_json(self):
        calib = [(c.min, c.max, c.angle_180, c.subtrim) for c in self.out_calib]
        values = [self.out_name, self.out_speed, calib, self.sv_enabled]
        return json.dumps(values)

    def export_all_to_json(self):
        res = self._export_config_to_json()
        #FIXME: frame is not json serializable
        for slot_id, seq in enumerate(self._sequence):
            res += '\n' + json.dumps((slot_id, seq.all_to_json()))
        return res

    def import_all_from_json(self, json_txt):
        json_lines = json_txt.split('\n')
        self.import_config_from_json(json_lines[0])

        for line in json_lines[1:]:
            slot_id, seq_json = json.loads(line)
            self._sequence[slot_id].remove_all()
            self._sequence[slot_id].import_from_json(seq_json)

    def get_sequence_nb(self):
        return len(self._sequence)

    def get_sequence(self, slot):
        return self._sequence[slot - 1]

    def copy_sequence(self, slot, sequence):
        self._sequence[slot - 1].copy_sequence(sequence)

    def save_all_to_userflash(self):
        self.store_setting_values(0)
        for slot_id, seq in enumerate(self._sequence):
            self.upload_sequence(slot_id + 1, seq)

    def get_flash_slot(self, nb):
        return self.flash_slots[nb]

    def get_flash_slots(self):
        return self.flash_slots

    def update_flash_slot(self, slot_id):
        self.flash_slots[slot_id] = self._board.get_flash_slot(slot_id)

    def set_flash_slot_desc(self, slot_id, desc):
        self._board.set_flash_slot_description(slot_id, str(desc))
        self.update_flash_slot(slot_id)

    def set_mode(self, mode):
        self._board.set_mode(mode)

    def load_frame(self, duration, points):
        self._board.load_frame(1000 * duration, points)

    def disable_frame(self):
        self._board.disable_frame()

#    def play_sequence(self, slot_id):
#        self._board.play_sequence(slot_id)

    def upload_sequence(self, slot_id, sequence):
        frame_nb = len(sequence.frame)
        self._board.upload_sequence_start(slot_id, frame_nb)

        prev_time = 0
        for idx, frm in enumerate(sequence.frame):
            duration = frm.get_time() - prev_time
            self._board.upload_sequence_frame(idx, duration * 1000, frm.get_point_dict())
            prev_time = frm.get_time()

        self._board.upload_sequence_end(sequence.get_description())
        self.update_flash_slot(slot_id)

    def download_sequence(self, slot_id):
        if self.flash_slots[slot_id].type == pyopenscb.SlotHeader.SLOT_OUT_SEQUENCE:
            return self._board.download_sequence(slot_id)

    def input_calib_set_center(self):
        self._board.in_calib_set_center()

    def get_input_calib(self, in_no):
        return self.in_calib[in_no]

    def get_input_value(self, out_no):
        return self.in_values[out_no]

    def get_calib_min(self, out_no):
        return int(self.out_calib[out_no].min)
    def get_calib_center(self, out_no):
        return int(self.out_calib[out_no].subtrim)
    def get_calib_max(self, out_no):
        return int(self.out_calib[out_no].max)
    def get_calib_angle180(self, out_no):
        return int(self.out_calib[out_no].angle_180)

    def set_calib_min(self, out_no, value):
        self.out_calib[out_no].min = value
        self.settingsUpdated.emit()
    def set_calib_center(self, out_no, value):
        self.out_calib[out_no].subtrim = value
        self.settingsUpdated.emit()
    def set_calib_max(self, out_no, value):
        self.out_calib[out_no].max = value
        self.settingsUpdated.emit()
    def set_calib_angle180(self, out_no, value):
        self.out_calib[out_no].angle_180 = value
        self.settingsUpdated.emit()

    def send_all_calib_values(self):
        self._board.set_out_calib_values(self.out_calib)

    def set_calib_raw(self, out_no, value):
        self._board.set_out_calib_raw(out_no, value)

    def disable_calib_raw(self):
        self._board.set_out_calib_raw(0xFF, 2500)

    def get_output_range(self, out_no):
        min = self.get_calib_min(out_no)
        center = self.get_calib_center(out_no)
        max = self.get_calib_max(out_no)
        angle = self.get_calib_angle180(out_no)

        left = (min - center) / float(angle)
        right = (max - center) / float(angle)
        return (left, right)

    def is_output_enabled(self, out_no):
        return self.sv_enabled[out_no]

    def set_output_enabled(self, out_no, enabled):
        try:
            for sv in iter(out_no):
                self.sv_enabled[sv] = enabled
        except TypeError:
            self.sv_enabled[out_no] = enabled
        self._board.set_out_enabled(self.sv_enabled)
        self.enabledChanged.emit()

    def disable_all_outputs(self):
        if any(self.sv_enabled):
            self.prev_sv_enabled = self.sv_enabled[:]
        self.sv_enabled = [False, ] * self.output_nb
        self._board.set_out_enabled(self.sv_enabled)
        self.enabledChanged.emit()

    def enable_all_outputs(self):
        self.sv_enabled = self.prev_sv_enabled[:]
        self._board.set_out_enabled(self.sv_enabled)
        self.enabledChanged.emit()

    def is_out_api_controlled(self, out_no):
        return self.out_api_controlled[out_no]

    def set_out_api_controlled(self, out_no, controlled):
        self.out_api_controlled[out_no] = controlled
        self._board.set_out_controlled(self.out_api_controlled)

    def get_output_names(self):
        return self.out_name

    def get_output_name(self, out_no):
        if out_no < 0:
            return 'None'
        try:
            return self.out_name[out_no]
        except (IndexError, TypeError):
            return 'None'

    def find_output_name(self, name):
        try:
            return self.out_name.index(name)
        except ValueError:
            return - 1

    def get_output_color(self, out_no):
        if self.is_output_enabled(out_no):
            return self._out_color[out_no]
        else:
            return QtGui.QColor(QtCore.Qt.lightGray)

    def get_output_value(self, out_no):
        return self.out_values[out_no]

    def get_output_goal(self, out_no):
        return self.goal_values[out_no]

    def get_output_speed(self, out_no):
        return self.out_speed[out_no]

    def set_output_name(self, out_no, name):
        self.out_name[out_no] = str(name)
        self._board.set_output_names(self.out_name)
        self.settingsUpdated.emit()

    def set_output_color(self, out_no, color):
        self._out_color[out_no] = color

    def set_output_goal(self, out_no, goal):
        self.goal_values[out_no] = goal
        self._board.set_one_output_goal(out_no, goal)

    def set_output_speed(self, out_no, speed):
        self.out_speed[out_no] = speed
        self._board.set_output_speed(self.out_speed)

