#define _CRT_RAND_S
#include <windows.h>

#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

#include "fmem.h"

struct fmem_winimpl {
    FILE *file;
    HANDLE mapping;
    void *base;
};

union fmem_conv {
    fmem *fm;
    struct fmem_winimpl *impl;
};

void fmem_init(fmem *file)
{
    union fmem_conv cv = { .fm = file };
    memset(cv.impl, 0, sizeof (*cv.impl));
}

void fmem_term(fmem *file)
{
    union fmem_conv cv = { .fm = file };
    if (cv.impl->mapping != NULL) {
        UnmapViewOfFile(cv.impl->base);
        CloseHandle(cv.impl->mapping);
    }
}

FILE *fmem_open(fmem *file, const char *mode)
{
    union fmem_conv cv = { .fm = file };
    char path[MAX_PATH];

    DWORD rc = GetTempPathA(sizeof (path), path);
    errno = ENAMETOOLONG;
    if (rc >= sizeof (path))
        return NULL;

    errno = EIO;
    if (rc == 0)
        return NULL;

    HANDLE handle = INVALID_HANDLE_VALUE;
    do {
        unsigned int randnum;
        errno = rand_s(&randnum);
        if (errno)
            return NULL;

        int wb = snprintf(&path[rc], MAX_PATH - rc, "\\fmem%x.tmp", randnum);
        if (wb < 0)
            return NULL;
        if ((DWORD)wb >= (DWORD)MAX_PATH - rc) {
            errno = ENAMETOOLONG;
            return NULL;
        }

        handle = CreateFileA(path,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                CREATE_NEW,
                FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
                NULL);
        if (handle == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS) {
                errno = EIO;
                return NULL;
            }
        }
    } while (handle == INVALID_HANDLE_VALUE);

    int fd = _open_osfhandle((intptr_t) handle, _O_RDWR);
    if (fd == -1) {
        CloseHandle(handle);
        return NULL;
    }

    FILE *f = _fdopen(fd, mode);
    if (!f)
        _close(fd);
    cv.impl->file = f;
    return f;
}

void fmem_mem(fmem *file, void **mem, size_t *size)
{
    union fmem_conv cv = { .fm = file };
    *mem = NULL;
    *size = 0;

    if (!cv.impl->file)
        return;

    HANDLE handle = (HANDLE) _get_osfhandle(_fileno(cv.impl->file));

    DWORD filesize = GetFileSize(handle, NULL);
    if (filesize == INVALID_FILE_SIZE)
        return;

    if (cv.impl->mapping) {
        UnmapViewOfFile(cv.impl->base);
        CloseHandle(cv.impl->mapping);
    }

    HANDLE mapping = CreateFileMapping(handle, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (!mapping)
        return;

    void *base = MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!base) {
        CloseHandle(mapping);
        return;
    }

    cv.impl->mapping = mapping;
    cv.impl->base = base;

    *mem = base;
    *size = filesize;
}
