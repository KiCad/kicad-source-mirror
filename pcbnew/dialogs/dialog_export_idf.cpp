/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015  Cirilo Bernardo
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

#include <pcb_edit_frame.h>
#include <board.h>
#include <widgets/text_ctrl_eval.h>
#include <dialog_export_idf.h>
#include <pcbnew_settings.h>
#include <tools/board_editor_control.h>
#include <project/project_file.h> // LAST_PATH_TYPE
#include <kidialog.h>


DIALOG_EXPORT_IDF3::DIALOG_EXPORT_IDF3( PCB_EDIT_FRAME* aEditFrame ) :
        DIALOG_EXPORT_IDF3_BASE( aEditFrame ),
        m_xPos( aEditFrame, m_xLabel, m_IDF_Xref, m_xUnits ),
        m_yPos( aEditFrame, m_yLabel, m_IDF_Yref, m_yUnits )
{
    SetFocus();

    m_cbSetBoardReferencePoint->Bind( wxEVT_CHECKBOX, &DIALOG_EXPORT_IDF3::OnBoardReferencePointChecked, this );

    SetupStandardButtons();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_EXPORT_IDF3::OnBoardReferencePointChecked( wxCommandEvent& event )
{
    m_xPos.Enable( m_cbSetBoardReferencePoint->GetValue() );
    m_yPos.Enable( m_cbSetBoardReferencePoint->GetValue() );

    event.Skip();
}


bool DIALOG_EXPORT_IDF3::TransferDataToWindow()
{
    wxCommandEvent dummy;
    OnBoardReferencePointChecked( dummy );

    return true;
}


bool DIALOG_EXPORT_IDF3::TransferDataFromWindow()
{
    wxFileName brdFile( m_filePickerIDF->GetPath() );
    brdFile.SetExt( wxT( "emn" ) );

    wxFileName libFile( m_filePickerIDF->GetPath() );
    libFile.SetExt( wxT( "emp" ) );

    wxArrayString existing;

    if( brdFile.FileExists() )
        existing.push_back( brdFile.GetFullPath() );

    if( libFile.FileExists() )
        existing.push_back( libFile.GetFullPath() );

    if( !existing.empty() )
    {
        wxString msg;

        if( existing.size() == 1 )
            msg = wxString::Format( _( "File %s already exists." ), existing[0] );
        else
            msg = wxString::Format( _( "Files %s and %s already exist." ), existing[0], existing[1] );

        KIDIALOG dlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
        dlg.SetOKLabel( _( "Overwrite" ) );
        dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

        if( dlg.ShowModal() != wxID_OK )
            return false;
    }

    for( const wxFileName& fn : { brdFile, libFile } )
    {
        if( fn.FileExists() && !fn.IsFileWritable() )
        {
            wxMessageBox( wxString::Format( _( "Insufficient permissions to write file '%s'." ),
                                            fn.GetFullPath() ),
                          _( "IDF Export" ), wxOK | wxICON_ERROR );

            return false;
        }
    }

    return true;
}


int BOARD_EDITOR_CONTROL::ExportIDF( const TOOL_EVENT& aEvent )
{
    BOARD* board = m_frame->GetBoard();

    // Build default output file name
    wxString path = m_frame->GetLastPath( LAST_PATH_IDF );

    if( path.IsEmpty() )
    {
        wxFileName brdFile = board->GetFileName();
        brdFile.SetExt( wxT( "emn" ) );
        path = brdFile.GetFullPath();
    }

    DIALOG_EXPORT_IDF3 dlg( m_frame );
    dlg.FilePicker()->SetPath( path );

    if ( dlg.ShowModal() != wxID_OK )
        return 0;

    double aXRef;
    double aYRef;

    if( dlg.GetSetBoardReferencePoint() )
    {
        aXRef = dlg.GetXRefMM();
        aYRef = dlg.GetYRefMM();
    }
    else
    {
        BOX2I bbox = board->GetBoardEdgesBoundingBox();
        aXRef = bbox.Centre().x * pcbIUScale.MM_PER_IU;
        aYRef = bbox.Centre().y * pcbIUScale.MM_PER_IU;
    }

    wxString fullFilename = dlg.FilePicker()->GetPath();
    m_frame->SetLastPath( LAST_PATH_IDF, fullFilename );

    wxBusyCursor dummy;

    if( !m_frame->Export_IDF3( board, fullFilename, dlg.GetThouOption(), aXRef, aYRef,
                               !dlg.GetNoUnspecifiedOption(), !dlg.GetNoDNPOption() ) )
    {
        wxMessageBox( wxString::Format( _( "Failed to create file '%s'." ), fullFilename ) );
    }

    return 0;
}

