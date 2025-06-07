.. _core_module:

Core Module
===========

Navit is made up of several core modules for navigation, routing, maps,
graphics, GUI, vehicle and more. Many core components are stubs which
are extended with `plugins <Plugin>`__: the core module provides the
functionality that is shared between plugins, and the plugin provides
the actual implementation. This allows us to support graphics on
different platforms, different map formats, different vehicles
(essentially GPS sources) and so on. Some modules are singletons (e.g.
you can have only one GUI active at a time) while others allow multiple
instances (e.g. you can have any number of maps).

It is rarely the case that you would actually need to develop a new core
module for Navit from scratch. However, understanding the steps involved
in creating one will help you understand how they work, should you ever
need to add functionality a core module, or debug one.

Most of this information was gathered during development of the traffic
module (which, as of December 2017, is still under development), with a
lot of input taken from the proposed audio framework.

.. _source_files:

Source files
------------

Most likely you will create a file named *navit/mymodule.c* to hold the
code for your module, replacing *mymodule* with the name of your module.
While not strictly a requirement, this is a convention followed in Navit
which makes the code easier to read. Wherever you encounter ``mymodule``
as a name, or part of a name, in the following example, replace it with
the name you have chosen for your module.

Since you will likely be exporting function prototypes, structures and
more from your module, you will also need a header file
*navit/mymodule.h* to go with it. *mymodule.c* should contain a line
like

.. code:: c

   #include "mymodule.h"

Next you need to tell the build chain about the new file. Edit
*navit/CMakeLists.txt* and add *mymodule.c* to the list of source files
near the top of the file.

.. _add_an_attribute_type:

Add an attribute type
---------------------

Navit manages its core components in an object tree. Each core module is
a Navit object class and thus has its own attribute type. When an object
is instantiated, it is added to its parent as an attribute.

To add the attribute type for your module, edit *navit/attr_def.h*.

The file consists almost entirely of macros. You will see that it is
sorted by attribute types. Look for the following line:

.. code:: c

   ATTR2(0x0008ffff,type_object_end)

**Above** that line, insert the following:

.. code:: c

   ATTR(mymodule)

.. _add_basic_definitions_for_your_module:

Add basic definitions for your module
-------------------------------------

First you need to define a ``struct`` in which you will store all
information related to your module instance.

Next you need to provide methods for your module. Some methods are
optional (they can be ``NULL`` unless you need their functionality); for
others Navit provides default implementations that you can use if you
don’t need to do anything special. The only function you absolutely need
to write for yourself is the constructor function (though you can look
at other modules’ constructors to see what needs to go in there, and
copy over the relevant bits). Navit will call this function to create an
instance of your module.

Finally you need to tell Navit which methods to use for your module.
Here is the code you need to add to *mymodule.c*:

.. code:: c

   #include "xmlconfig.h"

   struct mymodule {
       NAVIT_OBJECT
       /* add members here as needed */
   };

   struct mymodule * mymodule_new(struct attr *parent, struct attr **attrs) {
       struct mymodule *this_;
       struct attr *attr;

       /* an example of getting an attribute from the XML file */
       attr = attr_search(attrs, NULL, attr_type);
       if (!attr) {
           dbg(lvl_error, "type missing\n");
           return NULL;
       }
       dbg(lvl_debug, "type='%s'\n", attr->u.str);

       /* call through to Navit’s constructor (mandatory) */
       this_ = (struct mymodule *) navit_object_new(attrs, &mymodule_func, sizeof(struct mymodule));

       /* unwind if there is a failure during initialization */
       if (something_went_wrong) {
           navit_object_destroy((struct navit_object *) this_);
           return NULL;
       }
       dbg(lvl_debug,"return %p\n", this_);

       /* reference the new object, else Navit will destroy it right after this function returns */
       navit_object_ref((struct navit_object *) this_);

       return this_;
   }

   struct object_func mymodule_func = {
       attr_mymodule,
       (object_func_new)mymodule_new,
       (object_func_get_attr)navit_object_get_attr,
       (object_func_iter_new)navit_object_attr_iter_new,         /* or NULL */
       (object_func_iter_destroy)navit_object_attr_iter_destroy, /* or NULL */
       (object_func_set_attr)navit_object_set_attr,              /* or NULL */
       (object_func_add_attr)navit_object_add_attr,              /* or NULL */
       (object_func_remove_attr)navit_object_remove_attr,        /* or NULL */
       (object_func_init)NULL,
       (object_func_destroy)navit_object_destroy,                /* or NULL */
       (object_func_dup)NULL,
       (object_func_ref)navit_object_ref,
       (object_func_unref)navit_object_unref,
   };

You can add members of your choice to ``struct mymodule`` **after** the
``NAVIT_OBJECT`` declaration.

For more information on what the ``struct object_func`` members do, look
at the documentation for ``struct object_func``.

Next, edit *navit/xmlconfig.c*. First, we need to tell ``initStatic()``
about the new XML element. Locate the function and edit the line near
the top:

