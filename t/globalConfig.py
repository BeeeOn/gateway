#!/bin/python3

from os import sys, path
baseDir = path.dirname(path.abspath(__file__))

sys.path.append(baseDir + "/testing-lib")

mosquittoBinary = "/usr/sbin/mosquitto"
mosquittoPort = 6699

gatewayBinary = baseDir + "/../build_x86/src/beeeon-gateway"
gatewayStartup = baseDir + "/../conf/gateway-testing.ini"
gwsPort = 8850

import logging
logging.basicConfig(level=logging.DEBUG)
