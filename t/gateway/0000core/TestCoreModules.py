import os
from os import sys, path
sys.path.append(path.dirname(path.abspath(__file__)))

import testCoreBase

class TestFitp(testCoreBase.TestCoreBase):
	def setUp(self):
		self.GR.overrideUpdate("fitp.enable","yes")
		super().setUp()

	def test_accept(self):
		self.generic_test_accept("0xa1ffffffffffffff")

	def test_unpair(self):
		self.generic_test_unpair("0xa1ffffffffffffff")

class TestPsdev(testCoreBase.TestCoreBase):
	def setUp(self):
		self.GR.overrideUpdate("psdev.enable","yes")
		super().setUp()

	def test_accept(self):
		self.generic_test_accept("0xa2ffffffffffffff")

	def test_unpair(self):
		self.generic_test_unpair("0xa2ffffffffffffff")

class TestBluetooth(testCoreBase.TestCoreBase):
	def setUp(self):
		self.GR.overrideUpdate("bluetooth.availability.enable","yes")
		super().setUp()

	def test_accept(self):
		self.generic_test_accept("0xa6ffffffffffffff")

	def test_unpair(self):
		self.generic_test_unpair("0xa6ffffffffffffff")

class TestVdev(testCoreBase.TestCoreBase):
	def setUp(self):
		self.GR.overrideUpdate("vdev.enable","yes")
		super().setUp()

	def test_accept(self):
		self.generic_test_accept("0xa3ffffffffffffff")

	def test_unpair(self):
		self.generic_test_unpair("0xa3ffffffffffffff")

	def test_setValue(self):
		self.generic_test_setValue("0xa3ffffffffffffff")

class TestJablotron(testCoreBase.TestCoreBase):
	def setUp(self):
		self.GR.overrideUpdate("jablotron.enable","yes")
		super().setUp()

	def test_accept(self):
		self.generic_test_accept("0x09ffffffffffffff")

	def test_unpair(self):
		self.generic_test_unpair("0x09ffffffffffffff")

	def test_setValue(self):
		self.generic_test_setValue("0x09ffffffffffffff")

class TestVPT(testCoreBase.TestCoreBase):
	def setUp(self):
		self.GR.overrideUpdate("vpt.enable","yes")
		super().setUp()

	def test_accept(self):
		self.generic_test_accept("0xa4ffffffffffffff")

	def test_unpair(self):
		self.generic_test_unpair("0xa4ffffffffffffff")

	def test_setValue(self):
		self.generic_test_setValue("0xa4ffffffffffffff")

class TestPhilipshue(testCoreBase.TestCoreBase):
	def setUp(self):
		self.GR.overrideUpdate("philipshue.enable","yes")
		super().setUp()

	def test_accept(self):
		self.generic_test_accept("0xabffffffffffffff")

	def test_unpair(self):
		self.generic_test_unpair("0xabffffffffffffff")

	def test_setValue(self):
		self.generic_test_setValue("0xabffffffffffffff")

class TestZwave(testCoreBase.TestCoreBase):
	def setUp(self):
		self.GR.overrideUpdate("zwave.enable","yes")
		super().setUp()

	def test_accept(self):
		self.generic_test_accept("0xa8ffffffffffffff")

	def test_unpair(self):
		self.generic_test_unpair("0xa8ffffffffffffff")

	def test_setValue(self):
		self.generic_test_setValue("0xa8ffffffffffffff")

class TestBLESmart(testCoreBase.TestCoreBase):
	def setUp(self):
		self.GR.overrideUpdate("blesmart.enable","yes")
		super().setUp()

	def test_accept(self):
		self.generic_test_accept("0xacffffffffffffff")

	def test_unpair(self):
		self.generic_test_unpair("0xacffffffffffffff")

	def test_setValue(self):
		self.generic_test_setValue("0xacffffffffffffff")

class TestVektiva(testCoreBase.TestCoreBase):
	def setUp(self):
		self.GR.overrideUpdate("vektiva.enable","yes")
		super().setUp()

	def test_accept(self):
		self.generic_test_accept("0xadffffffffffffff")

	def test_unpair(self):
		self.generic_test_unpair("0xadffffffffffffff")

	def test_setValue(self):
		self.generic_test_setValue("0xadffffffffffffff")

class TestSonoff(testCoreBase.TestCoreBase):
	def setUp(self):
		self.GR.overrideUpdate("sonoff.enable","yes")
		super().setUp()

	def test_accept(self):
		self.generic_test_accept("0xaeffffffffffffff")

	def test_unpair(self):
		self.generic_test_unpair("0xaeffffffffffffff")

