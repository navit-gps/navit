Translations
============

.. _submitting_translations:

Submitting translations
=======================

Navit uses `gettext <http://en.wikipedia.org/wiki/Gettext>`__ to display
translated text. Gettext uses so-called ".po files" to hold the
translated texts. The .po files for Navit are located in the source tree
under navit/po. In Navit, they have the file ending .po.in (the .po
files are generated from these files during the build).

.. _web_based_translation_tool:

Web-based translation tool
--------------------------

The easiest and prefered way to are the web-based translation tools of
Launchpad:

-  https://translations.launchpad.net/navit/trunk

This allows everybody who signs up at the service to
browse/contribute/edit the translations of every language. You are
guided by launchpad during the process and help is offered in the whole
wizard. In case of any questions, please get in contact with the
`team <team>`__.

.. _po.in_files:

.po.in files
------------

Please submit manual translations as patches against the .po.in files.
Before correcting the translations, please update the .po.in file as
described below.

Before working on the translations, it's important for the **po.in files
to be current** - the original texts in the source code change
regularly, and if the .po.in files are not current, you may translate
text that is no longer part of Navit.

During Navit's build, .po files are generated automatically from the
.po.in files under navit/po, taking into account any changed texts in
the source code. To update a .po.in file, just do a complete build (make
all), then copy the [language].po file that was created over the
[language].po.in file. This new .po.in file will contain the strings as
they appear in the source code, along with the old translations from the
previous .po.in file. Old translations for texts that no longer occur in
the source code will appear commented out (with a leading "#"). New
strings from the source code will have emtpy translations.

You can now correct/add translations, possibly using the old
translations.

.. _developers_with_commit_access:

Developers with commit access
-----------------------------

Even if you have commit access, please ensure your translations go
through Launchpad! Otherwise the next update from Launchpad will wipe
out your work. Also, going through Launchpad ensures your translations
are visible to contributors there so they won't unnecessarily translate
strings which you have already taken care of.

Launchpad lets you work online through the web interface, or offline by
exporting .po files, editing them locally and uploading them back to
Launchpad.

To upload a file to Launchpad:

-  **Make sure your work is based on a translation file exported from
   Launchpad.** This is a requirement in order to prevent translators
   who work offline from inadvertently reverting translations made by
   others. Launchpad will check all uploaded files and reject those that
   don't have an export timestamp set by Launchpad.
-  Open the translation page for the language you want to update. (If
   you can't find it, open the page for a random language and replace
   the language identifier in the URL.)
-  Click **Upload translation**.
-  Click **Browse** and select your translation file. **The file must
   have a .po extension, else it will be rejected – rename it if
   necessary.**
-  Click **Upload**.
-  You're almost done! Your translations now need to be reviewed in
   order to finish the import.

The workflow to add translations is as follows:

-  If you want to work online, make your edits first.
-  Pull the latest version of the source code from the repository.
-  Export translations from Launchpad (using the
   `Download <https://translations.launchpad.net/navit/trunk/+export>`__
   link on the Translations Overview page) and copy them into your code
   tree.
-  Build Navit (for whichever platform you like) – this will regenerate
   the .po files in your build directory.
-  Rename the .po files that were created in your build directory to
   .po.in and copy them over the existing ones in the Navit code tree.
-  If you want to work offline, make your changes now, then upload the
   changed files to Launchpad.
-  Now commit your changes to SVN trunk. (A script to update
   translations from Launchpad is currently being worked on.)

Dictionary
==========

To have a consistent user experience, we have a dictionary of terms used
in different places in our application. Please refer to this dictionary
for translations.

============ ================
English      German
============ ================
Bookmark     Lesezeichen
Waypoint     Wegpunkt
street, road | Straße
             | Straßenverlauf
exit         Ausfahrt
ramp         Auffahrt
roundabound  Kreisverkehr
position     Standort
roadbook     Wegbeschreibung
destination  Ziel
navigation   Navigation
position     Position
command      Befehl
Point        ?Punkt
Map point    ?Kartenpunkt
screen coord Bildschirm
settings     Einstellungen
vehicle      Fahrzeug
\            
============ ================

.. _not_to_translate:

Not to translate
----------------

-  (OSM) "tag"
-  "POI"
