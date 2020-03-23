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
paired = no
refresh = 2
device_id = 0xa300000000000001
vendor = BeeeOn
product = Temperature Sensor
module0.type = temperature, inner
module0.generator = 5
"""

"""
We emulate the whole pairing process of virtual devices using
GWServerConnector.
"""
class TestPairing(VdevModule.VdevModule, GWServerModule.GWServerModule, GatewayBaseModule.GatewayBaseModule):

	def setUp(self):
		self.vdevConfig = vdevIni
		self.gwsConfig['autoRegister'] = True
		self.gwsConfig['autoData'] = True
		self.gwsConfig['autoResponse'] = True
		self.gwsConfig['autoNewDevice'] = True
		super().setUp()

	def test_pairSingleDevice(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())
		commander = FakeGWServer.Commander(self.GWS)
		self.assertEqual(commander.listen(10),1)
		newDevice = self.GWS.nextNewDevice()
		self.assertEqual(newDevice.id, "0xa300000000000001")
		self.assertEqual(newDevice.name, "Temperature Sensor")
		self.assertEqual(newDevice.vendor, "BeeeOn")
		self.assertEqual(newDevice.refresh, 2)
		self.assertEqual(len(newDevice.moduleTypes), 1)
		self.assertEqual(len(newDevice.moduleTypes[0][1]), 1) # number of attributes
		self.assertEqual(newDevice.moduleTypes[0][1][0], "inner")
		self.assertEqual(newDevice.moduleTypes[0][0], "temperature")
		#device has reported correctly, now lets accept it
		self.assertTrue(commander.deviceAccept("0xa300000000000001"))
		#device should be now paired and exporting data
		sensorData = self.GWS.nextData()
		self.assertEqual(sensorData.deviceID, "0xa300000000000001")
		self.assertEqual(len(sensorData.data), 1)
		self.assertEqual(sensorData.data[0][0], 0)
		self.assertAlmostEqual(sensorData.data[0][1], 5)

		self.assertFalse(self.GWS.waitForDisconnect(5))

vdevIniMultiModule="""
[virtual-devices]
request.device_list = no

[virtual-device0]
enable = yes
paired = no
refresh = 2
device_id = 0xa300000000000001
vendor = BeeeOn
product = MultiSensor
module0.type = temperature, inner
module0.generator = 5
module1.type = pressure, outer
module1.generator = 10
"""

class TestPairingMultiModule(VdevModule.VdevModule, GWServerModule.GWServerModule, GatewayBaseModule.GatewayBaseModule):
	def setUp(self):
		self.vdevConfig = vdevIniMultiModule
		self.gwsConfig['autoRegister'] = True
		self.gwsConfig['autoData'] = True
		self.gwsConfig['autoResponse'] = True
		self.gwsConfig['autoNewDevice'] = True
		super().setUp()

	def test_pairSingleDevice(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())
		commander = FakeGWServer.Commander(self.GWS)
		self.assertEqual(commander.listen(10),1)
		newDevice = self.GWS.nextNewDevice()
		self.assertEqual(newDevice.id, "0xa300000000000001")
		self.assertEqual(newDevice.name, "MultiSensor")
		self.assertEqual(newDevice.vendor, "BeeeOn")
		self.assertEqual(newDevice.refresh, 2)
		self.assertEqual(len(newDevice.moduleTypes), 2)
		self.assertEqual(len(newDevice.moduleTypes[0][1]), 1) # number of attributes
		self.assertEqual(len(newDevice.moduleTypes[1][1]), 1) # number of attributes
		self.assertEqual(newDevice.moduleTypes[0][1][0], "inner")
		self.assertEqual(newDevice.moduleTypes[1][1][0], "outer")
		self.assertEqual(newDevice.moduleTypes[0][0], "temperature")
		self.assertEqual(newDevice.moduleTypes[1][0], "pressure")
		#device has reported correctly, now lets accept it
		self.assertTrue(commander.deviceAccept("0xa300000000000001"))
		#device should be now paired and exporting data
		sensorData = self.GWS.nextData()
		self.assertEqual(sensorData.deviceID, "0xa300000000000001")
		self.assertEqual(len(sensorData.data), 2)
		self.assertEqual(sensorData.data[0][0], 0)
		self.assertAlmostEqual(sensorData.data[0][1], 5)

		self.assertEqual(sensorData.data[1][0], 1)
		self.assertAlmostEqual(sensorData.data[1][1], 10)

		#self.assertFalse(self.GWS.waitForDisconnect(5))

vdevIniMultiDevices="""
[virtual-devices]
request.device_list = no

[virtual-device0]
enable = yes
paired = no
refresh = 2
device_id = 0xa300000000000001
vendor = BeeeOn
product = MultiSensor
module0.type = temperature, inner
module0.generator = 5
module1.type = pressure, outer
module1.generator = 10

