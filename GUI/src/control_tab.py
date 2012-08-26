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
Tab for servo control array and board display
"""


from PyQt4 import QtGui, QtCore
import main_win_tab
import board_display
import servo_table
import output_table
import table_utils

class ServoControlTab(main_win_tab.MainTabWidget):
    def __init__(self, tab, board):
        main_win_tab.MainTabWidget.__init__(self, tab, board)

        lay = QtGui.QHBoxLayout()

        #left widget is a drawing of the board
        self.board_scene = QtGui.QGraphicsScene()
        self.board_view = QtGui.QGraphicsView(self.board_scene)
        self.board_view.setMinimumWidth(450)
        self.board_graphic = None
        lay.addWidget(self.board_view)


        lay2 = QtGui.QVBoxLayout()

        #A table with configured servo (enabled)
        lay2.addWidget(QtGui.QLabel("Enabled outputs:"))
        self.out_table = output_table.OutputTable(self.board)
        self.out_table_view = output_table.OutputTableView(self.board)
        lay2.addWidget(self.out_table_view)

        #A table with all available servo
        lay2.addWidget(QtGui.QLabel("Available outputs:"))
        self.sv_table = servo_table.ServoTable(self.board)
        self.sv_table_view = servo_table.ServoTableView(self.board)
        lay2.addWidget(self.sv_table_view)

        lay.addLayout(lay2, stretch=1)
        self.setLayout(lay)

    def _on_connect(self):
        self.out_table.update_all()
        self.out_table_view.setModel(self.out_table)
        self.out_table_view.update_model()

        self.sv_table.update_from_board()
        self.sv_table_view.setModel(self.sv_table)

        if self.board_graphic:
            self.board_scene.removeItem(self.board_graphic)
        self.board_graphic = board_display.BoardGraphics()
        self.board_scene.addItem(self.board_graphic)

        board_display.BoardInput(self.board_graphic)

        for i in range(self.board.output_nb):
            board_display.BoardOutput(self.board_graphic, self.board, i)

        for i in range(self.board.output_nb):
            self.out_table_view.openPersistentEditor(self.out_table.createIndex(i, self.out_table.ENABLED))


    def on_tab_exit(self):
        for i in range(self.board.output_nb):
            self.board.set_out_api_controlled(i, False)

    def on_tab_enter(self):
        pass
