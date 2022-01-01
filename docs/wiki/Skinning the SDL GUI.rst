.. _skinning_the_sdl_gui:

Skinning the SDL GUI
====================

.. raw:: mediawiki

   {{warning
   |'''DEPRECATED''', use [[Internal_GUI]] instead.
   }}

.. _the_quick_howto:

The quick howto
===============

The SDL Gui uses the CEGUI system.

It can be really easily skinned. Share with us what you do :)

If you want to customize NavIt, you should read the followings : `Cegui
Tutorials <http://www.cegui.org.uk/wiki/index.php/Tutorials>`__

And especially this page : `Cegui, creating
skins <http://www.cegui.org.uk/wiki/index.php/Creating_Skins>`__

You will find the needed files in navit's folder, under
navit/gui/sdl/datafiles.

-  navit.layout defines the buttons and their placement. You can change
   the controls coordinates and size.
-  TaharezLook.scheme defines the scheme used. Like fonts, etc.
-  TaharezLook.imageset defines where to find the controls graphics in
   the TaharezLook.tga file. Have a look at the .tga file, it should be
   quite obvious.

.. _a_more_detailed_approach:

A more detailed approach
========================

A skin consist of 3 parts :

-  The layout defines the buttons and their placement. You can change
   the controls coordinates and size.
-  The imageset defines where to find the graphics items in the .tga
   file.
-  The looknfeel ties them up, defining how a given control should react
   to a user input (is it a button? is it a window?)

There is two work in progress currently about guis.

-  Metallic Cherry by Eubey
-  Blackie by Mineque. (current default skin)

If you want to design your own gui, we'll be more than happy to help you
do so.

.. _about_the_layout.:

About the Layout.
=================

The layout defines where the controls should be displayed. (e.g. : the
Quit button).

The default layout file is navit.layout, which you can find in
navit/gui/sdl/datafiles/layout.

Soon you will be able to specify which layout file to use in the
navit.xml configuration file.

You can change many things in the layout file :

-  position and size of the controls,
-  visibility (you can hide a button or widget that you don't want to be
   displayed)

**Avoid changing the widget names**, because the gui expects some to be
present (and it won't work without them). It's best to hide a control
rather than to remove it from the layout.

A list of the needed widget and their use will be put here soon.

.. _structure_of_.layout_files:

Structure of \*.layout files
============================

This part is WIP

Typical description of object in .layout file

| ``       ``\ 
| ``           ``\ 
| ``           ``\ 
| ``   ``\ 

-  UnifiedAreaRect - sth here must be added
-  "UnifiedAreaRect" Value - tell's Navit where parts of skin must be
   placed in the app. Starting from left 635 is X and 1 Y position of
   upper left corner. Then 800 and 600 are X,Y position of bottom right
   corner of skin element

.. _about_the_imageset:

About the Imageset
==================

This is probably the easiest part.

The default imageset file is Taharez, which you can find in
navit/gui/sdl/datafiles/imageset.

The imageset is composed of two files, a .tga and a .imageset (which is
a xml file).

Each widget is based upon on or several graphical element.

The imageset is simply a big .tga file which contains every items all
together. The .imageset file is a xml file which defines where to find a
specifical graphic item in the .tga file.

So, to add your own graphical item (like a button, or a mouse cursor)
just add it to an empty place in the file, and update the xml file.

You can name it almost the way you want but it's best to match the names
used elsewhere. We'll put a comprehensive list here, which will also
ease the writing of the looknfeel.

**Warning** : the renderer cuts controls in rectangular areas. This mean
that you can draw a rounded component, but it has to be placed in the
.tga file somewhere where the ractangular area in which it is contained
does not overlap another control.

.. _structure_of_.imageset_files:

Structure of \*.imageset files
==============================

| ``   * Imageset Name - name of your skin``
| ``   * Imagefile - file with all textures inside``
| ``   * NativeHorzRes, NativeVertRes - image file size in pixels``

.. raw:: html

   <?xml version="1.0" ?>

| ``   * Image Name - name of texture we will further use in layout and looknfeel files``
| ``   * XPos, YPos - position (X,Y) of texture parts in image file it is counted from upper left corner``
| ``   * Width, Height - skin part dimension``

| ``   ``\ 
| ``   ``\ 
| ``   ``\ 

| ``   ``\ 
| 

It's nice to make code more readable by spliting it like below

``    ``\ 

...

``   ``\ 

...

``   ``\ 

...

``   ``\ 

...

``   ``\ 

...

And that is example of finished .imageset file

.. raw:: html

   <?xml version="1.0" ?>

| 
| `` ``
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| ``   ``\ 
| 

.. _about_the_looknfeel:

About the looknfeel
===================

Got the name of the control? You put the graphic items in the imageset?
Great. Now the only part left is to check the looknfeel.

The LooknFeel defines for each item what kind of widget it is.
