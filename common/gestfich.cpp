/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2017 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file gestfich.cpp
 * @brief Functions for file management
 */

#include <fctsys.h>
#include <wx/mimetype.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <pgm_base.h>
#include <confirm.h>
#include <common.h>
#include <macros.h>

#include <gestfich.h>

void AddDelimiterString( wxString& string )
{
    if( !string.StartsWith( wxT( "\"" ) ) )
    {
        string.Prepend ( wxT( "\"" ) );
        string.Append ( wxT( "\"" ) );
    }
}


wxString EDA_FILE_SELECTOR( const wxString& aTitle,
                            const wxString& aPath,
                            const wxString& aFileName,
                            const wxString& aExtension,
                            const wxString& aWildcard,
                            wxWindow*       aParent,
                            int             aStyle,
                            const bool      aKeepWorkingDirectory,
                            const wxPoint&  aPosition,
                            wxString*       aMruPath )
{
    wxString fullfilename;
    wxString curr_cwd    = wxGetCwd();
    wxString defaultname = aFileName;
    wxString defaultpath = aPath;
    wxString dotted_Ext = wxT(".") + aExtension;

#ifdef __WINDOWS__
    defaultname.Replace( wxT( "/" ), wxT( "\\" ) );
    defaultpath.Replace( wxT( "/" ), wxT( "\\" ) );
#endif

    if( defaultpath.IsEmpty() )
    {
        if( aMruPath == NULL )
            defaultpath = wxGetCwd();
        else
            defaultpath = *aMruPath;
    }

    wxSetWorkingDirectory( defaultpath );

#if 0 && defined (DEBUG)
    printf( "defaultpath=\"%s\" defaultname=\"%s\" Ext=\"%s\" Mask=\"%s\" flag=%d keep_working_directory=%d\n",
            TO_UTF8( defaultpath ),
            TO_UTF8( defaultname ),
            TO_UTF8( aExtension ),
            TO_UTF8( aWildcard ),
            aStyle,
            aKeepWorkingDirectory );
#endif

    fullfilename = wxFileSelector( aTitle, defaultpath, defaultname,
                                   dotted_Ext, aWildcard,
                                   aStyle,         // open mode wxFD_OPEN, wxFD_SAVE ..
                                   aParent, aPosition.x, aPosition.y );

    if( aKeepWorkingDirectory )
        wxSetWorkingDirectory( curr_cwd );

    if( !fullfilename.IsEmpty() && aMruPath )
    {
        wxFileName fn = fullfilename;
        *aMruPath = fn.GetPath();
    }

    return fullfilename;
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

    // Path list for KiCad binary files
    const static wxChar* possibilities[] = {
#if defined( __WINDOWS__ )
        wxT( "c:/kicad/bin/" ),
        wxT( "d:/kicad/bin/" ),
        wxT( "c:/Program Files/kicad/bin/" ),
        wxT( "d:/Program Files/kicad/bin/" ),
#elif defined( __WXMAC__ )
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


int ExecuteFile( wxWindow* frame, const wxString& ExecFile, const wxString& param,
                 wxProcess *callback )
{
    wxString fullFileName = FindKicadFile( ExecFile );

    if( wxFileExists( fullFileName ) )
    {
        if( !param.IsEmpty() )
            fullFileName += wxT( " " ) + param;

        return ProcessExecute( fullFileName, wxEXEC_ASYNC, callback );
    }
#ifdef __WXMAC__
    else
    {
        AddDelimiterString( fullFileName );

        if( !param.IsEmpty() )
            fullFileName += wxT( " " ) + param;

        return ProcessExecute( wxT( "/usr/bin/open -a " ) + fullFileName, wxEXEC_ASYNC, callback );
    }
#endif

    wxString msg;
    msg.Printf( _( "Command \"%s\" could not found" ), GetChars( fullFileName ) );
    DisplayError( frame, msg, 20 );
    return -1;
}


bool OpenPDF( const wxString& file )
{
    wxString command;
    const wxString& filename = file;

    Pgm().ReadPdfBrowserInfos();

    if( !Pgm().UseSystemPdfBrowser() )    //  Run the preferred PDF Browser
    {
        command = Pgm().GetPdfBrowserName() + wxT( " '" ) + filename + wxT( "'" );
    }
    else
    {
        if( wxLaunchDefaultApplication( filename ) )
            return true;

#ifdef __WXMAC__
        command = wxT( "/usr/bin/open -a '" ) + filename + wxT( "'" );
#endif
        // If launching the system default PDF viewer fails, fall through with empty command
        // string so the error message is displayed.
    }

    if( !command.IsEmpty() )
    {
        if( ProcessExecute( command ) != -1 )
        {
            return true;
        }
        else
        {
            wxString msg;
            msg.Printf( _( "Problem while running the PDF viewer\nCommand is \"%s\"" ), command );
            DisplayError( NULL, msg );
        }
    }
    else
    {
        wxString msg;
        msg.Printf( _( "Unable to find a PDF viewer for \"%s\"" ), file );
        DisplayError( NULL, msg );
    }

    return false;
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
        ProcessExecute( command );
}


bool doPrintFile( const wxString& file, bool aDryRun )
{
    wxFileName  fileName( file );
    wxString    ext = fileName.GetExt();
    wxFileType* filetype = wxTheMimeTypesManager->GetFileTypeFromExtension( ext );

    if( !filetype )
        return false;

    wxString    printCommand;
    wxString    openCommand;
    wxString    application;

    wxFileType::MessageParameters params( file );
    filetype->GetPrintCommand( &printCommand, params );
    filetype->GetOpenCommand( &openCommand, params );
    delete filetype;

    if( !printCommand.IsEmpty() )
    {
        if( !aDryRun )
            ProcessExecute( printCommand );

        return true;
    }

#ifdef __WXMAC__
    if( ext == "ps" || ext == "pdf" )
        application = "Preview";
    else if( ext == "csv" )
        application = "Numbers";
    else if( ext == "txt" || ext == "rpt" || ext == "pos" || ext == "cmp" || ext == "net" )
        application = "TextEdit";

    if( !application.IsEmpty() )
    {
        printCommand.Printf( "osascript -e 'tell application \"%s\"' "
                                  "-e '   set srcFileRef to (open POSIX file \"%s\")' "
                                  "-e '   activate' "
                                  "-e '   print srcFileRef print dialog true' "
                                  "-e 'end tell' ",
                             application,
                             file );

        if( !aDryRun )
            system( printCommand.c_str() );

        return true;
    }
#endif

#ifdef __WXGTK__
    if( ext == "ps" || ext == "pdf"
            || ext == "csv"
            || ext == "txt" || ext == "rpt" || ext == "pos" || ext == "cmp" || ext == "net" )
    {
        printCommand.Printf( "lp \"%s\"", file );

        if( !aDryRun )
            ProcessExecute( printCommand );

        return true;
    }
#endif

    if( !aDryRun )
    {
        DisplayError( nullptr, wxString::Format( _( "Cannot print '%s'.\n\nUnknown filetype." ),
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


void CopyFile( const wxString& aSrcPath, const wxString& aDestPath, wxString& aErrors )
{
    if( !wxCopyFile( aSrcPath, aDestPath ) )
    {
        wxString msg;

        if( !aErrors.IsEmpty() )
            aErrors += "\n";

        msg.Printf( _( "Cannot copy file \"%s\"." ), aDestPath );
        aErrors += msg;
    }
}


wxString QuoteFullPath( wxFileName& fn, wxPathFormat format )
{
    return wxT( "\"" ) + fn.GetFullPath( format ) + wxT( "\"" );
}


bool DeleteDirectory( const wxString& aDirName, bool aRecurse, bool aIncludeHidden )
{
    int   hiddenFlag = ( aIncludeHidden ? wxDIR_HIDDEN : 0 );
    wxDir mainDir( aDirName );

    if( !mainDir.IsOpened() )
        return false;

    if( mainDir.HasSubDirs() && !aRecurse )
    {
        mainDir.Close();
        return false;
    }

    // Get the name from wxWidgets so that we are sure all the separators are correct
    wxString fullDirName = mainDir.GetNameWithSep();

    wxString dir;
    bool     valid = mainDir.GetFirst( &dir, wxEmptyString, wxDIR_DIRS | hiddenFlag );

    // Iterate over the subdirectories to recursively delete them and their contents
    while( valid )
    {
        dir.Prepend( fullDirName );

        // This call will also delete the actual directory, so we don't have to
        if( !DeleteDirectory( dir, true, aIncludeHidden ) )
        {
            mainDir.Close();
            return false;
        }
        valid = mainDir.GetNext( &dir );
    }


    wxString file;
    valid = mainDir.GetFirst( &file, wxEmptyString, wxDIR_FILES | hiddenFlag );

    // Iterate over the files to remove all of them from the directory
    while( valid )
    {
        file.Prepend( fullDirName );

        if( !wxRemoveFile( file ) )
        {
            mainDir.Close();
            return false;
        }
        valid = mainDir.GetNext( &file );
    }

    mainDir.Close();

    // Now delete the actual directory
    if( !wxRmdir( aDirName ) )
    {
        mainDir.Close();
        return false;
    }

    return true;
}
