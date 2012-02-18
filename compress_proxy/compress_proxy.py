#!/bin/env python2

import sys
from time import sleep
import asyncore
import socket
from subprocess import PIPE, Popen

class BufferFull(Exception):
	def __init__(self, value):
		self.value = value
	def __str__(self):
		return repr(self.value)

class ClientOutput(asyncore.dispatcher):
	def __init__(self, sock):
		asyncore.dispatcher.__init__(self, sock = sock)
		self.queue = []
		self.buffered = 0
		self.max_buffered = 1 * 1024 * 1024
		self.write = False

	def enqueue(self, data):
		#print "enqueing %d bytes, buffered %d" % (len(data), self.buffered)
		new_size = self.buffered + len(data)
		if new_size > self.max_buffered:
			self.close()
			raise BufferFull(new_size)

		self.queue.append(data)
		self.buffered = new_size
		self.write = True

	def handle_close(self):
		# this is dirty, come up with a better idea
		self.queue = []
		self.buffered = 0
		self.max_buffered = -1
		self.write = False

	def handle_write(self):
		while True:
			if self.buffered == 0:
				self.write = False
				return
			packet = self.queue.pop(0)
			sent = self.send(packet)
			packet = packet[sent:]
			self.buffered -= sent

			if len(packet) > 0:
				self.queue = [packet] + self.queue
				break

	def readable(self):
		return False

	def writable(self):
		return self.write

class WorldSplitter(list):
	def __init__(self):
		self.json_depth = 0
		self.temp_buff = []

	def parse(self, data):
		for c in data:
			if self.json_depth == 0:
				if self.temp_buff:
					t = "".join(self.temp_buff)
					if not t.isspace():
						self.append(t)
						self.temp_buff = []
			if c == '{':
				self.json_depth += 1
			elif c == '}':
				self.json_depth -= 1 if self.json_depth != 0 else 0
			self.temp_buff.append(str(c))

def compress(data):
	xz = Popen(['xz', '-z', '-0e', '-c'], stdin = PIPE, stdout = PIPE)
	compressed, stderrout = xz.communicate(data)
	return compressed

class ClientHandler(asyncore.dispatcher):
	def __init__(self, host, port):
		asyncore.dispatcher.__init__(self)
		self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
		self.connect( (host, port) )
		self.clients = []
		self.worldsplitter = WorldSplitter()
		self.worldcounter = 0
		self.compressed = 0.0
		self.uncompressed = 0.0

	def handle_read(self):
		"""new data from the server, broadcast to all clients"""

		data = self.recv(8192)
		if data:
			self.worldsplitter.parse(data)

		if self.worldsplitter:
			t = "".join(self.worldsplitter)
			while self.worldsplitter: self.worldsplitter.pop()
			data = compress(t)
			self.compressed += len(data)
			self.uncompressed += len(t)
			self.worldcounter += 1
			to_remove = []
			print "broadcast %8d: %8d * %4d = %8d bytes (ratio: %.4f)\n" % \
					(self.worldcounter, len(data), len(self.clients), len(data) * len(self.clients), self.compressed / self.uncompressed)
			for client in self.clients:
				try:
					client.enqueue(data)
				except BufferFull as e:
					print 'Removing slow client: %s bytes in Buffer' % (e)
					to_remove.append(client)

			for broken in to_remove:
				print 'connection closed'
				self.clients.remove(broken)

	def handle_close(self):
		self.close_all()
		self.close()
		
	def handle_error(self):
		self.handle_close()
		raise

	def add_client(self, sock):
		sock.setblocking(0)
		self.clients.append(ClientOutput(sock))

	def close_all(self):
		for client in self.clients:
			client.shutdown(socket.SHUT_RDWR)
			client.close()
		self.clients = []

	def writable(self):
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
		self.client_handler.close()
		self.close()
		raise

	def writable(self):
		return False

def print_usage():
	print "USAGE: %s bindip:port foreignip:port" % (sys.argv[0])

def main():
	try:
		bindaddr,    bindport    = sys.argv[1].rsplit(":", 1)
		foreignaddr, foreignport = sys.argv[2].rsplit(":", 1)
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
