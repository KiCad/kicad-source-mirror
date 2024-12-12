/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "drc_re_condition_group_panel.h"
#include "drc_re_condition_row_panel.h"

#include <bitmaps.h>

#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/bmpbuttn.h>
#include <wx/stattext.h>
#include <wx/log.h>

static constexpr const wxChar* TRACE_COND = wxT( "KI_TRACE_DRC_RULE_EDITOR" );


DRC_RE_CONDITION_GROUP_PANEL::DRC_RE_CONDITION_GROUP_PANEL( wxWindow* aParent, BOARD* aBoard,
                                                            bool aTwoObjectConstraint ) :
        wxPanel( aParent ),
        m_board( aBoard ),
        m_twoObjectConstraint( aTwoObjectConstraint ),
        m_suppressCallbacks( false )
{
    wxLogTrace( TRACE_COND, wxS( "[DRC_RE_CONDITION_GROUP_PANEL] ctor START" ) );
    m_mainSizer = new wxBoxSizer( wxVERTICAL );

    // Label
    wxString labelText = aTwoObjectConstraint ? _( "Where conditions match:" )
                                              : _( "Where object matches:" );
    m_labelText = new wxStaticText( this, wxID_ANY, labelText );
    m_mainSizer->Add( m_labelText, 0, wxALL, 5 );

    // Add button (added to sizer before first condition row so insert logic works)
    m_addBtn = new wxBitmapButton( this, wxID_ANY, KiBitmapBundle( BITMAPS::small_plus ) );
    m_addBtn->SetToolTip( _( "Add another condition" ) );
    m_mainSizer->Add( m_addBtn, 0, wxALL | wxALIGN_RIGHT, 5 );

    SetSizer( m_mainSizer );

    m_addBtn->Bind( wxEVT_BUTTON, &DRC_RE_CONDITION_GROUP_PANEL::onAddClicked, this );

    // Start with one empty condition row
    addConditionRow();

    // Ensure proper initial layout
    m_mainSizer->Fit( this );
    Layout();
}


void DRC_RE_CONDITION_GROUP_PANEL::ParseCondition( const wxString& aConditionExpr )
{
    wxLogTrace( TRACE_COND, wxS( "[ParseCondition] expr='%s'" ), aConditionExpr );

    // Suppress change callbacks during parsing to avoid premature visibility updates
    m_suppressCallbacks = true;

    // Clear existing rows
    while( m_conditions.size() > 1 )
        removeConditionRow( static_cast<int>( m_conditions.size() ) - 1 );

    if( aConditionExpr.IsEmpty() )
    {
        wxLogTrace( TRACE_COND, wxS( "[ParseCondition] empty expression, setting to ANY" ) );

        if( m_conditions.empty() )
            addConditionRow();

        m_conditions[0].panel->ParseExpression( wxEmptyString );
        m_suppressCallbacks = false;
        return;
    }

    // Tokenize the expression by boolean operators
    std::vector<std::pair<BOOL_OPERATOR, wxString>> parts;

    if( !tokenizeCondition( aConditionExpr, parts ) )
    {
        wxLogTrace( TRACE_COND, wxS( "[ParseCondition] tokenize failed, using single custom row" ) );

        // Parsing failed - fall back to single custom query row
        if( m_conditions.empty() )
            addConditionRow();

        m_conditions[0].panel->ParseExpression( aConditionExpr );
        m_suppressCallbacks = false;
        return;
    }

    wxLogTrace( TRACE_COND, wxS( "[ParseCondition] tokenized into %zu parts" ), parts.size() );

    // Create rows for each part
    for( size_t i = 0; i < parts.size(); ++i )
    {
        wxLogTrace( TRACE_COND, wxS( "[ParseCondition] part[%zu] op=%d expr='%s'" ),
                    i, static_cast<int>( parts[i].first ), parts[i].second );

        if( i == 0 )
        {
            if( m_conditions.empty() )
                addConditionRow();

            m_conditions[0].panel->ParseExpression( parts[i].second );
        }
        else
        {
            addConditionRow( parts[i].first );
            m_conditions.back().panel->ParseExpression( parts[i].second );

            if( m_conditions.back().operatorChoice )
                m_conditions.back().operatorChoice->SetSelection( static_cast<int>( parts[i].first ) );
        }
    }

    updateDeleteButtonVisibility();

    m_suppressCallbacks = false;

    wxLogTrace( TRACE_COND, wxS( "[ParseCondition] done, HasCustomQuerySelected=%d" ),
                HasCustomQuerySelected() ? 1 : 0 );
}


