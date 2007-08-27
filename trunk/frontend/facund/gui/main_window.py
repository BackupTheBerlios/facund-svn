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

import facund
import gconf
import gtk
import gtk.gdk
import gtk.glade

class MainWindow:
	'''Creates the main window from a glade file. The widgets in the
	glade file must have to correct naming scheme to work'''

	def __init__(self, glade_file):
		gtk.gdk.threads_init()

		self.__xml = gtk.glade.XML(glade_file)
		self.__widget = self.__xml.get_widget('facundWindow')
            	self.__widget.connect('destroy', self.onQuit)

		self.__client = gconf.client_get_default()
		self.__width = self.__client.get_int('/facund/gui/width') or 400
		self.__height = self.__client.get_int('/facund/gui/height') or 300
		self.__widget.set_default_size(self.__width, self.__height)
		self.__widget.connect('configure-event', self.onConfigure)

		# Open the new computer window on File > New
		menuItem = self.__xml.get_widget('newConnection')
		menuItem.connect('activate', self.newConnection)

		# Remove the selected computer on File > Delete
		menuItem = self.__xml.get_widget('delConnection')
		menuItem.connect('activate', self.delConnection)

		# Attach the quit signal to File > Quit
		menuItem = self.__xml.get_widget('progQuit')
		menuItem.connect('activate', self.onQuit)

		self.__newConnectionDialog = None
		self.__passwordDialog = None

		# Connect the signals to the new connection dialog box
		button = self.__xml.get_widget('newConnectionCancel')
		button.connect('clicked', self.connectionCancel)

		button = self.__xml.get_widget('newConnectionSave')
		button.connect('clicked', self.connectionSave)

		# Connect the signals to the password dialog
		button = self.__xml.get_widget('passwordOkButton')
		button.connect('clicked', self.connectionStart)

		button = self.__xml.get_widget('passwordCancelButton')
		button.connect('clicked', self.connectionCancel)

	def onQuit(self, data):
		self.__controller.shutdown()

		# Save the width/height
		self.__client.set_int('/facund/gui/width', self.__width)
		self.__client.set_int('/facund/gui/height', self.__height)
		gtk.main_quit()

	def onConfigure(self, widget, event):
		self.__width = event.width
		self.__height = event.height

	def newConnection(self, data):
		widget = self.__xml.get_widget('newConnectionDialog')
		self.__newConnectionDialog = widget
		self.__newConnectionDialog.show()

	def delConnection(self, data):
		# Make sure the we have disconnected
		self.onDisconnectClick(data)
		computer = self.__controller.getCurrentComputer()
		computerList = facund.ComputerList()
		computerList.delComputer(computer)
		self.__computerTreeModel.removeComputer(computer)
		del computerList

	def connectionCancel(self, data):
		self.__newConnectionDialog.hide()
		self.__newConnectionDialog = None

		self.__xml.get_widget('computerNameEntry').set_text('')
		self.__xml.get_widget('computerEntry').set_text('')
		self.__xml.get_widget('computerEntry').set_text('')

	def connectionSave(self, data):
		item = self.__xml.get_widget('computerEntry')
		server = item.get_text()
		if server is '':
			server = None

		item = self.__xml.get_widget('socketEntry')
		socket =  item.get_text()
		if socket is '':
			socket = '/tmp/facund'

		item = self.__xml.get_widget('computerNameEntry')
		name = item.get_text()
		if name is '':
			name = server or 'Local Computer'

		computer = facund.Computer(name, server, socket)
		computerList = facund.ComputerList()
		computerList.addComputer(computer)
		del computerList
		self.__computerTreeModel.addComputer(computer)

		# Use the connectionCancel handler as it
		# closes the dialog and cleans up the fields
		self.connectionCancel(data)

	def setController(self, controller):
		self.__controller = controller

		installButton = self.__xml.get_widget('installButton')
		installButton.connect('clicked', self.onInstallClick)
		removeButton = self.__xml.get_widget('deinstallButton')
		removeButton.connect('clicked', self.onRemoveClick)
		restartButton = self.__xml.get_widget('restartButton')
		restartButton.connect('clicked', self.onRestartClick)

	def setUpdateViewModel(self, model):
		'''Sets the model to use to for the computer tree'''
		self.__updateViewModel = model
		treeView = self.__xml.get_widget('updateView')
		treeView.set_model(model)
		treeView.connect('cursor-changed', self.onSelectUpdate)

		cell = gtk.CellRendererText()
		column = gtk.TreeViewColumn("Update", cell, text=0)
		treeView.append_column(column)

	def setComputerTreeModel(self, model):
		'''Sets the model to use to for the computer tree'''
		self.__computerTreeModel = model
		treeView = self.__xml.get_widget('computerView')
		treeView.set_model(model)
		cell = gtk.CellRendererText()
		column = gtk.TreeViewColumn("Computer", cell, text=0)
		treeView.append_column(column)

		treeView.connect('cursor-changed', self.onSelectComputer)

		# Add signal handlers to connect/disconnect
		connectedButton = self.__xml.get_widget('connectButton')
		connectedButton.connect('clicked', self.onConnectClick)
		disconnectedButton = self.__xml.get_widget('disconnectButton')
		disconnectedButton.connect('clicked', self.onDisconnectClick)

	def setConnected(self, connected):
		connectedButton = self.__xml.get_widget('connectButton')
		disconnectedButton = self.__xml.get_widget('disconnectButton')

		connectedButton.set_sensitive(not connected)
		disconnectedButton.set_sensitive(connected)

	def setInstallable(self, installable, uninstallable):
		installButton = self.__xml.get_widget('installButton')
		installButton.set_sensitive(installable)

		deinstallButton = self.__xml.get_widget('deinstallButton')
		deinstallButton.set_sensitive(uninstallable)

	def setRestartable(self, restartable):
		restartButton = self.__xml.get_widget('restartButton')
		restartButton.set_sensitive(restartable)

	def onConnectClick(self, widget):
		'''Signal handler for the connect button'''
		self.__passwordDialog = self.__xml.get_widget('passwordDialog')
		self.__passwordDialog.show()

		self.setInstallable(False, False)

	def onDisconnectClick(self, widget):
		'''Signal handler for the connect button'''
		computer = self.__controller.getCurrentComputer()
		computer.disconnect()
		self.setConnected(computer.getConnectionStatus())
		self.__computerTreeModel.populateComputer(computer)

		# Disable the install/remove buttons
		self.setInstallable(False, False)

	def connectionStart(self, widget):
		password = self.__xml.get_widget('passwordEntry').get_text()

		computer = self.__controller.getCurrentComputer()
		computer.connect(password)
		self.setConnected(computer.getConnectionStatus())
		self.__computerTreeModel.populateComputer(computer)

		self.connectionCancel(widget)

	def connectionCancel(self, widget):
		self.__passwordDialog.hide()
		self.__passwordDialog = None

		self.__xml.get_widget('passwordEntry').set_text('')

	def onInstallClick(self, widget):
		dir = self.__controller.getCurrentDirectory()
		self.__controller.installUpdates((dir.getName(), 'base'))

	def onRemoveClick(self, widget):
		dir = self.__controller.getCurrentDirectory()
		self.__controller.removeUpdates((dir.getName(), 'base'))

	def onRestartClick(self, widget):
		dir = self.__controller.getCurrentDirectory()
		service = self.__controller.getCurrentService()
		self.__controller.restartService(dir.getName(), service)

	def onSelectComputer(self, widget):
		'''Signal handler for when the selected item is changed'''
		cursor = widget.get_cursor()
		self.__controller.onComputerTreeSelect(cursor[0])

	def onSelectUpdate(self, widget):
		cursor = widget.get_cursor()
		self.__controller.onSelectUpdate(cursor[0][0])

	def run(self):
		'''Displays the main window. Does't return'''
		self.__widget.show()
		gtk.main()

