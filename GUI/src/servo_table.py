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
Model and view for servo list as a table.
"""


from PyQt4 import QtGui, QtCore
import table_utils
from functools import partial

#TODO: Try to use proxy model of an output table instead of a separate model

class ServoTable(QtCore.QAbstractTableModel):
    (ENABLED, OUTPUT) = range(2)
    _table_columns = {
        ENABLED: ("Enabled", "Globally enable/disable servo"),
        OUTPUT: ("Output Name", "Map servo to logical output number"),
        }

    def __init__(self, board):
        QtCore.QAbstractTableModel.__init__(self)
        self.board = board

        self.board.enabledChanged.connect(self.update_all)
        self.board.settingsUpdated.connect(self.update_all)

        self.setSupportedDragActions(QtCore.Qt.CopyAction | QtCore.Qt.MoveAction)

    def update_all(self):
        self.layoutChanged.emit()

    def update_from_board(self):
        self.update_all()

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole:
            if orientation == QtCore.Qt.Horizontal:
                return self._table_columns[section][0]
            else:
                #display servo 1 instead of servo 0
                return '#%d' % (section + 1)

    def flags(self, index):
        flags = QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsDragEnabled

        if index.column() in (self.ENABLED,):
            flags = flags | QtCore.Qt.ItemIsUserCheckable
        else:
            flags = flags | QtCore.Qt.ItemIsEditable
        return flags

    def data(self, index, role=QtCore.Qt.DisplayRole):
        output = index.row()
        col = index.column()
        if output < self.board.output_nb:

            if role == QtCore.Qt.BackgroundRole:
                return QtGui.QBrush(self.board.get_output_color(output))
            elif role == QtCore.Qt.ToolTipRole:
                return self._table_columns[col][1]

            if col == self.ENABLED:
                if role == QtCore.Qt.CheckStateRole:
                    if self.board.is_output_enabled(output):
                        return QtCore.Qt.Checked
                    else:
                        return QtCore.Qt.Unchecked

            elif col == self.OUTPUT:
                if role in (QtCore.Qt.DisplayRole, QtCore.Qt.EditRole):
                    return self.board.get_output_name(output)

        return None

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        servo = index.row()
        col = index.column()
        if servo < self.board.output_nb:
            if role == QtCore.Qt.CheckStateRole:
                if col == self.ENABLED:
                    enabled = (value == QtCore.Qt.Checked)
                    self.board.set_output_enabled(servo, enabled)
                    self.layoutChanged.emit()

            if role == QtCore.Qt.EditRole:
                if col == self.OUTPUT:
                    val = value.toString()
                    if val:
                        self.board.set_output_name(servo, val)

            first = self.createIndex(servo, 0)
            last = self.createIndex(servo, self.columnCount() - 1)
            self.emit(QtCore.SIGNAL("dataChanged(const QModelIndex&,const QModelIndex&)"), first, last)
        return True

    def rowCount(self, parent=None):
        return self.board.output_nb

    def columnCount(self, parent=None):
        return len(self._table_columns)

    def mimeData(self, indexes):
        data = QtCore.QMimeData()
        servos = set([str(idx.row()) for idx in indexes])
        data.setText(",".join(servos))
        return data


class ServoTableView(table_utils.TableView):
    def __init__(self, board):
        table_utils.TableView.__init__(self)
        self.board = board
        self.horizontalHeader().setStretchLastSection(True)
        self.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
        self.setDragEnabled(True)

    def mouseDoubleClickEvent(self, ev):
        row = self.indexAt(ev.pos()).row()
        if row >= 0:
            state = self.board.is_output_enabled(row)
            self.board.set_output_enabled(row, not state)

    def enable_selected(self, enable):
        rows = set([idx.row() for idx in self.selectedIndexes()])
        self.board.set_output_enabled(rows, enable)

    def contextMenuEvent(self, event):
        menu = QtGui.QMenu()
        menu.addAction("Enable selection", partial(self.enable_selected, True))
        menu.addAction("Disable selection", partial(self.enable_selected, False))
        menu.exec_(event.globalPos())

