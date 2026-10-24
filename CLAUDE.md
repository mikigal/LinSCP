# OpenSCP

Native Linux (Wayland) port of WinSCP. This repository is a fork of upstream WinSCP; a Windows
build will never be made from it. Read `docs/port/PLAN.md` (architecture, milestones, decisions)
and `docs/port/PROGRESS.md` (where the work stands) before starting.

## Rules

- Everything in the repository is English: code, comments, commit messages, documentation.
- Commit directly to `master`, in small logical commits. Never push — the user pushes.
  `master` must always build and pass `ctest --preset dev`.
- Never run a GUI under test on the user's real desktop (`WAYLAND_DISPLAY`, `DISPLAY`). GUI runs
  only inside the headless KWin harness (`tools/wlharness`).
- Never touch the user's `~/.config`, `~/.ssh` or SSH agent. Tests use isolated directories,
  the local test sshd, or the test VM.
- Do not modify Windows-only upstream directories (`source/forms`, `source/windows`,
  `source/packages`, `source/components`, `source/dragext`, `dotnet`, `deployment`). Keep manual
  edits in converted upstream code minimal; new code goes into new directories. See
  `docs/port/UPSTREAM.md`.
- Update `docs/port/PROGRESS.md` when a milestone step is completed or a decision is made.

## Build and test

Clang only (the port relies on `-fdeclspec`); the presets select it.

```bash
cmake --preset dev                  # Debug + ASan/UBSan + tests + harness, build/dev
cmake --build --preset dev
ctest --preset dev                  # all tests
ctest --preset unit                 # unit tests only
ctest --preset dev -L gui           # labels: unit, integration, ssh, gui
```

`WLH_TMPDIR` sets where harness sessions live; the path must be short (Wayland socket path
limit, 108 bytes). In Claude's sandbox use `WLH_TMPDIR=/tmp/claude-1000`.

## Wayland test harness (`tools/wlharness`)

`wlharness.py` starts an isolated headless `kwin_wayland --virtual` session (own D-Bus,
`XDG_RUNTIME_DIR`, `HOME`, pinned Breeze theme, no animations) inside a user+mount namespace,
so the XDG document portal can mount FUSE without setuid (needed for Qt drag & drop of files).

```bash
S=$(tools/wlharness/wlharness.py start --width 1280 --height 800)
tools/wlharness/wlharness.py -s "$S" spawn --log app -- build/dev/tools/wlharness/wlh-probe
tools/wlharness/wlharness.py -s "$S" windows                 # JSON with frame/client geometry
tools/wlharness/wlharness.py -s "$S" place wlh-probe 40 40 600 400
tools/wlharness/wlharness.py -s "$S" input move 100 120 click type hello key ctrl+a
tools/wlharness/wlharness.py -s "$S" shot "$S/shot.png"      # then look at the PNG
tools/wlharness/wlharness.py -s "$S" stop
```

Logs of every process are in `$S/logs/`. `wlh-input` injects real compositor input through
`org_kde_kwin_fake_input` (required for drag & drop); `wlh-probe` is a peer window that logs
mouse, keyboard, drag & drop and clipboard events as JSON lines. From Python tests use
`wlharness.Session` directly (see `tests/integration/test_wlharness.py`).

## Test servers (`tools/testenv`)

- `local_sshd.py`: unprivileged OpenSSH on `127.0.0.1` with its own keys (public key auth only).
- Test VM over SSH (root, from milestone M3): FTP/FTPS, WebDAV, S3, password authentication.
  Provisioned only by scripts in `tools/testenv/vm/`.

## Code style

New C++ code follows the WinSCP conventions so ported and new code read the same: Allman
braces, 2-space indentation, `T` class prefix, `F` field prefix, `A` prefix for parameters that
would clash with properties, PascalCase for functions and locals. Python follows PEP 8.
