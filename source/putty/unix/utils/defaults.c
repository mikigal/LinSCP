/*
 * defaults.c: default settings that are specific to the platform, for
 * WinSCP's PuTTY on Linux (LinSCP). As WinSCP's windows/utils/defaults.c,
 * without the Windows specifics: WinSCP does not use fonts and serial
 * lines.
 */

#include "putty.h"

FontSpec *platform_default_fontspec(const char *name)
{
    return fontspec_new("");
}

Filename *platform_default_filename(const char *name)
{
    if (!strcmp(name, "LogFileName"))
        return filename_from_str("putty.log");
    else
        return filename_from_str("");
}

char *platform_default_s(const char *name)
{
    if (!strcmp(name, "SerialLine"))
        return dupstr("/dev/ttyS0");
    return NULL;
}

bool platform_default_b(const char *name, bool def)
{
    return def;
}

int platform_default_i(const char *name, int def)
{
    return def;
}
