#!/bin/python3

from os import sys, path
sys.path.append(path.dirname(path.dirname(path.dirname(path.abspath(__file__)))))

import globalConfig

import GatewayBaseModule
import GWServerModule
import time
import json
import string
import random

def randomStringGen(size):
	return ''.join(random.choice(string.ascii_lowercase) for _ in range(size))


"""
We test the various basic connection reactions of gateway.
"""
class TestConnection(GWServerModule.GWServerModule, GatewayBaseModule.GatewayBaseModule):

	def test_connectAndRegister(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())
		jsonMessage = json.loads(self.GWS.nextMessage())
		self.assertEqual(jsonMessage['message_type'],"gateway_register")
		self.assertEqual(jsonMessage['ip_address'],"127.0.0.1")
		self.assertEqual(jsonMessage['gateway_id'],"1254321374233360")

		self.GWS.sendMessage('{ "message_type" : "gateway_accepted" }')
		self.assertFalse(self.GWS.waitForDisconnect(10))

	def test_sendNonParseable(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())
		jsonMessage = json.loads(self.GWS.nextMessage())
		self.assertEqual(jsonMessage['message_type'],"gateway_register")
		self.assertEqual(jsonMessage['ip_address'],"127.0.0.1")
		self.assertEqual(jsonMessage['gateway_id'],"1254321374233360")

		self.GWS.sendMessage("NOT A JSON MESSAGE")

		self.assertTrue(self.GWS.waitForDisconnect())
		self.GWS.enableReset()
		self.assertTrue(self.GWS.waitForConnection())
		jsonMessage = json.loads(self.GWS.nextMessage())
		self.assertEqual(jsonMessage['message_type'],"gateway_register")
		self.assertEqual(jsonMessage['ip_address'],"127.0.0.1")
		self.assertEqual(jsonMessage['gateway_id'],"1254321374233360")
		self.GWS.sendMessage('{ "message_type" : "gateway_accepted" }')
		self.GWS.sendMessage("NOT A JSON MESSAGE")
		self.assertTrue(self.GWS.waitForDisconnect())

		self.GWS.enableReset()
		self.assertTrue(self.GWS.waitForConnection())
		jsonMessage = json.loads(self.GWS.nextMessage())
		self.assertEqual(jsonMessage['message_type'],"gateway_register")
		self.assertEqual(jsonMessage['ip_address'],"127.0.0.1")
		self.assertEqual(jsonMessage['gateway_id'],"1254321374233360")
		self.GWS.sendMessage('{ "message_type" : "gateway_accepted" }')
		self.assertFalse(self.GWS.waitForDisconnect())

	def test_sendInvalidMessageType(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())
		jsonMessage = json.loads(self.GWS.nextMessage())
		self.assertEqual(jsonMessage['message_type'],"gateway_register")
		self.assertEqual(jsonMessage['ip_address'],"127.0.0.1")
		self.assertEqual(jsonMessage['gateway_id'],"1254321374233360")

		self.GWS.sendMessage('{ "message_type" : "invalid_message" }')

		self.assertTrue(self.GWS.waitForDisconnect())
		self.GWS.enableReset()
		self.assertTrue(self.GWS.waitForConnection())
		jsonMessage = json.loads(self.GWS.nextMessage())
		self.assertEqual(jsonMessage['message_type'],"gateway_register")
		self.assertEqual(jsonMessage['ip_address'],"127.0.0.1")
		self.assertEqual(jsonMessage['gateway_id'],"1254321374233360")
		self.GWS.sendMessage('{ "message_type" : "gateway_accepted" }')
		self.GWS.sendMessage('{ "message_type" : "invalid_message" }')
		self.assertTrue(self.GWS.waitForDisconnect())

		self.GWS.enableReset()
		self.assertTrue(self.GWS.waitForConnection())
		jsonMessage = json.loads(self.GWS.nextMessage())
		self.assertEqual(jsonMessage['message_type'],"gateway_register")
		self.assertEqual(jsonMessage['ip_address'],"127.0.0.1")
		self.assertEqual(jsonMessage['gateway_id'],"1254321374233360")
		self.GWS.sendMessage('{ "message_type" : "gateway_accepted" }')
		self.assertFalse(self.GWS.waitForDisconnect())

	def test_sendNoMessageType(self):
		self.GWS.start()
		self.assertTrue(self.GWS.waitForConnection())
		jsonMessage = json.loads(self.GWS.nextMessage())
		self.assertEqual(jsonMessage['message_type'],"gateway_register")
		self.assertEqual(jsonMessage['ip_address'],"127.0.0.1")
		self.assertEqual(jsonMessage['gateway_id'],"1254321374233360")

		self.GWS.sendMessage('{ "no_message_type" : "no_message_type" }')

		self.assertTrue(self.GWS.waitForDisconnect())
		self.GWS.enableReset()
		self.assertTrue(self.GWS.waitForConnection())
		jsonMessage = json.loads(self.GWS.nextMessage())
		self.assertEqual(jsonMessage['message_type'],"gateway_register")
		self.assertEqual(jsonMessage['ip_address'],"127.0.0.1")
		self.assertEqual(jsonMessage['gateway_id'],"1254321374233360")
		self.GWS.sendMessage('{ "message_type" : "gateway_accepted" }')
		self.GWS.sendMessage('{ "no_message_type" : "no_message_type" }')
		self.assertTrue(self.GWS.waitForDisconnect())

		self.GWS.enableReset()
		self.assertTrue(self.GWS.waitForConnection())
		jsonMessage = json.loads(self.GWS.nextMessage())
		self.assertEqual(jsonMessage['message_type'],"gateway_register")
		self.assertEqual(jsonMessage['ip_address'],"127.0.0.1")
		self.assertEqual(jsonMessage['gateway_id'],"1254321374233360")
		self.GWS.sendMessage('{ "message_type" : "gateway_accepted" }')
		self.assertFalse(self.GWS.waitForDisconnect())

	def test_sendRandomLong(self):
		self.GWS.start()
		size = 8192
		self.assertTrue(self.GWS.waitForConnection())
		jsonMessage = json.loads(self.GWS.nextMessage())
		self.assertEqual(jsonMessage['message_type'],"gateway_register")
		self.assertEqual(jsonMessage['ip_address'],"127.0.0.1")
		self.assertEqual(jsonMessage['gateway_id'],"1254321374233360")
		self.GWS.sendMessage(randomStringGen(size))

		self.assertTrue(self.GWS.waitForDisconnect())
		self.GWS.enableReset()
		self.assertTrue(self.GWS.waitForConnection())
		jsonMessage = json.loads(self.GWS.nextMessage())
		self.assertEqual(jsonMessage['message_type'],"gateway_register")
		self.assertEqual(jsonMessage['ip_address'],"127.0.0.1")
		self.assertEqual(jsonMessage['gateway_id'],"1254321374233360")
		self.GWS.sendMessage('{ "message_type" : "gateway_accepted" }')
		self.GWS.sendMessage(randomStringGen(size))
		self.assertTrue(self.GWS.waitForDisconnect())

		self.GWS.enableReset()
		self.assertTrue(self.GWS.waitForConnection())
		jsonMessage = json.loads(self.GWS.nextMessage())
		self.assertEqual(jsonMessage['message_type'],"gateway_register")
		self.assertEqual(jsonMessage['ip_address'],"127.0.0.1")
		self.assertEqual(jsonMessage['gateway_id'],"1254321374233360")
		self.GWS.sendMessage('{ "message_type" : "gateway_accepted" }')
		self.assertFalse(self.GWS.waitForDisconnect())
