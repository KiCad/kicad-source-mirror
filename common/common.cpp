/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file common.cpp
 */

#include <fctsys.h>
#include <eda_base_frame.h>
#include <base_struct.h>
#include <common.h>
#include <macros.h>
#include <base_units.h>
#include <reporter.h>

#include <wx/process.h>
#include <wx/config.h>
#include <wx/utils.h>
#include <wx/stdpaths.h>
#include <wx/url.h>

#include <pgm_base.h>
#include <clocale>

using KIGFX::COLOR4D;


/**
 * Global variables definitions.
 *
 * TODO: All of these variables should be moved into the class were they
 *       are defined and used.  Most of them probably belong in the
 *       application class.
 */

COLOR4D        g_GhostColor;


// When reading/writing files, we need to swtich to setlocale( LC_NUMERIC, "C" ).
// Works fine to read/write files with floating point numbers.
// We can call setlocale( LC_NUMERIC, "C" ) or wxLocale( "C", "C", "C", false )
// wxWidgets discourage a direct call to setlocale
// However, for us, calling wxLocale( "C", "C", "C", false ) has a unwanted effect:
// The I18N translations are no longer active, because the English dictionary is selected.
// To read files, this is not a major issues, but the resul can differ
// from using setlocale(xx, "C").
// Previouly, we used only setlocale( LC_NUMERIC, "C" )
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


// set USE_WXLOCALE 0 to use setlocale, 1 to use wxLocale:
#if defined( _WIN32 )
#define USE_WXLOCALE 1
#else
#define USE_WXLOCALE 0
#endif

// On Windows, when using setlocale, a wx alert is generated
// in some cases (reading a bitmap for instance)
// So we disable alerts during the time a file is read or written
#if !USE_WXLOCALE
#if defined( _WIN32 ) && defined( DEBUG )
// a wxAssertHandler_t function to filter wxWidgets alert messages when reading/writing a file
// when switching the locale to LC_NUMERIC, "C"
// It is used in class LOCALE_IO to hide a useless (in kicad) wxWidgets alert message
void KiAssertFilter( const wxString &file, int line,
                     const wxString &func, const wxString &cond,
                     const wxString &msg)
{
    if( !msg.Contains( "Decimal separator mismatch" ) )
        wxTheApp->OnAssertFailure( file.c_str(), line, func.c_str(), cond.c_str(), msg.c_str() );
}
#endif
#endif