.. code:: c

       elements=g_new0(struct element_func,44); //43 is a number of elements + ending NULL element

The number given here should be the number of elements (including your
new one) plus 1, so you will likely need to increase it by 1.

Then add your element declarations after the ones that are already
there:

.. code:: c

       elements[43].name="mymodule";
       elements[43].parent="navit";
       elements[43].func=NULL;
       elements[43].type=attr_mymodule;

The index should be one higher than the highest existing index in the
list (and one lower than the number of elements you specified before).
The parent element is important: the configuration element for your
module in *navit.xml* must be a child element of the element you specify
here; and your module will be a child of that module in the object
hierarchy. In most cases your module will have ``navit`` as its parent,
but exceptions are possible. For example, maps have ``mapset`` as their
parent element. These instructions assume that your module will live
directly under ``navit``.

Next, you need to tell Navit where to find the methods for your module.
This happens in the same file, in the ``object_func_lookup()`` function.
Look for the ``switch`` statement and add:

.. code:: c

       case attr_mymodule:
           return &mymodule_func;

Since ``mymodule_func`` lives in a different source file, we need to
tell the toolchain to look for it. This happens in *navit/xmlconfig.h*.
Look for a line like the following:

.. code:: c

   extern struct object_func map_func, mapset_func, navit_func, osd_func, tracking_func, vehicle_func, maps_func, layout_func, roadprofile_func, vehicleprofile_func, layer_func, config_func, profile_option_func, script_func, log_func, speech_func, navigation_func, route_func;

Add ``mymodule_func`` to the list.

.. _tell_the_parent_object_about_your_module:

Tell the parent object about your module
----------------------------------------

As previously mentioned, the object instance for your module will get
added to its parent as an attribute. This will only work if the parent
knows what to do with the attribute type—whether that is the case
depends on three things:

-  Are multiple instances of the module allowed?
-  Do the ``add_attr``, ``get_attr`` and ``remove_attr`` methods for the
   parent point directly to the default implementations, or does the
   parent object implement its own method for one or more of them?
-  Does the parent need to react in any way to the module being added,
   removed (if you need to support this) or queried?

If your module supports multiple instances, does not need any special
treatment by its parent and the parent uses the default ``add_attr``
implementation, you’re good. Unfortunately, ``navit`` comes with its own
``add_attr`` method which accepts only explicitly supported attributes.
Just read on.

.. _singleton_modules:

Singleton modules
~~~~~~~~~~~~~~~~~

If your module allows only one instance, look at ``navit_add_attr`` in
*navit/navit.c* to see how Navit handles ``mapset``, ``gui``,
``navigation`` or ``route`` children.

.. _parent_overriding_add_attr_get_attr_or_remove_attr:

Parent overriding ``add_attr``, ``get_attr`` or ``remove_attr``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is something you will need to take care of if the parent object of
your module is ``navit``. In *navit/navit.c*, locate the
``navit_add_attr()`` function. It looks something like this:

.. code:: c

   int
   navit_add_attr(struct navit *this_, struct attr *attr)
   {
       int ret=1;
       switch (attr->type) {
       case attr_callback:
           navit_add_callback(this_, attr->u.callback);
           break;

       /* some lines omitted... */

       case attr_autozoom_max:
           this_->autozoom_max = attr->u.num;
           break;
       case attr_layer:
       case attr_script:
           break;
       default:
           return 0;
       }
       if (ret)
           this_->attrs=attr_generic_add_attr(this_->attrs, attr);
       callback_list_call_attr_2(this_->attr_cbl, attr->type, this_, attr);
       return ret;
   }

If you look closely, you will see that the default behavior is to return
0 (indicating an error), unless the attribute type is explicitly handled
here. Some attributes are handled in a special way, but near the bottom
of the function there are two attribute types (``attr_layer`` and
``attr_script``) which are simply passed through to
``attr_generic_add_attr()``. Add ``attr_mymodule`` here and you’re good.

This works in a similar way for ``get_attr`` and ``remove_attr``. Also
look at the respective functions to see if they do what you expect.

.. _special_handling_by_parent:

Special handling by parent
~~~~~~~~~~~~~~~~~~~~~~~~~~

Again, ``navit_add_attr()`` in *navit/navit.c* is a good example, as
several attribute types are handled in a special manner. Also look at
``navit_get_attr()`` and ``navit_remove_attr()``.

If you are dealing with a parent which relies on the default
implementation and you need some special handling, you need to write
your own method for the parent. Test for the attribute type, have it do
what it needs to do upon detecting your attribute type, then call
through to the default implementation.

.. _initializing_your_module:

Initializing your module
------------------------

You may need to set up a few things on startup, after the whole object
tree has been created. For instance, the GUI module needs to be
connected to the graphics module. The route and tracking modules need to
be told where to find the active mapset. These things happen in the
parent’s ``init`` function. Look at ``navit_init()`` in *navit.c* for
some examples.
