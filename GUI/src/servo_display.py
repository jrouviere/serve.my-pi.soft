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
Display class for a servo.
"""

from PyQt4 import QtGui, QtCore

class ServoGraphics(QtGui.QGraphicsItem):

    def __init__(self):
        QtGui.QGraphicsItemGroup.__init__(self)
        self.pixmap = QtGui.QPixmap(r"../../../Artwork/servo.png")
        self.setZValue(1)

        self.mid_x = 0
        self.mid_y = -60

    def boundingRect(self):
        return QtCore.QRectF(-self.pixmap.width() / 2,
                             - self.pixmap.height() / 2,
                             self.pixmap.width(),
                             self.pixmap.height())

    def paint(self, painter, option, widget=None):
        if self.pixmap:
            painter.drawPixmap(self.boundingRect().topLeft(), self.pixmap)
