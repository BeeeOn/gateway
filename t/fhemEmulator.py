#!/usr/bin/python
import argparse
from datetime import datetime
import logging
import random
import socket
import sys
import threading
import time

DEFAULT_IP = '127.0.0.1'
DEFAULT_SERIAL_NUMBER = "MEQ0106579"
MAX_MAC = 2**48 - 1
RANDOM_MAC = random.randint(1,MAX_MAC)
RANDOM_PORT = 7072

MAX_MESSAGE_SIZE = 1024

def getNowString():
		now = datetime.now()
		return now.strftime("%Y-%m-%d %H:%M:%S")

class TelnetMessage:
	def __init__(self, message):
		tokens = message.split(' ')

		self._command = tokens[0]
		self._parameters = tokens[1:]

	def getCommand(self):
		return self._command

	def getParameters(self):
		return self._parameters

	def __str__(self):
		delimitr = ' '

		return self._command + ' ' + delimitr.join(self._parameters)

class DeviceChannel:
	def __init__(self, fuuid, suffix, nr, value):
		self._fuuid = fuuid
		self._suffix = suffix
		self._nr = nr
		self._value = value

	def getFuuid(self):
		return self._fuuid

	def getSuffix(self):
		return self._suffix

	def getNr(self):
		return self._nr

	def getValue(self):
		return self._value

	def setValue(self, value):
		self._value = value

class VirtualHomeMaticDevice:
	def __init__(self, mac, serial):
		self._mac = mac
		self._serial = serial
		self._alive = True

		self._def = format(mac, '012x')[6:12].upper()
		self._devName = 'HM_' + self._def

	def getDeviceName(self):
		return self._devName

	def getAlive(self):
		return self._alive

	def setAlive(self, value):
		self._alive = value

class VirtualHomeMaticContact(VirtualHomeMaticDevice):
	def __init__(self, mac, serial):
		super().__init__(mac, serial)

	def processSetMessage(self, params):
		logging.info("Ignoring message (unknown set parameters)")

	def sendDeviceRecord(self, conn):
		response = '{'\
			           '"Arg":"' + self._devName + '",'\
			           '"Results": ['\
			           '{'\
			               '"Name":"' + self._devName + '",'\
			               '"PossibleSets":" ",'\
			               '"PossibleAttrs":" ",'\
			               '"Internals": {'\
			                   '"CFGFN": "",'\
			                   '"CUL_0_MSGCNT": "10",'\
			                   '"CUL_0_RAWMSG": "A0C19A64130B0BEF11034011000::-48:CUL_0",'\
			                   '"CUL_0_RSSI": "-48",'\
			                   '"CUL_0_TIME": "2020-02-26 14:20:04",'\
			                   '"DEF": "' + self._def + '",'\
			                   '"FUUID": "5e56704e-f33f-7c84-a149-f836132a664f8057",'\
			                   '"LASTInputDev": "CUL_0",'\
			                   '"MSGCNT": "10",'\
			                   '"NAME": "' + self._devName + '",'\
			                   '"NOTIFYDEV": "global",'\
			                   '"NR": "3004",'\
			                   '"NTFY_ORDER": "50-' + self._devName + '",'\
			                   '"STATE": "closed",'\
			                   '"TYPE": "CUL_HM",'\
			                   '"chanNo": "01",'\
			                   '"lastMsg": "No:19 - t:41 s:' + self._def + ' d:F11034 011000",'\
			                   '"protLastRcv": "' + getNowString() + '",'\
			                   '"protRcv": "11 last_at:2020-02-26 14:20:04",'\
			                   '"protSnd": "12 last_at:2020-02-26 14:20:04",'\
			                   '"protState": "CMDs_done",'\
			                   '"rssi_at_CUL_0": "cnt:11 min:-53.5 max:-48 avg:-51.86 lst:-48 "'\
			               '},'\
			               '"Readings": {'\
			                   '"Activity": { "Value":"alive", "Time":"2020-02-26 14:19:56" },'\
			                   '"CommandAccepted": { "Value":"yes", "Time":"2020-02-26 14:19:57" },'\
			                   '"D-firmware": { "Value":"2.4", "Time":"2020-02-26 14:19:56" },'\
			                   '"D-serialNr": { "Value":"' + self._serial + '", "Time":"2020-02-26 14:19:56" },'\
			                   '"PairedTo": { "Value":"0xF11034", "Time":"2020-02-26 14:19:57" },'\
			                   '"R-cyclicInfoMsg": { "Value":"off", "Time":"2020-02-26 14:19:57" },'\
			                   '"R-pairCentral": { "Value":"set_0xF11034", "Time":"2020-02-26 14:19:33" },'\
			                   '"alive": { "Value":"yes", "Time":"2020-02-26 14:20:03" },'\
			                   '"battery": { "Value":"ok", "Time":"2020-02-26 14:20:04" },'\
			                   '"contact": { "Value":"closed (to CUL_0)", "Time":"2020-02-26 14:20:04" },'\
			                   '"recentStateType": { "Value":"info", "Time":"2020-02-26 14:20:03" },'\
			                   '"sabotageError": { "Value":"off", "Time":"2020-02-26 14:20:03" },'\
			                   '"state": { "Value":"closed", "Time":"2020-02-26 14:20:04" }'\
			               '},'\
			               '"Attributes": {'\
			                   '"IODev": "CUL_0",'\
			                   '"actStatus": "alive",'\
			                   '"firmware": "2.4",'\
			                   '"model": "HM-SEC-SC-2",'\
			                   '"room": "CUL_HM",'\
			                   '"serialNr": "' + self._serial + '",'\
			                   '"subType": "threeStateSensor"'\
			               '}'\
			           '}  ],'\
			           '"totalResultsReturned":1'\
			       '}\n'

		conn.sendall(response.encode('utf-8'))

	def sendChannelRecord(self, conn, channelName):
		response = '{'\
			            '"Arg":"ActionDetector",'\
			            '"Results": [ ],'\
			            '"totalResultsReturned":0'\
			        '}\n'

		conn.sendall(response.encode('utf-8'))

