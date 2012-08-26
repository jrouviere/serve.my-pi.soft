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
Utils for Qt table
"""

from PyQt4 import QtGui, QtCore

from functools import partial

class ContextMenuHeaderView(QtGui.QHeaderView):
    def __init__(self, table_view):
        QtGui.QHeaderView.__init__(self, QtCore.Qt.Vertical)
        self.table_view = table_view
        self.setResizeMode(QtGui.QHeaderView.Fixed)
        self.setClickable(True)

    def contextMenuEvent(self, event):
        return self.table_view.contextMenuEvent(event)


class TableView(QtGui.QTableView):
    def __init__(self):
        QtGui.QTableView.__init__(self)
        vert_header = ContextMenuHeaderView(self)
        self.setVerticalHeader(vert_header)

    def keyPressEvent(self, ev):
        if ev.matches(QtGui.QKeySequence.Copy):
            self.copy()
        elif ev.matches(QtGui.QKeySequence.Paste):
            self.paste()
        else:
            QtGui.QTableView.keyPressEvent(self, ev)

    def copy(self):
        if self.selectedIndexes():
            rows = set([a.row() for a in self.selectedIndexes()])
            selected_text = ''

            mod = self.model()
            for row in rows:
                data = []
                for i in range(mod.columnCount()):
                    idx = mod.createIndex(row, i)
                    data.append(str(idx.data(QtCore.Qt.EditRole).toString()))
                row_text = "\t".join(data)
                selected_text += row_text + '\n'

            clipboard = QtGui.QApplication.clipboard()
            clipboard.setText(selected_text)

    def paste(self):
        #only paste first row
        clip_text = QtGui.QApplication.clipboard().text()
        row = clip_text.split("\n")[0]
        values = row.split("\t")
        for idx in self.selectedIndexes():
            try:
                idx.model().setData(idx, QtCore.QVariant(values[idx.column()]))
            except ValueError:
                pass

    def paste_col_only(self, col):
        clip_text = QtGui.QApplication.clipboard().text()
        values = clip_text.split("\t")
        for idx in self.selectedIndexes():
            try:
                if idx.column() == col:
                    idx.model().setData(idx, QtCore.QVariant(values[idx.column()]))
            except ValueError:
                pass



class SliderDelegate(QtGui.QItemDelegate):
    HALF_RANGE = 100.0  #range in degree of the slider
    HALF_RES = 360.0 #0.5 deg resolution
    TRI_W = 4 #triangle cursor half width

    def __init__(self, board, half_range=HALF_RANGE):
        QtGui.QItemDelegate.__init__(self)
        self.board = board
        self._edited_index = None

        self.half_range = float(half_range)

    def _angle_to_x(self, width, angle):
        return angle * (width / 2.0) * (180.0 / self.half_range)

    def _x_to_angle(self, width, x):
        return x / ((width / 2.0) * (180.0 / self.half_range))

    def paint(self, painter, option, index):
        painter.save()

        if (option.state & QtGui.QStyle.State_Selected) and \
            (option.state & QtGui.QStyle.State_Active):
            brush = option.palette.highlight()
            cross_brush = QtGui.QBrush(brush.color(), QtCore.Qt.DiagCrossPattern)
        elif option.state & QtGui.QStyle.State_Selected:
            brush = QtGui.QBrush(index.data(QtCore.Qt.BackgroundRole))
            cross_brush = QtGui.QBrush(option.palette.alternateBase().color(), QtCore.Qt.DiagCrossPattern)
        else:
            brush = QtGui.QBrush(index.data(QtCore.Qt.BackgroundRole))
            cross_brush = QtGui.QBrush(brush.color(), QtCore.Qt.DiagCrossPattern)

        #get servo accessible range and draw with a different shade
        range = index.data(QtCore.Qt.UserRole)
        painter.setPen(QtCore.Qt.NoPen)
        painter.setBrush(brush)
        if range and range.toList():
            min = (180.0 / self.half_range) * range.toList()[0].toFloat()[0]
            max = (180.0 / self.half_range) * range.toList()[1].toFloat()[0]

            width = option.rect.width()
            rect = QtCore.QRect(option.rect)
            rect.setLeft(rect.left() + width * (min + 1.0) / 2.0)
            rect.setRight(rect.right() + width * (max - 1.0) / 2.0)
            painter.drawRect(rect)
            painter.setBrush(cross_brush)
            painter.drawRect(QtCore.QRect(option.rect.topLeft(), rect.bottomLeft()))
            painter.drawRect(QtCore.QRect(rect.topRight(), option.rect.bottomRight()))
        else:
            painter.drawRect(option.rect)

        width = option.rect.width()
        height = option.rect.height()

        painter.translate(option.rect.x() + option.rect.width() / 2,
                          option.rect.y() + height / 2)

        #draw -45, 0, +45 degrees line
        painter.setPen(QtGui.QPen(QtCore.Qt.gray, 0, QtCore.Qt.DashLine))
        for angle in [-0.25, 0, 0.25]:
            x = self._angle_to_x(width, angle)
            painter.drawLine(x, -height / 2, x, height / 2)

        #draw a marker at current servo position
        painter.setPen(QtCore.Qt.gray)
        painter.setBrush(QtCore.Qt.gray)
        v, _ = index.data(QtCore.Qt.DisplayRole).toFloat()
        pos_offset = self._angle_to_x(width, v)
        painter.drawConvexPolygon(QtCore.QPointF(pos_offset - self.TRI_W, -height / 2),
                                   QtCore.QPointF(pos_offset + self.TRI_W, -height / 2),
                                   QtCore.QPointF(pos_offset, 0))

        #draw a marker at current servo goal
        painter.setPen(QtCore.Qt.black)
        painter.setBrush(QtCore.Qt.gray)
        v, _ = index.data(QtCore.Qt.EditRole).toFloat()
        goal_offset = self._angle_to_x(width, v)
        painter.drawConvexPolygon(QtCore.QPointF(goal_offset - self.TRI_W, height / 2),
                                   QtCore.QPointF(goal_offset + self.TRI_W, height / 2),
                                   QtCore.QPointF(goal_offset, 0))

        painter.restore()

    def createEditor(self, parent, option, index):
        return None

    def editorEvent(self, ev, model, option, index):
        move = False

        if ev.type() == ev.MouseButtonPress and ev.button() == QtCore.Qt.LeftButton:
            self._edited_index = index
            move = True

        #don't grab mouse focus if it already belong to another cell
        if ev.type() == ev.MouseMove and self._edited_index == index:
            move = True

        if move:
            #compute the new goal position by comparing
            #mouse event position and cell rectangle
            x = ev.pos().x()
            x0 = option.rect.left()
            width = option.rect.width()

            value = self._x_to_angle(width, (x - x0 - width / 2))

            #round up to resolution
            value = int(value * self.HALF_RES) / float(self.HALF_RES)
            model.setData(index, QtCore.QVariant(value), QtCore.Qt.EditRole)

        return False




class OutputMapDelegate(QtGui.QItemDelegate):
    def __init__(self, board):
        QtGui.QItemDelegate.__init__(self)
        self.board = board

    def createEditor(self, parent, option, index):
        editor = QtGui.QComboBox(parent)

        for a in self.board.get_output_names():
            editor.addItem(a)
        return editor

    def setEditorData(self, editor, index):
        value = index.model().data(index, QtCore.Qt.EditRole)
        if value:
            editor.setCurrentIndex(value)

    def setModelData(self, editor, model, index):
        value = editor.currentIndex()
        model.setData(index, QtCore.QVariant(value), QtCore.Qt.EditRole)

    def updateEditorGeometry(self, editor, option, index):
        editor.setGeometry(option.rect)

class SpeedSpinBox(QtGui.QSpinBox):
    def textFromValue(self, v):
        if v == 0:
            return "Max"
        else:
            return QtGui.QSpinBox.textFromValue(self, v)

class SpeedDelegate(QtGui.QItemDelegate):
    def createEditor(self, parent, option, index):
        editor = SpeedSpinBox(parent)
        editor.setMinimum(0)
        editor.setMaximum(255)
        editor.setSingleStep(1)
        editor.setWrapping(True)
        return editor

    def setEditorData(self, editor, index):
        value = index.model().data(index, QtCore.Qt.EditRole)
        if value:
            editor.setValue(value)

    def setModelData(self, editor, model, index):
        editor.interpretText()
        value = editor.value()
        model.setData(index, QtCore.QVariant(value), QtCore.Qt.EditRole)

    def updateEditorGeometry(self, editor, option, index):
        editor.setGeometry(option.rect)


class SpinBoxDelegate(QtGui.QItemDelegate):
    def createEditor(self, parent, option, index):
        editor = QtGui.QSpinBox(parent)
        editor.setMinimum(1)
        editor.setMaximum(10000)
        editor.setSingleStep(10)
        return editor

    def setEditorData(self, editor, index):
        value = index.model().data(index, QtCore.Qt.EditRole)
        if value:
            editor.setValue(value)

    def setModelData(self, editor, model, index):
        editor.interpretText()
        value = editor.value()
        model.setData(index, QtCore.QVariant(value), QtCore.Qt.EditRole)

    def updateEditorGeometry(self, editor, option, index):
        editor.setGeometry(option.rect)



class BtnDelegate(QtGui.QItemDelegate):
    def __init__(self, txt, on_press):
        QtGui.QItemDelegate.__init__(self)
        self.text = txt
        self.on_press = on_press

    def createEditor(self, parent, option, index):
        editor = QtGui.QPushButton(self.text, parent)
        editor.pressed.connect(partial(self.on_press, index))
        return editor

    def updateEditorGeometry(self, editor, option, index):
        editor.setGeometry(option.rect)


class ColorPickDelegate(QtGui.QItemDelegate):
    def createEditor(self, parent, option, index):
        initial = QtGui.QColor(index.data(QtCore.Qt.EditRole))
        color = QtGui.QColorDialog.getColor(initial, parent, "Pick a color")
        if color.isValid():
            index.model().setData(index, QtCore.QVariant(color))
        return None
