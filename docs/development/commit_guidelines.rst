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

We have agreed about using the following syntax : ` <Action>:<component>:<log message>[|Optional comments]`

Examples :
 Fix:Core:Fixed nasty bug in ticket #134
 Fix:GTK:Fixed nasty bug about destination button|Thanks someguy for the patch!

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

Commit tips and tricks
======================

There are a few tricks that you can use to make your life easier.

Git hook: commit-msg
--------------------

You can use the following git hook to automatically skip the run of the CI when you are making modifications to
documentation. To use that hook put the following code in the `.git/hooks/commit-msg` file (you will need to have python
installed to have this hook working) and make it executable.

.. code-block:: python
    #!/usr/bin/env python

    import sys
    import os
    import subprocess
    import re


    def replace_commit_msg(message):
        '''
        Replaces the commit message (which is an array of lines) with the given one.
        '''
        with open(sys.argv[1], 'w') as f:
            f.writelines(message)

    def get_commit_msg():
        '''
        commit-msg receives 1 argument which is a temp file where the commit
        message is stored. This function pulls the commit msg.
        '''
        with open(sys.argv[1], 'r') as f:
            return f.readlines()


    if __name__ == '__main__':
        # List of files about to be comitted
        files = subprocess.check_output(['git', 'diff', '--name-only', '--staged']).decode('utf-8').splitlines()

        # file patterns we want to skip
        skip = re.compile(r'^docs/|^CHANGELOG.md$|^CONTRIBUTING.md$|^AUTHORS$|^README.md$|^COPYING$|^COPYRIGHT$|^GPL-2$|^LGPL-2$')

        for item in files:
            match = skip.match(item)
            # Delete files that match the current skip pattern
            if match is not None:
                files.remove(item)

        # Add "[skip ci]" at the end of the commit message if all files match the pattern
        if len(files) == 0:
            msg = get_commit_msg()
            msg[0] = msg[0].rstrip() + ' [skip ci]'+ os.linesep
            replace_commit_msg(msg)

        sys.exit(0)

This will add the sentence ` [skip ci]` automatically at the end of your commit message if all the files you commit are
documentation from the `docs` folder or some other well known documentation that don't need to run through the CI
process as not affecting the build.
