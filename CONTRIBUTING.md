# Contributing

Welcome to Navit! We welcome contributions!

If you are interested in contributing to the [Navit code repo](README.md)
then checkout the [old Wiki](https://wiki.navit-project.org/index.php/Main_Page)
and our [new wiki hosted on ReadTheDocs](https://navit.readthedocs.io)

When pushing a pull request, please make sure you follow our:
* [programming guidelines](https://navit.readthedocs.io/en/trunk/development/programming_guidelines.html)
* [commit message guidelines](https://navit.readthedocs.io/en/trunk/development/commit_guidelines.html)

For more information on our development process, see: https://wiki.navit-project.org/index.php/Development

## Submitting patches

We are very happy that you like to participate and help the [[team]] to improve Navit :) To make this teamwork a pleasure for all, we will try to guide you trough this process:

### Preparation

Make sure you are familar with our (development tips)[https://wiki.navit-project.org/index.php/Development], learned about the codebase and our guidelines.
If you found a bug, please open a [GitHub issue](https://github.com/navit-gps/navit/issues) and bring up all details so others can check them and help you on isolating the defective code.
Point out that you like to submit a patch.

### Contibuting via a Pull Request

The easiest way to get started is to fork Navit, work on the feature and submit a pull request.

Prepare your repository:
 * Fork the [Github repository](https://github.com/navit-gps/navit) and clone it using `git clone` (note that we use the `trunk`
   branch as our branch of reference for developments)
 * always use a separate branch for your improvement: ` git checkout -b <name of your branch>`

Enhance Navit:
 * Find the bug (and please address only one issue per patch!) and try to fix it
 * always document new functions according to the doxygen standard discussed in the [programming guidelines](https://navit.readthedocs.io/en/trunk/development/programming_guidelines.html)
 * Test test test if still compiles and the behaviour is as expected
 * Think about possible side effects (as performance, different settings, ...)

Submit your work:
 * Get the newest Git Navit version (see [this documentation](https://help.github.com/en/articles/syncing-a-fork) on how to sync your fork) and apply your changes once more
 * Test if everything still works fine
 * double check that you are on the correct branch with git status
 * commit your change using our [commit message guidelines](https://navit.readthedocs.io/en/trunk/development/commit_guidelines.html)
 * push your branch (`git push origin <name of your branch>`)
 * [Create a pull request](https://help.github.com/articles/creating-a-pull-request/) on github
 * Wait to verify that all the tests in our CI finish successfully

If at some point you want to get write access to our repository, just [contact us](https://wiki.navit-project.org/index.php/Contacts).

### Review

It might take some time until somebody reviews the pull request (maybe try to reach out using one of the various ccontact methods [listed in the wiki](https://wiki.navit-project.org/index.php/Contacts)).
If your changes are more complex, catch up new ideas, or still have some minor problems, it might be discussed and we might ask you to submit an updated version/adapt your changes.

So that's it, you helped Navit to go one step forward. Thank you very much :)

## Contributing on the documentation

For the documentation we now use [readthedocs](https://navit.readthedocs.io/en/trunk/) pulled from the docs folder inside this repository<.

We follow the [Documentation style guide from Sphinx](https://documentation-style-guide-sphinx.readthedocs.io/en/latest/style-guide.html) (as readthedocs uses sphinx).
With a small exception which is that we still use the `.rst` extension instead of the recommended `.txt` one for now.

The [old wiki](https://wiki.navit-project.org/index.php) has been deprecated and is slowly being migrated to our
[new wiki](https://navit.readthedocs.io/en/trunk/).

To update or add documentation, create a pull request on github using a PR title starting by `update:doc:` and followed by a
message explaining the change.

You can use the following command from the root of the git repository clone to check the linting of the documentation (The last argument is the output directory so you might have to adjust that) if you have `sphinx-build` installed:

```
mkdir /tmp/navit
sphinx-build -Dhtml_theme="sphinx_rtd_theme" -n -E -W --keep-going docs /tmp/navit/
```

Now if you want to view how the result would look like you can:
 * open <file:///tmp/navit/index.html> in your favorite browser
 * or if you prefer to run an http server on the directory you created the build in (`/tmp/navit/` in the previously mentionned command) you can do so using: `python3 -m http.server 8000` and then call <http://localhost:8000> from your browser.

## See also

 * [programming guidelines](https://navit.readthedocs.io/en/trunk/development/programming_guidelines.html)
 * [commit message guidelines](https://navit.readthedocs.io/en/trunk/development/commit_guidelines.html)
 * [Reporting Bugs](https://wiki.navit-project.org/index.php/Reporting_Bugs)
 * [Translations](https://wiki.navit-project.org/index.php/Translations)
