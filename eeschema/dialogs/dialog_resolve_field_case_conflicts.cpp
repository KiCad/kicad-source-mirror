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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dialog_resolve_field_case_conflicts.h"

#include <sch_edit_frame.h>
#include <sch_symbol.h>
#include <sch_commit.h>
#include <wx/grid.h>

#include "grid_tricks.h"


DIALOG_RESOLVE_FIELD_CASE_CONFLICTS::DIALOG_RESOLVE_FIELD_CASE_CONFLICTS( wxWindow* aParent, SCH_EDIT_FRAME* aFrame,
                                                                          std::vector<FIELD_CASE_CONFLICT> conflicts ) :
        DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE( aParent ),
        m_frame( aFrame ),
        m_conflicts( std::move( conflicts ) ),
        m_rowAction( m_conflicts.size(), 0 )
{
    m_grid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_headerLabel->SetLabel( wxString::Format( _( "%zu symbols have user fields whose names differ only in case. "
                                                  "Choose how to resolve each before opening the table." ),
                                               m_conflicts.size() ) );

    m_grid->PushEventHandler( new GRID_TRICKS( m_grid ) );
    m_grid->SetupColumnAutosizer( 3 );

    populateGrid();

    SetupStandardButtons( { { wxID_OK,     _( "Apply and Continue" ) },
                            { wxID_CANCEL, _( "Cancel" ) } } );

    finishDialogSettings();
}


DIALOG_RESOLVE_FIELD_CASE_CONFLICTS::~DIALOG_RESOLVE_FIELD_CASE_CONFLICTS()
{
    // destroy GRID_TRICKS before m_conflictsGrid.
    m_grid->PopEventHandler( true );
}


void DIALOG_RESOLVE_FIELD_CASE_CONFLICTS::populateGrid()
{
    m_grid->BeginBatch();
    m_grid->ClearRows();
    m_grid->AppendRows( static_cast<int>( m_conflicts.size() ) );

    auto formatValue =
            []( const wxString& aValue )
            {
                return aValue.IsEmpty() ? wxString( _( "(empty)" ) ) : aValue;
            };

    for( int i = 0; i < (int) m_conflicts.size(); ++i )
    {
        const FIELD_CASE_CONFLICT& c = m_conflicts[i];

        m_grid->SetCellValue( i, 0, c.reference );
        m_grid->SetCellValue( i, 1, c.sheetPath.PathHumanReadable( false ) );

        wxString fieldsLabel = c.variants[0].first;
        wxString valuesLabel = formatValue( c.variants[0].second );

        for( size_t ii = 1; ii < c.variants.size(); ++ii )
        {
            fieldsLabel += '\n' + c.variants[ii].first;
            valuesLabel += '\n' + formatValue( c.variants[ii].second );
        }

        m_grid->SetCellValue( i, 2, fieldsLabel );
        m_grid->SetCellValue( i, 3, valuesLabel );

        wxArrayString choices;
        choices.Add( wxString::Format( _( "Keep '%s'" ), c.variants[0].first ) );

        for( size_t ii = 1; ii < c.variants.size(); ++ii )
            choices.Add( wxString::Format( _( "Keep '%s'" ), c.variants[ii].first ) );

        choices.Add( _( "Join values" ) );

        m_grid->SetCellEditor( i, 4, new wxGridCellChoiceEditor( choices, false ) );
        m_grid->SetCellValue( i, 4, choices[0] );
    }

    m_grid->AutoSizeRows();
    m_grid->EndBatch();
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
        if( static_cast<int>( i ) != aRow && m_conflicts[i].caseFoldedKey.IsSameAs( key, false ) )
            siblings.push_back( static_cast<int>( i ) );
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

    m_rowAction[row] = -1;  // join values

    for( int ii = 0; ii < (int) m_conflicts[row].variants.size(); ++ii )
    {
        if( m_grid->GetCellValue( row, 4 ).Contains( m_conflicts[row].variants[ii].first ) )
        {
            m_rowAction[row] = ii;
            break;
        }
    }

    if( m_bulkApplyCheckbox->IsChecked() )
    {
        for( int sibling : findSiblingRows( row ) )
        {
            m_rowAction[sibling] = m_rowAction[row];
            m_grid->SetCellValue( sibling, 4, m_grid->GetCellValue( row, 4 ) );
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

    for( size_t row = 0; row < m_conflicts.size(); ++row )
    {
        FIELD_CASE_CONFLICT& c   = m_conflicts[row];
        SCH_SYMBOL*          sym = c.symbol;
        int                  action = m_rowAction[row];

        commit.Modify( sym, c.sheetPath.LastScreen() );

        wxString value;

        if( action < 0 )    // join
        {
            for( const auto& [fieldName, fieldValue] : c.variants )
            {
                if( !value.IsEmpty() && !fieldValue.IsEmpty() )
                    value += sep;

                value += fieldValue;
            }

            action = 0;     // apply result to first field
        }
        else
        {
            if( action < (int) c.variants.size() )
                value = c.variants[action].second;

            for( const auto& [fieldName, fieldValue] : c.variants )
            {
                if( value.IsEmpty() )
                    value = fieldValue;
            }
        }

        for( int jj = 0; jj < (int) c.variants.size(); ++jj )
        {
            if( jj == action )
            {
                if( SCH_FIELD* field = sym->GetField( c.variants[jj].first ) )
                    field->SetText( value );
            }
            else
            {
                sym->RemoveField( c.variants[jj].first );
            }
        }
    }

    commit.Push( _( "Resolve Duplicate Field Names" ) );
}
