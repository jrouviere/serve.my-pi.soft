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
Tab for sequence editor
"""

from PyQt4 import QtGui, QtCore

import main_win_tab
import seq_editor
import sequence
import seq_table
import table_utils

#import json
#from os import path

from functools import partial


class SequenceTab(main_win_tab.MainTabWidget):
    def __init__(self, tab, board):
        main_win_tab.MainTabWidget.__init__(self, tab, board)

        #sequence currently edited in the editor
        self.seq_proxy = sequence.SequenceSelectorProxy(sequence.Sequence())

        #build tab menu
        self.menu = QtGui.QMenu("&Sequence")
        self.menu.aboutToShow.connect(self._update_menu)

        self._last_loaded_sequence = None

        lay = QtGui.QVBoxLayout()

        lay_form = QtGui.QFormLayout()

        self.seq_desc = QtGui.QLineEdit()
        self.seq_desc.textChanged.connect(self._update_seq_desc)
        self.seq_desc_lbl = QtGui.QLabel("")
        lay_form.addRow(self.seq_desc_lbl, self.seq_desc)
        lay.addLayout(lay_form)

        #top layout: graphical seq_proxy editor
        self.seq_editor = seq_editor.SequenceEditor(self.board, self.seq_proxy)
        self.seq_editor_view = QtGui.QGraphicsView(self.seq_editor)
        self.seq_editor_view.setDragMode(QtGui.QGraphicsView.RubberBandDrag)
        self.seq_editor_view.setInteractive(True)
        self.seq_editor_view.centerOn(0, 0)
        self.seq_editor_view.setMinimumHeight(440)
        lay.addWidget(self.seq_editor_view)


        #bottom layout
        lay2 = QtGui.QHBoxLayout()
        #table with list of frame
        self.seq_table = seq_table.SequenceTableModel(self.seq_proxy)
        self.seq_table_view = QtGui.QTableView()
        self.seq_table_view.setModel(self.seq_table)
        self.seq_table.set_selection(self.seq_table_view.selectionModel())
        self.seq_table_view.horizontalHeader().setStretchLastSection(True)
        self.seq_table_view.verticalHeader().setResizeMode(QtGui.QHeaderView.Fixed)
        lay2.addWidget(self.seq_table_view)

        #output position table (for one frame)
        self.seq_sv_table = seq_table.SeqOutputTableModel(self.board, self.seq_proxy)
        self.seq_sv_table_view = seq_table.SeqOutputTableView(self.board)
        lay2.addWidget(self.seq_sv_table_view, stretch=1)

        lay.addLayout(lay2)
        self.setLayout(lay)

    def _update_seq_desc(self, txt):
        self.seq_proxy.set_description(str(txt))

    def _update_menu(self):
        self.menu.clear()
        self.menu_slot = QtGui.QMenu("Select slot")
        self.menu_copy_slot = QtGui.QMenu("Copy to slot")
        for slot in range(1, self.board.get_sequence_nb() + 1):
            self.menu_slot.addAction("# %d (%s)" % (slot, self.board.get_sequence(slot).get_description()),
                                     partial(self.select_slot, slot))
            self.menu_copy_slot.addAction("# %d" % (slot), partial(self.copy_to_slot, slot))

        self.menu.addAction("&Clear", self.seq_clear)
        self.menu.addAction("&Reverse", self.seq_proxy.reverse_frames)

        self.menu.addSeparator()
        self.menu.addMenu(self.menu_slot)
        self.menu.addMenu(self.menu_copy_slot)

        self.menu.addSeparator()
        self.menu.addAction("&Play", self.seq_play, QtCore.Qt.Key_F5)
        self.menu.addAction("&Play loop", self.seq_play_loop, QtCore.Qt.Key_F6)
        self.menu.addAction("&Stop", self.seq_stop, QtCore.Qt.Key_F9)


    def select_slot(self, slot):
        self.seq_proxy.change_sequence(self.board.get_sequence(slot))

        self.seq_desc_lbl.setText("Editing sequence # %d\tTitle:" % slot)
        self.seq_desc.setText(self.seq_proxy.get_description())

    def copy_to_slot(self, slot):
        answer = QtGui.QMessageBox.question(self, "Replace sequence?",
                "This will delete sequence %d and replace it with the\n"\
                "content of current sequence.\n"\
                "Are you sure?" % slot,
                QtGui.QMessageBox.Yes, QtGui.QMessageBox.No)

        if answer == QtGui.QMessageBox.Yes:
            self.board.copy_sequence(slot, self.seq_proxy.get_sequence())

    def seq_play(self):
        self.seq_editor.play_sequence()

    def seq_play_loop(self):
        self.seq_editor.play_sequence(looping=True)

    def seq_stop(self):
        self.seq_editor.stop_sequence()

    def seq_clear(self):
        answer = QtGui.QMessageBox.question(self, "Clear sequence?",
                "This will delete all frame in the sequence.\nAre you sure?",
                QtGui.QMessageBox.Yes, QtGui.QMessageBox.No)

        if answer == QtGui.QMessageBox.Yes:
            self.seq_desc.clear()
            self.seq_proxy.remove_all()

    def _on_connect(self):
        self.select_slot(1)

        self.seq_sv_table_view.setModel(self.seq_sv_table)
        self.seq_sv_table.dataModified.connect(self.seq_sv_table_view.updateEditorData)

        self.seq_sv_table_view.resizeColumnToContents(self.seq_sv_table.HIDDEN)

    def on_tab_exit(self):
        self.seq_editor.stop_sequence()

    def on_tab_enter(self):
        pass
