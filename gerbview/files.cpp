/**
 * @file gerbview/files.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <common.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gestfich.h>

#include <gerbview.h>
#include <gerbview_id.h>
#include <class_gerbview_layer_widget.h>
#include <wildcards_and_files_ext.h>


void GERBVIEW_FRAME::OnGbrFileHistory( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Gerber files" ) );

    if( !fn.IsEmpty() )
    {
        Erase_Current_Layer( false );
        LoadGerberFiles( fn );
    }
}


void GERBVIEW_FRAME::OnDrlFileHistory( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Drill files" ), &m_drillFileHistory );

    if( !fn.IsEmpty() )
    {
        Erase_Current_Layer( false );
        LoadExcellonFiles( fn );
    }
}


/* File commands. */
void GERBVIEW_FRAME::Files_io( wxCommandEvent& event )
{
    int        id = event.GetId();

    switch( id )
    {
    case wxID_FILE:
        Erase_Current_Layer( false );
        LoadGerberFiles( wxEmptyString );
        break;

    case ID_GERBVIEW_ERASE_ALL:
        Clear_Pcb( true );
        Zoom_Automatique( false );
        m_canvas->Refresh();
        ClearMsgPanel();
        break;

    case ID_GERBVIEW_LOAD_DRILL_FILE:
        LoadExcellonFiles( wxEmptyString );
        m_canvas->Refresh();
        break;

    default:
        wxFAIL_MSG( wxT( "File_io: unexpected command id" ) );
        break;
    }
}


bool GERBVIEW_FRAME::LoadGerberFiles( const wxString& aFullFileName )
{
    wxString   filetypes;
    wxArrayString filenamesList;
    wxFileName filename = aFullFileName;
    wxString currentPath;

    if( !filename.IsOk() )
    {
        /* Standard gerber filetypes
         * (See http://en.wikipedia.org/wiki/Gerber_File)
         * the .pho extension is the default used in Pcbnew
         * However there are a lot of other extensions used for gerber files
         * Because the first letter is usually g, we accept g* as extension
         * (Mainly internal copper layers do not have specific extention,
         *  and filenames are like *.g1, *.g2 *.gb1 ...).
         */
        filetypes = _( "Gerber files (.g* .lgr .pho)" );
        filetypes << wxT("|");
        filetypes += wxT("*.g*;*.G*;*.lgr;*.LGR;*.pho;*.PHO" );
        filetypes << wxT("|");

        /* Special gerber filetypes */
        filetypes += _( "Top layer (*.GTL)|*.GTL;*.gtl|" );
        filetypes += _( "Bottom layer (*.GBL)|*.GBL;*.gbl|" );
        filetypes += _( "Bottom solder resist (*.GBS)|*.GBS;*.gbs|" );
        filetypes += _( "Top solder resist (*.GTS)|*.GTS;*.gts|" );
        filetypes += _( "Bottom overlay (*.GBO)|*.GBO;*.gbo|" );
        filetypes += _( "Top overlay (*.GTO)|*.GTO;*.gto|" );
        filetypes += _( "Bottom paste (*.GBP)|*.GBP;*.gbp|" );
        filetypes += _( "Top paste (*.GTP)|*.GTP;*.gtp|" );
        filetypes += _( "Keep-out layer (*.GKO)|*.GKO;*.gko|" );
        filetypes += _( "Mechanical layers (*.GMx)|*.GM1;*.gm1;*.GM2;*.gm2;*.GM3;*.gm3|" );
        filetypes += _( "Top Pad Master (*.GPT)|*.GPT;*.gpt|" );
        filetypes += _( "Bottom Pad Master (*.GPB)|*.GPB;*.gpb|" );

        /* All filetypes */
        filetypes += AllFilesWildcard;

        /* Use the current working directory if the file name path does not exist. */
        if( filename.DirExists() )
            currentPath = filename.GetPath();
        else
            currentPath = wxGetCwd();

        wxFileDialog dlg( this,
                          _( "Open Gerber File" ),
                          currentPath,
                          filename.GetFullName(),
                          filetypes,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        dlg.GetPaths( filenamesList );
        currentPath = wxGetCwd();
    }
    else
    {
        wxFileName filename = aFullFileName;
        filenamesList.Add( aFullFileName );
        currentPath = filename.GetPath();
    }

    // Read gerber files: each file is loaded on a new GerbView layer
    LAYER_NUM layer = getActiveLayer();

    for( unsigned ii = 0; ii < filenamesList.GetCount(); ii++ )
    {
        wxFileName filename = filenamesList[ii];

        if( !filename.IsAbsolute() )
            filename.SetPath( currentPath );

        m_lastFileName = filename.GetFullPath();

        setActiveLayer( layer, false );

        if( Read_GERBER_File( filename.GetFullPath(), filename.GetFullPath() ) )
        {
            UpdateFileHistory( m_lastFileName );

            layer = getNextAvailableLayer( layer );

            if( layer == NO_AVAILABLE_LAYERS )
            {
                wxString msg = wxT( "No more empty available layers.\n"
                                    "The remaining gerber files will not be loaded." );
                wxMessageBox( msg );
                break;
            }

            setActiveLayer( layer, false );
        }
    }

    Zoom_Automatique( false );

    // Synchronize layers tools with actual active layer:
    setActiveLayer( getActiveLayer() );
    m_LayersManager->UpdateLayerIcons();
    syncLayerBox();
    return true;
}

bool GERBVIEW_FRAME::LoadExcellonFiles( const wxString& aFullFileName )
{
    wxString   filetypes;
    wxArrayString filenamesList;
    wxFileName filename = aFullFileName;
    wxString currentPath;

    if( !filename.IsOk() )
    {
        filetypes = wxGetTranslation( DrillFileWildcard );
        filetypes << wxT("|");
        /* All filetypes */
        filetypes += wxGetTranslation( AllFilesWildcard );

        /* Use the current working directory if the file name path does not exist. */
        if( filename.DirExists() )
            currentPath = filename.GetPath();
        else
            currentPath = wxGetCwd();

        wxFileDialog dlg( this,
                          _( "Open Drill File" ),
                          currentPath,
                          filename.GetFullName(),
                          filetypes,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        dlg.GetPaths( filenamesList );
        currentPath = wxGetCwd();
    }
    else
    {
        wxFileName filename = aFullFileName;
        filenamesList.Add( aFullFileName );
        currentPath = filename.GetPath();
    }

    // Read gerber files: each file is loaded on a new GerbView layer
    LAYER_NUM layer = getActiveLayer();

    for( unsigned ii = 0; ii < filenamesList.GetCount(); ii++ )
    {
        wxFileName filename = filenamesList[ii];

        if( !filename.IsAbsolute() )
            filename.SetPath( currentPath );

        m_lastFileName = filename.GetFullPath();

        setActiveLayer( layer, false );

        if( Read_EXCELLON_File( filename.GetFullPath() ) )
        {
            // Update the list of recentdrill files.
            UpdateFileHistory( filename.GetFullPath(),  &m_drillFileHistory );

            layer = getNextAvailableLayer( layer );

            if( layer == NO_AVAILABLE_LAYERS )
            {
                wxString msg = wxT( "No more empty available layers.\n"
                                    "The remaining gerber files will not be loaded." );
                wxMessageBox( msg );
                break;
            }

            setActiveLayer( layer, false );
        }
    }

    Zoom_Automatique( false );

    // Synchronize layers tools with actual active layer:
    setActiveLayer( getActiveLayer() );
    m_LayersManager->UpdateLayerIcons();
    syncLayerBox();

    return true;
}
