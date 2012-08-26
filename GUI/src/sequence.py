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
Representation for sequences and frames
"""

from PyQt4.QtCore import QObject, pyqtSignal
import copy
import json


class SequenceSelectorProxy(QObject, object):
    """ Proxy class to handle sequence switching transparently """
    sequenceChanged = pyqtSignal()
    selectionChanged = pyqtSignal()

    def __init__(self, seq):
        QObject.__init__(self)
        self.change_sequence(seq)

    def __on_seq_chg(self):
        self.sequenceChanged.emit()

    def __on_sel_chg(self):
        self.selectionChanged.emit()

    def change_sequence(self, seq):
        self._sequence = seq
        self._sequence.sequenceChanged.connect(self.__on_seq_chg)
        self._sequence.selectionChanged.connect(self.__on_sel_chg)
        self.__on_seq_chg()

    def get_sequence(self):
        return self._sequence

    def __getattr__(self, name):
        return getattr(self._sequence, name)


class Frame(QObject):
    frameChanged = pyqtSignal()

    def __init__(self, sequence, time=0.0, name=None):
        QObject.__init__(self)
        if name is None:
            name = "Frm"
        self.name = name
        self._time = time
        self._point = {}
        self._updated = False
        self._sequence = sequence

    def clone(self, seq):
        new_frm = Frame(seq, self.get_time(), self.name)
        new_frm._point = copy.deepcopy(self._point)
        return new_frm


    def is_point_present(self, no):
        return no in self._point

    def change_output(self, prev, new):
        if prev != new:
            self._point[new] = self._point[prev]
            self.remove_point(prev)

    def add_point(self, no, value=0.0):
        if no in self._point:
            return
        self.set_point_value(no, value)
        self.frameChanged.emit()

    def remove_point(self, no):
        del self._point[no]
        self.frameChanged.emit()

    def get_point_value(self, no):
        return self._point[no]

    def set_point_value(self, no, value):
        if no in self._point and self._point[no] == value:
            return
        self._point[no] = value
        self._updated = True

    def get_time(self):
        return self._time

    def set_time(self, time):
        if time == self._time:
            return
        self._time = time
        self._updated = True
        self._sequence.time_updated()

    def get_points(self):
        return sorted(self._point.items())

    def get_visible_points(self):
        return [(k, p) for k, p in self.get_points() if self.is_visible(k)]

    def get_point_dict(self):
        return self._point

    def is_visible(self, no):
        return self._sequence.is_visible(no)

    def set_visibility(self, no, visible):
        return self._sequence.set_visibility(no, visible)

    def check_update(self):
        if self._updated:
            self.frameChanged.emit()
            self._updated = False

    def get_previous(self):
        frm_list = self._sequence.frame

        i = frm_list.index(self)
        if i == 0:
            return None
        return frm_list[i - 1]

class Sequence(QObject):
    sequenceChanged = pyqtSignal()
    selectionChanged = pyqtSignal()

    def __init__(self):
        QObject.__init__(self)
        self.frame = []
        self._desc = ""
        self._selection = []
        self._highlighted = None
        self._last_selected = []
        self._visible = {}

    def copy_sequence(self, seq):
        self.frame = []
        for frm in seq.frame:
            self.frame.append(frm.clone(self))
        self._desc = seq.get_description()
        self._selection = []
        self._highlighted = None
        self._last_selected = []
        self._visible = {}


    def get_description(self):
        return self._desc

    def set_description(self, desc):
        self._desc = desc

    def get_selected(self):
        return self._selection

    def set_selected(self, frames):
        self._selection = [f for f in frames if f in self.frame]

        if self._selection != self._last_selected:
            self._last_selected = self._selection
            if self._selection:
                self.set_highlighted(self._selection[0])
            else:
                self.set_highlighted(None)

    def get_highlighted(self):
        return self._highlighted

    def set_highlighted(self, frame):
        self._highlighted = frame
        self.selectionChanged.emit()

    def set_visibility(self, no, visible):
        self._visible[no] = visible
        #signal modification
        for frm in self.frame:
            frm.frameChanged.emit()
        self.sequenceChanged.emit()

    def is_visible(self, no):
        if not no in self._visible:
            self._visible[no] = True
        return self._visible[no]

    def remove_all(self):
        self.frame = []
        self.sequenceChanged.emit()

    def reverse_frames(self):
        old_times = [frm.get_time() for frm in self.frame]
        for new, old in zip(self.frame, reversed(old_times)):
            new.set_time(old)

    def change_output(self, old, new):
        for frm in self.frame:
            frm.change_output(old, new)

    def add_output(self, no, y):
        for frm in self.frame:
            frm.add_point(no, y)

    def remove_output(self, no):
        for frm in self.frame:
            frm.remove_point(no)
        self.sequenceChanged.emit()

    def add_frame(self, time=0.0, pts=None, name=None):
        frm = Frame(self, time, name)
        if len(self.frame) > 0:
            #build a list of frame with their respective distance
            #to the new one, sort it and copy the closest
            distance_list = [(abs(f.get_time() - time), f) for f in self.frame]
            distance_list.sort(key=lambda f: f[0])
            frm._point = copy.copy(distance_list[0][1].get_point_dict())
        else:
            frm._point = {}

        #merge with the points from new frame
        if pts:
            frm._point.update(pts)

            #add new frame points to other frames
            for no, val in pts.items():
                for f in self.frame:
                    f.add_point(no, val)

        self.frame.append(frm)
        self.frame.sort(key=lambda f: f.get_time())
        self.sequenceChanged.emit()

        return frm

    def remove_frame(self, frm):
        self.frame.remove(frm)
        self.sequenceChanged.emit()

    def time_updated(self):
        self.frame.sort(key=lambda f: f.get_time())
        self.sequenceChanged.emit()

    def check_update(self):
        for frm in self.frame:
            frm.check_update()

    def get_outputs(self):
        if self.frame:
            return [p[0] for p in self.frame[0].get_points()]

    def _seq_to_json(self, frames):
        values = [self.get_description(), ]
        for frm in frames:
            values.append((frm.name, frm.get_time(), frm.get_point_dict()))
        return json.dumps(values)

    def selected_to_json(self):
        return self._seq_to_json(self.get_selected())

    def all_to_json(self):
        return self._seq_to_json(self.frame)

    def import_from_json(self, text, time=None):
        try:
            t0 = None
            added_frames = []
            values = json.loads(text)

            self.set_description(values[0])
            for name, t, pts in values[1:]:
                new_pts = dict([(int(n), val) for n, val in pts.items()])

                #compute where to insert the frame
                if time is None:
                    frm_time = t
                else:
                    if t0 is None:
                        t0 = t
                    frm_time = time + t - t0
                added_frames.append(self.add_frame(frm_time, new_pts, name))

            self.set_selected(added_frames)
            return True
        except ValueError:
            return False


