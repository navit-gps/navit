# Claude / AI project context

Use **`AGENTS.md`** in this repository for Navit CI rules, `clang-format` usage, XML validation, C style expectations, and links to the official [programming guidelines](https://navit.readthedocs.io/en/v0.5.7/development/programming_guidelines.html).

Summary for quick reference:

- No **trailing whitespace** on edited text files (CI enforces via `scripts/ci_sanity_checks.sh`).
- Format **`.c` / `.h` / `.cpp`** under `navit/` with **`clang-format`** and `navit/.clang-format` (120 cols, 4 spaces, attach braces); skip applies to `navit/support/`, `navit/fib-1.1/`, `navit/traffic/permanentrestrictions/`.
- Validate **`*shipped.xml`** with `xmllint --noout --dtdvalid navit/navit.dtd`.
- Prefer **C95-safe patterns**, **`/* */`** comments, and **GLib** in Navit C code; add **Doxygen** for new APIs.
