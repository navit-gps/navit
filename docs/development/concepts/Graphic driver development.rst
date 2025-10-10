.. _graphic_driver_development:

Graphic driver development
==========================

.. _writing_a_new_graphic_driver:

Writing a new graphic driver
----------------------------

The graphic driver tells NavIt how to render the map using a specific
library. The same graphic driver can be used in several GUIs. For
example, you can render the map in `OpenGL <OpenGL>`__ with the `GTK
GUI <GTK_GUI>`__ and the `SDL GUI <SDL_GUI>`__.

When the `GUI <GUI>`__ handles the user interaction, the graphic driver
is called internally by NavIt when necessary.

Here is what you need to write your own graphic driver:

To register the graphics_driver, call
**plugin_register_graphics_type**\ ("your_type", graphics_new_func).
"your_type" is what you will need to put in navit instance, in
navit.xml.

-  **graphics_new_func**\ (struct graphics_methods \*meth) should fill
   meth and return a private pointer representing the graphics for the
   driver

See for the graphics_methods

-  **graphics_destroy**: Forget the private data, graphics no longer
   needed

   -  draw_mode: Is either called with draw_mode_begin (the display list
      will be drawn), draw_mode_end (the display list is finished) or
      draw_mode_cursor (the cursor is drawn)

-  **draw_lines**: Draw a polyline, parameters are the graphics context,
   the point count and the points
-  **draw_polygon**: Like draw_lines, but draw a filled polygon instead
-  **draw_rectangle**, **draw_circle**: Obvious
-  **draw_text**: Render text. dx and dy is the direction for the text,
   fg and bg Foreground and Background graphic contexts, font a driver
   private font structure

-  **draw_image** draws an image which has been created before
-  **draw_image_warp** is similar, but can adjust the image on
   coordinates, used for bitmap maps
-  **draw_restore** restores the damaged area from the cursor
-  **font_new**: Creates a new font, used for draw_text. Parameters are
   font_methods (which is just a destroy function) and the size of the
   font
-  **gc_new**: Creates a new graphics context (Not usefull for every
   renderers, imagine it as the pen you are drawing with) Methods are:

   -  gc_destroy,
   -  gc_set_linewidth,
   -  gc_set_dashes (for dotted lines etc.),
   -  gc_set_foreground,
   -  gc_set_background (don't know if this one is used)

-  **background_gc**: Sets the background graphics context (Background
   of the map). This one will be removed soon
-  **overlay_new**: Creates a new overlay. Currently unused
-  **image_new**: Creates a new image for use with draw_image or
   draw_image_warp. Methods is just a destroy function
-  **get_data**: Returns a private pointer for the gui, representing an
   object specified in type, or NULL if the graphics driver is unable to
   provide such an object
-  **register_resize_callback**: Registers a function which should be
   called if the graphics area changes its size
-  **register_button_callback**: Registers a function which should be
   called when a mouse button is pressed or released
-  **register_motion_callback**: Registers a function which should be
   called when the mouse is moved

.. _so_now_the_stuff_that_you_probably_really_need:

So, now the stuff that you (probably) really need:
--------------------------------------------------

-  **draw_mode**: Useful for example to swap the opengl buffers when
   called with draw_mode_end
-  **draw_lines**, draw_polygon to draw the stuff
-  **draw_text**: Implement aftger draw_lines and draw_polygon works
-  **draw_image**: Maybe later, only used for POIs
-  **draw_image_warp**: A very optional feature
-  **font_new**: When draw_text is implemented
-  **gc_new**: Required, you should at least support gc_set_linewidth
   and gc_set_foreground
-  **image_new**: Needed when draw_image will be implemented
-  **get_data**: Required! At least a dummy.
-  **register_*_callback**: Optional, you can also handle it in your GUI
