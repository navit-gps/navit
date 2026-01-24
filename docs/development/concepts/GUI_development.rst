.. _gui_development:

GUI development
===============

Foreword
========

Almost everything in NavIt works as a `plugin <plugin>`__, and so do the
guis.

.. _initializing_your_gui:

Initializing your gui
=====================

You have to provide a function plugin_init in your plugin.

This function should call

.. code:: c

   plugin_register_gui_type("gui-name", function);

The function has the following parameters:
``struct gui_methods *meth, int w, int h``

-  ``meth`` is a structure with gui-functions you have to fill
-  ``w`` is the expected width
-  ``h`` is the expected height of the gui.

But you might ignore w and h, they are only recommendations.

``meth`` should be filled with the following data:

-  ``menubar_new``
-  ``toolbar_new``
-  ``statusbar_new``
-  ``popup_new``
-  ``set_graphics``

You can probably implement ``menubar_new``, ``toolbar_new``,
``statusbar_new`` and ``popup_new`` as a dummy, since it is rather
GTK-specific, but ``set_graphics`` is required. It connects the graphics
to the gui.

.. code:: c

    struct gui_methods gui_<plugin>_methods = {
       gui_<plugin>_menubar_new,
       gui_<plugin>_toolbar_new,
       gui_<plugin>_statusbar_new,
       gui_<plugin>_popup_new,
       gui_<plugin>_set_graphics,
       gui_<plugin>_main_loop,
    };

Here is a code sample :

.. code:: c

    void
    plugin_init(void)
    {
       dbg(1,"registering <your plugin>\n");
       plugin_register_gui_type("<your plugin>", <your init function>);
    }

Now, here is a sample of the init function :

.. code:: c

    static struct gui_priv *
    gui_<plugin>_new(struct navit *nav, struct gui_methods *meth, struct attr **attrs) 
    {
       dbg(1,"Begin initialization of <your plugin>\n");
       struct gui_priv *this_;
           *meth=gui_<plugin>_methods;
    
       this_=g_new0(struct gui_priv, 1);
    
       // Perform initializations specific to your gui
       // ...
    
       // If you want to register a callback function for the vehicle, you can do it so:
       struct callback *cb=callback_new_0(callback_cast(vehicle_callback_handler));
    
       navit_add_vehicle_cb(nav,cb);
       this_->nav=nav;
       
       return this_;
   }

And you plugin (gui) should initialize. Now, you need somme dummy
functions (for now). Thoses are requested for GTK.

.. code:: c

    static struct menu_priv *
    gui_<plugin>_toolbar_new(struct gui_priv *this_, struct menu_methods *meth)
    {
       return NULL;
    }
    
    static struct statusbar_priv *
    gui_<plugin>_statusbar_new(struct gui_priv *gui, struct statusbar_methods *meth)
    {
       return NULL;
    }
    
    static struct menu_priv *
    gui_<plugin>_popup_new(struct gui_priv *this_, struct menu_methods *meth)
    {
       return NULL;
    }

The following function allows to get menu entries for bookmarks

.. code:: c

    static struct menu_priv *
    gui_<plugin>_menubar_new(struct gui_priv *this_, struct menu_methods *meth)
    {
       *meth=menu_methods;
       return (struct menu_priv *) 1;
    }

.. _the_plugin_main:

The plugin main()
=================

And, last but not least :

.. code:: c

    static int gui_run_main_loop(struct gui_priv *this_)
    {
           // Whatever needs to be done right before beginning the navigation
           
           // Define your viewport
       struct map_selection sel;
            
       memset(&sel, 0, sizeof(sel));
       sel.u.c_rect.rl.x=800;
       sel.u.c_rect.rl.y=600;
       
       transform_set_screen_selection(navit_get_trans(this_->nav), &sel);
           
           
       navit_draw(this_->nav);
           
           // Register the callback to handle navigation instructions updates
       struct navigation *navig;
       navig=navit_get_navigation(navit);
           
       navigation_register_callback(navig,
           attr_navigation_long,
           callback_new_0((void (*)())update_roadbook)
       );
           
       timeout = g_timeout_source_new(100);
       g_source_set_callback(timeout, gui_timeout_cb, NULL, NULL);
       g_source_attach(timeout, NULL);
       while (!must_quit)
       {
           //Poll event, process them
           
           // Call update of gps
           g_main_context_iteration (NULL, TRUE);
           
           // Any other task you may need
    
           }
           g_source_destroy(timeout);
            
    }
