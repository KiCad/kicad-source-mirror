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

#include <3d_cache/3d_cache.h>
#include <board_design_settings.h>
#include <export_idf.h>
#include <jobs/job_export_pcb_idf.h>
#include <string_utils.h>
#include <tools/board_editor_control.h>
#include <project_pcb.h>
#include <project/project_file.h> // LAST_PATH_TYPE
#include <kidialog.h>
#include <reporter.h>


DIALOG_EXPORT_IDF3::DIALOG_EXPORT_IDF3( PCB_EDIT_FRAME* aEditFrame ) :
        DIALOG_EXPORT_IDF3_BASE( aEditFrame ),
        m_xPos( aEditFrame, m_xLabel, m_IDF_Xref, m_xUnits ),
        m_yPos( aEditFrame, m_yLabel, m_IDF_Yref, m_yUnits ),
        m_parent( aEditFrame )
{
    setupDialog();
}


DIALOG_EXPORT_IDF3::DIALOG_EXPORT_IDF3( JOB_EXPORT_PCB_IDF* aJob, PCB_EDIT_FRAME* aEditFrame, wxWindow* aParent ) :
        DIALOG_EXPORT_IDF3_BASE( aParent ),
        m_xPos( aEditFrame, m_xLabel, m_IDF_Xref, m_xUnits ),
        m_yPos( aEditFrame, m_yLabel, m_IDF_Yref, m_yUnits ),
        m_parent( aEditFrame ),
        m_job( aJob )
{
    setupDialog();
}


