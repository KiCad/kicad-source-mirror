/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 Krishna Swaroop <krishna.swaroop@pixxel.co.in>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/dialog_board_stackup_job.h>
#include <map>
#include <i18n_utility.h>
#include <wildcards_and_files_ext.h>
#include <wx/filename.h>


static std::map<JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT, wxString> stackupOutputFormatMap = {
    { JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT::CSV, _HKI( "CSV" ) },
    { JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT::JSON, _HKI( "JSON" ) }
};


DIALOG_BOARD_STACKUP_JOB::DIALOG_BOARD_STACKUP_JOB( wxWindow* aParent, JOB_EXPORT_PCB_STACKUP* aJob ) :
        DIALOG_BOARD_STACKUP_JOB_BASE( aParent, wxID_ANY ),
        m_job( aJob )
{
    for( const auto& [format, name] : stackupOutputFormatMap )
        m_choiceFormat->Append( wxGetTranslation( name ) );

    if( aJob )
        SetTitle( aJob->GetSettingsDialogTitle() );

    SetupStandardButtons();

    finishDialogSettings();
}


JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT DIALOG_BOARD_STACKUP_JOB::getSelectedFormat()
{
    int selIndx = m_choiceFormat->GetSelection();

    if( selIndx < 0 )
        return JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT::CSV;

    auto it = stackupOutputFormatMap.begin();
    std::advance( it, selIndx );
    return it->first;
}


void DIALOG_BOARD_STACKUP_JOB::setSelectedFormat( JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT aFormat )
{
    auto it = stackupOutputFormatMap.find( aFormat );

    if( it != stackupOutputFormatMap.end() )
        m_choiceFormat->SetSelection( std::distance( stackupOutputFormatMap.begin(), it ) );
}


void DIALOG_BOARD_STACKUP_JOB::OnFormatChoice( wxCommandEvent& aEvent )
{
    updateCsvOnlyControls();

    if( !m_textCtrlOutputPath->GetValue().IsEmpty() )
    {
        wxFileName fn( m_textCtrlOutputPath->GetValue() );

        switch( getSelectedFormat() )
        {
        case JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT::CSV: fn.SetExt( FILEEXT::CsvFileExtension ); break;
        case JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT::JSON: fn.SetExt( FILEEXT::JsonFileExtension ); break;
        }

        m_textCtrlOutputPath->SetValue( fn.GetFullPath() );
    }
}


void DIALOG_BOARD_STACKUP_JOB::updateCsvOnlyControls()
{
    const bool csv = getSelectedFormat() == JOB_EXPORT_PCB_STACKUP::OUTPUT_FORMAT::CSV;

    m_labelUnits->Enable( csv );
    m_choiceUnits->Enable( csv );
    m_sbSizerFields->GetStaticBox()->Enable( csv );
    m_cbThickness->Enable( csv );
    m_cbMaterial->Enable( csv );
    m_cbColor->Enable( csv );
    m_cbEpsilonR->Enable( csv );
    m_cbLossTangent->Enable( csv );
    m_cbFinish->Enable( csv );
    m_cbBoardOptions->Enable( csv );
}


bool DIALOG_BOARD_STACKUP_JOB::TransferDataToWindow()
{
    m_textCtrlOutputPath->SetValue( m_job->GetConfiguredOutputPath() );
    setSelectedFormat( m_job->m_format );
    m_choiceUnits->SetSelection( m_job->m_units == JOB_EXPORT_PCB_STACKUP::UNITS::MM ? 0 : 1 );
    m_cbThickness->SetValue( m_job->m_includeThickness );
    m_cbMaterial->SetValue( m_job->m_includeMaterial );
    m_cbColor->SetValue( m_job->m_includeColor );
    m_cbEpsilonR->SetValue( m_job->m_includeEpsilonR );
    m_cbLossTangent->SetValue( m_job->m_includeLossTangent );
    m_cbFinish->SetValue( m_job->m_includeFinish );
    m_cbBoardOptions->SetValue( m_job->m_includeBoardOptions );

    updateCsvOnlyControls();

    return true;
}


bool DIALOG_BOARD_STACKUP_JOB::TransferDataFromWindow()
{
    m_job->SetConfiguredOutputPath( m_textCtrlOutputPath->GetValue() );
    m_job->m_format = getSelectedFormat();
    m_job->m_units = m_choiceUnits->GetSelection() == 0 ? JOB_EXPORT_PCB_STACKUP::UNITS::MM
                                                        : JOB_EXPORT_PCB_STACKUP::UNITS::INCH;
    m_job->m_includeThickness = m_cbThickness->GetValue();
    m_job->m_includeMaterial = m_cbMaterial->GetValue();
    m_job->m_includeColor = m_cbColor->GetValue();
    m_job->m_includeEpsilonR = m_cbEpsilonR->GetValue();
    m_job->m_includeLossTangent = m_cbLossTangent->GetValue();
    m_job->m_includeFinish = m_cbFinish->GetValue();
    m_job->m_includeBoardOptions = m_cbBoardOptions->GetValue();

    return true;
}
