% if data["title"]:
# ${data["title"]}
% endif
All notable changes to this project will be documented in this file.

Changes and documentation about Navit can be found in the wiki at:
  http://wiki.navit-project.org
A timeline of opened and closed issue tickets can be found at our trac instance:
  http://trac.navit-project.org and on our github project:
  https://github.com/navit-gps/navit/issues

Navit follows the semantic versioning:
* `x.y.Z` (patch): only bug fixes or refactoring, no changes in functionality
* `x.Y.z` (minor): added or changed functionality but can be used as a drop-in
  replacement for the previous version (all data formats and interfaces are still
  supported); minor UI changes (such as moving individual menu items) are also
  allowed
* X.y.z (major): at least one of the following:
  * Major new functionality (such as Augmented Reality, inertial navigation or
    support for live traffic services): de-facto standard for end-user apps
  * New user interface (such as moving from the old pulldown menu UI to the Internal
    GUI): this is definitely the UI equivalent of a breaking API change
  * Dropped support for a data format or interface: also a breaking change and
    usually tends to occur along with larger changes which would warrant a new major
    version anyway

% for version in data["versions"]:

<% title = "## [%s] - %s" % (version["tag"], version["date"]) if version["tag"] else "## %s" % opts["unreleased_version_label"] %>${title}
% for section in version["sections"]:

<% lbl = "### %s" % section["label"] %>${lbl}

% for commit in section["commits"]:
<%
author = commit["author"].replace('_', '\_')
subject = "%s [%s]" % (commit["subject"], author)
entry = indent(subject, first=" * ").strip()
%>${entry}
% endfor
% endfor
% endfor