void DIALOG_EXPORT_IDF3::setupDialog()
{
    m_hash_key = TO_UTF8( GetTitle() );

    m_rbOriginUser->Bind( wxEVT_RADIOBUTTON, &DIALOG_EXPORT_IDF3::onRadioButtonsChanged, this );
    m_rbOriginDrill->Bind( wxEVT_RADIOBUTTON, &DIALOG_EXPORT_IDF3::onRadioButtonsChanged, this );
    m_rbOriginGrid->Bind( wxEVT_RADIOBUTTON, &DIALOG_EXPORT_IDF3::onRadioButtonsChanged, this );
    m_rbOriginBoardCenter->Bind( wxEVT_RADIOBUTTON, &DIALOG_EXPORT_IDF3::onRadioButtonsChanged, this );

    if( m_job )
        SetupStandardButtons();
    else
        SetupStandardButtons( { { wxID_OK, _( "Export" ) }, { wxID_CANCEL, _( "Close" ) } } );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_EXPORT_IDF3::onRadioButtonsChanged( wxCommandEvent& event )
{
    m_xPos.Enable( m_rbOriginUser->GetValue() );
    m_yPos.Enable( m_rbOriginUser->GetValue() );

    event.Skip();
}


void DIALOG_EXPORT_IDF3::OnOKButton( wxCommandEvent& event )
{
    if( m_job )
    {
        GetJobSettings( *m_job );
        EndModal( wxID_OK );
    }

    doExport();
}


void DIALOG_EXPORT_IDF3::ApplyJobSettings( const JOB_EXPORT_PCB_IDF& aSettings )
{
    SetTitle( aSettings.GetSettingsDialogTitle() );

    switch( aSettings.m_units )
    {
    default:
    case JOB_EXPORT_PCB_IDF::UNITS::MM:     m_outputUnitsChoice->SetSelection( 0 ); break;
    case JOB_EXPORT_PCB_IDF::UNITS::MILS:   m_outputUnitsChoice->SetSelection( 1 ); break;
    }

    switch( aSettings.m_originMode )
    {
    default:
    case JOB_EXPORT_PCB_IDF::COORD_ORIGIN::CENTER:  m_rbOriginBoardCenter->SetValue( true ); break;
    case JOB_EXPORT_PCB_IDF::COORD_ORIGIN::GRID:    m_rbOriginGrid->SetValue( true );        break;
    case JOB_EXPORT_PCB_IDF::COORD_ORIGIN::DRILL:   m_rbOriginDrill->SetValue( true );       break;
    case JOB_EXPORT_PCB_IDF::COORD_ORIGIN::USER:    m_rbOriginUser->SetValue( true );        break;
    }

    m_xPos.SetValue( aSettings.m_userOrigin.x );
    m_yPos.SetValue( aSettings.m_userOrigin.y );

    m_cbRemoveDNP->SetValue( !aSettings.m_includeDNP );
    m_cbRemoveUnspecified->SetValue( !aSettings.m_includeUnspecified );
    m_cbHeightFromModels->SetValue( aSettings.m_calculateHeightFromModels );
}


void DIALOG_EXPORT_IDF3::GetJobSettings( JOB_EXPORT_PCB_IDF& aSettingsOut ) const
{
    aSettingsOut.m_units = m_outputUnitsChoice->GetSelection() ? JOB_EXPORT_PCB_IDF::UNITS::MILS
                                                               : JOB_EXPORT_PCB_IDF::UNITS::MM;

    if( m_rbOriginBoardCenter->GetValue() )
        aSettingsOut.m_originMode = JOB_EXPORT_PCB_IDF::COORD_ORIGIN::CENTER;
    else if( m_rbOriginGrid->GetValue() )
        aSettingsOut.m_originMode = JOB_EXPORT_PCB_IDF::COORD_ORIGIN::GRID;
    else if( m_rbOriginDrill->GetValue() )
        aSettingsOut.m_originMode = JOB_EXPORT_PCB_IDF::COORD_ORIGIN::DRILL;
    else if( m_rbOriginUser->GetValue() )
        aSettingsOut.m_originMode = JOB_EXPORT_PCB_IDF::COORD_ORIGIN::USER;

    aSettingsOut.m_userOrigin.x = m_xPos.GetValue();
    aSettingsOut.m_userOrigin.y = m_yPos.GetValue();

    aSettingsOut.m_includeDNP = !m_cbRemoveDNP->GetValue();
    aSettingsOut.m_includeUnspecified = !m_cbRemoveUnspecified->GetValue();
    aSettingsOut.m_calculateHeightFromModels = m_cbHeightFromModels->GetValue();
}


bool DIALOG_EXPORT_IDF3::TransferDataToWindow()
{
    m_tcLog->Clear();

    if( m_job )
        ApplyJobSettings( *m_job );

    wxString path = m_parent->GetLastPath( LAST_PATH_IDF );

    if( m_job )
        path = m_job->GetConfiguredOutputPath();

    if( path.IsEmpty() )
    {
        wxFileName brdFile = m_parent->GetBoard()->GetFileName();
        brdFile.SetExt( FILEEXT::IdfV3BoardFileExtension );
        path = brdFile.GetFullPath();
    }

    m_filePickerIDF->SetPath( path );

    wxCommandEvent dummy;
    onRadioButtonsChanged( dummy );

    return true;
}


bool DIALOG_EXPORT_IDF3::TransferDataFromWindow()
{
    if( m_job )
        GetJobSettings( *m_job );

    return true;
}


bool DIALOG_EXPORT_IDF3::checkFilenames()
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


void DIALOG_EXPORT_IDF3::doExport()
{
    if( !checkFilenames() )
        return;

    m_tcLog->Clear();
    WX_TEXT_CTRL_REPORTER reporter( m_tcLog );

    FILENAME_RESOLVER* resolver = PROJECT_PCB::Get3DCacheManager( &m_parent->Prj() )->GetResolver();

    bool resetJob = false;
    JOB_EXPORT_PCB_IDF job;

    if( !m_job )
    {
        GetJobSettings( job );
        m_job = &job;
        resetJob = true;
    }

    IDF_EXPORTER exporter( m_parent->GetBoard(), resolver, &job, &reporter );

    wxBusyCursor dummy;

    if( !exporter.Export( m_filePickerIDF->GetPath() ) )
        wxMessageBox( wxString::Format( _( "Failed to create file '%s'." ), m_filePickerIDF->GetPath() ) );

    if( resetJob )
        m_job = nullptr;
}


wxString DIALOG_EXPORT_IDF3::GetFilePath() const
{
    return m_filePickerIDF->GetPath();
}


int BOARD_EDITOR_CONTROL::ExportIDF( const TOOL_EVENT& aEvent )
{
    DIALOG_EXPORT_IDF3 dlg( m_frame );

    if ( dlg.ShowModal() != wxID_OK )
        return 0;

    m_frame->SetLastPath( LAST_PATH_IDF, dlg.GetFilePath() );

    return 0;
}
