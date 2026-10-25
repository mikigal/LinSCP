# LinSCP — plan for porting WinSCP to Linux (Wayland)

Status: accepted · 2026-10-24

## 1. Goals and assumptions

- A native Linux application with WinSCP's functionality: SFTP, SCP, FTP/FTPS, WebDAV, S3,
  site manager, Commander/Explorer interfaces, transfer queue, synchronization, file editing
  (internal and external), scripting (a `winscp.com`-compatible CLI), command console.
- First-class Wayland support (KDE Plasma/KWin is the reference platform, but with no hard
  dependency on KDE — it must also work on GNOME/wlroots).
- Proper desktop integration: drag & drop in both directions with the file manager, file
  clipboard, XDG portals, notifications, `sftp://` URL handling, etc.
- A Windows version will **never** be built from this repository → the code may stop being
  compilable with C++Builder; we do not maintain `#ifdef`s for Windows.
- The repository is a 1:1 fork of upstream WinSCP and **we keep pulling new upstream
  releases** (security and protocol fixes). This strongly shapes the strategy (section 3.2).
- Everything in the repository (code, comments, commit messages, documentation) is written
  in English.

## 2. Analysis results

| Directory | Size | What it is | Dependencies | Decision |
|---|---|---|---|---|
| `source/core` | 69 files, ~76k lines | protocol logic, sessions, queue, scripting, synchronization | Delphi RTL (`UnicodeString`, `TStrings`, `TDateTime`, …), C++Builder dialect, some WinAPI | **port** (RTL shim + dialect conversion) |
| `source/putty` | ~83k lines | PuTTY 0.85 with WinSCP modifications (51× `MPEXT`) | only the `windows/` part is platform specific | **port**: shared code unchanged, `windows/` → `unix/` from upstream PuTTY 0.85 |
| `source/filezilla` | ~19k lines | FTP engine from FileZilla 2 | MFC (`CString`, `CTime`, `CFile`), `CAsyncSocketEx` on hidden windows and `WSAAsyncSelect` | **port**: own mini MFC shim + `poll()` reactor instead of a window message loop |
| `libs/neon`, `libs/libs3` | — | WebDAV/HTTP and S3, **modified by WinSCP** (31 and 22 files with markers) | OpenSSL, expat — available on the system | **build the in-tree versions** (not the system ones), CMake |
| `libs/openssl`, `libs/expat` | — | library copies | — | use system ones (`openssl` 3.6, `expat` 2.9) |
| `libs/mfc` | — | excerpt of MFC sources | Win32 | **dropped** (replaced by the shim) |
| `source/windows` | ~29k lines | GUI glue: configuration, `TerminalManager`, `QueueController`, `EditorManager`, `ConsoleRunner`, `WinInterface` | VCL + WinAPI | **partially ported** (logic), the rest rewritten for Qt |
| `source/forms` | 48 `.dfm` forms, ~47k lines | all windows and dialogs (`CustomScpExplorer.cpp` alone is 12.6k lines) | VCL | **rewritten in Qt 6**, `.dfm` used as the specification |
| `source/components`, `source/packages` | ~125k lines | DirView/DriveView, Toolbar2000/TBX, DragDrop, JCL, PNG | VCL, Pascal | **dropped** — Qt equivalents |
| `source/dragext` | <1k lines | Explorer shell extension for D&D | COM | **dropped** — replaced by the mechanism in section 5 |
| `source/console` | ~1k lines | `winscp.com` (thin client of `WinSCP.exe /console`) | WinAPI IPC | **replaced** by a single CLI binary |
| `dotnet/` | — | .NET assembly driving `winscp.com` | Win32 P/Invoke (events, file mapping) | **out of scope** for the first version |
| `deployment/` | — | Inno Setup installer | — | **replaced** by PKGBUILD (Flatpak/AppImage later) |
| `translations/` | 3.7k lines/language | `.ini` translations keyed by `Form.dfm` → `Component.Caption` | — | **reused** (section 3.4) |

Key observations:

1. **The core is cleanly separated from the GUI.** Communication goes through `core/Interface.h`
   (callbacks such as `QueryUser`, `PromptUser`, `ShowExtendedException`, `ProcessGUI`) and
   `TTerminal` events. Direct WinAPI use in the core is small and concentrated in a few files
   (`Terminal.cpp` — local file operations, `Common.cpp`, `HierarchicalStorage.cpp` — registry,
   `SecureShell.cpp` — socket events, `PuttyIntf.cpp`, `FileBuffer.cpp`).
