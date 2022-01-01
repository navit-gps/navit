Attributes
==========

Attributes are a main communication concept within navit. Callbacks and
configuration for example are realized with attributes. todo: image of
the lifecycle of an attribute.

Types
-----

| Attributes in navit are defined by their type. Type can be string,
  int, bool, object and some more. (see attr_def.h for all types)
| In the source code the attributes are represented as structs which
  hold all data.

Definition
----------

If one wants to use an existing attribute, nothing special has to be
done. If one wants to introduce a new attribute to navit, it must be
defined in the file attr_def.h.

Configuration
-------------

All content of navit.xml is converted to attributes on startup. For
example:

.. code:: xml

   <speech type="cmdline" data="echo 'Fix the speech tag in navit.xml to let navit say:' '%s'" cps="15"/>

This configuration tag holds four attributes. One for the object
(speech) and three for the objects configurable attributes. These
configuration attributes will be passed to the object (speech) during
initialisation and can be checked in the code to use them. Attributes
can be retrieved like this:

.. code:: c

   attr = attr_search(attrs, NULL, attr_type);

Callbacks
---------

If an attribute is changed, it is possible to attach a callback to it to
notify objects and do things on the change. One important example for
this feature is the ``enable_expression``, which can show or hide a OSD
item depending on the status of the attribute. To notify the enable
expression on the change of an attribute the attribute must be linked to
a callback.

todo...

.. _user_defined_attributes:

User defined Attributes
-----------------------

It is also possible to define an attribute from within the code and not
only from navit.xml.
