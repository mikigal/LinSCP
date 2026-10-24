/*
 * unix/platform.h: Unix-specific inter-module stuff, for WinSCP's PuTTY
 * on Linux (OpenSCP).
 *
 * Derived from PuTTY 0.85's unix/platform.h, without the GTK front end,
 * and with WinSCP's additions from windows/platform.h: the per-session
 * socket selection (do_select), the redirection of PuTTY's registry
 * storage to WinSCP's (reg_*_winscp) and the critical section guarding
 * PuTTY's shared state.
 */

#ifndef PUTTY_UNIX_PLATFORM_H
#define PUTTY_UNIX_PLATFORM_H

#include <stdio.h>                     /* for FILENAME_MAX */
#include <stddef.h>
#include <stdint.h>                    /* C99 int types */
#include <strings.h>                   /* strcasecmp */
#include <sys/types.h>                 /* for mode_t */
#include <dlfcn.h>                     /* Dynamic library loading */

#include "defs.h"
#include "marshal.h"
#include "tree234.h"

/*
 * The Windows types and functions WinSCP's portable PuTTY code uses
 * (putty.h, under WINSCP/MPEXT). C++ code (the core) gets them from the
 * platform layer (source/linux), which declares the functions with C
 * linkage too.
 */
#ifndef __cplusplus
typedef void *HANDLE;
typedef unsigned int DWORD;
typedef struct HKEY__ *HKEY;
typedef struct CRITICAL_SECTION {
    void *Mutex;
} CRITICAL_SECTION;
void InitializeCriticalSection(CRITICAL_SECTION *CriticalSection);
void DeleteCriticalSection(CRITICAL_SECTION *CriticalSection);
void EnterCriticalSection(CRITICAL_SECTION *CriticalSection);
void LeaveCriticalSection(CRITICAL_SECTION *CriticalSection);
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* WinSCP's help contexts are names */
typedef const char *HelpCtx;
#define HELPCTX(x) #x
#define NULL_HELPCTX ((HelpCtx)NULL)

#define BUILDINFO_PLATFORM "Linux"

#define THREADLOCAL __thread

struct Filename {
    char *path;
};
FILE *f_open(const struct Filename *, char const *, bool);

#ifndef SUPERSEDE_FONTSPEC_FOR_TESTING
struct FontSpec {
    char *name;    /* may be "" to indicate no selected font at all */
};
struct FontSpec *fontspec_new(const char *name);
#endif

#define BROKEN_PIPE_ERROR_CODE EPIPE   /* used in ssh/sharing.c */

#define MULTICLICK_ONLY_EVENT 0
#define SELECTION_NUL_TERMINATED 0
#define SEL_NL { 10 }

/* Simple wraparound timer function */
unsigned long getticks(void);
#define GETTICKCOUNT getticks
#define TICKSPERSEC    1000            /* we choose to use milliseconds */
#define CURSORBLINK     450            /* no standard way to set this */

#define PLATFORM_CLIPBOARDS(X)                      \
    X(CLIP_SYSTEM, "system clipboard")              \
    /* end of list */

#define CLIPNAME_IMPLICIT "Last selected text"
#define CLIPNAME_EXPLICIT "System clipboard"
#define CLIPNAME_EXPLICIT_OBJECT "system clipboard"
#define CLIPUI_DEFAULT_AUTOCOPY true
#define CLIPUI_DEFAULT_MOUSE CLIPUI_EXPLICIT
#define CLIPUI_DEFAULT_INS CLIPUI_EXPLICIT

/*
 * Code pages: the numbers of Windows (WinSCP passes them through), the
 * default one (0, as Windows' CP_ACP) is UTF-8. See unix/unicode.c.
 */
#define DEFAULT_CODEPAGE 0
#ifndef CP_UTF8
#define CP_UTF8 65001
#endif
#define CP_437 437                     /* used for test suites */
#define CP_ISO8859_1 0x10001           /* used for test suites */

