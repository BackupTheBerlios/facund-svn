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

import threading

class Call:
	def __init__(self, name, args):
		self.__name = name
		self.__args = args
		self.__response = None

		self.__responseLock = threading.Lock()

	def getCall(self):
		# TODO: Use a better call ID
		return "<call id=\"1\" name=\"%s\">%s</call>" \
		    % (self.__name, self.__args)

	def getID(self):
		return 1

	def setResponse(self, response):
		self.__response = response

	def getResponse(self):
		return self.__response

	def acquireLock(self):
		'''Aquires the lock to use to set the response.'''
		self.__responseLock.acquire()

	def releaseLock(self):
		'''Releases the lock to use and set the response.'''
		self.__responseLock.release()

class Response:
	'''
	A response is the data returned from a remote server when
	issued a call.
	'''
	def __init__(self, id, code, message, data = None):
		self.__id = id
		self.__code = code
		self.__message = message
		self.__data = data

	def __str__(self):
		'''
		Returns the XML string that can be sent to a client after
		a call.

		>>> import data
		>>> b = data.Bool(False)
		>>> str(Response(1, 0, 'Ok', b))
		'<response id="1" message="Ok" code="0"><data type="bool">False</data></response>'
		'''
		return "<response id=\"%s\" message=\"%s\" code=\"%s\">" \
		    "%s</response>" \
		    % (self.__id, self.__message, self.__code, 
		       str(self.__data or ''))

	def getData(self):
		'''
		Gets the data object returned by the call. The data will
		be a facund object when the response is from facund server.

		>>> import data
		>>> b = data.Bool(True)
		>>> r = Response(1, 0, 'Ok', b)
		>>> r.getData() is b
		True
		'''

		return self.__data

	def getCode(self):
		'''
		Returns the return code. It will be zero for a valid call
		or non zero on failure.

		>>> Response(1, 0, 'Ok').getCode()
		0

		>>> Response(2, 50, 'Failure').getCode()
		50
		'''
		return self.__code

def _test():
	import doctest
	doctest.testmod()

if __name__ == "__main__":
	_test()
