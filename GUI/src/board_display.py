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
Display class for the main board.
"""

from PyQt4 import QtGui, QtCore
from functools import partial

class BoardOutput(QtGui.QGraphicsItem):
    HRECT = QtCore.QRectF(-25, -8, 50, 16)
    VRECT = QtCore.QRectF(-8, -25, 16, 50)

    HRECT_TXT_L = QtCore.QRectF(-75, -8, 45, 16)
    HRECT_TXT_R = QtCore.QRectF(30, -8, 45, 16)
    VRECT_TXT_T = QtCore.QRectF(-8, -85, 40, 60)

    #position and size of each output on the picture
    OUT = {0: ((-146, -4), HRECT, HRECT_TXT_L),
           1: ((-146, -4 - 1 * 17), HRECT, HRECT_TXT_L),
           2: ((-146, -4 - 2 * 17), HRECT, HRECT_TXT_L),

           3: ((-146, -62), HRECT, HRECT_TXT_L),
           4: ((-146, -62 - 1 * 17), HRECT, HRECT_TXT_L),
           5: ((-146, -62 - 2 * 17), HRECT, HRECT_TXT_L),

           6: ((-102, -112), VRECT, VRECT_TXT_T),
           7: ((-102 + 1 * 17, -112), VRECT, VRECT_TXT_T),
           8: ((-102 + 2 * 17, -112), VRECT, VRECT_TXT_T),

           9: ((-45, -112), VRECT, VRECT_TXT_T),
           10: ((-45 + 1 * 17, -112), VRECT, VRECT_TXT_T),
           11: ((-45 + 2 * 17, -112), VRECT, VRECT_TXT_T),

           12: ((12, -112), VRECT, VRECT_TXT_T),
           13: ((12 + 1 * 17, -112), VRECT, VRECT_TXT_T),
           14: ((12 + 2 * 17, -112), VRECT, VRECT_TXT_T),

           15: ((70, -112), VRECT, VRECT_TXT_T),
           16: ((70 + 1 * 17, -112), VRECT, VRECT_TXT_T),
           17: ((70 + 2 * 17, -112), VRECT, VRECT_TXT_T),

           18: ((146, -58 - 2 * 17), HRECT, HRECT_TXT_R),
           19: ((146, -58 - 1 * 17), HRECT, HRECT_TXT_R),
           20: ((146, -58), HRECT, HRECT_TXT_R),

           21: ((146, -2 - 2 * 17), HRECT, HRECT_TXT_R),
           22: ((146, -2 - 1 * 17), HRECT, HRECT_TXT_R),
           23: ((146, -2), HRECT, HRECT_TXT_R),
           }

    def __init__(self, board_graphic, board, no):
        QtGui.QGraphicsItem.__init__(self, board_graphic)

        self.board = board
        self.no = no

        self.setFlags(self.ItemIsFocusable)

        self.setZValue(2)

        self._hovering = False
        self.setAcceptHoverEvents(True)

        self.setPos(*self.OUT[self.no][0])

        self.board.settingsUpdated.connect(self.update)
        self.board.enabledChanged.connect(self.update)

    def enable(self, on):
        self.board.set_output_enabled(self.no, on)

    def mouseDoubleClickEvent(self, event):
        if self.board.is_output_enabled(self.no):
            self.enable(False)
        else:
            self.enable(True)

    def contextMenuEvent(self, event):
        menu = QtGui.QMenu()
        if self.board.is_output_enabled(self.no):
            menu.addAction("Disable", partial(self.enable, False))
        else:
            menu.addAction("Enable", partial(self.enable, True))
        menu.exec_(event.screenPos())

    def hoverEnterEvent(self, event):
        self._hovering = True
        self.update()

    def hoverLeaveEvent(self, event):
        self._hovering = False
        self.update()

    def boundingRect(self):
        return self.OUT[self.no][1].united(self.OUT[self.no][2])

    def paint(self, painter, option, widget):
        hue = self.board.get_output_color(self.no).hueF()
        col = QtGui.QColor.fromHslF(hue, 0.3, 0.7, 0.8)
        painter.setBrush(QtGui.QBrush(col))
        if self._hovering:
            painter.setPen(QtGui.QPen(QtCore.Qt.darkGray, 1))
        elif self.isSelected():
            painter.setPen(QtGui.QPen(QtCore.Qt.black, 1))
        else:
            painter.setPen(QtGui.QPen(col, 1))


        painter.drawRect(self.OUT[self.no][1])
        out_name = self.board.get_output_name(self.no)

        text_rect = QtCore.QRectF(0, -8, 50, 16)
        if self.no <= 5:
            painter.translate(-80, 0)
            painter.drawText(text_rect, QtCore.Qt.AlignRight,
                         "%s" % out_name)
        elif self.no >= 18:
            painter.translate(30, 0)
            painter.drawText(text_rect, QtCore.Qt.AlignLeft,
                         "%s" % out_name)
        else:
            painter.translate(0, -30)
            painter.rotate(-60)
            painter.drawText(text_rect, QtCore.Qt.AlignLeft,
                         "%s" % out_name)



class BoardInput(QtGui.QGraphicsItem):
    RECT = QtCore.QRectF(-17, -45, 34, 90)
    POS = (-120, 62)
    COL = QtGui.QColor.fromHslF(0.5, 0.1, 0.6, 0.8)
    NAME = "PPM"

    def __init__(self, board_graphic):
        QtGui.QGraphicsItem.__init__(self, board_graphic)

        self.setFlags(self.ItemIsFocusable)
        self.setZValue(2)

        self._hovering = False
        self.setAcceptHoverEvents(True)

        self.setPos(*self.POS)

    def hoverEnterEvent(self, event):
        self._hovering = True
        self.update()

    def hoverLeaveEvent(self, event):
        self._hovering = False
        self.update()

    def boundingRect(self):
        return self.RECT

    def paint(self, painter, option, widget):
        cross_brush = QtGui.QBrush(self.COL, QtCore.Qt.DiagCrossPattern)
        painter.setBrush(cross_brush)
        if self._hovering:
            painter.setPen(QtGui.QPen(QtCore.Qt.darkGray, 1))
        elif self.isSelected():
            painter.setPen(QtGui.QPen(QtCore.Qt.black, 1))
        else:
            painter.setPen(QtGui.QPen(self.COL, 1))

        painter.drawRect(self.RECT)

        text_rect = QtCore.QRectF(0, -8, 50, 16)
        painter.translate(-110, 0)
        painter.drawText(text_rect, QtCore.Qt.AlignRight,
                     "%s" % self.NAME)



class BoardGraphics(QtGui.QGraphicsItem):

    def __init__(self):
        QtGui.QGraphicsItem.__init__(self)
        self.pixmap = QtGui.QPixmap(r"../../../Artwork/board.png")
        self.setZValue(0)

    def boundingRect(self):
        return QtCore.QRectF(-self.pixmap.width() / 2,
                             - self.pixmap.height() / 2,
                             self.pixmap.width(),
                             self.pixmap.height())

    def paint(self, painter, option, widget=None):
        if self.pixmap:
            painter.drawPixmap(self.boundingRect().topLeft(), self.pixmap)
