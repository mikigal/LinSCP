/*
 * storage.c: implementation of the interface defined in storage.h for
 * WinSCP's PuTTY on Linux (OpenSCP).
 *
 * Derived from WinSCP's windows/storage.c. WinSCP keeps PuTTY's settings
 * (host keys, the random seed file location) in its own configuration:
 * on Windows, PuTTY's registry access is redirected to it, here the
 * "registry" is that redirection only (see utils/registry.c). Host CAs and
 * sessions are WinSCP's business (core/PuttyIntf.cpp).
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "putty.h"
#include "storage.h"

static const char *const puttystr = PUTTY_REG_POS "\\Sessions";

struct settings_w {
    HKEY sesskey;
};

settings_w *open_settings_w(const char *sessionname, char **errmsg)
{
    *errmsg = NULL;

    if (!sessionname || !*sessionname)
        sessionname = "Default Settings";

    strbuf *sb = strbuf_new();
    escape_registry_key(sessionname, sb);

    HKEY sesskey = create_regkey(HKEY_CURRENT_USER, puttystr, sb->s);
    if (!sesskey) {
        *errmsg = dupprintf("Unable to create registry key\n"
                            "HKEY_CURRENT_USER\\%s\\%s", puttystr, sb->s);
        strbuf_free(sb);
        return NULL;
    }
    strbuf_free(sb);

    settings_w *handle = snew(settings_w);
    handle->sesskey = sesskey;
    return handle;
}

void write_setting_s(settings_w *handle, const char *key, const char *value)
{
    if (handle)
        put_reg_sz(handle->sesskey, key, value);
}

void write_setting_i(settings_w *handle, const char *key, int value)
{
    if (handle)
        put_reg_dword(handle->sesskey, key, value);
}

void close_settings_w(settings_w *handle)
{
    close_regkey(handle->sesskey);
    sfree(handle);
}

struct settings_r {
    HKEY sesskey;
};

settings_r *open_settings_r(const char *sessionname)
{
    if (!sessionname || !*sessionname)
        sessionname = "Default Settings";

    strbuf *sb = strbuf_new();
    escape_registry_key(sessionname, sb);
    HKEY sesskey = open_regkey_ro(HKEY_CURRENT_USER, puttystr, sb->s);
    strbuf_free(sb);

    if (!sesskey)
        return NULL;

    settings_r *handle = snew(settings_r);
    handle->sesskey = sesskey;
    return handle;
}

char *read_setting_s(settings_r *handle, const char *key)
{
    if (!handle)
        return NULL;
    return get_reg_sz(handle->sesskey, key);
}

int read_setting_i(settings_r *handle, const char *key, int defvalue)
{
    DWORD val;
    if (!handle || !get_reg_dword(handle->sesskey, key, &val))
        return defvalue;
    else
        return val;
}

/* As PuTTY's unix/storage.c: a font is its name */
FontSpec *read_setting_fontspec(settings_r *handle, const char *name)
{
    char *fontname = read_setting_s(handle, name);
    if (!fontname)
        return NULL;
    FontSpec *ret = fontspec_new(fontname);
    sfree(fontname);
    return ret;
}

void write_setting_fontspec(settings_w *handle,
                            const char *name, FontSpec *font)
{
    write_setting_s(handle, name, font->name);
}

Filename *read_setting_filename(settings_r *handle, const char *name)
{
    char *tmp = read_setting_s(handle, name);
    if (tmp) {
        Filename *ret = filename_from_utf8(tmp); // WINSCP (though it might not have any effect for our uses of the code)
        sfree(tmp);
        return ret;
    } else
        return NULL;
}

void write_setting_filename(settings_w *handle,
                            const char *name, Filename *result)
{
    write_setting_s(handle, name, result->path);
}

void close_settings_r(settings_r *handle)
{
    if (handle) {
        close_regkey(handle->sesskey);
        sfree(handle);
    }
}

static void hostkey_regname(strbuf *sb, const char *hostname,
                            int port, const char *keytype)
{
    put_fmt(sb, "%s@%d:", keytype, port);
    escape_registry_key(hostname, sb);
}

