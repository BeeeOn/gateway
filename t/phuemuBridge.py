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

USERNAME = 'YTV2PIPXrtrnHFLafGQlcVyrcxSgNo8wv-NQPmVk'
DEFAULT_IP = '127.0.0.1'
MAX_MAC = 2**48 - 1
RANDOM_MAC = random.randint(1,MAX_MAC)
RANDOM_PORT = random.randint(49152, 65535)

class PhiHueHandler(http.server.BaseHTTPRequestHandler):
	def do_POST(self):
		logging.info("client on address %s:%s connects" % (self.client_address[0], self.client_address[1]));

		return self.__processRequestWithBody("POST");

	def do_GET(self):
		logging.info("client on address %s:%s connects" % (self.client_address[0], self.client_address[1]));

		return self.__processRequestWithoutBody("GET");

	def do_PUT(self):
		logging.info("client on address %s:%s connects" % (self.client_address[0], self.client_address[1]));

		return self.__processRequestWithBody("PUT");

	def __processRequestWithBody(self, method):
		contentLen = int(self.headers['content-length'])
		body = self.rfile.read(contentLen).decode('utf-8')

		logging.debug(body);

		msg = self.server._device.createMsg(method, self.path, body)

		self.__prepareResponse(msg)

		return PhiHueHandler

	def __processRequestWithoutBody(self, method):
		msg = self.server._device.createMsg(method, self.path, "")

		self.__prepareResponse(msg)

		return PhiHueHandler

	def __prepareResponse(self, msg):
		self.send_response(200)
		self.send_header('content-type', 'application/json')
		self.send_header('content-type', 'charset="utf-8"')
		self.send_header('content-length', len(msg))
		self.end_headers()

		self.wfile.write(msg.encode())

class PhiHueServer(http.server.HTTPServer):
	def __init__(self, device, *args, **kw):
		http.server.HTTPServer.__init__(self, *args, **kw)
		self._device = device

class VirtualPhilipsHueDimmableBulb:
	def __init__(self, bulbID):
		self._state = "false"
		self._brightness = 255
		self._id = bulbID

	def modifyState(self, capability, value):
		if capability == "on":
			if value == "false" or value == "true":
				self._state = value
				return True
			else:
				return False
		elif capability == "bri":
			if 0 <= value and value <= 255:
				self._brightness = value
				return True
			else:
				return False
		else:
			return False

	def getState(self):
		return self._state

	def getBrightness(self):
		return self._brightness

	def getBulbID(self):
		return self._id

