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

## Quick start

The BeeeOn Gateway is built by the CMake build system:

```
$ mkdir build
$ (cd build && cmake ..)
$ make -C build
```

During development, the BeeeOn Gateway can be started as:

```
$ build/src/beeeon-gateway -c conf/gateway-startup.ini
```

The executable `beeeon-gateway` reads the given configuration file and configuration
files from conf/config.d. Then, the definition of the _main_ instance (of class
LoopRunner) is searched, created and executed. Then, all main application threads
start.

## Hacking the BeeeOn Gateway

In the following sections, you can find some tips how to work with the BeeeOn
Gateway or how to customize it.

### Server-less debugging (TestingCenter)

The BeeeOn Gateway is primarily intended to be controlled from a remote server.
The server allows its users to communicate with gateways and their sensors.
However, for development, it is uncomfortable to always work with a server. For this
purpose, there is an internal debugging component named TestingCenter.

The TestingCenter is disabled as default. It must be enabled explicitly:

```
$ beeeon-gateway -c conf/gateway-startup -Dtesting.center.enable=yes
...
BeeeOn::TestingCenter 20:46:34.627 7052 [Critical] TESTING CENTER IS NOT INTENDED FOR PRODUCTION
BeeeOn::TCPConsole 20:46:34.627 7052 [Information] waiting for connection...
...
```

During startup, it will inform you that the TestingCenter is listening for TCP
connections. The default port is 6000. Use some TCP line oriented tool (e.g. telnet,
netcat) to connect and get the prompt:

```
$ nc localhost 6000
> help
Gateway Testing Center
Commands:
  help - print this help
  exit - exit the console session
  command - dispatch a command into the system
  credentials - manage credentials storage
  device - simulate device in server database
  echo - echo arguments to output separated by space
  wait-queue - wait for new command answers
```

The TestingCenter allows to see some internals of the BeeeOn Gateway runtime.
It provides a very basic help. The most important feature of the TestingCenter
is the possibility to mimic the server. It allows to dispatch commands to the
particular connected devices and receive results.

### Pairing a device via TestingCenter

The BeeeOn Gateway implements the pairing process used in the BeeeOn system.
The pairing process is used for all technologies regardless of their own
pairing mechanisms. Some technologies may require a manual intervention
with the particular devices (Jablotron, Z-Wave).

Pairing has always the two steps:

1. Discover new devices (listen).
2. Confirm devices to be paired (accept).

To pair a device using the TestingCenter, type the following commands into the
TestingCenter prompt:

```
> command listen 30
0x7F3CB0002030 PENDING 0/3
> wait-queue
0x7F3CB0002030 DONE 3/3 SSS
> device list-new
0xa300000000000003
> command device-accept 0xa300000000000003
0x7F3CB0002CD0 PENDING 0/1
> wait-queue
0x7F3CB0002CD0 DONE 1/1 S
```

First, the _listen_ command has discovered there is a single available device with ID `0xa300000000000003`.
Note, that there were 3 Device Managers handling the _listen_ command. Then, we accepted the device by
sending _accept_ command and waiting for the result. This time, only one Device Manager has responded.

A paired device would ship its measured sensor data to the Distributor component that would export it to
the configured Exporters.

### Device Manager

Each supported technology (Jablotron, Z-Wave, ...) is represented by a Device Manager. A Device Manager is
usually a singleton class created once on startup. Its purpose is to translate the BeeeOn common interfaces
to the specifics of the target technology. Each Device Manager is basically a thread (but no necessarily).
It reacts to certain commands dispatched by the component CommandDispatcher. All such commands are asynchronous
and can report the status of execution multiple times until finished. A Device Manager also ensures that
the data delivered from the paired sensors are shipped into the Distributor component (and then exported out).
Device Managers can also report technology-specific events via custom interfaces.

### Configuration

The BeeeOn Gateway is configured at 2 levels of abstraction:

1. user level - mostly INI files
2. developer level - INI and XML files

The _user level_ configuration can be found in the `conf/gateway-startup.ini`. To
configure virtual sensors, see also `conf/virtual-devices.ini`.

The _developer level_ configuration is made by files located in the `conf/config.d`
directory. Those files are loaded automatically during startup and define how to
startup the whole system. The main component that is created and build is called
_main_ and it is defined in the `conf/config.d/factory.xml` file. It denotes an
instance of the LoopRunner class. Its purpose is to start all main application
threads (internally called loops).

#### User level configuration

The _user level_ configuration is divided into several sections according to the
number of subsystems in the BeeeOn Gateway.

##### Core configuration entries

* ssl.certificate.filePath - path to the SSL certificate used to identify the BeeeOn
Gateway when connecting to the BeeeOn Server. Each BeeeOn Gateway should have a unique
certificate signed by the BeeeOn Server authority.

* ssl.privateKey.filePath - path to the SSL private key.

* gateway.id - ID of the BeeeOn Gateway intended for *testing only*. In production, the
ID is extracted from the SSL certificate.

##### Credentials storage facility

The BeeeOn Gateway provides a credentials storage facility. Its purpose is to serve
credentials to Device Managers to authenticate with its sensors properly. The credentials
storage can persist the credentials with a certain level of security. The current
implementation stores all credentials in an encrypted form.

* credentials.file - encrypted file where to persist credentials

* credentials.configuration.root - name of the configuration root element

* credentials.save.delay - persisting of the dirty credentials cache is not done immediately,
instead, it waits for the given amount of seconds

* credentials.crypto.passphrase - secret passphrase intended for encryption; this should be
unique for each BeeeOn Gateway

* credentials.crypto.algorithm - algorithm used for credentials encryption

##### TestingCenter configuration

* testing.center.enable - enable or disable starting of the TestingCenter on startup

* testing.center.address - address to listen on for new connections (should be localhost)

* testing.center.port - port to listen on for new connections

##### Hotplug configuration

Some Device Managers depend on external hardware - USB dongles. To detect that a dongle is present
or disconnected, a hotplug mechanism is used. The preferred hotplug implementation is the udev
(via UDevMonitor). However, there is also the PipeHotplugMonitor, the second hotplug interface
that is controlled via a named pipe from some external sources.

* hotplug.pipe.path - path to the named pipe when the pipeHotplug implementation is used

* hotplug.impl - name of the hotplug mechanism to use (available: udev, pipeHotplug)

##### Exporters configuration

* exporter.pipe.enable - enable the NamedPipeExporter to be registered with the Distributor

* exporter.pipe.path - path to the named pipe file to be used (created if not exists)

* exporter.pipe.format - output format (available: CSV, JSON)

* exporter.mqtt.enable - enable the MosquittoExporter to be registered with the Distributor

* exporter.mqtt.host - host where the MQTT broker is running

* exporter.mqtt.port - port where the MQTT broker is running

* exporter.mqtt.topic - name of topic where to advertise data

* exporter.mqtt.qos - MQTT QoS specification

* exporter.mqtt.clientName - name of the MQTT client

* exporter.mqtt.format - output format (available: CSV, JSON)