wxString DRC_RE_CONDITION_GROUP_PANEL::BuildCondition() const
{
    wxString result;

    for( size_t i = 0; i < m_conditions.size(); ++i )
    {
        wxString rowExpr = m_conditions[i].panel->BuildExpression();

        if( rowExpr.IsEmpty() )
            continue;

        if( !result.IsEmpty() )
        {
            switch( m_conditions[i].boolOp )
            {
            case BOOL_OPERATOR::AND:     result += " && ";   break;
            case BOOL_OPERATOR::OR:      result += " || ";   break;
            case BOOL_OPERATOR::AND_NOT: result += " && !";  break;
            case BOOL_OPERATOR::OR_NOT:  result += " || !";  break;
            }
        }

        result += rowExpr;
    }

    return result;
}


bool DRC_RE_CONDITION_GROUP_PANEL::HasCustomQuerySelected() const
{
    for( size_t i = 0; i < m_conditions.size(); ++i )
    {
        bool hasCustom = m_conditions[i].panel->HasCustomQuerySelected();

        wxLogTrace( TRACE_COND, wxS( "[HasCustomQuerySelected] row[%zu] type=%d hasCustom=%d" ),
                    i, static_cast<int>( m_conditions[i].panel->GetConditionType() ), hasCustom ? 1 : 0 );

        if( hasCustom )
            return true;
    }

    return false;
}


void DRC_RE_CONDITION_GROUP_PANEL::addConditionRow( BOOL_OPERATOR aOp )
{
    wxLogTrace( TRACE_COND, wxS( "[addConditionRow] START, existing rows=%zu" ), m_conditions.size() );

    CONDITION_ENTRY entry;
    entry.boolOp = aOp;
    entry.operatorChoice = nullptr;
    entry.rowSizer = new wxBoxSizer( wxHORIZONTAL );

    // Add operator choice for non-first rows
    if( !m_conditions.empty() )
    {
        wxArrayString operators;
        operators.Add( _( "AND" ) );
        operators.Add( _( "OR" ) );
        operators.Add( _( "AND NOT" ) );
        operators.Add( _( "OR NOT" ) );

        entry.operatorChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, operators );
        entry.operatorChoice->SetSelection( static_cast<int>( aOp ) );
        entry.rowSizer->Add( entry.operatorChoice, 0, wxALL | wxALIGN_TOP, 2 );

        entry.operatorChoice->Bind( wxEVT_CHOICE,
                [this, &entry]( wxCommandEvent& )
                {
                    // Find this entry and update its operator
                    for( size_t i = 0; i < m_conditions.size(); ++i )
                    {
                        if( m_conditions[i].operatorChoice == entry.operatorChoice )
                        {
                            m_conditions[i].boolOp = static_cast<BOOL_OPERATOR>(
                                    entry.operatorChoice->GetSelection() );
                            break;
                        }
                    }

                    if( m_changeCallback && !m_suppressCallbacks )
                        m_changeCallback();
                } );
    }

    wxLogTrace( TRACE_COND, wxS( "[addConditionRow] creating row panel" ) );
    entry.panel = new DRC_RE_CONDITION_ROW_PANEL( this, m_board, m_twoObjectConstraint );
    wxLogTrace( TRACE_COND, wxS( "[addConditionRow] adding row panel to rowSizer" ) );
    entry.rowSizer->Add( entry.panel, 1, wxEXPAND | wxALL, 0 );

    // Set up callbacks
    int rowIndex = static_cast<int>( m_conditions.size() );

    entry.panel->SetDeleteCallback(
            [this, rowIndex]()
            {
                removeConditionRow( rowIndex );
            } );

    entry.panel->SetChangeCallback(
            [this]()
            {
                if( m_changeCallback && !m_suppressCallbacks )
                    m_changeCallback();
            } );

    m_conditions.push_back( entry );

    // Insert before the add button
    m_mainSizer->Insert( m_mainSizer->GetItemCount() - 1, entry.rowSizer, 0, wxEXPAND | wxALL, 2 );

    updateDeleteButtonVisibility();
    m_mainSizer->Fit( this );
    Layout();

    // Notify parent to update layout
    if( wxWindow* parent = GetParent() )
        parent->Layout();
}


