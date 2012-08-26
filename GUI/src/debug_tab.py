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
Model and view to display board user flash.
"""


from PyQt4 import QtGui, QtCore
import main_win_tab
import user_flash_table
import pyopenscb

class DebugLogTab(main_win_tab.MainTabWidget):
    def __init__(self, tab, board):
        main_win_tab.MainTabWidget.__init__(self, tab, board)

        lay = QtGui.QVBoxLayout()

        lay.addWidget(QtGui.QLabel("Flash overview:"))
        self.flash_table = user_flash_table.FlashTable(self.board)
        self.flash_table_view = user_flash_table.FlashTableView()
        lay.addWidget(self.flash_table_view)

        lay.addWidget(QtGui.QLabel("Log console:"))
        self.log_console = QtGui.QPlainTextEdit()
        self.log_console.setReadOnly(True)
        lay.addWidget(self.log_console)

        self.setLayout(lay)

    def _on_connect(self):
        self.flash_table_view.setModel(self.flash_table)
        self.flash_table_view.resizeColumnsToContents()

        #Using a timer is a best-effort solution to overcome the fact
        #that libusb 0.1 is limited to single threaded application and
        #synchronous call. As a result the interface might be a bit
        #laggy when too many log messages are received.
        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self._update_log)
        self.timer.start(40)

    def _update_log(self):
        try:
            while True:
                log_txt = self.board.get_debug_message(10)
                if log_txt:
                    self.log_console.moveCursor(QtGui.QTextCursor.End)
                    self.log_console.insertPlainText(log_txt)
                else:
                    return
        except pyopenscb.ConnectionProblem:
            pass
