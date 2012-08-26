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
Calibration dialog.
"""

from PyQt4 import QtGui, QtCore
import pyopenscb

import table_utils
import servo_display
import board_display



LABEL_CALIB_ANG = """
The angular resolution of the servo is used by the board to determine the exact angle made by the servo.
The calibration of this value can be difficult, that's why you can use one of the three methods bellow.
They should all give you a similar result, but one might be more convenient than another depending on
your setting.

"""

LABEL_CALIB_180 = """Calibrate a 180 degrees angle.
Move 'left' point and 'right' point until the angle between
left and right is 180 degrees.
"""

LABEL_CALIB_90 = """Calibrate a 90 degrees angle.
Move 'left' point and 'right' point until the angle between
left and right is 90 degrees.
"""

LABEL_CALIB_MINMAX = """Max angle measurement.
Use a protractor or a printed pattern to measure the angle
between current minimum and maximum movement.
"""


class ServoCalibTable(QtCore.QAbstractTableModel):
    (REVERT, MINIMUM, CENTER, MAXIMUM, ANGLE180) = range(5)
    _table_columns = {
        REVERT: ("Rev.",),
        MINIMUM: ("Minimum",),
        CENTER: ("Center",),
        MAXIMUM: ("Maximum",),
        ANGLE180: ("Ang. res.",), }

    def __init__(self, board):
        QtCore.QAbstractTableModel.__init__(self)
        self.board = board

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole:
            if orientation == QtCore.Qt.Horizontal:
                return self._table_columns[section][0]
            else:
                #display servo 1 instead of servo 0
                return '%d: %s' % (section + 1, self.board.get_output_name(section))

    def flags(self, index):
        col = index.column()
        if col == self.REVERT:
            return QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled | \
            QtCore.Qt.ItemIsUserCheckable
        else:
            return QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsSelectable | \
            QtCore.Qt.ItemIsEnabled

    def data(self, index, role=QtCore.Qt.DisplayRole):
        output = index.row()
        col = index.column()
        if output < self.board.output_nb:
            if role == QtCore.Qt.BackgroundRole:
                return QtGui.QBrush(self.board.get_output_color(output))

            if col == self.REVERT:
                if role == QtCore.Qt.CheckStateRole:
                    if self.board.get_calib_angle180(output) < 0:
                        return QtCore.Qt.Checked
                    else:
                        return QtCore.Qt.Unchecked

            if role in (QtCore.Qt.DisplayRole, QtCore.Qt.EditRole):
                if col == self.MINIMUM:
                    return self.board.get_calib_min(output)
                elif col == self.CENTER:
                    return self.board.get_calib_center(output)
                elif col == self.MAXIMUM:
                    return self.board.get_calib_max(output)
                elif col == self.ANGLE180:
                    angle180 = self.board.get_calib_angle180(output)
                    return abs(angle180)

        return None

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        servo = index.row()
        col = index.column()
        if servo < self.board.output_nb:

            if col == self.REVERT:
                if role == QtCore.Qt.CheckStateRole:
                    val = value.toBool()
                    angle180 = self.board.get_calib_angle180(servo)
                    self.board.set_calib_angle180(servo, -angle180)

            if role == QtCore.Qt.EditRole:
                val, ok = value.toInt()
                if not ok:
                    return False
                if val < 1: val = 1
                elif val > 6000: val = 6000

                if col == self.MINIMUM:
                    self.board.set_calib_min(servo, val)
                    self.board.set_calib_raw(servo, val)
                elif col == self.CENTER:
                    self.board.set_calib_center(servo, val)
                    self.board.set_calib_raw(servo, val)
                elif col == self.MAXIMUM:
                    self.board.set_calib_max(servo, val)
                    self.board.set_calib_raw(servo, val)
                elif col == self.ANGLE180:
                    angle180 = self.board.get_calib_angle180(servo)
                    if angle180 < 0:
                        val = -val
                    self.board.set_calib_angle180(servo, val)

                first = self.createIndex(servo, 0)
                last = self.createIndex(servo, self.columnCount() - 1)
                self.emit(QtCore.SIGNAL("dataChanged(const QModelIndex&,const QModelIndex&)"), first, last)
        return True

    def rowCount(self, parent=None):
        return self.board.output_nb

    def columnCount(self, parent=None):
        return len(self._table_columns)

class CalibAngleDialog(QtGui.QDialog):
    def __init__(self, parent, board, index):
        QtGui.QDialog.__init__(self, parent)

        self.board = board
        self.index = index
        self.servo = self.index.row()
        self.setWindowTitle("Servo %d: Angular resolution" % \
                            (self.servo + 1))

        lay = QtGui.QGridLayout()

        row = 0
        lbl = QtGui.QLabel(LABEL_CALIB_ANG)
        lay.addWidget(lbl, row, 1, 1, 3)

        row += 1

        lbl = QtGui.QLabel(LABEL_CALIB_180)
        lay.addWidget(lbl, row, 1)
        form = QtGui.QFormLayout()
        self.spn_left_180 = QtGui.QSpinBox()
        form.addRow("&Left:", self.spn_left_180)
        self.spn_right_180 = QtGui.QSpinBox()
        form.addRow("&Right:", self.spn_right_180)
        lay.addLayout(form, row, 2)
        self.btn_go_180 = QtGui.QPushButton("Ok")
        lay.addWidget(self.btn_go_180, row, 3)

        row += 1

        lbl = QtGui.QLabel(LABEL_CALIB_90)
        lay.addWidget(lbl, row, 1)
        form = QtGui.QFormLayout()
        self.spn_left_90 = QtGui.QSpinBox()
        form.addRow("&Left:", self.spn_left_90)
        self.spn_right_90 = QtGui.QSpinBox()
        form.addRow("&Right:", self.spn_right_90)
        lay.addLayout(form, row, 2)
        self.btn_go_90 = QtGui.QPushButton("Ok")
        lay.addWidget(self.btn_go_90, row, 3)

        row += 1

        lbl = QtGui.QLabel(LABEL_CALIB_MINMAX)
        lay.addWidget(lbl, row, 1)
        form = QtGui.QFormLayout()
        self.spn_minmax = QtGui.QSpinBox()
        form.addRow("&Angle (deg):", self.spn_minmax)
        self.btn_goto_min = QtGui.QPushButton("Goto min")
        self.btn_goto_max = QtGui.QPushButton("Goto max")
        form.addRow(self.btn_goto_min, self.btn_goto_max)
        lay.addLayout(form, row, 2)
        self.btn_go_minmax = QtGui.QPushButton("Ok")
        lay.addWidget(self.btn_go_minmax, row, 3)

        self.setLayout(lay)

        for spn in (self.spn_left_180, self.spn_right_180,
                    self.spn_left_90, self.spn_right_90):
            spn.setMinimum(100)
            spn.setMaximum(10000)
            spn.setSingleStep(10)
        self.spn_minmax.setMinimum(0)
        self.spn_minmax.setMaximum(360)
        self.spn_minmax.setSingleStep(1)
        self.init_values()

        for spn in (self.spn_left_180, self.spn_right_180,
                    self.spn_left_90, self.spn_right_90):
            spn.valueChanged.connect(self.on_val_changed)

        self.btn_goto_min.pressed.connect(self.on_goto_min)
        self.btn_goto_max.pressed.connect(self.on_goto_max)

        self.btn_go_90.pressed.connect(self.on_ok_90)
        self.btn_go_180.pressed.connect(self.on_ok_180)
        self.btn_go_minmax.pressed.connect(self.on_ok_minmax)

    def on_val_changed(self, val):
        self.board.set_calib_raw(self.servo, val)

    def on_ok_90(self):
        left = self.spn_left_90.value()
        right = self.spn_right_90.value()
        self.set_value_and_close(2 * abs(right - left))

    def on_ok_180(self):
        left = self.spn_left_180.value()
        right = self.spn_right_180.value()
        self.set_value_and_close(abs(right - left))

    def on_ok_minmax(self):
        min = self.board.get_calib_min(self.servo)
        max = self.board.get_calib_max(self.servo)
        angle = self.spn_minmax.value()
        self.set_value_and_close(180 * abs(max - min) / float(angle))

    def set_value_and_close(self, value):
        idx = self.index
        new_val = QtCore.QVariant(int(value))
        idx.model().setData(idx, new_val)
        self.accept()


    def on_goto_min(self):
        min = self.board.get_calib_min(self.servo)
        self.board.set_calib_raw(self.servo, min)

    def on_goto_max(self):
        max = self.board.get_calib_max(self.servo)
        self.board.set_calib_raw(self.servo, max)

    def init_values(self):
        center = self.board.get_calib_center(self.servo)
        angle180 = abs(self.board.get_calib_angle180(self.servo))
        min = self.board.get_calib_min(self.servo)
        max = self.board.get_calib_max(self.servo)

        self.spn_left_180.setValue(center - angle180 / 2)
        self.spn_right_180.setValue(center + angle180 / 2)

        self.spn_left_90.setValue(center - angle180 / 4)
        self.spn_right_90.setValue(center + angle180 / 4)

        self.spn_minmax.setValue(180 * (max - min) / angle180)




class CalibTableView(table_utils.TableView):
    def __init__(self, board):
        table_utils.TableView.__init__(self)
        self.board = board

    def mousePressEvent(self, ev):
        if ev.button() == QtCore.Qt.LeftButton:
            idx = self.indexAt(ev.pos())
            if idx.column() in (ServoCalibTable.MINIMUM,
                                ServoCalibTable.CENTER,
                                ServoCalibTable.MAXIMUM):
                self._pressed_idx = idx
                self._press_pos = ev.pos()
                self._curs_pos = QtGui.QCursor.pos()
                QtGui.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.BlankCursor))
                ev.accept()
            else:
                self._pressed_idx = None

        return QtGui.QTableView.mousePressEvent(self, ev)

    def mouseReleaseEvent(self, ev):
        if ev.button() == QtCore.Qt.LeftButton:
            if self._pressed_idx:
                QtGui.QApplication.restoreOverrideCursor()
            self._pressed_idx = None
        return QtGui.QTableView.mouseReleaseEvent(self, ev)

    def mouseMoveEvent(self, ev):
        try:
            idx = self._pressed_idx
            if idx:
                curval, _ = idx.data().toInt()
                diff = (ev.pos().x() - self._press_pos.x()) / 2
                new_val = QtCore.QVariant(int(diff + curval))
                idx.model().setData(idx, new_val)

                QtGui.QCursor.setPos(self._curs_pos)
        except AttributeError:
            pass

    def wheelEvent(self, ev):
        try:
            idx = self.indexAt(ev.pos())
            if idx:
                curval, _ = idx.data().toInt()
                if ev.delta() > 0:
                    delta = 10
                else:
                    delta = -10
                new_val = QtCore.QVariant(int(delta + curval))
                idx.model().setData(idx, new_val)

        except AttributeError:
            pass

    def mouseDoubleClickEvent(self, ev):
        idx = self.indexAt(ev.pos())
        if idx.column() == ServoCalibTable.ANGLE180:
            dial = CalibAngleDialog(self, self.board, idx)
            dial.exec_()
        else:
            QtGui.QTableView.mouseDoubleClickEvent(self, ev)

    def keyPressEvent(self, ev):
        if ev.matches(QtGui.QKeySequence.Copy):
            self.copy()
        elif ev.matches(QtGui.QKeySequence.Paste):
            self.paste()
        else:
            QtGui.QTableView.keyPressEvent(self, ev)

    def contextMenuEvent(self, ev):
        menu = QtGui.QMenu()
        menu.addAction("Copy", self.copy, QtGui.QKeySequence.Copy)
        menu.addAction("Paste", self.paste, QtGui.QKeySequence.Paste)
        menu.exec_(ev.globalPos())

    def copy(self):
        data = []
        for idx in self.selectedIndexes():
            data.append(str(idx.data(QtCore.Qt.EditRole).toString()))
        selected_text = "\t".join(data)

        clipboard = QtGui.QApplication.clipboard()
        clipboard.setText(selected_text)

    def paste(self):
        clipboard = QtGui.QApplication.clipboard()
        clip_text = clipboard.text()
        for idx, val in zip(self.selectedIndexes(), clip_text.split("\t")):
            try:
                idx.model().setData(idx, QtCore.QVariant(val))
            except ValueError:
                pass


class CalibText(QtGui.QGraphicsTextItem):
    def __init__(self, board, text, index, pos_x, pos_y, font):
        QtGui.QGraphicsTextItem.__init__(self, text)
        self.setZValue(1)
        self.board = board
        self._text = text
        self._pos = (pos_x, pos_y)
        self._index = index

        self._value = 0
        self._updated_value = 0

        self.setPos(pos_x, pos_y)
        self.setFont(font)
        self.setCursor(QtCore.Qt.SizeHorCursor)

        self.setFlags(self.ItemIsMovable |
                      self.ItemIsSelectable |
                      self.ItemIsFocusable |
                      self.ItemSendsGeometryChanges)
        self.update_value()

    def itemChange(self, change, value):
        if change == self.ItemPositionChange:
            point = value.toPointF()
            delta = (point.x() - self._pos[0], point.y() - self._pos[1])

            self._updated_value = self._press_value + delta[0] / 2
            new_val = QtCore.QVariant(int(self._updated_value))
            self._index.model().setData(self._index, new_val)

            #force fixed object position
            point.setX(self._pos[0])
            point.setY(self._pos[1])
            return QtCore.QVariant(point)

        elif change == self.ItemSelectedHasChanged:
            self.update_value()
            if self.isSelected():
                self.setDefaultTextColor(QtCore.Qt.red)
            else:
                self.setDefaultTextColor(QtCore.Qt.black)

        return QtGui.QGraphicsItem.itemChange(self, change, value)

    def mousePressEvent(self, event):
        self._press_value, _ = self._index.data().toInt()
        QtGui.QGraphicsTextItem.mousePressEvent(self, event)

    def mouseReleaseEvent(self, event):
        if self._updated_value:
            new_val = QtCore.QVariant(int(self._updated_value))
            self._index.model().setData(self._index, new_val)
        QtGui.QGraphicsTextItem.mouseReleaseEvent(self, event)

    def update_value(self):
        self._value, ok = self._index.data().toInt()
        if ok:
            self.setPlainText(self._text + "\n(%d)" % self._value)

class ServoDisplayScene(QtGui.QGraphicsScene):
    def __init__(self, board, model):
        QtGui.QGraphicsScene.__init__(self)
        self.board = board
        self.table_model = model
        self.servo = servo_display.ServoGraphics()
        self.calib_txt = []
        self.sel_servo = 0

        self.board_graphics = board_display.BoardGraphics()
        self.board_outputs = []
        for i in range(self.board.MAX_SERVO_NB):
            board_out = board_display.BoardOutput(self.board_graphics, self.board, i)
            board_out.setVisible(False)
            self.board_outputs.append(board_out)

        self.addItem(self.servo)
        self.addItem(self.board_graphics)

        self.servo.setPos(0, -60)
        self.board_graphics.setPos(15, 250)

    def on_table_row_changed(self, new, prev):
        if len(new.indexes()) > 0:
            new_servo = new.indexes()[0]
            self.selected_servo_changed(new_servo.row())

    def on_table_row_updated(self, first, last):
        for txt in self.calib_txt:
            txt.update_value()
        self.update()

    def selected_servo_changed(self, new):
        self.board_outputs[self.sel_servo].setVisible(False)

        for txt in self.calib_txt:
            self.removeItem(txt)
        self.calib_txt = []

        self.create_servo_text(new)

        self.sel_servo = new
        self.board_outputs[new].setVisible(True)

    def create_servo_text(self, servo_no):
        font = self.font()
        font.setPointSize(12)
        mid_x = self.servo.mid_x
        mid_y = self.servo.mid_y
        for col, txt, dx, dy in [(ServoCalibTable.MINIMUM, "Min", -140, +20),
                             (ServoCalibTable.CENTER, "Center", -30, -140),
                             (ServoCalibTable.MAXIMUM, "Max", +70, +20),
                             (ServoCalibTable.ANGLE180, "Angle 180", +60, -80)]:
            idx = self.table_model.createIndex(servo_no, col)
            txt = CalibText(self.board, txt, idx, mid_x + dx, mid_y + dy, font)
            self.addItem(txt)
            self.calib_txt.append(txt)

    def drawForeground(self, painter, rect):
        mid_x = self.servo.mid_x
        mid_y = self.servo.mid_y

        pen = QtGui.QPen(QtCore.Qt.gray, 0, QtCore.Qt.DashLine)
        painter.setPen(pen)
        painter.drawLine(mid_x, mid_y, mid_x - 100, mid_y + 20)
        painter.drawLine(mid_x, mid_y, mid_x + 100, mid_y + 20)
        painter.drawLine(mid_x, mid_y, mid_x, mid_y - 100)
        painter.drawLine(mid_x - 100, mid_y, mid_x + 100, mid_y)
        painter.drawArc(QtCore.QRectF(mid_x - 80, mid_y - 80, 160, 160),
                        0 * 16, 180 * 16)

