Speech
======

.. raw:: mediawiki

   {{merge|Configuration}}

eSpeak
======

First, install espeak either via apt-get or manually. The advantage of
the manual installation is that from 1.45-28 onwards the -a parameter
for volume works as well. This is useful, because mbrola voices are
generally a bit quiter than the default espeak voice. Espeak >= 1.44
calls mbrola directly, so make sure your repository version is equal or
above 1.44.

repository
----------

-  apt-get install espeak

.. _manually_done_with_ubuntu_natty:

manually (done with Ubuntu Natty)
---------------------------------

| ``apt-get remove espeak``
| ``apt-get remove espeak-data``
| ``apt-get remove libportaudio0``
| ``apt-get install portaudio19-dev``
| ``apt-get install libportaudio2``
| ``apt-get install debhelper``
| ``# download the latest version from http://espeak.sourceforge.net/test/latest.html``
| ``unzip espeak-*.zip``
| ``cd /usr/lib64``
| ``ln -s libaudio.so.2 libaudio.so``
| ``cd -``
| ``cd espeak-*/src/``
| ``cp portaudio19.h portaudio.h                    # ReadMe is obsolete for Ubuntu Natty``
| ``make``
| ``make install``
| ``cd -``

Mbrola
======

.. _install_mbrola_from_the_repositories:

install mbrola from the repositories
------------------------------------

``apt-get install mbrola``

.. _get_a_mbrola_voice:

get a mbrola voice
------------------

```http://www.tcts.fpms.ac.be/synthesis/mbrola/mbrcopybin.html`` <http://www.tcts.fpms.ac.be/synthesis/mbrola/mbrcopybin.html>`__

```http://tcts.fpms.ac.be/synthesis/mbrola.html`` <http://tcts.fpms.ac.be/synthesis/mbrola.html>`__

.. _create_the_needed_folders:

create the needed folders
-------------------------

| ``mkdir /usr/share/mbrola/``
| ``mkdir /usr/share/mbrola/voices/``

.. _unzip_the_voice_package_and_put_the_binary_file_e.g._en1_to_usrsharembrolavoices:

unzip the voice package and put the binary! file (e.g. en1) to /usr/share/mbrola/voices/
----------------------------------------------------------------------------------------

.. _mbrola_on_raspbian:

Mbrola on Raspbian
==================

-  tested under Raspbian Jessie on a Raspberry Pi 3

.. _get_mbrola_working_on_raspbian_jessie:

get mbrola working on Raspbian Jessie
-------------------------------------

-  found in the Raspberry Pi Forum