[virtual-device1]
enable = yes
paired = no
refresh = 3
device_id = 0xa300000000000002
vendor = BeeeOn
product = MultiSensor
module0.type = temperature, inner
module0.generator = 50
module1.type = pressure, outer
module1.generator = 100
"""

class TestPairingMultipleDevices(VdevModule.VdevModule, GWServerModule.GWServerModule, GatewayBaseModule.GatewayBaseModule):
	def setUp(self):
		self.vdevConfig = vdevIniMultiDevices
		self.gwsConfig['autoRegister'] = True
		self.gwsConfig['autoData'] = True
		self.gwsConfig['autoResponse'] = True
		self.gwsConfig['autoNewDevice'] = True
		super().setUp()

	def test_pairMultiDevices(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())
		commander = FakeGWServer.Commander(self.GWS)
		self.assertEqual(commander.listen(10),1)
		newDevice1 = self.GWS.nextNewDevice()
		newDevice2 = self.GWS.nextNewDevice()
		if newDevice1.id == "0xa300000000000002":
			newDevice1, newDevice2 = newDevice2, newDevice1 # swap variables

		#device1 assertions
		self.assertEqual(newDevice1.id, "0xa300000000000001")
		self.assertEqual(newDevice1.name, "MultiSensor")
		self.assertEqual(newDevice1.vendor, "BeeeOn")
		self.assertEqual(newDevice1.refresh, 2)
		self.assertEqual(len(newDevice1.moduleTypes), 2)

		self.assertEqual(len(newDevice1.moduleTypes[0][1]), 1) # number of attributes
		self.assertEqual(newDevice1.moduleTypes[0][0], "temperature")
		self.assertEqual(newDevice1.moduleTypes[0][1][0], "inner")

		self.assertEqual(len(newDevice1.moduleTypes[1][1]), 1) # number of attributes
		self.assertEqual(newDevice1.moduleTypes[1][0], "pressure")
		self.assertEqual(newDevice1.moduleTypes[1][1][0], "outer")

		#device2 assertions
		self.assertEqual(newDevice2.id, "0xa300000000000002")
		self.assertEqual(newDevice2.name, "MultiSensor")
		self.assertEqual(newDevice2.vendor, "BeeeOn")
		self.assertEqual(newDevice2.refresh, 3)
		self.assertEqual(len(newDevice1.moduleTypes), 2)

		self.assertEqual(len(newDevice1.moduleTypes[0][1]), 1) # number of attributes
		self.assertEqual(newDevice1.moduleTypes[0][0], "temperature")
		self.assertEqual(newDevice1.moduleTypes[0][1][0], "inner")

		self.assertEqual(len(newDevice1.moduleTypes[1][1]), 1) # number of attributes
		self.assertEqual(newDevice1.moduleTypes[1][0], "pressure")
		self.assertEqual(newDevice1.moduleTypes[1][1][0], "outer")

		#both devices has reported correctly, now lets accept first one
		self.assertTrue(commander.deviceAccept("0xa300000000000001"))
		#device should be now paired and exporting data
		sensorData = self.GWS.nextData()
		self.assertEqual(sensorData.deviceID, "0xa300000000000001")
		self.assertEqual(len(sensorData.data), 2)
		self.assertEqual(sensorData.data[0][0], 0)
		self.assertAlmostEqual(sensorData.data[0][1], 5)

		self.assertEqual(sensorData.data[1][0], 1)
		self.assertAlmostEqual(sensorData.data[1][1], 10)

		self.assertFalse(self.GWS.waitForDisconnect(5))

		# pair second device
		self.assertTrue(commander.deviceAccept("0xa300000000000002"))
		self.GWS.clearData()

		firstDeviceReceived = False
		secondDeviceReceived = False
		for _ in range(0,10):
			sensorData = self.GWS.nextData()
			if sensorData.deviceID == "0xa300000000000001":
				self.assertEqual(sensorData.data[0][0], 0)
				self.assertAlmostEqual(sensorData.data[0][1], 5)

				self.assertEqual(sensorData.data[1][0], 1)
				self.assertAlmostEqual(sensorData.data[1][1], 10)
				firstDeviceReceived = True
			elif sensorData.deviceID == "0xa300000000000002":
				self.assertEqual(sensorData.data[0][0], 0)
				self.assertAlmostEqual(sensorData.data[0][1], 50)

				self.assertEqual(sensorData.data[1][0], 1)
				self.assertAlmostEqual(sensorData.data[1][1], 100)
				secondDeviceReceived = True
			else:
				self.fail("Unknown deviceID received: " + sensorData.deviceID)

			if firstDeviceReceived and secondDeviceReceived:
				break

		if not firstDeviceReceived:
			self.fail("first device do not export data")

		if not secondDeviceReceived:
			self.fail("second device do not export data")
