#ifndef INTERNAL_FMEM_H
#define INTERNAL_FMEM_H

#if defined(__unix__) || defined(__APPLE__)
/* Unix platforms use native open_memstream. */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct fmemi_buf {
    char *mem;
    size_t size;
};

typedef struct fmemi_buf fmem;

union fmem_conv {
    fmem *fm;
    struct fmemi_buf *buf;
};

static inline void fmem_init(fmem *file)
{
    union fmem_conv cv = { .fm = file };
    memset(cv.buf, 0, sizeof (*cv.buf));
}

static inline void fmem_term(fmem *file)
{
    union fmem_conv cv = { .fm = file };
    free(cv.buf->mem);
}

static inline FILE *fmem_open(fmem *file, const char *mode)
{
    (void) mode;

    union fmem_conv cv = { .fm = file };
    free(cv.buf->mem);
    return open_memstream(&cv.buf->mem, &cv.buf->size);
}

static inline void fmem_mem(fmem *file, void **mem, size_t *size)
{
    union fmem_conv cv = { .fm = file };
    *mem = cv.buf->mem;
    *size = cv.buf->size;
}

#else
/* Windows uses the external fmem library. */
#include <fmem.h>
#endif

#endif /* INTERNAL_FMEM_H */
