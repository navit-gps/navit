Plugin
======

Nearly everything in navit is a plugin. A plugin is a shared library
which needs to contain a function

.. code:: c

   void plugin_init(void)

This function is called when the plugin gets loaded. Usually it
registers the plugin by calling an appropriate function (more on that
below).

Unless you know better, start by creating a new folder under */plugin*.
Create a C source file with at least a definition for ``plugin_init()``.

Then add *CMakeLists.txt* to your plugin directory, so your plugin is
built as a module (e.g. libmyplugin.so). A minimal version looks like
this:

.. code:: bash

   module_add_library(plugin_myplugin myplugin.c)

``plugin_myplugin`` is the name for the plugin. The following parameters
are the C source files from which the module will be built.

Finally, add a module definition to */CMakeLists.txt*, to have it built
at all.

To have the plugin loaded, you need to add it to the element in
*navit.xml*.

.. _including_the_plugin_in_the_build:

Including the plugin in the build
---------------------------------

The simplest scenario is to build the plugin on all platforms. This is
done by adding the following module definition to */CMakeLists.txt*.

.. code:: bash

   add_module(plugin/myplugin "Default" TRUE)

If you want your plugin to build only in certain configurations (most
likely because your plugin targets a particular platform), the module
definition looks a bit different:

.. code:: bash

   add_module(plugin/myplugin "Default" FALSE)

Here, we are telling CMake not to build our plugin by default. We then
need to add a conditional statement to activate the plugin if we want
it—something like this:

.. code:: bash

   if(FOOBAR)
       set_with_reason(plugin/myplugin "Foobar detected" TRUE)
   endif(FOOBAR)

It is easiest if you look for another plugin that is built under the
same conditions as your new one, then look for its ``set_with_reason``
statement and place your own one right next to it. This way you don’t
have to take care about the conditional part.

Of course, it is also possible to build a plugin by default and disable
it only under certain conditions. In this case, keep ``TRUE`` in the
module definition and set it to ``FALSE`` where needed.

.. _registering_the_plugin:

Registering the plugin
----------------------

Usually ``plugin_init()`` should call an appropriate
``plugin_register_category_``\ \ ``()`` function:

.. code:: c

   void plugin_init(void) {
       dbg(lvl_debug, "Enter\n");
       plugin_register_category_<foo>("myplugin", myplugin_new);
       dbg(lvl_debug, "Exit\n");
   }

Instead of , specify the plugin category you want to register. The first
parameter to the function is the name by which the plugin will be known;
the second one is the constructor function which Navit will call when it
instantiates the plugin.

For a list of valid plugin categories, see
`plugin_def.h <https://github.com/navit-gps/navit/blob/trunk/navit/plugin_def.h>`__.

If you register a plugin in this manner, you will also need to supply a
constructor function to instantiate the plugin. The signature for this
function depends on the type of plugin you’re registering—see
*plugin_def.h* for a list.

Multiple registrations of the same plugin are possible. For example, a
single plugin could register two different maps (each with its own
constructor function) and an OSD.

This goes for plugins that provide the functionality of one of the
predefined categories (e.g. maps, GUI or graphics). Some plugins (those
in
`binding/ <https://github.com/navit-gps/navit/tree/trunk/navit/binding>`__
being notable examples) do none of the above registrations but register
callback functions instead. This allows a plugin to run actions on a
wide range of events—but make sure you understand when your intended
callback will fire.

.. _instantiating_the_plugin:

Instantiating the plugin
------------------------

When Navit decides it needs to use the plugin, it will call the
constructor function you supplied. Its arguments depend on the plugin
type. Common arguments are:

-  ``struct navit *nav`` A reference to the Navit instance, if your
   plugin needs to access it.
-  ``struct``\ \ ``_methods *meth`` A pointer to the methods for the
   plugin, which again depend on the plugin category. Your plugin needs
   to implement these methods with the expected signatures and store
   pointers to them in this argument.
-  ``struct attr **attrs`` The attributes set for this plugin.
-  ``struct callback_list *cbl``
-  ``struct attr *parent`` The parent of this plugin in the Navit object
   hierarchy. Most plugins have the Navit instance as a parent; map
   plugins have a *mapset* as a parent.

The return value of this function is a pointer to a
``struct``\ \ ``_priv``, which you can define in your plugin. It is
meant to hold your plugin’s private data that is specific to this plugin
instance.

The constructor function will not get called until Navit actually
instantiates the plugin, i.e. starts using its functionality. This
typically happens when the functionality of a plugin is requested in the
configuration, i.e. a plugin of this category and with a ``type``
matching your name is specified in *navit.xml*. For example, the
following elements all cause a plugin to be instantiated when Navit
starts up:

