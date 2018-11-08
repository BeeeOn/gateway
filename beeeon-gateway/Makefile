#
# Copyright (c) 2016, 2017, Brno University of Technology
#
# This is free software, licensed under the BSD-3-Clause.
#
# This file is based on
# https://github.com/BeeeOn/openwrt-packages/tree/master/beeeon-gateway
#

include $(TOPDIR)/rules.mk

PKG_NAME:=beeeon-gateway
PKG_VERSION:=v2018.07.1
PKG_RELEASE:=1
INIT_SCRIPT_NAME:=$(PKG_NAME)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/BeeeOn/gateway.git
PKG_SOURCE_VERSION:=9535495ca000af8d05f421ceb6f3d385cad38328

PKG_BUILD_PARALLEL:=1
CMAKE_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_OPTIONS += \
	-DCMAKE_INSTALL_PREFIX=/usr \
	-DCMAKE_BUILD_TYPE:STRING=Release \
	-DENABLE_TESTS=OFF \
	-DENABLE_UDEV=OFF \
	-DENABLE_BELKIN_WEMO=$(if $(CONFIG_BEEEON_GATEWAY_BELKIN_WEMO),ON,OFF) \
	-DENABLE_BLE_SMART=$(if $(CONFIG_BEEEON_GATEWAY_BLE_SMART),ON,OFF) \
	-DENABLE_BLUETOOTH_AVAILABILITY=$(if $(CONFIG_BEEEON_GATEWAY_BLUETOOTH_AVAILABILITY),ON,OFF) \
	-DENABLE_HCI_INFO_REPORTER=$(if $(CONFIG_BEEEON_GATEWAY_HCI_INFO_REPORTER),ON,OFF) \
	-DENABLE_FITP=$(if $(CONFIG_BEEEON_GATEWAY_FITP),ON,OFF) \
	-DENABLE_IQRF=$(if $(CONFIG_BEEEON_GATEWAY_IQRF),ON,OFF) \
	-DENABLE_JABLOTRON=$(if $(CONFIG_BEEEON_GATEWAY_JABLOTRON),ON,OFF) \
	-DENABLE_PHILIPS_HUE=$(if $(CONFIG_BEEEON_GATEWAY_PHILIPS_HUE),ON,OFF) \
	-DENABLE_PRESSURE_SENSOR=$(if $(CONFIG_BEEEON_GATEWAY_PRESSURE_SENSOR),ON,OFF) \
	-DENABLE_VIRTUAL_DEVICES=$(if $(CONFIG_BEEEON_GATEWAY_VIRTUAL_DEVICES),ON,OFF) \
	-DENABLE_VPT=$(if $(CONFIG_BEEEON_GATEWAY_VPT),ON,OFF) \
	-DENABLE_ZWAVE=$(if $(CONFIG_BEEEON_GATEWAY_ZWAVE),ON,OFF) \
	-DENABLE_TESTING_CENTER=$(if $(CONFIG_BEEEON_GATEWAY_TESTING_CENTER),ON,OFF)

TARGET_CXXFLAGS += -DPOCO_NO_FPENVIRONMENT

define Package/beeeon-gateway/config
	source "$(SOURCE)/Config.in"
endef

ifeq ($(CONFIG_BEEEON_GATEWAY_ID_RANDOM),y)
PKG_BUILD_DEPENDS += python3/host
endif

ifeq ($(CONFIG_BEEEON_GATEWAY_BLE_SMART),y)
_IPKG_DEPENDS += +bluez-daemon +bluez-libs +glib2
endif

ifeq ($(CONFIG_BEEEON_GATEWAY_BLUETOOTH_AVAILABILITY),y)
_IPKG_DEPENDS += +bluez-libs +glib2
endif

ifeq ($(CONFIG_BEEEON_GATEWAY_HCI_INFO_REPORTER),y)
_IPKG_DEPENDS += +bluez-libs +glib2
endif

ifeq ($(CONFIG_BEEEON_GATEWAY_BLUETOOTH_AVAILABILITY),y)
_IPKG_DEPENDS += +libmosquittopp
endif

_IPKG_DEPENDS += +BEEEON_GATEWAY_ZWAVE:libopenzwave
_IPKG_DEPENDS += +BEEEON_GATEWAY_ZWAVE:openzwave-config

define Package/beeeon-gateway
	TITLE:=BeeeOn Gateway
	SECTION:=cesnet
	CATEGORY:=CESNET
	DEPENDS:=+poco-all $(_IPKG_DEPENDS)
	MAINTAINER:=Jan Viktorin <iviktorin@fit.vutbr.cz>
endef

define Package/beeeon-gateway/description
 Main application for BeeeOn Gateway.
endef

BEEEON_GATEWAY_MODULE_ENABLE  += $(if $(CONFIG_BEEEON_GATEWAY_BELKIN_WEMO),belkinwemo)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_BELKIN_WEMO),,belkinwemo)

BEEEON_GATEWAY_MODULE_ENABLE  += $(if $(CONFIG_BEEEON_GATEWAY_BLE_SMART),blesmart)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_BLE_SMART),,blesmart)

