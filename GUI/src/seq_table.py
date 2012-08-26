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
Display class for a sequence of move for a set of servo.
"""

from PyQt4 import QtGui, QtCore

import table_utils


class SequenceTableModel(QtCore.QAbstractTableModel):

    _table_columns = ['Frame', 'Length']

    def __init__(self, sequence):
        QtCore.QAbstractTableModel.__init__(self)

        self.sequence = sequence
        self.sequence.sequenceChanged.connect(self.update)
        self.selection = None

    def set_selection(self, selection):
        self.selection = selection
        self.sequence.selectionChanged.connect(self.on_seq_sel_changed)
        QtCore.QObject.connect(self.selection,
            QtCore.SIGNAL("selectionChanged(const QItemSelection&,const QItemSelection&)"),
            self.on_sel_changed)

    def on_sel_changed(self):
        selected = []
        for ind in self.selection.selectedIndexes():
            frm = self.sequence.frame[ind.row()]
            if frm not in selected:
                selected.append(frm)
        self.sequence.set_selected(selected)

    def on_seq_sel_changed(self):
        selected = self.sequence.get_selected()
        items = QtGui.QItemSelection()
        for i, frm in enumerate(self.sequence.frame):
            if frm in selected:
                id0 = self.index(i, 0)
                id1 = self.index(i, 1)
                items.select(id0, id1)
        self.selection.select(items, QtGui.QItemSelectionModel.ClearAndSelect)

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole:
            if orientation == QtCore.Qt.Horizontal:
                return self._table_columns[section]
            else:
                return '#%d' % section

    def flags(self, index):
        return QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsSelectable | \
            QtCore.Qt.ItemIsEnabled

    def data(self, index, role=QtCore.Qt.DisplayRole):
        row = index.row()
        col = index.column()
        if role in (QtCore.Qt.DisplayRole, QtCore.Qt.EditRole):
            if col == 1:
                t1 = self.sequence.frame[row]._time
                t0 = self.sequence.frame[row - 1]._time
                if row > 0:
                    return t1 - t0
                else:
                    return t1
            elif col == 0:
                return self.sequence.frame[row].name

        return None

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        row = index.row()
        col = index.column()
        if role == QtCore.Qt.EditRole:
            if col == 1:
                v, _ = value.toFloat()
                t0 = self.sequence.frame[row - 1]._time
                if row > 0:
                    self.sequence.frame[row].set_time(t0 + v)
                else:
                    self.sequence.frame[row].set_time(v)
            elif col == 0:
                v = value.toString()
                self.sequence.frame[row].name = str(v)

            first = self.createIndex(row, 0)
            last = self.createIndex(row, col)
            self.emit(QtCore.SIGNAL("dataChanged(const QModelIndex&,const QModelIndex&)"), first, last)
        return True

    def update(self):
        self.emit(QtCore.SIGNAL("layoutChanged()"))


    def rowCount(self, parent=None):
        return len(self.sequence.frame)

    def columnCount(self, parent=None):
        return len(self._table_columns)



class SeqOutputTableModel(QtCore.QAbstractTableModel):
    (HIDDEN, INV_SPEED, GOAL_NUM, GOAL,) = range(4)
    _table_columns = {
         HIDDEN: ("Hide",),
         INV_SPEED: ("Inv. speed",),
         GOAL: ("Goal",),
         GOAL_NUM: ("Goal value",),
    }
    dataModified = QtCore.pyqtSignal()

    def __init__(self, board, sequence):
        QtCore.QAbstractTableModel.__init__(self)

        self.board = board
        self.sequence = sequence

        self._once = True
        self.frame = None
        self.sequence.selectionChanged.connect(self.update_selected_frame)

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole:
            if orientation == QtCore.Qt.Horizontal:
                return self._table_columns[section][0]
            else:
                out = self.frame.get_points()[section][0]
                return '%d: %s' % (out + 1, self.board.get_output_name(out))

    def flags(self, index):
        flags = QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsEnabled
        if index.column() in (self.GOAL, self.GOAL_NUM):
            flags = flags | QtCore.Qt.ItemIsEditable
        elif index.column() == self.HIDDEN:
            flags = flags | QtCore.Qt.ItemIsUserCheckable
        return flags

    def update_selected_frame(self):
        if self.frame is not None:
            self.frame.frameChanged.disconnect(self.update_frame)
        sel = self.sequence.get_highlighted()
        if sel is None:
            self.frame = None
        else:
            self.frame = sel
            self.frame.frameChanged.connect(self.update_frame)
            self.update_frame()
        if self.frame:
            self.board.load_frame(1.0, self.frame.get_point_dict())
        self.layoutChanged.emit()

    def update_frame(self):
        if self.frame and self._once:
            self._once = False
            self.board.load_frame(0.5, self.frame.get_point_dict())
            self.dataModified.emit()
            self.layoutChanged.emit()
            self._once = True

    def data(self, index, role=QtCore.Qt.DisplayRole):
        if not self.frame:
            return None

        out = index.row()
        col = index.column()

        pts = self.frame.get_points()
        if out < len(pts):
            if role == QtCore.Qt.BackgroundRole:
                out_no = pts[out][0]
                return QtGui.QBrush(self.board.get_output_color(out_no))

            elif col == self.HIDDEN:
                if role == QtCore.Qt.CheckStateRole:
                    if self.frame.is_visible(pts[out][0]):
                        return QtCore.Qt.Unchecked
                    else:
                        return QtCore.Qt.Checked

            #compute speed (inverse) in manufacturer unit: sec / 60 deg 
            elif col == self.INV_SPEED:
                if role in (QtCore.Qt.DisplayRole,):
                    prev_frame = self.frame.get_previous()

                    if prev_frame:
                        prev_pts = prev_frame.get_points()

                        dT = self.frame.get_time() - prev_frame.get_time()
                        dV = abs(pts[out][1] - prev_pts[out][1])
                        if dV == 0.0 or (60 / (dV * 180) * dT > 10):
                            return u'> 10 sec/60\xb0'

                        return u'%.2f sec/60\xb0' % (60 / (dV * 180) * dT)
                    else:
                        return ''

            elif col == self.GOAL_NUM:
                if role == QtCore.Qt.DisplayRole:
                    return u"% 4.1f\xb0" % (180 * pts[out][1])
                elif role == QtCore.Qt.EditRole:
                    return 180 * pts[out][1]

            elif col == self.GOAL:
                val = pts[out][1]
                if role in (QtCore.Qt.DisplayRole, QtCore.Qt.EditRole):
                    return val
                elif role == QtCore.Qt.UserRole:
                    out_no = pts[out][0]
                    left, right = self.board.get_output_range(out_no)
                    return QtCore.QVariant.fromList([left, right])

        return None

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        out = index.row()
        col = index.column()
        out_no = self.frame.get_points()[out][0]
        if out < self.board.output_nb:
            if col == self.GOAL:
                if role == QtCore.Qt.EditRole:
                    goal, ok = value.toFloat()
                    if ok:
                        self.frame.set_point_value(out_no, goal)
                        self.frame.check_update()

            elif col == self.GOAL_NUM:
                if role == QtCore.Qt.EditRole:
                    goal, ok = value.toFloat()
                    if ok:
                        self.frame.set_point_value(out_no, goal / 180.0)
                        self.frame.check_update()

            elif col == self.HIDDEN:
                if role == QtCore.Qt.CheckStateRole:
                    visible = (value == QtCore.Qt.Unchecked)
                    self.frame.set_visibility(out_no, visible)


        first = self.createIndex(out, 0)
        last = self.createIndex(out, self.columnCount() - 1)
        self.emit(QtCore.SIGNAL("dataChanged(const QModelIndex&,const QModelIndex&)"), first, last)
        return True

    def rowCount(self, parent=None):
        if self.frame is None:
            return 0
        return len(self.frame.get_points())

    def columnCount(self, parent=None):
        return len(self._table_columns)



class SeqOutputTableView(table_utils.TableView):

    def __init__(self, board):
        table_utils.TableView.__init__(self)

        self.board = board
        self.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)

        self._sld_del = table_utils.SliderDelegate(self.board)
        self.setItemDelegateForColumn(SeqOutputTableModel.GOAL, self._sld_del)
        self.horizontalHeader().setStretchLastSection(True)
        self.verticalHeader().setResizeMode(QtGui.QHeaderView.Fixed)

    def remove_selected(self):
        model = self.model()
        pts = model.frame.get_points()
        rows = set([idx.row() for idx in self.selectedIndexes()])
        for row in rows:
            no = pts[row][0]
            model.sequence.remove_output(no)
        model.emit(QtCore.SIGNAL("layoutChanged()"))

    def contextMenuEvent(self, event):
        menu = QtGui.QMenu()

        act_rem = menu.addAction("Remove outputs", self.remove_selected)
        if not self.selectedIndexes():
            act_rem.setDisabled(True)

        menu.exec_(event.globalPos())

