.. _release_process:

Release process
===============

When we release Navit, here are the steps we have to follow for now
(April 2018):

-  Update NAVIT_VERSION_MAJOR, NAVIT_VERSION_MINOR and
   NAVIT_VERSION_PATCH in **CMakeLists.txt**
-  Update Sailfish spec (Version, Release and Changelog) in
   **contrib/sailfish/navit-sailfish.spec**
-  update the contributors list using:
   **scripts/generate_contributors.sh** (takes a little while because of
   the size of our git history, just be patient)
-  generate changelog using **~/.local/bin/gitchangelog ^v0.5.1 HEAD**
   with **^v0.5.1** being the latest tag available. Then edit the
   CHANGELOG.md and clean it up (no, the gitchangelog python module is
   not perfect :) )
-  cut tag (verify that master is up-to-date with trunk)
-  Wait that circleci finishes to build all the jobs for the master
   branch (not trunk)
-  attach artifacts from the master branch build from circleCI to the
   tag (don't forget the .cab and the .exe for wince)
-  Generate the release based on the tag
-  download the tarball and generate the release signature using **gpg
   --armor --detach-sign ** then attach the **.asc** file as an artifact
-  Grab the Versioncode from the build_android and update
   https://github.com/navit-gps/download-center/edit/master/_data/version.yml
   (Search for "Version Code" within the Build Log on Circleci)

-  On
   `Sourceforge <https://sourceforge.net/projects/navit/files/?source=navbar>`__:

   -  wait about 30min that the files appear at:
      https://sourceforge.net/projects/navit/files/
   -  If that's not the case you might have to reset the integration:
      https://sourceforge.net/p/navit/admin/files/gh_integration
   -  On the folder of the latest release, click on the little
      information symbol at the end of the line for the apk file and set
      it as default for android, set the win32 one as default for
      windows and the tarball as default for all the rest, this will
      make the latest download link on sourceforge point to the right
      file
   -  update sourceforge news by adding a new post:
      https://sourceforge.net/p/navit/news/

-  On Launchpad:

   -  update: https://launchpad.net/navit/+announcements
   -  verify that https://code.launchpad.net/navit is up-to-date

-  

   -  add the milestone and release on the trunk series, then create the
      release: https://launchpad.net/navit/trunk

-  publish a post to:

   -  twitter: https://twitter.com/navitproject
   -  facebook: https://www.facebook.com/NavitProject

-  update the website download section and the blog:
   https://github.com/navit-gps/website/
-  update https://en.wikipedia.org/wiki/Navit
