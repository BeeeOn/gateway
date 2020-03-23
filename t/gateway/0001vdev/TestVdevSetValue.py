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
module0.reaction = success

[virtual-device1]
enable = yes
paired = yes
refresh = 2
device_id = 0xa300000000000002
vendor = BeeeOn
product = Temperature Sensor
module0.type = temperature, inner
module0.reaction = failure
"""

"""
We try to set value of virtual device with different reactions.
"""
class TestVdevSetValue(VdevModule.VdevModule, TCModule.TCModule, NamedPipeModule.NamedPipeModule, GatewayBaseModule.GatewayBaseModule):
	def setUp(self):
		self.vdevConfig = vdevIni
		super().setUp()

	def test_vdevSetValue(self):
		commander = self.TC.createCommander()
		ret = commander.deviceSetValue("0xa300000000000001", "0", "0")
		self.assertEqual(ret, (1,0)) # should succeed
		ret = commander.deviceSetValue("0xa300000000000002", "0", "0")
		self.assertEqual(ret, (0,1)) # should fail
