#!/bin/bash

python3 -m unittest \
	gateway/0000core/TestCoreModules.py \
	gateway/0001vdev/*.py \
	gateway/0003psdev/*.py \
	gateway/0004belkin/*.py \
	gateway/0005philips/*.py \
	gateway/0006homematic/*.py \
	gateway/0007sonoff/*.py \
	gateway/0008vektiva/*.py
