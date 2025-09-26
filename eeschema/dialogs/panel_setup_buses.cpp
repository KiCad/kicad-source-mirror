/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#include <algorithm>
#include <widgets/wx_grid.h>
#include <widgets/std_bitmap_button.h>
#include <confirm.h>
#include <project/project_file.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <dialogs/panel_setup_buses.h>
#include "grid_tricks.h"
#include <wx/clipbrd.h>

PANEL_SETUP_BUSES::PANEL_SETUP_BUSES( wxWindow* aWindow, SCH_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_BUSES_BASE( aWindow ),
        m_frame( aFrame ),
        m_lastAlias( 0 ),
        m_membersGridDirty( false ),
        m_errorGrid( nullptr ),
        m_errorRow( -1 )
{
    m_membersLabelTemplate = m_membersLabel->GetLabel();

    m_addAlias->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_deleteAlias->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_addMember->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_removeMember->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_source->SetFont( KIUI::GetSmallInfoFont( aWindow ) );

    m_aliasesGrid->OverrideMinSize( 0.6, 0.3 );
    m_membersGrid->OverrideMinSize( 0.6, 0.3 );
    m_aliasesGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_membersGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_aliasesGrid->PushEventHandler( new GRID_TRICKS( m_aliasesGrid,
                                                      [this]( wxCommandEvent& aEvent )
                                                      {
                                                          OnAddAlias( aEvent );
                                                      } ) );

    m_membersGrid->PushEventHandler( new GRID_TRICKS( m_membersGrid,
                                                      [this]( wxCommandEvent& aEvent )
                                                      {
                                                          OnAddMember( aEvent );
                                                      } ) );

    m_aliasesGrid->SetUseNativeColLabels();
    m_membersGrid->SetUseNativeColLabels();

    // wxFormBuilder doesn't include this event...
    m_aliasesGrid->Connect( wxEVT_GRID_CELL_CHANGING,
                            wxGridEventHandler( PANEL_SETUP_BUSES::OnAliasesGridCellChanging ),
                            nullptr, this );
    m_membersGrid->Connect( wxEVT_GRID_CELL_CHANGING,
                            wxGridEventHandler( PANEL_SETUP_BUSES::OnMemberGridCellChanging ),
                            nullptr, this );
    m_membersGrid->Connect( wxEVT_GRID_CELL_CHANGED,
                            wxGridEventHandler( PANEL_SETUP_BUSES::OnMemberGridCellChanged ),
                            nullptr, this );

    Layout();
}


PANEL_SETUP_BUSES::~PANEL_SETUP_BUSES()
{
    // Delete the GRID_TRICKS.
    m_aliasesGrid->PopEventHandler( true );
    m_membersGrid->PopEventHandler( true );

    m_aliasesGrid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                               wxGridEventHandler( PANEL_SETUP_BUSES::OnAliasesGridCellChanging ),
                               nullptr, this );
    m_membersGrid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                               wxGridEventHandler( PANEL_SETUP_BUSES::OnMemberGridCellChanging ),
                               nullptr, this );
    m_membersGrid->Disconnect( wxEVT_GRID_CELL_CHANGED,
                               wxGridEventHandler( PANEL_SETUP_BUSES::OnMemberGridCellChanged ),
                               nullptr, this );
}


void PANEL_SETUP_BUSES::loadAliases()
{
    m_aliases.clear();

    const auto& projectAliases = m_frame->Prj().GetProjectFile().m_BusAliases;

    std::vector<std::pair<wxString, std::vector<wxString>>> aliasList( projectAliases.begin(),
                                                                      projectAliases.end() );

    std::sort( aliasList.begin(), aliasList.end(),
            []( const std::pair<wxString, std::vector<wxString>>& a,
                const std::pair<wxString, std::vector<wxString>>& b )
            {
                return a.first.CmpNoCase( b.first ) < 0;
            } );

    for( const auto& alias : aliasList )
    {
        std::shared_ptr<BUS_ALIAS> entry = std::make_shared<BUS_ALIAS>();

        entry->SetName( alias.first );
        entry->Members() = alias.second;

        m_aliases.push_back( entry );
    }

    int ii = 0;

    m_aliasesGrid->ClearRows();
    m_aliasesGrid->AppendRows( m_aliases.size() );

    for( const std::shared_ptr<BUS_ALIAS>& alias : m_aliases )
        m_aliasesGrid->SetCellValue( ii++, 0, alias->GetName() );

    m_membersBook->SetSelection( 1 );
}


