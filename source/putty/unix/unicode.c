/*
 * unicode.c: character set conversions for WinSCP's PuTTY on Linux
 * (OpenSCP).
 *
 * WinSCP passes Windows code page numbers. The default code page (0, as
 * Windows' CP_ACP) and CP_UTF8 are UTF-8, as the rest of OpenSCP treats
 * them; other code pages convert with iconv ("CP<number>"). PuTTY's own
 * unix/unicode.c needs its terminal character set library instead, which
 * WinSCP does not build.
 */

#include <errno.h>
#include <iconv.h>
#include <stdio.h>

#include "putty.h"
#include "misc.h"

static bool is_utf8_codepage(int codepage)
{
    return codepage == DEFAULT_CODEPAGE || codepage == CP_UTF8;
}

static iconv_t open_codepage(int codepage, bool to_codepage)
{
    char name[32];
    snprintf(name, sizeof(name), "CP%d", codepage);
    return to_codepage ? iconv_open(name, "WCHAR_T") : iconv_open("WCHAR_T", name);
}

/* Converts in to out with iconv; replacement is used for what does not convert */
static bool iconv_to_sink(iconv_t cd, BinarySink *bs, const char *in, size_t inlen,
                          const char *replacement, size_t replacement_len, size_t unit)
{
    char buffer[1024];
    char *inp = (char *)in;
    while (inlen > 0) {
        char *outp = buffer;
        size_t outleft = sizeof(buffer);
        size_t ret = iconv(cd, &inp, &inlen, &outp, &outleft);
        put_data(bs, buffer, sizeof(buffer) - outleft);
        if (ret == (size_t)-1) {
            if (errno == E2BIG)
                continue;
            if (errno == EILSEQ || errno == EINVAL) {
                /* Skip one unit of input, emitting the replacement */
                if (replacement)
                    put_data(bs, replacement, replacement_len);
                size_t skip = unit < inlen ? unit : inlen;
                inp += skip;
                inlen -= skip;
                continue;
            }
            return false;
        }
    }
    return true;
}

bool BinarySink_put_wc_to_mb(
    BinarySink *bs, int codepage, const wchar_t *wcstr, int wclen,
    const char *defchr)
{
    if (!wclen)
        return true;

    if (is_utf8_codepage(codepage)) {
        for (int i = 0; i < wclen; i++)
            put_utf8_char(bs, wcstr[i]);
        return true;
    }

    iconv_t cd = open_codepage(codepage, true);
    if (cd == (iconv_t)-1)
        return false;
    bool success = iconv_to_sink(
        cd, bs, (const char *)wcstr, wclen * sizeof(wchar_t),
        (defchr && *defchr) ? defchr : NULL, (defchr ? strlen(defchr) : 0),
        sizeof(wchar_t));
    iconv_close(cd);
    return success;
}

bool BinarySink_put_mb_to_wc(
    BinarySink *bs, int codepage, const char *mbstr, int mblen)
{
    if (!mblen)
        return true;

    if (is_utf8_codepage(codepage)) {
        BinarySource src[1];
        BinarySource_BARE_INIT(src, mbstr, mblen);

        while (get_avail(src)) {
            wchar_t wcbuf[2];
            size_t nwc = decode_utf8_to_wchar(src, wcbuf, NULL);
            put_data(bs, wcbuf, nwc * sizeof(wchar_t));
        }
        return true;
    }

    iconv_t cd = open_codepage(codepage, false);
    if (cd == (iconv_t)-1)
        return false;
    wchar_t replacement = 0xFFFD;
    bool success = iconv_to_sink(
        cd, bs, mbstr, mblen, (const char *)&replacement, sizeof(replacement), 1);
    iconv_close(cd);
    return success;
}

bool is_dbcs_leadbyte(int codepage, char byte)
{
    return false;                      /* no double-byte code pages */
}
