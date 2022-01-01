.. _marco_polo_grosser_reiseplaner:

Marco Polo Grosser Reiseplaner
==============================

.. _marco_polo_grosser_reiseplaner_1:

Marco Polo Grosser Reiseplaner
------------------------------

Marco Polo Grosser Reiseplanner are extremely detailed digital maps of
Europe, split into 5 regions (with Germany having its very own region!).
These maps were the first map types which Navit supported.

Note that Navit can only read Grosser Reiseplanner versions 2002/2003 to
2007/2008. Newer versions are currently incomptible with Navit.

Description
~~~~~~~~~~~

This is a detailed map of Europe. Works with version from 2003/2004 to
2007/2008 versions.

You can buy the latest version from here:
http://www.amazon.de/o/ASIN/3829731434/navit-21 (remember the little
reward please ;))

Warning: Version 2008/2009 has a different map format and does not yet
work with navit

Installation
~~~~~~~~~~~~

**This manual was written with a german version of Grosser Reiseplanner
aside. So it may differ a bit.**

#. Change directory to where DVD is mounted
#. Unpack data2.cab with

      ``unshield x travel/data2.cab``

#. Now, you should get a directory named like DIRLAN_GER. It contains:
   :\* The dem.map folder with all the majors roads and towns of
   Europe...

   :\* ...and five smpX.smp folders, which contain the details of
   countries:

   :*\* smp1: DK,S,N,IS,FIN,N

   :*\* smp2: F,E,GBZ,P,AND,MC

   :*\* smp3: Germany

   :*\* smp4:
   CZ,SK,RSM,EST,GEO,LV,LT,MD,RUS,UA,BY,GR,H,I,RO,CH,A,PL,AL,MT,CY,BG,FL,SRB,MNE,HR,SLO,BIH,MK,TR,AZ,AM,V

   :*\* smp5: NL,B,GB,L,IRL

      Update your `Configuation <Configuation>`__ accordingly. A sample
      for Germany would be:

| 
| ``        ``\ 
| ``        ``\ 
| 

Optional: You can save some space (up to 2G) by deleting all the \*d60\*
files. They are not needed.

.. _note_for_low_memory_devices:

Note for low memory devices
~~~~~~~~~~~~~~~~~~~~~~~~~~~

For relatively low memory devices (for example 128MB of RAM) under
GNU/Linux, please type as root:

``echo 1 > /proc/sys/vm/overcommit_memory``
