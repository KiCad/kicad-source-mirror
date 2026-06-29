/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <locale_io.h>
#include <wx/intl.h>
#include <clocale>
#include <mutex>

// When reading/writing files, we need to switch to setlocale( LC_NUMERIC, "C" ).
// Works fine to read/write files with floating point numbers.
// We can call setlocale( LC_NUMERIC, "C" ) or wxLocale( "C", "C", "C", false )
// wxWidgets discourage a direct call to setlocale
// However, for us, calling wxLocale( "C", "C", "C", false ) has a unwanted effect:
// The I18N translations are no longer active, because the English dictionary is selected.
// To read files, this is not a major issues, but the result can differ
// from using setlocale(xx, "C").
// Previously, we used only setlocale( LC_NUMERIC, "C" )
//
// Known issues are
// on MSW
//    using setlocale( LC_NUMERIC, "C" ) generates an alert message in debug mode,
//    and this message ("Decimal separator mismatch") must be disabled.
//    But calling wxLocale( "C", "C", "C", false ) works fine
// On unix:
//    calling wxLocale( "C", "C", "C", false ) breaks env vars containing non ASCII7 chars.
//    these env vars return a empty string from wxGetEnv() in many cases, and if such a
//    var must be read after calling wxLocale( "C", "C", "C", false ), it looks like empty
//
// So use wxLocale on Windows and setlocale on unix

// On Windows, when using setlocale, a wx alert is generated
// in some cases (reading a bitmap for instance)
// So we disable alerts during the time a file is read or written


// set USE_WXLOCALE to 0 to use setlocale, 1 to use wxLocale:
#if defined( _WIN32 )
#define USE_WXLOCALE 1
#else
#define USE_WXLOCALE 0
#endif

#if !USE_WXLOCALE
#if defined( _WIN32 ) && defined( DEBUG )
#include <wx/appl.h>    // for wxTheApp

// a wxAssertHandler_t function to filter wxWidgets alert messages when reading/writing a file
// when switching the locale to LC_NUMERIC, "C"
// It is used in class LOCALE_IO to hide a useless (in KiCad) wxWidgets alert message
void KiAssertFilter( const wxString &file, int line,
                     const wxString &func, const wxString &cond,
                     const wxString &msg)
{
    if( !msg.Contains( "Decimal separator mismatch" ) )
        wxTheApp->OnAssertFailure( file.c_str(), line, func.c_str(), cond.c_str(), msg.c_str() );
}
#endif
#endif

// Allow for nesting of LOCALE_IO instantiations.  SPICE library parsing (and other code) toggles
// the locale from worker threads in parallel, so both the nesting count and the global locale
// transitions it triggers must be serialized together under one mutex.  An atomic count alone is
// not enough, since setlocale() and the wxLocale new/delete it guards mutate process-global state
// and the heap, which corrupts when two threads transition at the same time.
static std::mutex        locale_mutex;
static unsigned int      locale_count = 0;
#if USE_WXLOCALE
static wxLocale*         locale_wxLocale = nullptr;
#endif
static std::string       locale_user_locale;

LOCALE_IO::LOCALE_IO()
{
    std::lock_guard<std::mutex> lock( locale_mutex );

    if( locale_count++ == 0 )
    {
#if USE_WXLOCALE
        #define C_LANG "C"
        locale_wxLocale = new wxLocale( C_LANG, C_LANG, C_LANG, false );
#else
        // Store the user locale name, to restore this locale later, in dtor
        locale_user_locale = setlocale( LC_NUMERIC, nullptr );
#if defined( _WIN32 ) && defined( DEBUG )
        // Disable wxWidgets alerts
        wxSetAssertHandler( KiAssertFilter );
#endif
        // Switch the locale to C locale, to read/write files with fp numbers
        setlocale( LC_NUMERIC, "C" );
#endif
    }
}


LOCALE_IO::~LOCALE_IO()
{
    std::lock_guard<std::mutex> lock( locale_mutex );

    if( --locale_count == 0 )
    {
        // revert to the user locale
#if USE_WXLOCALE
        delete locale_wxLocale;      // Deleting the wxLocale restored the previous locale
        locale_wxLocale = nullptr;
#else
        setlocale( LC_NUMERIC, locale_user_locale.c_str() );
#if defined( _WIN32 ) && defined( DEBUG )
        // Enable wxWidgets alerts
        wxSetDefaultAssertHandler();
#endif
#endif
    }
}
