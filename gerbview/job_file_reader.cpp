/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2019 Jean-Pierre Charras  jp.charras at wanadoo.fr
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
 * @file job_file_reader.cpp
 */

#include <fctsys.h>
#include <wx/filename.h>
#include <wildcards_and_files_ext.h>
#include <gerbview.h>
#include <richio.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <gerbview_frame.h>
#include <reporter.h>
#include <gbr_metadata.h>
#include <html_messagebox.h>
#include <view/view.h>
#include "json11.hpp"   // A light JSON parser

/**
 * this class read and parse a Gerber job file to extract useful info
 * for GerbView
 *
 * In a gerber job file, old (deprecated) format, data lines start by
 * %TF.     (usual Gerber X2 info)
 * %TJ.B.   (board info)
 * %TJ.D.   (design info)
 * %TJ.L.   (layers info)
 * some others are not yet handled by Kicad
 * M02*     is the last line

 * In a gerber job file, JSON format, first lines are
 *   {
 *    "Header":
 * and the block ( a JSON array) containing the filename of files to load is
 *    "FilesAttributes":
 *    [
 *      {
 *        "Path":  "interf_u-Composant.gbr",
 *        "FileFunction":  "Copper,L1,Top",
 *        "FilePolarity":  "Positive"
 *      },
 *      {
 *        "Path":  "interf_u-In1.Cu.gbr",
 *        "FileFunction":  "Copper,L2,Inr",
 *        "FilePolarity":  "Positive"
 *      },
 *    ],
 */

class GERBER_JOBFILE_READER
{
public:
    GERBER_JOBFILE_READER( const wxString& aFileName, REPORTER* aReporter )
    {
        m_filename = aFileName;
        m_reporter = aReporter;
    }

    ~GERBER_JOBFILE_READER() {}

    bool ReadGerberJobFile();       /// read a .gbrjob file
    wxArrayString& GetGerberFiles() { return m_GerberFiles; }

private:
    REPORTER* m_reporter;
    wxFileName m_filename;
    wxArrayString m_GerberFiles;    // List of gerber files in job

    // Convert a JSON string, that uses escaped sequence of 4 hexdecimal digits
    // to encode unicode chars when not ASCII7 codes
    // json11 converts this sequence to UTF8 string
    wxString formatStringFromJSON( const std::string& name );
};


bool GERBER_JOBFILE_READER::ReadGerberJobFile()
{
    // Read the gerber file */
   FILE* jobFile = wxFopen( m_filename.GetFullPath(), wxT( "rt" ) );

    if( jobFile == nullptr )
        return false;

    LOCALE_IO toggleIo;

    FILE_LINE_READER jobfileReader( jobFile, m_filename.GetFullPath() );  // Will close jobFile

    wxString msg;
    wxString data;

    // detect the file format: old (deprecated) gerber format of official JSON format
    bool json_format = false;

    char* line = jobfileReader.ReadLine();

    if( !line )     // end of file
        return false;

    data = line;

    if( data.Contains( "{" ) )
        json_format = true;

    if( json_format )
    {
        while( ( line = jobfileReader.ReadLine() ) )
            data << '\n' << line;

        std::string err;
        json11::Json json_parser = json11::Json::parse( TO_UTF8( data ), err );

        if( !err.empty() )
            return false;

        for( auto& entry : json_parser["FilesAttributes"].array_items() )
        {
            std::string name = entry["Path"].string_value();
            m_GerberFiles.Add( formatStringFromJSON( name ) );
        }
    }
    else
    {
        if( m_reporter )
            m_reporter->ReportTail( _( "This job file uses an outdated format. Please, recreate it." ),
                                REPORTER::RPT_WARNING );

        return false;
    }

    return true;
}


wxString GERBER_JOBFILE_READER::formatStringFromJSON( const std::string& name )
{
    // Convert a JSON string, that uses a escaped sequence of 4 hexdecimal digits
    // to encode unicode chars
    // Our json11 library returns in this case a UTF8 sequence. Just convert it to
    // a wxString.
    wxString wstr = FROM_UTF8( name.c_str() );
    return wstr;
}



bool GERBVIEW_FRAME::LoadGerberJobFile( const wxString& aFullFileName )
{
    wxFileName filename = aFullFileName;
    wxString currentPath;
    bool success = true;

    if( !filename.IsOk() )
    {
        // Use the current working directory if the file name path does not exist.
        if( filename.DirExists() )
            currentPath = filename.GetPath();
        else
            currentPath = m_mruPath;

        wxFileDialog dlg( this, _( "Open Gerber Job File" ),
                          currentPath,
                          filename.GetFullName(),
                          GerberJobFileWildcard(),
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        filename = dlg.GetPath();
        currentPath = filename.GetPath();
        m_mruPath = currentPath;
    }
    else
    {
        currentPath = filename.GetPath();
        m_mruPath = currentPath;
    }

    wxString msg;
    WX_STRING_REPORTER reporter( &msg );

    if( filename.IsOk() )
    {
        GERBER_JOBFILE_READER gbjReader( filename.GetFullPath(), &reporter );

        if( gbjReader.ReadGerberJobFile() )
        {
            // Update the list of recent drill files.
            UpdateFileHistory( filename.GetFullPath(), &m_jobFileHistory );

            Clear_DrawLayers( false );
            ClearMsgPanel();

            wxArrayString& gbrfiles = gbjReader.GetGerberFiles();

            success = loadListOfGerberAndDrillFiles( currentPath, gbrfiles );
        }
    }

    SortLayersByX2Attributes();

    SetActiveLayer( 0 );

    if( !msg.IsEmpty() )
    {
        wxSafeYield();  // Allows slice of time to redraw the screen
                        // to refresh widgets, before displaying messages
        HTML_MESSAGE_BOX mbox( this, _( "Messages" ) );
        mbox.ListSet( msg );
        mbox.ShowModal();
    }

    return success;
}
