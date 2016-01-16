/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <wx/log.h>
#include <wx/dynlib.h>

#include <macros.h>
#include <fctsys.h>
#include <kicad_curl/kicad_curl.h>
#include <ki_mutex.h>       // MUTEX and MUTLOCK
#include <richio.h>



// These are even more private than class members, and since there is only
// one instance of KICAD_CURL ever, these statics are hidden here to simplify the
// client (API) header file.
static volatile bool s_initialized;

static MUTEX s_lock;        // for s_initialized

// Assume that on these platforms libcurl uses OpenSSL
#if defined(__linux__) || defined(_WIN32)

#include <openssl/crypto.h>

static MUTEX* s_crypto_locks;

static void lock_callback( int mode, int type, const char* file, int line )
{
    (void)file;
    (void)line;

    wxASSERT( s_crypto_locks && unsigned( type ) < unsigned( CRYPTO_num_locks() ) );

    //DBG( printf( "%s: mode=0x%x type=%d file=%s line=%d\n", __func__, mode, type, file, line );)

    if( mode & CRYPTO_LOCK )
    {
        s_crypto_locks[ type ].lock();
    }
    else
    {
        s_crypto_locks[ type ].unlock();
    }
}


static void init_locks()
{
    s_crypto_locks = new MUTEX[ CRYPTO_num_locks() ];

    // From http://linux.die.net/man/3/crypto_set_id_callback:

    /*

    OpenSSL can safely be used in multi-threaded applications provided that at
    least two callback functions are set, locking_function and threadid_func.

    locking_function(int mode, int n, const char *file, int line) is needed to
    perform locking on shared data structures. (Note that OpenSSL uses a number
    of global data structures that will be implicitly shared whenever multiple
    threads use OpenSSL.) Multi-threaded applications will crash at random if it
    is not set.

    threadid_func( CRYPTO_THREADID *id) is needed to record the
    currently-executing thread's identifier into id. The implementation of this
    callback should not fill in id directly, but should use
    CRYPTO_THREADID_set_numeric() if thread IDs are numeric, or
    CRYPTO_THREADID_set_pointer() if they are pointer-based. If the application
    does not register such a callback using CRYPTO_THREADID_set_callback(), then
    a default implementation is used - on Windows and BeOS this uses the
    system's default thread identifying APIs, and on all other platforms it uses
    the address of errno. The latter is satisfactory for thread-safety if and
    only if the platform has a thread-local error number facility.

    Dick: "sounds like CRYPTO_THREADID_set_callback() is not mandatory on our
    2 OpenSSL platforms."

    */

    CRYPTO_set_locking_callback( &lock_callback );
}


static void kill_locks()
{
    CRYPTO_set_locking_callback( NULL );

    delete[] s_crypto_locks;

    s_crypto_locks = NULL;
}

#else

inline void init_locks()    { /* dummy */ }
inline void kill_locks()    { /* dummy */ }

#endif

/// At process termination, using atexit() keeps the CURL stuff out of the
/// singletops and PGM_BASE.
static void at_terminate()
{
    KICAD_CURL::Cleanup();
}


void KICAD_CURL::Init()
{
    // We test s_initialized twice in an effort to avoid
    // unnecessarily locking s_lock.  This understands that the common case
    // will not need to lock.
    if( !s_initialized )
    {
        MUTLOCK lock( s_lock );

        if( !s_initialized )
        {
            if( curl_global_init( CURL_GLOBAL_ALL ) != CURLE_OK )
            {
                THROW_IO_ERROR( "curl_global_init() failed." );
            }

            init_locks();

            wxLogDebug( "Using %s", GetVersion() );

            s_initialized = true;
        }
    }
}


void KICAD_CURL::Cleanup()
{
    /*

    Calling MUTLOCK() from a static destructor will typically be bad, since the
    s_lock may already have been statically destroyed itself leading to a boost
    exception. (Remember C++ does not provide certain sequencing of static
    destructor invocation.)

    To prevent this we test s_initialized twice, which ensures that the MUTLOCK
    is only instantiated on the first call, which should be from
    PGM_BASE::destroy() which is first called earlier than static destruction.
    Then when called again from the actual PGM_BASE::~PGM_BASE() function,
    MUTLOCK will not be instantiated because s_initialized will be false.

    */

    if( s_initialized )
    {
        MUTLOCK lock( s_lock );

        if( s_initialized )
        {
            curl_global_cleanup();

            kill_locks();

            atexit( &at_terminate );

            s_initialized = false;
        }
    }
}


std::string KICAD_CURL::GetSimpleVersion()
{
    if( !s_initialized )
        Init();

    curl_version_info_data* info = curl_version_info( CURLVERSION_NOW );

    std::string res;

    if( info->version )
    {
        res += "libcurl version: " + std::string( info->version );
    }

    res += " (";

    if( info->features & CURL_VERSION_SSL )
    {
        res += "with SSL - ";
        res += std::string( info->ssl_version );
    }
    else
    {
        res += "without SSL";
    }
    res += ")";

    return res;
}
