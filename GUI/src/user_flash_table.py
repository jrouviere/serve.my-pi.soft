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
import pyopenscb


class FlashTable(QtCore.QAbstractTableModel):
    (TYPE, DESCRIPTION) = range(2)

    TYPE_NAME = {
    pyopenscb.SlotHeader.SLOT_UNINIT : "Uninitialized",
    pyopenscb.SlotHeader.SLOT_EMPTY : "Empty slot",
    pyopenscb.SlotHeader.SLOT_IO_SETTINGS : "IO settings",
    pyopenscb.SlotHeader.SLOT_OUT_SEQUENCE : "Output sequence",
    pyopenscb.SlotHeader.SLOT_IN_SEQUENCE : "Input sequence"}

    _table_columns = {
        TYPE: ("Slot type", ""),
        DESCRIPTION: ("Name", ""), }

    def __init__(self, board):
        QtCore.QAbstractTableModel.__init__(self)
        self.board = board

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole:
            if orientation == QtCore.Qt.Horizontal:
                return self._table_columns[section][0]
            else:
                return '#%d' % section

    def flags(self, index):
        flags = QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled
        return flags

    def data(self, index, role=QtCore.Qt.DisplayRole):
        slot = index.row()
        col = index.column()

        flash_slot = self.board.get_flash_slot(slot)

        if col == self.TYPE:
            if role in (QtCore.Qt.DisplayRole, QtCore.Qt.EditRole):
                try:
                    return self.TYPE_NAME[flash_slot.type]
                except KeyError:
                    return "Unknown type"
        elif col == self.DESCRIPTION:
            if role in (QtCore.Qt.DisplayRole, QtCore.Qt.EditRole):
                return flash_slot.desc


        return None

    def rowCount(self, parent=None):
        return len(self.board.get_flash_slots())

    def columnCount(self, parent=None):
        return len(self._table_columns)


class FlashTableView(QtGui.QTableView):
    def __init__(self):
        QtGui.QTableView.__init__(self)
        self.horizontalHeader().resizeSection(FlashTable.TYPE, 150)
        self.horizontalHeader().resizeSection(FlashTable.DESCRIPTION, 200)
        self.horizontalHeader().setStretchLastSection(True)

