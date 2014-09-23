/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
 * @file gestfich.cpp
 * @brief Functions for file management
 */

// For compilers that support precompilation, includes "wx.h".
#include <fctsys.h>
#include <pgm_base.h>
#include <confirm.h>

#include <common.h>
#include <macros.h>
#include <gestfich.h>

#include <wx/mimetype.h>
#include <wx/filename.h>
#include <wx/dir.h>


void AddDelimiterString( wxString& string )
{
    wxString text;

    if( !string.StartsWith( wxT( "\"" ) ) )
        text = wxT( "\"" );

    text += string;

    if( (text.Last() != '"' ) || (text.length() <= 1) )
        text += wxT( "\"" );

    string = text;
}


bool EDA_DirectorySelector( const wxString& Title,
                            wxString&       Path,
                            int             flag,
                            wxWindow*       Frame,
                            const wxPoint&  Pos )
{
    int          ii;
    bool         selected = false;

    wxDirDialog* DirFrame = new wxDirDialog( Frame,
                                             wxString( Title ),
                                             Path,
                                             flag,
                                             Pos );

    ii = DirFrame->ShowModal();

    if( ii == wxID_OK )
    {
        Path     = DirFrame->GetPath();
        selected = true;
    }

    DirFrame->Destroy();
    return selected;
}


wxString EDA_FileSelector( const wxString& Title,
                           const wxString& Path,
                           const wxString& FileName,
                           const wxString& Ext,
                           const wxString& Mask,
                           wxWindow*       Frame,
                           int             flag,
                           const bool      keep_working_directory,
                           const wxPoint&  Pos )
{
    wxString fullfilename;
    wxString curr_cwd    = wxGetCwd();
    wxString defaultname = FileName;
    wxString defaultpath = Path;
    wxString dotted_Ext = wxT(".") + Ext;

#ifdef __WINDOWS__
    defaultname.Replace( wxT( "/" ), wxT( "\\" ) );
    defaultpath.Replace( wxT( "/" ), wxT( "\\" ) );
#endif

    if( defaultpath.IsEmpty() )
        defaultpath = wxGetCwd();

    wxSetWorkingDirectory( defaultpath );

#if 0 && defined (DEBUG)
    printf( "defaultpath=\"%s\" defaultname=\"%s\" Ext=\"%s\" Mask=\"%s\" flag=%d keep_working_directory=%d\n",
            TO_UTF8( defaultpath ),
            TO_UTF8( defaultname ),
            TO_UTF8( Ext ),
            TO_UTF8( Mask ),
            flag,
            keep_working_directory );
#endif

    fullfilename = wxFileSelector( wxString( Title ),
                                   defaultpath,
                                   defaultname,
                                   dotted_Ext,
                                   Mask,
                                   flag, // open mode wxFD_OPEN, wxFD_SAVE ..
                                   Frame,
                                   Pos.x, Pos.y );

    if( keep_working_directory )
        wxSetWorkingDirectory( curr_cwd );

    return fullfilename;
}


wxString FindKicadFile( const wxString& shortname )
{
    // Test the presence of the file in the directory shortname of
    // the KiCad binary path.
    wxString fullFileName = Pgm().GetExecutablePath() + shortname;

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

    // find binary file from possibilities list:
    //  /usr/local/kicad/linux or c:/kicad/winexe

    // Path list for KiCad binary files
    const static wxChar* possibilities[] = {
#ifdef __WINDOWS__
        wxT( "c:/kicad/bin/" ),
        wxT( "d:/kicad/bin/" ),
        wxT( "c:/Program Files/kicad/bin/" ),
        wxT( "d:/Program Files/kicad/bin/" ),
#else
        wxT( "/usr/bin/" ),
        wxT( "/usr/local/bin/" ),
        wxT( "/usr/local/kicad/bin/" ),
#endif
    };

    for( unsigned i=0;  i<DIM(possibilities);  ++i )
    {
        fullFileName = possibilities[i] + shortname;

        if( wxFileExists( fullFileName ) )
            return fullFileName;
    }

    return shortname;
}


int ExecuteFile( wxWindow* frame, const wxString& ExecFile, const wxString& param,
                 wxProcess *callback )
{
    wxString fullFileName;


    fullFileName = FindKicadFile( ExecFile );

#ifdef __WXMAC__
    if( wxFileExists( fullFileName ) || wxDir::Exists( fullFileName ) )
    {
        return ProcessExecute( Pgm().GetExecutablePath() + wxT( "/" )
                               + ExecFile + wxT( " " )
                               + param, wxEXEC_ASYNC, callback );
    }
    else
    {
        return ProcessExecute( wxT( "/usr/bin/open " ) + param, wxEXEC_ASYNC, callback );
    }
#else
    if( wxFileExists( fullFileName ) )
    {
        if( !param.IsEmpty() )
            fullFileName += wxT( " " ) + param;

        return ProcessExecute( fullFileName, wxEXEC_ASYNC, callback );
    }
#endif
    wxString msg;
    msg.Printf( _( "Command <%s> could not found" ), GetChars( fullFileName ) );
    DisplayError( frame, msg, 20 );
    return -1;
}


