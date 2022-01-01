.. _virtual_appliance:

Virtual appliance
=================

Hi,
---

This page is about navit as a virtual appliance. This means that you run
a virtual machine (guest), which runs navit itself.

This ofcourse will add some overhead.

On the otherhand if you have a machine that you cannot install software
on as administrator, this virtual appliance (VA) might be a good method
of running navit, without the need for administrator rights. Or if you,
like me, update your machine quiet often, the VA can be updated only
when necessary, making it a stable appliance. You can put it on an USB
stick (>= 2Gb) and run it from there.

I use `qemu <http://fabrice.bellard.free.fr/qemu>`__ for the virtual
machine. This program runs in userspace, so no "installation" is
necessary.

For the host OS that runs navit, i have choossen
`gentoo <http://www.gentoo.org>`__ because it optimize code for the
emulated CPU.

Below are the general steps to install
`qemu <http://www.qemu.org/index.html>`__, details can be added over
time.

--------------

Step 1 Download `qemu <http://www.qemu.org/index.html>`__ and install
it, using these `instructions <http://www.qemu.org/user-doc.html>`__.

Step 2 Download `the virtual
appliance <http://maps.navit-project.org/virtual/navit_must.zip.tar>`__.

| ``       (download file seem to have moved, so just use this page for inspiration)``
| ``       This is a *large* file, so downloading might take a while.``
| ``       After downloading the navit_must.zip file, you can unzip it.``
| ``       This will give you two files, nav_root.img, which is the virtual machine itself,``
| ``       and maps.iso, a iso image that contains some maps.``

Step 3 Start qemu. Just type qemu -hda nav_root.img -hdc maps.iso
-soundhw es1370

``       This should start gentoo, and after some diagnostics, you will see the navit application itself.``

--------------

GPSD
----

For navit to show you your position on the map, you need a gps receiver.
The VA assumes that you run `gpsd <http://gpsd.berlios.de>`__. on the
host machine. I run the VA on a linux gentoo host, for a gpsd for
windows, you can try
`wingpsd <http://www5.musatcha.com/musatcha/computers/software/gpsd>`__
i don't know to much about windows, so anyone with more knowledge about
gpsd for windows please edit this
section!\ `edit <http://wiki.navit-project.org/index.php?title=Navit_va&action=edit&section=2>`__

I use a bluetooth gps receiver and a usb bluetooth dongle from sweex.
currently qemu 0.90 does not support this usb bluetooth dongle, but
perhaps the new 0.91 qemu will support it. You can then use navit
without the need for a running gpsd on the host. If anyone has success
with that, please add it to this section.
`edit <http://wiki.navit-project.org/index.php?title=Navit_va&action=edit&section=3>`__

--------------

Comments
--------

Please leave your commnents
`here <http://wiki.navit-project.org/index.php?title=Navit_va&action=edit&section=4>`__,
i will monitor this page from time to time and try to help.

For questions about qemu, please visit the qemu site.

For questions about navit, please visit the navit site.

For questions about gentoo, please visit the gentoo site.

For questions about \*this\* virtual machine, please leave a me a
message

Messages
--------

It seems that the file is no longer available for download.
--`Fab <User:Fab>`__ 09:38, 3 April 2010 (UTC)