class VirtualHomeMaticSwitch(VirtualHomeMaticDevice):
	def __init__(self, mac, serial):
		super().__init__(mac, serial)
		self.__initChannels()

	def __initChannels(self):
		fuuid = '5e561d90-f33f-7c84-49f4-341a1a69a18f076'
		self._channels = {
			(self._devName + '_Sw'): DeviceChannel(fuuid + '1', '_Sw', '1', 'off'),
			(self._devName + '_Pwr'): DeviceChannel(fuuid + '2', '_Pwr', '2', '0'),
			(self._devName + '_SenPwr'): DeviceChannel(fuuid + '3', '_SenPwr', '3', '0'),
			(self._devName + '_SenI'): DeviceChannel(fuuid + '4', '_SenI', '4', '0'),
			(self._devName + '_SenU'): DeviceChannel(fuuid + '5', '_SenU', '5', '240.5'),
			(self._devName + '_SenF'): DeviceChannel(fuuid + '6', '_SenF', '6', '50'),
		}

	def processSetMessage(self, params):
		if len(params) == 2 and params[0] == (self._devName + '_Sw') and params[1] == 'on':
			logging.info("Swithing device %s on" % self._devName)
			self._channels[self._devName + '_Sw'].setValue('on')
		elif len(params) == 2 and params[0] == (self._devName + '_Sw') and params[1] == 'off':
			logging.info("Swithing device %s off" % self._devName)
			self._channels[self._devName + '_Sw'].setValue('off')
		else:
			logging.info("Ignoring message (unknown set parameters)")

	def sendDeviceRecord(self, conn):
		response = '{'\
			           '"Arg":"' + self._devName + '",'\
			           '"Results": ['\
			           '{'\
			               '"Name":"' + self._devName + '",'\
			               '"PossibleSets":" ",'\
			               '"PossibleAttrs":" ",'\
			               '"Internals": {'\
			                   '"CFGFN": "",'\
			                   '"CUL_0_MSGCNT": "87",'\
			                   '"CUL_0_RAWMSG": "A0E20A01038D649F110340100000000::-29:CUL_0",'\
			                   '"CUL_0_RSSI": "-29",'\
			                   '"CUL_0_TIME": "2020-02-26 10:53:42",'\
			                   '"DEF": "' + self._def + '",'\
			                   '"FUUID": "5e561d90-f33f-7c84-49f4-341a1a69a18f0760",'\
			                   '"LASTInputDev": "CUL_0",'\
			                   '"MSGCNT": "87",'\
			                   '"NAME": "' + self._devName + '",'\
			                   '"NOTIFYDEV": "global",'\
			                   '"NR": "44",'\
			                   '"NTFY_ORDER": "50-' + self._devName + '",'\
			                   '"STATE": "CMDs_done",'\
			                   '"TYPE": "CUL_HM",'\
			                   '"channel_01": "' + self._devName + '_Sw",'\
			                   '"channel_02": "' + self._devName + '_Pwr",'\
			                   '"channel_03": "' + self._devName + '_SenPwr",'\
			                   '"channel_04": "' + self._devName + '_SenI",'\
			                   '"channel_05": "' + self._devName + '_SenU",'\
			                   '"channel_06": "' + self._devName + '_SenF",'\
			                   '"lastMsg": "No:20 - t:10 s:' + self._def + ' d:F11034 0100000000",'\
			                   '"protLastRcv": "' + getNowString() + '",'\
			                   '"protRcv": "88 last_at:2020-02-26 10:53:42",'\
			                   '"protSnd": "37 last_at:2020-02-26 10:53:42",'\
			                   '"protState": "CMDs_done",'\
			                   '"rssi_at_CUL_0": "cnt:88 min:-32 max:-27 avg:-29.47 lst:-29 "'\
			               '},'\
			               '"Readings": {'\
			                   '"Activity": { "Value":"alive", "Time":"2020-02-26 08:26:28" },'\
			                   '"CommandAccepted": { "Value":"yes", "Time":"2020-02-26 10:53:32" },'\
			                   '"D-firmware": { "Value":"1.6", "Time":"2020-02-26 08:26:28" },'\
			                   '"D-serialNr": { "Value":"' + self._serial + '", "Time":"2020-02-26 08:26:28" },'\
			                   '"PairedTo": { "Value":"0xF11034", "Time":"2020-02-26 10:53:36" },'\
			                   '"R-pairCentral": { "Value":"0xF11034", "Time":"2020-02-26 10:53:36" },'\
			                   '"state": { "Value":"CMDs_done", "Time":"2020-02-26 10:53:42" }'\
			               '},'\
			               '"Attributes": {'\
			                   '"IODev": "CUL_0",'\
			                   '"actStatus": "alive",'\
			                   '"firmware": "1.6",'\
			                   '"model": "HM-ES-PMSW1-PL",'\
			                   '"room": "CUL_HM",'\
			                   '"serialNr": "' + self._serial + '",'\
			                   '"subType": "powerMeter"'\
			               '}'\
			           '}  ],'\
			           '"totalResultsReturned":1'\
			       '}\n'

		conn.sendall(response.encode('utf-8'))

	def sendChannelRecord(self, conn, channelName):
		channel = self._channels[channelName]
		response = '{'\
			            '"Arg":"' + channelName + '",'\
			            '"Results": ['\
			            '{'\
			                '"Name":"' + channelName + '",'\
			                '"PossibleSets":" ",'\
			                '"PossibleAttrs":" ",'\
			                '"Internals": {'\
			                    '"CFGFN": "",'\
			                    '"DEF": "' + self._def + '",'\
			                    '"FUUID": "' + channel.getFuuid() + '",'\
			                    '"NAME": "' + channelName + '",'\
			                    '"NOTIFYDEV": "global",'\
			                    '"NR": "' + channel.getNr() + '",'\
			                    '"NTFY_ORDER": "50-' + channelName + '",'\
			                    '"STATE": "' + channel.getValue() + '",'\
			                    '"TYPE": "CUL_HM",'\
			                    '"chanNo": "0' + channel.getNr() + '",'\
			                    '"device": "' + self._devName + '"'\
			                '},'\
			                '"Readings": {'\
			                    '"state": { "Value":"' + channel.getValue() + '", "Time":"' + getNowString() + '" }'\
			                '},'\
			                '"Attributes": {'\
			                    '"model": "HM-ES-PMSW1-PL"'\
			                '}'\
			            '}  ],'\
			            '"totalResultsReturned":1'\
			        '}\n'

		conn.sendall(response.encode('utf-8'))

