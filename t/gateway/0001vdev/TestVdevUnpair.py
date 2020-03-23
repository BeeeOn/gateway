#!/bin/python3

from os import sys, path
sys.path.append(path.dirname(path.dirname(path.dirname(path.abspath(__file__)))))

import globalConfig

import GatewayBaseModule
import VdevModule
import NamedPipeModule
import TCModule
import time
import SensorData

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

"""
We try to unpair pre-paired device from Gateway and assert it is no longer exporting any data.
"""
class TestVdevUnpairing(VdevModule.VdevModule, TCModule.TCModule, NamedPipeModule.NamedPipeModule, GatewayBaseModule.GatewayBaseModule):
	def setUp(self):
		self.vdevConfig = vdevIni
		super().setUp()

	def test_vdevUnpairOne(self):
		firstDeviceReceived = False
		secondDeviceReceived = False

		commander = self.TC.createCommander()
		commander.listen(10)
		commander.deviceAccept("0xa300000000000001")
		commander.deviceAccept("0xa300000000000002")

		for i in range(0,10):
			self.assertTrue(self.NP.wait(10))
			data = SensorData.SensorData()
			data.fromCSV(self.NP.nextData())

			if data.deviceID == "0xa300000000000001":
				self.assertEqual(len(data.data), 1)
				self.assertEqual(data.data[0][0], 0)
				self.assertAlmostEqual(data.data[0][1], 5)
				firstDeviceReceived = True
			elif data.deviceID == "0xa300000000000002":
				self.assertEqual(len(data.data), 1)
				self.assertEqual(data.data[0][0], 0)
				self.assertAlmostEqual(data.data[0][1], 10)
				secondDeviceReceived = True
			else:
				self.fail("Unknown deviceID received: " + data.deviceID)

			if firstDeviceReceived and secondDeviceReceived:
				break

		if not firstDeviceReceived:
			self.fail("first device do not export data")

		if not secondDeviceReceived:
			self.fail("second device do not export data")

		commander = self.TC.createCommander()
		ret = commander.unpair("0xa300000000000001")
		self.assertEqual(ret, (1,0))

		self.NP.clearData()

		for i in range(0,5):
			self.assertTrue(self.NP.wait(10))
			data = SensorData.SensorData()
			data.fromCSV(self.NP.nextData())
			self.assertEqual(data.deviceID, "0xa300000000000002")
			self.assertEqual(len(data.data), 1)
			self.assertEqual(data.data[0][0], 0)
			self.assertAlmostEqual(data.data[0][1], 10)

	def test_vdevUnpairBoth(self):
		firstDeviceReceived = False
		secondDeviceReceived = False

		commander = self.TC.createCommander()
		commander.listen(10)
		commander.deviceAccept("0xa300000000000001")
		commander.deviceAccept("0xa300000000000002")

		for i in range(0,10):
			self.assertTrue(self.NP.wait(10))
			data = SensorData.SensorData()
			data.fromCSV(self.NP.nextData())

			if data.deviceID == "0xa300000000000001":
				self.assertEqual(len(data.data), 1)
				self.assertEqual(data.data[0][0], 0)
				self.assertAlmostEqual(data.data[0][1], 5)
				firstDeviceReceived = True
			elif data.deviceID == "0xa300000000000002":
				self.assertEqual(len(data.data), 1)
				self.assertEqual(data.data[0][0], 0)
				self.assertAlmostEqual(data.data[0][1], 10)
				secondDeviceReceived = True
			else:
				self.fail("Unknown deviceID received: " + data.deviceID)

			if firstDeviceReceived and secondDeviceReceived:
				break

		if not firstDeviceReceived:
			self.fail("first device do not export data")

		if not secondDeviceReceived:
			self.fail("second device do not export data")

		commander = self.TC.createCommander()
		ret = commander.unpair("0xa300000000000001")
		self.assertEqual(ret, (1,0))
		ret = commander.unpair("0xa300000000000002")
		self.assertEqual(ret, (1,0))

		self.NP.clearData()

		self.assertFalse(self.NP.wait(10))
