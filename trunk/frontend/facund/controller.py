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

class Controller:
	def __init__(self, view, computersModel, updateModel):
		self.__view = view
		self.__view.setController(self)
		self.__computersModel = computersModel
		self.__view.setComputerTreeModel(self.__computersModel)
		self.__currentComputer = None
		self.__currentDirectory = None
		self.__updateModel = updateModel
		self.__view.setUpdateViewModel(self.__updateModel)
		self.__inServices = False
		self.__selectedUpdate = None

	def run(self):
		self.__view.run()

	def shutdown(self):
		'''Disconnect's from all computers'''
		computers = self.__computersModel.getComputers()
		for c in computers:
			computers[c].disconnect()

	def onComputerTreeSelect(self, position):
		self.__currentDirectory = None
		self.__updateModel.empty()
		self.__inServices = False
		self.__selectedUpdate = None

		computer = self.__computersModel.getComputer(position[0])
		self.__view.setConnected(computer.getConnectionStatus())
		if computer.getConnectionStatus() is not True:
			self.__view.setInstallable(False, False)

		# We can disable the restart button as it will be
		# enabled only when we select a service to start
		self.__view.setRestartable(False)

		self.__currentComputer = computer

		if len(position) == 1:
			return

		dir = computer.getDirs()[position[1]]
		self.__currentDirectory = dir

		if len(position) == 2:
			return

		command = dir.getCommands()[position[2]]
		response = dir.runCommand(position[2])
		data = response.getData()
		if data is None:
			# We can't to an install or remove when we have nothing
			self.__view.setInstallable(False, False)
			return

		if command is 'Services':
			self.__inServices = True
			for service in data.getData():
				self.__updateModel.addUpdate(service)
		else:
			item = data.getData()[0]
			# Each item will be a pair of <dir, update list>
			pair = item.getData()
			theDir = pair[0].getData()

			for update in pair[1].getData():
				self.__updateModel.addUpdate(update)

			if self.__updateModel.getSize() > 0:
				if command == "Avaliable":
					self.__view.setInstallable(True, False)
				elif command == "Installed":
					self.__view.setInstallable(False, True)
			else:
				self.__view.setInstallable(False, False)

	def onSelectUpdate(self, item):
		if not self.__inServices:
			return
		self.__selectedUpdate = self.__updateModel.getUpdate(item)
		self.__view.setRestartable(True)

	def getCurrentComputer(self):
		return self.__currentComputer

	def getCurrentDirectory(self):
		return self.__currentDirectory

	def getCurrentService(self):
		return self.__selectedUpdate

	def installUpdates(self, updates):
		computer = self.getCurrentComputer()
		computer.installUpdates(True, updates)

	def removeUpdates(self, updates):
		computer = self.getCurrentComputer()
		computer.installUpdates(False, updates)

	def restartService(self, dir, service):
		computer = self.getCurrentComputer()
		computer.restartService(dir, service)