bool PANEL_SETUP_BUSES::TransferDataToWindow()
{
    loadAliases();
    return true;
}


bool PANEL_SETUP_BUSES::TransferDataFromWindow()
{
    if( !m_aliasesGrid->CommitPendingChanges() || !m_membersGrid->CommitPendingChanges() )
        return false;

    // Copy names back just in case they didn't get caught on the GridCellChanging event
    for( int ii = 0; ii < m_aliasesGrid->GetNumberRows(); ++ii )
        m_aliases[ii]->SetName( m_aliasesGrid->GetCellValue( ii, 0 ) );

    // Associate the respective members with the last alias that is active.
    updateAliasMembers( m_lastAlias );

    m_frame->Schematic().SetBusAliases( m_aliases );

    return true;
}


void PANEL_SETUP_BUSES::OnAddAlias( wxCommandEvent& aEvent )
{
    if( !m_aliasesGrid->CommitPendingChanges() || !m_membersGrid->CommitPendingChanges() )
        return;

    m_aliasesGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                // New aliases are stored at the project level
                m_aliases.push_back( std::make_shared<BUS_ALIAS>() );

                int row = m_aliasesGrid->GetNumberRows();

                // Associate the respective members with the previous alias. This ensures that the association
                // starts correctly when adding more than one row.
                // But in order to avoid overwriting the members of the last (row - 1) alias with those of the
                // selected alias (since the user may choose a different alias), the alias member update here
                // should only happen if the current alias (m_lastAlias) is the last one (row - 1).
                if( ( row > 0 ) && ( m_lastAlias == ( row - 1 ) ) )
                    updateAliasMembers( row - 1 );

                m_aliasesGrid->AppendRows();
                return { row, 0 };
            } );
}


void PANEL_SETUP_BUSES::OnDeleteAlias( wxCommandEvent& aEvent )
{
    if( !m_aliasesGrid->CommitPendingChanges() || !m_membersGrid->CommitPendingChanges() )
        return;

    m_aliasesGrid->OnDeleteRows(
            [&]( int row )
            {
                // Clear the members grid first so we don't try to write it back to a deleted alias
               m_membersGrid->ClearRows();
               m_lastAlias = -1;
               m_lastAliasName = wxEmptyString;

               m_aliases.erase( m_aliases.begin() + row );

               m_aliasesGrid->DeleteRows( row, 1 );
            } );
}


void PANEL_SETUP_BUSES::OnAddMember( wxCommandEvent& aEvent )
{
    m_membersGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                int row = m_membersGrid->GetNumberRows();
                m_membersGrid->AppendRows();

                /*
                 * Check if the clipboard contains text data.
                 *
                 * - If `clipboardHasText` is true, select the specified row in the members grid to allow
                 *   our custom context menu to paste the clipboard .
                 * - Otherwise, enable and display the cell edit control, allowing the user to manually edit
                 *   the cell.
                 */
                bool clipboardHasText = false;

                if( wxTheClipboard->Open() )
                {
                    if( wxTheClipboard->IsSupported( wxDF_TEXT )
                        || wxTheClipboard->IsSupported( wxDF_UNICODETEXT ) )
                    {
                        clipboardHasText = true;
                    }

                    wxTheClipboard->Close();
                }

                if( clipboardHasText )
                    return { row, -1 };
                else
                    return { row, 0 };
            } );
}


void PANEL_SETUP_BUSES::OnRemoveMember( wxCommandEvent& aEvent )
{
    m_membersGrid->OnDeleteRows(
            [&]( int row )
            {
                m_membersGrid->DeleteRows( row, 1 );

                // Update the member list of the current bus alias from the members grid
                const std::shared_ptr<BUS_ALIAS>& alias = m_aliases[ m_lastAlias ];
                alias->Members().clear();

                for( int ii = 0; ii < m_membersGrid->GetNumberRows(); ++ii )
                    alias->Members().push_back( m_membersGrid->GetCellValue( ii, 0 ) );
            } );
}


