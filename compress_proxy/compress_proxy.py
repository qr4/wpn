#!/bin/env python2

import sys
from time import sleep
import asyncore
import socket

class ClientHandler(asyncore.dispatcher_with_send):
	def __init__(self, host, port):
		asyncore.dispatcher_with_send.__init__(self)
		self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
		self.connect( (host, port) )
		self.clients = []

	def handle_read(self):
		"""new data from the server, broadcast to all clients"""
		data = self.recv(8192)
		if data:
			to_remove = []
			for client in self.clients:
				try:
					client.sendall(data)
				except:
					to_remove.append(client)

			for broken in to_remove:
				print 'connection closed'
				self.clients.remove(broken)

	def handle_close(self):
		self.close_all()
		self.close()
		raise
		
	def handle_error(self):
		self.handle_close()
		raise

	def add_client(self, sock):
		sock.setblocking(0)
		self.clients.append(sock)

	def close_all(self):
		for client in self.clients:
			client.shutdown(socket.SHUT_RDWR)
			client.close()
		self.clients = []

	def writeable():
		return False

class CompressServer(asyncore.dispatcher):
	def __init__(self, host, port, wpnhost, wpnport):
		asyncore.dispatcher.__init__(self)
		self.client_handler = ClientHandler(wpnhost, wpnport)
		self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
		self.set_reuse_addr()
		self.bind((host, port))
		self.listen(5)

	def handle_accept(self):
		pair = self.accept()
		if pair is None:
			pass
		else:
			sock, addr = pair
			print 'Incoming connection from %s' % repr(addr)
			self.client_handler.add_client(sock)

	def handle_error(self):
		self.close()
		raise

	def writeable():
		return False

def print_usage():
	print "USAGE: %s bindip:port foreignip:port" % (sys.argv[0])

def main():
	try:
		bindaddr,    bindport    = sys.argv[1].split(":")
		foreignaddr, foreignport = sys.argv[2].split(":")
		bindport    = int(bindport)
		foreignport = int(foreignport)
	except:
		print_usage()
		exit(1)

	retry = True
	while retry:
		try:
			server = CompressServer(bindaddr, bindport, foreignaddr, foreignport)
			asyncore.loop(use_poll = True)
		except (socket.gaierror, socket.herror, socket.error) as (error, string):
			print "%s (%s)" % (string, error)
		except socket.timeout:
			print "Connection timed out"

		server.close()
		del server

		sleep(3)
	
if __name__ == '__main__':
	main()
