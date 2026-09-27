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

#include <set>
#include <3d_cache/3d_cache.h>
#include <board_design_settings.h>
#include <export_idf.h>
#include <footprint.h>
#include <jobs/job_export_pcb_idf.h>
#include <pcb_field.h>
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

    OptOut( m_filePickerIDF );

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


void DIALOG_EXPORT_IDF3::ApplySettings( const IDF_EXPORT_SETTINGS& aSettings )
{
    m_outputUnitsChoice->SetSelection( aSettings.units == IDF_SETTINGS::UNITS::MILS ? 1 : 0 );

    switch( aSettings.originMode )
    {
    default:
    case IDF_SETTINGS::COORD_ORIGIN::CENTER:  m_rbOriginBoardCenter->SetValue( true ); break;
    case IDF_SETTINGS::COORD_ORIGIN::GRID:    m_rbOriginGrid->SetValue( true );        break;
    case IDF_SETTINGS::COORD_ORIGIN::DRILL:   m_rbOriginDrill->SetValue( true );       break;
    case IDF_SETTINGS::COORD_ORIGIN::USER:    m_rbOriginUser->SetValue( true );        break;
    }

    m_xPos.SetValue( pcbIUScale.mmToIU( aSettings.userOriginX ) );
    m_yPos.SetValue( pcbIUScale.mmToIU( aSettings.userOriginY ) );

    m_cbRemoveDNP->SetValue( !aSettings.includeDNP );
    m_cbRemoveUnspecified->SetValue( !aSettings.includeUnspecified );
    m_cbHeightFromModels->SetValue( aSettings.calculateHeightFromModels );

    if( !m_choicePartNumberField->SetStringSelection( aSettings.partNumberField ) )
        m_choicePartNumberField->SetStringSelection( wxS( "Value" ) );
}


IDF_EXPORT_SETTINGS DIALOG_EXPORT_IDF3::GetSettings() const
{
    IDF_EXPORT_SETTINGS settings;

    settings.units = m_outputUnitsChoice->GetSelection() ? IDF_SETTINGS::UNITS::MILS : IDF_SETTINGS::UNITS::MM;

    if( m_rbOriginBoardCenter->GetValue() )
        settings.originMode = IDF_SETTINGS::COORD_ORIGIN::CENTER;
    else if( m_rbOriginGrid->GetValue() )
        settings.originMode = IDF_SETTINGS::COORD_ORIGIN::GRID;
    else if( m_rbOriginDrill->GetValue() )
        settings.originMode = IDF_SETTINGS::COORD_ORIGIN::DRILL;
    else if( m_rbOriginUser->GetValue() )
        settings.originMode = IDF_SETTINGS::COORD_ORIGIN::USER;

    settings.userOriginX = m_xPos.GetValue() / pcbIUScale.IU_PER_MM;
    settings.userOriginY = m_yPos.GetValue() / pcbIUScale.IU_PER_MM;

    settings.includeDNP = !m_cbRemoveDNP->GetValue();
    settings.includeUnspecified = !m_cbRemoveUnspecified->GetValue();
    settings.calculateHeightFromModels = m_cbHeightFromModels->GetValue();
    settings.partNumberField = m_choicePartNumberField->GetStringSelection();

    return settings;
}


void DIALOG_EXPORT_IDF3::ApplyJobSettings( const JOB_EXPORT_PCB_IDF& aSettings )
{
    SetTitle( aSettings.GetSettingsDialogTitle() );

    IDF_EXPORT_SETTINGS settings;

    switch( aSettings.m_units )
    {
    default:
    case IDF_SETTINGS::UNITS::MM:     settings.units = IDF_SETTINGS::UNITS::MM;     break;
    case IDF_SETTINGS::UNITS::MILS:   settings.units = IDF_SETTINGS::UNITS::MILS;   break;
    }

    switch( aSettings.m_originMode )
    {
    default:
    case IDF_SETTINGS::COORD_ORIGIN::CENTER:  settings.originMode = IDF_SETTINGS::COORD_ORIGIN::CENTER; break;
    case IDF_SETTINGS::COORD_ORIGIN::GRID:    settings.originMode = IDF_SETTINGS::COORD_ORIGIN::GRID;   break;
    case IDF_SETTINGS::COORD_ORIGIN::DRILL:   settings.originMode = IDF_SETTINGS::COORD_ORIGIN::DRILL;  break;
    case IDF_SETTINGS::COORD_ORIGIN::USER:    settings.originMode = IDF_SETTINGS::COORD_ORIGIN::USER;   break;
    }

    // The job stores the origin in m_units; the project stores it in mm
    double iuPerUnit = aSettings.m_units == IDF_SETTINGS::UNITS::MM ? pcbIUScale.IU_PER_MM : pcbIUScale.IU_PER_MILS;

    settings.userOriginX = aSettings.m_userOrigin.x * iuPerUnit * pcbIUScale.MM_PER_IU;
    settings.userOriginY = aSettings.m_userOrigin.y * iuPerUnit * pcbIUScale.MM_PER_IU;
    settings.includeDNP = aSettings.m_includeDNP;
    settings.includeUnspecified = aSettings.m_includeUnspecified;
    settings.calculateHeightFromModels = aSettings.m_calculateHeightFromModels;
    settings.partNumberField = aSettings.m_partNumberField;

    ApplySettings( settings );
}


