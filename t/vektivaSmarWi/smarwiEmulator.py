import argparse
import http.server
import json
import logging
import paho.mqtt.client as mqtt
import re
import signal
import socketserver
import sys
import time
from threading import Thread, Lock, Event, current_thread

DEFAULT_HTTP_SERVER_PORT = 8080
# HTTP server runs on localhost
DEFAULT_MQTT_URL = "localhost"
DEFAULT_MQTT_PORT = 1883

# Class Thread extended to be able to stop running thread
class StoppableThread(Thread):
	"""Thread class with a stop() method. The thread itself has to check
	regularly for the stopped() condition."""

	def __init__(self, *args, **kwargs):
		super(StoppableThread, self).__init__(*args, **kwargs)
		self._stopEvent = Event()

	def stop(self):
		self._stopEvent.set()

	def stopped(self):
		return self._stopEvent.is_set()


# Class for handling smarwis
class VektivaSmarwiHandler(mqtt.Client):
	def __init__(self, router, *args):
		logging.info("Vektiva Smarwi handler instantiated.")

		self._smarwisData = []
		self._threads = {}
		self._router = router
		mqtt.Client.__init__(self, *args)

	def run(self, url, port):
		self.connect(url, port, 60)
		self.subscribe("ion/#")
		self.loop_start()

	def on_connect(self, mqttc, obj, flags, rc):
		pass
		#print("rc: "+str(rc))

	def on_message(self, mqttc, obj, msg):
		topicSplit = msg.topic.split("/")
		if topicSplit and topicSplit[-1] == "cmd":
			macAddr = topicSplit[-2][1:] # Second from the end and cut the first character out
			msg2 = str(msg.payload)[2:-1]
			msg2 = msg2.replace(";", "/") # in case of open;50 messages
			route = "/" + macAddr + "/" + msg2

			logging.info("MQTT message sending to router (" + route + ")")

			self._router.route(route)

	def on_publish(self, mqttc, obj, mid):
		pass
		#print("mid: "+str(mid))

	def on_subscribe(self, mqttc, obj, mid, granted_qos):
		pass
		#print("Subscribed: "+str(mid)+" "+str(granted_qos))

	def on_log(self, mqttc, obj, level, string):
		pass
		#print(string)

	def smarwis(self):
		return self._smarwisData

	def addSmarwi(self, smarwi):
		if (isinstance(smarwi, Smarwi) and self.getSmarwi(smarwi.getMAC()) == None):
			logging.info("Smarwi (" + smarwi.getMAC() + ") added to the local database.")

			smarwi.on(self)
			self._smarwisData.append(smarwi)
			return "OK"
		return "Err"

	def removeSmarwi(self, macAddr):
		for smarwi in self._smarwisData:
			if macAddr == smarwi.getMAC():
				smarwi.eraseMessages(self)
				self._smarwisData.remove(smarwi)
				return "OK"
		return "Err"

	def getSmarwi(self, macAddr):
		for smarwi in self._smarwisData:
			if macAddr == smarwi.getMAC():
				return smarwi

	def route(self, reqType, body, route):
		macAddr = route[0]
		smarwi = self.getSmarwi(macAddr)
		if (hasattr(smarwi, route[1])): # Checks if function for the second part of URL exists
			method = getattr(smarwi, route[1]) # if exists, proper method is called
			if (route[1] == "error"):
				errno = route[2] if len(route) > 2 else "10"
				thread = StoppableThread(target=smarwi.error, kwargs={'mqttClient':self, 'errno':errno})
			else:
				thread = StoppableThread(target=method, kwargs={'mqttClient':self})

			oldThread  = self._threads.get(macAddr)
			if (oldThread == None or not oldThread.isAlive()): # if there is no old thread
				self._threads.update({macAddr : thread})
			else:
				oldThread.stop()
				self._threads.update({macAddr: thread})

			thread.start()

			return "OK"
		return "Err"

