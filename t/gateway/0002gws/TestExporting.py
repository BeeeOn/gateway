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
The basic exporting test. We are expecting Virtual Devices to start
exporting data immidiately after start, because they are pre-paired.
"""
class TestExporting(VdevModule.VdevModule, GWServerModule.GWServerModule, GatewayBaseModule.GatewayBaseModule):

	def setUp(self):
		self.vdevConfig = vdevIni
		self.gwsConfig['autoRegister'] = True
		self.gwsConfig['autoData'] = True
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
