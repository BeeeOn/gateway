#!/bin/python3

from os import sys, path
sys.path.append(path.dirname(path.dirname(path.dirname(path.abspath(__file__)))))

import globalConfig

import GatewayBaseModule
import VdevModule
import SensorData
import GWServerModule
import FakeGWServer
import time
import json

vdevIni="""
[virtual-devices]
request.device_list = no

[virtual-device0]
enable = yes
paired = yes
refresh = 2
device_id = 0xa300000000000001
vendor = BeeeOn
product = Temperature Sensor
module0.type = temperature, inner
module0.generator = 5
"""

"""
Unpairing the device from the system and asserting it is no longer
exporting any data.
"""
class TestUnpairingSingle(VdevModule.VdevModule, GWServerModule.GWServerModule, GatewayBaseModule.GatewayBaseModule):

	def setUp(self):
		self.vdevConfig = vdevIni
		self.gwsConfig['autoRegister'] = True
		self.gwsConfig['autoData'] = True
		self.gwsConfig['autoResponse'] = True
		self.gwsConfig['autoNewDevice'] = True
		super().setUp()

	def test_unpairDevice(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())
		sensorData = self.GWS.nextData()
		self.assertEqual(sensorData.deviceID, "0xa300000000000001")
		self.assertEqual(len(sensorData.data), 1)
		self.assertEqual(sensorData.data[0][0], 0)
		self.assertAlmostEqual(sensorData.data[0][1], 5)

		commander = FakeGWServer.Commander(self.GWS)
		self.assertEqual(commander.unpair("0xa300000000000001"),1)

		# data could be still cached in Distributor, lets wait for it to flush
		self.assertFalse(self.GWS.waitForDisconnect(5))

		self.GWS.clearData()
		self.assertEqual(self.GWS.nextData(), None)

vdevIniMultiple="""
[virtual-devices]
request.device_list = no

[virtual-device0]
enable = yes
paired = yes
refresh = 2
device_id = 0xa300000000000001
vendor = BeeeOn
product = Temperature Sensor
module0.type = temperature, inner
module0.generator = 5

[virtual-device1]
enable = yes
paired = yes
refresh = 2
device_id = 0xa300000000000002
vendor = BeeeOn
product = Temperature Sensor
module0.type = temperature, inner
module0.generator = 10
"""

class TestUnpairingMultiple(VdevModule.VdevModule, GWServerModule.GWServerModule, GatewayBaseModule.GatewayBaseModule):

	def setUp(self):
		self.vdevConfig = vdevIniMultiple
		self.gwsConfig['autoRegister'] = True
		self.gwsConfig['autoData'] = True
		self.gwsConfig['autoResponse'] = True
		self.gwsConfig['autoNewDevice'] = True
		super().setUp()

	def test_unpairDevices(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())

		firstDeviceReceived = False
		secondDeviceReceived = False
		for _ in range(0,10):
			sensorData = self.GWS.nextData()
			if sensorData.deviceID == "0xa300000000000001":
				self.assertEqual(sensorData.data[0][0], 0)
				self.assertAlmostEqual(sensorData.data[0][1], 5)
				firstDeviceReceived = True
			elif sensorData.deviceID == "0xa300000000000002":
				self.assertEqual(sensorData.data[0][0], 0)
				self.assertAlmostEqual(sensorData.data[0][1], 10)
				secondDeviceReceived = True
			else:
				self.fail("Unknown deviceID received: " + sensorData.deviceID)

			if firstDeviceReceived and secondDeviceReceived:
				break

		if not firstDeviceReceived:
			self.fail("first device do not export data")

		if not secondDeviceReceived:
			self.fail("second device do not export data")
		# both devices are correctly exporting dataMessages
		# now, lets unpair first one
		commander = FakeGWServer.Commander(self.GWS)
		self.assertEqual(commander.unpair("0xa300000000000001"),1)

		# data could be still cached in Distributor, lets wait for it to flush
		self.assertFalse(self.GWS.waitForDisconnect(5))
		self.GWS.clearData()

		for _ in range(0,5):
			sensorData = self.GWS.nextData()
			# only second device is allowed to export
			self.assertEqual(sensorData.deviceID, "0xa300000000000002")
			self.assertEqual(len(sensorData.data), 1)
			self.assertEqual(sensorData.data[0][0], 0)
			self.assertAlmostEqual(sensorData.data[0][1], 10)

		# now, lets unpair second device
		self.assertEqual(commander.unpair("0xa300000000000002"),1)

		# data could be still cached in Distributor, lets wait for it to flush
		self.assertFalse(self.GWS.waitForDisconnect(5))

		self.GWS.clearData()

		self.assertEqual(self.GWS.nextData(), None)
