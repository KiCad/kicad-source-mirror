/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <gr_basic.h>
#include <trigo.h>
#include <wxstruct.h>
#include <base_struct.h>
#include <common.h>
#include <macros.h>
#include <build_version.h>
#include <confirm.h>
#include <base_units.h>
#include <reporter.h>

#include <wx/process.h>
#include <wx/config.h>
#include <wx/utils.h>
#include <wx/stdpaths.h>


/**
 * Global variables definitions.
 *
 * TODO: All of these variables should be moved into the class were they
 *       are defined and used.  Most of them probably belong in the
 *       application class.
 */

bool           g_ShowPageLimits = true;
EDA_UNITS_T    g_UserUnit;
EDA_COLOR_T    g_GhostColor;


/**
 * Function to use local notation or C standard notation for floating point numbers
 * some countries use 1,5 and others (and C) 1.5
 * so we switch from local to C and C to local when reading or writing files
 * And other problem is a bug when cross compiling under linux:
 * a printf print 1,5 and the read functions expects 1.5
 * (depending on version print = 1.5 and read = 1,5
 * Very annoying and we detect this and use a stupid but necessary workaround
*/
bool g_DisableFloatingPointLocalNotation = false;


int LOCALE_IO::C_count;


void SetLocaleTo_C_standard()
{
    setlocale( LC_NUMERIC, "C" );    // Switch the locale to standard C
}

void SetLocaleTo_Default()
{
    if( !g_DisableFloatingPointLocalNotation )
        setlocale( LC_NUMERIC, "" );      // revert to the current locale
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


wxString ReturnUnitSymbol( EDA_UNITS_T aUnit, const wxString& formatString )
{
    wxString tmp;
    wxString label;

    switch( aUnit )
    {
    case INCHES:
        tmp = _( "\"" );
        break;

    case MILLIMETRES:
        tmp = _( "mm" );
        break;

    case UNSCALED_UNITS:
        break;
    }

    if( formatString.IsEmpty() )
        return tmp;

    label.Printf( formatString, GetChars( tmp ) );

    return label;
}


wxString GetUnitsLabel( EDA_UNITS_T aUnit )
{
    wxString label;

    switch( aUnit )
    {
    case INCHES:
        label = _( "inches" );
        break;

    case MILLIMETRES:
        label = _( "millimeters" );
        break;

    case UNSCALED_UNITS:
        label = _( "units" );
        break;
    }

    return label;
}


wxString GetAbbreviatedUnitsLabel( EDA_UNITS_T aUnit )
{
    wxString label;

    switch( aUnit )
    {
    case INCHES:
        label = _( "in" );
        break;

    case MILLIMETRES:
        label = _( "mm" );
        break;

    case UNSCALED_UNITS:
        break;
    }

    return label;
}


void AddUnitSymbol( wxStaticText& Stext, EDA_UNITS_T aUnit )
{
    wxString msg = Stext.GetLabel();

    msg += ReturnUnitSymbol( aUnit );

    Stext.SetLabel( msg );
}


wxArrayString* wxStringSplit( wxString aString, wxChar aSplitter )
{
    wxArrayString* list = new wxArrayString();

    while( 1 )
    {
        int index = aString.Find( aSplitter );

        if( index == wxNOT_FOUND )
            break;

        wxString tmp;
        tmp = aString.Mid( 0, index );
        aString = aString.Mid( index + 1, aString.size() - index );
        list->Add( tmp );
    }

    if( !aString.IsEmpty() )
    {
        list->Add( aString );
    }

    return list;
}


int ProcessExecute( const wxString& aCommandLine, int aFlags, wxProcess *callback )
{
    return wxExecute( aCommandLine, aFlags, callback );
}


time_t GetNewTimeStamp()
{
    static time_t oldTimeStamp;
    time_t newTimeStamp;

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
        NEGATE( ix );

    int remainder = ix % 10;   // remainder is in precision mm

    if ( remainder <= 2 )
        ix -= remainder;       // truncate to the near number
    else if (remainder >= 8 )
        ix += 10 - remainder;  // round to near number

    if ( x < 0 )
        NEGATE( ix );

    return (double) ix / precision;
}

wxString FormatDateLong( const wxDateTime &aDate )
{
    // GetInfo was introduced only on wx 2.9; for portability reason an
    // hardcoded format is used on wx 2.8
#if wxCHECK_VERSION( 2, 9, 0 )
    return aDate.Format( wxLocale::GetInfo( wxLOCALE_LONG_DATE_FMT ) );
#else
    return aDate.Format( wxT("%d %b %Y") );
#endif
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

    // From the wxWidgets wxStandardPaths::GetUserConfigDir() help:
    //      Unix: ~ (the home directory)
    //      Windows: "C:\Documents and Settings\username\Application Data"
    //      Mac: ~/Library/Preferences
    cfgpath.AssignDir( wxStandardPaths::Get().GetUserConfigDir() );

#if !defined( __WINDOWS__ ) && !defined( __WXMAC__ )
    wxString envstr;

    if( !wxGetEnv( wxT( "XDG_CONFIG_HOME" ), &envstr ) || envstr.IsEmpty() )
    {
        // XDG_CONFIG_HOME is not set, so use the fallback
        cfgpath.AppendDir( wxT( ".config" ) );
    }
    else
    {
        // Override the assignment above with XDG_CONFIG_HOME
        cfgpath.AssignDir( envstr );
    }
#endif

    cfgpath.AppendDir( wxT( "kicad" ) );

#if !wxCHECK_VERSION( 2, 9, 0 )
    #define wxS_DIR_DEFAULT  0777
#endif

    if( !cfgpath.DirExists() )
    {
        cfgpath.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
    }

    return cfgpath.GetPath();
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
    return wxExpandEnvVars( aString );
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
            msg.Printf( _( "*** Error: cannot make path '%s' absolute with respect to '%s'! ***" ),
                        GetChars( aTargetFullFileName->GetPath() ),
                        GetChars( baseFilePath ) );
            aReporter->Report( msg );
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
                msg.Printf( _( "Output directory '%s' created.\n" ), GetChars( outputPath ) );
                aReporter->Report( msg );
                return true;
            }
        }
        else
        {
            if( aReporter )
            {
                msg.Printf( _( "*** Error: cannot create output directory '%s'! ***\n" ),
                            GetChars( outputPath ) );
                aReporter->Report( msg );
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
