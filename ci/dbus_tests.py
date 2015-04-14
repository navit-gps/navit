import glob
import dbus
from dbus import glib
import gobject
import sys
import os
import time
from subprocess import call
from junit_xml import TestSuite, TestCase


gobject.threads_init()

glib.init_threads()

bus = dbus.SessionBus()

navit_object = bus.get_object("org.navit_project.navit", # Connection name
                               "/org/navit_project/navit/default_navit" ) # Object's path

iface = dbus.Interface(navit_object, dbus_interface="org.navit_project.navit.navit")
junit_directory=sys.argv[1]
if not os.path.exists(junit_directory):
    os.makedirs(junit_directory)

tests=[]
start_time = time.time()
test_cases = TestCase("zoom (factor) expected 512", '', time.time() - start_time, '', '')
iface.zoom(-2)
zoom=iface.get_attr("zoom")[1]
if zoom !=512 :
   test_cases.add_failure_info('zoom level mismatch. Got '+str(zoom)+', expected 512')
tests.append(test_cases)

test_cases = TestCase("zoom (factor) expected 1024", '', time.time() - start_time, '', '')
iface.zoom(-2)
zoom=iface.get_attr("zoom")[1]
if zoom !=1024 :
   test_cases.add_failure_info('zoom level mismatch. Got '+str(zoom)+', expected 1024')
tests.append(test_cases)

test_cases = TestCase("zoom via set_attr expected 512", '', time.time() - start_time, '', '')
iface.set_attr("zoom", 512)
zoom=iface.get_attr("zoom")[1]
if zoom !=512 :
   test_cases.add_failure_info('zoom level mismatch. Got '+str(zoom)+', expected 512')
tests.append(test_cases)

ts = [TestSuite("Navit dbus tests", tests)]

with open(junit_directory+'dbus.xml', 'w+') as f:
    TestSuite.to_file(f, ts, prettyprint=False)
