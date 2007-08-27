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

import gconf

class ComputerList:
	def __init__(self):
		self.client = gconf.client_get_default()

	def getComputerList(self):
		list = self.client.get_list('/facund/computers', gconf.VALUE_STRING)
		computers = []
		for i in range(len(list)):
			key = list[i].replace(' ', '_')
			server = self.client.get_string(
			    '/facund/computer/%s/server' % key)
			socket = self.client.get_string(
			    '/facund/computer/%s/socket' % key) or '/tmp/facund'
			computers.append((list[i], server, socket))
		return computers

	def addComputer(self, computer):
		list = self.client.get_list('/facund/computers', gconf.VALUE_STRING)
		if computer.getName() in list:
			# The Update the item to change it's server and socket
			pass
		else:
			# Add the item to gconf
			key = computer.getName().replace(' ', '_')
			host = computer.getHost()
			if host is not None:
				self.client.set_string(
				    '/facund/computer/%s/server' % key, host)

			socket = computer.getSocket()
			if socket is not None:
				self.client.set_string(
				    '/facund/computer/%s/socket' % key, socket)

			list.append(computer.getName())
			self.client.set_list('/facund/computers',
			    gconf.VALUE_STRING, list)

	def delComputer(self, computer):
		key = computer.getName().replace(' ', '_')
		self.client.unset('/facund/computer/%s' % key)

		list = self.client.get_list('/facund/computers',
		    gconf.VALUE_STRING)

		list.remove(computer.getName())
		self.client.set_list('/facund/computers', gconf.VALUE_STRING,
		    list)
