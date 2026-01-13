#ifndef SENTRY_UNIX_SPINLOCK_H_INCLUDED
#define SENTRY_UNIX_SPINLOCK_H_INCLUDED

#include "sentry_boot.h"
#include "sentry_cpu_relax.h"

typedef volatile sig_atomic_t sentry_spinlock_t;

/**
 * On UNIX Systems, inside the signal handler, sentry will switch from standard
 * `malloc` to a custom page-based allocator, which is protected by this special
 * spinlock.
 */

#define SENTRY__SPINLOCK_INIT 0
#define sentry__spinlock_lock(spinlock_ref)                                    \
    while (!__sync_bool_compare_and_swap(spinlock_ref, 0, 1)) {                \
        sentry__cpu_relax();                                                   \
    }
#define sentry__spinlock_unlock(spinlock_ref) (*spinlock_ref = 0)

#endif
