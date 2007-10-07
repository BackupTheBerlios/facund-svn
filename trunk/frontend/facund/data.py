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
	'''
	A Facund object is the basic data type that can be sent
	between the client and server. Most methods can be used
	with all children objects. The only exception is setData()
	can not be used with an Array.
	
	Do not use Object directly,
	rather use one of the base classes.
	'''
	def __init__(self, type):
		self.__parent = None
		self.__data = None
		self.__type = type

	def __str__(self):
		'''
		Get the XML string for the current object.

		>>> str(Bool(False))
		'<data type="bool">False</data>'

		>>> str(Int(10))
		'<data type="int">10</data>'

		>>> str(String('Hello'))
		'<data type="string">Hello</data>'
		'''
		if self.__data is None:
			raise ValueError("No value set")
		return "<data type=\"%s\">%s</data>" % \
		    (self.__type, str(self.getData()))
		
	def setParent(self, parent):
		'''
		Sets the object's parent. This is used with arrays and
		shouldn't be used by the end user.
		'''
		self.__parent = parent

	def getParent(self):
		'''
		Returns the parent object when the current object is part
		of an array.

		>>> a = Array()
		>>> b = Bool()
		>>> a.append(b)
		>>> b.getParent() is a
		True

		>>> UnsignedInt(10).getParent() is None
		True
		'''
		return self.__parent

	def setData(self, data):
		self.__data = data

	def getData(self):
		'''
		Returns the value of the data stored in the object

		>>> String("Hello World").getData()
		'Hello World'
		'''
		return self.__data

	def getType(self):
		'''
		Returns a string containing the type of data stored

		>>> Array().getType()
		'array'
		'''
		return self.__type

class Bool(Object):
	'''
	Creates a boolean Facund Object

	>>> Bool().getType()
	'bool'

	The value of a Bool can be retrieved with getData().
	It is true for all values where str(value) is True.
	>>> Bool(True).getData()
	True

	As the Bool class assumes any data that
	is not true is false this is valid:
	>>> Bool('Bad String').getData()
	False
	'''
	def __init__(self, data = None):
		Object.__init__(self, "bool")
		if data is not None:
			self.setData(data)

	def setData(self, data):
		'''
		Updates the value of a boolean

		The data can be True:
		>>> b = Bool()
		>>> b.setData('True')
		>>> b.getData()
		True

		Or the data can be anything else for false:
		>>> b = Bool()
		>>> b.setData('Foo')
		>>> b.getData()
		False

		setData can also be used to change the value of an object:
		>>> b = Bool(True)
		>>> b.getData()
		True
		>>> b.setData(False)
		>>> b.getData()
		False
		'''
		data = str(data).lower()
		Object.setData(self, data == 'true')

class Int(Object):
	'''
	An object to holds a 32 bit signed int

	>>> Int(-1024).getType()
	'int'
	'''
	def __init__(self, data = None):
		Object.__init__(self, "int")
		self.__min = (-0x7fffffff-1)
		self.__max = (0x7fffffff)
		if data is not None:
			self.setData(data)

	def setData(self, data):
		'''
		Updates the value of an integer

		>>> i = Int(100)
		>>> i.setData(-200)
		>>> i.getData()
		-200
		'''
		data = int(data)
		if data < self.__min or data > self.__max:
			raise ValueError("Out of range")
		Object.setData(self, int(data))

class UnsignedInt(Object):
	'''
	An object to holds a 32 bit unsigned int

	>>> UnsignedInt(1024).getType()
	'unsigned int'
	'''
	def __init__(self, data = None):
		Object.__init__(self, "unsigned int")
		self.__min = 0
		self.__max = (0xffffffff)
		if data is not None:
			self.setData(data)

	def setData(self, data):
		'''
		Updates the value of an unsigned integer

		>>> u = UnsignedInt(100)
		>>> u.setData(200)
		>>> u.getData()
		200
		'''
		data = int(data)
		if data < self.__min or data > self.__max:
			raise ValueError("Out of range")
		Object.setData(self, int(data))

class String(Object):
	'''
	An object to holds a string of characters

	>>> String('Hello World').getType()
	'string'
	'''
	def __init__(self, data = None):
		Object.__init__(self, "string")
		if data is not None:
			self.setData(data)
		
	def setData(self, data):
		'''
		Updates the value of a string

		>>> s = String('Hello World')
		>>> s.setData('Goodbye cruel world')
		>>> s.getData()
		'Goodbye cruel world'
		'''
		Object.setData(self, str(data))

class Array(Object):
	'''
	An object containing other objects in an order.

	>>> Array().getType()
	'array'
	'''
	def __init__(self):
		Object.__init__(self, "array")
		self.__data = []

	def __str__(self):
		'''
		Returns an XML string containing the array objects

		>>> a = Array()
		>>> b = Bool(True)
		>>> a.append(b)
		>>> str(a)
		'<data type="array"><data type="bool">True</data></data>'
		'''
		s = "<data type=\"array\">"
		for data in self.__data:
			s += str(data)
		s += "</data>"
		return s

	def append(self, data):
		'''
		Appends the data to the end of the array.

		>>> a = Array()
		>>> b = Bool(True)
		>>> a.append(b)
		>>> i = Int(-10)
		>>> a.append(i)
		>>> a.getData()[0] is b
		True
		>>> a.getData()[1] is i
		True
		'''
		if not isinstance(data, Object):
			raise ValueError("Not a Faund Object")
		self.__data.append(data)
		data.setParent(self)

	def getData(self):
		'''
		Gets a Python array of the objects in the facund Array

		>>> a = Array()
		>>> u = UnsignedInt(300)
		>>> a.append(u)
		>>> ar = a.getData()
		>>> type(ar)
		<type 'list'>
		>>> ar[0] is u
		True
		'''
		return self.__data

	def setData(self, data):
		'''
		>>> a = Array()
		>>> try:
		... 	a.setData(None)
		... except ValueError, e:
		... 	print e
		Array's can't have data set
		'''
		raise ValueError("Array's can't have data set")

def _test():
	import doctest
	doctest.testmod()

if __name__ == "__main__":
	_test()
