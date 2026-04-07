# Agent instructions (Navit / Driver Break tree)

Follow the official [Navit programming guidelines](https://navit.readthedocs.io/en/v0.5.7/development/programming_guidelines.html) and the checks below so changes match CI and project conventions.

## CI sanity checks (`scripts/ci_sanity_checks.sh`)

Used by GitHub Actions (`sanity_check` job) and CircleCI. The script diffs changed files against `origin/trunk`, then:

1. **Trailing whitespace** — Strips trailing spaces on changed **text** files (`.bat` excluded). The job fails if `git diff` is non-empty afterward. Do not leave trailing spaces on touched lines.
2. **C / C++ / headers** — Runs `clang-format -i` on changed `.c`, `.h`, `.cpp`. Style is defined in `navit/.clang-format` (4-space indent, attach braces, 120 column limit, LF). **Skipped paths** (not reformatted by this script): `navit/support/`, `navit/fib-1.1/`, `navit/traffic/permanentrestrictions/`.
3. **Shipped XML** — All `*shipped.xml` files must validate:  
   `xmllint --noout --dtdvalid navit/navit.dtd <file>`
4. **Translations** — The script rebuilds locales and fails if `po/*.po` would change `msgid` entries relative to `po/*.po.in`. Coordinate with the normal gettext workflow when changing translatable strings.

Upstream docs still describe **astyle** for Navit; this repository’s CI uses **clang-format** with `navit/.clang-format` instead. If you format locally, prefer:

```bash
clang-format -i path/to/file.c
```

(from the repo root, or with a config path that picks up `navit/.clang-format`).

## C coding expectations (Navit tradition)

- Treat core Navit code as **C95-oriented**: avoid inline declarations after statements, avoid VLAs (use `g_alloca` + size as in guidelines), use `struct foo m = {0,};` rather than empty `{}` initializers where required by older targets.
- Use **`/* */`** for comments in C sources unless the surrounding file already consistently uses another style.
- Prefer **GLib** helpers (`g_new`, `g_free`, `g_strdup`, `g_strcmp0`, …) over libc equivalents when both exist.
- New or significantly changed **public APIs** should have **Doxygen** file/function comments (see guidelines).

## Other

- **Encoding / line endings**: UTF-8; line feeds only (no CRLF) for text that CI treats as text.
- **Java / Android**: Checkstyle is referenced in CI but currently commented out; the project targets [Google Java Style](https://google.github.io/styleguide/javaguide.html) / `checkstyle.xml` when enabled.
- **Docs**: On the `readthedocs` branch, CircleCI runs **misspell** on `docs/`.
