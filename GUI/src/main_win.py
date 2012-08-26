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
Main window
"""

import sys
import time
import pyopenscb

import board
import dfuprog

import output_config_tab
import input_config_tab
import control_tab
import seq_tab
import debug_tab


from PyQt4 import QtGui, QtCore


ABOUT_OPENSCB = \
"""<br />
OpenSCB GUI v%s<br />
<br />
OpenSCB is a fully open source servo controller.<br />
Find out more at <a href='http://openscb.org/'>openscb.org</a>.
""" % pyopenscb.OPENSCB_BINDING_VERSION

DEV_NOT_FOUND = \
"""Cannot connect to OpenSCB board.

If the board is actually connected:
 - check device privileges (Linux)
 - try to unplug, and replug the board

You can still use the GUI in simulation mode, launch with
'--simu' parameter."""


BOARD_BOOTLOADER = \
"""Board is now in bootloader mode.

Would you like to load a new firmware?"""

TWO_BOARD_FOUND = \
"""Two or more OpenSCB board have been detected on your computer.
For the moment the software can handle only one. Unplug the extra boards
and retry."""

INCONSISTENT_VERSION = \
"""The version of the different softwares are inconsistent:
-firmware (v%s)
-API / binding (v%s)

You need to upgrade the firmware, do you want to switch
the board to bootloader mode?
"""


class MainWindow(QtGui.QMainWindow):
    """Application main window."""
    def __init__(self, options):
        QtGui.QMainWindow.__init__(self)
        self.options = options
        self.simu = None
        self._tabs = []
        self._current_tab = 0
        self._cur_project_file = None

        #in simulation mode use the simulation backend
        if self.options.simulation:
            self.openscb_board = pyopenscb.OpenSCBSimu()
        else:
            self.openscb_board = pyopenscb.OpenSCBBoard()
        self.board = board.Board(self.openscb_board)

        #init window properties
        self.setWindowTitle("OpenSCB v0.2")
        self.resize(1024, 768)


        #main widget is a tab manager
        self.tab = QtGui.QTabWidget()
        self.setCentralWidget(self.tab)

        #file menu is created first
        self.file_menu = self.menuBar().addMenu("&File")
        self.file_menu.aboutToShow.connect(self._update_file_menu)

        #create each application tab, they might create their own menu
        self._control_tab = self._add_tab(control_tab.ServoControlTab, "Control")
        self._sequence_tab = self._add_tab(seq_tab.SequenceTab, "Sequence")
        self._out_calib_tab = self._add_tab(output_config_tab.ServoCalibTab, "Output configuration")
        self._in_calib_tab = self._add_tab(input_config_tab.InputCalibTab, "Input configuration")
        self._flash_tab = self._add_tab(debug_tab.DebugLogTab, "Debug/log")

        #help menu is the last one
        self.help_menu = self.menuBar().addMenu("&Help")
        self.help_menu.addAction("&About", self._about)

        self.tab.currentChanged.connect(self._on_tab_changed)

        self.timer = QtCore.QTimer()
        self.timer.timeout.connect(self._update)
        self.timer.start(40)

    def _add_tab(self, tab_cls, name):
        new_tab = tab_cls(self.tab, self.board)
        self.tab.addTab(new_tab, name)
        self._tabs.append(new_tab)

        if new_tab.menu:
            self.menuBar().addMenu(new_tab.menu)

        return new_tab

    def _on_tab_changed(self, nb):
        old_tab = self._tabs[self._current_tab]
        old_tab.on_tab_exit()

        new_tab = self._tabs[nb]
        new_tab.on_tab_enter()

        self._current_tab = nb

    def _set_project_name(self, filename):
        self.setWindowTitle("OpenSCB v0.2 - %s" % filename)
        self._cur_project_file = filename

    def _save_to_filename(self, filename):
        try:
            with open(filename, 'w') as f:
                f.write("OpenSCB 0.2 Project\n")
                f.write(self.board.export_all_to_json())
                self.board.save_all_to_userflash()
                self._set_project_name(filename)
        except IOError as (_, strerror):
            QtGui.QMessageBox.critical("Save failed",
                            "Cannot open file for writing:\n%s\n%s" % (filename, strerror))

    def _save_project(self):
        if self._cur_project_file:
            self._save_to_filename(self._cur_project_file)
        else:
            self.board.save_all_to_userflash()

    def _save_project_as(self):
        filename = QtGui.QFileDialog.getSaveFileName(self, "Save project",
                                            filter="OpenSCB project files (*.scb)",
                                            directory=self._cur_project_file or "")
        if filename:
            self._save_to_filename(filename)

    def _load_project(self):
        filename = QtGui.QFileDialog.getOpenFileName(self, "Load project",
                                            filter="OpenSCB project files (*.scb)",
                                            directory=self._cur_project_file or "")
        if not filename:
            return
        try:
            with open(filename, 'r') as f:
                if f.readline() != "OpenSCB 0.2 Project\n":
                    print "Unrecognized file format"
                    return
                self.board.import_all_from_json(f.read())
                self.board.save_all_to_userflash()
                self._set_project_name(filename)
        except IOError as (_, strerror):
            QtGui.QMessageBox.critical("Load failed",
                            "Cannot open file for reading: %s\n%s\n" % (filename, strerror))

    def _update_file_menu(self):
        self.file_menu.clear()

        if self._cur_project_file:
            txt = "&Save project (file+flash)"
        else:
            txt = "&Save project (flash only)"
        self.file_menu.addAction(txt, self._save_project,
                                        QtGui.QKeySequence.Save)
        self.file_menu.addSeparator()

        self.file_menu.addAction("&Backup project as...", self._save_project_as,
                                 QtGui.QKeySequence.SaveAs)
        self.file_menu.addAction("&Restore project...", self._load_project,
                                 QtGui.QKeySequence.Open)
        self.file_menu.addSeparator()

        self.file_menu.addAction("&Kill switch (Disable all)", self._stop_servos,
                                 QtGui.QKeySequence(QtCore.Qt.Key_Escape))
        self.file_menu.addAction("&Re-enable servos", self._enable_servos)
        self.file_menu.addSeparator()

        self.file_menu.addAction("Upload new firmware", self._reset_bootloader)
        self.file_menu.addSeparator()

        self.file_menu.addAction("&Exit", self._exit, QtGui.QKeySequence.Close)

    def _update(self):
        try:
            if self.board.is_connected():
                self.board.update_position()
            else:
                self._connect()
        except pyopenscb.ConnectionProblem:
            pass

    def _connect(self):
        try:
            self.board.connect()

        except pyopenscb.BoardInBootloaderMode:
            ret = QtGui.QMessageBox.critical(None, "Bootloader mode",
                                       BOARD_BOOTLOADER,
                                       QtGui.QMessageBox.Yes,
                                       QtGui.QMessageBox.No)
            if ret == QtGui.QMessageBox.No:
                sys.exit(-1)
            else:
                dfuprog.prog_popup()

        except pyopenscb.TwoOrMoreBoard:
            ret = QtGui.QMessageBox.critical(None, "Several Board Detected",
                                       TWO_BOARD_FOUND, QtGui.QMessageBox.Retry,
                                       QtGui.QMessageBox.Abort)
            if ret == QtGui.QMessageBox.Abort:
                sys.exit(-1)
        except pyopenscb.DeviceNotFound:
            ret = QtGui.QMessageBox.critical(None, "OpenSCB Connection Error",
                                       DEV_NOT_FOUND, QtGui.QMessageBox.Retry,
                                       QtGui.QMessageBox.Abort)
            if ret == QtGui.QMessageBox.Abort:
                sys.exit(-1)
        except board.IncompatibleVersion, ex:
            ret = QtGui.QMessageBox.critical(None, "OpenSCB need firmware upgrade",
                               INCONSISTENT_VERSION % (ex.ver_firm, ex.ver_api),
                               "Bootloader",
                               "Exit")
            if ret == 1:
                sys.exit(-1)
            else:
                self._reset_bootloader()

    def closeEvent(self, event):
        self.openscb_board.close()

        if self.simu is not None:
            self.simu.stop()

    def _stop_servos(self):
        self.board.disable_all_outputs()

    def _enable_servos(self):
        self.board.enable_all_outputs()
        self._control_tab.out_table.layoutChanged.emit()
        self._control_tab.board_scene.update()

    def _reset_bootloader(self):
        self.openscb_board.restart_bootloader()
        time.sleep(1)

    def _about(self):
        QtGui.QMessageBox.about(self, "About OpenSCB GUI", ABOUT_OPENSCB)

    def _exit(self):
        QtCore.QCoreApplication.quit()
