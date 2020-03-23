#!/bin/python3

from os import sys, path
sys.path.append(path.dirname(path.dirname(path.dirname(path.abspath(__file__)))))

import globalConfig

import GatewayBaseModule
import TCModule
import time

"""
This is the generic base class for testing the core system stability
of various commands with multiple DeviceManager-s enabled.
We test that commands like listen should be finished in time.
We also test that commands with specific deviceID is accepted only
by the DeviceManager it belongs to.
"""

class TestCoreBase(TCModule.TCModule, GatewayBaseModule.GatewayBaseModule):
	def test_echo(self):
		commander = self.TC.createCommander()

		testString = "Hello gateway"
		res = commander.echo(testString)
		self.assertEqual(res, testString)

		testString = "Very long string, very long string, very long string"
		res = commander.echo(testString)
		self.assertEqual(res, testString)

		testString = "Hello gateway again"
		res = commander.echo(testString)
		self.assertEqual(res, testString)

	def test_listen(self):
		commander = self.TC.createCommander()
		res = commander.listen(10)
		self.assertEqual(res[1], 0)
		time.sleep(12)

	def test_listenTwice(self):
		commander = self.TC.createCommander()
		res = commander.listen(10)
		self.assertEqual(res[1], 0)
		res = commander.listen(10)
		time.sleep(12)

	def test_listenAndKill(self):
		commander = self.TC.createCommander()
		res = commander.listen(10)
		self.assertEqual(res[1], 0)

	def generic_test_accept(self, deviceID):
		commander = self.TC.createCommander()
		res = commander.deviceAccept("0xffffffffffffffff")
		self.assertEqual(sum(res), 0)
		res = commander.deviceAccept(deviceID)
		self.assertEqual(sum(res), 1)

	def generic_test_unpair(self, deviceID):
		commander = self.TC.createCommander()
		res = commander.unpair("0xffffffffffffffff")
		self.assertEqual(sum(res), 0)
		res = commander.unpair(deviceID)
		self.assertEqual(sum(res), 1)

	def generic_test_setValue(self, deviceID):
		commander = self.TC.createCommander()
		res = commander.deviceSetValue("0xffffffffffffffff", "0", "0")
		self.assertEqual(sum(res), 0)
		res = commander.deviceSetValue(deviceID, "0", "0")
		self.assertEqual(sum(res), 1)
