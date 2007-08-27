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
import fcntl
import os
import socket
import subprocess
import threading
import xml.sax.handler

class PipeComms:
	def __init__(self, server, socket):
		self.popen = subprocess.Popen(["/usr/bin/ssh", server, "/usr/bin/nc -oU %s" % socket], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
		self.stdout = self.popen.stdout.fileno()

	def isOpen(self):
		return self.popen.poll() is None

	def read(self, len):
		assert self.isOpen()
		return os.read(self.stdout, len)

	def write(self, buf):
		assert self.isOpen()
		self.popen.stdin.write(buf)

class SocketComms:
	def __init__(self, server):
		self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
		self.socket.connect(server)

	def isOpen(self):
		return True

	def read(self, len):
		return self.socket.recv(len)

	def write(self, buf):
		self.socket.send(buf)

class Connection(xml.sax.handler.ContentHandler):
	'''A class that works as a client with the Facund XML IPC'''
	def __init__(self, server, socket):
		self.isReady = False
		self.connectionType = "unix"

		self.__data = None
		self.__calls = {}

		self.bufSize = 1024
		if server is None:
			self.__connection = SocketComms(socket)
		else:
			self.__connection = PipeComms(server, socket)

		self.send("<facund-client version=\"0\">")

		self.parser = xml.sax.make_parser()
		self.parser.setContentHandler(self)

		self.__connected_lock = threading.Lock()
		self.startLock = threading.Lock()
		self.startLock.acquire()

		self.canClose = False
		# Mark the class as ready and able to disconnect
		self.isReady = True

	def isOpen(self):
		return self.__connection.isOpen()

	def disconnect(self):
		if self.isReady:
			self.isReady = False
			# Send a connection close
			try:
				self.send("</facund-client>")
			except socket.error:
				pass

			# Wait for the other end to close
			self.__connected_lock.acquire()
			self.__connected_lock.release()

			try:
				self.parser.close()
			except xml.sax._exceptions.SAXParseException:
				pass

	def doCall(self, call):
		call.acquireLock()
		self.__calls[str(call.getID())] = call
		self.send(call.getCall())

	def send(self, buf):
		self.__connection.write(buf)

	def recv(self, len):
		return self.__connection.read(len)

	def getSalt(self):
		return self.__salt

	def interact(self):
		'''Reads data from the connection and passes it to the
		XML parser'''
		if not self.canClose:
			data = self.recv(self.bufSize)
			try:
				self.parser.feed(data)
			except xml.sax._exceptions.SAXParseException:
				return False
			return True
		return False
	
	def startElement(self, name, attributes):
		if name == "data":
			data_type = None
			data = None
			for attr in attributes.items():
				if attr[0] == "type":
					# TODO: If data_type is not None then raise exception
					data_type = attr[1]

			if data_type == "bool":
				data = facund.Bool()
			elif data_type == "int":
				data = facund.Int()
			elif data_type == "unsigned int":
				data = facund.UInt()
			elif data_type == "string":
				data = facund.String()
			elif data_type == "array":
				data = facund.Array()

			# TODO: Check if data == None
			data.setParent(self.__data)
			self.__data = data

		elif name == "response":
			self.__responseID = None
			self.__responseMessage = None
			self.__responseCode = None

			for name, value in attributes.items():
				if name == "id":
					self.__responseID = int(value)
				elif name == "code":
					self.__responseCode = int(value)
				elif name == "message":
					self.__responseMessage = str(value)
				else:
					print attr

		elif name == "facund-server":
			for name, value in attributes.items():
				if name == 'salt':
					self.__salt = int(value)
			self.__connected_lock.acquire()
			self.startLock.release()

	def endElement(self, name):
		if name == "data":
			data = self.__data.getParent()
			if data is not None:
				data.append(self.__data)
				self.__data = data

		elif name == "response":
			response = facund.Response(self.__responseID,
			    self.__responseCode, self.__responseMessage,
			    self.__data)

			# TODO: Check this is a valid item
			call = self.__calls[str(self.__responseID)]
			call.setResponse(response)
			call.releaseLock()
			self.__calls[self.__responseID] = None

			self.__data = None

		elif name == "facund-server":
			# The server sent a close message
			self.__connected_lock.release()
			self.canClose = True

	def characters(self, text):
		print "==> " + text
		self.__data.setData(text)
