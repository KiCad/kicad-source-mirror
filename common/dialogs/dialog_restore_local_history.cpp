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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dialog_restore_local_history.h"

#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>


DIALOG_RESTORE_LOCAL_HISTORY::DIALOG_RESTORE_LOCAL_HISTORY(
        wxWindow* aParent, const std::vector<LOCAL_HISTORY_SNAPSHOT_INFO>& aSnapshots ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Restore Project from Local History..." ) ),
        m_snapshots( aSnapshots )
{
    BuildUi();
    Populate();
    Centre();
}


void DIALOG_RESTORE_LOCAL_HISTORY::BuildUi()
{
    m_list = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             wxLC_HRULES | wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_VRULES );
    m_list->AppendColumn( _( "Time" ) );
    m_list->AppendColumn( _( "Action" ) );
    m_list->AppendColumn( _( "Count" ) );

    m_details = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_READONLY );

    wxStdDialogButtonSizer* buttons = new wxStdDialogButtonSizer();
    m_restoreButton = new wxButton( this, wxID_OK, _( "Restore" ) );
    m_restoreButton->Enable( false );
    m_restoreButton->Bind( wxEVT_BUTTON, &DIALOG_RESTORE_LOCAL_HISTORY::OnRestoreClicked, this );
    buttons->AddButton( m_restoreButton );
    buttons->AddButton( new wxButton( this, wxID_CANCEL ) );
    buttons->Realize();

    wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );
    topSizer->Add( m_list, 1, wxEXPAND | wxALL, 5 );
    topSizer->Add( m_details, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
    topSizer->Add( buttons, 0, wxEXPAND | wxALL, 5 );
    SetSizerAndFit( topSizer );

    SetMinSize( FromDIP( wxSize( 700, 500 ) ) );

    m_list->Bind( wxEVT_LIST_ITEM_SELECTED, &DIALOG_RESTORE_LOCAL_HISTORY::OnSelectionChanged, this );
    m_list->Bind( wxEVT_LIST_ITEM_ACTIVATED, &DIALOG_RESTORE_LOCAL_HISTORY::OnItemActivated, this );
}


void DIALOG_RESTORE_LOCAL_HISTORY::Populate()
{
    for( size_t i = 0; i < m_snapshots.size(); ++i )
    {
        const auto& snapshot = m_snapshots[i];
        long        row = m_list->InsertItem( m_list->GetItemCount(), snapshot.date.FormatISOCombined() );
        wxString    countText =
                snapshot.filesChanged > 0 ? wxString::Format( "%d", snapshot.filesChanged ) : wxString( "-" );

        m_list->SetItem( row, 1, snapshot.summary );

        m_list->SetItem( row, 2, countText );
    }

    m_list->SetColumnWidth( 0, FromDIP( 170 ) );
    m_list->SetColumnWidth( 1, FromDIP( 380 ) );
    m_list->SetColumnWidth( 2, FromDIP( 70 ) );

    m_details->Clear();
    m_selectedIndex = wxNOT_FOUND;
    m_selectedHash.clear();

    if( m_restoreButton )
        m_restoreButton->Enable( false );
}


void DIALOG_RESTORE_LOCAL_HISTORY::UpdateDetails( long aIndex )
{
    if( aIndex < 0 || aIndex >= static_cast<long>( m_snapshots.size() ) )
        return;

    const auto& snapshot = m_snapshots[aIndex];

    wxString text;
    text << snapshot.summary << "\n";
    text << snapshot.date.FormatISOCombined() << "\n";
    text << snapshot.hash;

    if( !snapshot.changedFiles.empty() )
        text << "\n\n";

    for( size_t ii = 0; ii < snapshot.changedFiles.size(); ++ii )
    {
        if( ii > 0 )
            text << "\n";

        text << snapshot.changedFiles[ii];
    }

    m_details->SetValue( text );
}


wxString DIALOG_RESTORE_LOCAL_HISTORY::GetSelectedHash() const
{
    if( m_selectedIndex == wxNOT_FOUND || m_selectedIndex >= static_cast<long>( m_snapshots.size() ) )
    {
        return wxEmptyString;
    }

    return m_snapshots[m_selectedIndex].hash;
}


void DIALOG_RESTORE_LOCAL_HISTORY::OnSelectionChanged( wxListEvent& aEvent )
{
    long selected = m_list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( selected == wxNOT_FOUND )
    {
        m_selectedIndex = wxNOT_FOUND;
        m_selectedHash.clear();
        m_details->Clear();

        if( m_restoreButton )
            m_restoreButton->Enable( false );
    }
    else
    {
        m_selectedIndex = selected;
        m_selectedHash = m_snapshots[selected].hash;
        UpdateDetails( selected );

        if( m_restoreButton )
            m_restoreButton->Enable( true );
    }

    aEvent.Skip();
}


void DIALOG_RESTORE_LOCAL_HISTORY::OnItemActivated( wxListEvent& aEvent )
{
    long index = aEvent.GetIndex();

    if( index >= 0 && index < static_cast<long>( m_snapshots.size() ) )
    {
        m_selectedIndex = index;
        m_selectedHash = m_snapshots[index].hash;
        UpdateDetails( index );
    }

    if( !m_selectedHash.IsEmpty() )
        EndModal( wxID_OK );
}


void DIALOG_RESTORE_LOCAL_HISTORY::OnRestoreClicked( wxCommandEvent& aEvent )
{
    long index = m_list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( index == wxNOT_FOUND )
        index = m_list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED );

    if( index == wxNOT_FOUND || index >= static_cast<long>( m_snapshots.size() ) )
        return;

    m_selectedIndex = index;
    m_selectedHash = m_snapshots[index].hash;

    aEvent.Skip();
}
