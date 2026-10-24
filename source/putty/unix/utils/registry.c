/*
 * The registry access of PuTTY's storage, for WinSCP's PuTTY on Linux
 * (OpenSCP).
 *
 * Derived from WinSCP's windows/utils/registry.c. There is no registry:
 * keys and values are WinSCP's configuration, through the hooks the core
 * implements (reg_*_winscp), when it redirects the access; otherwise they
 * do not exist.
 */

#include "putty.h"

void close_regkey(HKEY key)
{
    if (reg_override_winscp())
        close_regkey_winscp(key);
}

bool get_reg_dword(HKEY key, const char *name, DWORD *out)
{
    if (reg_override_winscp())
        return get_reg_dword_winscp(key, name, out);
    return false;
}

bool put_reg_dword(HKEY key, const char *name, DWORD value)
{
    if (reg_override_winscp())
        return put_reg_dword_winscp(key, name, value);
    return false;
}

char *get_reg_sz(HKEY key, const char *name)
{
    if (reg_override_winscp())
        return get_reg_sz_winscp(key, name);
    return NULL;
}

bool put_reg_sz(HKEY key, const char *name, const char *str)
{
    if (reg_override_winscp())
        return put_reg_sz_winscp(key, name, str);
    return false;
}