(https://www.raspberrypi.org/forums/viewtopic.php?f=66&t=148096)

| ``wget ``\ ```http://steinerdatenbank.de/software/mbrola3.0.1h_armhf.deb`` <http://steinerdatenbank.de/software/mbrola3.0.1h_armhf.deb>`__
| ``sudo dpkg -i mbrola3.0.1h_armhf.deb``

.. _unzip_the_voice_package_and_put_the_binary_file_e.g._en1_to_usrsharembrolaen1:

unzip the voice package and put the binary! file (e.g. en1) to /usr/share/mbrola/en1/
-------------------------------------------------------------------------------------

-  Now you should give the Mbrola Voice a try

``espeak -v mb/mb-en1 "This is mbrola voice english 1"``

.. _speech_output:

Speech output
=============

-  If you want to use several arguments, e.g. to play an acoustic tone
   prior to every speech output or if you use an eSpeak version lower
   than 1.44 and as a consequence a double pipe, you need to put the
   speechline in a script and add the path to the speech line of
   navit.xml .

-  Speech.sh could have the following entry: **(don't forget to chmod
   u+x the script!)**

| ``#!/bin/bash/``
| ``aplay -r 44100 /home/CHANGEME/.navit/sound.wav ``
| ``espeak -vmb-en1 -s 150 -a 150 -p 50 "$1"``

-  Note that espeak will take 1-2 seconds to start if you put the
   command in a new line. For an instant play use:

| ``#!/bin/bash/``
| ``aplay -r 44100 /home/CHANGEME/.navit/sound.wav & espeak -vmb-en1 -s 150 -a 150 -p 50 "$1"``

-  For an eSpeak version **below** 1.44 (repository version of lucid)
   the pipe would look like:

| ``#!/bin/bash/``
| ``espeak -vmb-en1 -s 150 -p 45 "$1" | mbrola -e /usr/share/mbrola/voices/en1 - - | aplay -r16550 -fs16 >> /dev/null``

-  -s sets the speed
-  -a sets the volume(av. 100)
-  -p sets the pitch(av. 50)

Sounds
------

http://www.freesound.org/tagsViewSingle.php?id=1885

.. _more_information:

More information
================

**For more information regarding eSpeak go to:**
http://espeak.sourceforge.net/

.. _configuring_speech_in_your_language:

Configuring Speech in your language
===================================

.. raw:: mediawiki

   {{merge|Speech}}

To test results and for normal operation you should configure a speech
program in navit.xml to listen the sentences, for example in spanish you
could substitute the default speech definition:

.. code:: xml

    <speech type="cmdline" data="echo 'Fix the speech tag in navit.xml to let navit say:' '%s'" />

with an appropiate one, that uses festival:

.. code:: xml

    <speech type="cmdline" data="/usr/local/bin/speech-wrapper %s spanish">

You will also need following /usr/local/bin/speech-wrapper file wich
must have an executable bit set:

.. code:: bash

    #!/bin/sh
    echo "$1" | festival --tts --language "$2"

Here's how to use espeak:

.. code:: xml

    <speech type="cmdline" data="espeak -s 150 -v german %s" />

where -s is the "Speed in words per minute" (150 seems to be quite good
for german) and -v specifies the language to use. Please refer to espeak
to see which languages you can use on your system and which other
command-line-options are useful.

The call to external speech command is asynchronus, but it will block if
previous phrase is still being speaked. The GUI will freeze while espeak
is speaking to you only if it's going to say a new phrase before it end
with previous. If you do not want this behaviour, you can try using
wrapper script like in festival smaple but with & sign at the end of
last line:

.. code:: xml

    <speech type="cmdline" data="/usr/local/bin/speech-wrapper %s german" />

Script /usr/local/bin/speech-wrapper would be like this (don't forget to
set executable bit):

.. code:: bash

    #!/bin/sh
    espeak -s 150 -v "$2" "$1" &amp;

If you are running multiple programs with audio output, it is possible
that /dev/dsp is locked (the default DSP for both festival and espeak).
Use the following command to re-route the sound to ALSA. (To be exact,
the Wave channel on the default sound card).

.. code:: xml

    <speech type="cmdline" data="/usr/local/bin/speech-wrapper %s english">

And following /usr/local/bin/speech-wrapper for espeak:

.. code:: bash

    #!/bin/sh
    espeak -s 150 -v "$2" --stdout "$1" | aplay > /dev/null

.. _note_on_the_dutch_language:

Note on the Dutch language
--------------------------

I had good success with the dutch extensions for festival.
`nextens <http://nextens.uvt.nl/nextens-wiki/DownloadNextens>`__

Especially the spoken dutch streetnames are much more clear this way.

It takes a bit of work to compile it, but i think it is worth the
effort.

.. _letting_navit_speak_through_kdes_kttsd_in_german:

Letting Navit speak through KDEs kttsd in German
------------------------------------------------

When you get textsynthesis in KDE running with KDEs **kttsd**, you can
also let Navit speak through **kttsd**. Use this line for it (using
german as language):

.. code:: xml

    <speech type="cmdline" data="dcop kttsd KSpeech sayText %s de" cps="15"/>

When the **kttsd** is running, Navit will now speak with your configured
language/voice from **kttsd** (run *kttsmgr* to configure it). Because
**kttsd** can be made to speak german, you can let Navit now speak
german, which is quite an improvement when it comes to pronouncing
german words.

There is a good howto about setting **kttsd** up to speak german:
`deutsche-sprachausgabe-in-kde <http://kanotix.wordpress.com/2006/08/14/deutsche-sprachausgabe-in-kde/>`__

Running Debian Lenny, the mbrola-packages can be installed with apt-get
(or synaptic, or aptitude, ...). I have just installed all the stuff
with "*apt-get install kttsd kttsd-contrib-plugins kmouth ksayit mbrola
mbrola-de6 mbrola-de7*" and installed and configured the "*txt2pho*"
like described in the howto and got a full functioning, german speech
synthesis in KDE. Which Navit (with the **) uses without problems.

.. _use_google_translate_online_to_speak_directions:

Use Google translate (online) to speak directions
-------------------------------------------------

You can let navit speak its directions using the TTS api from Google
translate. It wil send the directions to the online service
(internetacces is needed) and it generates an MP3 with the spoken
directions that you can play using different audioplayers. This script
is generated and working in win32, other oss need to be tested and
confirmed. First download the VLC portable player from
http://portableapps.com/apps/music_video/vlc_portable , extract and put
vlc.exe somewhere in you're navit folder. You only need the exe itself.

Open a texteditor and copy the next script in it and save as .bat
somewhere in the navit folder.

| ``set ESCAPED=%~1%``
| ``wget -q -U Mozilla -O output.mp3 "``\ ```http://translate.google.com/translate_tts?ie=UTF-8&tl=en&q=%ESCAPED%`` <http://translate.google.com/translate_tts?ie=UTF-8&tl=en&q=%ESCAPED%>`__\ ``"``
| ``FULL-PATH-TO/vlc.exe --qt-start-minimized --play-and-exit -q output.mp3``

change the path-to=vlc.exe to the FULL path to the exe.

You can define the language that is spoken by setting the &tl=en to the
correct country tag, so for dutch it would be &tl=nl , german would be
&tl=de etc.

Download WGET and put it in the same directory as you're .bat file.
Download WGET from http://users.ugent.be/~bpuype/wget/ .

Open navit.xml and set the speech command to:

Thats it! This is still a work in progress but works in its simplest
form.
