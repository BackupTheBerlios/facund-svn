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

import gtk
import gobject

class ComputerTreeModel(gtk.TreeStore):
	'''A model to be passed to a GTK tree view where computers can be
	added and removed and base directories can be added and removed.'''
	def __init__(self):
		gtk.TreeStore.__init__(self, gobject.TYPE_STRING)

		self.__computers = {}
		self.__iterators = {}

	def addComputer(self, computer):
		'''Adds a computer to the computer tree view'''

		computer_name = computer.getName()
		if self.__computers.has_key(computer_name):
			# TODO: This should either raise an exception or just replace the item and update the tree view
			return
		self.__computers[computer_name] = computer

		# Add the computer
		iter = self.append(None)
		self.set(iter, 0, computer_name)
		self.__iterators[computer] = iter
		self.populateComputer(computer)

	def populateComputer(self, computer):
		if computer.getConnectionStatus():
			# Add the directories
			iter = self.__iterators[computer]
			for dir in computer.getDirs():
				dir_iter = self.append(iter)
				self.set(dir_iter, 0, dir.getName())
				# Add the commands for each directory
				for command in dir.getCommands():
					command_iter = self.append(dir_iter)
					self.set(command_iter, 0, command)
		else:
			iter = self.iter_children(self.__iterators[computer])
			while iter is not None:
				self.remove(iter)
				iter = self.iter_children(self.__iterators[computer])


	def getComputer(self, position):
		'''Returns the computer at the given position in the tree'''
		name = self[position][0]
		return self.__computers[name]

	def getComputers(self):
		return self.__computers

	def removeComputer(self, computer):
		'''Removes a computer from the computer tree'''
		computer_name = computer.getName()
		if self.__computers.has_key(computer_name) and \
		    self.__iterators.has_key(computer):
			del self.__computers[computer_name]
			iter = self.__iterators[computer]
			self.remove(iter)