class Router:
	def __init__(self, mqttUrl, mqttPort):
		logging.info("Router instantiated.")

		self._smarwiHandler = VektivaSmarwiHandler(self)
		self._smarwiHandler.run(mqttUrl, mqttPort)

	def serializeSmarwis(self, smarwi):
		properties = smarwi.getProperties()
		return properties

	def route(self, route, reqType = "GET", body = None):
		# Splits route to list by slash
		routeParts = route.split('/')
		# Filters out empty strings e.g. in case of /open
		routeFiltered = list(filter(None, routeParts))

		if (routeFiltered and hasattr(self, routeFiltered[0])): # If list is not empty and checks the first route part
			method = getattr(self, routeFiltered[0]) # If exists, proper method is called
			return method(reqType = reqType, body = body, route = routeFiltered[1:])
		elif (routeFiltered and re.search("^[a-fA-F0-9]{12}$", routeFiltered[0]) != None) and len(routeFiltered) > 1:
			return self.control(reqType = reqType, body = body, route = routeFiltered)
		else: # Else method index is called
			return self.index(reqType = "GET", body = body, route = routeFiltered)

	def index(self, reqType, body, route):
		file = open("index.html", "r")
		page = file.read()
		file.close()
		return page

	def jquery(self, reqType, body, route):
		jqueryFileName = "jquery.min.js"
		try:
			open(jqueryFileName, "r")
		except IOError:
			logging.critical(jqueryFileName + " file does not appear to exist.")
			return

		file = open(jqueryFileName, "r")
		page = file.read()
		file.close()
		return page

	# GET
	# returns list of currently existing Smarwi devices
	# POST
	# creates a new device and adds it to the list
	# DELETE
	# deletes a Smarwi device with the MAC address equal to the MAC address specified in the URL
	def devices(self, reqType, body, route):
		if (reqType == "GET"):
			jsonSmarwis = json.dumps(self._smarwiHandler.smarwis(), default=self.serializeSmarwis)
			return jsonSmarwis

		elif (reqType == "POST"):
			logging.debug(body)

			smarwiJson = json.loads(body)
			if (smarwiJson.get("macAddr") == None or (re.search("^[a-fA-F0-9]{12}$", smarwiJson.get("macAddr")) == None)):
				return "Err"

			macAddr = smarwiJson.get("macAddr").lower()
			smarwi = Smarwi(macAddr)
			return self._smarwiHandler.addSmarwi(smarwi)
		if (reqType == "DELETE"):
			return self._smarwiHandler.removeSmarwi(route[0].lower())

	def control(self, reqType, body, route):
		route[0] = route[0].lower()
		return self._smarwiHandler.route(reqType, body, route)

class VektivaHTTPHandler(http.server.BaseHTTPRequestHandler):
	def __init__(self, router, *args):
		self._router = router
		http.server.BaseHTTPRequestHandler.__init__(self, *args)

	def do_GET(self):
		resp = self._router.route(self.path)
		self.send_response(200)
		if (resp == None or "<html>" in resp):
			self.send_header('content-type', 'text/html')
		else:
			self.send_header('content-type', 'application/json')
		self.send_header('content-type', 'charset="utf-8"')
		if (resp != None):
			self.send_header('content-length', len(resp))
		self.end_headers()

		if (resp != None):
			self.wfile.write(resp.encode())

		return VektivaHTTPHandler

	def do_POST(self):
		contentLen = int(self.headers['content-length'])
		postBody = self.rfile.read(contentLen).decode('utf-8')
		resp = self._router.route(self.path, "POST", postBody)

		self.send_response(200)
		if (resp == None or "<html>" in resp):
			self.send_header('content-type', 'text/html')
		else:
			self.send_header('content-type', 'application/json')
		self.send_header('content-type', 'charset="utf-8"')
		if (resp != None):
			self.send_header('content-length', len(resp))
		self.end_headers()

		if (resp != None):
			self.wfile.write(resp.encode())
		return VektivaHTTPHandler

	def do_DELETE(self):
		resp = self._router.route(self.path, "DELETE")
		self.send_response(200)
		if (resp == None or "<html>" in resp):
			self.send_header('content-type', 'text/html')
		else:
			self.send_header('content-type', 'application/json')
		self.send_header('content-type', 'charset="utf-8"')
		if (resp != None):
			self.send_header('content-length', len(resp))
		self.end_headers()

		if (resp != None):
			self.wfile.write(resp.encode())
		return VektivaHTTPHandler

