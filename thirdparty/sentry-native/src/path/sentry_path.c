#include "sentry_path.h"
#include "sentry_alloc.h"

sentry_path_t *
sentry__path_from_str_n(const char *s, size_t s_len)
{
    char *path = sentry__string_clone_n(s, s_len);
    if (!path) {
        return NULL;
    }
    // NOTE: function will free `path` on error
    return sentry__path_from_str_owned(path);
}

sentry_path_t *
sentry__path_from_str(const char *s)
{
    return s ? sentry__path_from_str_n(s, strlen(s)) : NULL;
}

sentry_path_t *
sentry__path_from_str_owned(char *s)
{
    sentry_path_t *rv = SENTRY_MAKE(sentry_path_t);
    if (!rv) {
        sentry_free(s);
        return NULL;
    }
    rv->path = s;
#ifdef SENTRY_PLATFORM_WINDOWS
    rv->path_w = sentry__string_to_wstr(s);
    if (!rv->path_w) {
        sentry__path_free(rv);
        return NULL;
    }
#endif
    return rv;
}

void
sentry__path_free(sentry_path_t *path)
{
    if (!path) {
        return;
    }
#ifdef SENTRY_PLATFORM_WINDOWS
    sentry_free(path->path_w);
#endif
    sentry_free(path->path);
    sentry_free(path);
}

bool
sentry__path_eq(const sentry_path_t *path_a, const sentry_path_t *path_b)
{
    size_t i = 0;
    while (path_a->path[i] == path_b->path[i]) {
        if (path_a->path[i] == (char)0) {
            return true;
        }
        i++;
    }
    return false;
}

int
sentry__path_remove_all(const sentry_path_t *path)
{
    if (sentry__path_is_dir(path)) {
        sentry_pathiter_t *piter = sentry__path_iter_directory(path);
        if (piter) {
            const sentry_path_t *p;
            while ((p = sentry__pathiter_next(piter)) != NULL) {
                sentry__path_remove_all(p);
            }
            sentry__pathiter_free(piter);
        }
    }
    return sentry__path_remove(path);
}

sentry_filelock_t *
sentry__filelock_new(sentry_path_t *path)
{
    sentry_filelock_t *rv = SENTRY_MAKE(sentry_filelock_t);
    if (!rv) {
        sentry__path_free(path);
        return NULL;
    }
    rv->path = path;
    rv->is_locked = false;

    return rv;
}

void
sentry__filelock_free(sentry_filelock_t *lock)
{
    sentry__filelock_unlock(lock);
    sentry__path_free(lock->path);
    sentry_free(lock);
}
