/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/dialog_mdw_release.h>

#include <wx/intl.h>


DIALOG_MDW_RELEASE::DIALOG_MDW_RELEASE( wxWindow* aParent, const MDW_RELEASE_PROMPT& aPrompt, RELEASE aRelease ) :
        DIALOG_MDW_RELEASE_BASE( aParent ),
        m_release( std::move( aRelease ) ),
        m_alive( std::make_shared<bool>( true ) )
{
    m_partsList->AppendTextColumn( _( "Part" ), wxDATAVIEW_CELL_INERT, FromDIP( 200 ) );
    m_partsList->AppendTextColumn( _( "Releases as" ), wxDATAVIEW_CELL_INERT, FromDIP( 100 ) );
    m_partsList->AppendTextColumn( _( "Status" ), wxDATAVIEW_CELL_INERT, FromDIP( 240 ) );

    for( size_t i = 0; i < aPrompt.rows.size(); ++i )
    {
        m_partsList->AppendItem( { wxVariant( aPrompt.rows[i].first ), wxVariant( aPrompt.rows[i].second ),
                                   wxVariant( aPrompt.concerns[i] ) } );
    }

    SetPrompt( aPrompt );

    // A restored comment would describe the previous release
    OptOut( m_comment );

    Bind( wxEVT_BUTTON, &DIALOG_MDW_RELEASE::onRelease, this, wxID_OK );
    Bind( wxEVT_BUTTON, &DIALOG_MDW_RELEASE::onCancel, this, wxID_CANCEL );

    SetupStandardButtons( { { wxID_OK, aPrompt.okLabel } } );
    finishDialogSettings();
}


void DIALOG_MDW_RELEASE::SetStatus( size_t aRow, const wxString& aStatus )
{
    m_partsList->SetTextValue( aStatus, static_cast<unsigned>( aRow ), 2 );
}


void DIALOG_MDW_RELEASE::SetPrompt( const MDW_RELEASE_PROMPT& aPrompt )
{
    if( m_state != STATE::READY || aPrompt.rows.size() != static_cast<size_t>( m_partsList->GetItemCount() ) )
        return;

    for( size_t row = 0; row < aPrompt.concerns.size(); ++row )
        SetStatus( row, aPrompt.concerns[row] );

    m_notes = aPrompt.notes;
    showNote( wxString() );
    m_sdbSizerOK->SetLabel( aPrompt.okLabel );
}


void DIALOG_MDW_RELEASE::showNote( const wxString& aOutcome )
{
    wxArrayString lines = m_notes;

    lines.Add( _( "A release cannot be undone once it starts." ) );

    if( !aOutcome.IsEmpty() )
        lines.Add( aOutcome );

    m_note->SetLabel( wxJoin( lines, '\n', '\0' ) );
    m_note->Wrap( FromDIP( 560 ) );
    Layout();
}


void DIALOG_MDW_RELEASE::SetInFlight( bool aCanStop )
{
    if( m_state == STATE::RUNNING )
        m_sdbSizerCancel->Enable( aCanStop );
}


void DIALOG_MDW_RELEASE::Finish( const wxString& aSummary )
{
    m_state = STATE::FINISHED;

    if( !aSummary.IsEmpty() )
        showNote( aSummary );

    m_sdbSizerCancel->Enable();
    m_sdbSizerCancel->SetLabel( _( "Close" ) );
    m_sdbSizerCancel->SetFocus();
}


void DIALOG_MDW_RELEASE::onRelease( wxCommandEvent& aEvent )
{
    // Ctrl+Return posts OK past the disabled button, and a second run would release from stale listings
    if( m_state != STATE::READY )
        return;

    m_state = STATE::RUNNING;

    for( int row = 0; row < m_partsList->GetItemCount(); ++row )
        SetStatus( row, _( "Waiting" ) );

    m_sdbSizerOK->Disable();
    m_comment->Disable();
    m_sdbSizerCancel->SetLabel( _( "Stop After This Part" ) );
    m_sdbSizerCancel->Disable();
    m_release( m_comment->GetValue() );
}


void DIALOG_MDW_RELEASE::onCancel( wxCommandEvent& aEvent )
{
    // A release in flight cannot be called back, so the dialog stays to show how it ends
    if( m_state == STATE::RUNNING )
    {
        m_state = STATE::CANCELLING;
        m_sdbSizerCancel->SetLabel( _( "Cancelling..." ) );
        m_sdbSizerCancel->Disable();
        return;
    }

    if( m_state != STATE::CANCELLING )
        aEvent.Skip();
}
