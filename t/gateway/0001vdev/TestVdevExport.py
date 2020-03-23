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
Simple export test. As the device is pre paired already it should immidiately start
exporting data.
"""
class TestVdevExport(VdevModule.VdevModule, TCModule.TCModule, NamedPipeModule.NamedPipeModule, GatewayBaseModule.GatewayBaseModule):

	def setUp(self):
		self.vdevConfig = vdevIni
		super().setUp()

	def test_vdevExport(self):
		commander = self.TC.createCommander()
		commander.listen(10)
		commander.deviceAccept("0xa300000000000001")

		self.assertTrue(self.NP.wait(10))
		data = SensorData.SensorData()
		data.fromCSV(self.NP.nextData())
		self.assertEqual(data.deviceID, "0xa300000000000001")
		self.assertEqual(data.data[0][0], 0)
		self.assertAlmostEqual(data.data[0][1], 5)

		self.assertTrue(self.NP.wait(10))
		data = SensorData.SensorData()
		data.fromCSV(self.NP.nextData())
		self.assertEqual(data.deviceID, "0xa300000000000001")
		self.assertEqual(data.data[0][0], 0)
		self.assertAlmostEqual(data.data[0][1], 5)