void DRC_RE_CONDITION_GROUP_PANEL::removeConditionRow( int aIndex )
{
    if( aIndex < 0 || aIndex >= static_cast<int>( m_conditions.size() ) )
        return;

    // Don't remove the last row
    if( m_conditions.size() <= 1 )
        return;

    CONDITION_ENTRY& entry = m_conditions[aIndex];

    // Remove row sizer from main sizer and destroy widgets
    m_mainSizer->Detach( entry.rowSizer );

    if( entry.operatorChoice )
        entry.operatorChoice->Destroy();

    entry.panel->Destroy();
    delete entry.rowSizer;

    m_conditions.erase( m_conditions.begin() + aIndex );

    // Update delete callbacks with new indices
    for( size_t i = 0; i < m_conditions.size(); ++i )
    {
        int newIndex = static_cast<int>( i );

        m_conditions[i].panel->SetDeleteCallback(
                [this, newIndex]()
                {
                    removeConditionRow( newIndex );
                } );
    }

    updateDeleteButtonVisibility();
    m_mainSizer->Fit( this );
    Layout();

    // Notify parent to update layout
    if( wxWindow* parent = GetParent() )
        parent->Layout();

    if( m_changeCallback && !m_suppressCallbacks )
        m_changeCallback();
}


void DRC_RE_CONDITION_GROUP_PANEL::onAddClicked( wxCommandEvent& aEvent )
{
    addConditionRow( BOOL_OPERATOR::AND );

    if( m_changeCallback && !m_suppressCallbacks )
        m_changeCallback();
}


void DRC_RE_CONDITION_GROUP_PANEL::rebuildLayout()
{
    // Add button should always be at the end
    m_mainSizer->Detach( m_addBtn );
    m_mainSizer->Add( m_addBtn, 0, wxALL | wxALIGN_RIGHT, 5 );

    Layout();
}


void DRC_RE_CONDITION_GROUP_PANEL::updateDeleteButtonVisibility()
{
    // Hide delete button if only one row
    bool showDelete = m_conditions.size() > 1;

    for( CONDITION_ENTRY& entry : m_conditions )
        entry.panel->ShowDeleteButton( showDelete );
}


bool DRC_RE_CONDITION_GROUP_PANEL::tokenizeCondition(
        const wxString& aExpr, std::vector<std::pair<BOOL_OPERATOR, wxString>>& aParts )
{
    wxString remaining = aExpr;
    BOOL_OPERATOR nextOp = BOOL_OPERATOR::AND;
    bool first = true;

    while( !remaining.IsEmpty() )
    {
        remaining.Trim( false );

        if( remaining.IsEmpty() )
            break;

        // Look for operator at start (except for first part)
        if( !first )
        {
            if( remaining.StartsWith( "&&" ) )
            {
                remaining = remaining.Mid( 2 ).Trim( false );

                if( remaining.StartsWith( "!" ) )
                {
                    nextOp = BOOL_OPERATOR::AND_NOT;
                    remaining = remaining.Mid( 1 ).Trim( false );
                }
                else
                {
                    nextOp = BOOL_OPERATOR::AND;
                }
            }
            else if( remaining.StartsWith( "||" ) )
            {
                remaining = remaining.Mid( 2 ).Trim( false );

                if( remaining.StartsWith( "!" ) )
                {
                    nextOp = BOOL_OPERATOR::OR_NOT;
                    remaining = remaining.Mid( 1 ).Trim( false );
                }
                else
                {
                    nextOp = BOOL_OPERATOR::OR;
                }
            }
            else
            {
                // Expected operator not found
                return false;
            }
        }

        // Find end of this expression (next operator at depth 0)
        int depth = 0;
        bool inQuote = false;
        size_t end = 0;

        for( size_t i = 0; i < remaining.length(); ++i )
        {
            wxChar c = remaining[i];

            if( c == '\\' && i + 1 < remaining.length() )
            {
                i++;  // Skip escaped char
                continue;
            }

            if( c == '\'' )
            {
                inQuote = !inQuote;
                continue;
            }

            if( inQuote )
                continue;

            if( c == '(' )
                depth++;

            if( c == ')' )
                depth--;

            if( depth == 0 && i + 1 < remaining.length() )
            {
                if( remaining.Mid( i, 2 ) == "&&" || remaining.Mid( i, 2 ) == "||" )
                {
                    end = i;
                    break;
                }
            }
        }

        if( end == 0 )
            end = remaining.length();

        wxString part = remaining.Left( end ).Trim( true ).Trim( false );
        aParts.push_back( { nextOp, part } );

        remaining = remaining.Mid( end );
        first = false;
    }

    return !aParts.empty();
}
