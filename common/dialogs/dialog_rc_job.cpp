/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/dialog_rc_job.h>
#include <map>
#include <i18n_utility.h>
#include <wildcards_and_files_ext.h>

static std::map<JOB_RC::OUTPUT_FORMAT, wxString> outputFormatMap = {
    { JOB_RC::OUTPUT_FORMAT::REPORT, _HKI( "Report" ) },
    { JOB_RC::OUTPUT_FORMAT::JSON, _HKI( "JSON" ) }
};


DIALOG_RC_JOB::DIALOG_RC_JOB( wxWindow* parent, JOB_RC* aJob, const wxString& aTitle ) :
        DIALOG_RC_JOB_BASE( parent, wxID_ANY, aTitle ),
        m_job( aJob )
{
    for( const auto& [format, name] : outputFormatMap )
        m_choiceFormat->Append( wxGetTranslation( name ) );

    SetupStandardButtons();
}


JOB_RC::OUTPUT_FORMAT DIALOG_RC_JOB::getSelectedFormat()
{
    int selIndx = m_choiceFormat->GetSelection();
    auto it = outputFormatMap.begin();
    std::advance( it, selIndx );
    return it->first;
}


void DIALOG_RC_JOB::setSelectedFormat( JOB_RC::OUTPUT_FORMAT format )
{
    auto it = outputFormatMap.find( format );
    if( it != outputFormatMap.end() )
    {
        int idx = std::distance( outputFormatMap.begin(), it );
        m_choiceFormat->SetSelection( idx );
    }
}


void DIALOG_RC_JOB::OnFormatChoice( wxCommandEvent& event )
{
    JOB_RC::OUTPUT_FORMAT selectedFormat = getSelectedFormat();

    if( !m_textCtrlOutputPath->GetValue().IsEmpty() )
    {
        wxFileName fn( m_textCtrlOutputPath->GetValue() );

        switch( selectedFormat )
        {
        case JOB_RC::OUTPUT_FORMAT::REPORT: fn.SetExt( FILEEXT::ReportFileExtension ); break;
        case JOB_RC::OUTPUT_FORMAT::JSON:   fn.SetExt( FILEEXT::JsonFileExtension );   break;
        }

        m_textCtrlOutputPath->SetValue( fn.GetFullPath() );
    }
}


bool DIALOG_RC_JOB::TransferDataToWindow()
{
    m_textCtrlOutputPath->SetValue( m_job->GetConfiguredOutputPath() );
    setSelectedFormat( m_job->m_format );
    m_cbHaltOutput->SetValue( m_job->m_exitCodeViolations );

    m_cbViolationErrors->SetValue( m_job->m_severity & SEVERITY::RPT_SEVERITY_ERROR );
    m_cbViolationWarnings->SetValue( m_job->m_severity & SEVERITY::RPT_SEVERITY_WARNING );

    return true;
}


bool DIALOG_RC_JOB::TransferDataFromWindow()
{
    m_job->SetConfiguredOutputPath( m_textCtrlOutputPath->GetValue() );
    m_job->m_format = getSelectedFormat();
    m_job->m_exitCodeViolations = m_cbHaltOutput->GetValue();

    m_job->m_severity = 0;

    if( m_cbViolationErrors->GetValue() )
        m_job->m_severity |= SEVERITY::RPT_SEVERITY_ERROR;

    if( m_cbViolationWarnings->GetValue() )
        m_job->m_severity |= SEVERITY::RPT_SEVERITY_WARNING;

    return true;
}