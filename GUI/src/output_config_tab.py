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
Tab for servo calibration.
"""

from PyQt4 import QtGui, QtCore
import pyopenscb

import table_utils
import servo_calib
import main_win_tab

from os import path
from functools import partial

class ServoCalibTab(main_win_tab.MainTabWidget):

    def __init__(self, tab, board):
        main_win_tab.MainTabWidget.__init__(self, tab, board)

        lay = QtGui.QHBoxLayout()

        self.sv_table = servo_calib.ServoCalibTable(board)

        #Left widget is a drawing of a servo with value that need to be calibrated
        #plus a drawing of the board showing which servo is being modified 
        self.servo_scene = servo_calib.ServoDisplayScene(board, self.sv_table)
        self.servo_view = QtGui.QGraphicsView(self.servo_scene)
        self.servo_view.setMinimumWidth(350)

        lay.addWidget(self.servo_view)

        #right widget is a table with calibration values for each servo
        self.sv_table_view = servo_calib.CalibTableView(board)
        self.sv_table_view.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
        self.sv_table_view.setSelectionMode(QtGui.QAbstractItemView.SingleSelection)
        self.sv_table_view.verticalHeader().setResizeMode(QtGui.QHeaderView.Fixed)
        self._sp_del = table_utils.SpinBoxDelegate()
        self.sv_table_view.setItemDelegateForColumn(self.sv_table.MINIMUM, self._sp_del)
        self.sv_table_view.setItemDelegateForColumn(self.sv_table.CENTER, self._sp_del)
        self.sv_table_view.setItemDelegateForColumn(self.sv_table.MAXIMUM, self._sp_del)


        lay.addWidget(self.sv_table_view, stretch=1)
        self.setLayout(lay)

        board.settingsUpdated.connect(self._on_settings_updated)


    def _on_settings_updated(self):
        self.sv_table.layoutChanged.emit()
        self.servo_view.update()

    def _on_connect(self):
        self.sv_table_view.setModel(self.sv_table)
        sel_model = self.sv_table_view.selectionModel()
        QtCore.QObject.connect(sel_model,
            QtCore.SIGNAL("selectionChanged(const QItemSelection&,const QItemSelection&)"),
            self.servo_scene.on_table_row_changed)
        QtCore.QObject.connect(self.sv_table,
            QtCore.SIGNAL("dataChanged(const QModelIndex&,const QModelIndex&)"),
            self.servo_scene.on_table_row_updated)
        self.sv_table_view.resizeColumnToContents(self.sv_table.REVERT)
        self.sv_table_view.selectRow(0)

    def on_tab_exit(self):
        self.board.send_all_calib_values()
        self.board.set_mode(pyopenscb.MODE_NORMAL)
        self.board.disable_calib_raw()

    def on_tab_enter(self):
        self.board.disable_calib_raw()
        self.board.set_mode(pyopenscb.MODE_OUTPUT_CALIB)