void DIALOG_EXPORT_IDF3::GetJobSettings( JOB_EXPORT_PCB_IDF& aSettingsOut ) const
{
    IDF_EXPORT_SETTINGS settings = GetSettings();

    aSettingsOut.m_units = settings.units;

    switch( settings.originMode )
    {
    default:
    case IDF_SETTINGS::COORD_ORIGIN::CENTER:
        aSettingsOut.m_originMode = IDF_SETTINGS::COORD_ORIGIN::CENTER;
        break;
    case IDF_SETTINGS::COORD_ORIGIN::GRID:
        aSettingsOut.m_originMode = IDF_SETTINGS::COORD_ORIGIN::GRID;
        break;
    case IDF_SETTINGS::COORD_ORIGIN::DRILL:
        aSettingsOut.m_originMode = IDF_SETTINGS::COORD_ORIGIN::DRILL;
        break;
    case IDF_SETTINGS::COORD_ORIGIN::USER:
        aSettingsOut.m_originMode = IDF_SETTINGS::COORD_ORIGIN::USER;
        break;
    }

    // The project stores the origin in mm; the job stores it in m_units
    double iuPerUnit = aSettingsOut.m_units == IDF_SETTINGS::UNITS::MM ? pcbIUScale.IU_PER_MM : pcbIUScale.IU_PER_MILS;

    aSettingsOut.m_userOrigin.x = settings.userOriginX * iuPerUnit;
    aSettingsOut.m_userOrigin.y = settings.userOriginY * iuPerUnit;
    aSettingsOut.m_includeDNP = settings.includeDNP;
    aSettingsOut.m_includeUnspecified = settings.includeUnspecified;
    aSettingsOut.m_calculateHeightFromModels = settings.calculateHeightFromModels;
    aSettingsOut.m_partNumberField = settings.partNumberField;
}


bool DIALOG_EXPORT_IDF3::TransferDataToWindow()
{
    m_tcLog->Clear();
    std::set<wxString> fieldNames;

    for( FOOTPRINT* fp : m_parent->GetBoard()->Footprints() )
    {
        for( PCB_FIELD* field : fp->GetFields() )
        {
            wxCHECK2( field, continue );

            if( field->IsReference() )
                continue;

            fieldNames.insert( field->GetUntranslatedName() );
        }
    }

    m_choicePartNumberField->Append( std::vector<wxString>( fieldNames.begin(), fieldNames.end() ) );

    if( m_job )
        ApplyJobSettings( *m_job );
    else
        ApplySettings( m_parent->Prj().GetProjectFile().m_IdfExportSettings );

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
    {
        wxMessageBox( wxString::Format( _( "Failed to create file '%s'." ), m_filePickerIDF->GetPath() ) );
        return;
    }

    GetJobSettings( job );

    IDF_EXPORT_SETTINGS& projectSettings = m_parent->Prj().GetProjectFile().m_IdfExportSettings;

    projectSettings.units = job.m_units;

    switch( job.m_originMode )
    {
    default:
    case IDF_SETTINGS::COORD_ORIGIN::CENTER:  projectSettings.originMode = IDF_SETTINGS::COORD_ORIGIN::CENTER; break;
    case IDF_SETTINGS::COORD_ORIGIN::GRID:    projectSettings.originMode = IDF_SETTINGS::COORD_ORIGIN::GRID;   break;
    case IDF_SETTINGS::COORD_ORIGIN::DRILL:   projectSettings.originMode = IDF_SETTINGS::COORD_ORIGIN::DRILL;  break;
    case IDF_SETTINGS::COORD_ORIGIN::USER:    projectSettings.originMode = IDF_SETTINGS::COORD_ORIGIN::USER;   break;
    }

    // The job stores the origin in m_units; the project stores it in mm
    double iuPerUnit = job.m_units == IDF_SETTINGS::UNITS::MM ? pcbIUScale.IU_PER_MM : pcbIUScale.IU_PER_MILS;

    projectSettings.userOriginX = job.m_userOrigin.x * iuPerUnit * pcbIUScale.MM_PER_IU;
    projectSettings.userOriginY = job.m_userOrigin.y * iuPerUnit * pcbIUScale.MM_PER_IU;
    projectSettings.includeDNP = job.m_includeDNP;
    projectSettings.includeUnspecified = job.m_includeUnspecified;
    projectSettings.calculateHeightFromModels = job.m_calculateHeightFromModels;
    projectSettings.partNumberField = job.m_partNumberField;

    m_parent->SetLastPath( LAST_PATH_IDF, m_filePickerIDF->GetPath() );

    if( resetJob )
        m_job = nullptr;
}


int BOARD_EDITOR_CONTROL::ExportIDF( const TOOL_EVENT& aEvent )
{
    DIALOG_EXPORT_IDF3 dlg( m_frame );
    dlg.ShowModal();

    return 0;
}