2. **The main barrier is the dialect and the RTL, not WinAPI.** In the core: 613× `__property`,
   45× `__closure` types (+ assignments of methods as event handlers), 181× `try/__finally`,
   5277× `__fastcall`, 6249× `UnicodeString`, ~700× `TStrings/TStringList`, 266× `TDateTime`,
   ~700× Delphi-style formatting (`FORMAT`/`FMTLOAD` → `Format(..., ARRAYOFCONST(...))`).
   No compiler other than C++Builder compiles this without a compatibility layer.
3. **Local paths are "Windows style".** 343 calls to `IncludeTrailingBackslash`/`ExtractFilePath`
   etc. and 47 `\\` literals in the core. Remote paths already use `/`
   (`UnixIncludeTrailingBackslash`).
4. Configuration lives in the registry by default, but WinSCP natively supports an INI file
   (`TIniFileStorage`) too — on Linux we use INI only, in a format compatible with `WinSCP.ini`
   (sessions can be imported from Windows by copying the file).

## 3. Architecture decisions

### 3.1 Port, not rewrite

Core + PuTTY + neon + libs3 + the FileZilla engine are ~180k lines of mature, battle-tested
code handling hundreds of server quirks. A rewrite (e.g. in Rust/Qt with WinSCP as the
specification) would lose that knowledge and take years. Wine is out — it does not meet the
Wayland and D&D requirements. **We port all non-visual code and write the GUI from scratch
in Qt.**

### 3.2 C++Builder dialect: RTL shim + reproducible source conversion

- **Compiler: Clang** (23.1 is installed) with `-fdeclspec`. Clang supports
  `__declspec(property(get=…, put=…))`, including indexed ones (`T Items[]`), so **property
  use sites (thousands of them) stay unchanged** — only declarations are converted. GCC does
  not support this, so the project is deliberately Clang-only.
- **`source/rtl/`** — a new library implementing the subset of the RTL/VCL used by the core,
  in standard C++20: `UnicodeString` (1-based indexing, Delphi API), `AnsiString`/
  `RawByteString`/`UTF8String` (code pages via iconv), `TObject`/`TPersistent`, `TList`/
  `TObjectList`, `TStrings`/`TStringList` (with `CommaText`, `Names/Values`, sorting), streams
  (`THandleStream` on an fd), `TDateTime` + `DateUtils`/`FormatDateTime`, `Format` with Delphi
  semantics (`%0:s` indexes, `*`, widths), the `Exception` hierarchy, `TMemIniFile`,
  `TCriticalSection` (recursive), events (eventfd/condvar), `TSearchRec`/`FindFirst`,
  `TPath`/`TFile` (std::filesystem), Base64, a small `IXMLNode` on top of expat. Prior art:
  Far-NetBox (a port of the WinSCP core to MSVC) — inspiration only, not a copy (it is still
  Windows-only).
- **`UnicodeString` is based on `wchar_t`** (UTF-32 on Linux). This keeps `L"…"` literals and
  `wchar_t` APIs in the core working unchanged. UTF-8 only at the boundaries (file system,
  network). File names that are not valid UTF-8 are mapped reversibly (like Python's
  `surrogateescape`).
