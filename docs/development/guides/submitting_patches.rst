.. _submitting_patches:

Submitting patches
==================

We are happy that you would like to participate and help the
:ref:`team` improve Navit. To make this teamwork productive for
all, we will guide you through this process:

Preparations
------------

Make sure you are familiar with our :doc:`/development/index`,
the codebase, and our guidelines. If you found a bug,
please open a `GitHub issue <https://github.com/navit-gps/navit/issues>`__ and include all relevant details so others
can verify it and help isolate the defective code. Indicate
that you intend to submit a patch.

Process
-------

#. Clone the codebase via
   `GitHub <https://github.com/navit-gps/navit>`__
#. Find the bug (address only one issue per patch) and fix
   it
#. Run ``scripts/ci_sanity_check.sh`` and test your changes
#. Consider possible side effects (performance, different
   settings, etc.)
#. Pull the latest Git Navit version and re-apply your changes
#. Test that everything still works
#. `Create a pull
   request <https://help.github.com/articles/creating-a-pull-request/>`__
   on GitHub

Review
------

It may take some time for someone to review the pull request (you
can ping via :doc:`/user/community/contacts`). If your changes are
complex, raise new ideas, or still have minor issues, we may discuss
them and ask you to submit an updated version.

See also
--------

-  :doc:`/development/programming_guidelines`
-  :doc:`/development/commit_guidelines`
-  :doc:`/user/community/Reporting_Bugs`
-  :doc:`/user/community/Translations`
