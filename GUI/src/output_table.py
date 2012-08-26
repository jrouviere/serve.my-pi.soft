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
Model and view for output list as a table.
"""


from PyQt4 import QtGui, QtCore
import table_utils
from functools import partial

class OutputTable(QtCore.QAbstractTableModel):
    (ENABLED, COLOR, NAME, SPEED, GOAL_NUM, GOAL) = range(6)
    _table_columns = {
        ENABLED: ("Disable", "Globally enable/disable servo"),
        COLOR: ("Color", "Display color"),
        NAME: ("Name", "Output name"),
        SPEED: ("Speed", "Set maximum speed reachable by the output"),
        GOAL: ("Goal", "Set goal position of the output"),
        GOAL_NUM: ("Goal value", "Goal position of output")}

    def __init__(self, board):
        QtCore.QAbstractTableModel.__init__(self)
        self.board = board

        self.board.enabledChanged.connect(self.update_all)
        self.board.settingsUpdated.connect(self.update_all)
        self.board.positionUpdated.connect(self.update_position)

    def update_all(self):
        self.layoutChanged.emit()

    def update_position(self):
        first = self.createIndex(0, self.GOAL)
        last = self.createIndex(self.board.output_nb - 1, self.GOAL_NUM)
        self.emit(QtCore.SIGNAL("dataChanged(const QModelIndex&,const QModelIndex&)"), first, last)

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole:
            if orientation == QtCore.Qt.Horizontal:
                return self._table_columns[section][0]
            else:
                #display output 1 instead of output 0
                try:
                    output = self.board.get_enabled_outputs()[section]
                except IndexError:
                    return ''
                return '%d ' % (output + 1)

    def flags(self, index):
        flags = QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsDropEnabled
        flags = flags | QtCore.Qt.ItemIsEditable
        return flags

    def data(self, index, role=QtCore.Qt.DisplayRole):
        try:
            output = self.board.get_enabled_outputs()[index.row()]
        except IndexError:
            return None
        col = index.column()
        if output < self.board.output_nb:
            if role == QtCore.Qt.BackgroundRole:
                return QtGui.QBrush(self.board.get_output_color(output))
            elif role == QtCore.Qt.ToolTipRole:
                return self._table_columns[col][1]

            if col == self.COLOR:
                if role == QtCore.Qt.EditRole:
                    return self.board.get_output_color(output)
                elif role == QtCore.Qt.DisplayRole:
                    return ""

            elif col == self.GOAL:
                if role == QtCore.Qt.EditRole:
                    return self.board.get_output_goal(output)
                elif role == QtCore.Qt.DisplayRole:
                    return self.board.get_output_value(output)
                elif role == QtCore.Qt.UserRole:
                    left, right = self.board.get_output_range(output)
                    return QtCore.QVariant.fromList([left, right])

            elif col == self.GOAL_NUM:
                if role == QtCore.Qt.DisplayRole:
                    return u"% 4.1f\xb0" % (180 * self.board.get_output_goal(output))
                elif role == QtCore.Qt.EditRole:
                    return 180 * self.board.get_output_goal(output)

            elif col == self.NAME:
                if role in (QtCore.Qt.EditRole, QtCore.Qt.DisplayRole):
                    return self.board.get_output_name(output)

            elif col == self.SPEED:
                if role in (QtCore.Qt.DisplayRole, QtCore.Qt.EditRole):
                    speed = self.board.get_output_speed(output)
                    if role == QtCore.Qt.DisplayRole and speed == 0:
                        return "Max"
                    else:
                        return speed

        return None

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        try:
            output = self.board.get_enabled_outputs()[index.row()]
        except IndexError:
            return False
        col = index.column()
        if output < self.board.output_nb:

            if role == QtCore.Qt.EditRole:

                if col == self.COLOR:
                    color = QtGui.QColor(value)
                    self.board.set_output_color(output, color)

                elif col == self.GOAL:
                    goal, ok = value.toFloat()
                    if ok:
                        self.board.set_output_goal(output, goal)
                        self.board.set_out_api_controlled(output, True)


                elif col == self.GOAL_NUM:
                    goal, ok = value.toFloat()
                    if ok:
                        self.board.set_output_goal(output, goal / 180.0)

                elif col == self.NAME:
                    val = value.toString()
                    if val:
                        self.board.set_output_name(output, val)

                elif col == self.SPEED:
                    speed, ok = value.toInt()
                    if ok:
                        self.board.set_output_speed(output, speed)

            first = self.createIndex(output, 0)
            last = self.createIndex(output, self.columnCount() - 1)
            self.emit(QtCore.SIGNAL("dataChanged(const QModelIndex&,const QModelIndex&)"), first, last)

        return True

    def rowCount(self, parent=None):
        return len(self.board.get_enabled_outputs())

    def columnCount(self, parent=None):
        return len(self._table_columns)

    def supportedDropActions(self):
        return QtCore.Qt.CopyAction | QtCore.Qt.MoveAction

    def dropMimeData(self, data, action, row, column, parent):
        servo_list = [int(d) for d in data.text().split(",")]
        self.board.set_output_enabled(servo_list, True)
        return True

    def mimeTypes(self):
        return ["text/plain", "text/csv"]



class OutputTableView(table_utils.TableView):
    def __init__(self, board):
        table_utils.TableView.__init__(self)
        self.board = board

        self._sld_del = table_utils.SliderDelegate(self.board)
        self._sp_del = table_utils.SpeedDelegate()
        self._btn_del = table_utils.BtnDelegate("Del", self.disable_servo)

        self._col_del = table_utils.ColorPickDelegate()

        self.horizontalHeader().setStretchLastSection(True)
        self.verticalHeader().setMovable(True)
        self.setItemDelegateForColumn(OutputTable.GOAL, self._sld_del)
        self.setItemDelegateForColumn(OutputTable.SPEED, self._sp_del)
        self.setItemDelegateForColumn(OutputTable.ENABLED, self._btn_del)
        self.setItemDelegateForColumn(OutputTable.COLOR, self._col_del)

        self.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
        self.setDragDropMode(self.DropOnly)
        self.setDragEnabled(True)
        self.setAcceptDrops(True)
        self.setDropIndicatorShown(True)

    def disable_servo(self, index):
        output = self.board.get_enabled_outputs()[index.row()]
        self.board.set_output_enabled(output, False)

    def update_model(self):
        self.resizeColumnsToContents()
        self.horizontalHeader().resizeSection(OutputTable.NAME, 70)
        self.horizontalHeader().resizeSection(OutputTable.GOAL, 200)

    def contextMenuEvent(self, event):
        idx = self.indexAt(event.pos())
        col = idx.column()
        model = self.model()

        menu = QtGui.QMenu()
        menu.addAction("Disable output", self.disable_selected)
        menu.addAction("Reset values", self.reset_selected)
        menu.addAction("Copy", self.copy, QtGui.QKeySequence.Copy)
        menu_paste = QtGui.QMenu('Paste')
        menu_paste.addAction("All", self.paste, QtGui.QKeySequence.Paste)
        if col >= 0:
            menu_paste.addAction(model.headerData(col, QtCore.Qt.Horizontal), partial(self.paste_col_only, col))

        menu.addMenu(menu_paste)
        menu.exec_(event.globalPos())

    def disable_selected(self):
        rows = set([idx.row() for idx in self.selectedIndexes()])
        outputs = self.board.get_enabled_outputs()
        to_disable = [outputs[row] for row in rows]
        self.board.set_output_enabled(to_disable, False)

    def reset_selected(self):
        model = self.model()
        for idx in self.selectedIndexes():
            model.setData(idx.sibling(idx.row(), model.SPEED), QtCore.QVariant(0))
            model.setData(idx.sibling(idx.row(), model.GOAL), QtCore.QVariant(0.0))



