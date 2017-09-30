/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2017 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <gerbview.h>
#include <richio.h>
#include <class_drawpanel.h>
#include <class_gerber_file_image.h>
#include <class_gerber_file_image_list.h>
#include <gerbview_frame.h>
#include <reporter.h>
#include <plot_auxiliary_data.h>
#include <html_messagebox.h>


/**
 * this class read and parse a Gerber job file to extract useful info
 * for GerbView
 *
 * In a gerber job file, data lines start by
 * %TF.     (usual Gerber X2 info)
 * %TJ.B.   (board info)
 * %TJ.D.   (design info)
 * %TJ.L.   (layers info)
 * some others are not yet handled by Kicad
 * M02*     is the last line
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

    bool ReadGerberJobFile();       /// read the .gbj file
    wxArrayString& GetGerberFiles() { return m_GerberFiles; }

private:
    /** parse a string starting by "%TJ.L" (layer gerber filename)
     * and add the filename in m_GerberFiles
     */
    bool parseTJLayerString( wxString& aText );

private:
    REPORTER* m_reporter;
    wxFileName m_filename;
    wxArrayString m_GerberFiles;    // List of gerber files in job
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

    while( true )
    {
        char* line = jobfileReader.ReadLine();

        if( !line )     // end of file
            break;

        wxString text( line );
        text.Trim( true );
        text.Trim( false );

        // Search for lines starting by '%', others are not usefull
        if( text.StartsWith( "%TJ.L.", &data ) )
        {
            parseTJLayerString( data );
            continue;
        }

        if( text.StartsWith( "M02" ) )  // End of file
            break;
    }

    return true;
}


bool GERBER_JOBFILE_READER::parseTJLayerString( wxString& aText )
{
    // Parse a line like:
    // %TJ.L."Copper,L1,Top",Positive,kit-dev-coldfire-xilinx_5213-Top_layer.gbr*%
    // and extract the .gbr filename

    // The filename is between the last comma in string and the '*' char
    // the filename cannot contain itself a comma:
    // this is a not allowed char, that is transcoded in hexa sequence
    // if found in filename
    wxString name = aText.AfterLast( ',' ).BeforeFirst( '*' );
    m_GerberFiles.Add( FormatStringFromGerber( name ) );

    return true;
}


bool GERBVIEW_FRAME::LoadGerberJobFile( const wxString& aFullFileName )
{
#define jobFileWildcard  _( "Gerber job file (*.gbrjob)|*.gbrjob;.gbrjob" )
    wxFileName filename = aFullFileName;
    wxString currentPath;

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
                          jobFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        filename = dlg.GetPath();
        currentPath = wxGetCwd();
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

            wxFileName gbr_fn = filename;
            bool read_ok;
            int layer = 0;
            SetActiveLayer( layer, false );

            for( unsigned ii = 0; ii < gbrfiles.GetCount(); ii++ )
            {
                gbr_fn.SetFullName( gbrfiles[ii] );

                if( gbr_fn.FileExists() )
                {
                    //LoadGerberFiles( gbr_fn.GetFullPath() );
                    read_ok = Read_GERBER_File( gbr_fn.GetFullPath() );

                    if( read_ok )
                    {
                        layer = getNextAvailableLayer( layer );
                        SetActiveLayer( layer, false );
                    }
                }
                else
                    read_ok = false;

                if( !read_ok )
                {
                    wxString err;
                    err.Printf( _( "Can't load Gerber file:<br><i>%s</i><br>" ), gbr_fn.GetFullPath() );
                    reporter.Report( err, REPORTER::RPT_WARNING );
                }
            }

            GetImagesList()->SortImagesByZOrder();
            ReFillLayerWidget();
            syncLayerBox( true );
            GetCanvas()->Refresh();
        }
    }

    Zoom_Automatique( false );

    if( !msg.IsEmpty() )
    {
        wxSafeYield();  // Allows slice of time to redraw the screen
                        // to refresh widgets, before displaying messages
        HTML_MESSAGE_BOX mbox( this, _( "Messages" ) );
        mbox.ListSet( msg );
        mbox.ShowModal();
    }

    return true;
}


