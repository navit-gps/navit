.. _programming_guidelines:

Programming guidelines
======================

.. raw:: mediawiki

   {{warning|1='''The content of this document has been moved to:''' https://navit.readthedocs.io/en/trunk/development/programming_guidelines.html . It is only kept here for archiving purposes.}}

NAVIT is a team-project, thus a lot of coders are working on it's
development and code base. To get a unified coding style and make it
easier for everybody to work with parts, that someone else wrote, we
tried to specify the formatting of our source and how we deal with third
party modules as following.

.. _coding_style:

Coding Style
============

We try to follow those simple rules:

-  indentation is done using 4 spaces
-  the return type of a function is specified on the same line as the
   function name
-  the open brackets should be at the end of the line (on the same line
   as the function name or the if/for/while statement)
-  out line length is of 120 characters

To help us keeping a coherent indentation, we use astyle on C, C++ and
java files. Usage example:

.. code:: c

       astyle --indent=spaces=4 --style=attach -n --max-code-length=120 -xf -xh my_file.c

Note: Because of the bug: https://sourceforge.net/p/astyle/bugs/230/ on
astyle, we cannot rely on astyle to handle properly the line length of
120 characters that we choose. It would be recommended to set that line
length in the editor you are using.

.. _character_encoding_and_line_breaks:

Character encoding and line breaks
----------------------------------

All non-ascii-strings must be utf-8 encoded. Line breaks consist of a
linefeed (no carriage return).

.. _c_standard:

C Standard
----------

C95. That means:

-  No inline declarations of variables

Instead of

.. code:: c

    {
       do_something();
       int a=do_something_else();
    }

use

.. code:: c

    {
       int a;
       do_something();
       a=do_something_else();
    }

-  No dynamic arrays

Instead of

.. code:: c

    void myfunc(int count) {
       struct mystruct x[count]
    }

use

.. code:: c

    void myfunc(int count) {
       struct mystruct *x=g_alloca(count*sizeof(struct mystruct));
    }

-  No empty initializers

Instead of

.. code:: c

    struct mystruct m={};

use

.. code:: c

    struct mystruct m={0,};

-  Use /\* and \*/ for comments instead of //

*Note:* The restriction to C95 exists mainly to help the
`WinCE <WinCE>`__ port, which uses a compiler without full support for
C99. Once all platforms supported by Navit use a compiler capable of
C99, this decision may be reconsidered.

.. _use_of_libraries:

Use of libraries
----------------

-  Navit uses `GLib <http://developer.gnome.org/glib/>`__ extensively.
   In general, code should use GLib's functions in preference to
   functions from libc. For example, use
   ``g_new() / g_free() / g_realloc()``, rather than
   ``malloc() / free() / realloc()``, ``g_strdup()`` rather than
   ``strdup()``, ``g_strcmp0()`` rather than ``strcmp()`` etc.
-  Unfortunately, not all platforms that Navit runs on have a native
   GLib version. For these platforms, there is code replacing these
   libraries under ``navit/support/``. Take care to only use functions
   from GLib (or other libraries), that is also present under
   ``navit/support/``. If you need something that is not present there,
   please discuss it on IRC - it may be possible to add it to the
   support code.

Comments
--------

.. _general_guidelines:

General guidelines
~~~~~~~~~~~~~~~~~~

-  Comments for the entire *file* and
   *classes/structs/methods/functions* is the **minimum requirement**.
   Examples see below.
-  Please comment your code in a significant and reasonable way.
-  A quick description of (complicated) algorithms makes it easier for
   other developers and helps them to save a lot of time.
-  Please add a doxygen description for all function you should add. You
   are we1come to add it too to older functions. Doxygen result can be
   found there : http://doxygen.navit-project.org

Example :

.. code:: c

    /**
    * @brief Change the current zoom level, zooming closer to the ground.
    *
    * This sentence starts the "detailed" description (because this is a new
    * paragraph).
    *
    * @param navit The navit instance
    * @param factor The zoom factor, usually 2
    * @param p The invariant point (if set to NULL, default to center)
    * @returns nothing
    */
    void navit_zoom_in(struct navit *this_, int factor, struct point *p)

Templates
~~~~~~~~~

This is an example how you could (should) comment your files and
functions. If you have any suggestions for improvement, please feel free
to `discuss <Talk:Programming_guidelines>`__ them with us. These
templates should be doxygen-conform - if not, please correct them. A
more comprehensive overview of possible documentation can be found
`here <http://www.stack.nl/~dimitri/doxygen/manual.html>`__.

Files
^^^^^

.. code:: c

   /** @file can.cpp
    * @brief CAN-Camera Framework :: CAN container class and high level functions
    * 
    * Some documentation regarding this file.
    *
    * @Author Stefan Klumpp <sk@....> 
    * @date 2008
    */
   <include "can.h">
   .
   .
   .

Classes/Structs/Functions/Methods
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: c

   /** 
    * @brief A short description of this function
    *
    * A lot more of documentation regarding this function.
    * @param raw_data Some string to pass to the function
    * @return Nothing
    */

   void CanData::processData(string &raw_data) {
   .
   .
   .
   }

Please add yourself to the list of authors, if you make a significant
change.

Contributing
============

.. raw:: mediawiki

   {{warning|1='''The content of this section has been moved to:''' https://github.com/navit-gps/navit/blob/trunk/CONTRIBUTING.md . It is only kept here for archiving purposes.}}

We welcome contributions!

The easiest way to get started is to fork Navit, work on the feature and
submit a pull request.

The following workflow will make it easier for us to review your
changes:

-  start working on your improvement with an up to date copy of trunk:

``  git checkout trunk; git pull``

-  always use a branch for your improvement :

``  git checkout -b ``\ 

-  improve the thing
-  always document new functions according to the doxygen standard
   discussed above
-  double check that you are on the correct branch with git status
-  commit your change
-  push your branch

``  git push -u origin ``\ 

-  open a `pull
   request <https://help.github.com/articles/about-pull-requests/>`__

If at some point you want to get write access to our repository, just
`contact us <https://wiki.navit-project.org/index.php/Contacts>`__.
