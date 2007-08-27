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

import facund.network
import hashlib
import socket
import threading
import types

class Directory:
	def __init__(self, name, computer):
		self.__name = name
		self.__computer = computer

	def getName(self):
		return self.__name

	def getCommands(self):
		return self.__computer.getCommands(self.__name)

	def runCommand(self, position):
		return self.__computer.runCommand(position, self.__name)

class Computer(threading.Thread):
	'''A class to describe each computer able to be connected to'''
	def __init__(self, name, host, socket):
		threading.Thread.__init__(self)
		self.__name = name
		self.__host = host
		self.__socket = socket
		self.__dirs = []
		self.__connected = False
		self.__connection = None
		self.__commands = ['Avaliable', 'Installed', 'Services']

	def __str__(self):
		return self.__name + ": " + (self.__host or self.__socket)

	def addDir(self, dir):
		'''Adds a directory to the avaliable directories to update'''
		self.__dirs.append(facund.Directory(dir, self))

	def getConnectionStatus(self):
		'''Returns the connection state'''
		return self.__connection is not None and \
		    self.__connection.isOpen()

	def getDirs(self):
		'''Returns the computer's directories to update'''
		return self.__dirs

	def getName(self):
		'''Returns the Human readable name for the computer'''
		return self.__name
	
	def getHost(self):
		'''Returns the hostname/ip of the computer'''
		return self.__host

	def getSocket(self):
		return self.__socket

	def getCommands(self, dir):
		return self.__commands

	def runCommand(self, command, dir = None):
		print self.__commands[command]
		if self.__commands[command] == 'Avaliable':
			return self.getUpdateList(dir)
		elif self.__commands[command] == 'Installed':
			return self.getInstalledList(dir)
		elif self.__commands[command] == 'Services':
			return self.getServicesList(dir)
		else:
			print 'TODO: Handle this command (%d)' % (command,);

	def buildInstallArg(self, location, patches):
		arg = facund.Array()
		arg.append(facund.String(location))

		if isinstance(patches, types.StringTypes):
			arg.append(facund.String(patches))
		elif isinstance(patches, types.ListType):
			items = facund.Array()
			for patch in patches:
				items.append(facund.String(patch))
			arg.append(items)
		return arg

	def installUpdates(self, isInstall, installTypes):
		args = self.buildInstallArg(installTypes[0], installTypes[1])

		if isInstall:
			callType = "install_patches"
		else:
			callType = "rollback_patches"
		call = facund.Call(callType, args)
		self.__connection.doCall(call)
		# Wait for the response
		call.acquireLock()
		call.releaseLock()
		return call.getResponse()

	def restartService(self, dir, service):
		args = self.buildInstallArg(dir, service)
		call = facund.Call("restart_services", args)
		self.__connection.doCall(call)
		# Wait for the response
		call.acquireLock()
		call.releaseLock()
		return call.getResponse()

	def getUpdateList(self, dir = None):
		if dir is None:
			for dir in self.__dirs:
				args = None
		else:
			args = facund.Array()
			args.append(facund.String("base"))
			args.append(facund.String(dir))

		call = facund.Call("list_updates", args)
		self.__connection.doCall(call)
		# Wait for the response
		call.acquireLock()
		call.releaseLock()
		return call.getResponse()

	def getInstalledList(self, dir = None):
		if dir is None:
			for dir in self.__dirs:
				args = None
		else:
			args = facund.Array()
			args.append(facund.String("base"))
			args.append(facund.String(dir))

		call = facund.Call("list_installed", args)
		self.__connection.doCall(call)
		# Wait for the response
		call.acquireLock()
		call.releaseLock()
		return call.getResponse()

	def getServicesList(self, dir):
		arg = facund.String(dir)
		call = facund.Call("get_services", arg)
		self.__connection.doCall(call)
		# Wait for the response
		call.acquireLock()
		call.releaseLock()
		return call.getResponse()


	def connect(self, password):
		'''Connects to the remote computer'''
		if self.__connection is not None:
			return

		try:
			self.__connection = \
			    facund.network.Connection(self.__host,self.__socket)
			if not self.__connection.isOpen():
				print "Couldn't connect to %s " % (self.__host or self.__socket)
				del self.__connection
				self.__connection = None
				return

			# Start the communication thread
			self.start()

			self.__connection.startLock.acquire()
			self.__connection.startLock.release()

			# Authenticate with the server
			salt = self.__connection.getSalt()
			pass_hash = hashlib.sha256(password).hexdigest()
			pass_hash = pass_hash + str(salt)
			pass_hash = hashlib.sha256(pass_hash).hexdigest()
			pass_hash = facund.String(pass_hash)
			call = facund.Call("authenticate", pass_hash)
			self.__connection.doCall(call)
			call.acquireLock()
			call.releaseLock()
			# Disconnect if we failed to authenticate
			if call.getResponse().getCode() != 0:
				print call.getResponse().getCode()
				self.disconnect()
				return

			# Get a list of directories the server offers
			call = facund.Call("get_directories", None)
			self.__connection.doCall(call)
			call.acquireLock()
			call.releaseLock()
			dirs = call.getResponse().getData().getData()
			for dir in dirs:
				self.addDir(dir.getData())

		except socket.error:
			print "Couldn't connect to %s " % (self.__host or self.__socket)
			del self.__connection
			self.__connection = None

	def disconnect(self):
		'''Disconnects from the remote computer'''
		if self.__connection is None:
			return

		self.__connection.disconnect()
		# Wait for the thread to exit then create a new thread
		self.join()
		threading.Thread.__init__(self)
		del self.__connection
		self.__connection = None
		self.__dirs = []

	def run(self):
		'''The main communications thread'''
		while self.__connection.interact():
			continue
