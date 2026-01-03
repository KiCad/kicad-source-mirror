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

#include "widgets/grid_readonly_text_helpers.h"
#include <dialogs/dialog_executecommand_job_settings.h>
#include <jobs/job_special_execute.h>
#include <scintilla_tricks.h>
#include <grid_tricks.h>
#include <project.h>
#include <env_vars.h>
#include <wx/regex.h>


DIALOG_EXECUTECOMMAND_JOB_SETTINGS::DIALOG_EXECUTECOMMAND_JOB_SETTINGS( wxWindow* aParent,
                                                                        JOB_SPECIAL_EXECUTE* aJob ) :
        DIALOG_EXECUTECOMMAND_JOB_SETTINGS_BASE( aParent ),
        m_job( aJob ),
        m_scintillaTricks( nullptr )
{
    m_textCtrlCommand->SetWrapMode( wxSTC_WRAP_CHAR );

    m_scintillaTricks = new SCINTILLA_TRICKS( m_textCtrlCommand, wxT( "{}" ), false,
            // onAcceptFn
            [this]( wxKeyEvent& aEvent )
            {
                wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
            },

            // onCharFn
            [this]( wxStyledTextEvent& aEvent )
            {
                m_scintillaTricks->DoTextVarAutocomplete(
                        // getTokensFn
                        []( const wxString& xRef, wxArrayString* tokens )
                        {
                            ENV_VAR::GetEnvVarAutocompleteTokens( tokens );
                            tokens->Add( OUTPUT_TMP_PATH_VAR_NAME );
                        } );
            } );

    // add Cut, Copy, and Paste to wxGrids
    m_path_subs_grid->PushEventHandler( new GRID_TRICKS( m_path_subs_grid ) );

    populateEnvironReadOnlyTable();

    m_path_subs_grid->SetColLabelValue( 0, _( "Name" ) );
    m_path_subs_grid->SetColLabelValue( 1, _( "Value" ) );

    SetupStandardButtons();

    finishDialogSettings();
}


DIALOG_EXECUTECOMMAND_JOB_SETTINGS::~DIALOG_EXECUTECOMMAND_JOB_SETTINGS()
{
    delete m_scintillaTricks;
    m_scintillaTricks = nullptr;

    // Delete the GRID_TRICKS.
    m_path_subs_grid->PopEventHandler( true );
}


bool DIALOG_EXECUTECOMMAND_JOB_SETTINGS::TransferDataFromWindow()
{
    m_job->m_command = m_textCtrlCommand->GetValue();
    m_job->m_ignoreExitcode = m_cbIgnoreExitCode->GetValue();
    m_job->m_recordOutput = m_cbRecordOutput->GetValue();
    m_job->SetConfiguredOutputPath( m_textCtrlOutputPath->GetValue() );

    return true;
}


bool DIALOG_EXECUTECOMMAND_JOB_SETTINGS::TransferDataToWindow()
{
    m_textCtrlCommand->SetValue( m_job->m_command );
    m_cbIgnoreExitCode->SetValue( m_job->m_ignoreExitcode );
    m_cbRecordOutput->SetValue( m_job->m_recordOutput );

    m_textCtrlOutputPath->SetValue( m_job->GetConfiguredOutputPath() );
    m_textCtrlOutputPath->Enable( m_cbRecordOutput->GetValue() );

    return true;
}


void DIALOG_EXECUTECOMMAND_JOB_SETTINGS::OnRecordOutputClicked( wxCommandEvent& aEvent )
{
    m_textCtrlOutputPath->Enable( m_cbRecordOutput->GetValue() );
}


void DIALOG_EXECUTECOMMAND_JOB_SETTINGS::populateEnvironReadOnlyTable()
{
    wxRegEx re( ".*?(\\$\\{(.+?)\\})|(\\$\\((.+?)\\)).*?", wxRE_ADVANCED );
    wxASSERT( re.IsValid() );   // wxRE_ADVANCED is required.

    std::set<wxString> unique;
    wxString           src = m_textCtrlCommand->GetValue();

    // clear the table
    m_path_subs_grid->ClearRows();

    while( re.Matches( src ) )
    {
        wxString envvar = re.GetMatch( src, 2 );

        // if not ${...} form then must be $(...)
        if( envvar.IsEmpty() )
            envvar = re.GetMatch( src, 4 );

        // ignore duplicates
        unique.insert( envvar );

        // delete the last match and search again
        src.Replace( re.GetMatch( src, 0 ), wxEmptyString );
    }

    // Make sure these variables shows up even if not used yet.
    unique.insert( OUTPUT_TMP_PATH_VAR_NAME );
    unique.insert( PROJECT_VAR_NAME );

    for( const wxString& evName : unique )
    {
        int row = m_path_subs_grid->GetNumberRows();
        m_path_subs_grid->AppendRows( 1 );

        m_path_subs_grid->SetCellValue( row, 0, wxT( "${" ) + evName + wxT( "}" ) );
        m_path_subs_grid->SetCellEditor( row, 0, new GRID_CELL_READONLY_TEXT_EDITOR() );

        wxString evValue;

        if( evName.IsSameAs( OUTPUT_TMP_PATH_VAR_NAME ) )
        {
            evValue = _( "<value set at runtime>" );
            m_path_subs_grid->SetCellFont( row, 1, m_path_subs_grid->GetCellFont( row, 1 ).Italic() );
        }
        else
        {
            wxGetEnv( evName, &evValue );
        }

        m_path_subs_grid->SetCellValue( row, 1, evValue );
        m_path_subs_grid->SetCellEditor( row, 1, new GRID_CELL_READONLY_TEXT_EDITOR() );
    }

    adjustPathSubsGridColumns( m_path_subs_grid->GetRect().GetWidth() );
}


void DIALOG_EXECUTECOMMAND_JOB_SETTINGS::adjustPathSubsGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_path_subs_grid->GetSize().x - m_path_subs_grid->GetClientSize().x );

    m_path_subs_grid->AutoSizeColumn( 0 );
    m_path_subs_grid->SetColSize( 0, std::max( 200, m_path_subs_grid->GetColSize( 0 ) ) );
    m_path_subs_grid->SetColSize( 1, std::max( 300, aWidth - m_path_subs_grid->GetColSize( 0 ) ) );
}


void DIALOG_EXECUTECOMMAND_JOB_SETTINGS::onSizeGrid( wxSizeEvent& event )
{
    adjustPathSubsGridColumns( event.GetSize().GetX() );

    event.Skip();
}


