#!/usr/bin/python
import argparse
import datetime
import json
import logging
import paho.mqtt.client as mqtt
import random
import sys
import time

MQTT_BROKER_URL = "localhost"
MQTT_BROKER_PORT = 1883
MQTT_TOPIC = "sonoffsc/data"

MAX_MAC = 2**48 - 1
RANDOM_MAC = random.randint(1, MAX_MAC)
DEFAULT_IP = "127.0.0.1"

HEARTBEAT_REFRESH_TIME = 300
DATA_REFRESH_TIME = 30

class VirtualSonoffSC:
	def __init__(self, mqttUrl, mqttPort, ip, mac):
		self._mqttUrl = mqttUrl
		self._mqttPort = mqttPort
		self._ip = ip
		self._mac = mac

		self._temperature = 20
		self._humidity = 50
		self._noise = 30
		self._dust = 25
		self._light = 60
		self._lastHeartbeatSent = time.time()

		self._mqtt = mqtt.Client()
		self._mqtt.on_connect = self.onConnect
		self._mqtt.on_publish = self.onPublish

	def onConnect(self, mqtt, obj, flags, rc):
		logging.info("virtual Sonoff SC connected to MQTT broker (rc:" + str(rc) + ")")

	def onPublish(self, mqtt, obj, mid):
		logging.info("message published (mid:" + str(mid) + ")")

	def start(self):
		logging.info("virtual Sonoff SC started with MAC address " + format(self._mac, '012x'))

		self._mqtt.connect(self._mqttUrl, self._mqttPort, 60)
		self._mqtt.loop_start()
		while True:
			self._mqtt.publish(MQTT_TOPIC, self.createMessage())
			time.sleep(DATA_REFRESH_TIME)

	def createMessage(self):
		stringMac = format(self._mac, '012x').upper()[6::]
		timestamp = datetime.datetime.now().strftime("%Y/%m/%d %H:%M:%S")

		message = {}
		message["host"] = "SONOFFSC_" + stringMac
		message["ip"] = self._ip
		message["time"] = timestamp

		if (time.time() - self._lastHeartbeatSent) > HEARTBEAT_REFRESH_TIME:
			message["heartbeat"] = 1
			self._lastHeartbeatSent = time.time()

			logging.info("heartbeat message will be published")
		else:
			self.generateNewData()
			message["temperature"] = self._temperature
			message["humidity"] = self._humidity
			message["noise"] = self._noise
			message["dust"] = self._dust
			message["light"] = self._light

			logging.info("data message will be published")

		return json.dumps(message)

	def generateNewData(self):
		self._temperature = self.uniformGeneration(self._temperature, 10, 50)
		self._humidity = round(self.uniformGeneration(self._humidity, 20, 100))
		self._noise = round(self.uniformGeneration(self._noise, 5, 160))
		self._dust = self.uniformGeneration(self._dust, 10, 50)
		self._light = round(self.uniformGeneration(self._light, 1, 100000))

	def uniformGeneration(self, data, minVal, maxVal):
		dataDiff = 0.1 * data
		newData = data + round(random.uniform(-dataDiff, dataDiff), 2)
		if newData < minVal:
			newData = minVal
		elif newData > maxVal:
			newData = maxVal

		return newData

	def __enter__(self):
		return self

	def __exit__(self, type, value, traceback):
		self._mqtt.disconnect()
		self._mqtt.loop_stop()
		logging.info("virtual Sonoff SC successly stopped")


if __name__ == "__main__":
	logging.basicConfig(level = logging.DEBUG)

	parser = argparse.ArgumentParser()
	parser.add_argument("--mqtt-url", metavar="MQTT_BROKER_URL",
		help="The URL of the MQTT Broker to which the multisensor emulator will attempt to connect. The default URL is localhost.")
	parser.add_argument("--mqtt-port", metavar="MQTT_BROKER_PORT",
		type=int, help="The port of the MQTT Broker to which the multisensor emulator will attempt to connect. The default port is 1883.")
	parser.add_argument("--ip", metavar="IP_ADDRESS",
		help="The multisensor will have this ip address. The default IP address is 127.0.0.1.")
	parser.add_argument("--mac", metavar="MAC",
		help="The multisensor will have this mac address. Has to be put as hex number.")
	args = parser.parse_args()

	mqttUrl = args.mqtt_url if args.mqtt_url is not None else MQTT_BROKER_URL
	mqttPort = int(args.mqtt_port) if args.mqtt_port is not None else MQTT_BROKER_PORT
	ip = args.ip if args.ip is not None else DEFAULT_IP
	mac = int(args.mac, 16) if args.mac is not None else RANDOM_MAC

	with VirtualSonoffSC(mqttUrl, mqttPort, ip, mac) as multisensor:
		multisensor.start()
