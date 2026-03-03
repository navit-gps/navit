.. _failed_to_connect_graphics_to_gui:

Failed to connect graphics to gui
=================================

Ah. this (in)famous Failed to connect graphics to gui.

It maybe deserves its own page :)

This error occurs under one of the following conditions :

-  You choosed a gui which doesn't exist or doesn't work
-  You choosed a graphics which doesn't exist or doesn't work
-  The graphics driver is not compatible with the gui

BUT the MOST common cause for this error is that you are missing a
dependency for both guis, and none of them got built.

Have you checked the dependencies list? Here is a reminder :

For `GTK Gui <GTK_Gui>`__: you need gtk2

For `SDL Gui <SDL_Gui>`__: you need `cegui <cegui>`__,
`quesoglc <quesoglc>`__