wxString KicadDatasPath()
{
    bool     found = false;
    wxString data_path;

    if( Pgm().IsKicadEnvVariableDefined() ) // Path defined by the KICAD environment variable.
    {
        data_path = Pgm().GetKicadEnvVariable();
        found = true;
    }
    else    // Path of executables.
    {
        wxString tmp = Pgm().GetExecutablePath();
#ifdef __WINDOWS__
        tmp.MakeLower();
#endif
        if( tmp.Contains( wxT( "kicad" ) ) )
        {
#ifdef __WINDOWS__
            tmp = Pgm().GetExecutablePath();
#endif
            if( tmp.Last() == '/' )
                tmp.RemoveLast();

            data_path  = tmp.BeforeLast( '/' ); // id cd ../
            data_path += UNIX_STRING_DIR_SEP;

            // Old versions of KiCad use kicad/ as default for data
            // and last versions kicad/share/
            // So we search for kicad/share/ first
            wxString old_path = data_path;
            data_path += wxT( "share/" );

            if( wxDirExists( data_path ) )
            {
                found = true;
            }
            else if( wxDirExists( old_path ) )
            {
                data_path = old_path;
                found = true;
            }
        }
    }

    if( !found )
    {
        // find KiCad from possibilities list:
        //  /usr/local/kicad/ or c:/kicad/

        const static wxChar*  possibilities[] = {
#ifdef __WINDOWS__
            wxT( "c:/kicad/share/" ),
            wxT( "d:/kicad/share/" ),
            wxT( "c:/kicad/" ),
            wxT( "d:/kicad/" ),
            wxT( "c:/Program Files/kicad/share/" ),
            wxT( "d:/Program Files/kicad/share/" ),
            wxT( "c:/Program Files/kicad/" ),
            wxT( "d:/Program Files/kicad/" ),
#else
            wxT( "/usr/share/kicad/" ),
            wxT( "/usr/local/share/kicad/" ),
            wxT( "/usr/local/kicad/share/" ),   // default data path for "universal
                                                // tarballs" and build for a server
                                                // (new)
            wxT( "/usr/local/kicad/" ),         // default data path for "universal
                                                // tarballs" and build for a server
                                                // (old)
#endif
        };

        for( unsigned i=0;  i<DIM(possibilities);  ++i )
        {
            data_path = possibilities[i];

            if( wxDirExists( data_path ) )
            {
                found = true;
                break;
            }
        }
    }

    if( found )
    {
        data_path.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

        if( data_path.Last() != '/' )
            data_path += UNIX_STRING_DIR_SEP;
    }
    else
    {
        data_path.Empty();
    }

    return data_path;
}


bool OpenPDF( const wxString& file )
{
    wxString command;
    wxString filename = file;
    wxString type;
    bool     success = false;

    Pgm().ReadPdfBrowserInfos();

    if( !Pgm().UseSystemPdfBrowser() )    //  Run the preferred PDF Browser
    {
        AddDelimiterString( filename );
        command = Pgm().GetPdfBrowserName() + wxT( " " ) + filename;
    }
    else
    {
        wxFileType* filetype = NULL;
        wxFileType::MessageParameters params( filename, type );
        filetype = wxTheMimeTypesManager->GetFileTypeFromExtension( wxT( "pdf" ) );

        if( filetype )
            success = filetype->GetOpenCommand( &command, params );

        delete filetype;

#ifndef __WINDOWS__

        // Bug ? under linux wxWidgets returns acroread as PDF viewer, even if
        // it does not exist.
        if( command.StartsWith( wxT( "acroread" ) ) ) // Workaround
            success = false;
#endif

        if( success && !command.IsEmpty() )
        {
            success = ProcessExecute( command );

            if( success )
                return success;
        }

        success = false;
        command.clear();

        if( !success )
        {
#if !defined(__WINDOWS__)
            AddDelimiterString( filename );

            // here is a list of PDF viewers candidates
            static const wxChar* tries[] =
            {
                wxT( "/usr/bin/evince" ),
                wxT( "/usr/bin/okular" ),
                wxT( "/usr/bin/gpdf" ),
                wxT( "/usr/bin/konqueror" ),
                wxT( "/usr/bin/kpdf" ),
                wxT( "/usr/bin/xpdf" ),
                wxT( "/usr/bin/open" ),     // BSD and OSX file & dir opener
                wxT( "/usr/bin/xdg-open" ), // Freedesktop file & dir opener
            };

            for( unsigned ii = 0;  ii<DIM(tries);  ii++ )
            {
                if( wxFileExists( tries[ii] ) )
                {
                    command = tries[ii];
                    command += wxT( ' ' );
                    command += filename;
                    break;
                }
            }
#endif
        }
    }

    if( !command.IsEmpty() )
    {
        success = ProcessExecute( command );

        if( !success )
        {
            wxString msg;
            msg.Printf( _( "Problem while running the PDF viewer\nCommand is '%s'" ),
                        GetChars( command ) );
            DisplayError( NULL, msg );
        }
    }
    else
    {
        wxString msg;
        msg.Printf( _( "Unable to find a PDF viewer for <%s>" ), GetChars( filename ) );
        DisplayError( NULL, msg );
        success = false;
    }

    return success;
}


void OpenFile( const wxString& file )
{
    wxString    command;
    wxString    filename = file;

    wxFileName  CurrentFileName( filename );
    wxString    ext, type;

    ext = CurrentFileName.GetExt();
    wxFileType* filetype = wxTheMimeTypesManager->GetFileTypeFromExtension( ext );

    bool        success = false;

    wxFileType::MessageParameters params( filename, type );

    if( filetype )
        success = filetype->GetOpenCommand( &command, params );

    delete filetype;

    if( success && !command.IsEmpty() )
        ProcessExecute( command );
}


wxString QuoteFullPath( wxFileName& fn, wxPathFormat format )
{
    return wxT( "\"" ) + fn.GetFullPath( format ) + wxT( "\"" );
}
