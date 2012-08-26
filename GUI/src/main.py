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
Main program.
"""

#temp, add the path to pyopenscb
import sys
sys.path.append('../../API')

from pyopenscb import OpenSCBBoard

try:
    from PyQt4 import QtGui
except:
    print "Cannot load PyQt library."
    sys.exit(-1)

import main_win

from optparse import OptionParser
import logging



if __name__ == '__main__':
    parser = OptionParser()
    parser.add_option("-v", "--verbose",
                      action="store_true", dest="verbose", default=False,
                      help="Activate debug messages")
    parser.add_option("-p", "--profile",
                      action="store_true", dest="profile", default=False,
                      help="Activate profiling messages")


    parser.add_option("-s", "--simulation",
                      action="store_true", dest="simulation", default=False,
                      help="Connect to a simulator instead of a board")

    (options, args) = parser.parse_args()

    if options.verbose:
        level = logging.DEBUG
    else:
        level = logging.WARNING
    logging.basicConfig(level=level)





    app = QtGui.QApplication(args)
    win = main_win.MainWindow(options)
    win.show()

    if options.profile:
        import cProfile
        import pstats
        cProfile.run('app.exec_()', 'gui_prof')

        p = pstats.Stats('gui_prof')
        p.sort_stats('time').print_stats(20)
    else:
        app.exec_()

