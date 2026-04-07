# Navit (this repo) — notes for Claude

## Authoritative docs

- [Programming guidelines (Read the Docs v0.5.7)](https://navit.readthedocs.io/en/v0.5.7/development/programming_guidelines.html)

## What CI actually runs

Script: **`scripts/ci_sanity_checks.sh`** (CircleCI “sanity” job).

1. **`check_po`**: `cmake` + `make locales` in `build-locale/po`; each generated `*.po` must match `po/*.po.in` on `msgid` lines. Update `po/*.po.in` when translatable strings change. Duplicate `msgid` in a `.po.in` breaks `msgmerge`.
2. **Per file** in `git diff` vs `SANITY_GIT_BASE` (default: `origin/trunk`, else `main` / `master` / `@{upstream}` / `HEAD`):
   - Trailing whitespace removed on text files: `sed 's/\s*$//' -i` (skip `.bat`).
   - **`clang-format -i`** on `*.c`, `*.h`, `*.cpp` using **`navit/.clang-format`** (aligned with clang-format 18 in this tree).
   - Skipped directories: `navit/support/`, `navit/fib-1.1/`, `navit/traffic/permanentrestrictions/`.
3. **All `*shipped.xml`**: `xmllint --noout --dtdvalid navit/navit.dtd`.
4. **`git diff` must be empty** after the above (commit formatter/PO/XML fixes).

The RTD v0.5.7 page still mentions **astyle** for C; this repository’s sanity script uses **clang-format**—follow `.clang-format`, not astyle, for CI.

## C style (summary)

4 spaces, attached braces, ~120 columns, LF, UTF-8; C95-oriented rules in the guidelines (`/* */`, GLib over libc, Doxygen for new APIs). Match existing files in the same directory.

## Java

Google Java Style; `checkstyle.xml`; `./gradlew checkstyleMain` for Android/Java changes.

## Cursor

Project rules live in **`.cursor/rules/*.mdc`** (same expectations as this file).