BEEEON_GATEWAY_MODULE_ENABLE  += $(if $(CONFIG_BEEEON_GATEWAY_BLUETOOTH_AVAILABILITY),bluetooth.availability)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_BLUETOOTH_AVAILABILITY),,bluetooth.availability)

BEEEON_GATEWAY_MODULE_ENABLE  += $(if $(CONFIG_BEEEON_GATEWAY_HCI_INFO_REPORTER),bluetooth.reporting)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_HCI_INFO_REPORTER),,bluetooth.reporting)

BEEEON_GATEWAY_MODULE_ENABLE  += $(if $(CONFIG_BEEEON_GATEWAY_FITP),fitp)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_FITP),,fitp)

BEEEON_GATEWAY_MODULE_ENABLE  += $(if $(CONFIG_BEEEON_GATEWAY_IQRF),iqrf)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_IQRF),,iqrf)

BEEEON_GATEWAY_MODULE_ENABLE  += $(if $(CONFIG_BEEEON_GATEWAY_JABLOTRON),jablotron)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_JABLOTRON),,jablotron)

BEEEON_GATEWAY_MODULE_ENABLE  += $(if $(CONFIG_BEEEON_GATEWAY_MQTT_EXPORTER),exporter.mqtt)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_MQTT_EXPORTER),,exporter.mqtt)

BEEEON_GATEWAY_MODULE_ENABLE  += $(if $(CONFIG_BEEEON_GATEWAY_PHILIPS_HUE),philipshue)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_PHILIPS_HUE),,philipshue)

BEEEON_GATEWAY_MODULE_ENABLE  += $(if $(CONFIG_BEEEON_GATEWAY_PRESSURE_SENSOR),psdev)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_PRESSURE_SENSOR),,psdev)

BEEEON_GATEWAY_MODULE_ENABLE  += $(if $(CONFIG_BEEEON_GATEWAY_VIRTUAL_DEVICES),vdev)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_VIRTUAL_DEVICES),,vdev)

BEEEON_GATEWAY_MODULE_ENABLE += $(if $(CONFIG_BEEEON_GATEWAY_VPT),vpt)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_VPT),,vpt)

BEEEON_GATEWAY_MODULE_ENABLE += $(if $(CONFIG_BEEEON_GATEWAY_ZWAVE),zwave)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_ZWAVE),,zwave)

BEEEON_GATEWAY_MODULE_ENABLE += $(if $(CONFIG_BEEEON_GATEWAY_TESTING_CENTER),testing.center)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_TESTING_CENTER),,testing.center)

BEEEON_GATEWAY_MODULE_ENABLE += $(if $(CONFIG_BEEEON_GATEWAY_TESTING_COLLECTOR),testing.collector)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_TESTING_COLLECTOR),,testing.collector)

BEEEON_GATEWAY_MODULE_ENABLE += $(if $(CONFIG_BEEEON_GATEWAY_GWS),gws)
BEEEON_GATEWAY_MODULE_DISABLE += $(if $(CONFIG_BEEEON_GATEWAY_GWS),,gws)

# FIXME: missing mosquitto dependency
BEEEON_GATEWAY_MODULE_DISABLE += exporter.mqtt

# Generate random BeeeOn Gateway ID
ifeq ($(CONFIG_BEEEON_GATEWAY_ID_RANDOM),y)
BEEEON_GATEWAY_MODULE_ENABLE += gateway.id

define BEEEON_GATEWAY_ID_INSTALL
	echo gateway.id = `python3 $(PKG_BUILD_DIR)/tools/gateway-genid.py` \
		>> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-gws.properties
	echo ssl.key = >> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-gws.properties
	echo ssl.certificate = >> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-gws.properties
endef
endif

# Configure the manually inserted BeeeOn Gateway ID
ifeq ($(CONFIG_BEEEON_GATEWAY_ID_CUSTOM),y)
BEEEON_GATEWAY_MODULE_ENABLE += gateway.id

define BEEEON_GATEWAY_ID_INSTALL
	echo gateway.id = $(CONFIG_BEEEON_GATEWAY_ID_CUSTOM_VALUE) \
		>> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-gws.properties
	echo ssl.key = >> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-gws.properties
	echo ssl.certificate = >> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-gws.properties
endef
endif

# Use SSL/TLS to provide BeeeOn Gateway ID
ifeq ($(CONFIG_BEEEON_GATEWAY_ID_CERT),y)
BEEEON_GATEWAY_MODULE_DISABLE += gateway.id

define BEEEON_GATEWAY_ID_INSTALL
	$(INSTALL_DIR) $(PKG_INSTALL_DIR)/etc/ssl/beeeon/private
	$(CP) $(CONFIG_BEEEON_GATEWAY_ID_CERT_PKEY) \
		$(PKG_INSTALL_DIR)/etc/ssl/beeeon/private/beeeon_gateway.key

	$(INSTALL_DIR) $(PKG_INSTALL_DIR)/etc/ssl/beeeon/certs
	$(CP) $(CONFIG_BEEEON_GATEWAY_ID_CERT_FILE) \
		$(PKG_INSTALL_DIR)/etc/ssl/beeeon/certs/beeeon_gateway.crt
	$(CP) $(CONFIG_BEEEON_GATEWAY_ID_CERT_AUTHORITY) \
		$(PKG_INSTALL_DIR)/etc/ssl/beeeon/certs/beeeon_root_ca.crt
