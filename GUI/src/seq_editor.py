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
Display class for a sequence of move for a set of servo.
"""

from PyQt4 import QtGui, QtCore

import seq_graphics
import output_table

from functools import partial
import time

class SequenceEditor(QtGui.QGraphicsScene):
    UNIT_X = 200.0
    UNIT_Y = 180.0

    def __init__(self, board, sequence):
        QtGui.QGraphicsScene.__init__(self)

        self.board = board
        self.sequence = sequence

        self._no_recurse = False
        self._play_bar = None
        self._last_mouse_x = None

        self.frame_graph = [seq_graphics.FrameGraphics(self, f)
                            for f in self.sequence.frame]

        x0, y0 = self.logical_to_pixel(0, -1)
        x1, y1 = self.logical_to_pixel(10, 1)
        self.setSceneRect(QtCore.QRectF(x0 - 30, y0 - 10, x1 + 20, y1 - y0 + 40))

        self.sequence.selectionChanged.connect(self.on_sel_changed)
        self.sequence.sequenceChanged.connect(self.on_sequence_changed)
        QtCore.QObject.connect(self, QtCore.SIGNAL("selectionChanged()"),
                               self.on_diag_sel_changed)

    def play_sequence(self, looping=False):
        if self._play_bar:
            self.stop_sequence()

        if not self.sequence.frame:
            return

        self._play_loop = looping
        self._play_start_time = time.time()

        x0, y0 = self.logical_to_pixel(0, -1)
        x1, y1 = self.logical_to_pixel(0, 1)
        pen = QtGui.QPen(QtCore.Qt.red, 0, QtCore.Qt.SolidLine)
        self._play_bar = self.addLine(x0, y0, x1, y1, pen)

        self._play_next = self.sequence.frame[0].get_time()
        self._load_frame(self._play_next, self.sequence.frame[0])

        self._play_timer = QtCore.QTimer()
        self._play_timer.timeout.connect(self._play_update)
        self._play_timer.start(20)

    def stop_sequence(self):
        if self._play_bar:
            self._play_timer.timeout.disconnect(self._play_update)
            self._play_timer.stop()
            self.removeItem(self._play_bar)
            self._play_bar = None
        self.board.disable_frame()

    def _play_update(self):
        now = time.time()
        elapsed = now - self._play_start_time

        if elapsed > self._play_next:
            for frm in self.sequence.frame:
                next_time = frm.get_time()
                if elapsed < next_time:
                    duration = next_time - elapsed
                    self._load_frame(duration, frm)
                    self._play_next = next_time
                    break

        if elapsed > self.sequence.frame[-1].get_time():
            if self._play_loop:
                self._play_start_time = time.time()
                self._play_next = self.sequence.frame[0].get_time()
                self._load_frame(self._play_next, self.sequence.frame[0])
            else:
                self.stop_sequence()
        else:
            x, _ = self.logical_to_pixel(elapsed, 0)
            self._play_bar.setPos(x, 0)


    def _load_frame(self, duration, frame):
        self.board.load_frame(duration, frame.get_point_dict())

    def on_sel_changed(self):
        self._no_recurse = True
        for fg in self.frame_graph:
            if fg.frame in self.sequence.get_selected():
                fg.v_line.setSelected(True)
            else:
                fg.v_line.setSelected(False)
        self._no_recurse = False

    def on_diag_sel_changed(self):
        if self._no_recurse:
            return
        selection = []
        for fg in self.frame_graph:
            if fg.v_line in self.selectedItems():
                selection.append(fg.frame)

        self.sequence.set_selected(selection)

        #no frame selected, let's find if some points are
        if not selection:
            for fg in self.frame_graph:
                if fg.selected_points():
                    self.sequence.set_highlighted(fg.frame)
                    return

            self.sequence.set_highlighted(None)


    def on_sequence_changed(self):
        frames_in_graph = [fg.frame for fg in self.frame_graph]

        tmp = []
        for i, f in enumerate(frames_in_graph):
            if f not in self.sequence.frame:
                self.frame_graph[i].cleanup()
            else:
                tmp.append(self.frame_graph[i])
        self.frame_graph = tmp

        for f in self.sequence.frame:
            if f not in frames_in_graph:
                fg = seq_graphics.FrameGraphics(self, f)
                self.frame_graph.append(fg)

        self.update()

    def logical_to_pixel(self, x, y):
        return x * self.UNIT_X, y * self.UNIT_Y

    def pixel_to_logical(self, x, y):
        return x / self.UNIT_X, y / self.UNIT_Y

    def remove_selected_frame(self):
        for frm in self.sequence.get_selected():
            self.sequence.remove_frame(frm)


    def _get_selected_out(self):
        selection = self.selectedItems()
        if selection:
            for fg in self.frame_graph:
                sel_pts = fg.selected_points()
                if sel_pts:
                    return sel_pts[0]
        return None

    def change_output(self, prev, new):
        self.sequence.change_output(prev, new)

    def add_output(self, no, y=0.0):
        self.sequence.add_output(no, y)

    def remove_output(self, no):
        self.sequence.remove_output(no)

    def select_output(self, no):
        self.clearSelection()
        for fg in self.frame_graph:
            fg.select_point(no)
        self.update()

    def copy(self):
        clipboard = QtGui.QApplication.clipboard()
        clipboard.setText(self.sequence.selected_to_json())

    def paste(self, time):
        clipboard = QtGui.QApplication.clipboard()
        text = str(clipboard.text())
        if not self.sequence.import_from_json(text, time):
            try:
                #import failed, maybe that was copied from control tab 
                rows = text.split("\n")
                pts = {}
                for row in rows[:-1]:
                    values = row.split("\t")
                    out = values[output_table.OutputTable.NAME]
                    pts[ self.board.find_output_name(out) ] = float(values[output_table.OutputTable.GOAL])
                self.sequence.add_frame(time, pts)
            except (ValueError, IndexError):
                pass

    def mouseMoveEvent(self, ev):
        ret = QtGui.QGraphicsScene.mouseMoveEvent(self, ev)

        pos = ev.scenePos()
        self._last_mouse_x, _ = self.pixel_to_logical(pos.x(), pos.y())

        self.sequence.check_update()
        return ret

    def mouseDoubleClickEvent(self, event):
        pos = event.scenePos()

        sel_pt = self._get_selected_out()
        if sel_pt is not None:
            self.select_output(sel_pt)
        else:
            x, _ = self.pixel_to_logical(pos.x(), pos.y())
            if x < 0:
                x = 0
            self.sequence.add_frame(x)

    def keyPressEvent(self, event):
        if event.matches(QtGui.QKeySequence.Copy):
            self.copy()
        elif event.matches(QtGui.QKeySequence.Paste):
            if self._last_mouse_x:
                self.paste(self._last_mouse_x)
            else:
                self.paste(0.0)
        elif event.matches(QtGui.QKeySequence.Delete):
            self.remove_selected_frame()
        else:
            event.ignore()

    def contextMenuEvent(self, event):
        """Context menu of graphics zone, display appropriate choice,
        depending on the selection and current sequence"""
        en_outs = self.board.get_enabled_outputs()

        pos = event.scenePos()
        x, y = self.pixel_to_logical(pos.x(), pos.y())
        menu = QtGui.QMenu()

        is_frame_selected = len(self.sequence.get_selected()) > 0

        #copy/paste frames
        act_cpy = menu.addAction("Copy", self.copy, QtGui.QKeySequence.Copy)
        if not is_frame_selected:
            act_cpy.setDisabled(True)
        menu.addAction("Paste", partial(self.paste, x), QtGui.QKeySequence.Paste)

        menu.addSeparator()

        #add a frame in the sequence
        menu.addAction("Add frame", partial(self.sequence.add_frame, x))
        act_rem = menu.addAction("Remove frame", self.remove_selected_frame,
                           QtGui.QKeySequence.Delete)
        if not is_frame_selected:
            act_rem.setDisabled(True)

        menu.addSeparator()

        point_present = []
        if self.sequence.frame:
            for i in range(self.board.output_nb):
                if self.sequence.frame[0].is_point_present(i):
                    point_present.append(i)

        #add an output to the sequence
        menu_add_out = QtGui.QMenu("Add output")
        output_to_add = [i for i in en_outs if i not in point_present]
        if not self.sequence.frame or not output_to_add:
            menu_add_out.setDisabled(True)
        else:
            for i in output_to_add:
                name = self.board.get_output_name(i)
                menu_add_out.addAction("#%s" % name, partial(self.add_output, i, y))

        #menu to switch an output with another without modifying the diagram
        sel_out = self._get_selected_out()
        menu_change_out = QtGui.QMenu("Change output")
        if sel_out is None:
            menu_change_out.setDisabled(True)
        else:
            name = self.board.get_output_name(sel_out)
            for i in en_outs:
                if i not in point_present:
                    new_name = self.board.get_output_name(i)
                    menu_change_out.addAction("#%s -> #%s" % (name, new_name),
                                    partial(self.change_output, sel_out, i))

        #menu to remove or select all points from an output
        menu_rem_out = QtGui.QMenu("Remove output")
        menu_sel_out = QtGui.QMenu("Select output")
        if not point_present:
            menu_rem_out.setDisabled(True)
            menu_sel_out.setDisabled(True)
        for i in range(self.board.output_nb):
            if i in point_present:
                name = self.board.get_output_name(i)
                menu_rem_out.addAction("#%s" % name, partial(self.remove_output, i))
                menu_sel_out.addAction("#%s" % name, partial(self.select_output, i))


        menu.addMenu(menu_add_out)
        menu.addMenu(menu_rem_out)
        menu.addMenu(menu_sel_out)
        menu.addMenu(menu_change_out)


        menu.exec_(event.screenPos())


    def drawBackground(self, painter, rect):
        """Draw grid and helper lines in the background of the scene"""
        painter.setPen(QtCore.Qt.gray)

        _, top = self.logical_to_pixel(0, -1.0)
        _, top_h = self.logical_to_pixel(0, -0.5)
        _, top_q = self.logical_to_pixel(0, -0.25)
        _, middle = self.logical_to_pixel(0, 0.0)
        _, bottom_q = self.logical_to_pixel(0, 0.25)
        _, bottom_h = self.logical_to_pixel(0, 0.5)
        _, bottom = self.logical_to_pixel(0, 1.0)

        painter.setPen(QtCore.Qt.darkGray)

        # graph borders
        for y in (top, middle, bottom):
            painter.drawLine(max(0, rect.left()), y, rect.right(), y)
        px0, _ = self.logical_to_pixel(0, 1.0)
        painter.drawLine(px0, top, px0, bottom + 10)

        # 0/45/90 degrees text
        for y, ly in ((-180, top), (-90, top_h), (-45, top_q), (0, middle), (45, bottom_q), (90, bottom_h), (180, bottom)):
            painter.drawText(QtCore.QRectF(-35, ly - 7, 30, 16),
                             QtCore.Qt.AlignRight, "%d" % y)

        #time values
        for i in range(0, 10):
            px0, _ = self.logical_to_pixel(i, 1.0)
            painter.drawText(QtCore.QRectF(px0 - 20, bottom + 10, 40, 20),
                             QtCore.Qt.AlignCenter, "%d" % i)

        pen = QtGui.QPen(QtCore.Qt.lightGray, 0, QtCore.Qt.DashLine)
        painter.setPen(pen)

        # 45 / 90 degrees mark
        for y in (top_h, top_q, bottom_q, bottom_h):
            painter.drawLine(max(0, rect.left()), y, rect.right(), y)

        #vertical lines (time) 
        for i in range(1, 10):
            px0, _ = self.logical_to_pixel(i, 1.0)
            painter.drawLine(px0, top, px0, bottom + 10)

        QtGui.QGraphicsScene.drawBackground(self, painter, rect)

    def drawForeground(self, painter, rect):
        """Draw lines linking frame points for a given output"""
        # Find the first valid _point for the same output
        # for each _point in each frame, and draw a link
        frames = self.sequence.frame
        for i in range(len(frames) - 1):
            x0 = frames[i].get_time()

            for k, y0 in frames[i].get_visible_points():
                for j in range(i + 1, len(frames)):
                    x1 = frames[j].get_time()
                    if frames[j].is_point_present(k):
                        y1 = frames[j].get_point_value(k)
                        px0, py0 = self.logical_to_pixel(x0, y0)
                        px1, py1 = self.logical_to_pixel(x1, y1)
                        hue = self.board.get_output_color(k).hueF()
                        col = QtGui.QColor.fromHslF(hue, 0.7, 0.5)
                        pen = QtGui.QPen(col, 0, QtCore.Qt.DashDotLine)
                        painter.setPen(pen)
                        painter.drawLine(px0, py0, px1, py1)
                        break

        QtGui.QGraphicsScene.drawForeground(self, painter, rect)

