#!/bin/python3

from os import sys, path
sys.path.append(path.dirname(path.dirname(path.dirname(path.abspath(__file__)))))

import globalConfig

import GatewayBaseModule
import VdevModule
import SensorData
import GWServerModule
import time
import json

vdevIni="""
[virtual-devices]
request.device_list = yes

[virtual-device0]
enable = yes
paired = no
refresh = 2
device_id = 0xa300000000000001
vendor = BeeeOn
product = Temperature Sensor
module0.type = temperature, inner
module0.generator = 5

[virtual-device1]
enable = yes
paired = no
refresh = 2
device_id = 0xa300000000000002
vendor = BeeeOn
product = Temperature Sensor
module0.type = temperature, inner
module0.generator = 10
"""

"""
Verification, that GWServerConnector will send Vritual Device's deviceList request
to get paired devices. We answer it with specified devices and assert it start exporting data.
"""
class TestDeviceListSingle(VdevModule.VdevModule, GWServerModule.GWServerModule, GatewayBaseModule.GatewayBaseModule):

	def setUp(self):
		self.vdevConfig = vdevIni
		self.gwsConfig['autoRegister'] = True
		self.gwsConfig['autoData'] = True
		self.gwsConfig['prePaired'] = {"VirtualDevice" : [{"device_id":'0xa300000000000001'}]}
		super().setUp()

	def test_exportSingleData(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())
		sensorData = self.GWS.nextData()
		self.assertEqual(sensorData.deviceID, "0xa300000000000001")
		self.assertEqual(len(sensorData.data), 1)
		self.assertEqual(sensorData.data[0][0], 0)
		self.assertAlmostEqual(sensorData.data[0][1], 5)
		self.assertFalse(self.GWS.waitForDisconnect(2))

	def test_exportMultipleData(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())
		for _ in range(0,5):
			sensorData = self.GWS.nextData()
			self.assertEqual(sensorData.deviceID, "0xa300000000000001")
			self.assertEqual(len(sensorData.data), 1)
			self.assertEqual(sensorData.data[0][0], 0)
			self.assertAlmostEqual(sensorData.data[0][1], 5)
		self.assertFalse(self.GWS.waitForDisconnect(2))


class TestDeviceListBoth(VdevModule.VdevModule, GWServerModule.GWServerModule, GatewayBaseModule.GatewayBaseModule):

	def setUp(self):
		self.vdevConfig = vdevIni
		self.gwsConfig['autoRegister'] = True
		self.gwsConfig['autoData'] = True
		self.gwsConfig['prePaired'] = {"VirtualDevice" : [{"device_id":'0xa300000000000001'},{"device_id":'0xa300000000000002'}]}
		super().setUp()

	def test_exportMultipleData(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())
		firstDeviceReceived = False
		secondDeviceReceived = False
		for i in range(0,10):
			sensorData = self.GWS.nextData()
			if sensorData.deviceID == "0xa300000000000001":
				self.assertEqual(len(sensorData.data), 1)
				self.assertAlmostEqual(sensorData.data[0], (0,5.00))
				firstDeviceReceived = True
			elif sensorData.deviceID == "0xa300000000000002":
				self.assertEqual(len(sensorData.data), 1)
				self.assertAlmostEqual(sensorData.data[0], (0,10.00))
				secondDeviceReceived = True
			else:
				self.fail("Unknown deviceID received: " + data.deviceID)

			if firstDeviceReceived and secondDeviceReceived:
				break

		if not firstDeviceReceived:
			self.fail("first device do not export data")

		if not secondDeviceReceived:
			self.fail("second device do not export data")

class TestDeviceListNone(VdevModule.VdevModule, GWServerModule.GWServerModule, GatewayBaseModule.GatewayBaseModule):
	def setUp(self):
		self.vdevConfig = vdevIni
		self.gwsConfig['autoRegister'] = True
		self.gwsConfig['autoData'] = True
		self.gwsConfig['prePaired'] = {"VirtualDevice" : []}
		super().setUp()

	def test_exportSingleData(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())
		self.assertEqual(self.GWS.nextData(), None)
