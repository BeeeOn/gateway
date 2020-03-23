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
We testing that pre-paired virtual device should start exporting data
immidiately after start.
"""
class TestVdevDeviceList(VdevModule.VdevModule, TCModule.TCModule, NamedPipeModule.NamedPipeModule, GatewayBaseModule.GatewayBaseModule):
	def setUp(self):
		self.vdevConfig = vdevIni
		self.GR.overrideUpdate("testing.center.pairedDevices", "0xa300000000000001")
		super().setUp()

	def test_vdevExportPaired(self):
		for i in range(0,5):
			self.assertTrue(self.NP.wait(10))
			data = SensorData.SensorData()
			data.fromCSV(self.NP.nextData())
			self.assertEqual(data.deviceID, "0xa300000000000001")
			self.assertEqual(len(data.data), 1)
			self.assertEqual(data.data[0][0], 0)
			self.assertAlmostEqual(data.data[0][1], 5)
