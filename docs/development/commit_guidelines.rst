======================
Commit guidelines
======================

Here are the current guidelines if you want to commit something. Please also read the [programming guidelines](../programming_guidelines.html)
We are using conventional commits because when the release comes we want to be
the process of creating a changelog as simple as possible. That is why, commit
message conventions are enforced at PR-time via the same tool, that is also used in
assembling the changelog: git-cliff.

Commit per feature
==================

When committing, try to have one commit per feature (or per meaningful part of a larger feature). The goal is to always have working code; at least make sure each commit leaves the repository in a compilable state.

Also avoid putting multiple, independent changes into one commit.
Thus if you have multiple, independent changes in your local working copy, avoid committing a whole folder at once, especially Navit's Sourcecode root. Instead, explicitly select the files for each commit.

Format of the commit log
========================

Since we are too lazy to maintain a Changelog, we have a script which parses the commit logs and generate a Changelog for us.
For this to work, the (https://www.conventionalcommits.org/en/v1.0.0/)[conventional commit standard] is enforced.

In the message body, try to explain the reason for the change rather than how
the change was implemented.
