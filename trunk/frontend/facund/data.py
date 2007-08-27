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

import struct

class Object:
	def __init__(self, type):
		self.__parent = None
		self.__data = None
		self.__type = type

	def __str__(self):
		'''Get the XML string for the current object'''
		if self.__data is None:
			raise ValueError("No value set")
		return "<data type=\"%s\">%s</data>" % \
		    (self.__type, str(self.getData()).lower())
		
	def setParent(self, parent):
		self.__parent = parent

	def getParent(self):
		return self.__parent

	def setData(self, data):
		self.__data = data

	def getData(self):
		return self.__data

	def getType(self):
		return self.__type

class Bool(Object):
	def __init__(self, data = None):
		Object.__init__(self, "bool")
		if data is not None:
			self.setData(data)

	def setData(self, data):
		data = str(data).lower()
		Object.setData(self, data == 'true')

class Int(Object):
	def __init__(self, data = None):
		Object.__init__(self, "int")
		self.__min = (-0x7fffffff-1)
		self.__max = (0x7fffffff)
		if data is not None:
			self.setData(data)

	def setData(self, data):
		data = int(data)
		if data < self.__min or data > self.__max:
			raise ValueError("Out of range")
		Object.setData(self, int(data))

class UnsignedInt(Object):
	def __init__(self, data = None):
		Object.__init__(self, "unsigned int")
		self.__min = 0
		self.__max = (0xffffffff)
		if data is not None:
			self.setData(data)

	def setData(self, data):
		data = int(data)
		if data < self.__min or data > self.__max:
			raise ValueError("Out of range")
		Object.setData(self, int(data))

class String(Object):
	def __init__(self, data = None):
		Object.__init__(self, "string")
		if data is not None:
			self.setData(data)
		
	def setData(self, data):
		Object.setData(self, str(data))

class Array(Object):
	def __init__(self):
		Object.__init__(self, "array")
		self.__data = []

	def __str__(self):
		s = "<data type=\"array\">"
		for data in self.__data:
			s += str(data)
		s += "</data>"
		return s

	def append(self, data):
		'''Appends the data to the end of the array'''
		if not isinstance(data, Object):
			raise ValueError("Not a Faund Object")
		self.__data.append(data)
		data.setParent(self)

	def getData(self):
		return self.__data

	def setData(self, data):
		raise ValueError("Array's can't have data set")

