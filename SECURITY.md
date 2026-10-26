# Security policy

## Reporting a vulnerability

**Please do not report security problems in public issues.**

Report them privately through GitHub's
[private vulnerability reporting](https://github.com/mikigal/LinSCP/security/advisories/new)
(*Security → Report a vulnerability*). Include:

- the affected component and commit (`git rev-parse HEAD`),
- the protocol and the server software, if they matter,
- steps to reproduce, or a proof of concept,
- the impact as you see it.

LinSCP is a small volunteer project. Reports are handled on a best-effort basis: you will get
a reply once the report has been read, and fixes for confirmed problems take priority over
other work. Credit is given in the advisory unless you prefer otherwise.

## Supported versions

LinSCP has no releases yet. Only the latest `master` gets security fixes.

## Scope

LinSCP combines new code with upstream code. Problems can be reported here in any case, but
it helps to know where they will end up:

| Area | Code | Where it is fixed |
|---|---|---|
| The port | `source/rtl`, `source/linux`, `source/app`, `source/cli`, `source/qt`, `tools`, edits of upstream code marked `LinSCP:` | Here |
| WinSCP's logic | `source/core`, the ported files of `source/windows`, `libs/neon`, `libs/libs3` | Upstream ([WinSCP](https://winscp.net/)), then merged here. If WinSCP on Windows is affected too, please also tell its author. |
| SSH | `source/putty` | Upstream ([PuTTY](https://www.chiark.greenend.org.uk/~sgtatham/putty/) and WinSCP), then merged here |
| System libraries | OpenSSL, expat, Qt, glibc | Their projects and your distribution |

Upstream fixes reach LinSCP through the vendor-branch merge described in
[`docs/port/UPSTREAM.md`](docs/port/UPSTREAM.md).

## Security model

> [!IMPORTANT]
> The protocol and cryptographic code is WinSCP's own: the WinSCP core, PuTTY, the FileZilla
> FTP engine, neon and libs3, compiled for Linux. LinSCP implements no protocol or
> cryptography of its own. Its edits to that code are limited to what Linux requires, marked
> with `LinSCP:` comments and listed in [`docs/port/PROGRESS.md`](docs/port/PROGRESS.md). TLS
> uses the system's OpenSSL 3 instead of WinSCP's bundled copy.

### Remote servers are untrusted

Everything a server sends is untrusted: file names, listings, sizes, timestamps, banners and
error messages. WinSCP's protections are kept. Remote file names cannot escape the target
directory of a download: `/` is encoded (or the file is refused when encoding is off), and
`.` and `..` from SCP are refused. These rules were adapted to Linux file names without
weakening them.

- **SSH host keys** are shown for confirmation on the first connection and cached in the
  configuration. A changed key is reported. SSH host certificate authorities can be trusted
  in *Preferences → Security*.
- **TLS certificates** (FTPS, WebDAVS, S3) are verified against the system CA store through
  OpenSSL. If verification fails, LinSCP asks before it connects, as WinSCP does, and remembers
  the certificate only if you accept it.

### Stored credentials

LinSCP stores credentials like WinSCP does, in `~/.config/linscp/linscp.ini`:

- **Without a master password**, saved passwords are only obfuscated. Anyone who can read the
  file can recover them.
- **With a master password** (*Preferences → Security*), saved passwords are encrypted with
  AES-256 (CTR mode, HMAC-SHA1, a PBKDF2-derived key).
- Best: do not save passwords or key passphrases. Use public keys with `ssh-agent`, which
  LinSCP reaches through `SSH_AUTH_SOCK`.

See WinSCP's notes on [security of credentials](https://winscp.net/eng/docs/security_credentials),
which apply to LinSCP as well.

### Other users of the computer

WinSCP keeps its configuration and its temporary files in the folders of the Windows user.
On Linux, `/tmp` is shared by all users, so LinSCP makes these files private itself:

- The directory `~/.config/linscp` is accessible to you only (0700), and `linscp.ini` is
  readable and writable by you only (0600). A configuration file given with `/ini=` is made
  private once it is saved.
- Files downloaded for editing, drag and drop or custom commands go to a directory of your
  own, `$TMPDIR/linscp-UID` (0700). LinSCP uses an existing directory of that name only if
  it is yours and not a link. Each operation gets a new directory there, created atomically
  with the 0700 mode, also when you choose a shared temporary directory in the preferences.
- When LinSCP cleans up old temporary directories, it deletes only directories that belong
  to you, and it deletes links instead of following them.

### Local processes

LinSCP treats other processes of the same user as trusted. They can already read your files
and control your session. The D-Bus services that LinSCP registers on the **session bus** are
reachable only by your own processes:

- `org.linscp.LinSCP` passes a session or URL to a running instance (single instance). It
  opens the session as `linscp URL` would, including the host key prompt.
- For drags in KDE mode, LinSCP exposes Ark's `org.kde.ark.DndExtract` interface, which
  Dolphin calls with the drop directory.

### No network access of its own

LinSCP has no update checks, no usage statistics and no crash reporting. WinSCP's code for
them (in `source/windows`) was not ported. LinSCP connects only to the servers and proxies you
configure. The Help buttons open WinSCP's online documentation in your browser only when you
click them.

### Automation hook

The end-to-end tests drive the GUI through a JSON API on a Unix socket
(`source/qt/app/Automation.cpp`). Anyone who can connect to that socket controls the
application: they can open saved sites, browse servers and transfer files. Therefore:

- The hook is compiled only with the CMake option `LINSCP_AUTOMATION`, which is **off** by
  default, in the `release` preset and in the package. Only the `dev` preset turns it on.
- Even in such a build, the socket exists only when the variable `LINSCP_AUTOMATION_SOCKET`
  is set. It is created accessible to the owner only, and the variable is removed from the
  environment of programs that LinSCP starts.

**Never distribute a build with `LINSCP_AUTOMATION` enabled.**

### Building securely

The CMake presets add no hardening flags. Packagers should build with their distribution's
standard flags, as `makepkg` does for the PKGBUILD. For example:

```bash
CFLAGS="-O2 -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection" \
CXXFLAGS="$CFLAGS" \
LDFLAGS="-Wl,-z,relro,-z,now" \
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DLINSCP_BUILD_TESTS=OFF -DLINSCP_BUILD_HARNESS=OFF -DLINSCP_RELEASE_BUILD=ON
```

## Development practices

- The test suite runs under AddressSanitizer and UndefinedBehaviorSanitizer. Typical porting
  bugs, such as the size of `wchar_t`, object lifetimes and 64-bit `long`, show up there
  first.
- Tests never touch the developer's `~/.ssh`, SSH agent, `~/.config` or desktop. GUI tests run
  in a private headless KWin with its own D-Bus session **and** system bus, so a test cannot
  suspend or power off the machine. Test servers listen only on `127.0.0.1`, or run on a
  disposable test VM.
- Test credentials in [`tools/testenv/vm/testvm.env`](tools/testenv/vm/testvm.env) belong to
  the disposable test VM only. The keys that reach the VM live in `tools/testenv/vm/state/`,
  which git ignores.