- **`tools/bcb2clang/`** — a script (Python, tokenizer with bracket matching) performing a
  deterministic conversion:
  - `__property T X = { read = FX, write = SetX };` → `__declspec(property(...)) T X;`
    with generated inline accessors that forward to the declared ones (Clang checks accessor
    access at use sites, and C++Builder code has public properties with private accessors),
  - `typedef void __fastcall (__closure *TEvt)(Args)` → `typedef TMethodPtr<void(Args)> TEvt;`
    (an object+method delegate, comparable like Delphi's `TMethod`),
  - `try { A } __finally { B }` → a block with a scope guard (B runs on exit, including on
    exception; the guard does not lose an exception thrown from B during unwinding),
  - assignments of methods as event handlers (`OnX = Method;`) — fixed in a loop driven by
    Clang diagnostics (the compiler reports the exact location, the script applies the fix),
  - `__fastcall`, `__published`, `PACKAGE`, `DYNAMIC` → empty macros; `__int64` → `long long`.
- **Upstream workflow ("vendor branch"):** the `upstream` remote holds pristine WinSCP; the
  `upstream-converted` branch holds the output of `bcb2clang` applied to upstream. For a new
  WinSCP release: fetch upstream, run the conversion, commit to `upstream-converted`, merge
  into `master`. Conflicts only appear where we changed code by hand — not in the thousands of
  mechanically converted lines. Therefore: manual changes in `core/` are kept minimal, all new
  logic goes into new directories (`rtl/`, `linux/`, `qt/`), and the old Windows directories
  stay untouched as a reference (zero conflicts on merges). See `docs/port/UPSTREAM.md`.

### 3.3 Platform layer (`source/linux/`)

- Local file operations in `Terminal.cpp`/`FileBuffer.cpp` → POSIX (`openat`, `fstatat`,
  `utimensat`, `rename`, optionally xattrs). Windows attributes (`faHidden` etc.) are mapped
  (hidden = leading dot). The local path delimiter is `/` in the shim (`IncludeTrailingBackslash`
  behaves like `IncludeTrailingPathDelimiter`); the 47 `\\` literals are audited by hand.
- The local file system is **case-sensitive** — audit `SameText` comparisons of local paths
  (especially synchronization and directory comparison).
- Local file name sanitization (`\/:*?"<>|` → `%XX`) changes meaning: on Linux only `/` and NUL
  are invalid, but **path traversal protection must stay** (see the latest upstream commit,
  issue 2477). A dedicated security review of this area.
- Threads/events: `TSimpleThread` → `std::thread`, events → `eventfd`, `WaitForMultipleObjects`
  on sockets → `poll()`.
- PuTTY: add the `unix/` files from upstream PuTTY 0.85 (networking, `uxsel`, agent via
  `SSH_AUTH_SOCK`, GSSAPI via libgssapi_krb5, local proxy via fork/exec, noise).
  `SecureShell.cpp` switches from `WSAEventSelect` to the fd set from `uxsel` + `poll()`.
- FileZilla: `CAsyncSocketEx` + hidden message window → a per-thread `poll()` reactor with a
  message queue (`PostMessage` → queue + `eventfd`). A mini `CString`/`CTime`/`CFile` shim on
  top of `rtl/`.
- Configuration: `$XDG_CONFIG_HOME/linscp/linscp.ini` (`WinSCP.ini` format). Passwords as in
  WinSCP (optional master password); later optionally Secret Service (KWallet/libsecret) via
  QtKeychain.
- Resource strings (`TextsCore*.rc`, `TextsWin*.rc`, `TextsFileZilla.rc`) → C++ tables
  generated at build time for `LoadStr(id)`.

### 3.4 GUI: Qt 6 Widgets

- **Why Qt 6 Widgets:** the core is C++ (direct linking, no FFI); WinSCP is a dense, classic
  UI (trees, column lists, toolbars, ~45 dialogs) — Widgets map 1:1 onto VCL concepts;
  Wayland is a first-class platform (fractional scaling, D&D, clipboard, xdg-activation,
  KWin server-side decorations); on Plasma the look and the file dialogs are native.
  Rejected: GTK4 (C, worse fit for a C++ core, weaker KDE integration), QML (a poor choice for
  dense file lists and a large number of dialogs), Lazarus/LCL (Pascal, not C++).
- Optional KF6 integrations behind a CMake option (KNotifications, KIO `KFilePlacesModel` for
  the "drives" in the local panel) — the application must also build and run without KDE.
- Threading model as in WinSCP: `TTerminal` operations are called synchronously from the GUI
  thread, `ProcessGUI()` → `QCoreApplication::processEvents()`; the background transfer queue
  runs on its own threads (`TTerminalQueue`) — unchanged.
- Structure: `source/qt/app` (application, `TerminalManager` equivalent, implementation of
  `Interface.h`), `models` (`RemoteDirModel` over `TRemoteFileList`, local model),
  `views` (panels, trees, queue), `dialogs` (one per `.dfm`).
- **Qt object names = component names from the `.dfm` files** — so the translations in
  `translations/*.ini` (`Copy.dfm` → `OkButton.Caption=…`) can be loaded in their existing
  format, and E2E tests can address controls by stable names.
- Icons: LinSCP's own colorful icon set (SVG, drawn from scratch in the style of WinSCP's
  icons), or the freedesktop icon theme of the desktop by a preference. WinSCP's own icons
  (`Glyphs*.dfm`, `source/resource`) are not GPL: their license (`license.txt`, part B) forbids
  reusing them in other software, so they are never built into LinSCP (M9).