class VirtualFHEMServer(threading.Thread):
	def __init__(self, ip, port, device, mac, serial):
		self._ip = ip
		self._port = port

		if device == 'switch':
			self._device = VirtualHomeMaticSwitch(mac, serial)
			logging.info("FHEM is starting with HomeMatic Switch")
		elif device == 'contact':
			self._device = VirtualHomeMaticContact(mac, serial)
			logging.info("FHEM is starting with HomeMatic Magnetic Contact")
		else:
			logging.critical("Unknown device type FHEM could not start")
			exit(1)

		self._connectionThreads = []

		self._event = threading.Event()
		super().__init__()

	def run(self):
		self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self._socket.settimeout(2)
		self._socket.bind((self._ip, self._port))
		self._socket.listen()

		logging.info("Virtual FHEM server started")
		logging.info("Listening on %s:%u" % (self._ip, self._port))

		while not self._event.is_set():
			self._removeFinishedThreads()

			try:
				conn, address = self._socket.accept()
			except:
				continue

			if address[0] != 0: # accept() was interrupted
				thread = threading.Thread(target = self._processConnection, args = (conn, address))
				self._connectionThreads.append(thread)
				thread.start()

		logging.info("Virtual FHEM server successly stopped.")

	def stop(self):
		self._event.set()
		self._socket.close()

	def _removeFinishedThreads(self):
		finishedThreads = []

		for t in self._connectionThreads:
			if t.is_alive() == False:
				t.join(1)
				finishedThreads.append(t)

		for t in finishedThreads:
			self._connectionThreads.remove(t)

	def _processConnection(self, conn, address):
		logging.info("Accepted connection from client %s:%s" % (address[0], str(address[1])))

		while not self._event.is_set():
			data = conn.recv(MAX_MESSAGE_SIZE)

			# no data, connection closed
			if len(data) == 0:
				break
			# invalid first character
			elif data[0] < 32 or data[0] > 126:
				break

			message = TelnetMessage(data.decode('utf-8')[:-2])
			logging.info("Received message '%s' from %s:%s" % (message, address[0], str(address[1])))

			if message.getCommand() == 'exit':
				break
			elif message.getCommand() == "jsonlist2":
				self._processJsonList2(conn, message.getParameters())
			elif message.getCommand() == "set":
				self._processSet(conn, message.getParameters())
			elif message.getCommand() == "delete":
				self._processDelete(conn, message.getParameters())
			else:
				continue

		logging.info("Disconnect client %s:%s" % (address[0], str(address[1])))
		conn.close()

	def _processJsonList2(self, conn, params):
		if len(params) == 1 and params[0] == 'ActionDetector':
			self._sendActionDetector(conn)
		elif self._device.getAlive() and len(params) == 1 and params[0] == self._device.getDeviceName():
			self._device.sendDeviceRecord(conn)
		elif self._device.getAlive() and len(params) == 1 and params[0].startswith(self._device.getDeviceName()):
			self._device.sendChannelRecord(conn, params[0])
		else:
			self._sendEmpty(conn)

	def _processSet(self, conn, params):
		if len(params) == 3 and params[0] == 'CUL_0' and params[1] == 'hmPairForSec':
			logging.info("Searching for new devices for %s seconds" % params[2])
			self._device.setAlive(True)
		else:
			self._device.processSetMessage(params)

	def _processDelete(self, conn, params):
		if len(params) == 1 and params[0] == self._device.getDeviceName():
			logging.info("Deleting device %s" % self._device.getDeviceName())
			self._device.setAlive(False)
		else:
			logging.info("Ignoring message (deleting unknown device)")

	def _sendEmpty(self, conn):
		response = '{'\
			            '"Arg":"ActionDetector",'\
			            '"Results": [ ],'\
			            '"totalResultsReturned":0'\
			        '}\n'

		conn.sendall(response.encode('utf-8'))

	def _sendActionDetector(self, conn):
		response = '{'\
			            '"Arg":"ActionDetector",'\
			            '"Results": ['\
			            '{'\
			                '"Name":"ActionDetector",'\
			                '"PossibleSets":" ",'\
			                '"PossibleAttrs":" ",'\
			                '"Internals": {'\
			                    '"CFGFN": "",'\
			                    '"DEF": "000000",'\
			                    '"FUUID": "5e561d90-f33f-7c84-a52a-bf0114b98c579a6b",'\
			                    '"NAME": "ActionDetector",'\
			                    '"NOTIFYDEV": "global",'\
			                    '"NR": "51",'\
			                    '"NTFY_ORDER": "50-ActionDetector",'\
			                    '"STATE": "alive:1 dead:0 unkn:0 off:0",'\
			                    '"TYPE": "CUL_HM",'\
			                    '"chanNo": "01"'\
			                '},'\
			                '"Readings": {'

		if self._device.getAlive():
			response +=              '"state": { "Value":"alive:1 dead:0 unkn:0 off:0", "Time":"' + getNowString() + '" },'\
				                    '"status_' + self._device.getDeviceName() + '": { "Value":"alive", "Time":"' + getNowString() + '" }'
		else:
			response +=              '"state": { "Value":"alive:0 dead:0 unkn:0 off:0", "Time":"' + getNowString() + '" }'

		response +=          '},'\
			                '"Attributes": {'\
			                    '"event-on-change-reading": ".*",'\
			                    '"model": "ActionDetector"'\
			                '}'\
			            '}  ],'\
			            '"totalResultsReturned":1'\
			        '}\n'

		conn.sendall(response.encode('utf-8'))

