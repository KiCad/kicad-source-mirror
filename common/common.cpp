/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

using KIGFX::COLOR4D;


/**
 * Global variables definitions.
 *
 * TODO: All of these variables should be moved into the class were they
 *       are defined and used.  Most of them probably belong in the
 *       application class.
 */

COLOR4D        g_GhostColor;


std::atomic<unsigned int> LOCALE_IO::m_c_count(0);

LOCALE_IO::LOCALE_IO()
{
    // use thread safe, atomic operation
    if( m_c_count++ == 0 )
    {
        // Store the user locale name, to restore this locale later, in dtor
        m_user_locale = setlocale( LC_ALL, 0 );
        // Switch the locale to C locale, to read/write files with fp numbers
        setlocale( LC_ALL, "C" );
    }
}


LOCALE_IO::~LOCALE_IO()
{
    // use thread safe, atomic operation
    if( --m_c_count == 0 )
    {
        // revert to the user locale
        setlocale( LC_ALL, m_user_locale.c_str() );
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


wxConfigBase* GetNewConfig( const wxString& aProgName )
{
    wxConfigBase* cfg = 0;
    wxFileName configname;
    configname.AssignDir( GetKicadConfigPath() );
    configname.SetFullName( aProgName );

    cfg = new wxFileConfig( wxT( "" ), wxT( "" ), configname.GetFullPath() );
    return cfg;
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
    strResult.Alloc(strlen);

    for ( size_t n = 0; n < strlen; n++ ) {
        wxUniChar str_n = str[n];

        switch ( str_n.GetValue() ) {
#ifdef __WINDOWS__
        case wxT('%'):
#endif // __WINDOWS__
        case wxT('$'):
        {
            Bracket bracket;
#ifdef __WINDOWS__
            if ( str_n == wxT('%') )
              bracket = Bracket_Windows;
            else
#endif // __WINDOWS__
            if ( n == strlen - 1 ) {
                bracket = Bracket_None;
            }
            else {
                switch ( str[n + 1].GetValue() ) {
                case wxT('('):
                    bracket = Bracket_Normal;
                    str_n = str[++n];                   // skip the bracket
                    break;

                case wxT('{'):
                    bracket = Bracket_Curly;
                    str_n = str[++n];                   // skip the bracket
                    break;

                default:
                    bracket = Bracket_None;
                }
            }

            size_t m = n + 1;
            wxUniChar str_m = str[m];

            while ( m < strlen && (wxIsalnum(str_m) || str_m == wxT('_')) )
                str_m = str[++m];

            wxString strVarName(str.c_str() + n + 1, m - n - 1);

#ifdef __WXWINCE__
            const bool expanded = false;
#else
            // NB: use wxGetEnv instead of wxGetenv as otherwise variables
            //     set through wxSetEnv may not be read correctly!
            bool expanded = false;
            wxString tmp;
            if (wxGetEnv(strVarName, &tmp))
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
            if ( bracket != Bracket_None ) {
                if ( m == strlen || str_m != (wxChar)bracket ) {
                    // under MSW it's common to have '%' characters in the registry
                    // and it's annoying to have warnings about them each time, so
                    // ignroe them silently if they are not used for env vars
                    //
                    // under Unix, OTOH, this warning could be useful for the user to
                    // understand why isn't the variable expanded as intended
#ifndef __WINDOWS__
                    wxLogWarning(_("Environment variables expansion failed: missing '%c' at position %u in '%s'."),
                                 (char)bracket, (unsigned int) (m + 1), str.c_str());
#endif // __WINDOWS__
                }
                else {
                    // skip closing bracket unless the variables wasn't expanded
                    if ( !expanded )
                        strResult << (wxChar)bracket;
                    str_m = str[++m];
                }
            }

            n = m - 1;  // skip variable name
            str_n = str[n];
        }
            break;

        case wxT('\\'):
            // backslash can be used to suppress special meaning of % and $
            if ( n != strlen - 1 && (str[n + 1] == wxT('%') || str[n + 1] == wxT('$')) ) {
                str_n = str[++n];
                strResult += str_n;

                break;
            }
            //else: fall through

        default:
            strResult += str_n;
        }
    }

    return strResult;
}


#include <ki_mutex.h>
const wxString ExpandEnvVarSubstitutions( const wxString& aString )
{
    // wxGetenv( wchar_t* ) is not re-entrant on linux.
    // Put a lock on multithreaded use of wxGetenv( wchar_t* ), called from wxEpandEnvVars(),
    static MUTEX    getenv_mutex;

    MUTLOCK lock( getenv_mutex );

    // We reserve the right to do this another way, by providing our own member
    // function.
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
    udir.AppendDir( wxT( "kicad" ) );

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


//
// A cover of wxFileName::SetFullName() which avoids expensive calls to wxFileName::SplitPath().
//
void WX_FILENAME::SetFullName( const wxString& aFileNameAndExtension )
{
    m_fullName = aFileNameAndExtension;

    size_t dot = m_fullName.find_last_of( wxT( '.' ) );
    m_fn.SetName( m_fullName.substr( 0, dot ) );
    m_fn.SetExt( m_fullName.substr( dot + 1 ) );
}


//
// An alernative to wxFileName::GetModificationTime() which avoids multiple calls to stat() on
// POSIX kernels.
//
long long WX_FILENAME::GetTimestamp()
{
#ifdef __WINDOWS__
    if( m_fn.FileExists() )
        return m_fn.GetModificationTime().GetValue().GetValue();
#else
    // By stat-ing the file ourselves we save wxWidgets from doing it three times:
    // Exists( wxFILE_EXISTS_SYMLINK ), FileExists(), and finally GetModificationTime()
    struct stat fn_stat;
    wxLstat( GetFullPath(), &fn_stat );

    // Timestamp the source file, not the symlink
    if( S_ISLNK( fn_stat.st_mode ) )    // wxFILE_EXISTS_SYMLINK
    {
        char buffer[ PATH_MAX + 1 ];
        ssize_t pathLen = readlink( TO_UTF8( GetFullPath() ), buffer, PATH_MAX );

        if( pathLen > 0 )
        {
            buffer[ pathLen ] = '\0';
            wxString srcPath = m_path + wxT( '/' ) + wxString::FromUTF8( buffer );
            wxLstat( srcPath, &fn_stat );
        }
    }

    if( S_ISREG( fn_stat.st_mode ) )    // wxFileExists()
        return fn_stat.st_mtime * 1000;
#endif
    return 0;
}

#ifndef __WINDOWS__

//
// A version of wxDir which avoids expensive calls to wxFileName::wxFileName().
//
WX_DIR::WX_DIR( const wxString& aDirPath ) :
    m_dirpath( aDirPath )
{
    m_dir = NULL;

    // throw away the trailing slashes
    size_t n = m_dirpath.length();

    while ( n > 0 && m_dirpath[--n] == '/' )
        ;

    m_dirpath.Truncate(n + 1);

    m_dir = opendir( m_dirpath.fn_str() );
}


bool WX_DIR::IsOpened() const
{
    return m_dir != nullptr;
}


WX_DIR::~WX_DIR()
{
    if ( m_dir )
        closedir( m_dir );
}


bool WX_DIR::GetFirst( wxString *filename, const wxString& filespec )
{
    m_filespec = filespec;

    rewinddir( m_dir );
    return GetNext( filename );
}


bool WX_DIR::GetNext(wxString *filename) const
{
    dirent *dirEntry = NULL;
    wxString dirEntryName;
    bool matches = false;

    while ( !matches )
    {
        dirEntry = readdir( m_dir );

        if ( !dirEntry )
            return false;

#if wxUSE_UNICODE
        dirEntryName = wxString( dirEntry->d_name, *wxConvFileName );
#else
        dirEntryName = dirEntry->d_name;
#endif

        matches = wxMatchWild( m_filespec, dirEntryName );
    }

    *filename = dirEntryName;

    return true;
}
#endif
