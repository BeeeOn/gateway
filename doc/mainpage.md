# Introduction

BeeeOn Gateway is a software platform for integrating IoT technologies from
different vendors. It abstracts from the sensoric network specifics and provides
a unified interface to control sensors and actuator devices.

The BeeeOn Gateway is written in C++ and is divided into several components,
especially:

* DeviceManager - a concept (usually a class) that implements specifics for a
certain sensoric network. E.g. a single DeviceManager implementation manages
all Z-Wave devices.
* Distributor - a component that allows to distribute collected sensor data
to a number of Exporters.
* Exporter - a component that delivers sensor data to some collection point.
The collection point can be MQTT, some cloud service, Unix Pipe, etc.
* CommandDispatcher - a component that distributes commands into the BeeeOn
Gateway. A class that implements the CommandHandler interface can be registered
with the CommandDispatcher to be able to handle incoming commands. Commands
are usually inserted into the BeeeOn Gateway from a remote control server.