if __name__ == "__main__":
	logging.basicConfig(level = logging.DEBUG)

	parser = argparse.ArgumentParser()
	parser.add_argument("--ip", metavar="IP_ADDRESS",
		help="The FHEM server will have this ip address.")
	parser.add_argument("--port", metavar="PORT",
		help="The FHRM server tries to bind on this port number. Has to be put as number.")
	parser.add_argument("--mac", metavar="MAC",
		help="The device will have this mac address. Has to be put as hex number.")
	parser.add_argument("--serial", metavar="SERIAL_NUMBER",
		help="The device will have this serial number.")

	group = parser.add_mutually_exclusive_group(required=True)
	group.add_argument("--switch", action='store_true',
		help="The FHEM server will takes care of HomeMatic switch.")
	group.add_argument("--contact", action='store_true',
		help="The FHEM server will takes care of HomeMatic magnetic contact.")
	args = parser.parse_args()

	ip = int(args.ip) if args.ip is not None else DEFAULT_IP
	port = int(args.port) if args.port is not None else RANDOM_PORT
	mac  = int(args.mac, 16) if args.mac is not None else RANDOM_MAC
	serial  = int(args.serial, 16) if args.serial is not None else DEFAULT_SERIAL_NUMBER
	if args.switch:
		device = 'switch'
	elif args.contact:
		device = 'contact'

	fhem = VirtualFHEMServer(ip, port, device, mac, serial)
	fhem.start()
	try:
		fhem.join()
	except KeyboardInterrupt:
		fhem.stop()
		fhem.join()
