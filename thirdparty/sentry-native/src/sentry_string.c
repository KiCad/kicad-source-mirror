#include <string.h>

#include "sentry_alloc.h"
#include "sentry_string.h"

#ifdef SENTRY_PLATFORM_WINDOWS
#    include <wchar.h>
#endif

#define INITIAL_BUFFER_SIZE 128

void
sentry__stringbuilder_init(sentry_stringbuilder_t *sb)
{
    sb->buf = NULL;
    sb->allocated = 0;
    sb->len = 0;
}

char *
sentry__stringbuilder_reserve(sentry_stringbuilder_t *sb, size_t len)
{
    const size_t needed = sb->len + len;
    if (!sb->buf || needed > sb->allocated) {
        size_t new_alloc_size = sb->allocated;
        if (new_alloc_size == 0) {
            new_alloc_size = INITIAL_BUFFER_SIZE;
        }
        while (new_alloc_size < needed) {
            new_alloc_size = new_alloc_size * 2;
        }
        char *new_buf = sentry_malloc(new_alloc_size);
        if (!new_buf) {
            return NULL;
        }
        if (sb->buf) {
            memcpy(new_buf, sb->buf, sb->allocated);
            sentry_free(sb->buf);
        }
        sb->buf = new_buf;
        sb->allocated = new_alloc_size;
    }
    return &sb->buf[sb->len];
}

char *
sentry_stringbuilder_take_string(sentry_stringbuilder_t *sb)
{
    char *rv = sb->buf;
    if (!rv) {
        rv = sentry__string_clone("");
    }
    sb->buf = NULL;
    sb->allocated = 0;
    sb->len = 0;
    return rv;
}

char *
sentry__stringbuilder_into_string(sentry_stringbuilder_t *sb)
{
    char *rv = sentry_stringbuilder_take_string(sb);
    sentry__stringbuilder_cleanup(sb);
    return rv;
}

void
sentry__stringbuilder_cleanup(sentry_stringbuilder_t *sb)
{
    sentry_free(sb->buf);
}

size_t
sentry__stringbuilder_len(const sentry_stringbuilder_t *sb)
{
    return sb->len;
}

void
sentry__stringbuilder_set_len(sentry_stringbuilder_t *sb, size_t len)
{
    sb->len = len;
}

#ifdef SENTRY_PLATFORM_WINDOWS
char *
sentry__string_from_wstr(const wchar_t *s)
{
    if (!s) {
        return NULL;
    }
    const int len = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, s, -1, NULL, 0, NULL, NULL);
    if (len <= 0) {
        return NULL;
    }
    char *rv = sentry_malloc((size_t)len);
    if (!rv) {
        return NULL;
    }
    const int written = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, s, -1, rv, len, NULL, NULL);
    if (written != len) {
        sentry_free(rv);
        return NULL;
    }
    return rv;
}

char *
sentry__string_from_wstr_n(const wchar_t *s, size_t s_len)
{
    if (!s) {
        return NULL;
    }
    if (s_len == 0) {
        char *rv = sentry_malloc(1);
        if (!rv) {
            return NULL;
        }
        rv[0] = '\0';
        return rv;
    }

    const int len = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, s, (int)s_len, NULL, 0, NULL, NULL);
    if (len <= 0) {
        return NULL;
    }

    char *rv = sentry_malloc((size_t)len + 1);
    if (!rv) {
        return NULL;
    }

    const int written = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, s, (int)s_len, rv, len, NULL, NULL);
    if (written != len) {
        sentry_free(rv);
        return NULL;
    }
    rv[len] = '\0';
    return rv;
}

wchar_t *
sentry__string_to_wstr(const char *s)
{
    if (!s) {
        return NULL;
    }
    const int len
        = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
    if (len <= 0) {
        return NULL;
    }
    wchar_t *rv = sentry_malloc(sizeof(wchar_t) * (size_t)len);
    if (!rv) {
        return NULL;
    }
    const int written
        = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, rv, len);
    if (written != len) {
        sentry_free(rv);
        return NULL;
    }
    return rv;
}

wchar_t *
sentry__string_clone_wstr(const wchar_t *s)
{
    if (!s) {
        return NULL;
    }
    const size_t s_len = wcslen(s) + 1;
    wchar_t *clone = sentry_malloc(sizeof(wchar_t) * s_len);
    if (!clone) {
        return NULL;
    }
    wmemcpy(clone, s, s_len);
    return clone;
}
#endif

size_t
sentry__unichar_to_utf8(uint32_t c, char *buf)
{
    size_t len;
    uint32_t first;

    if (c < 0x80) {
        first = 0;
        len = 1;
    } else if (c < 0x800) {
        first = 0xc0;
        len = 2;
    } else if (c < 0x10000) {
        first = 0xe0;
        len = 3;
    } else if (c <= 0x10FFFF) {
        first = 0xf0;
        len = 4;
    } else {
        return 0;
    }

    for (size_t i = len - 1; i > 0; --i) {
        buf[i] = (char)((c & 0x3f) | 0x80);
        c >>= 6;
    }
    buf[0] = (char)(c | first);
    return len;
}
