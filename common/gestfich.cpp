/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file gestfich.cpp
 * @brief Functions for file management
 */

#include <wx/mimetype.h>
#include <wx/filename.h>
#include <wx/dir.h>

#include <pgm_base.h>
#include <confirm.h>
#include <core/arraydim.h>
#include <gestfich.h>

void QuoteString( wxString& string )
{
    if( !string.StartsWith( wxT( "\"" ) ) )
    {
        string.Prepend ( wxT( "\"" ) );
        string.Append ( wxT( "\"" ) );
    }
}


wxString FindKicadFile( const wxString& shortname )
{
    // Test the presence of the file in the directory shortname of
    // the KiCad binary path.
#ifndef __WXMAC__
    wxString fullFileName = Pgm().GetExecutablePath() + shortname;
#else
    wxString fullFileName = Pgm().GetExecutablePath() + wxT( "Contents/MacOS/" ) + shortname;
#endif
    if( wxFileExists( fullFileName ) )
        return fullFileName;

    // Test the presence of the file in the directory shortname
    // defined by the environment variable KiCad.
    if( Pgm().IsKicadEnvVariableDefined() )
    {
        fullFileName = Pgm().GetKicadEnvVariable() + shortname;

        if( wxFileExists( fullFileName ) )
            return fullFileName;
    }

#if defined( __WINDOWS__ )
    // kicad can be installed highly portably on Windows, anywhere and concurrently
    // either the "kicad file" is immediately adjacent to the exe or it's not a valid install
    return shortname;
#endif

    // Path list for KiCad binary files
    const static wxChar* possibilities[] = {
#if defined( __WXMAC__ )
        // all internal paths are relative to main bundle kicad.app
        wxT( "Contents/Applications/pcbnew.app/Contents/MacOS/" ),
        wxT( "Contents/Applications/eeschema.app/Contents/MacOS/" ),
        wxT( "Contents/Applications/gerbview.app/Contents/MacOS/" ),
        wxT( "Contents/Applications/bitmap2component.app/Contents/MacOS/" ),
        wxT( "Contents/Applications/pcb_calculator.app/Contents/MacOS/" ),
        wxT( "Contents/Applications/pl_editor.app/Contents/MacOS/" ),
#else
        wxT( "/usr/bin/" ),
        wxT( "/usr/local/bin/" ),
        wxT( "/usr/local/kicad/bin/" ),
#endif
    };

    // find binary file from possibilities list:
    for( unsigned i=0;  i<arrayDim(possibilities);  ++i )
    {
#ifndef __WXMAC__
        fullFileName = possibilities[i] + shortname;
#else
        // make relative paths absolute
        fullFileName = Pgm().GetExecutablePath() + possibilities[i] + shortname;
#endif

        if( wxFileExists( fullFileName ) )
            return fullFileName;
    }

    return shortname;
}


int ExecuteFile( const wxString& aEditorName, const wxString& aFileName, wxProcess *aCallback )
{
    wxString fullEditorName;
    wxString param;

#ifdef __UNIX__
    int space = aEditorName.Find( ' ' );

    if( space > 0 && !aEditorName.Contains( "\"" ) && !aEditorName.Contains( "\'" ) )
    {
        fullEditorName = FindKicadFile( aEditorName.Mid( 0, space ) );
        param = aEditorName.Mid( space + 1 );
    }
    else
#endif
    {
        fullEditorName = FindKicadFile( aEditorName );
    }

    if( wxFileExists( fullEditorName ) )
    {
        int i = 0;
        char const* args[4];

        args[i++] = fullEditorName.c_str();

        if( !param.IsEmpty() )
            args[i++] = param.c_str();

        args[i++] = aFileName.c_str();
        args[i] = nullptr;

        return wxExecute( args, wxEXEC_ASYNC, aCallback );
    }

    wxString msg;
    msg.Printf( _( "Command '%s' could not be found." ), fullEditorName );
    DisplayError( nullptr, msg, 20 );
    return -1;
}