void PANEL_SETUP_BUSES::OnAliasesGridCellChanging( wxGridEvent& event )
{
    int row = event.GetRow();

    if( row >= 0 )
    {
        wxString name = event.GetString();

        for( int ii = 0; ii < m_aliasesGrid->GetNumberRows(); ++ii )
        {
            if( ii == event.GetRow() )
                continue;

            if( name == m_aliasesGrid->GetCellValue( ii, 0 ) )
            {
                m_errorMsg = wxString::Format( _( "Alias name '%s' already in use." ), name );
                m_errorGrid = m_aliasesGrid;
                m_errorRow = row;

                event.Veto();
                return;
            }
        }

        m_aliases[ row ]->SetName( name );
    }
}


void PANEL_SETUP_BUSES::OnMemberGridCellChanging( wxGridEvent& event )
{
    int row = event.GetRow();

    if( row >= 0 )
    {
        wxString name = event.GetString();

        if( name.IsEmpty() )
        {
            m_errorMsg = _( "Member net/alias name cannot be empty." );
            m_errorGrid = m_membersGrid;
            m_errorRow = event.GetRow();

            event.Veto();
            return;
        }

        const std::shared_ptr<BUS_ALIAS>& alias = m_aliases[ m_lastAlias ];

        alias->Members().clear();

        for( int ii = 0; ii < m_membersGrid->GetNumberRows(); ++ii )
        {
            if( ii == row )
            {
                // Parse a space-separated list and add each one
                wxStringTokenizer tok( name, " " );

                if( tok.CountTokens() > 1 )
                {
                    m_membersGridDirty = true;
                    Bind( wxEVT_IDLE, &PANEL_SETUP_BUSES::reloadMembersGridOnIdle, this );
                }

                while( tok.HasMoreTokens() )
                    alias->Members().push_back( tok.GetNextToken() );
            }
            else
            {
                alias->Members().push_back( m_membersGrid->GetCellValue( ii, 0 ) );
            }
        }
    }
}


void PANEL_SETUP_BUSES::OnMemberGridCellChanged( wxGridEvent& event )
{
    if( event.GetRow() >= 0 && m_lastAlias >= 0
            && m_lastAlias < (int) m_aliases.size() )
    {
        updateAliasMembers( m_lastAlias );
    }

    event.Skip();
}


void PANEL_SETUP_BUSES::doReloadMembersGrid()
{
    if( m_lastAlias >= 0 && m_lastAlias < m_aliasesGrid->GetNumberRows() )
    {
        const std::shared_ptr<BUS_ALIAS>& alias = m_aliases[ m_lastAlias ];
        wxString                          source;
        wxString                          membersLabel;

        membersLabel.Printf( m_membersLabelTemplate, m_lastAliasName );

        m_source->SetLabel( source );
        m_membersLabel->SetLabel( membersLabel );

        m_membersGrid->ClearRows();
        m_membersGrid->AppendRows( alias->Members().size() );

        int ii = 0;

        for( const wxString& member : alias->Members() )
            m_membersGrid->SetCellValue( ii++, 0, member );
    }

    m_membersGridDirty = false;
}


void PANEL_SETUP_BUSES::reloadMembersGridOnIdle( wxIdleEvent& aEvent )
{
    if( m_membersGridDirty )
        doReloadMembersGrid();

    Unbind( wxEVT_IDLE, &PANEL_SETUP_BUSES::reloadMembersGridOnIdle, this );
}



