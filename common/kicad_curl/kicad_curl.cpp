/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mark Roszko <mark.roszko@gmail.com>
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
#include <kicad_curl/kicad_curl.h>
#include <ki_mutex.h>       // MUTEX and MUTLOCK
#include <richio.h>

// These are even more private than class members, and since there is only
// one instance of KICAD_CURL ever, these statics are hidden here to simplify the
// client (API) header file.
static volatile bool s_initialized;

static MUTEX s_lock;


void        (CURL_EXTERN * KICAD_CURL::easy_cleanup)    ( CURL* curl );
CURL*       (CURL_EXTERN * KICAD_CURL::easy_init)       ( void );
CURLcode    (CURL_EXTERN * KICAD_CURL::easy_perform)    ( CURL* curl );
CURLcode    (CURL_EXTERN * KICAD_CURL::easy_setopt)     ( CURL* curl, CURLoption option, ... );
const char* (CURL_EXTERN * KICAD_CURL::easy_strerror)   ( CURLcode );
CURLcode    (CURL_EXTERN * KICAD_CURL::global_init)     ( long flags );
void        (CURL_EXTERN * KICAD_CURL::global_cleanup)  ( void );
curl_slist* (CURL_EXTERN * KICAD_CURL::slist_append)    ( curl_slist*, const char* );
void        (CURL_EXTERN * KICAD_CURL::slist_free_all)  ( curl_slist* );
char*       (CURL_EXTERN * KICAD_CURL::version)         ( void );
curl_version_info_data* (CURL_EXTERN * KICAD_CURL::version_info) (CURLversion);


struct DYN_LOOKUP
{
    const char* name;
    void**      address;
};

// May need to modify "name" for each platform according to how libcurl is built on
// that platform and the spelling or partial mangling of C function names.  On linux
// there is no mangling.
#define DYN_PAIR( basename )    { "curl_" #basename, (void**) &KICAD_CURL::basename }


const DYN_LOOKUP KICAD_CURL::dyn_funcs[] = {
    DYN_PAIR( easy_cleanup ),
    DYN_PAIR( easy_init ),
    DYN_PAIR( easy_perform ),
    DYN_PAIR( easy_setopt ),
    DYN_PAIR( easy_strerror ),
    DYN_PAIR( global_init ),
    DYN_PAIR( global_cleanup ),
    DYN_PAIR( slist_append ),
    DYN_PAIR( slist_free_all ),
    DYN_PAIR( version ),
    DYN_PAIR( version_info ),
};


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
            // dynamically load the library.
            wxDynamicLibrary dso;
            wxString canonicalName = dso.CanonicalizeName( wxT( "curl" ) );

            // This is an ugly hack for MinGW builds.  We should probably use something
            // like objdump to get the actual library file name from the link file.
#if defined( __MINGW32__ )
            canonicalName = dso.CanonicalizeName( wxT( "curl-4" ) );
            canonicalName = wxT( "lib" ) + canonicalName;
#endif

            if( !dso.Load( canonicalName, wxDL_NOW | wxDL_GLOBAL ) )
            {
                // Failure: error reporting UI was done via wxLogSysError().
                std::string msg = StrPrintf( "%s not wxDynamicLibrary::Load()ed",
                                             static_cast< const char *>( canonicalName.mb_str() ) );
                THROW_IO_ERROR( msg );
            }

            // get addresses.

            for( unsigned i=0; i < DIM(dyn_funcs); ++i )
            {
                *dyn_funcs[i].address = dso.GetSymbol( dyn_funcs[i].name );

                if( *dyn_funcs[i].address == NULL )
                {
                    // Failure: error reporting UI was done via wxLogSysError().
                    // No further reporting required here.

                    std::string msg = StrPrintf( "%s has no function %s",
                                                 static_cast<const char*>( canonicalName.mb_str() ),
                                                 dyn_funcs[i].name );

                    THROW_IO_ERROR( msg );
                }
            }

            if( KICAD_CURL::global_init( CURL_GLOBAL_ALL ) != CURLE_OK )
            {
                THROW_IO_ERROR( "curl_global_init() failed." );
            }

            wxLogDebug( "Using %s", GetVersion() );

            // Tell dso's wxDynamicLibrary destructor not to Unload() the program image,
            // since everything is fine before this.  In those cases where THROW_IO_ERROR
            // is called, dso is destroyed and the DSO/DLL is unloaded before returning in
            // those error cases.
            (void) dso.Detach();

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

            KICAD_CURL::global_cleanup();

            // dyn_funcs are not good for anything now, assuming process is ending soon here.
            for( unsigned i=0; i < DIM(dyn_funcs);  ++i )
            {
                *dyn_funcs[i].address = 0;
            }

            s_initialized = false;
        }
    }
}


std::string KICAD_CURL::GetSimpleVersion()
{
    if( !s_initialized )
        Init();

    curl_version_info_data *info = KICAD_CURL::version_info( CURLVERSION_NOW );

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
