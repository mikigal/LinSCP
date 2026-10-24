/*
 * uxsel.c
 *
 * This module is a sort of all-purpose interchange for file
 * descriptors. At one end it talks to network.c and pty.c and
 * anything else which might have one or more fds that need
 * select() or poll()-type things doing to them during an extended
 * program run; at the other end it talks to window.c or plink.c or
 * anything else which might have its own means of actually doing
 * those select()-type things.
 *
 * WinSCP (OpenSCP): every descriptor belongs to the callback set of a
 * session, whose event loop (TSecureShell) enumerates its descriptors and
 * reports their events; there is no front end hook (uxsel_input_add). The
 * tree is shared by the sessions' threads, so it is guarded by
 * putty_section, and created on first use.
 */

#include <assert.h>

#include "putty.h"
#include "tree234.h"

struct fd {
    int fd;
    int rwx;                           /* 4=except 2=write 1=read */
    uxsel_callback_fn callback;
    struct callback_set *set;          /* WINSCP: of the owning session */
};

static tree234 *fds;

static int uxsel_fd_cmp(void *av, void *bv)
{
    struct fd *a = (struct fd *)av;
    struct fd *b = (struct fd *)bv;
    if (a->fd < b->fd)
        return -1;
    if (a->fd > b->fd)
        return +1;
    return 0;
}
static int uxsel_fd_findcmp(void *av, void *bv)
{
    int *a = (int *)av;
    struct fd *b = (struct fd *)bv;
    if (*a < b->fd)
        return -1;
    if (*a > b->fd)
        return +1;
    return 0;
}

static void uxsel_del_locked(int fd)
{
    struct fd *oldfd = fds ? find234(fds, &fd, uxsel_fd_findcmp) : NULL;
    if (oldfd) {
        del234(fds, oldfd);
        sfree(oldfd);
    }
}

/*
 * Here is the interface to fd-supplying modules. They supply an
 * fd, a set of read/write/execute states, and a callback function
 * for when the fd satisfies one of those states. Repeated calls to
 * uxsel_set on the same fd are perfectly legal and serve to change
 * the rwx state (typically you only want to select an fd for
 * writing when you actually have pending data you want to write to
 * it!).
 */

void uxsel_set(struct callback_set *set, int fd, int rwx, uxsel_callback_fn callback)
{
    struct fd *newfd;

    assert(fd >= 0);

    WINSCP_PUTTY_SECTION_ENTER;
    if (!fds)
        fds = newtree234(uxsel_fd_cmp);

    uxsel_del_locked(fd);

    if (rwx) {
        newfd = snew(struct fd);
        newfd->fd = fd;
        newfd->rwx = rwx;
        newfd->callback = callback;
        newfd->set = set;
        add234(fds, newfd);
    }
    WINSCP_PUTTY_SECTION_LEAVE;
}

void uxsel_del(int fd)
{
    WINSCP_PUTTY_SECTION_ENTER;
    uxsel_del_locked(fd);
    WINSCP_PUTTY_SECTION_LEAVE;
}

/*
 * And here is the interface to select-functionality-supplying
 * modules.
 */

/* The descriptors of the callback set, in the order of the tree */
int next_fd(struct callback_set *set, int *state, int *rwx)
{
    struct fd *fd;
    int result = -1;
    WINSCP_PUTTY_SECTION_ENTER;
    while (fds && (fd = index234(fds, (*state)++)) != NULL) {
        if (fd->set == set) {
            *rwx = fd->rwx;
            result = fd->fd;
            break;
        }
    }
    WINSCP_PUTTY_SECTION_LEAVE;
    return result;
}

int first_fd(struct callback_set *set, int *state, int *rwx)
{
    *state = 0;
    return next_fd(set, state, rwx);
}

void select_result(int fd, int event)
{
    uxsel_callback_fn callback = NULL;
    WINSCP_PUTTY_SECTION_ENTER;
    struct fd *fdstruct = fds ? find234(fds, &fd, uxsel_fd_findcmp) : NULL;
    if (fdstruct)
        callback = fdstruct->callback;
    WINSCP_PUTTY_SECTION_LEAVE;

    noise_ultralight(NOISE_SOURCE_IOID, fd);

    /*
     * Apparently this can sometimes be NULL. Can't see how, but I
     * assume it means I need to ignore the event since it's on an
     * fd I've stopped being interested in. Sigh.
     */
    if (callback)
        callback(fd, event);
}

/* WinSCP's core frees the handle waits of a session; there are none here */
void delete_handle_wait(struct callback_set *callback_set, HandleWait *hw)
{
}
