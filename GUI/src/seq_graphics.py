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
Display model for sequence, frames and point
"""

from PyQt4 import QtGui, QtCore


class PointGraphics(QtGui.QGraphicsItem):
    """Point representing the position of a servo in a frame"""

    CIRCLE_RECT = QtCore.QRectF(-4, -4, 8, 8)


    def __init__(self, editor, frame, no):
        QtGui.QGraphicsItem.__init__(self)

        self.editor = editor
        self.frame = frame
        self.no = no


        self.setFlags(self.ItemIsMovable |
                      self.ItemIsSelectable |
                      self.ItemIsFocusable |
                      self.ItemSendsGeometryChanges)

        self.setZValue(1)

        self._hovering = False
        self.setAcceptHoverEvents(True)

        self.update_time()

    def _value(self):
        return self.frame.get_point_value(self.no)

    def hoverEnterEvent(self, event):
        self._hovering = True
        self.update()

    def hoverLeaveEvent(self, event):
        self._hovering = False
        self.update()

    def itemChange(self, change, value):
        if change == self.ItemPositionChange:
            point = value.toPointF()
            x, y = self.editor.pixel_to_logical(point.x(), point.y())

            if y < -1: y = -1
            elif y > 1: y = 1

            x, y = self.editor.logical_to_pixel(self.frame.get_time(), y)
            point.setX(x)
            point.setY(y)

            return QtCore.QVariant(point)

        elif change == self.ItemPositionHasChanged:
            point = value.toPointF()
            _, y = self.editor.pixel_to_logical(point.x(), point.y())
            self.frame.set_point_value(self.no, y)
            self.editor.update()
        elif change == self.ItemSelectedChange:
            #only accept selection if no frame line are selected
            if value == True:
                for sel in self.scene().selectedItems():
                    if not isinstance(sel, PointGraphics):
                        return False
        elif change == self.ItemSelectedHasChanged:
            self.editor.update()

        return QtGui.QGraphicsItem.itemChange(self, change, value)

    def boundingRect(self):
        return QtCore.QRectF(-4, -4, 44, 20)

    def shape(self):
        path = QtGui.QPainterPath()
        path.addEllipse(self.CIRCLE_RECT)
        return path


    def paint(self, painter, option, widget):
        font = painter.font()
        if self._hovering:
            pen = QtGui.QPen(QtCore.Qt.red, 1)
            font.setPointSize(8)
        elif self.isSelected():
            pen = QtGui.QPen(QtCore.Qt.blue, 1)
            font.setPointSize(8)
        else:
            pen = QtGui.QPen(QtCore.Qt.gray, 1)
            font.setPointSize(8)

        painter.setPen(pen)
        painter.setFont(font)

        name = self.editor.board.get_output_name(self.no)
        painter.drawEllipse(self.CIRCLE_RECT)
        painter.drawText(QtCore.QRectF(3, 0, 40, 16),
                             QtCore.Qt.AlignLeft, name)

    def update_time(self):
        x, y = self.editor.logical_to_pixel(self.frame.get_time(), self._value())
        self.setPos(x, y)


class FrameVLineGraphics(QtGui.QGraphicsItem):
    """Vertical line marking a frame on the graph"""
    def __init__(self, frame, editor):
        QtGui.QGraphicsItem.__init__(self)

        self.frame = frame
        self.editor = editor

        x, y = self.editor.logical_to_pixel(self.frame.get_time(), 0.0)
        self.setPos(x, y)

        self.setFlags(self.ItemIsMovable |
                      self.ItemIsSelectable |
                      self.ItemIsFocusable |
                      self.ItemSendsGeometryChanges)

        self.setZValue(0)

        self._hovering = False
        self.setAcceptHoverEvents(True)

        self.frame.frameChanged.connect(self.update_time)

    def hoverEnterEvent(self, event):
        self._hovering = True
        self.update()

    def hoverLeaveEvent(self, event):
        self._hovering = False
        self.update()

    def itemChange(self, change, value):
        if change == self.ItemPositionChange:
            point = value.toPointF()
            time, _ = self.editor.pixel_to_logical(point.x(), point.y())

            if time < 0:
                time = 0.0

            time, y = self.editor.logical_to_pixel(time, 0.0)
            point.setX(time)
            point.setY(y)

            return QtCore.QVariant(point)

        elif change == self.ItemPositionHasChanged:
            self.editor.update()
            time, _ = self.editor.pixel_to_logical(self.x(), 0)
            self.frame.set_time(time)
        elif change == self.ItemSelectedChange:
            #only accept selection if no points are selected
            if value == True:
                for sel in self.scene().selectedItems():
                    if not isinstance(sel, FrameVLineGraphics):
                        return False

        elif change == self.ItemSelectedHasChanged:
            self.editor.update()

        return QtGui.QGraphicsItem.itemChange(self, change, value)

    def _get_coords(self):
        x0, y0 = self.editor.logical_to_pixel(0.0, -1.0)
        x1, y1 = self.editor.logical_to_pixel(0.0, 1.0)
        return x0, y0, x1, y1

    def boundingRect(self):
        _, y0, _, y1 = self._get_coords()
        return QtCore.QRectF(-20, y0, 40, y1 - y0 + 20)

    def shape(self):
        path = QtGui.QPainterPath()
        _, y0, _, y1 = self._get_coords()
        path.addRect(-3, y0, 6, y1 - y0)
        return path

    def paint(self, painter, option, widget):
        if self._hovering:
            painter.setPen(QtGui.QPen(QtCore.Qt.red, 0))
        elif self.isSelected():
            painter.setPen(QtGui.QPen(QtCore.Qt.blue, 0))
        else:
            painter.setPen(QtGui.QPen(QtCore.Qt.black, 0))

        font = painter.font()
        font.setPointSize(8)
        painter.setFont(font)

        x0, y0, x1, y1 = self._get_coords()
        painter.drawLine(x0, y0, x1, y1)

        painter.drawText(QtCore.QRectF(-30, y1, 60, 20), QtCore.Qt.AlignCenter,
                         self.frame.name)

    def update_time(self):
        x, y = self.editor.logical_to_pixel(self.frame.get_time(), 0.0)
        self.setPos(x, y)


class FrameGraphics():
    """A frame define a static position for all servos. The animation
    is then defined by a sequence of frames."""
    def __init__(self, scene, frame):
        self.scene = scene
        self.frame = frame
        self._point = {}

        self.v_line = FrameVLineGraphics(self.frame, self.scene)
        self.scene.addItem(self.v_line)
        self.update_points()

        self.frame.frameChanged.connect(self.update_points)
        self.frame.frameChanged.connect(self.update_time)

    def cleanup(self):
        self.scene.removeItem(self.v_line)
        for _, pt in self._point.items():
            self.scene.removeItem(pt)

    def update_points(self):
        for k, pt in self._point.items():
            if not self.frame.is_point_present(k):
                self.scene.removeItem(pt)
                del self._point[k]

        for k, pt in self.frame.get_points():
            if k not in self._point:
                point = PointGraphics(self.scene, self.frame, k)
                self.scene.addItem(point)
                self._point[k] = point

        for k, pt in self._point.items():
            pt.setVisible(self.frame.is_visible(k))

    def select_point(self, no):
        self._point[no].setSelected(True)

    def selected_points(self):
        res = []
        for k, pt in self._point.items():
            if pt.isSelected():
                res.append(k)
        return res

    def update_time(self):
        for _, pt in self._point.items():
            pt.update_time()
        self.v_line.update_time()