### 3.5 CLI

A single `linscp-cli` binary (the `winscp.com` equivalent), based on a port of
`ConsoleRunner.cpp` and `core/Script.cpp`: the same scripting language, switches, exit codes,
XML log. This is the first fully working product and the main tool for protocol tests.

### 3.6 Naming

Product/binaries: `linscp`, `linscp-cli` (the "WinSCP" name stays only in history and
license attribution). The project was called OpenSCP until 2026-10-25 (section 9); the old
name stays only in the history.

## 4. Windows integrations → Linux/Wayland

| Windows (WinSCP) | Linux (LinSCP) |
|---|---|
| Registry | INI file in `$XDG_CONFIG_HOME/linscp/` |
| Pageant | `ssh-agent` (`SSH_AUTH_SOCK`), PuTTY and OpenSSH keys as before |
| PuTTYgen (GUI) | `puttygen` (Unix PuTTY) / `ssh-keygen`; key conversion in the core stays |
| "Open in PuTTY" | terminal emulator (`xdg-terminal-exec`/configurable) + `ssh` or `putty` |
| SSPI/Kerberos | GSSAPI (krb5) |
| Windows certificate store | system OpenSSL CA bundle |
| `ShellExecute`, "Open with" | `QDesktopServices`/`OpenURI` portal, "Open with" dialog (KDE/portal) |
| Watching an externally edited file | `QFileSystemWatcher` (inotify) |
| Keep remote directory up to date | inotify (recursive) |
| DragExt (D&D to Explorer) | see section 5 |
| Copy/paste files with Explorer | clipboard `text/uri-list` + `x-kde-cutselection` / `x-special/gnome-copied-files` |
| Taskbar: progress, jump lists | `com.canonical.Unity.LauncherEntry` (Plasma, Dash to Dock), actions in the `.desktop` file |
| Tray icon | `QSystemTrayIcon` (StatusNotifierItem) |
| Notifications after transfer | `org.freedesktop.Notifications` / KNotification |
| Single instance, passing sessions | D-Bus (`org.linscp.LinSCP`) + xdg-activation token |
| Registering `sftp://`, `scp://`… | `x-scheme-handler/*` in the `.desktop` file |
| Explorer "Send To" | none (the user's decision, 2026-10-25); `linscp /upload FILES` stays |
| "Drives" panel (DriveView) | places: `$HOME`, mount points (`QStorageInfo`), bookmarks/`KFilePlacesModel` |
| Session import (PuTTY from the registry, FileZilla, OpenSSH) | `~/.putty/sessions`, `~/.config/filezilla/sitemanager.xml`, `~/.ssh/config`, `known_hosts`, `WinSCP.ini` |
| Installer, updates, "Run as administrator" | packages (PKGBUILD, later Flatpak/AppImage); no auto-update and no elevation |
| Remembering window positions | size/maximized state only (Wayland does not let clients position windows) |
| Local file permissions (absent on Windows) | **new**: option to preserve the local `mode` on upload |

## 5. Wayland: drag & drop and clipboard

- **Into the application** (from Dolphin/Nautilus): `text/uri-list` → upload via the standard
  copy dialog/queue. Data without a file (e.g. an image from a browser) is also accepted by
  saving it to a temporary file.
- **From the application to the file manager** — the problem known from Windows (WinSCP solves
  it with a shell extension or a temporary directory). Wayland has no XDS (`XdndDirectSave` is
  X11 only). Two modes, analogous to WinSCP:
  1. **KDE**: the protocol used by Ark — MIME types `application/x-kde-ark-dndextract-service`
     and `…-path`; after the drop, Dolphin calls a D-Bus method with the target directory →
     we download straight to the target, with the copy dialog. Verified with Dolphin 26.08
     (M7). The clipboard has upstream's fake directory instead, found where it was pasted by
     KIO's `org.kde.KDirNotify.FilesAdded` signals.
  2. **Universal**: lazy `QMimeData::retrieveData()` — only when the target requests the data,
     we download to a temporary directory (with a progress window) and hand over `file://` URIs.
     Targets of Qt wait for the data a second only, so this suits fast transfers and GTK
     targets (M7).
- D&D within the application (panel ↔ panel, onto the directory tree) — standard Qt.
- D&D is tested only with real input from the compositor (section 6.4), because on Wayland a
  drag requires the serial of a real pointer event.

## 6. Testing and autonomous work

### 6.1 Safety rules (hard)

- GUI under test runs **only** in a separate headless KWin — never on the user's
  `WAYLAND_DISPLAY`.
- Every test run has an isolated `HOME`, `XDG_CONFIG_HOME`, `XDG_RUNTIME_DIR` and its own D-Bus
  session (`dbus-run-session`). Never touch the user's `~/.config`, `~/.ssh` or SSH agent.
- Local test servers bind to `127.0.0.1` only, high ports, no `sudo`. Root is available only on
  the test VM (section 6.3), which holds nothing of value.

### 6.2 Test layers

| Layer | Tools | What it checks |
|---|---|---|
| L1 unit | GoogleTest 1.18 (installed), ASan+UBSan | RTL shim semantics matching Delphi 1:1 (1-based indexes, `Format`, dates, `TStringList`), pure core logic (file masks, listing parsing, session URLs, `CopyParam`) |
| L2 protocols | `linscp-cli` + pytest + test servers | open/ls/get/put/rm/mv/mkdir/chmod, resume, synchronize, checksums, timestamps, permissions, host keys, authentication — a matrix across protocols |
| L3 GUI components | QTest, `QT_QPA_PLATFORM=offscreen` | dialog and model logic, fast and deterministic |
| L4 GUI E2E | headless KWin + harness | full scenarios in a real compositor, screenshots reviewed by Claude, D&D with Dolphin |
| Smoke | the GUI offscreen, own profile and D-Bus (`test_smoke`) | the release build (no automation hook) starts, connects and keeps running: what the debug build hides |

Sanitizers matter a lot here: typical porting bugs (`wchar_t` size, object lifetime under the
changed exception model) show up in ASan/UBSan immediately.

### 6.3 Test servers (`tools/testenv/`)

- **SSH/SFTP/SCP locally — verified, works:** an unprivileged `/usr/bin/sshd` on
  `127.0.0.1:22222`, own host keys, public key authentication, `sftp-server`, multiple
  connections (the config path must be absolute). Limitation: without root there is no
  password/PAM authentication.
- **Test VM (SSH access, root)** — agreed on 2026-10-24, needed from M3. It hosts everything
  that cannot reasonably run locally without root: OpenSSH with passwords/keyboard-interactive/
  password change/banners/chroot, vsftpd + proftpd + pure-ftpd (different listing formats —
  the core of the FTP engine), explicit/implicit FTPS, Apache `mod_dav` and nginx (WebDAV),
  MinIO (S3), containers if needed. Servers are configured only by an idempotent script in
  the repository (`tools/testenv/vm/`), so the VM can be rebuilt from scratch; L2 tests take
  the VM address from an environment variable and are skipped when the VM is unreachable.
  Extra value: a real network (latency, large files, dropped connections).
- L1/L3/L4 tests and quick SFTP iteration stay local (local sshd as above).

### 6.4 Wayland GUI harness (`tools/wlharness/`) — verified, works

Verified on 2026-10-24: `kwin_wayland --virtual` inside `dbus-run-session` with an isolated
environment starts without access to the user's session, Qt applications run in it, and
`spectacle -b -n -f` takes a screenshot of the virtual output (KWin draws server-side
decorations — the real behaviour of the user's WM is visible). Technical note: the socket path
must fit in 108 bytes, so `XDG_RUNTIME_DIR` must be a short path.

Harness components:

1. Session start/stop of the headless KWin (resolution, scale e.g. 1.25 for HiDPI tests).
2. Screenshots (spectacle); Claude reviews the PNG against expectations.
3. A small C client of the `org_kde_kwin_fake_input` protocol (the XML ships in
   `plasma-wayland-protocols`, `wayland-scanner` is installed) — real mouse/keyboard events from
   the compositor, needed for D&D and clipboard between applications.
4. KWin scripts over D-Bus: deterministic window placement (e.g. LinSCP on the left, Dolphin
   on the right) and reading their geometry, because on Wayland a client does not know its
   position on screen.
5. **Automation hook in the application** (test builds only, CMake option): a Unix socket with
   a JSON API — widget tree, click/trigger action by `objectName`, type text, read state (e.g.
   the queue), "wait until idle", grab a single window. Additionally AT-SPI
   (`gi.repository.Atspi` is installed) as an independent accessibility check.

Example D&D test: the harness starts KWin + LinSCP + Dolphin (isolated HOME, local sshd),
places the windows, the input injector drags a file from the remote panel to Dolphin, and the
test checks that the file appeared in the target directory with correct content.

### 6.5 Autonomous workflow

- Every milestone has **measurable exit criteria** (section 7) and its own tests; a milestone is
  "done" when its tests pass, not when the code compiles.
- Loop: small change → build (Ninja) → L1/L2 tests → for GUI: E2E + screenshot reviewed →
  commit with a description. Small, logical commits directly on `master`, local only — the user
  pushes. `master` must always build and pass tests.
- `CLAUDE.md` in the repository holds build/test commands and the rules from 6.1, so every new
  session starts with the same context; `docs/port/` holds decisions and the progress log.
- Things that need a user decision or `sudo` are collected and asked about in one place instead
  of blocking the work.

## 7. Milestones

| # | Scope | Exit criteria |
|---|---|---|
| M0 | Infrastructure: CMake+Ninja (Clang), `CLAUDE.md`, `tools/testenv`, `tools/wlharness`, test skeleton, upstream workflow | `cmake --build` passes; the harness starts KWin + a Qt probe application and takes a screenshot; injected input reaches the probe; the test sshd starts from a script |
| M1 | RTL shim (`source/rtl`) + tests | shim L1 tests pass under ASan/UBSan; core headers parse |
| M2 | `bcb2clang` + core compilation (UI callbacks as stubs), `.rc` string generator | `liblinscp-core` links; L1 tests of core logic pass |
| M3 | PuTTY `unix/`, `SecureShell` on `poll()`, `linscp-cli` (port of `ConsoleRunner`); test VM provisioning script | L2 tests for SFTP and SCP pass locally and on the VM (including passwords, resume, synchronize, host keys, agent) |
| M4 | FTP/FTPS (FileZilla port), WebDAV (neon), S3 (libs3) | the same L2 matrix passes for every protocol against the VM servers |
| M5 | GUI skeleton: application, site manager (Login), Commander window with panels, navigation, basic operations, progress, authentication prompts | E2E: connect from the site manager, browse, upload/download; screenshots OK |
| M6 | Queue, copy dialog with options, properties/chmod, rename, find files, synchronization + checklist, keep-up-to-date, bookmarks/location profiles, custom commands, console, editor + external editing | E2E for each feature |
| M7 | Desktop integration: D&D (both modes), file clipboard, URL handlers, single instance, notifications, tray, taskbar progress, `.desktop`, terminal | E2E D&D and clipboard with Dolphin in the headless KWin |
| M8 | Preferences, Explorer interface, tabs/workspaces, session import, translations, packaging (PKGBUILD) | functionally complete per section 1 |
| M9 | Fidelity of the main window to upstream: its toolbars (with their drop-down menus, `ToolbarsLayout2`, Options > Toolbars), the panels' two rows of toolbars, path labels and path combo boxes, the "New Tab" tab, the four-part status bar, the Explorer's toolbars, the command line panel, the buttons of the site manager, a review of the other dialogs, an own icon set | each window matches upstream's layout (its `.dfm` and the screenshots of upstream's documentation) in E2E screenshots; the new controls have E2E tests |

This order gives a working, testable product (the CLI) right after M3 — later steps build the
GUI on top of a verified core.

## 8. Risks

| Risk | Mitigation |
|---|---|
| Subtle semantic differences between the shim and the Delphi RTL (indexes, `Format`, dates, sorting) | dense L1 tests based on Delphi documentation; sanitizers |
| Code assuming `sizeof(wchar_t) == 2` (UTF-16) | audit (only 2 explicit `sizeof(wchar_t)` in the core), tests with emoji/non-BMP file names |
| Time zones/DST (the core uses `TIME_ZONE_INFORMATION`) | reimplementation on `localtime_r`/`timegm`, tests under several `TZ` values |
| Local paths: case sensitivity, `\` as a legal character, path traversal | audit + security tests (names with `..`, `\`, `/` coming from the server) |
| Porting the FTP engine (asynchronous sockets driven by window messages) | `poll()` reactor with the same event semantics; tests against several FTP servers |
| Reentrancy of `processEvents()` in the synchronous model | the same model as VCL (`ProcessMessages`), hence the same constraints; E2E tests of cancelling operations |
| Conflicts when merging upstream | vendor branch + minimal manual changes in `core/` |
| Outgoing D&D depends on the file manager | two modes (KDE D-Bus + lazy download), tests with Dolphin |

## 9. Decisions log

- 2026-10-24: test servers on a user-provided test VM over SSH (needed from M3).
- 2026-10-24: commits go directly to `master`, local only; the user pushes.
- 2026-10-24: we keep pulling new upstream WinSCP releases (vendor branch workflow).
- 2026-10-24: everything in the repository is written in English.
- 2026-10-24: the project builds as C++17, the dialect of the upstream core (C++20 makes some
  of its mixed-type comparisons ambiguous).
- 2026-10-24: OS errors in the RTL and the platform layer are Windows error codes; the ANSI code
  page is UTF-8; the declaration-level part of `bcb2clang` was done in M1 (needed for the core
  headers to parse).
- 2026-10-24: PuTTY's `unix/` part, neon and libs3 were built in M2 (the core does not link
  without them); the FileZilla engine is a stub until M4.
- 2026-10-24: Delphi object semantics live in the RTL (zero-filled objects and thrown
  exceptions); C++Builder semantics that differ at the language level (inheritance access,
  `int` enumerations) are `bcb2clang` conversions, not manual edits.
- 2026-10-24: the application layer is ported from converted copies of upstream's
  `source/windows` files (`PORTED_FILES` of `convert.py`, in `source/app` and `source/cli`),
  so upstream changes of them are merged like those of the core.
- 2026-10-24: `bcb2clang` makes `long` 32-bit (Windows is LLP64), field-backed property
  reads return the field and out-of-class member definitions in source files are not
  `inline`, as C++Builder does.
- 2026-10-24: the L2 tests use unittest, as the other Python tests (pytest is not installed).
- 2026-10-24: the FileZilla engine keeps its Windows message loop on an emulation of window
  messages and `WSAAsyncSelect` in the platform layer (no changes of its event handling); it
  uses upstream's reduced MFC (`libs/mfc`). The WinSock error is the thread's last error.
- 2026-10-24: the FTP servers of the test VM are vsftpd, ProFTPD and Pure-FTPd; the S3
  server is moto (Debian has no MinIO), with authentication enforced.
- 2026-10-24: ASCII transfers convert to LF line endings locally.
- 2026-10-25: the GUI has a single session until the tabs (M8); the E2E tests drive it
  through an automation hook built only with `LINSCP_AUTOMATION` (dev preset).
- 2026-10-25: remote files go to other applications in upstream's two modes (`DDFakeFile`,
  on by default on KDE): Ark's drop protocol and a fake directory found by KIO's
  notifications, else downloads to a temporary directory on demand (see section 5).
- 2026-10-25: the harness isolates the system bus too; fake desktop services
  (notifications, tray, task manager, logind) run in the session for the tests.
- 2026-10-25: a session of the command line goes to a running instance (D-Bus,
  `org.linscp.LinSCP`) only while it has no session, until the tabs (M8).
- 2026-10-25: no Dolphin service menu ("Upload with LinSCP"); the user does not want it.
- 2026-10-25: the tabs (M8) hold remote sessions only, no local browser tabs; the sessions
  of the command line and of other instances open in new tabs.
- 2026-10-25: the GUI is translated by upstream's translations as they are: its texts are
  the English texts of upstream's forms, matched by a `QTranslator` (M8).
- 2026-10-25: the help opens upstream's online documentation; the project's pages are its
  GitHub pages; the version is the core's (upstream's).
- 2026-10-25: the package is a PKGBUILD in `packaging/arch`, built from the checkout.
- 2026-10-25: M9 makes the main window look and work as upstream's (toolbars, path labels,
  status bar); the icons are LinSCP's own (WinSCP's are not free to reuse), the path combo
  boxes are upstream's (places and the path's hierarchy) but a path can still be typed in.
- 2026-10-25: the project is renamed from OpenSCP to LinSCP (the user's decision), everywhere in
  the repository: the binaries (`linscp`, `linscp-cli`), the desktop file, icons and D-Bus names,
  the configuration (`$XDG_CONFIG_HOME/linscp/linscp.ini`, no migration: nothing was released),
  the code (`LinSCP:` marks the edits of converted upstream code), the build options and
  environment variables (`LINSCP_*`) and the servers of the test VM (`provision.sh` removes the
  former ones). The repository's directory and remote are the user's to rename.
- Deferred unless decided otherwise: .NET assembly, Flatpak/AppImage packaging.
