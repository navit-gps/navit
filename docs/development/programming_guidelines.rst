======================
Programming guidelines
======================

NAVIT is a team-project, thus a lot of coders are working on it's development and code base.
To get a unified coding style and make it easier for everybody to work with parts, that someone else wrote, we tried to specify the formatting of our source and how we deal with third party modules as following.

Coding Style
============

We try to follow those simple rules:
 * indentation is done using 4 spaces
 * the return type of a function is specified on the same line as the function name
 * the open brackets should be at the end of the line (on the same line as the function name or the if/for/while statement)
 * out line length is of 120 characters

To help us keeping a coherent indentation, we use astyle on C, C++ and java files. Usage example:

.. code-block:: C

    astyle --indent=spaces=4 --style=attach -n --max-code-length=120 -xf -xh my_file.c


.. note::

    Because of the bug: [https://sourceforge.net/p/astyle/bugs/230/](https://sourceforge.net/p/astyle/bugs/230/) on astyle,
    we cannot rely on astyle to handle properly the line length of 120 characters that we choose.
    It would be recommended to set that line length in the editor you are using.

Character encoding and line breaks
----------------------------------

All non-ascii-strings must be utf-8 encoded. Line breaks consist of a linefeed (no carriage return).

C Standard
----------

C95. That means:
 * No inline declarations of variables

Instead of

.. code-block:: C

    {
       do_something();
       int a=do_something_else();
    }

use

.. code-block:: C

    {
       int a;
       do_something();
       a=do_something_else();
    }

 * No dynamic arrays

Instead of

.. code-block:: C

    void myfunc(int count) {
       struct mystruct x[count]
    }

use

.. code-block:: C

    void myfunc(int count) {
       struct mystruct *x=g_alloca(count*sizeof(struct mystruct));
    }

 * No empty initializers

Instead of

.. code-block:: C

    struct mystruct m={};

use

.. code-block:: C

    struct mystruct m={0,};

 * Use `/*` and `*/` for comments instead of `//`

.. note::

    The restriction to C95 exists mainly to help the [[WinCE]] port, which uses a compiler without full support for C99. Once all platforms supported by Navit use a compiler capable of C99, this decision may be reconsidered.


Use of libraries
----------------

 * Navit uses `GLIB <http://developer.gnome.org/glib/>`_ extensively. In general, code should use GLib's functions in preference to functions from libc.
   For example, use `g_new()` / `g_free()` / `g_realloc()`, rather than `malloc()` / `free()` / `realloc()`, `g_strdup()` rather than `strdup()`, `g_strcmp0()` rather than `strcmp()` etc.
 * Unfortunately, not all platforms that Navit runs on have a native GLib version.
   For these platforms, there is code replacing these libraries under `navit/support/`.
   Take care to only use functions from GLib (or other libraries), that is also present under `navit/support/`.
   If you need something that is not present there, please discuss it on IRC - it may be possible to add it to the support code.

Comments
--------

General guidelines
``````````````````

 * Comments for the entire `file` and `classes/structs/methods/functions` is the `'minimum requirement`'. Examples see below.
 * Please comment your code in a significant and reasonable way.
 * A quick description of (complicated) algorithms makes it easier for other developers and helps them to save a lot of time.
 * Please add a doxygen description for all function you should add. You are we1come to add it too to older functions. Doxygen result can be found `there <http://doxygen.navit-project.org>`_

Example :

.. code-block:: C

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
`````````

This is an example how you could (should) comment your files and functions. If you have any suggestions for improvement, please feel free to [[Talk:Programming_guidelines|discuss]] them with us. These templates should be doxygen-conform - if not, please correct them. A more comprehensive overview of possible documentation can be found `here <http://www.stack.nl/~dimitri/doxygen/manual.html>`_.

Files
'''''

.. code-block:: C

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
'''''''''''''''''''''''''''''''''


.. code-block:: C

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

Please add yourself to the list of authors, if you make a significant change.
