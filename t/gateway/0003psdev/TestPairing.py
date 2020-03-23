#!/bin/python3

from os import sys, path
sys.path.append(path.dirname(path.dirname(path.dirname(path.abspath(__file__)))))

import globalConfig

import GatewayBaseModule
import PressureModule
import NamedPipeModule
import TCModule
import time
import SensorData

class TestPsdevPairing(PressureModule.PressureModule, TCModule.TCModule, NamedPipeModule.NamedPipeModule, GatewayBaseModule.GatewayBaseModule):

	def setUp(self):
		super().setUp()

	def test_psdevPairing(self):
		self.assertFalse(self.NP.wait(10))
		commander = self.TC.createCommander()
		commander.listen(10)
		time.sleep(3)
		devices = self.TC.listDevices()
		self.assertEqual(len(devices),1)
		commander.deviceAccept(devices[0])
		self.assertTrue(self.NP.wait(10))
		data = SensorData.SensorData()
		data.fromCSV(self.NP.nextData())
		self.assertEqual(data.deviceID, devices[0])
		self.assertEqual(data.data[0][0], 0)
		self.assertAlmostEqual(data.data[0][1], 12340)

		# now unpair device
		commander.unpair(devices[0])
		time.sleep(3)
		self.NP.clearData()
		self.assertFalse(self.NP.wait(10))
