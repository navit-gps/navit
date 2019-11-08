======================
Commit guidelines
======================

Here are the current guidelines if you want to commit something. Please also read the [programming guidelines](../programming_guidelines.html)

'Core' components changes
=========================

Do not modify a 'core' component without discussing it first with the project leads.

Core components include data structures, configuration handling. If you are unsure, just ask.

Commit per feature
==================

When committing, try to have one commit per feature (or per meaningful part of a larger feature). The goal is to always have working code; at least make sure each commit leaves the repository in a compilable state.

Also avoid putting multiple, independent changes into one commit.
Thus if you have multiple, independent changes in your local working copy, avoid committing a whole folder at once, especially Navit's Sourcecode root. Instead, explicitly select the files for each commit.

Format of the commit log
========================

Since we are too lazy to maintain a Changelog, we have a script which parses the commit logs and generate a Changelog for us.

We have agreed about using the following syntax : `<Action>:<component>:<log message>[|Optional comments]`

Examples :
 * Fix:Core:Fixed nasty bug in ticket #134
 * Fix:GTK:Fixed nasty bug about destination button|Thanks someguy for the patch!

Action can be something like:
 * Fix (bug fix)
 * Add (new feature)
 * Patch
 * Refactoring (does not change behavior of the program)

It allows the changes to be sorted by categories

The most common components are:
 * core
 * gui/gtk
 * gui/internal
 * graphics/gtk
 * graphics/qt_qpainter
 * graphics/opengl
 * mapdriver
 * tools

The comment part is optional. Useful for when applying a patch for example, and giving credits.
The part after `|` will not appear in the wiki.

About the log message, it's up to you :)