bool OpenPDF( const wxString& file )
{
    wxString msg;
    wxString filename = file;

    Pgm().ReadPdfBrowserInfos();

    if( Pgm().UseSystemPdfBrowser() )
    {
        // wxLaunchDefaultApplication on Unix systems is run as an external process passing
        // the filename to the appropriate application as a command argument.
        // depending on wxWidgets version, spaces in the path and/or file name will cause
        // argument parsing issues so always quote the filename and path.
        // This is applicable to all Unix platforms with wxWidgets version < 3.1.0.
        // See https://github.com/wxWidgets/wxWidgets/blob/master/src/unix/utilsx11.cpp#L2654
#ifdef __WXGTK__
    #if !wxCHECK_VERSION( 3, 1, 0 )
        // Quote in case there are spaces in the path.
        // Not needed on 3.1.4, but needed in 3.0 versions
        // Moreover, on Linux, 3.1.4 wx version, adding quotes breaks wxLaunchDefaultApplication
        QuoteString( filename );
    #endif
#endif

        if( !wxLaunchDefaultApplication( filename ) )
        {
            msg.Printf( _( "Unable to find a PDF viewer for '%s'." ), filename );
            DisplayError( nullptr, msg );
            return false;
        }
    }
    else
    {
        char const*  args[3];

        args[0] = Pgm().GetPdfBrowserName().c_str();
        args[1] = filename.c_str();
        args[2] = nullptr;

        if( wxExecute( args ) == -1 )
        {
            msg.Printf( _( "Problem while running the PDF viewer '%s'." ), args[0] );
            DisplayError( nullptr, msg );
            return false;
        }
    }

    return true;
}


void OpenFile( const wxString& file )
{
    wxFileName  fileName( file );
    wxFileType* filetype = wxTheMimeTypesManager->GetFileTypeFromExtension( fileName.GetExt() );

    if( !filetype )
        return;

    wxString    command;
    wxFileType::MessageParameters params( file );

    filetype->GetOpenCommand( &command, params );
    delete filetype;

    if( !command.IsEmpty() )
        wxExecute( command );
}


bool doPrintFile( const wxString& file, bool aDryRun )
{
    wxString ext = wxFileName( file ).GetExt();
    wxString application;

#ifdef __WXMSW__
    wxFileType* filetype = wxTheMimeTypesManager->GetFileTypeFromExtension( ext );

    if( filetype )
    {
        wxString                      printCommand;
        wxFileType::MessageParameters params( file );

        filetype->GetPrintCommand( &printCommand, params );

        delete filetype;

        if( !printCommand.IsEmpty() )
        {
            if( !aDryRun )
                wxExecute( printCommand );

            return true;
        }
    }
#endif

#ifdef __WXMAC__
    if( ext == "ps" || ext == "pdf" )
        application = "Preview";
    else if( ext == "csv" )
        application = "Numbers";
    else if( ext == "txt" || ext == "rpt" || ext == "pos" || ext == "cmp" || ext == "net" )
        application = "TextEdit";

    if( !application.IsEmpty() )
    {
        wxString ascript;
        ascript.Printf( "osascript -e 'tell application \"%s\"' "
                                  "-e '   set srcFileRef to (open POSIX file \"%s\")' "
                                  "-e '   activate' "
                                  "-e '   print srcFileRef print dialog true' "
                                  "-e 'end tell' ",
                        application,
                        file );

        if( !aDryRun )
            wxExecute( ascript );

        return true;
    }
#endif

#ifdef __WXGTK__
    if( ext == "ps" || ext == "pdf"
            || ext == "csv"
            || ext == "txt" || ext == "rpt" || ext == "pos" || ext == "cmp" || ext == "net" )
    {
        if( !aDryRun )
        {
            char const* args[3];
            args[0] = "lp";
            args[1] = file.c_str();
            args[2] = nullptr;

            wxExecute( args );
        }

        return true;
    }
#endif

    if( !aDryRun )
    {
        DisplayError( nullptr, wxString::Format( _( "Cannot print '%s'.\n\nUnknown file type." ),
                                                 file ) );
    }

    return false;
}


void PrintFile( const wxString& file )
{
    doPrintFile( file, false );
}


bool CanPrintFile( const wxString& file )
{
    return doPrintFile( file, true );
}


void KiCopyFile( const wxString& aSrcPath, const wxString& aDestPath, wxString& aErrors )
{
    if( !wxCopyFile( aSrcPath, aDestPath ) )
    {
        wxString msg;

        if( !aErrors.IsEmpty() )
            aErrors += "\n";

        msg.Printf( _( "Cannot copy file '%s'." ), aDestPath );
        aErrors += msg;
    }
}


wxString QuoteFullPath( wxFileName& fn, wxPathFormat format )
{
    return wxT( "\"" ) + fn.GetFullPath( format ) + wxT( "\"" );
}
