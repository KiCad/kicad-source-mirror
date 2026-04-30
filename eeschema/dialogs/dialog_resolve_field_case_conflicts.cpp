/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "dialog_resolve_field_case_conflicts.h"

#include <sch_edit_frame.h>
#include <sch_symbol.h>
#include <sch_commit.h>
#include <wx/grid.h>


DIALOG_RESOLVE_FIELD_CASE_CONFLICTS::DIALOG_RESOLVE_FIELD_CASE_CONFLICTS(
        wxWindow* aParent, SCH_EDIT_FRAME* aFrame,
        std::vector<FIELD_CASE_CONFLICT> aConflicts ) :
        DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE( aParent ),
        m_frame( aFrame ),
        m_conflicts( std::move( aConflicts ) ),
        m_rowAction( m_conflicts.size(), ACTION_KEEP_FIRST )
{
    m_conflictsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_headerLabel->SetLabel( wxString::Format(
            _( "%zu symbols have user fields whose names differ only in case. "
               "Choose how to resolve each before opening the table." ),
            m_conflicts.size() ) );

    populateGrid();

    SetupStandardButtons( { { wxID_OK,     _( "Apply and Continue" ) },
                            { wxID_CANCEL, _( "Cancel" ) } } );

    finishDialogSettings();
}


void DIALOG_RESOLVE_FIELD_CASE_CONFLICTS::populateGrid()
{
    wxGrid* g = m_conflictsGrid;
    g->BeginBatch();

    if( g->GetNumberRows() > 0 )
        g->DeleteRows( 0, g->GetNumberRows() );

    g->AppendRows( static_cast<int>( m_conflicts.size() ) );

    for( size_t i = 0; i < m_conflicts.size(); ++i )
    {
        const FIELD_CASE_CONFLICT& c = m_conflicts[i];

        g->SetCellValue( i, 0, c.reference );
        g->SetCellValue( i, 1, c.sheetPath.PathHumanReadable( false ) );

        wxString fieldsLabel = wxString::Format( "%s\n%s",
                                                 c.variants[0].first,
                                                 c.variants[1].first );

        wxString valuesLabel = wxString::Format( "%s\n%s",
                                                 c.variants[0].second.IsEmpty()
                                                         ? wxString( _( "(empty)" ) )
                                                         : c.variants[0].second,
                                                 c.variants[1].second.IsEmpty()
                                                         ? wxString( _( "(empty)" ) )
                                                         : c.variants[1].second );

        g->SetCellValue( i, 2, fieldsLabel );
        g->SetCellValue( i, 3, valuesLabel );

        wxArrayString choices;
        choices.Add( wxString::Format( _( "Keep '%s'" ), c.variants[0].first ) );
        choices.Add( wxString::Format( _( "Keep '%s'" ), c.variants[1].first ) );
        choices.Add( _( "Join values" ) );

        g->SetCellEditor( i, 4, new wxGridCellChoiceEditor( choices, false ) );
        g->SetCellValue( i, 4, choices[0] );
    }

    g->AutoSizeRows();
    g->EndBatch();
}


std::vector<int>
DIALOG_RESOLVE_FIELD_CASE_CONFLICTS::findSiblingRows( int aRow ) const
{
    std::vector<int> siblings;

    if( aRow < 0 || static_cast<size_t>( aRow ) >= m_conflicts.size() )
        return siblings;

    const wxString& key = m_conflicts[aRow].caseFoldedKey;

    for( size_t i = 0; i < m_conflicts.size(); ++i )
    {
        if( static_cast<int>( i ) != aRow
            && m_conflicts[i].caseFoldedKey.IsSameAs( key, false ) )
        {
            siblings.push_back( static_cast<int>( i ) );
        }
    }

    return siblings;
}


void DIALOG_RESOLVE_FIELD_CASE_CONFLICTS::onActionCellChanged( wxGridEvent& event )
{
    int row = event.GetRow();
    int col = event.GetCol();

    if( col != 4 || row < 0 || static_cast<size_t>( row ) >= m_conflicts.size() )
    {
        event.Skip();
        return;
    }

    wxString choice = m_conflictsGrid->GetCellValue( row, 4 );
    ACTION   action = ACTION_KEEP_FIRST;

    if( choice.Contains( m_conflicts[row].variants[1].first ) )
        action = ACTION_KEEP_SECOND;
    else if( choice == _( "Join values" ) )
        action = ACTION_JOIN;

    m_rowAction[row] = action;

    if( m_bulkApplyCheckbox->IsChecked() )
    {
        for( int sibling : findSiblingRows( row ) )
        {
            m_rowAction[sibling] = action;
            m_conflictsGrid->SetCellValue( sibling, 4, choice );
        }
    }

    event.Skip();
}


void DIALOG_RESOLVE_FIELD_CASE_CONFLICTS::onApplyAndContinue( wxCommandEvent& event )
{
    applyResolutions();
    EndModal( wxID_OK );
}


void DIALOG_RESOLVE_FIELD_CASE_CONFLICTS::applyResolutions()
{
    SCH_COMMIT commit( m_frame );
    wxString   sep = m_separatorCtrl->GetValue();

    for( size_t i = 0; i < m_conflicts.size(); ++i )
    {
        FIELD_CASE_CONFLICT& c   = m_conflicts[i];
        SCH_SYMBOL*          sym = c.symbol;

        commit.Modify( sym, c.sheetPath.LastScreen() );

        SCH_FIELD* a = sym->GetField( c.variants[0].first );
        SCH_FIELD* b = sym->GetField( c.variants[1].first );

        if( !a || !b || a == b )
            continue;

        const wxString& aValue = c.variants[0].second;
        const wxString& bValue = c.variants[1].second;

        switch( m_rowAction[i] )
        {
        case ACTION_KEEP_FIRST:
        {
            wxString winning = aValue.IsEmpty() ? bValue : aValue;
            a->SetText( winning );
            sym->RemoveField( c.variants[1].first );
            break;
        }
        case ACTION_KEEP_SECOND:
        {
            wxString winning = bValue.IsEmpty() ? aValue : bValue;
            b->SetText( winning );
            sym->RemoveField( c.variants[0].first );
            break;
        }
        case ACTION_JOIN:
        {
            wxString joined;

            if( !aValue.IsEmpty() && !bValue.IsEmpty() )
                joined = aValue + sep + bValue;
            else
                joined = aValue.IsEmpty() ? bValue : aValue;

            a->SetText( joined );
            sym->RemoveField( c.variants[1].first );
            break;
        }
        }
    }

    commit.Push( _( "Resolve duplicate field names" ) );
}
