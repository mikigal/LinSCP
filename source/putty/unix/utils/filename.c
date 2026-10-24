/*
 * Implementation of Filename for Unix, including f_open().
 *
 * WinSCP (OpenSCP): PuTTY 0.85's unix/utils/filename.c, plus WinSCP's
 * in-memory keys (a key file name "@" followed by the key in hex) and
 * filename_from_utf8 (file names are bytes, normally UTF-8).
 */

#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "putty.h"

#ifdef WINSCP
Filename *filename_from_utf8(const char *str)
{
    return filename_from_str(str);
}

const char* in_memory_key_data(const Filename *fn)
{
    const char* result = fn->path;
    if (result[0] != '@')
    {
        result = NULL;
    }
    else
    {
        int len;
        result++;
        len = strlen(result);
        /* 260 is MAX_PATH of Windows, where file names cannot be longer */
        if (((len % 2) != 0) ||
            ((len / 2) < 260))
        {
            result = NULL;
        }
        else
        {
            int i;
            for (i = 0; (result != NULL) && (i < len); i++)
            {
                 if (!isxdigit((unsigned char)result[i]))
                 {
                     result = NULL;
                 }
            }
        }
    }
    return result;
}
#endif

Filename *filename_from_str(const char *str)
{
    Filename *fn = snew(Filename);
    fn->path = dupstr(str);
    return fn;
}

Filename *filename_copy(const Filename *fn)
{
    return filename_from_str(fn->path);
}

const char *filename_to_str(const Filename *fn)
{
    #ifdef WINSCP
    if (in_memory_key_data(fn) != NULL) return "in-memory";
    #endif
    return fn->path;
}

bool filename_equal(const Filename *f1, const Filename *f2)
{
    return !strcmp(f1->path, f2->path);
}

bool filename_is_null(const Filename *fn)
{
    return !fn->path[0];
}

void filename_free(Filename *fn)
{
    sfree(fn->path);
    sfree(fn);
}

void filename_serialise(BinarySink *bs, const Filename *f)
{
    put_asciz(bs, f->path);
}
Filename *filename_deserialise(BinarySource *src)
{
    return filename_from_str(get_asciz(src));
}

char filename_char_sanitise(char c)
{
    if (c == '/')
        return '.';
    return c;
}

FILE *f_open(const Filename *filename, char const *mode, bool is_private)
{
    if (!is_private) {
        return fopen(filename->path, mode);
    } else {
        int fd;
        assert(mode[0] == 'w');        /* is_private is meaningless for read,
                                          and tricky for append */
        fd = open(filename->path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd < 0)
            return NULL;
        return fdopen(fd, mode);
    }
}