endef
endif

ifeq ($(CONFIG_BEEEON_GATEWAY_MQTT_EXPORTER),y)
define BEEEON_GATEWAY_MQTT_INSTALL
	echo exporter.mqtt.host = $(CONFIG_BEEEON_GATEWAY_MQTT_EXPORTER_HOST) \
		>> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-mqtt.properties
	echo exporter.mqtt.port = $(CONFIG_BEEEON_GATEWAY_MQTT_EXPORTER_PORT) \
		>> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-mqtt.properties
	echo exporter.mqtt.topic = $(CONFIG_BEEEON_GATEWAY_MQTT_EXPORTER_TOPIC) \
		>> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-mqtt.properties
	echo exporter.mqtt.clientID = $(CONFIG_BEEEON_GATEWAY_MQTT_EXPORTER_CLIENT_ID) \
		>> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-mqtt.properties
endef
endif

define Package/beeeon-gateway/install
	$(if $(CONFIG_BEEEON_GATEWAY_PRESSURE_SENSOR_PATH),\
		echo psdev.path = $(CONFIG_BEEEON_GATEWAY_PRESSURE_SENSOR_PATH) \
		> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-pressure-sensor.properties)

	$(if $(CONFIG_BEEEON_GATEWAY_VIRTUAL_DEVICES_CUSTOM_CONFIG),\
		$(CP) $(CONFIG_BEEEON_GATEWAY_VIRTUAL_DEVICES_CUSTOM_CONFIG) \
		$(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/virtual-devices.ini)

	echo hotplug.impl = pipeHotplug \
		> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-hotplug.properties

	$(RM) $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-modules.properties
	$(foreach module,$(BEEEON_GATEWAY_MODULE_ENABLE),\
		echo $(module).enable = yes \
		>> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-modules.properties;)
	$(foreach module,$(BEEEON_GATEWAY_MODULE_DISABLE),\
		echo $(module).enable = no \
		>> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-modules.properties;)

	$(RM) $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-gws.properties

	$(if $(CONFIG_BEEEON_GATEWAY_GWS_HOST),\
		echo gws.host = $(CONFIG_BEEEON_GATEWAY_GWS_HOST) \
		>> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-gws.properties)
	$(if $(CONFIG_BEEEON_GATEWAY_GWS_PORT),\
		echo gws.port = $(CONFIG_BEEEON_GATEWAY_GWS_PORT) \
		>> $(PKG_INSTALL_DIR)/usr/etc/beeeon/gateway/config.d/x-gws.properties)

	$(BEEEON_GATEWAY_ID_INSTALL)
	$(BEEEON_GATEWAY_MQTT_INSTALL)

	sed -i "s|/var/cache|/usr/lib|" ${PKG_INSTALL_DIR}/usr/etc/beeeon/gateway/gateway-startup.ini
	mkdir -p $(1)/usr/lib/beeeon/gateway

	$(INSTALL_DIR) $(1)
	$(CP) -r $(PKG_INSTALL_DIR)/* $(1)
	$(RM) -r $(1)/var

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/$(INIT_SCRIPT_NAME) $(1)/etc/init.d/$(INIT_SCRIPT_NAME)

	$(INSTALL_DIR) $(1)/etc/hotplug.d
	$(CP) ./files/beeeon-hotplug-functions $(1)/etc/hotplug.d/beeeon-hotplug-functions
	$(INSTALL_DIR) $(1)/etc/hotplug.d/tty
	$(CP) ./files/tty-beeeon-dongle $(1)/etc/hotplug.d/tty/30-beeeon-dongle
	$(INSTALL_DIR) $(1)/etc/hotplug.d/bluetooth
	$(CP) ./files/bluetooth-beeeon-dongle $(1)/etc/hotplug.d/bluetooth/30-beeeon-dongle
endef

define Package/beeeon-gateway/postinst
#!/bin/sh
if [ -z "$${IPKG_INSTROOT}" ]; then
  echo "Enabling and starting $(PKG_NAME) service"
  /etc/init.d/$(INIT_SCRIPT_NAME) enable
  /etc/init.d/$(INIT_SCRIPT_NAME) start
fi
exit 0
endef

define Package/beeeon-gateway/prerm
#!/bin/sh
if [ -z "$${IPKG_INSTROOT}" ]; then
  echo "Disabling and stopping $(PKG_NAME) service"
  /etc/init.d/$(INIT_SCRIPT_NAME) disable
  /etc/init.d/$(INIT_SCRIPT_NAME) stop
fi
exit 0
endef

$(eval $(call BuildPackage,beeeon-gateway))
