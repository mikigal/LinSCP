# Progress log

Newest first. Milestones are defined in `PLAN.md`, section 7.

## M6 — the features of the GUI (in progress)

Queue, copy dialog with options, properties/chmod, rename, find files, synchronization +
checklist, keep-up-to-date, bookmarks/location profiles, custom commands, console, editor +
external editing.

Done so far (E2E tests in `test_gui_ssh`, `test_gui_vm`; L3 tests in `tests/unit/qt`):

- The properties dialog (`TPropertiesDialog`, F9) with the permissions editor
  (`TRightsFrame`): size calculation, owner and group (by ID where the protocol needs it),
  permissions, recursion, checksums (with the separate shell session where the protocol has
  none), S3 tags and ACL. Local files get the same dialog (upstream shows the shell's), their
  permissions are changed as `chmod -R` would (`app/LocalFiles.cpp`).
- The copy dialog with the transfer settings (`TCopyParamsFrame`, the custom settings and
  preset dialogs, the presets menu, the file mask editor), move (F6), the duplication
  (Shift+F5: directly, via the separate shell session or a local temporary copy) and move
  (Shift+F6) of remote files, the transfer settings toolbar with presets selected by their
  rules. Histories of combo boxes are kept in the configuration (`History\<name>`).
- The transfer queue (`TQueueController`, `views/ScpCommanderQueue.cpp`): background
  transfers from the copy dialog, the queue panel with its toolbar (process queue, query,
  error, prompt, execute now, suspend, resume, cancel, move up/down, delete completed, once
  empty: stay idle or disconnect), pending user actions popped up automatically, the panel
  shown always, when not empty or never. The session upkeep runs on a timer
  (`TTerminalManager::Idle`: keepalives of the session, the queue, its events), which M5
  lacked.
- Find files (`TFileFindDialog`, Alt+F7, modeless): the search by a file mask in a
  remote directory, focusing, downloading and deleting the found files.
- Synchronization (`TFullSynchronizeDialog`, Ctrl+S; `views/ScpCommanderSynchronize.cpp`):
  directions, modes (files, mirror, timestamps), criteria, the comparison progress, the
  checklist (`TSynchronizeChecklistDialog`: check, reverse, calculate sizes, synchronize
  directly or in the queue) and the summary.
- Keeping the remote directory up to date (`TSynchronizeDialog`, Ctrl+U): the local
  directory is watched with inotify (`app/DiscMon.cpp`, through `TSynchronizeController`),
  its changes are uploaded, deletions reflected, new subdirectories watched; the log of
  the changes, the full synchronization first (asked by default), errors only logged when
  continuing on errors. The requests of threads to the main thread (`TThread::Synchronize`)
  are run from the Qt event loop (`WakeMainThread`).
- Bookmarks and location profiles: the open directory dialog (`TOpenDirectoryDialog`,
  Ctrl+O; Ctrl+B adds the current directory) with the bookmarks of the site and the shared
  ones, the location profiles (`TLocationProfilesDialog`: both directories at once, in
  folders, renamed in place, moved by drag and drop or to a folder), switching between the
  two, the bookmarks in the drop down menu of the panels' button, the shortcuts of the
  shared bookmarks (Ctrl+0..9, Shift+Ctrl+Alt+A..Z; `app/ShortCuts.cpp` converts the VCL's
  `TShortCut`). Stored as upstream (`Bookmarks`, `Interface\UseLocationProfiles`,
  `Interface\UseSharedBookmarks`).

Decisions:

- Popup menus and shortcuts need the real input of the compositor: the automation hook
  reports the geometry of widgets and `OpenScpGui.click_real` clicks them with `wlh-input`;
  items of open popup menus are activated by the hook. `wlharness` lists dialogs among the
  windows.
- Ctrl+Q stays the Quit of the Linux desktops; upstream's "go to queue" has no shortcut.
- Notifications of the queue (empty, pending user action) and shutting down or suspending
  the computer once it is empty are left for M7. With a single session (no tabs), a session
  closed once the queue got empty is freed, so it is not offered to reconnect.
- On Linux, the transfer mode is disabled for Unix servers (the same line endings), as
  upstream's attributes say; "Clear archive attribute" is hidden.
- Not ported yet: "Start in new window" of the synchronization dialogs (needs the command
  line switches of the GUI), minimizing to the system tray while keeping up to date (M7),
  moving, exploring and custom commands of the checklist.
- A dialog that grows (the log of keeping up to date) corrects a size the compositor sets
  from before (KWin re-sends the old size when it activates the window).

Manual edits in upstream directories since M5:

- `core/Terminal.cpp`: `TCalculateSizeParams` zeroes itself with `memset`, which corrupted
  its `UnicodeString` member (a pointer in C++Builder, a `std::wstring` here); the member is
  constructed again (parallel downloads of the queue crashed).

## M5 — GUI skeleton (done, 2026-10-25)

Exit criteria met: the E2E tests connect from the site manager, browse, upload, download
and operate on files, in the headless KWin, against the local sshd (SFTP with a key file
and with an SSH agent) and against the test VM (banner, password and keyboard-interactive
prompts); the screenshots were reviewed. All tests pass under ASan/UBSan (260 with the
VM); the release build runs.

- `source/qt` (Qt 6 Widgets): the `openscp_qt` library and `openscp`. The application
  object shows the exceptions that escape event handlers (as the VCL does); `ProcessGUI`
  processes the Qt events. Object names are the component names of upstream's `.dfm` files.
  - `app/`: `TTerminalManager` (one session yet: connecting in a `TTerminalThread`, the
    handlers of the terminal's events, reconnecting after a fatal error, the master
    password prompt), the Qt parts of `core/Interface.h`, helpers.
  - `dialogs/`: the message dialog (`TMessageForm`: answers with aliases and submit
    handlers, never ask again, timer, timeout, Ctrl+C), simple dialogs and the input
    dialog, the site manager (`TLoginDialog`: the tree of sites, folders and workspaces,
    the basic settings, save/edit/delete/rename in place/clone/new folder/defaults/paste
    URL), the save site dialog, the authentication window (`TAuthenticateForm`: log,
    prompts, banner), the progress window (`TProgressForm`), the copy and create
    directory dialogs (without the transfer settings and attributes yet).
  - `models/`, `views/`: the file panels (`TDirModel`, `TDirPanel`): local directories
    read with POSIX calls (names keep their bytes), a copy of the terminal's remote
    directory; directories first, the parent directory, history, path box, focus kept
    over reloads; the Commander window (`TScpCommanderForm`) with menus, toolbars, the
    current panel and the commands: copy (upload/download), rename in place, delete
    (local files to the trash), create directory, show hidden files.
- The automation hook (`OPENSCP_AUTOMATION`, on in the `dev` preset; `app/Automation.h`):
  a Unix socket with a JSON API to describe and drive the widgets by object names.
  `tools/testenv/openscp_gui.py` runs the GUI in a harness session and drives it;
  `tests/integration/guitests.py` is the base of the L4 tests (`test_gui`, `test_gui_ssh`,
  `test_gui_vm`); `OPENSCP_GUI_KEEP` keeps the sessions with their screenshots. L3 tests
  of components on the offscreen platform: `tests/unit/qt` (`openscp_qt_tests`).
- `TAppConfiguration` stores the settings of the GUI under the keys of `WinSCP.ini`
  (`Interface`: `ConfirmDeleting`, `ShowHiddenFiles`, `LastStoredSession`,
  `OpenedStoredSessionFolders`).

Decisions:

- **A single session** until the tabs (M8); a failed connection opens the site manager
  again, a lost one offers to reconnect, and the session is closed otherwise.
- **Qt keywords off** (`QT_NO_KEYWORDS`): `Q_SIGNALS`, `Q_SLOTS`, `Q_EMIT`.
- **Commands of the automation that act reply before they act**, so that a modal window
  they open serves the next requests. Key events of the hook are synthetic (no shortcuts);
  shortcuts and drag and drop need the real input of `wlh-input`.
- Delphi-style objects in new code are allocated on the heap (zero-filled), also the
  exceptions the GUI creates.
- LeakSanitizer ignores what Qt's Wayland plugin leaves at exit (`libwayland-client`).

Manual edits in upstream directories since M4:

- `core/Queue.cpp`: `TTerminalThread` read `FPendingIdle` uninitialized (the thread is not
  a Delphi object, its memory is not zero-filled).
- `core/SecureShell.cpp`: the error of PuTTY's `backend_init` is freed.

Findings for later milestones:

- The XDG portal sometimes does not know OpenSCP's app ID when the application starts
  right after the harness session (a race in the portal; matters for D&D in M7).
- Core strings mention WinSCP (e.g. the host key prompt); a branding pass with M8.
- The advanced site settings, the transfer settings of the copy dialog, the attributes of
  new directories, the editor/open actions on double click and grouped buttons of the
  message dialog (upstream's drop-down menus) are M6/M8.
- Upstream remembers the last local directory of the panels and their columns; not yet.

## M4 — FTP/FTPS, WebDAV, S3 (done, 2026-10-24)

Exit criteria met: the L2 matrix of the protocols passes against the servers of the test VM
(200 tests, under ASan/UBSan and with the release build): SFTP and SCP; FTP with vsftpd
(LIST), ProFTPD and Pure-FTPd (MLSD), plain, explicit and implicit TLS, passive and active;
WebDAV with Apache and nginx over HTTP and HTTPS; S3 with moto over HTTP and HTTPS.

- **The FileZilla engine** (`source/filezilla`) builds with the subset of MFC that upstream
  already reduced to `CString`, `CFile` and `CTime` on the VCL (`libs/mfc`); no MFC shim of
  our own was needed. The engine's thread runs its Windows message loop on the platform
  layer.
- `source/linux`, `winapi/Messages.h`: message queues of threads (an eventfd each), message-only
  windows, `RegisterWindowMessage`, `Post`/`Get`/`PeekMessage`, timers (`WM_TIMER` generated
  when due, one at a time per timer). `WSAAsyncSelect` with the Windows semantics: an event is
  posted once until the call that re-enables it (`recv`, `send` that would block or sent
  partially, `accept`); `FD_CONNECT` when a connect completes; `FD_CLOSE` once the data before
  the end of the connection was read (`POLLRDHUP`; the socket is then not polled for reading,
  `recv` posts what follows). The sockets are polled by the thread of their window together
  with its queue and timers. `WSAAsyncGetHostByName` resolves before it returns; only the reply
  is posted.
- WinSock: the socket functions with Windows signatures are overloads of the POSIX ones for
  the `SOCKET` type (`recv(SOCKET, char *, int, int)`, ...); the error of a socket call is the
  thread's last error (`WSAGetLastError` is `GetLastError`, as on Windows). WinSock error codes
  are in `rtl/WinError.h`, mapped to errno values and back. Microsoft C runtime functions in
  `winapi/Crt.h`.
- `tools/testenv/vm/provision.sh`: vsftpd (port 21, implicit TLS on 990), ProFTPD (2121) and
  Pure-FTPd (2122) — Debian's FTP servers conflict with each other, so the last two are
  unpacked from their packages to `/opt/openscp`; Apache and nginx WebDAV; moto as the S3
  server with authentication enforced and nginx as its TLS proxy; a self-signed certificate
  kept across runs. moto verifies signatures against the URL with the query decoded, so it
  runs with the URL as the client sent it (`s3-server.py`).
- L2 tests: `tests/integration/vmtests.py` (the VM, a remote directory or S3 bucket per test,
  the remote files as seen on the VM, the matrix every protocol passes), `test_cli_vm`,
  `test_cli_vm_ftp`, `test_cli_vm_webdav`, `test_cli_vm_s3` (label `vm`, one at a time).

Decisions:

- **ASCII transfers use Linux line endings**: the core's local line ending is LF (it was
  CRLF, so ASCII downloads over SFTP and SCP wrote CRLF). The FileZilla engine leaves ASCII
  conversion to the server, as CRLF is the Windows line ending; on Linux it converts CRLF to
  LF in downloads and LF to CRLF in uploads, as FileZilla 3 does. A CR before a LF of a local
  file is content (SFTP, SCP), or a line end already (FTP, where it is not doubled).
- **`data()` of an empty RTL string is NULL**, as in C++Builder; the core passes optional
  values to libs3 that way (the S3 session token).
- libs3's zero-length variable length arrays are not reported by UBSan (`vla-bound`).

Manual edits in upstream directories since M3:

- `filezilla/FtpControlSocket.cpp`: local paths delimited by `/` (directories created for a
  download, the overwrite prompt), no replacement of `:` in local file names, `RAND_MAX`
  overflows (`RAND_MAX` is `0x7FFF` on Windows), `std::transform` with `::towlower`.
- `filezilla/AsyncSslSocketLayer.cpp`: after a partial send, the rest was buffered from the
  wrong offset and later data was sent before it; a BIO read could overflow its stack buffer;
  the TLS session reused for data connections leaked (upstream bugs; Windows non-blocking
  sockets rarely send partially).
- `filezilla/TransferSocket.cpp`, `.h`: ASCII line endings; `RAND_MAX` overflow.
- `filezilla/AsyncProxySocketLayer.cpp`: `in_addr` has no `S_un` on Linux.
- `libs/libs3/src/request.c`: neon's zero-length body reads (a rewind) are not passed to the
  core's callbacks (they copied zero bytes to a NULL buffer).
- `core/Configuration.cpp`: the local line ending.

Findings for later milestones:

- Proxies (the engine's proxy layer, neon's, PuTTY's) are not tested yet; the VM could run a
  SOCKS and an HTTP proxy.
- With the address family on auto, an FTP host name that does not resolve fails with
  "Connection failed." only (upstream does the same); IPv4 only reports it.
- `CTime` of `libs/mfc` uses `localtime`, which is not thread-safe (as upstream).
- The S3 server keeps objects in memory; restarting it (provisioning) creates a new key.

## M3 — SSH and the CLI (done, 2026-10-24)

Exit criteria met: `openscp-cli` works for SFTP and SCP; the L2 tests pass against the local
sshd (under ASan/UBSan and with the release build) and against the test VM (passwords,
keyboard-interactive, password change, resume, synchronize; host keys and the agent locally).

- **Ported application files**: `tools/bcb2clang/convert.py` writes converted copies of the
  files of upstream's `source/windows` listed in `PORTED_FILES` to `source/app` and
  `source/cli`; they are edited on `master`, so upstream changes arrive through the regular
  merges (`UPSTREAM.md`). Ported so far: `ConsoleRunner`, `GUIConfiguration`, `ProgParams`,
  `SynchronizeController`.
- `source/app` (`openscp::app`), the application layer shared by the front ends:
  `TGUIConfiguration` (resource modules of translations dropped: the internal locale only),
  `TAppConfiguration` (the INI file `$XDG_CONFIG_HOME/openscp/openscp.ini` unless `/ini=`,
  the master password of `TWinConfiguration`), `TProgramParams` (the process's arguments as
  they are), `TDiscMonitor` on inotify (a thread, changes reported with
  `TThread::Synchronize`), the common parts of `core/Interface.h`, helpers of upstream's GUI
  code (`ApplyTabs`, `FormatMainFormCaption`, ...), string tables of `TextsWin1.rc`.
- `source/cli` (`openscp-cli`, the counterpart of `winscp.com`): the ported `ConsoleRunner`
  with `TUnixConsole` in place of the Windows consoles. It works as `winscp.com`: interactive
  on a terminal (line input, echo off for passwords, choices by a single key, Esc cancels),
  redirected input read and echoed line by line, redirected output not rewritten (progress
  lines are committed by the next line), `/stdout` and `/stdin` streams with messages on
  stderr. SIGINT is blocked in all threads and waited for by a thread of its own (no system
  call gets `EINTR`); it wakes up the console and `CheckSynchronize`. SIGPIPE is ignored.
  Not ported: `/dumpcallstack`, `/comregistration`, console titles.
- RTL: `TComponent` (ownership), `TThread::Synchronize`/`Queue` with `CheckSynchronize`,
  `WrapText`/`TSysCharSet`; `sprintf`'s `%ld` is 32-bit.
- L2 tests (`tests/integration/test_cli_ssh.py`, label `ssh`): unittest (as the other Python
  tests; pytest is not installed) with `tools/testenv/openscp_cli.py`, which runs the CLI with
  an isolated HOME, XDG directories and TMPDIR, without the user's SSH agent, and on a
  pseudo terminal for the interactive tests. They cover transfers (files, directories, masks,
  permissions, timestamps, resume, streams, Unicode names and names invalid on Windows), file
  operations, listing, synchronization, keepuptodate with Ctrl+C, host keys (fingerprints,
  the interactive prompt and the cache), public key, agent (an agent of the test's own) and
  passphrase authentication, XML and session logs, scripts and parameters, exit codes, stored
  sessions in an INI file. `/copyid` is not tested locally: the local sshd runs as the user,
  it would install the key in the user's own `~/.ssh/authorized_keys`.

- **Test VM** (Debian 13, an LXC container; its address in `OPENSCP_TEST_VM`): reached as
  root over SSH with a key of its own (`tools/testenv/vm/state/`, ignored by git), never with
  the user's `~/.ssh` or agent. `tools/testenv/vm/provision.sh` (idempotent, run with
  `tools/testenv/testvm.py provision`) sets up a second OpenSSH server on port 2222 (the one
  on port 22 stays for root, so no test can lock the provisioning out) with test users per
  authentication method (`tools/testenv/vm/testvm.env`): password only, keyboard-interactive
  (PAM), an expired password, public key and password, a chrooted SFTP user, a user for
  `/copyid`. `tests/integration/test_cli_vm.py` (label `vm`) is skipped without the VM.
  As upstream, scripting asks for no other credentials once a pre-entered password was used
  (a password change needs the password typed in), and the CLI does not show banners (only
  the GUI does).

Decisions:

- **`long` is 32-bit in the converted core** (a `bcb2clang` conversion): Windows is LLP64 and
  the core relies on it (SFTP packets read `sizeof(unsigned long)` bytes, times are
  sign-extended with `static_cast<signed long>`, the HMAC code XORs keys in `unsigned long`
  steps). The RTL's `sprintf` takes `%ld` as 32-bit, as the Windows C runtime does. Where the
  core meets other code (PuTTY's `schedule_timer`, the C library's `swscanf`), the `long`s
  are edited by hand.
- **Field-backed property reads return a reference to the field** (`bcb2clang`): C++Builder
  reads the field itself, so `Rights.Number = ...` or `IncludeFileMask.Masks = ...` modify
  the field; with a getter returning a copy the assignments were lost.
- **No `inline` on member definitions outside of classes in source files** (`bcb2clang`):
  C++Builder emits them as external functions and other files call them
  (`TTerminal::LogEvent`); Clang does not emit them with optimizations, the release build
  did not link.
- **Storage keys are delimited by backslashes** (`THierarchicalStorage`), not by the path
  delimiter, so the INI file has the sections of `WinSCP.ini` (`[Sessions\name]`) and
  `/rawconfig` takes the same keys as on Windows.
- **Local file names follow Linux**: only `/` is invalid (`LocalInvalidChars`), no encoding of
  trailing spaces and dots, no reserved names (`CON`, ...), no `\\?\` prefix for long paths
  (`ApiPath`). The path traversal protection stays (`/` is encoded, or refused when the
  replacement is off; `.` and `..` from SCP are refused).
- Known upstream leaks on PuTTY's fatal error paths (WinSCP's seat throws a C++ exception
  through PuTTY's C code) are suppressed for LeakSanitizer in the L2 tests
  (`tools/testenv/lsan-suppressions.txt`); the tests that cancel the host key prompt run
  without leak detection (a packet of the abandoned key exchange has no frame of its own).
  The leak of `TlsCipherList` and of interrupted PuTTY callbacks were fixed.
- As upstream: SCP has no streams (`/stdout`, `/stdin`) and no newer-only upload; Ctrl+C at
  the prompt ends the console with exit code 0; in keepuptodate it ends watching and the
  script goes on.

Manual edits in upstream directories since M2 (see also the list in M2):

- `core/HierarchicalStorage.cpp`: backslash-delimited keys (also `TOptionsIniFile`).
- `core/Common.cpp`: `LocalInvalidChars`, `ValidLocalFileName`, `ApiPath`, the path delimiter
  in `DoRecursiveDeleteFile`, `TlsCipherList` frees its SSL objects.
- `core/PuttyIntf.cpp` (`schedule_timer`), `core/ScpFileSystem.cpp` (`swscanf`),
  `core/SftpFileSystem.cpp` (a pointer-sized token): the 32-bit `long`.
- `putty/putty.h`: `SeatOutputType` has the underlying type `int` in C++ (WinSCP passes
  `(SeatOutputType)-1`).
- `core/SecureShell.cpp` (`FreeBackend`): frees the PuTTY callback interrupted by an exception
  and the callbacks and packets left of the destroyed backend (LeakSanitizer).

Findings for later milestones:

- The default random seed file is `$XDG_DATA_HOME/winscp.rnd`, the default log file names
  use `%TEMP%` (not set on Linux) — to settle with the preferences (M8).
- `TGUIConfiguration` has no translations (resource modules); M8 brings the `.ini`
  translations.
- The interactive console has no line editing beyond the terminal's own (no history).

## M2 — Core compilation (done, 2026-10-24)

Exit criteria met: `libopenscp-core` (`openscp::core`) links into executables, and the L1
tests of core logic (`openscp_core_tests`) pass under ASan/UBSan.

- `bcb2clang` converts statements too: `try`/`__finally` (a `::rtl::TFinally` scope guard,
  also in macro definitions), references to methods (`::rtl::MethodPtr(Object,
  &TClass::Method)`, resolved with a table of the classes of all core files and the RTL
  properties), non-public inheritance from Delphi-style classes (made public) and unscoped
  enumerations (given the underlying type `int`). `convert.py` converts a tree,
  `update-upstream.sh` records conversions on the `upstream-converted` branch (`UPSTREAM.md`).
- `tools/rc2cpp` generates C++ string tables from the `.rc` files (`TextsCore*.rc`,
  `TextsFileZilla.rc`); they register with the RTL's `LoadStr` before the core's static
  objects are constructed.
- `source/linux` (`openscp::linux`): the Windows API the core uses, with C linkage, in
  `include/winapi/` (SDK header names in `include/`): files (a `HANDLE` is an fd), kernel
  objects (events, threads, processes, waits that poll sockets), WinSock on POSIX sockets,
  time zones from tzdata, CryptoAPI on OpenSSL, an empty registry, COM stubs, XDG folders,
  version information.
- PuTTY: the portable sources of WinSCP's `Putty.cbproj` plus `source/putty/unix`, derived
  from PuTTY 0.85's `unix/` with WinSCP's changes (callback sets per session, `uxsel` per
  callback set, the agent via `SSH_AUTH_SOCK`, subprocesses waited for with pidfds, storage
  through the core's registry hooks, character sets via iconv). `SecureShell` waits on the
  session's file descriptors with `poll()`.
- neon and libs3 build from the in-tree (WinSCP-modified) sources with the system OpenSSL
  and expat (`source/libneon`, `source/libs3`).
- The FileZilla engine is MFC-based and stays for M4; `source/libfilezilla` implements
  `TFileZillaIntf` with a stub whose `Init` throws, so the core links and an FTP session
  fails to open with an error.
- `tests/unit/core`: the test implementation of `core/Interface.h` (no UI, configuration
  without storage) and tests of file masks, listing parsing, permissions, remote paths,
  session URLs, transfer settings and general helpers.

Decisions:

- **Third-party code was pulled into M2** (PuTTY's `unix/`, neon, libs3): the core is
  monolithic, even a file mask test links `TTerminal` and with it every file system.
- **Delphi object semantics in the RTL**: `TObject::operator new` and the storage of thrown
  objects (`--wrap=__cxa_allocate_exception`) are zero-filled, as Delphi objects are; the core
  leaves fields uninitialized (`TSessionData`, `ExtException`). Delphi-style objects in new
  code must be heap-allocated, as in C++Builder.
- **Dialect conversions instead of manual edits** where C++Builder semantics differ: public
  inheritance from Delphi classes (C++Builder allocates and casts them ignoring the access),
  `int` enumerations (C++Builder, like MSVC; `TGssLib(-1)` compares as negative). On Windows
  `wint_t` is `wchar_t`, so the RTL adds `wchar_t` overloads of `towupper`/`towlower`.
- The core's headers are system headers for its consumers (their upstream warnings are not
  reported in our code).
- The application name and the SSH client version string are the front end's
  (`AppNameString`, `SshVersionString` in `Interface.h`); OpenSCP uses `OpenSCP`.

Manual edits in upstream directories (keep the list current; `git diff upstream-converted
master -- source/core source/putty libs` shows them):

- `core/Common.cpp`, `HierarchicalStorage.cpp`, `Queue.cpp`, `CoreMain.cpp`,
  `SecureShell.cpp`, `Terminal.cpp`: pointer/integer casts that truncate on LP64 (module
  handles, file handles, `TStrings::Objects` flags), a functional cast of a closure type.
- `core/PuttyIntf.cpp`: does not include `windows/platform.h` (the PuTTY headers bring the
  platform's own).
- `core/SecureShell.cpp`: the event loop waits with `poll()` on the session's descriptors
  (`EventSelectLoop`).
- `putty/puttyexp.h`: `select_result` is declared by the platform header.
- `libs/neon/src/config.h`: includes `neon-linux-config.h` outside Windows.

Findings for later milestones:

- SIGPIPE must be ignored by the applications (writes to closed sockets).
- Local paths: audit `\\` literals in the core (e.g. `FileName + L"\\*"` in
  `DoRecursiveDeleteFile`), `\\?\` long path prefixes (`ApiPath`), `ValidLocalFileName`
  (Windows-invalid characters, path traversal protection) — M3, with the transfer tests.
- Unavailable on Linux: local-to-local synchronization through COM `IFileOperation`,
  SSPI/NTLM for WebDAV; moving to the trash only in the home directory's file system.
- `SessionInfo`'s memory statistics use `VirtualQuery` with pointer arithmetic from NULL,
  which UBSan may report.
- `Security.cpp` has a size assertion that fails only in `_DEBUG` builds (not defined).
- Exceptions copied with their implicit copy constructors share `ExtException`'s
  `FMoreMessages` (C++Builder never copies Delphi objects); watch for double deletes under
  ASan when exceptions are rethrown by value.
- Upstream bugs (also on Windows, left as they are): `UnixExtractFileExt` returns a wrong part
  of a path that has directories (its only caller passes a name); `ParseShortEngMonthName`
  passes `ShortMonthNames.Size()` as the highest index, so an unknown name reads one element
  past the array (inside `TFormatSettings`, unnoticed by ASan).

## M1 — RTL shim (done, 2026-10-24)

Exit criteria met: the shim's L1 tests pass under ASan/UBSan, and the core headers, converted
by `bcb2clang`, parse against the shim (`unit.bcb2clang.test_core_headers`; the only
exceptions are `PuttyIntf.h`, `NeonIntf.h` and `WebDAVFileSystem.h`, which stop at the
missing PuTTY/neon headers of M3/M4).

`source/rtl` (`openscp::rtl`): the Delphi RTL subset the core uses, in standard C++. Headers
named like C++Builder's (`System.hpp`, `SysUtils.hpp`, `Classes.hpp`, `vcl.h`, ...) in
`source/rtl/include` include the implementation headers in `source/rtl/include/rtl`.
Tests: `tests/unit/rtl` (GoogleTest).

- System: `UnicodeString`, `AnsiStringT` (`AnsiString`, `UTF8String`, `RawByteString`),
  character set conversions, `sprintf` with Windows conventions, `TObject`/`__classid`,
  `TMethodPtr` (the replacement of `__closure`), `Set`, `DynamicArray`, `StaticArray`,
  `TVarRec`/`ARRAYOFCONST`, `TDateTime`, `Variant`, resource strings, command line, `Random`,
  Windows types and error codes.
- SysUtils: exceptions, `Format` and float conversions with Delphi rounding, integer and
  string routines, file names, files, `FindFirst`, dates, `TFormatSettings` (from the locale),
  `TEncoding`. StrUtils, Math, DateUtils (with `TTimeZone`), TimeSpan, Types.
- Classes: `TPersistent`, `TList`, `TStrings`/`TStringList`, streams; Contnrs (`TObjectList`),
  SyncObjs (`TCriticalSection`, `TEvent`), IniFiles (`TMemIniFile`), Masks (`TMask`), IOUtils,
  Base64 (Soap.EncdDecd), WideStrUtils, System.JSON, a read-only XML DOM on expat
  (Xml.XMLIntf/XMLDoc), declarations of System.Win.Registry.

`tools/bcb2clang/bcb2clang.py` converts the declaration-level dialect (pulled forward from M2,
it is needed for the headers to parse): `__property` and `__closure` declarations and
backslashes in `#include` paths. Statements (`try`/`__finally`, method pointer assignments)
follow in M2. Tests: `tests/unit/bcb2clang`.

Decisions:

- **C++17** for the whole project (was C++20 in M0). The core targets C++Builder's
  Clang-based compilers; C++20's rewritten comparison candidates make mixed-type comparisons
  with implicit conversions ambiguous (e.g. `Variant == UnicodeString`), which compile fine
  in C++17. The shim headers must stay C++17.
- **Properties get generated accessors.** Clang checks the access of a `__declspec(property)`
  accessor where the property is used, while C++Builder code declares public properties with
  private getters. Every converted property therefore gets inline accessors in its own access
  section that forward to the real ones (`__property_get_X`, `__property_set_X`); getters are
  `const` and call through `rtl::Mutable(this)`, so they work whether the real getter is
  `const` or not, and they return the real getter's type, as C++Builder does. Use sites stay
  unchanged.
- The ANSI code page is UTF-8, so `AnsiString`, `UTF8String` and `RawByteString` hold UTF-8
  and convert to each other by copying bytes. Other code pages go through iconv.
- UTF-8 decoding is lossless: invalid bytes map to U+DC80..U+DCFF and back
  ("surrogateescape"), so non-UTF-8 file names survive.
- OS errors reported by the RTL are **Windows error codes** (the core compares them with
  `ERROR_FILE_NOT_FOUND`, `ERROR_NO_MORE_FILES`, ...). errno values are mapped; those without
  an equivalent are encoded reversibly as `0x20000000 | errno`. `GetLastError`/`SetLastError`
  live in the RTL (`rtl/WinError.h`), the rest of the Windows API emulation will be in
  `source/linux`.
- `sLineBreak` is LF; file names are case-sensitive; `RenameFile` does not replace an
  existing target (like `MoveFile`); `FindFirst` treats `*.*` as `*` and only `*`/`?` as
  wildcards, as on Windows; attributes follow links and add `faSymLink` (like a Windows
  directory link: `faDirectory | faSymLink`).
- Locale-independent case mapping (C.UTF-8). `AnsiCompareStr`/`AnsiCompareText` compare by
  Unicode case folding (lowercase sorts first among otherwise equal strings), not by locale
  collation.
- `TEncoding::ANSI`/`Default` are UTF-8 without BOM, `TEncoding::UTF8` writes a BOM.
- Global RTL objects (`EmptyStr`, `FormatSettings`, `TEncoding` instances, `CmdLine`) use
  `init_priority`, so they are constructed before the core's static objects.
- Sources are ASCII; non-ASCII characters in literals are `\u` escapes.

Findings for M2:

- Delphi-style classes (derived from `TObject`) dispatch virtual calls made from constructors
  and destructors to the most derived class; C++ does not. The RTL accounts for it (e.g.
  `TObjectList`, `TStringList` free owned objects in their own destructors); the core's
  `TObject`-derived classes need an audit for virtual calls in constructors/destructors.
- Some overload sets are ambiguous on Linux where they are not on Windows, because `long` is
  64-bit (e.g. integer arguments mixed with `__int64`). The RTL adds `long`/`long long`
  overloads (`IntToStr`, `TVarRec`, ...) and makes `Min`/`Max` templates.
- `Common.cpp` passes `ShortMonthNames.Size()` (12) to `IndexStr` as the index of the last
  element; with C++Builder's `StaticArray` this reads one element past the end. To fix when
  the core compiles.
- `MakeMethod` (`Common.h`) and `NeonIntf.cpp` reinterpret closures as `TMethod`; the
  `TMethodPtr` layout differs, so these need a manual change (`TMethodPtr` has a
  data + function constructor for it).
- The core relies on `Variant` rounding floats half to even when converting to integers
  (`FtpFileSystem.cpp` time difference); implemented.

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

