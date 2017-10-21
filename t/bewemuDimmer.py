#!/usr/bin/python
import argparse
import http.server
import logging
import re
import random
import socket
import struct
import sys
import time

DEFAULT_IP = '127.0.0.1'
DEFAULT_SERIAL_NUMBER = "221618K1100220"
MAX_MAC = 2**48 - 1
RANDOM_MAC = random.randint(1,MAX_MAC)
RANDOM_PORT = random.randint(49152, 65535)

class BeWemoHandler(http.server.BaseHTTPRequestHandler):
	def do_POST(self):
		logging.info("client on address %s:%s connects" % (self.client_address[0], self.client_address[1]));

		soapAction = self.headers['soapaction']
		contentLen = int(self.headers['content-length'])
		reAction = re.search('#([a-zA-Z]+)"', soapAction)
		action = reAction.group(1)
		postBody = self.rfile.read(contentLen).decode('utf-8')

		logging.debug(postBody);

		msg = self.server._device.createMsg(action, postBody)

		self.send_response(200)
		self.send_header('content-type', 'text/xml')
		self.send_header('content-type', 'charset="utf-8"')
		self.send_header('content-length', len(msg))
		self.end_headers()

		self.wfile.write(msg.encode())

		return BeWemoHandler

class BeWemoServer(http.server.HTTPServer):
	def __init__(self, device, *args, **kw):
		http.server.HTTPServer.__init__(self, *args, **kw)
		self._device = device

class VirtualBelkinWemoDimmer:
	def __init__(self, ip, port, mac, serial):
		self._ip = ip
		self._port = port
		self._state = 0
		self._brightness = 100
		self._mac = mac
		self._serial = serial

	def start(self):
		try:
			self._httpServer = BeWemoServer(self, (self._ip, self._port), BeWemoHandler)
		except socket.error as eMsg:
			logging.exception(eMsg);
			exit(1)

		logging.info("Virtual Belkin WeMo Dimmer started")
		logging.info("Listening on %s:%u as %s" % (self._ip, self._port, format(self._mac, '012x')))

		self.shareLocationByUPnP()

		self._httpServer.serve_forever()

	def __enter__(self):
		return self

	def __exit__(self, type, value, traceback):
		self._httpServer.socket.close()
		logging.info("Virtual Belkin WeMo Dimmer successly stopped.")

	def shareLocationByUPnP(self):
		multicastGroup = '239.255.255.250'
		serverAddress = ('', 1900)

		logging.info("Waiting for UPnP discover")

		multicastSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		multicastSocket.bind(serverAddress)

		group = socket.inet_aton(multicastGroup)
		mreq = struct.pack('4sL', group, socket.INADDR_ANY)
		multicastSocket.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

		while True:
			data, address = multicastSocket.recvfrom(1024)
			data = data.decode('utf-8')
			if self.isUPnPValid(data) == True:
				multicastSocket.sendto(self.createUPnPMsg().encode(), address)
				multicastSocket.close()
				break

		logging.info("UPnP served")

	def isUPnPValid(self, msg):
		reSwitchDevice = re.search('ST: urn:Belkin:device:dimmer:1', msg)
		if reSwitchDevice != None:
			return True
		else:
			return False

	def createUPnPMsg(self):
		return 'HTTP/1.1 200 OK\r\n'\
		       'CACHE-CONTROL: max-age=86400\r\n'\
		       'DATE: ' + time.strftime("%a, %d %b %Y %H:%M:%S %Z", time.localtime()) + '\r\n'\
		       'EXT: \r\n'\
		       'LOCATION: http://' + self._ip + ':' + str(self._port) + '/setup.xml\r\n'\
		       'OPT: "http://schemas.upnp.org/upnp/1/0/"; ns=01\r\n'\
		       '01-NLS: 11111111-2222-2222-2222-3333333333\r\n'\
		       'SERVER: Unspecified, UP\r\n\r\n'

	def createMsg(self, action, body):
		if action == 'GetBinaryState':
			logging.info("Request: GetBinaryState")
			return self.createGetStateMsg()
		elif action == 'SetBinaryState':
			logging.info("Request: SetBinaryState")
			return self.createSetStateMsg(body)
		elif action == 'GetMacAddr':
			logging.info("Request: GetMacAddr")
			return self.createGetMacAddrMsg()

	def createGetStateMsg(self):
		result = '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">'\
		         '<s:Body>'\
		         '<u:GetBinaryStateResponse xmlns:u="urn:Belkin:service:basicevent:1">'\
		         '<BinaryState>' + str(self._state) + '</BinaryState>'\
		         '<brightness>' + str(self._brightness) + '</brightness>'\
		         '</u:GetBinaryStateResponse>'\
		         '</s:Body>'\
		         '</s:Envelope>'
		return result

	def createSetStateMsg(self, body):
		reSet = re.search('<(BinaryState|brightness)>([0-9]+)<\/(BinaryState|brightness)>', body);
		setElement = reSet.group(1)
		value = int(reSet.group(2), 0)

		if setElement == 'BinaryState':
			element = self.setBinaryStateMsg(value)
		else:
			element = self.setBrightnessMsg(value)

		result = '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">'\
		         '<s:Body>'\
		         '<u:SetBinaryStateResponse xmlns:u="urn:Belkin:service:basicevent:1">'\
		         + element +\
		         '</u:SetBinaryStateResponse>'\
		         '</s:Body>'\
		         '</s:Envelope>'
		return result

	def setBinaryStateMsg(self, state):
		if state == 1:
			if self._state == 1:
				state = "error"
			else:
				self._state = 1
		else:
			if self._state == 0:
				state = "error"
			else:
				self._state = 0

		return '<BinaryState>' + str(state) + '</BinaryState>'

	def setBrightnessMsg(self, value):
		if 0 <= value and value <= 100:
			self._brightness = value

		return '<Brightness>' + str(self._brightness) + '</Brightness>'

	def createGetMacAddrMsg(self):
		result = '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">'\
		         '<s:Body>'\
		         '<u:GetMacAddrResponse xmlns:u="urn:Belkin:service:basicevent:1">'\
		         '<MacAddr>' + format(self._mac, '02x') + '</MacAddr>'\
		         '<SerialNo>' + self._serial + '</SerialNo>'\
		         '<PluginUDN>uuid:Socket-1_0-' + self._serial + '</PluginUDN>'\
		         '</u:GetMacAddrResponse>'\
		         '</s:Body>'\
		         '</s:Envelope>'

		return result


if __name__ == "__main__":
	logging.basicConfig(level = logging.DEBUG)

	parser = argparse.ArgumentParser()
	parser.add_argument("--ip", metavar="IP_ADDRESS",
		help="The dimmer will have this ip address.")
	parser.add_argument("--port", metavar="PORT",
		help="The dimmer tries to bind on this port number. Has to be put as number.")
	parser.add_argument("--mac", metavar="MAC",
		help="The dimmer will have this mac address. Has to be put as hex number.")
	parser.add_argument("--serial", metavar="SERIAL_NUMBER",
		help="The dimmer will have this serial number.")
	args = parser.parse_args()

	ip = int(args.ip) if args.ip is not None else DEFAULT_IP
	port = int(args.port) if args.port is not None else RANDOM_PORT
	mac  = int(args.mac, 16) if args.mac is not None else RANDOM_MAC
	serial  = int(args.serial, 16) if args.serial is not None else DEFAULT_SERIAL_NUMBER

	with VirtualBelkinWemoDimmer(ip, port, mac, serial) as switch:
		switch.start()
