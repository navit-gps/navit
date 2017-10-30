# Change Log
All notable changes to this project will be documented in this file.

Changes and documentation about Navit can be found in the wiki at:
	http://wiki.navit-project.org
A timeline of opened and closed issue tickets can be found at our trac instance:
	http://trac.navit-project.org

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

## [Unreleased]

The list of changes that happened between the release 0.5.0 and the creation of
this changelog is too long to fit there so only a subset has been put here.

For the full list of changes, see [Github full commit list](https://github.com/navit-gps/navit/compare/v0.5.0...HEAD)


### Added
- core: Allow zipcode or town seamless search (#211)
- core: Add feature poly_reservoir (#251)
- core: Add grass, grassland, wetland and sand (#247)
- core: Add poly_basin (#248)
- gui_internal: Add auto zoom toggle to gui_internal (#237)
- gui_internal: Add network info menu in gui/internal for Linux (#228)
- graphics: RaspberryPI hardware accelerated graphics support (#208)
- graphics: Add Qt5 support
- graphics: add multiple icons
- gtk: Add menu entry to toggle vehicle tracking (#362)
- espeak: Add speech module using espeak on QMultimedia. (#233)
- contrib/sailfish: Add rules to build sailfish package (#221)
- contrib/sailfish: Add autozoom switch to sailfosh config (#238)
- contrib/sailfish: Add desktop icons in sizes required by Sailfish OS. (#220)
- install: Allow unusual building (#215)
- doc: Improved follow vehicle toggle doc function for GTK (#363)
- doc: Added download links for PlayStore and F-Droid in the README
- ci: Added automatic publishing to Playstore Beta

### Changed

- core: Removed autotools lefotover (#204)
- core: sunrise near poles simplification (#206)
- core: keep active vehicle profile when deactivating vehicle (#217)
- core: Hide impossible keys at the internal keyboard instead of highlighting the possible ones (leftover from PR5) (#210)
- core: Reduce POST_SHIFT to avoid int overflow
- core: Remove dependency on OpenSSL
- port/android: ask permissions on sdk >= 23
- i18n: Update of about all the translations available for the software
- ci: Updated the CI builds and tests
- ci: update zlib to 1.2.11
- doc: Added some usage images to the README.md

### Fixed
- port/android:moved Taiwan into its own map download entry #348
- maptool:Remove option -5 (MD5 checksum) from maptool
- gtk: fixed missing imperial units in the GTK ui (#359)
- core: Remove binfile map encryption support
- core: Fix iPhone build broken by plugin refactoring
- core: Fix Car layout issues with wood and water (#240)
- core: TRAC-1246: Draw background color even if text label is empty. (#234)
- core: TRAC-981: Add Align-Attr to osd type Odometer (#230)
- core: navigation_analyze_roundabout : central_angle may lead to division by 0 (#218)
- core: Fix POI toggle bug with Car-dark layout (#223)
- core: TRAC-1347 Add 'Follow' and 'Active' to vehicle_demo to remove the corresponding error messages (Unsupported Attribute) (#229)
- gui_internal: Rename new option to hide_impossible_next_keys
- gui_internal: Don't crash if LANG environment not set (#232)
- xml:Change colors for wetland in Car and Car-dark (#257)
- port/android: Fix "invalid DT_NEEDED" warnings on API 23+, fixes #1348 (#205)
- port/android: Fix apk signing and bump sdk to 25 - nougat (#209)
- port/android: Fix #1345 crash on Android
- port/android: TRAC-1071 'Toggle POIs' button also toggle POI labels (#226)
- ci: Tomtom:Switching to mirrored toolchain

### Removed

## [0.5.0] - 2015-12-31

This release was done before the adoption of this changelog format. Click
[here](https://github.com/navit-gps/navit/compare/v0.5.0-rc.2...v0.5.0)
to view the corresponding changes.

## [0.5.0-rc2] - 2015-09-02

This release was done before the adoption of this changelog format. Click
[here](https://github.com/navit-gps/navit/compare/v0.5.0-rc.1...v0.5.0-rc.2)
to view the corresponding changes.

## [0.5.0-rc1] - 2015-08-08

This release was done before the adoption of this changelog format. Click
[here](https://github.com/navit-gps/navit/compare/v0.5.0-beta.1...v0.5.0-rc.1)
to view the corresponding changes.

[Unreleased]: https://github.com/navit-gps/navit/compare/v0.5.0...HEAD
[0.5.0]: https://github.com/navit-gps/navit/compare/v0.5.0-rc.2...v0.5.0
[0.5.0-rc.2]: https://github.com/navit-gps/navit/compare/v0.5.0-rc.1...v0.5.0-rc.2
[0.5.0-rc.1]: https://github.com/navit-gps/navit/compare/v0.5.0-beta.1...v0.5.0-rc.1