.. code:: xml

   <gui type="gtk" enabled="yes" menubar="1" toolbar="1" statusbar="1"/>
   <osd type="compass" enabled="yes"/>
   <map type="binfile" enabled="yes" data="/media/mmc2/MapsNavit/osm_europe.bin"/>

For some plugins, instantiation is hardcoded into Navit and takes place
regardless of configuration. This is the case e.g. for the route and
navigation maps. For most plugin scenarios, this is not something you
would need to worry about.

.. _supplying_attributes:

Supplying attributes
~~~~~~~~~~~~~~~~~~~~

In order to supply an attribute to your plugin upon instantiation, you
need to specify it in your XML configuration as shown in the example
above. This requires the attribute to be defined in
`attr_def.h <https://github.com/navit-gps/navit/blob/trunk/navit/attr_def.h>`__.
There is no limitation on the attributes which can be supplied to a
plugin—if any other plugin already uses a certain attribute, your plugin
can use it as well. Some attribute types may not be suited to being
passed in this manner, but any string or integer attribute for which the
desired value is known in advance should work.

These attributes will be passed to your constructor as a
``struct attr **`` argument. All you need to make sure is your
constructor processes the attributes it needs. For example, let us
assume your plugin needs a ``data`` argument. Then your constructor
needs to execute something like the following code:

.. code:: C

   struct attr * data;
   data = attr_search(attrs, attr_interval);
   if (data) {
       // process data->u.str for a string attribute, or data->u.num for an integer attribute
   } else {
       // fall back to a default value
   }

Note that this code will only set attributes from the XML config upon
instantiation. Adding or changing attributes after the plugin is
instantiated must be handled by the ``object_func_set_attr`` and
``object_func_add_attr`` methods.

.. _loading_the_plugin:

Loading the plugin
------------------

To have the plugin loaded, you need to add it to the Object in
navit.xml:

.. code:: xml

   <plugin path="$NAVIT_LIBDIR/*/${NAVIT_LIBPREFIX}libmyplugin.so" active="yes"/>

Depending on your platform, there may already be entries to load certain
plugins on demand.

**TODO:** describe what the ``active`` and ``ondemand`` attributes do
(which values cause the plugin to be loaded, i.e. the ``plugin_init()``
function to get called, and when does that happen?)

.. _calling_navit_core_functions_from_a_plugin:

Calling Navit core functions from a plugin
------------------------------------------

Plugins can call any Navit core function which is exported via a C
header file.

There is one caveat, which is unlikely to be an issue in a fully
developed feature but may bite you in the early stages of development:
If you have added a new source file to Navit core as part of your
development process but not used any of its functions anywhere in Navit
core, the toolchain may optimize that file out. Navit will build without
errors but calls to functions in that source file will fail at runtime.
The simple workaround is to call a single function from your new source
file anywhere in Navit core—after that the functions exported from that
source file will be compiled into the Navit binary and available for
plugins to call.

.. _defining_a_new_plugin_category:

Defining a new plugin category
------------------------------

First off, this is an advanced level and you should know what you’re
doing. If you’re sure none of the existing plugin categories fit your
requirements, read on.

Pick a name for the plugin category; you will need it in multiple
places.

In
`plugin.h <https://github.com/navit-gps/navit/blob/trunk/navit/plugin.h>`__,
add another element just before ``plugin_category_last``. The name must
start with ``plugin_category_``, followed by the name for your plugin
category. Also provide a Doxygen comment describing what plugins in this
category do.

In
`plugin_def.h <https://github.com/navit-gps/navit/blob/trunk/navit/plugin_def.h>`__,
add another call to ``PLUGIN_CATEGORY`` with the name of your new plugin
category and the signature for the constructor function.

In a core header file of your choice, define ``struct``\ \ ``_meth``,
again including the name of your plugin category in the type identifier.
Decide the methods this plugin needs to provide, along with their
signatures, and add a member for each.

Finally, you will need to decide when Navit should instantiate this
plugin and provide logic to do that.

Examples
--------

The `j1850
plugin <https://github.com/navit-gps/navit/blob/trunk/navit/plugin/j1850/j1850.c>`__
is quite simple and can be a good start if you want to write your own
plugin. It demonstrates how to use event callbacks (using a timeout
during the plugin init, or using the idle loop during the regular use),
how to add a custom OSD for your plugin, and how to send commands to
another plugin (look at the spotify controls).
