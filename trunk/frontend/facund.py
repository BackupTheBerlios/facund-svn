#!/usr/local/bin/python
#
# Copyright (c) 2007 Andrew Turner
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

import socket

import facund, facund.gui, facund.network


#try:
#	fc = facund.network.Connection("/tmp/facund")
#except socket.error:
#	print "Couldn't connect to the back-end"
#while True:
#	fc.interact()

if __name__ == "__main__":
	computerList = facund.ComputerList()
	computers = computerList.getComputerList()

	computerModel = facund.gui.ComputerTreeModel()
	for i in range(len(computers)):
		computer = facund.Computer(computers[i][0], computers[i][1],
		    computers[i][2])
		computerModel.addComputer(computer)

	# If we nee the computer list again we gan get it in the same way
	del computers
	del computerList

	updateModel = facund.gui.UpdateListModel()

	mainWindow = facund.gui.MainWindow('facund-fe.glade')

	controller = facund.Controller(mainWindow, computerModel, updateModel);

	controller.run()

