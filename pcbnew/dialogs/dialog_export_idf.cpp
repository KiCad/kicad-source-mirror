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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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
    wxFileName fn = m_filePickerIDF->GetPath();

    if( fn.FileExists() )
    {
        KIDIALOG dlg( this, wxString::Format( _( "File %s already exists." ), fn.GetPath() ),
                      _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
        dlg.SetOKLabel( _( "Overwrite" ) );
        dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

        return ( dlg.ShowModal() == wxID_OK );
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