#define strnicmp strncasecmp
#define stricmp strcasecmp

/* utils */
void cloexec(int);
void noncloexec(int);
bool nonblock(int);
bool no_nonblock(int);
char *make_dir_and_check_ours(const char *dirname);
char *make_dir_path(const char *path, mode_t mode);

/* BSD-semantics version of signal(), and another helpful function */
void (*putty_signal(int sig, void (*func)(int)))(int);
void block_signal(int sig, bool block_it);

/*
 * Sockets (network.c). WinSCP drives the sockets of each session itself:
 * network.c tells the session, through do_select, in which events (rwx:
 * SELECT_R, SELECT_W, SELECT_X; 0 to stop) on a descriptor it is interested,
 * and the session reports them back with select_result.
 */
enum { SELECT_R = 1, SELECT_W = 2, SELECT_X = 4 };
void do_select(Plug *plug, int fd, int rwx); // WINSCP
void select_result(int fd, int event);
#define sk_getxdmdata(socket, lenp) (NULL)
int sk_net_get_fd(Socket *sock);
SockAddr *unix_sock_addr(const char *path);
Socket *new_unix_listener(SockAddr *listenaddr, Plug *plug);

/* network.c: network error reporting helper taking an OS error code */
void plug_closing_errno(Plug *plug, int error);

SeatPromptResult make_spr_sw_abort_errno(const char *prefix, int errno_value);

/*
 * peerinfo.c, wrapping getsockopt(SO_PEERCRED).
 */
bool so_peercred(int fd, int *pid, int *uid, int *gid);

/*
 * fd-socket.c: sockets over pipes, for local proxy commands.
 */
Socket *make_fd_socket(int infd, int outfd, int inerrfd,
                       SockAddr *addr, int port, Plug *plug);
Socket *make_deferred_fd_socket(DeferredSocketOpener *opener,
                                SockAddr *addr, int port, Plug *plug);
void setup_fd_socket(Socket *s, int infd, int outfd, int inerrfd);
void fd_socket_set_psb_prefix(Socket *s, const char *prefix);

/*
 * procnet.c.
 */
bool socket_peer_is_same_user(int fd);
static inline bool sk_peer_trusted(Socket *sock)
{
    int fd = sk_net_get_fd(sock);
    return fd >= 0 && socket_peer_is_same_user(fd);
}

/* Unix-specific API for making a SubprocessWaiter */
SubprocessWaiter *subproc_waiter_from_pid(pid_t pid);

/*
 * WinSCP stores PuTTY's settings (host keys, sessions to import) in its
 * own configuration storage, through these hooks implemented by the core
 * (PuttyIntf.cpp). They replace the registry of PuTTY on Windows.
 */
#define PUTTY_REG_POS "Software\\SimonTatham\\PuTTY"
#define PUTTY_REG_PARENT "Software\\SimonTatham"
#define PUTTY_REG_PARENT_CHILD "PuTTY"
#define PUTTY_REG_GPARENT "Software"
#define PUTTY_REG_GPARENT_CHILD "SimonTatham"
int reg_override_winscp(void);
HKEY open_regkey_fn_winscp(bool create, bool write, HKEY base, const char *path, ...);
void close_regkey_winscp(HKEY key);
void putty_registry_pass(bool enable);
bool get_reg_dword_winscp(HKEY key, const char *name, DWORD *out);
bool put_reg_dword_winscp(HKEY key, const char *name, DWORD value);
char *get_reg_sz_winscp(HKEY key, const char *name);
bool put_reg_sz_winscp(HKEY key, const char *name, const char *str);

/* WinSCP's debug tracing */
void ptrace(const char *msg);

/* Cleanup functions WinSCP calls on Windows (no-ops here) */
void win_misc_cleanup(void);
void win_secur_cleanup(void);

#endif /* PUTTY_UNIX_PLATFORM_H */
