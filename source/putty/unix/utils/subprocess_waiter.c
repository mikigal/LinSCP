/*
 * Unix implementation of SubprocessWaiter, for WinSCP (OpenSCP).
 *
 * PuTTY's own implementation catches SIGCHLD and handles it through a
 * self-pipe in the one event loop. WinSCP runs an event loop for each
 * session instead (and in a library, the signal handling belongs to the
 * application), so a waiter selects a pidfd of its process in the callback
 * set of the session: the pidfd becomes readable when the process exits.
 * Processes started without a waiter are reaped the same way, so that they
 * do not remain zombies.
 */

#include "putty.h"
#include "tree234.h"

#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/wait.h>

struct SubprocessWaiter {
    pid_t pid;
    int pidfd;
    SubprocessWaiterCallback callback;
    void *cbctx;
    /* Only reaps the process and frees itself */
    bool reaper;
};

static tree234 *waiters_by_pidfd;

static int subproc_waiter_compare_by_pidfd(void *av, void *bv)
{
    SubprocessWaiter *a = (SubprocessWaiter *)av;
    SubprocessWaiter *b = (SubprocessWaiter *)bv;

    if (a->pidfd < b->pidfd)
        return -1;
    else if (a->pidfd > b->pidfd)
        return +1;
    return 0;
}

static int subproc_waiter_find_by_pidfd(void *av, void *bv)
{
    int a = *(int *)av;
    SubprocessWaiter *b = (SubprocessWaiter *)bv;

    if (a < b->pidfd)
        return -1;
    else if (a > b->pidfd)
        return +1;
    return 0;
}

static void subproc_waiter_unregister(SubprocessWaiter *waiter)
{
    if (waiter->pidfd >= 0) {
        uxsel_del(waiter->pidfd);
        WINSCP_PUTTY_SECTION_ENTER;
        del234(waiters_by_pidfd, waiter);
        WINSCP_PUTTY_SECTION_LEAVE;
        close(waiter->pidfd);
        waiter->pidfd = -1;
    }
}

static void pidfd_select_result(int fd, int event)
{
    WINSCP_PUTTY_SECTION_ENTER;
    SubprocessWaiter *waiter = waiters_by_pidfd ?
        find234(waiters_by_pidfd, &fd, subproc_waiter_find_by_pidfd) : NULL;
    WINSCP_PUTTY_SECTION_LEAVE;
    if (!waiter) {
        uxsel_del(fd);
        return;
    }

    int status;
    pid_t pid = waitpid(waiter->pid, &status, WNOHANG);
    if (pid == 0)
        return;                        /* not terminated yet */

    int exittype = EXITTYPE_WEIRD;
    uint32_t exitdata = 0;
    if (pid > 0 && WIFEXITED(status)) {
        exittype = EXITTYPE_NORMAL;
        exitdata = WEXITSTATUS(status);
    } else if (pid > 0 && WIFSIGNALED(status)) {
        exittype = EXITTYPE_SIGNAL;
        exitdata = WTERMSIG(status);
    }

    subproc_waiter_unregister(waiter);
    if (waiter->reaper) {
        sfree(waiter);
    } else if (waiter->callback) {
        waiter->callback(waiter->cbctx, exittype, exitdata);
    }
}

static SubprocessWaiter *subproc_waiter_new(
    struct callback_set *set, pid_t pid, bool reaper)
{
    SubprocessWaiter *waiter = snew(SubprocessWaiter);
    waiter->pid = pid;
    waiter->callback = NULL;
    waiter->cbctx = NULL;
    waiter->reaper = reaper;
    /* Close-on-exec by default */
    waiter->pidfd = (int)syscall(SYS_pidfd_open, pid, 0);
    if (waiter->pidfd >= 0) {
        WINSCP_PUTTY_SECTION_ENTER;
        if (!waiters_by_pidfd)
            waiters_by_pidfd = newtree234(subproc_waiter_compare_by_pidfd);
        add234(waiters_by_pidfd, waiter);
        WINSCP_PUTTY_SECTION_LEAVE;
        uxsel_set(set, waiter->pidfd, SELECT_R, pidfd_select_result);
    }
    return waiter;
}

SubprocessWaiter *subproc_waiter_from_pid(struct callback_set *set, pid_t pid)
{
    return subproc_waiter_new(set, pid, false);
}

void subproc_waiter_reap(struct callback_set *set, pid_t pid)
{
    SubprocessWaiter *waiter = subproc_waiter_new(set, pid, true);
    if (waiter->pidfd < 0)
        sfree(waiter);                 /* cannot watch it, leave it */
}

void subproc_waiter_set_callback(
    SubprocessWaiter *waiter, SubprocessWaiterCallback cb, void *cbctx)
{
    waiter->callback = cb;
    waiter->cbctx = cbctx;
}

void subproc_waiter_free(SubprocessWaiter *waiter)
{
    if (waiter->pidfd >= 0) {
        /* Still running: reap it when it terminates */
        waiter->reaper = true;
        waiter->callback = NULL;
        return;
    }
    sfree(waiter);
}