int retrieve_host_key(const char *hostname, int port,
                      const char *keytype, char *key, int maxlen)
{
    /*
     * Read a saved key in from the registry and see what it says.
     */
    strbuf *regname = strbuf_new();
    hostkey_regname(regname, hostname, port, keytype);

    HKEY rkey = open_regkey_ro(HKEY_CURRENT_USER,
                               PUTTY_REG_POS "\\SshHostKeys");
    if (!rkey) {
        strbuf_free(regname);
        return 1;                      /* key does not exist in registry */
    }

    char *otherstr = get_reg_sz(rkey, regname->s);
    if (!otherstr && !strcmp(keytype, "rsa")) {
        /*
         * Key didn't exist. If the key type is RSA, we'll try
         * another trick, which is to look up the _old_ key format
         * under just the hostname and translate that.
         */
        char *justhost = regname->s + 1 + strcspn(regname->s, ":");
        char *oldstyle = get_reg_sz(rkey, justhost);

        if (oldstyle) {
            /*
             * The old format is two old-style bignums separated by
             * a slash. An old-style bignum is made of groups of
             * four hex digits: digits are ordered in sensible
             * (most to least significant) order within each group,
             * but groups are ordered in silly (least to most)
             * order within the bignum. The new format is two
             * ordinary C-format hex numbers (0xABCDEFG...XYZ, with
             * A nonzero except in the special case 0x0, which
             * doesn't appear anyway in RSA keys) separated by a
             * comma. All hex digits are lowercase in both formats.
             */
            strbuf *new = strbuf_new();
            const char *q = oldstyle;
            int i, j;

            for (i = 0; i < 2; i++) {
                int ndigits, nwords;
                put_datapl(new, PTRLEN_LITERAL("0x"));
                ndigits = strcspn(q, "/");      /* find / or end of string */
                nwords = ndigits / 4;
                /* now trim ndigits to remove leading zeros */
                while (q[(ndigits - 1) ^ 3] == '0' && ndigits > 1)
                    ndigits--;
                /* now move digits over to new string */
                for (j = ndigits; j-- > 0 ;)
                    put_byte(new, q[j ^ 3]);
                q += nwords * 4;
                if (*q) {
                    q++;                 /* eat the slash */
                    put_byte(new, ',');  /* add a comma */
                }
            }

            /*
             * Now _if_ this key matches, we'll enter it in the new
             * format. If not, we'll assume something odd went
             * wrong, and hyper-cautiously do nothing.
             */
            if (!strcmp(new->s, key)) {
                put_reg_sz(rkey, regname->s, new->s);
                otherstr = strbuf_to_str(new);
            } else {
                strbuf_free(new);
            }
        }

        sfree(oldstyle);
    }

    close_regkey(rkey);

    if (otherstr)
    {
        strncpy(key, otherstr, maxlen);
        key[maxlen - 1] = '\0';
    }
    else
    {
        key[0] = '\0';
    }

    sfree(otherstr);
    strbuf_free(regname);

    if (!otherstr)
        return 1;                      /* key does not exist in registry */
    else
        return 0;                      /* key matched OK in registry */
}

void store_host_key(Seat *seat, const char *hostname, int port,
                    const char *keytype, const char *key)
{
    strbuf *regname = strbuf_new();
    hostkey_regname(regname, hostname, port, keytype);

    HKEY rkey = create_regkey(HKEY_CURRENT_USER,
                              PUTTY_REG_POS "\\SshHostKeys");
    if (rkey) {
        put_reg_sz(rkey, regname->s, key);
        close_regkey(rkey);
    } /* else key does not exist in registry */

    strbuf_free(regname);
}

/*
 * Open (or delete) the random seed file, at the location in WinSCP's
 * configuration.
 */
enum { DEL, OPEN_R, OPEN_W };
static int access_random_seed(int action)
{
    int fd = -1;
    HKEY rkey = open_regkey_ro(HKEY_CURRENT_USER, PUTTY_REG_POS);
    if (rkey) {
        char *regpath = get_reg_sz(rkey, "RandSeedFile");
        close_regkey(rkey);
        if (regpath) {
            if (action == DEL) {
                if (unlink(regpath) < 0 && errno != ENOENT)
                    nonfatal("Unable to delete '%s': %s", regpath,
                             strerror(errno));
            } else if (action == OPEN_W) {
                fd = open(regpath, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
            } else {
                fd = open(regpath, O_RDONLY | O_CLOEXEC);
            }
            sfree(regpath);
        }
    }
    return fd;
}

void read_random_seed(noise_consumer_t consumer)
{
    int fd = access_random_seed(OPEN_R);
    if (fd >= 0) {
        while (1) {
            char buf[1024];
            ssize_t len = read(fd, buf, sizeof(buf));
            if (len > 0)
                consumer(buf, len);
            else
                break;
        }
        close(fd);
    }
}

void write_random_seed(void *data, int len)
{
    int fd = access_random_seed(OPEN_W);
    if (fd >= 0) {
        int done = 0;
        while (done < len) {
            ssize_t written = write(fd, (char *)data + done, len - done);
            if (written <= 0)
                break;
            done += written;
        }
        close(fd);
    }
}

void cleanup_all(void)
{
    /*
     * Wipe out the random seed file.
     */
    access_random_seed(DEL);
}