std::atomic<unsigned int> LOCALE_IO::m_c_count( 0 );
LOCALE_IO::LOCALE_IO() : m_wxLocale( nullptr )
{
    // use thread safe, atomic operation
    if( m_c_count++ == 0 )
    {
#if USE_WXLOCALE
        m_wxLocale = new wxLocale( "C", "C", "C", false );
#else
        // Store the user locale name, to restore this locale later, in dtor
        m_user_locale = setlocale( LC_NUMERIC, nullptr );
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
    // use thread safe, atomic operation
    if( --m_c_count == 0 )
    {
        // revert to the user locale
#if USE_WXLOCALE
        delete m_wxLocale;      // Deleting m_wxLocale restored previous locale
        m_wxLocale = nullptr;
#else
        setlocale( LC_NUMERIC, m_user_locale.c_str() );
#if defined( _WIN32 ) && defined( DEBUG )
        // Enable wxWidgets alerts
        wxSetDefaultAssertHandler();
#endif
#endif
    }
}


wxSize GetTextSize( const wxString& aSingleLine, wxWindow* aWindow )
{
    wxCoord width;
    wxCoord height;

    {
        wxClientDC dc( aWindow );
        dc.SetFont( aWindow->GetFont() );
        dc.GetTextExtent( aSingleLine, &width, &height );
    }

    return wxSize( width, height );
}


bool EnsureTextCtrlWidth( wxTextCtrl* aCtrl, const wxString* aString )
{
    wxWindow* window = aCtrl->GetParent();

    if( !window )
        window = aCtrl;

    wxString ctrlText;

    if( !aString )
    {
        ctrlText = aCtrl->GetValue();
        aString  = &ctrlText;
    }

    wxSize  textz = GetTextSize( *aString, window );
    wxSize  ctrlz = aCtrl->GetSize();

    if( ctrlz.GetWidth() < textz.GetWidth() + 10 )
    {
        ctrlz.SetWidth( textz.GetWidth() + 10 );
        aCtrl->SetSizeHints( ctrlz );
        return true;
    }

    return false;
}


void SelectReferenceNumber( wxTextEntry* aTextEntry )
{
    wxString ref = aTextEntry->GetValue();

    if( ref.find_first_of( '?' ) != ref.npos )
    {
        aTextEntry->SetSelection( ref.find_first_of( '?' ), ref.find_last_of( '?' ) + 1 );
    }
    else
    {
        wxString num = ref;

        while( !num.IsEmpty() && ( !wxIsdigit( num.Last() ) || !wxIsdigit( num.GetChar( 0 ) ) ) )
        {
            if( !wxIsdigit( num.Last() ) )
                num.RemoveLast();

            if( !wxIsdigit( num.GetChar ( 0 ) ) )
                num = num.Right( num.Length() - 1);
        }

        aTextEntry->SetSelection( ref.Find( num ), ref.Find( num ) + num.Length() );

        if( num.IsEmpty() )
            aTextEntry->SetSelection( -1, -1 );
    }
}


void wxStringSplit( const wxString& aText, wxArrayString& aStrings, wxChar aSplitter )
{
    wxString tmp;

    for( unsigned ii = 0; ii < aText.Length(); ii++ )
    {
        if( aText[ii] == aSplitter )
        {
            aStrings.Add( tmp );
            tmp.Clear();
        }

        else
            tmp << aText[ii];
    }

    if( !tmp.IsEmpty() )
    {
        aStrings.Add( tmp );
    }
}


int ProcessExecute( const wxString& aCommandLine, int aFlags, wxProcess *callback )
{
    return wxExecute( aCommandLine, aFlags, callback );
}


timestamp_t GetNewTimeStamp()
{
    static timestamp_t oldTimeStamp;
    timestamp_t newTimeStamp;

    newTimeStamp = time( NULL );

    if( newTimeStamp <= oldTimeStamp )
        newTimeStamp = oldTimeStamp + 1;

    oldTimeStamp = newTimeStamp;

    return newTimeStamp;
}


double RoundTo0( double x, double precision )
{
    assert( precision != 0 );

    long long ix = KiROUND( x * precision );

    if ( x < 0.0 )
        ix = -ix;

    int remainder = ix % 10;   // remainder is in precision mm

    if( remainder <= 2 )
        ix -= remainder;       // truncate to the near number
    else if( remainder >= 8 )
        ix += 10 - remainder;  // round to near number

    if ( x < 0 )
        ix = -ix;

    return (double) ix / precision;
}


std::unique_ptr<wxConfigBase> GetNewConfig( const wxString& aProgName )
{
    wxFileName configname;
    configname.AssignDir( GetKicadConfigPath() );
    configname.SetFullName( aProgName );

    // explicitly use wxFileConfig to prevent storing any settings in the system registry on Windows
    return std::make_unique<wxFileConfig>( wxT( "" ), wxT( "" ), configname.GetFullPath() );
}


wxString GetKicadConfigPath()
{
    wxFileName cfgpath;

    // http://docs.wxwidgets.org/3.0/classwx_standard_paths.html#a7c7cf595d94d29147360d031647476b0
    cfgpath.AssignDir( wxStandardPaths::Get().GetUserConfigDir() );

    // GetUserConfigDir() does not default to ~/.config which is the current standard
    // configuration file location on Linux.  This has been fixed in later versions of wxWidgets.
#if !defined( __WXMSW__ ) && !defined( __WXMAC__ )
    wxArrayString dirs = cfgpath.GetDirs();

    if( dirs.Last() != ".config" )
        cfgpath.AppendDir( ".config" );
#endif

    wxString envstr;

    // This shouldn't cause any issues on Windows or MacOS.
    if( wxGetEnv( wxT( "XDG_CONFIG_HOME" ), &envstr ) && !envstr.IsEmpty() )
    {
        // Override the assignment above with XDG_CONFIG_HOME
        cfgpath.AssignDir( envstr );
    }

    cfgpath.AppendDir( wxT( "kicad" ) );

    // Use KICAD_CONFIG_HOME to allow the user to force a specific configuration path.
    if( wxGetEnv( wxT( "KICAD_CONFIG_HOME" ), &envstr ) && !envstr.IsEmpty() )
    {
        // Override the assignment above with KICAD_CONFIG_HOME
        cfgpath.AssignDir( envstr );
    }

    if( !cfgpath.DirExists() )
    {
        cfgpath.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
    }

    return cfgpath.GetPath();
}


enum Bracket
{
    Bracket_None,
    Bracket_Normal  = ')',
    Bracket_Curly   = '}',
#ifdef  __WINDOWS__
    Bracket_Windows = '%',    // yeah, Windows people are a bit strange ;-)
#endif
    Bracket_Max
};


//
// Stolen from wxExpandEnvVars and then heavily optimized
//
wxString KIwxExpandEnvVars(const wxString& str)
{
    size_t strlen = str.length();

    wxString strResult;
    strResult.Alloc( strlen );

    for( size_t n = 0; n < strlen; n++ )
    {
        wxUniChar str_n = str[n];

        switch( str_n.GetValue() )
        {
#ifdef __WINDOWS__
        case wxT( '%' ):
#endif // __WINDOWS__
        case wxT( '$' ):
        {
            Bracket bracket;
#ifdef __WINDOWS__
            if( str_n == wxT( '%' ) )
              bracket = Bracket_Windows;
            else
#endif // __WINDOWS__
            if( n == strlen - 1 )
            {
                bracket = Bracket_None;
            }
            else
            {
                switch( str[n + 1].GetValue() )
                {
                case wxT( '(' ):
                    bracket = Bracket_Normal;
                    str_n = str[++n];                   // skip the bracket
                    break;

                case wxT( '{' ):
                    bracket = Bracket_Curly;
                    str_n = str[++n];                   // skip the bracket
                    break;

                default:
                    bracket = Bracket_None;
                }
            }

            size_t m = n + 1;
            wxUniChar str_m = str[m];

            while( m < strlen && ( wxIsalnum( str_m ) || str_m == wxT( '_' ) ) )
                str_m = str[++m];

            wxString strVarName( str.c_str() + n + 1, m - n - 1 );

#ifdef __WXWINCE__
            const bool expanded = false;
#else
            // NB: use wxGetEnv instead of wxGetenv as otherwise variables
            //     set through wxSetEnv may not be read correctly!
            bool expanded = false;
            wxString tmp;

            if( wxGetEnv( strVarName, &tmp ) )
            {
                strResult += tmp;
                expanded = true;
            }
            else
#endif
            {
                // variable doesn't exist => don't change anything
#ifdef  __WINDOWS__
                if ( bracket != Bracket_Windows )
#endif
                if ( bracket != Bracket_None )
                    strResult << str[n - 1];

                strResult << str_n << strVarName;
            }

            // check the closing bracket
            if( bracket != Bracket_None )
            {
                if( m == strlen || str_m != (wxChar)bracket )
                {
                    // under MSW it's common to have '%' characters in the registry
                    // and it's annoying to have warnings about them each time, so
                    // ignore them silently if they are not used for env vars
                    //
                    // under Unix, OTOH, this warning could be useful for the user to
                    // understand why isn't the variable expanded as intended
#ifndef __WINDOWS__
                    wxLogWarning( _( "Environment variables expansion failed: missing '%c' "
                                     "at position %u in '%s'." ),
                                  (char)bracket, (unsigned int) (m + 1), str.c_str() );
#endif // __WINDOWS__
                }
                else
                {
                    // skip closing bracket unless the variables wasn't expanded
                    if( !expanded )
                        strResult << (wxChar)bracket;

                    m++;
                }
            }

            n = m - 1;  // skip variable name
            str_n = str[n];
        }
            break;

        case wxT( '\\' ):
            // backslash can be used to suppress special meaning of % and $
            if( n != strlen - 1 && (str[n + 1] == wxT( '%' ) || str[n + 1] == wxT( '$' )) )
            {
                str_n = str[++n];
                strResult += str_n;

                break;
            }
            //else: fall through

        default:
            strResult += str_n;
        }
    }

#ifndef __WINDOWS__
    if( strResult.StartsWith( "~" ) )
        strResult.Replace( "~", wxGetHomeDir(), false );
#endif // __WINDOWS__

    return strResult;
}


#include <ki_mutex.h>
const wxString ExpandEnvVarSubstitutions( const wxString& aString )
{
    // wxGetenv( wchar_t* ) is not re-entrant on linux.
    // Put a lock on multithreaded use of wxGetenv( wchar_t* ), called from wxEpandEnvVars(),
    static MUTEX    getenv_mutex;

    MUTLOCK lock( getenv_mutex );

    // We reserve the right to do this another way, by providing our own member function.
    return KIwxExpandEnvVars( aString );
}


const wxString ResolveUriByEnvVars( const wxString& aUri )
{
    // URL-like URI: return as is.
    wxURL url( aUri );

    if( url.GetError() == wxURL_NOERR )
        return aUri;

    // Otherwise, the path points to a local file. Resolve environment
    // variables if any.
    return ExpandEnvVarSubstitutions( aUri );
}


bool EnsureFileDirectoryExists( wxFileName*     aTargetFullFileName,
                                const wxString& aBaseFilename,
                                REPORTER*       aReporter )
{
    wxString msg;
    wxString baseFilePath = wxFileName( aBaseFilename ).GetPath();

    // make aTargetFullFileName path, which is relative to aBaseFilename path (if it is not
    // already an absolute path) absolute:
    if( !aTargetFullFileName->MakeAbsolute( baseFilePath ) )
    {
        if( aReporter )
        {
            msg.Printf( _( "Cannot make path \"%s\" absolute with respect to \"%s\"." ),
                        GetChars( aTargetFullFileName->GetPath() ),
                        GetChars( baseFilePath ) );
            aReporter->Report( msg, REPORTER::RPT_ERROR );
        }

        return false;
    }

    // Ensure the path of aTargetFullFileName exists, and create it if needed:
    wxString outputPath( aTargetFullFileName->GetPath() );

    if( !wxFileName::DirExists( outputPath ) )
    {
        if( wxMkdir( outputPath ) )
        {
            if( aReporter )
            {
                msg.Printf( _( "Output directory \"%s\" created.\n" ), GetChars( outputPath ) );
                aReporter->Report( msg, REPORTER::RPT_INFO );
                return true;
            }
        }
        else
        {
            if( aReporter )
            {
                msg.Printf( _( "Cannot create output directory \"%s\".\n" ),
                            GetChars( outputPath ) );
                aReporter->Report( msg, REPORTER::RPT_ERROR );
            }

            return false;
        }
    }

    return true;
}


#ifdef __WXMAC__
wxString GetOSXKicadUserDataDir()
{
    // According to wxWidgets documentation for GetUserDataDir:
    // Mac: ~/Library/Application Support/appname
    wxFileName udir( wxStandardPaths::Get().GetUserDataDir(), wxEmptyString );

    // Since appname is different if started via launcher or standalone binary
    // map all to "kicad" here
    udir.RemoveLastDir();
    udir.AppendDir( "kicad" );

    return udir.GetPath();
}


wxString GetOSXKicadMachineDataDir()
{
    return wxT( "/Library/Application Support/kicad" );
}


wxString GetOSXKicadDataDir()
{
    // According to wxWidgets documentation for GetDataDir:
    // Mac: appname.app/Contents/SharedSupport bundle subdirectory
    wxFileName ddir( wxStandardPaths::Get().GetDataDir(), wxEmptyString );

    // This must be mapped to main bundle for everything but kicad.app
    const wxArrayString dirs = ddir.GetDirs();
    if( dirs[dirs.GetCount() - 3] != wxT( "kicad.app" ) )
    {
        // Bundle structure resp. current path is
        //   kicad.app/Contents/Applications/<standalone>.app/Contents/SharedSupport
        // and will be mapped to
        //   kicad.app/Contents/SharedSupprt
        ddir.RemoveLastDir();
        ddir.RemoveLastDir();
        ddir.RemoveLastDir();
        ddir.RemoveLastDir();
        ddir.AppendDir( wxT( "SharedSupport" ) );
    }

    return ddir.GetPath();
}
#endif


// add this only if it is not in wxWidgets (for instance before 3.1.0)
#ifdef USE_KICAD_WXSTRING_HASH
size_t std::hash<wxString>::operator()( const wxString& s ) const
{
    return std::hash<std::wstring>{}( s.ToStdWstring() );
}
#endif


#ifdef USE_KICAD_WXPOINT_LESS
bool std::less<wxPoint>::operator()( const wxPoint& aA, const wxPoint& aB ) const
{
    if( aA.x == aB.x )
        return aA.y < aB.y;

    return aA.x < aB.x;
}
#endif


/**
 * Performance enhancements to file and directory operations.
 *
 * Note: while it's annoying to have to make copies of wxWidgets stuff and then
 * add platform-specific performance optimizations, the following routines offer
 * SIGNIFICANT performance benefits.
 */

/**
 * WX_FILENAME
 *
 * A wrapper around a wxFileName which avoids expensive calls to wxFileName::SplitPath()
 * and string concatenations by caching the path and filename locally and only resolving
 * the wxFileName when it has to.
 */
WX_FILENAME::WX_FILENAME( const wxString& aPath, const wxString& aFilename ) :
        m_fn( aPath, aFilename ),
        m_path( aPath ),
        m_fullName( aFilename )
{ }


void WX_FILENAME::SetFullName( const wxString& aFileNameAndExtension )
{
    m_fullName = aFileNameAndExtension;
}


wxString WX_FILENAME::GetName() const
{
    size_t dot = m_fullName.find_last_of( wxT( '.' ) );
    return m_fullName.substr( 0, dot );
}


wxString WX_FILENAME::GetFullName() const
{
    return m_fullName;
}


wxString WX_FILENAME::GetPath() const
{
    return m_path;
}


wxString WX_FILENAME::GetFullPath() const
{
    return m_path + wxT( '/' ) + m_fullName;
}


// Write locally-cached values to the wxFileName.  MUST be called before using m_fn.
void WX_FILENAME::resolve()
{
    size_t dot = m_fullName.find_last_of( wxT( '.' ) );
    m_fn.SetName( m_fullName.substr( 0, dot ) );
    m_fn.SetExt( m_fullName.substr( dot + 1 ) );
}


long long WX_FILENAME::GetTimestamp()
{
    resolve();

    if( m_fn.FileExists() )
        return m_fn.GetModificationTime().GetValue().GetValue();

    return 0;
}


/**
 * A copy of wxMatchWild(), which wxWidgets attributes to Douglas A. Lewis
 * <dalewis@cs.Buffalo.EDU> and ircII's reg.c.
 *
 * This version is modified to skip any encoding conversions (for performance).
 */
bool matchWild( const char* pat, const char* text, bool dot_special )
{
    if( !*text )
    {
        /* Match if both are empty. */
        return !*pat;
    }

    const char *m = pat,
    *n = text,
    *ma = NULL,
    *na = NULL;
    int just = 0,
    acount = 0,
    count = 0;

    if( dot_special && (*n == '.') )
    {
        /* Never match so that hidden Unix files
         * are never found. */
        return false;
    }

    for(;;)
    {
        if( *m == '*' )
        {
            ma = ++m;
            na = n;
            just = 1;
            acount = count;
        }
        else if( *m == '?' )
        {
            m++;

            if( !*n++ )
                return false;
        }
        else
        {
            if( *m == '\\' )
            {
                m++;

                /* Quoting "nothing" is a bad thing */
                if( !*m )
                    return false;
            }
            if( !*m )
            {
                /*
                * If we are out of both strings or we just
                * saw a wildcard, then we can say we have a
                * match
                */
                if( !*n )
                    return true;

                if( just )
                    return true;

                just = 0;
                goto not_matched;
            }

            /*
            * We could check for *n == NULL at this point, but
            * since it's more common to have a character there,
            * check to see if they match first (m and n) and
            * then if they don't match, THEN we can check for
            * the NULL of n
            */
            just = 0;

            if( *m == *n )
            {
                m++;
                count++;
                n++;
            }
            else
            {
                not_matched:

                /*
                 * If there are no more characters in the
                 * string, but we still need to find another
                 * character (*m != NULL), then it will be
                 * impossible to match it
                 */
                if( !*n )
                    return false;

                if( ma )
                {
                    m = ma;
                    n = ++na;
                    count = acount;
                }
                else
                    return false;
            }
        }
    }
}


/**
 * A copy of ConvertFileTimeToWx() because wxWidgets left it as a static function
 * private to src/common/filename.cpp.
 */
#if wxUSE_DATETIME && defined(__WIN32__) && !defined(__WXMICROWIN__)

// Convert between wxDateTime and FILETIME which is a 64-bit value representing
// the number of 100-nanosecond intervals since January 1, 1601 UTC.
//
// This is the offset between FILETIME epoch and the Unix/wxDateTime Epoch.
static wxInt64 EPOCH_OFFSET_IN_MSEC = wxLL(11644473600000);


static void ConvertFileTimeToWx( wxDateTime *dt, const FILETIME &ft )
{
    wxLongLong t( ft.dwHighDateTime, ft.dwLowDateTime );
    t /= 10000; // Convert hundreds of nanoseconds to milliseconds.
    t -= EPOCH_OFFSET_IN_MSEC;

    *dt = wxDateTime( t );
}

#endif // wxUSE_DATETIME && __WIN32__


/**
 * TimestampDir
 *
 * This routine offers SIGNIFICANT performance benefits over using wxWidgets to gather
 * timestamps from matching files in a directory.
 * @param aDirPath the directory to search
 * @param aFilespec a (wildcarded) file spec to match against
 * @return a hash of the last-mod-dates of all matching files in the directory
 */
long long TimestampDir( const wxString& aDirPath, const wxString& aFilespec )
{
    long long timestamp = 0;

#if defined( __WIN32__ )
    // Win32 version.
    // Save time by not searching for each path twice: once in wxDir.GetNext() and once in
    // wxFileName.GetModificationTime().  Also cuts out wxWidgets' string-matching and case
    // conversion by staying on the MSW side of things.
    std::wstring filespec( aDirPath.t_str() );
    filespec += '\\';
    filespec += aFilespec.t_str();

    WIN32_FIND_DATA findData;
    wxDateTime      lastModDate;

    HANDLE fileHandle = ::FindFirstFile( filespec.data(), &findData );

    if( fileHandle != INVALID_HANDLE_VALUE )
    {
        do
        {
            ConvertFileTimeToWx( &lastModDate, findData.ftLastWriteTime );
            timestamp += lastModDate.GetValue().GetValue();
        }
        while ( FindNextFile( fileHandle, &findData ) != 0 );
    }

    FindClose( fileHandle );
#else
    // POSIX version.
    // Save time by not converting between encodings -- do everything on the file-system side.
    std::string filespec( aFilespec.fn_str() );
    std::string dir_path( aDirPath.fn_str() );

    DIR* dir = opendir( dir_path.c_str() );

    if( dir )
    {
        for( dirent* dir_entry = readdir( dir ); dir_entry; dir_entry = readdir( dir ) )
        {
            if( !matchWild( filespec.c_str(), dir_entry->d_name, true ) )
                continue;

            std::string entry_path = dir_path + '/' + dir_entry->d_name;
            struct stat entry_stat;

            wxCRT_Lstat( entry_path.c_str(), &entry_stat );

            // Timestamp the source file, not the symlink
            if( S_ISLNK( entry_stat.st_mode ) )    // wxFILE_EXISTS_SYMLINK
            {
                char buffer[ PATH_MAX + 1 ];
                ssize_t pathLen = readlink( entry_path.c_str(), buffer, PATH_MAX );

                if( pathLen > 0 )
                {
                    buffer[ pathLen ] = '\0';
                    entry_path = dir_path + buffer;

                    wxCRT_Lstat( entry_path.c_str(), &entry_stat );
                }
            }

            if( S_ISREG( entry_stat.st_mode ) )    // wxFileExists()
                timestamp += entry_stat.st_mtime * 1000;
        }

        closedir( dir );
    }
#endif

    return timestamp;
}
