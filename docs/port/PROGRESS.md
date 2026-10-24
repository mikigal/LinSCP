# Progress log

Newest first. Milestones are defined in `PLAN.md`, section 7.

## M0 — Infrastructure (done, 2026-10-24)

- CMake + Ninja, Clang-only, presets `dev` (Debug, ASan+UBSan) and `release`.
- `tests/unit/ToolchainTest.cpp` verifies the compiler features the port relies on:
  `__declspec(property)` incl. indexed properties, 32-bit `wchar_t`.
- `tools/testenv/local_sshd.py` + `integration.local_sshd` test: unprivileged sshd, shell
  command, SFTP round trip, host key verification.
- `tools/wlharness`: headless KWin sessions, `wlh-input` (fake-input injector), `wlh-probe`
  (event-logging peer window), KWin scripting for window geometry/placement, screenshots via
  spectacle. `integration.wlharness` verifies screenshots, exact pointer coordinates, keyboard
  input, and real drag & drop between two applications.
- Upstream remote and vendor-branch workflow documented in `UPSTREAM.md`.

Findings:

- A Plasma session puts `~/.config/kdedefaults` into `XDG_CONFIG_DIRS`; inheriting it leaks the
  user's theme into test sessions (and a missing Aurorae theme makes KWin fall back to
  client-side decorations, which breaks geometry). The harness pins XDG directories and Breeze.
- Qt 6.12 always offers `application/vnd.portal.filetransfer` for dragged files, and the
  receiver gets no URLs if the document portal is broken (no fallback to `text/uri-list`).
  The portal needs FUSE; under `no_new_privs` the setuid `fusermount3` fails, so the harness
  runs each session in a user+mount namespace with capabilities kept for the bus-activated
  portal. Relevant for M7: OpenSCP's drop handling depends on a working document portal.
- Qt logs to the systemd journal when stderr is not a terminal; the harness sets
  `QT_FORCE_STDERR_LOGGING=1`.

## Next: M1 — RTL shim

`source/rtl` with GoogleTest coverage under ASan/UBSan; then make core headers parse.
