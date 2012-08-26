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
Use dfu-programmer to flash a new firmware into the board
"""

import subprocess
import time

from PyQt4 import QtGui, QtCore


DFU_PROG = "../../build/dfu-programmer"
CPU_NAME = "at32uc3b0128"

DEFAULT_FIRMWARE = "../../Embedded/build/openscb_firmware.hex"

PROG_ERROR = \
"""Error programming firmware: %s
 - Check dfu-programmer tool is available
 - Check firmware file is correct"""

def program(filename):
    subprocess.call([DFU_PROG, CPU_NAME, "erase"])
    subprocess.call([DFU_PROG, CPU_NAME, "flash", filename, "--suppress-bootloader-mem"])
    subprocess.call([DFU_PROG, CPU_NAME, "start"])
    time.sleep(1)


def prog_popup():
    filename = QtGui.QFileDialog.getOpenFileName(None, "Load firmware",
                                        filter="OpenSCB firmware (*.hex)",
                                        directory=DEFAULT_FIRMWARE)
    if not filename:
        return

    try:
        program(str(filename))
    except Exception:
        QtGui.QMessageBox.critical(None, "Program failed",
                        PROG_ERROR % filename)

