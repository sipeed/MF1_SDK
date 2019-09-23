/* mini_httpd - small HTTP server
**
** Copyright � 1999,2000 by Jef Poskanzer <jef@mail.acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#include "httpd.h"

#include <ctype.h>

///////////////////////////////////////////////////////////////////////////////
void add_data(char **bufP, size_t *bufsizeP, size_t *buflenP, char *str, size_t len)
{
    if (*bufsizeP == 0)
    {
        *bufsizeP = len + 500;
        *buflenP = 0;
        *bufP = (char *)malloc(*bufsizeP);
    }
    else if (*buflenP + len >= *bufsizeP) /* allow for NUL */
    {
        *bufsizeP = *buflenP + len + 500;
        *bufP = (char *)realloc((void *)*bufP, *bufsizeP);
    }

    if (len > 0)
    {
        (void)memmove(&((*bufP)[*buflenP]), str, len);
        *buflenP += len;
    }

    (*bufP)[*buflenP] = '\0';
}

void add_str(char **bufP, size_t *bufsizeP, size_t *buflenP, char *str)
{
    size_t len;
    if (str == (char *)0)
        len = 0;
    else
        len = strlen(str);
    add_data(bufP, bufsizeP, buflenP, str, len);
}

///////////////////////////////////////////////////////////////////////////////
static int hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return 0; /* shouldn't happen, we're guarded by isxdigit() */
}

/* Copies and decodes a string.  It's ok for from and to to be the
** same string.
*/
void strdecode(char *to, char *from)
{
    for (; *from != '\0'; ++to, ++from)
    {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            *to = hexit(from[1]) * 16 + hexit(from[2]);
            from += 2;
        }
        else
            *to = *from;
    }
    *to = '\0';
}

///////////////////////////////////////////////////////////////////////////////
void de_dotdot(char *f)
{
    char *cp;
    char *cp2;
    int l;

    /* Collapse any multiple / sequences. */
    while ((cp = strstr(f, "//")) != (char *)0)
    {
        for (cp2 = cp + 2; *cp2 == '/'; ++cp2)
            continue;
        (void)strcpy(cp + 1, cp2);
    }

    /* Remove leading ./ and any /./ sequences. */
    while (strncmp(f, "./", 2) == 0)
        (void)strcpy(f, f + 2);
    while ((cp = strstr(f, "/./")) != (char *)0)
        (void)strcpy(cp, cp + 2);

    /* Alternate between removing leading ../ and removing xxx/../ */
    for (;;)
    {
        while (strncmp(f, "../", 3) == 0)
            (void)strcpy(f, f + 3);
        cp = strstr(f, "/../");
        if (cp == (char *)0)
            break;
        for (cp2 = cp - 1; cp2 >= f && *cp2 != '/'; --cp2)
            continue;
        (void)strcpy(cp2 + 1, cp + 4);
    }

    /* Also elide any xxx/.. at the end. */
    while ((l = strlen(f)) > 3 &&
           strcmp((cp = f + l - 3), "/..") == 0)
    {
        for (cp2 = cp - 1; cp2 >= f && *cp2 != '/'; --cp2)
            continue;
        if (cp2 < f)
            break;
        *cp2 = '\0';
    }
}

///////////////////////////////////////////////////////////////////////////////

/* Base-64 decoding.  This represents binary data as printable ASCII
** characters.  Three 8-bit binary bytes are turned into four 6-bit
** values, like so:
**
**   [11111111]  [22222222]  [33333333]
**
**   [111111] [112222] [222233] [333333]
**
** Then the 6-bit values are represented using the characters "A-Za-z0-9+/".
*/

static const int b64_decode_table[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 00-0F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 10-1F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, /* 20-2F */
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, /* 30-3F */
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,           /* 40-4F */
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, /* 50-5F */
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 60-6F */
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, /* 70-7F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 80-8F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 90-9F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* A0-AF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* B0-BF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* C0-CF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* D0-DF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* E0-EF */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1  /* F0-FF */
};

/* Do base-64 decoding on a string.  Ignore any non-base64 bytes.
** Return the actual number of bytes generated.  The decoded size will
** be at most 3/4 the size of the encoded, and may be smaller if there
** are padding characters (blanks, newlines).
*/
int b64_decode(const char *str, unsigned char *space, int size)
{
    const char *cp;
    int space_idx, phase;
    int d, prev_d = 0;
    unsigned char c;

    space_idx = 0;
    phase = 0;
    for (cp = str; *cp != '\0'; ++cp)
    {
        d = b64_decode_table[(int)((unsigned char)*cp)];
        if (d != -1)
        {
            switch (phase)
            {
            case 0:
                ++phase;
                break;
            case 1:
                c = ((prev_d << 2) | ((d & 0x30) >> 4));
                if (space_idx < size)
                    space[space_idx++] = c;
                ++phase;
                break;
            case 2:
                c = (((prev_d & 0xf) << 4) | ((d & 0x3c) >> 2));
                if (space_idx < size)
                    space[space_idx++] = c;
                ++phase;
                break;
            case 3:
                c = (((prev_d & 0x03) << 6) | d);
                if (space_idx < size)
                    space[space_idx++] = c;
                phase = 0;
                break;
            }
            prev_d = d;
        }
    }
    return space_idx;
}

///////////////////////////////////////////////////////////////////////////////
