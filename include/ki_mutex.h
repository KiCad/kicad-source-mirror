
#ifndef KI_MUTEX_H_
#define KI_MUTEX_H_


/// Establish KiCad MUTEX choices here in this file:
///    typedef MUTEX and typedef MUTLOCK.
///
/// Using an unnamed resource is easier, providing a textual name for a
/// constructor is cumbersome, so we make choice on that criteria mostly:

#if 0

// This is a fine choice between the two, but requires linking to ${Boost_LIBRARIES}
// which currently only happens when GITHUB_PLUGIN goes into pcbnew or _pcbnew.
// cvpcb is left out.  Changing the 4 CMakeLists.txts fixes all this easily.

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

typedef boost::interprocess::interprocess_mutex     MUTEX;
typedef boost::interprocess::scoped_lock<MUTEX>     MUTLOCK;

#else

#include <wx/thread.h>

typedef wxMutex             MUTEX;
typedef wxMutexLocker       MUTLOCK;

#endif

#endif  // KI_MUTEX_H_
