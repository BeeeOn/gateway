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
from xml.sax.saxutils import escape

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

class VirtualBelkinWemoBulb:
	def __init__(self, bulbID):
		self._state = 0
		self._brightness = 255
		self._id = bulbID

	def modifyState(self, capability, value):
		if capability == 10006:
			if value == 0 or value == 1:
				self._state = value
				return True
			else:
				return False
		elif capability == 10008:
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

class VirtualBelkinWemoLink:
	def __init__(self, ip, port, mac, serial, bulbs):
		self._ip = ip
		self._port = port
		self._state = 0
		self._mac = mac
		self._serial = serial
		self._bulbsID = []
		self._bulbs = dict()

		for i in range(bulbs):
			self._bulbs[mac + i] = VirtualBelkinWemoBulb(mac + i)

	def start(self):
		try:
			self._httpServer = BeWemoServer(self, (self._ip, self._port), BeWemoHandler)
		except socket.error as eMsg:
			logging.exception(eMsg);
			exit(1)

		logging.info("Virtual Belkin WeMo Link started")
		logging.info("Listening on %s:%u as %s" % (self._ip, self._port, format(self._mac, '012x')))

		self.shareLocationByUPnP()

		self._httpServer.serve_forever()

	def __enter__(self):
		return self

	def __exit__(self, type, value, traceback):
		self._httpServer.socket.close()
		logging.info("Virtual Belkin WeMo Link successly stopped.")

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
		reSwitchDevice = re.search('ST: urn:Belkin:device:bridge:1', msg)
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
		if action == 'GetDeviceStatus':
			logging.info("Request: GetDeviceStatus")
			return self.createGetDeviceStatusMsg(body)
		elif action == 'SetDeviceStatus':
			logging.info("Request: SetDeviceStatus")
			return self.createSetDeviceStatusMsg(body)
		elif action == 'GetEndDevices':
			logging.info("Request: GetEndDevices")
			return self.createGetEndDevicesMsg()
		elif action == 'GetMacAddr':
			logging.info("Request: GetMacAddr")
			return self.createGetMacAddrMsg()

	def createGetDeviceStatusMsg(self, body):
		reBulbID = re.search('<DeviceIDs>([a-zA-Z0-9]+)</DeviceIDs>', body)
		bulbID = int(reBulbID.group(1), 16)

		result = '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">'\
		         '<s:Body>'\
		         '<u:GetDeviceStatusResponse xmlns:u="urn:Belkin:service:bridge:1">'\
		         '<DeviceStatusList>'

		result += escape(
		            '<?xml version="1.0" encoding="utf-8"?>'\
		            '<DeviceStatusList>'\
		            '<DeviceStatus>'\
		            '<IsGroupAction>NO</IsGroupAction>'\
		            '<DeviceID available="YES">' + format(bulbID, '016x') + '</DeviceID>'\
		            '<CapabilityID>10006,10008,30008,30009,3000A</CapabilityID>'\
		            '<CapabilityValue>'\
		            + str(self._bulbs[bulbID].getState()) + ',' + str(self._bulbs[bulbID].getBrightness()) + ':0,,,'\
		            '</CapabilityValue>'\
		            '</DeviceStatus>'\
		            '</DeviceStatusList>',
		          {'"' : '&quot;'})

		result += '</DeviceStatusList>'\
		          '</u:GetDeviceStatusResponse>'\
		          '</s:Body>'\
		          '</s:Envelope>'

		return result

	def createSetDeviceStatusMsg(self, body):
		reBulbID = re.search('&lt;DeviceID available=&quot;YES&quot;&gt;([a-fA-F0-9]+)&lt;/DeviceID&gt;', body)
		reCapability = re.search('&lt;CapabilityID&gt;([0-9]+)&lt;/CapabilityID&gt;', body)
		reValue = re.search('&lt;CapabilityValue&gt;([0-9]+):{0,1}[0-9]*&lt;/CapabilityValue&gt;', body)

		bulbID = int(reBulbID.group(1), 16)
		capability = int(reCapability.group(1), 0)
		value = int(reValue.group(1), 0)

		if self._bulbs[bulbID].modifyState(capability, value):
			errorBulbID = ""
		else:
			errorBulbID = format(bulbID, '016x')

		result = '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">'\
		         '<s:Body>'\
		         '<u:SetDeviceStatusResponse xmlns:u="urn:Belkin:service:bridge:1">'\
		         '<ErrorDeviceIDs>' + errorBulbID + '</ErrorDeviceIDs>'\
		         '</u:SetDeviceStatusResponse>'\
		         '</s:Body>'\
		         '</s:Envelope>'\

		return result

	def createGetEndDevicesMsg(self):
		result = '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">'\
		         '<s:Body>'\
		         '<u:GetEndDevicesResponse xmlns:u="urn:Belkin:service:bridge:1">'\
		         '<DeviceLists>'

		result += escape(
		            '<?xml version="1.0" encoding="utf-8"?>'\
		            '<DeviceLists>'\
		            '<DeviceList>'\
		            '<DeviceListType>Paired</DeviceListType>'\
		            '<DeviceInfos>',
		          {'"' : '&quot;'})

		for bulbID in self._bulbs.keys():
			result += escape(
			            '<DeviceInfo>'\
			            '<DeviceIndex>0</DeviceIndex>'\
			            '<DeviceID>' + format(bulbID, '016x') + '</DeviceID>'\
			            '<FriendlyName>Lightbulb</FriendlyName>'\
			            '<IconVersion>1</IconVersion>'\
			            '<FirmwareVersion>7E</FirmwareVersion>'\
			            '<CapabilityIDs>10006,10008,30008,30009,3000A</CapabilityIDs>'\
			            '<CurrentState>,,,,</CurrentState>'\
			            '</DeviceInfo>',
			          {'"' : '&quot;'})

		result += escape(
		            '</DeviceInfos>'\
		            '</DeviceList>'\
		            '</DeviceLists>',
		          {'"' : '&quot;'})

		result += '</DeviceLists>'\
		          '</u:GetEndDevicesResponse>'\
		          '</s:Body>'\
		          '</s:Envelope>'

		return result

	def createGetMacAddrMsg(self):
		result = '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">'\
		         '<s:Body>'\
		         '<u:GetMacAddrResponse xmlns:u="urn:Belkin:service:basicevent:1">'\
		         '<MacAddr>' + format(self._mac, '02x') + '</MacAddr>'\
		         '<SerialNo>' + self._serial + '</SerialNo>'\
		         '<PluginUDN>uuid:Bridge-1_0-' + self._serial + '</PluginUDN>'\
		         '</u:GetMacAddrResponse>'\
		         '</s:Body>'\
		         '</s:Envelope>'

		return result


if __name__ == "__main__":
	logging.basicConfig(level = logging.DEBUG)

	parser = argparse.ArgumentParser()
	parser.add_argument("--bulbs", metavar="COUNT_OF_BULBS",
		help="The link will take care of this count of bulbs.")
	parser.add_argument("--ip", metavar="IP_ADDRESS",
		help="The link will have this ip address.")
	parser.add_argument("--port", metavar="PORT",
		help="The link tries to bind on this port number. Has to be put as number.")
	parser.add_argument("--mac", metavar="MAC",
		help="The link will have this mac address. Has to be put as hex number.")
	parser.add_argument("--serial", metavar="SERIAL_NUMBER",
		help="The lnik will have this serial number.")
	args = parser.parse_args()

	bulbs = int(args.bulbs) if args.bulbs is not None else 1
	ip = int(args.ip) if args.ip is not None else DEFAULT_IP
	port = int(args.port) if args.port is not None else RANDOM_PORT
	mac  = int(args.mac, 16) if args.mac is not None else RANDOM_MAC
	serial  = int(args.serial, 16) if args.serial is not None else DEFAULT_SERIAL_NUMBER

	with VirtualBelkinWemoLink(ip, port, mac, serial, bulbs) as link:
		link.start()
