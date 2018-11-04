#!/usr/bin/env python3

"""
Script for oscillation detection of bluetooth device presence. At first
the script connects to the MQTT broker from which receive the message sent
by the beeeon gateway. After that it processes the messages and detects the
fast changes. The oscillation is detected so that the change of the presence of
device occurs after less than 5 measurements.
"""

import sys
import argparse
import json
import logging
import paho.mqtt.client as mqtt

CLIENT_NAME = 'OscillationDetector'
BROKER_ADDRESS = '127.0.0.1'
DEFAULT_TOPIC = 'BeeeOnOut'
PREFIX_BLUETOOTH = '0xa6'
MAX_SAME_MEASUREMENTS = 5

class DeviceData:
	def __init__(self, deviceID, availability):
		self._deviceID = deviceID
		self._availability = availability
		self._sameMeasurements = 1

	def updateAvailability(self, availability):
		if availability == 1 and self._availability == 0 and self._sameMeasurements >= MAX_SAME_MEASUREMENTS:
			logging.info("device %s is available" % (self._deviceID))

		if self._availability != availability:
			if self._sameMeasurements in range(1, MAX_SAME_MEASUREMENTS):
				logging.info("presence of the device %s is oscillating(change occurred after %s measurements)" %
					(self._deviceID, self._sameMeasurements))

			self._availability = availability
			self._sameMeasurements = 1
		else:
			self._sameMeasurements += 1

		if self._availability == 0 and self._sameMeasurements == MAX_SAME_MEASUREMENTS:
			logging.info("device %s is unavailable" % (self._deviceID))

def on_message(client, devices, message):
	data = json.loads(message.payload.decode('utf-8'))

	deviceID = data["device_id"]
	if deviceID[0:4] != PREFIX_BLUETOOTH:
		return

	availability = data["data"][0]["value"]
	if deviceID not in devices.keys():
		devices[deviceID] = DeviceData(deviceID, availability)
	else:
		devices[deviceID].updateAvailability(availability)

if __name__ == "__main__":
	logging.basicConfig(level = logging.DEBUG)

	parser = argparse.ArgumentParser(description=sys.modules[__name__].__doc__)
	parser.add_argument("--address", metavar="BROKER_ADDRESS",
		help="Broker address from which messages will be received.")
	parser.add_argument("--topic", metavar="MQTT_TOPIC",
		help="From this topic will be recieved bluetooth device reports.")
	args = parser.parse_args()

	address = args.address if args.address is not None else BROKER_ADDRESS
	topic = args.topic if args.topic is not None else DEFAULT_TOPIC
	devices = dict()

	logging.info("creating MQTT client with name %s" % (CLIENT_NAME));
	client = mqtt.Client(CLIENT_NAME)
	client.user_data_set(devices)
	client.on_message = on_message

	logging.info("connecting to broker on address %s" % (address));
	client.connect(address)

	logging.info("subscribing to topic %s" % (topic));
	client.subscribe(topic)

	logging.info("starting to process messages");
	try:
		client.loop_forever()
	except KeyboardInterrupt:
		client.disconnect()

