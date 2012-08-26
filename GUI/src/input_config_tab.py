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
Calibration dialog for inputs.
"""

from PyQt4 import QtGui, QtCore
import pyopenscb

import table_utils
import main_win_tab


class InputCalibTable(QtCore.QAbstractTableModel):
    (MINIMUM, CENTER, MAXIMUM, VALUE) = range(4)
    _table_columns = {
        MINIMUM: ("Minimum",),
        CENTER: ("Center",),
        MAXIMUM: ("Maximum",),
        VALUE: ("Current value",), }

    def __init__(self, board):
        QtCore.QAbstractTableModel.__init__(self)
        self.board = board

        self.board.inputUpdated.connect(self.update_input)

    def update_input(self):
        first = self.createIndex(0, self.VALUE)
        last = self.createIndex(self.board.input_nb - 1, self.VALUE)
        self.emit(QtCore.SIGNAL("dataChanged(const QModelIndex&,const QModelIndex&)"), first, last)

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole:
            if orientation == QtCore.Qt.Horizontal:
                return self._table_columns[section][0]
            else:
                #display servo 1 instead of servo 0
                return '%d' % (section + 1)

    def flags(self, index):
        return  QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled

    def data(self, index, role=QtCore.Qt.DisplayRole):
        input = index.row()
        col = index.column()
        if input < self.board.input_nb:
            if role in (QtCore.Qt.DisplayRole, QtCore.Qt.EditRole):
                calib = self.board.get_input_calib(input)
                if col == self.MINIMUM:
                    return calib.min
                elif col == self.CENTER:
                    return calib.mid
                elif col == self.MAXIMUM:
                    return calib.max
                elif col == self.VALUE:
                    return self.board.get_input_value(input)

        return None

    def rowCount(self, parent=None):
        return self.board.input_nb

    def columnCount(self, parent=None):
        return len(self._table_columns)

class InputCalibTab(main_win_tab.MainTabWidget):

    def __init__(self, tab, board):
        main_win_tab.MainTabWidget.__init__(self, tab, board)

        lay = QtGui.QHBoxLayout()

        self.in_table = InputCalibTable(board)
        self.in_table_view = table_utils.TableView()
        self._sld_del = table_utils.SliderDelegate(self.board, 180.0)
        self.in_table_view.setItemDelegateForColumn(InputCalibTable.VALUE, self._sld_del)

        lay.addWidget(self.in_table_view, stretch=1)

        self.reset_btn = QtGui.QPushButton("Reset all")
        self.reset_btn.clicked.connect(self.on_reset)
        lay.addWidget(self.reset_btn)

        self.save_mid_btn = QtGui.QPushButton("Set center values")
        self.save_mid_btn.clicked.connect(self.on_save_middle)
        lay.addWidget(self.save_mid_btn)

        self.setLayout(lay)

        self.board.settingsUpdated.connect(self._on_settings_updated)


    def on_reset(self):
        self.board.set_mode(pyopenscb.MODE_INPUT_CALIB)

    def on_save_middle(self):
        self.board.input_calib_set_center()


    def _on_settings_updated(self):
        self.in_table.layoutChanged.emit()

    def _on_connect(self):
        self.in_table_view.setModel(self.in_table)

    def on_tab_exit(self):
        self.board.set_mode(pyopenscb.MODE_NORMAL)

    def on_tab_enter(self):
        pass