class http_server:
	def __init__(self, port, router):
		logging.info("HTTP server instantiated.")

		signal.signal(signal.SIGINT, self.emulatorShutdown)
		signal.signal(signal.SIGTERM, self.emulatorShutdown)

		def handler(*args):
			VektivaHTTPHandler(router, *args)

		self._router = router

		socketserver.TCPServer.allow_reuse_address = True
		self.httpd = socketserver.TCPServer(("", port), handler)
		self.httpd.serve_forever()

	def emulatorShutdown(self, signum, frame):
		logging.info("Emulator is shutting down.")

		def serverShutdown(server):
			server.shutdown()

		thread = StoppableThread(target=serverShutdown, kwargs={'server':self.httpd})
		thread.start()
		self.httpd.server_close()

		logging.info("Server was shutted down.")

		devicesJSONString = self._router.devices("GET", "", "")
		devices = json.loads(devicesJSONString)

		logging.debug("Devices: " + devicesJSONString)

		for device in devices:
			logging.info("Device " + device["macAddr"] + " is being deleted.")

			self._router.devices("DELETE", "", [device["macAddr"]])

		logging.info("All devices has been deleted.")

#Smarwi class representing standalone device
class Smarwi:
	def __init__(self, macAddr):
		self._macAddr = macAddr
		self._online = False
		self._isOpen = False
		self._lock = Lock()
		self._errno = "0"
		self._errorScheduled = False
		self._fixed = True
		self._statusno = 250
		self._ok = 1
		self._ro = 0

	def getMAC(self):
		return self._macAddr

	def getProperties(self):
		return {
			'macAddr': self._macAddr,
			'online': self._online,
			'isOpen': self._isOpen,
			'errno': self._errno,
			'errorScheduled': self._errorScheduled,
			'fixed': self._fixed,
			'statusno': self._statusno,
			'ok': self._ok,
			'ro': self._ro
		}

	def stop(self, mqttClient):
		self._lock.acquire()
		try:
			if self._online:
				self._fixed = False
				self._errno = "0"
				self._errorScheduled = False
				self._statusno = 250
				self._ok = 1
				self._ro = 0

				self.status(mqttClient)
		finally:
			self._lock.release()

	def fix(self, mqttClient):
		self._lock.acquire()
		try:
			if self._online:
				self._statusno = 250
				self._ok = 1
				self._ro = 0
				self._fixed = True

				self.status(mqttClient)
		finally:
			self._lock.release()

	def status(self, mqttClient):
		if self._online:
			mqttClient.publish("ion/dowaroxby/%" + self._macAddr + "/status",
				't:swr\n'\
				's:' + str(self._statusno) + '\n'\
				'e:' + ("0" if self._errorScheduled else self._errno) + '\n'\
				'ok:' + ("1" if self._errorScheduled or self._errno == "0" else "0") + '\n'\
				'ro:' + str(self._ro) + '\n'\
				'pos:' + ("o" if self._isOpen else "c") + '\n'\
				'fix:' + ("1" if self._fixed else "0") + '\n'\
				'a:-98\n'\
				'fw:3.4.1-15-g3d0f\n'\
				'mem:25344\n'\
				'up:583521029\n'\
				'ip:268446218\n'\
				'cid:xsismi01\n'\
				'rssi:-56\n'\
				'time:' + str(int(time.time())) + '\n'\
				'wm:1\n'\
				'wp:1\n'\
				'wst:3\n')

	def error(self, mqttClient, errno = "10"):
		self._lock.acquire()
		if (self._online):
			self._errno = errno
			self._errorScheduled = True

		self._lock.release()

	def on(self, mqttClient):
		self._lock.acquire()
		try:
			if not (self._online):
				self._online = True
				mqttClient.publish("ion/dowaroxby/%" + self._macAddr + "/online", "1", retain = True)
				self.status(mqttClient)
		finally:
			self._lock.release()

	def off(self, mqttClient):
		self._lock.acquire()
		try:
			if (self._online):
				self._online = False
				mqttClient.publish("ion/dowaroxby/%" + self._macAddr + "/online", "0", retain = True)
				self.status(mqttClient)
		finally:
			self._lock.release()

	def eraseMessages(self, mqttClient):
		self._lock.acquire()
		try:
			mqttClient.publish("ion/dowaroxby/%" + self._macAddr + "/online", '''''', retain = True)
			mqttClient.publish("ion/dowaroxby/%" + self._macAddr + "/status", '''''', retain = True)
			self._online = False
		finally:
			self._lock.release()

	def open(self, mqttClient):
		self._lock.acquire()
		try:
			# If error happened, SmarWi can not be controlled. It waits until "stop" message
			if ((not self._errorScheduled) and (self._errno != "0")):
				return

			if (self._isOpen):
				self._statusno = 212
				self._fixed = True
				self._isOpen = True
				# Sending the first message
				self.status(mqttClient)
				#Before the second step, check if error should happen. If so,
				#error message is published and then method ends.
				if (self._errorScheduled and self._errno != "0"):
					self._statusno = 10
					self._ok = 0
					self._errorScheduled = False
					self.status(mqttClient)
					return

				time.sleep(3)
				if current_thread().stopped():
					return

				self._statusno = 210
				self.status(mqttClient)

				time.sleep(3)
				if current_thread().stopped():
					return

				self._statusno = 250
				self.status(mqttClient)
			else:
				self._statusno = 200
				self._fixed = True
				self._isOpen = True
				# Sending the first message
				self.status(mqttClient)
				#Before the second step, check if error should happen. If so,
				#error message is published and then method ends.
				if (self._errorScheduled and self._errno != "0"):
					self._statusno = 10
					self._ok = 0
					self._errorScheduled = False
					self.status(mqttClient)
					return

				time.sleep(3)
				if current_thread().stopped():
					return

				self._statusno = 210
				self.status(mqttClient)

				time.sleep(3)
				if current_thread().stopped():
					return

				self._statusno = 250
				self.status(mqttClient)
		finally:
			self._lock.release()

	def close(self, mqttClient):
		self._lock.acquire()
		try:
			# If error happened, SmarWi can not be controlled. It waits until "stop"
			if ((not self._errorScheduled) and (self._errno != "0")):
				return

			if (self._isOpen):
				self._statusno = 220
				self._isOpen = True
				self._fixed = True
				self.status(mqttClient)
				#Before the second step, check if error should happen. If so,
				#error message is published and then method ends.
				if (self._errorScheduled and self._errno != "0"):
					self._statusno = 10
					self._ok = 0
					self._errorScheduled = False
					self.status(mqttClient)
					return

				time.sleep(3)
				if current_thread().stopped():
					return

				self._statusno = 230
				self._isOpen = False
				self.status(mqttClient)

				time.sleep(3)
				if current_thread().stopped():
					return

				self._statusno = 250
				self.status(mqttClient)
			else:
				self._statusno = 232
				self._isOpen = False
				self._fixed = True
				self.status(mqttClient)
				#Before the second step, check if error should happen. If so,
				#error message is published and then method ends.
				if (self._errorScheduled and self._errno != "0"):
					self._statusno = 10
					self._ok = 0
					self._errorScheduled = False
					self.status(mqttClient)
					return

				time.sleep(3)
				if current_thread().stopped():
					return

				self._statusno = 232
				self._isOpen = True
				self.status(mqttClient)

				time.sleep(3)
				if current_thread().stopped():
					return
				self._statusno = 234
				self._isOpen = True
				self.status(mqttClient)

				time.sleep(3)
				if current_thread().stopped():
					return

				self._statusno = 230
				self._isOpen = False
				self.status(mqttClient)

				time.sleep(3)
				if current_thread().stopped():
					return

				self._statusno = 250
				self._isOpen = False
				self.status(mqttClient)
		finally:
			self._lock.release()

class main:
	def __init__(self, httpServerPort, mqttUrl, mqttPort):
		self.router = Router(mqttUrl, mqttPort)
		self.server = http_server(httpServerPort, self.router)

if __name__ == '__main__':
	logging.basicConfig(level = logging.DEBUG)

	parser = argparse.ArgumentParser()
	parser.add_argument("--http-port", metavar="HTTP_SERVER_PORT",
		type=int, help="The emulator will run on specified port. The default port is 8080.")
	parser.add_argument("--mqtt-port", metavar="MQTT_BROKER_PORT",
		type=int, help="The MQTT client will attempt to connect on the specified port. The default port is 1883.")
	parser.add_argument("--mqtt-url", metavar="MQTT_BROKER_URL",
		help="The MQTT client will attempt to connect to the specified URL. The default URL is localhost.")
	args = parser.parse_args()

	httpServerPort = args.http_port if args.http_port is not None else DEFAULT_HTTP_SERVER_PORT
	mqttUrl = args.mqtt_url if args.mqtt_url is not None else DEFAULT_MQTT_URL
	mqttPort = args.mqtt_port if args.mqtt_port is not None else DEFAULT_MQTT_PORT

	m = main(httpServerPort, mqttUrl, mqttPort)
