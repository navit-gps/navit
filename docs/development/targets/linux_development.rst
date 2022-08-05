.. _linux_development:

Linux development
=================

.. raw:: mediawiki

   {{warning|1='''This is a page has been migrated to readthedocs:'''https://github.com/navit-gps/navit/pull/880 . It is only kept here for archiving purposes.}}

.. _taking_care_of_dependencies:

Taking care of dependencies
===========================

See the `Dependencies <Dependencies>`__ page for a list of software
packages required to build Navit. To build with `CMake <#CMake>`__ you
will need **CMake 2.6** or newer.

.. _getting_navit_from_the_git_repository:

Getting Navit from the GIT repository
=====================================

First, let's make sure we are in our home directory: this is only for
the sake of making this tutorial simple to follow. You can save that
directory anywhere you want, but you will have to adapt the rest of the
instructions of this guide to your particular case.

``cd ~ ``

Now, let's grab the code from Git. This assumes that you have git
binaries installed.

`` git clone ``\ ```https://github.com/navit-gps/navit.git`` <https://github.com/navit-gps/navit.git>`__

Compiling
=========

GNU autotools was the old method but is removed in favour of CMake.

CMake
-----

CMake builds Navit in a separate directory of your choice - this means
that the directory in which the Git source was checked out remains
untouched. See also `CMake <CMake>`__.

| ``mkdir navit-build``
| ``cd navit-build``

Once inside the build directory just call the following commands:

| ``cmake ~/navit``
| ``make``

**Note:** CMake will autodetect your system configuration on the first
run, and cache this information. Therefore installing or removing
libraries after the first CMake run may confuse it and cause weird
compilation errors (though installing new libraries should be ok). If
you install or remove libraries/packages and subsequently run into
errors, do a clean CMake run:

| `` rm -r ~/navit-build/*``
| `` cmake ~/navit``

.. _running_the_compiled_binary:

Running the compiled binary
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The binary is called **navit** and can be run without arguments:

| ``cd ~/navit-build/navit/``
| ``./navit``

It is advised to just run this binary locally at the moment (i.e. not to
install system-wide). Note that for this to work, Navit must be run from
the directory where it resides (that is, you must first change your
working directory, as described above). If Navit is run from another
directory, it will not find its plugins and image files, and will not
start.

.. _running_the_compiled_binary_1:

Running the compiled binary
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Here, I am skipping the usual "make install" because we don't need to
install navit systemwide for this example.

To execute navit, you can simply click on the binary file (if you are
sure it is compiled properly) and it should launch. If you prefer to
launch it from a terminal, you need to go into the directory containing
the binary, first, like so:

| ``cd ~/``\ **``navit``**\ ``/navit/``
| ``./navit``

.. _updating_the_git_code:

Updating the GIT code
=====================

You don't need to recompile everything to update navit to the latest
code; with 'git pull' only the edited files will be downloaded. Just go
to the navit directory (e.g. /home/CHANGEME/navit) and run:

``git pull``

You then only need to run "make" again from your binary folder (
navit-build in the cmake example, or the current folder when using
autotools).

.. _prebuild_binairies:

Prebuild binairies
==================

`Prebuilt binaries <Download_Navit>`__ exist for many distributions.

.. _configuring_the_beast:

Configuring the beast
=====================

This is `Configuration <Configuration>`__, young padawan. Good luck :)

You can also check a `post describing a Navit configuration on Ubuntu
Jaunty <http://www.len.ro/2009/07/navit-gps-on-a-acer-aspire-one/>`__.