class TestConrad(testCoreBase.TestCoreBase):
	def setUp(self):
		self.GR.overrideUpdate("conrad.enable","yes")
		super().setUp()

	def test_accept(self):
		self.generic_test_accept("0xafffffffffffffff")

	def test_unpair(self):
		self.generic_test_unpair("0xafffffffffffffff")

	def test_setValue(self):
		self.generic_test_setValue("0xafffffffffffffff")

class TestAll(testCoreBase.TestCoreBase):
	def setUp(self):
		self.GR.overrideUpdate("psdev.enable","yes")
		self.GR.overrideUpdate("fitp.enable","yes")
		self.GR.overrideUpdate("zwave.enable","yes")
		self.GR.overrideUpdate("bluetooth.availability.enable","yes")
		self.GR.overrideUpdate("vdev.enable","yes")
		self.GR.overrideUpdate("jablotron.enable","yes")
		self.GR.overrideUpdate("vpt.enable","yes")
		self.GR.overrideUpdate("philipshue.enable","yes")
		self.GR.overrideUpdate("blesmart.enable","yes")
		self.GR.overrideUpdate("vektiva.enable","yes")
		self.GR.overrideUpdate("sonoff.enable","yes")
		self.GR.overrideUpdate("conrad.enable","yes")
		super().setUp()

	def test_accept_fitp(self):
		self.generic_test_accept("0xa1ffffffffffffff")

	def test_unpair_fitp(self):
		self.generic_test_unpair("0xa1ffffffffffffff")

	def test_accept_psdev(self):
		self.generic_test_accept("0xa2ffffffffffffff")

	def test_unpair_psdev(self):
		self.generic_test_unpair("0xa2ffffffffffffff")

	def test_accept_bluetooth(self):
		self.generic_test_accept("0xa6ffffffffffffff")

	def test_unpair_bluetooth(self):
		self.generic_test_unpair("0xa6ffffffffffffff")

	def test_accept_vdev(self):
		self.generic_test_accept("0xa3ffffffffffffff")

	def test_unpair_vdev(self):
		self.generic_test_unpair("0xa3ffffffffffffff")

	def test_setValue_vdev(self):
		self.generic_test_setValue("0xa3ffffffffffffff")

	def test_accept_jablotron(self):
		self.generic_test_accept("0x09ffffffffffffff")

	def test_unpair_jablotron(self):
		self.generic_test_unpair("0x09ffffffffffffff")

	def test_setValue_jablotron(self):
		self.generic_test_setValue("0x09ffffffffffffff")

	def test_accept_vpt(self):
		self.generic_test_accept("0xa4ffffffffffffff")

	def test_unpair_vpt(self):
		self.generic_test_unpair("0xa4ffffffffffffff")

	def test_setValue_vpt(self):
		self.generic_test_setValue("0xa4ffffffffffffff")

	def test_accept_philips(self):
		self.generic_test_accept("0xabffffffffffffff")

	def test_unpair_philips(self):
		self.generic_test_unpair("0xabffffffffffffff")

	def test_setValue_philips(self):
		self.generic_test_setValue("0xabffffffffffffff")

	def test_accept_zwave(self):
		self.generic_test_accept("0xa8ffffffffffffff")

	def test_unpair_zwave(self):
		self.generic_test_unpair("0xa8ffffffffffffff")

	def test_setValue_zwave(self):
		self.generic_test_setValue("0xa8ffffffffffffff")

	def test_accept_blesmart(self):
		self.generic_test_accept("0xacffffffffffffff")

	def test_unpair_blesmart(self):
		self.generic_test_unpair("0xacffffffffffffff")

	def test_setValue_blesmart(self):
		self.generic_test_setValue("0xacffffffffffffff")

	def test_accept_vektiva(self):
		self.generic_test_accept("0xadffffffffffffff")

	def test_unpair_vektiva(self):
		self.generic_test_unpair("0xadffffffffffffff")

	def test_setValue_vektiva(self):
		self.generic_test_setValue("0xadffffffffffffff")

	def test_accept_sonoff(self):
		self.generic_test_accept("0xaeffffffffffffff")

	def test_unpair_sonoff(self):
		self.generic_test_unpair("0xaeffffffffffffff")

	def test_accept_conrad(self):
		self.generic_test_accept("0xafffffffffffffff")

	def test_unpair_conrad(self):
		self.generic_test_unpair("0xafffffffffffffff")

	def test_setValue_conrad(self):
		self.generic_test_setValue("0xafffffffffffffff")