class VirtualPhilipsHueBridge:
	def __init__(self, ip, port, mac, bulbs):
		self._ip = ip
		self._port = port
		self._state = 0
		self._mac = mac
		self._bulbsID = []
		self._bulbs = dict()

		for i in range(1, bulbs+1):
			self._bulbs[i] = VirtualPhilipsHueDimmableBulb(mac + i)

	def start(self):
		try:
			self._httpServer = PhiHueServer(self, (self._ip, self._port), PhiHueHandler)
		except socket.error as eMsg:
			logging.exception(eMsg);
			exit(1)

		logging.info("Virtual Philips Hue Bridge started")
		logging.info("Listening on %s:%u as %s" % (self._ip, self._port, format(self._mac, '012x')))

		self.shareLocationByUPnP()

		self._httpServer.serve_forever()

	def __enter__(self):
		return self

	def __exit__(self, type, value, traceback):
		self._httpServer.socket.close()
		logging.info("Virtual Philips Hue Bridge successly stopped.")

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
		reDevice = re.search('ST: urn:schemas-upnp-org:device:basic:1', msg)
		if reDevice != None:
			return True
		else:
			return False

	def createUPnPMsg(self):
		return 'HTTP/1.1 200 OK\r\n'\
		       'HOST: 239.255.255.250:1900\r\n'\
		       'EXT: \r\n'\
		       'CACHE-CONTROL: max-age=100\r\n'\
		       'LOCATION: http://' + self._ip + ':' + str(self._port) + '/description.xml\r\n'\
		       'SERVER: Linux/3.14.0 UPnP/1.0 IpBridge/1.21.0\r\n'\
		       'hue-bridgeid: 001788FFFE291217\r\n'\
		       'ST: urn:schemas-upnp-org:device:basic:1\r\n'\
		       'USN: \r\n\r\n'

	def createMsg(self, method, path, body):
		if method == 'POST':
			logging.info("Request: POST")
			if re.match('/api/' + USERNAME + '/lights', path) != None:
				return self.createSearchResponseMsg()
			elif re.match('/api', path) != None:
				return self.createAuthorizeResponseMsg()
		elif method == 'PUT':
			logging.info("Request: PUT")
			if re.match('/api/' + USERNAME + '/lights/([0-9]+)', path) != None:
				return self.createModifyStateMsg(path, body)
		elif method == 'GET':
			logging.info("Request: GET")
			if re.match('/api/.*/config', path) != None:
				return self.createGetDeviceInfoMsg();
			elif re.match('/api/' + USERNAME + '/lights/[0-9]', path) != None:
				return self.createGetStateMsg(path);
			elif re.match('/api/' + USERNAME + '/lights', path) != None:
				return self.createGetDevicesMsg();

		return '[{'\
		           '"error": {'\
		               '"type": 3,'\
		               '"address": "' + path + '",'\
		               '"description": "resource ' + path + ' not available"'\
		           '}'\
		       '}]'

	def createGetStateMsg(self, path):
		reOrdNum = re.search('/api/' + USERNAME + '/lights/([0-9]+)', path)
		ordNum = int(reOrdNum.group(1), 10)

		result = '{'\
		             '"state": {'\
		                 '"on": ' + self._bulbs[ordNum].getState() + ','\
		                 '"bri": ' + str(self._bulbs[ordNum].getBrightness()) + ','\
		                 '"alert": "none",'\
		                 '"reachable": true'\
		             '},'\
		             '"swupdate": {'\
		                 '"state": "readytoinstall",'\
		                 '"lastinstall": null'\
		             '},'\
		             '"type": "Dimmable light",'\
		             '"name": "Hue white lamp ' + str(ordNum) + '",'\
		             '"modelid": "LWB006",'\
		             '"manufacturername": "Philips",'\
		             '"uniqueid": "' + format(self._bulbs[ordNum].getBulbID(), '016x') + '-0b",'\
		             '"swversion": "5.38.1.15095"'\
		         '}'

		return result

	def createGetDevicesMsg(self):
		result = '{'

		for ordNum in self._bulbs.keys():
			strBulbID = format(self._bulbs[ordNum].getBulbID(), '016x')
			strBulbID = ':'.join(strBulbID[i:i + 2] for i in range(0, 16, 2))

			result += '"' + str(ordNum) + '" : {'\
			              '"state": {'\
			                  '"on": ' + self._bulbs[ordNum].getState() + ','\
			                  '"bri": ' + str(self._bulbs[ordNum].getBrightness()) + ','\
			                  '"alert": "none",'\
			                  '"reachable": true'\
			              '},'\
			              '"swupdate": {'\
			                  '"state": "readytoinstall",'\
			                  '"lastinstall": null'\
			              '},'\
			              '"type": "Dimmable light",'\
			              '"name": "Hue white lamp ' + str(ordNum) + '",'\
			              '"modelid": "LWB006",'\
			              '"manufacturername": "Philips",'\
			              '"uniqueid": "' + strBulbID + '-0b",'\
			              '"swversion": "5.38.1.15095"'\
			          '}'

			if ordNum != len(self._bulbs):
				result += ','

		result += '}'

		return result

	def createModifyStateMsg(self, path, body):
		reOrdNum = re.search('/api/' + USERNAME + '/lights/([0-9]+)/state', path)
		ordNum = int(reOrdNum.group(1), 10)

		reCapability = re.search('{"([a-z]+)" ?: ?([a-z0-9]+)}', body)
		capability = reCapability.group(1)
		value = reCapability.group(2)

		success = False
		if capability == "bri":
			success = self._bulbs[ordNum].modifyState("bri", int(value))
		elif capability == "on":
			success = self._bulbs[ordNum].modifyState("on", value)

		if success:
			return '[{'\
			           '"success": {'\
			               '"/lights/' + str(ordNum) + '/state/' + capability + '": ' + value + ''\
			           '}'\
			       '}]'
		else:
			return '[{'\
			           '"error": {'\
			               '"type": 7,'\
			               '"address": "/lights/1/state' + capability + '",'\
			               '"description": "failed to modify state"'\
			           '}'\
			       '}]'

	def createGetDeviceInfoMsg(self):
		strMac = format(self._mac, '012x')
		strMac = ':'.join(strMac[i:i + 2] for i in range(0, 12, 2))

		result = '{'\
		             '"name": "Philips hue",'\
		             '"datastoreversion": "63",'\
		             '"swversion": "1709131301",'\
		             '"apiversion": "1.21.0",'\
		             '"mac": "' + strMac + '",'\
		             '"bridgeid": "' + format(self._mac, '016x') + '",'\
		             '"factorynew": false,'\
		             '"replacesbridgeid": null,'\
		             '"modelid": "BSB002",'\
		             '"starterkitid": ""'\
		         '}'

		return result

	def createAuthorizeResponseMsg(self):
		result = '[{'\
		             '"success": {'\
		                 '"username": "' + USERNAME + '"'\
		             '}'\
		         '}]'

		return result

	def createSearchResponseMsg(self):
		result = '[{'\
		             '"success": {'\
		                 '"/lights": "Searching for new devices"'\
		             '}'\
		         '}]'

		return result

if __name__ == "__main__":
	logging.basicConfig(level = logging.DEBUG)

	parser = argparse.ArgumentParser()
	parser.add_argument("--bulbs", metavar="COUNT_OF_BULBS",
		help="The bridge will take care of this count of bulbs.")
	parser.add_argument("--ip", metavar="IP_ADDRESS",
		help="The bridge will have this ip address.")
	parser.add_argument("--port", metavar="PORT",
		help="The bridge tries to bind on this port number. Has to be put as number.")
	parser.add_argument("--mac", metavar="MAC",
		help="The bridge will have this mac address. Has to be put as hex number.")
	args = parser.parse_args()

	bulbs = int(args.bulbs) if args.bulbs is not None else 1
	ip = int(args.ip) if args.ip is not None else DEFAULT_IP
	port = int(args.port) if args.port is not None else RANDOM_PORT
	mac  = int(args.mac, 16) if args.mac is not None else RANDOM_MAC

	with VirtualPhilipsHueBridge(ip, port, mac, bulbs) as bridge:
		bridge.start()
