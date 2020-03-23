#!/bin/python3

from os import sys, path
sys.path.append(path.dirname(path.dirname(path.dirname(path.abspath(__file__)))))

import globalConfig

import GatewayBaseModule
import NamedPipeModule
import TCModule
import time
import SensorData
import SonoffModule

class TestSonoffSC(SonoffModule.SonoffModule, TCModule.TCModule, NamedPipeModule.NamedPipeModule, GatewayBaseModule.GatewayBaseModule):

	def setUp(self):
		self.sonoffConfig['sc'] = True
		super().setUp()

	def test_All(self):
		self.assertFalse(self.NP.wait(10))
		commander = self.TC.createCommander()
		commander.listen(30)
		time.sleep(5)
		devices = self.TC.listDevices()
		self.assertGreaterEqual(len(devices),1)
		commander.deviceAccept(devices[0])

		self.assertTrue(self.NP.wait(60))

		# temperature
		data = SensorData.SensorData()
		data.fromCSV(self.NP.nextData())
		self.assertEqual(data.deviceID, devices[0])
		self.assertEqual(data.data[0][0], 0)
		self.assertGreaterEqual(data.data[0][1], 10)
		self.assertLessEqual(data.data[0][1], 50)

		# humidity
		data = SensorData.SensorData()
		data.fromCSV(self.NP.nextData())
		self.assertEqual(data.deviceID, devices[0])
		self.assertEqual(data.data[0][0], 1)
		self.assertGreaterEqual(data.data[0][1], 20)
		self.assertLessEqual(data.data[0][1], 100)

		# noise
		data = SensorData.SensorData()
		data.fromCSV(self.NP.nextData())
		self.assertEqual(data.deviceID, devices[0])
		self.assertEqual(data.data[0][0], 2)
		self.assertGreaterEqual(data.data[0][1], 5)
		self.assertLessEqual(data.data[0][1], 160)

		# dust
		data = SensorData.SensorData()
		data.fromCSV(self.NP.nextData())
		self.assertEqual(data.deviceID, devices[0])
		self.assertEqual(data.data[0][0], 3)
		self.assertGreaterEqual(data.data[0][1], 10)
		self.assertLessEqual(data.data[0][1], 50)

		# light
		data = SensorData.SensorData()
		data.fromCSV(self.NP.nextData())
		self.assertEqual(data.deviceID, devices[0])
		self.assertEqual(data.data[0][0], 4)
		self.assertGreaterEqual(data.data[0][1], 1)
		self.assertLessEqual(data.data[0][1], 100000)

		# now unpair device
		commander.unpair(devices[0])
		time.sleep(3)
		self.NP.clearData()
		self.assertFalse(self.NP.wait(10))
