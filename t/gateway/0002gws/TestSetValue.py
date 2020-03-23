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
paired = yes
device_id = 0xa300000000000001
vendor = BeeeOn
product = Temperature Sensor
module0.type = temperature, inner
module0.reaction = success
module1.type = temperature, inner
module1.reaction = failure
"""

"""
We set the value of virtual device using GWServerConnector.
"""
class TestSetValue(VdevModule.VdevModule, GWServerModule.GWServerModule, GatewayBaseModule.GatewayBaseModule):

	def setUp(self):
		self.vdevConfig = vdevIni
		self.gwsConfig['autoRegister'] = True
		self.gwsConfig['autoData'] = True
		self.gwsConfig['autoResponse'] = True
		self.gwsConfig['autoNewDevice'] = True
		super().setUp()

	def test_setValue(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())

		commander = FakeGWServer.Commander(self.GWS)
		self.assertEqual(commander.deviceSetValue("0xa300000000000001",0,1),1) # success
		self.assertEqual(commander.deviceSetValue("0xa300000000000001",1,1),2) # failure

		self.assertFalse(self.GWS.waitForDisconnect(5))
