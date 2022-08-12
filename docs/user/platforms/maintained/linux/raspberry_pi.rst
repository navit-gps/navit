.. _raspberry_pi:

Raspberry Pi
============

.. _compiling_for_the_raspberry_pi:

Compiling for the Raspberry Pi
------------------------------

Compiling for the Pi is (of course) mostly like `Compiling for
Linux <Linux_development>`__.

(Thanks to Angelus88, who posted it here:
`1 <https://forum.navit-project.org/viewtopic.php?f=11&t=405>`__.)

.. _want_the_easy_way:

Want the easy way?
------------------

You can try out our automated, unattended install :
https://github.com/navit-gps/raspbian-ua-netinst

https://raw.githubusercontent.com/navit-gps/raspbian-ua-netinst/master/example.gif

.. _prefer_to_install_yourself_on_raspian:

Prefer to install yourself on Raspian?
''''''''''''''''''''''''''''''''''''''

First of all, we will have to deal with the
`dependencies <dependencies>`__:

.. code:: bash

   sudo apt-get install subversion git imagemagick libdbus-1-dev libdbus-glib-1-dev libfontconfig1-dev libfreetype6-dev libfribidi-dev libimlib2-dev librsvg2-bin libspeechd-dev libxml2-dev ttf-liberation libgtk2.0-dev

This is for the compiling process:

.. code:: bash

   sudo apt-get install gcc g++ cmake make zlib1g-dev libpng12-dev librsvg2-bin

This is for the SDL graphics (suggested):

.. code:: bash

   sudo apt-get install libsdl-image1.2-dev libdevil-dev libglc-dev freeglut3-dev libxmu-dev libfribidi-dev

This is for the OpenGL support:

.. code:: bash

   sudo apt-get install libglc-dev freeglut3-dev libgl1-mesa-dev libfreeimage-dev

This is QT (note that the QT gui is not maintained currently) :

.. code:: bash

   sudo apt-get install libqt4-dev

This is for GPSd support :

.. code:: bash

   sudo apt-get install libgps-dev

This is espeak, TTS (text to speech)(optional):

.. code:: bash

   sudo apt-get install espeak

If you want to use Garmin maps ( OpenStreetMap is natively supported and
are really detailed ) :

.. code:: bash

   sudo apt-get install libgarmin-dev

Ok, now let's download the latest version of Navit from the repository
(starting from your user's folder like /home/pi):

.. code:: bash

   git clone https://github.com/navit-gps/navit.git

CMake builds Navit in a separate directory of your choice - this means
that the directory in which the SVN source was checked out remains
untouched.

.. code:: bash

   mkdir navit-build
   cd navit-build

Now the compiling (if you need CSV, keep reading!):

Note : the freetype library has been updated on raspbian, and this broke
cmake's ability to find it. Until `this
bug <https://bugs.launchpad.net/raspbian/+bug/1417732>`__ is fixed ( and
cmake is updated to a version > 2.9 ) you will need to add an extra flag
to the cmake call :

.. code:: bash

   cmake ~/navit -DFREETYPE_INCLUDE_DIRS=/usr/include/freetype2/

Once the above bug has been fixed, you should be able to use only :

.. code:: bash

   cmake ~/navit

If you need the CSV support for POIs, you must use:

.. code:: bash

   cmake --enable-map-csv ~/navit

You are now ready to compile. For a raspberry A, B or B+:

.. code:: bash

   make

For a raspberry 2:

.. code:: bash

   make -j4

This can take A LOT of time:

| ``- on a raspberry 2, it takes ~4:30 minutes``
| ``- on a raspberry b+, it takes ~36 minutes``

At the end, you can start Navit (don't forget the configuration!
Navit.xml):

.. code:: bash

   cd ~/navit-build/navit/
   ./navit

Generating a 3290kms route on the Raspberry pi 2 takes ~ 55s.

Adding Support for UART Serial GPS

connect VCC to pin 1, RX to pin 8 TX to pin 10 and Ground to pin 6 on
GPIO for Pi2

Do sudo apt-get install gpsd gpsd-clients python-gps Then sudo
``gpsd /dev/ttyAMA0 -F /var/run/gpsd.sock`` test with ``cpgs -s`` to
autostart gpsd type ``sudo dpkg-rconfigure gpsd`` select yes add
/dev/ttyAMA0 defaults for everything else.
