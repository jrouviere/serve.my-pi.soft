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
Helper class for main window tab
"""

from PyQt4 import QtGui

class MainTabWidget(QtGui.QWidget):

    def __init__(self, tab, board):
        QtGui.QWidget.__init__(self)

        self.tab = tab
        self.board = board

        self.menu = None

        self.board.boardConnected.connect(self._on_connect)
        self.board.boardDisconnected.connect(self._on_disconnect)

    def focus_tab(self):
        self.tab.setCurrentWidget(self)

    def _on_connect(self):
        pass
    def _on_disconnect(self):
        pass

    def on_tab_exit(self):
        pass
    def on_tab_enter(self):
        pass