void PANEL_SETUP_BUSES::OnSizeGrid( wxSizeEvent& event )
{
    auto setColSize =
            []( WX_GRID* grid )
            {
                int colSize = std::max( grid->GetClientSize().x, grid->GetVisibleWidth( 0 ) );

                if( grid->GetColSize( 0 ) != colSize )
                    grid->SetColSize( 0, colSize );
            };

    setColSize( m_aliasesGrid );
    setColSize( m_membersGrid );

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


void PANEL_SETUP_BUSES::OnUpdateUI( wxUpdateUIEvent& event )
{
    // Handle a grid error.  This is delayed to OnUpdateUI so that we can change focus
    // even when the original validation was triggered from a killFocus event.
    if( !m_errorMsg.IsEmpty() )
    {
        // We will re-enter this routine when the error dialog is displayed, so make
        // sure we don't keep putting up more dialogs.
        wxString errorMsg = m_errorMsg;
        m_errorMsg = wxEmptyString;

        wxWindow* topLevelParent = wxGetTopLevelParent( this );

        DisplayErrorMessage( topLevelParent, errorMsg );

        m_errorGrid->SetFocus();
        m_errorGrid->MakeCellVisible( m_errorRow, 0 );
        m_errorGrid->SetGridCursor( m_errorRow, 0 );

        m_errorGrid->EnableCellEditControl( true );
        m_errorGrid->ShowCellEditControl();

        return;
    }

    if( !m_membersGrid->IsCellEditControlShown() )
    {
        int      row = -1;
        wxString aliasName;

        if( m_aliasesGrid->IsCellEditControlShown() )
        {
            row = m_aliasesGrid->GetGridCursorRow();
            wxGridCellEditor* cellEditor = m_aliasesGrid->GetCellEditor( row, 0 );

            if( wxTextEntry* txt = dynamic_cast<wxTextEntry*>( cellEditor->GetControl() ) )
                aliasName = txt->GetValue();

            cellEditor->DecRef();
        }
        else if( m_aliasesGrid->GetGridCursorRow() >= 0 )
        {
            row = m_aliasesGrid->GetGridCursorRow();
            aliasName = m_aliasesGrid->GetCellValue( row, 0 );
        }
        else if( m_lastAlias >= 0 && m_lastAlias < m_aliasesGrid->GetNumberRows() )
        {
            row = m_lastAlias;
            aliasName = m_lastAliasName;
        }

        if( row < 0 )
        {
            m_membersBook->SetSelection( 1 );
        }
        else if( row != m_lastAlias || aliasName != m_lastAliasName )
        {
            m_lastAlias = row;
            m_lastAliasName = aliasName;

            m_membersBook->SetSelection( 0 );
            m_membersBook->GetPage( 0 )->Layout();

            const std::shared_ptr<BUS_ALIAS>& alias = m_aliases[ row ];
            alias->SetName( aliasName );

            m_membersGridDirty = true;
            Bind( wxEVT_IDLE, &PANEL_SETUP_BUSES::reloadMembersGridOnIdle, this );
        }
    }
}


void PANEL_SETUP_BUSES::ImportSettingsFrom( const std::map<wxString, std::vector<wxString>>& aAliases )
{
    m_aliases.clear();

    std::vector<std::pair<wxString, std::vector<wxString>>> aliasList( aAliases.begin(),
                                                                      aAliases.end() );

    std::sort( aliasList.begin(), aliasList.end(),
            []( const std::pair<wxString, std::vector<wxString>>& a,
                const std::pair<wxString, std::vector<wxString>>& b )
            {
                return a.first.CmpNoCase( b.first ) < 0;
            } );

    for( const auto& alias : aliasList )
    {
        std::shared_ptr<BUS_ALIAS> entry = std::make_shared<BUS_ALIAS>();

        entry->SetName( alias.first );
        entry->Members() = alias.second;

        m_aliases.push_back( entry );
    }

    int ii = 0;

    m_aliasesGrid->ClearRows();
    m_aliasesGrid->AppendRows( m_aliases.size() );

    for( const std::shared_ptr<BUS_ALIAS>& alias : m_aliases )
        m_aliasesGrid->SetCellValue( ii++, 0, alias->GetName() );

    m_membersBook->SetSelection( 1 );
}


void PANEL_SETUP_BUSES::updateAliasMembers( int aAliasIndex )
{
    if( !m_aliases.empty() && m_membersGrid->GetNumberRows() > 0 && aAliasIndex >= 0
        && aAliasIndex < (int)m_aliases.size() )
    {
        const std::shared_ptr<BUS_ALIAS>& alias = m_aliases[aAliasIndex];

        alias->Members().clear();

        for( int ii = 0; ii < m_membersGrid->GetNumberRows(); ++ii )
        {
            wxString memberValue = m_membersGrid->GetCellValue( ii, 0 );

            if( !memberValue.empty() )
            {
                alias->Members().push_back( memberValue );
            }
        }
    }
}
