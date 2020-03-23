#!/bin/python3

from os import sys, path
sys.path.append(path.dirname(path.dirname(path.dirname(path.abspath(__file__)))))

import globalConfig

import GatewayBaseModule
import BelkinModule
import NamedPipeModule
import TCModule
import time
import SensorData

class TestBelkinDimmer(BelkinModule.BelkinModule, TCModule.TCModule, NamedPipeModule.NamedPipeModule, GatewayBaseModule.GatewayBaseModule):

	def setUp(self):
		self.bewemuConfig['dimmer'] = True
		super().setUp()

	def test_All(self):
		self.assertFalse(self.NP.wait(10))
		commander = self.TC.createCommander()
		commander.listen(10)
		time.sleep(5)
		devices = self.TC.listDevices()
		self.assertEqual(len(devices),1)
		commander.deviceAccept(devices[0])

		self.assertTrue(self.NP.wait(20))

		data = SensorData.SensorData()
		data.fromCSV(self.NP.nextData())

		self.assertEqual(data.deviceID, devices[0])
		self.assertEqual(data.data[0][0], 0)
		self.assertAlmostEqual(data.data[0][1], 0)

		commander.deviceSetValue(devices[0], "0", "1")

		time.sleep(3)
		self.NP.clearData()

		self.assertTrue(self.NP.wait(20))
		data = SensorData.SensorData()
		data.fromCSV(self.NP.nextData())

		self.assertEqual(data.deviceID, devices[0])
		self.assertEqual(data.data[0][0], 0)
		self.assertAlmostEqual(data.data[0][1], 1)

		# now unpair device
		commander.unpair(devices[0])
		time.sleep(3)
		self.NP.clearData()
		self.assertFalse(self.NP.wait(10))
