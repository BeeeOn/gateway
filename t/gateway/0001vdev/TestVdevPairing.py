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
This is the pairing process test, where we pair the device
and assert it is exporting data.
"""
class TestVdevPairing(VdevModule.VdevModule, TCModule.TCModule, NamedPipeModule.NamedPipeModule, GatewayBaseModule.GatewayBaseModule):
	def setUp(self):
		self.vdevConfig = vdevIni
		super().setUp()

	def test_vdevPairOne(self):
		self.NP.clearData()
		self.assertFalse(self.NP.wait(10))

		commander = self.TC.createCommander()
		commander.listen(10)
		# TODO wait for device list
		commander.deviceAccept("0xa300000000000001")

		self.assertTrue(self.NP.wait(10))
		data = SensorData.SensorData()
		data.fromCSV(self.NP.nextData())
		self.assertEqual(data.deviceID, "0xa300000000000001")
		self.assertEqual(len(data.data), 1)
		self.assertEqual(data.data[0], (int(0),float(5.00)))

	def test_vdevPairBoth(self):
		self.NP.clearData()
		self.assertFalse(self.NP.wait(10))

		commander = self.TC.createCommander()
		commander.listen(10)
		# TODO wait for device list
		commander.deviceAccept("0xa300000000000001")
		commander.deviceAccept("0xa300000000000002")

		firstDeviceReceived = False
		secondDeviceReceived = False
		for i in range(0,10):
			self.assertTrue(self.NP.wait(10))
			data = SensorData.SensorData()
			data.fromCSV(self.NP.nextData())

			if data.deviceID == "0xa300000000000001":
				self.assertEqual(len(data.data), 1)
				self.assertEqual(data.data[0], (int(0),float(5.00)))
				firstDeviceReceived = True
			elif data.deviceID == "0xa300000000000002":
				self.assertEqual(len(data.data), 1)
				self.assertEqual(data.data[0], (int(0),float(10.00)))
				secondDeviceReceived = True
			else:
				self.fail("Unknown deviceID received: " + data.deviceID)

			if firstDeviceReceived and secondDeviceReceived:
				break

		if not firstDeviceReceived:
			self.fail("first device do not export data")

		if not secondDeviceReceived:
			self.fail("second device do not export data")
