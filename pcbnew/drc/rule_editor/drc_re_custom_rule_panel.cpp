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

#include "drc_re_custom_rule_panel.h"

#include <stack>

#include <scintilla_tricks.h>
#include <wx/sizer.h>
#include <wx/stc/stc.h>

DRC_RE_CUSTOM_RULE_PANEL::DRC_RE_CUSTOM_RULE_PANEL(
        wxWindow* aParent, std::shared_ptr<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA> aConstraintData ) :
        wxPanel( aParent ),
        m_constraintData( aConstraintData )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    m_textCtrl = new wxStyledTextCtrl( this, wxID_ANY );
    sizer->Add( m_textCtrl, 1, wxEXPAND | wxALL, 5 );
    SetSizer( sizer );

    m_scintillaTricks = std::make_unique<SCINTILLA_TRICKS>(
            m_textCtrl, wxT( "()" ), false,
            // onAcceptFn
            []( wxKeyEvent& aEvent )
            {
                aEvent.Skip();
            },
            // onCharFn
            [this]( wxStyledTextEvent& aEvent )
            {
                onScintillaCharAdded( aEvent );
            } );

    m_textCtrl->AutoCompSetSeparator( '|' );
}


DRC_RE_CUSTOM_RULE_PANEL::~DRC_RE_CUSTOM_RULE_PANEL()
{
}


bool DRC_RE_CUSTOM_RULE_PANEL::TransferDataToWindow()
{
    if( m_constraintData )
        m_textCtrl->SetValue( m_constraintData->GetRuleText() );

    return true;
}


bool DRC_RE_CUSTOM_RULE_PANEL::TransferDataFromWindow()
{
    if( m_constraintData )
        m_constraintData->SetRuleText( m_textCtrl->GetValue() );

    return true;
}


bool DRC_RE_CUSTOM_RULE_PANEL::ValidateInputs( int* aErrorCount, std::string* aValidationMessage )
{
    (void) aErrorCount;
    (void) aValidationMessage;
    return true;
}


wxString DRC_RE_CUSTOM_RULE_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    (void) aContext;

    if( m_constraintData )
        return m_constraintData->GetRuleText();

    if( m_textCtrl )
        return m_textCtrl->GetValue();

    return wxEmptyString;
}


void DRC_RE_CUSTOM_RULE_PANEL::onScintillaCharAdded( wxStyledTextEvent& aEvent )
{
    if( aEvent.GetModifiers() == wxMOD_CONTROL && aEvent.GetKey() == ' ' )
    {
        // Short-cut for do-auto-complete
    }

    m_textCtrl->SearchAnchor();

    wxString rules = m_textCtrl->GetText();
    int      currentPos = m_textCtrl->GetCurrentPos();
    int      startPos = 0;

    enum class EXPR_CONTEXT_T : int
    {
        NONE,
        STRING,
        SEXPR_OPEN,
        SEXPR_TOKEN,
    };

    std::stack<wxString> sexprs;
    wxString             partial;
    EXPR_CONTEXT_T       context = EXPR_CONTEXT_T::NONE;

    for( int i = startPos; i < currentPos; ++i )
    {
        wxChar c = m_textCtrl->GetCharAt( i );

        if( c == '\\' )
        {
            i++; // skip escaped char
        }
        else if( context == EXPR_CONTEXT_T::STRING )
        {
            if( c == '"' )
                context = EXPR_CONTEXT_T::NONE;
        }
        else if( c == '"' )
        {
            partial = wxEmptyString;
            context = EXPR_CONTEXT_T::STRING;
        }
        else if( c == '(' )
        {
            if( context == EXPR_CONTEXT_T::SEXPR_OPEN && !partial.IsEmpty() )
            {
                m_textCtrl->AutoCompCancel();
                sexprs.push( partial );
            }

            partial = wxEmptyString;
            context = EXPR_CONTEXT_T::SEXPR_OPEN;
        }
        else if( c == ')' )
        {
            if( !sexprs.empty() )
                sexprs.pop();

            context = EXPR_CONTEXT_T::NONE;
        }
        else if( c == ' ' )
        {
            if( context == EXPR_CONTEXT_T::SEXPR_OPEN && !partial.IsEmpty() )
            {
                m_textCtrl->AutoCompCancel();
                sexprs.push( partial );
                context = EXPR_CONTEXT_T::SEXPR_TOKEN;
                partial = wxEmptyString;
                continue;
            }

            context = EXPR_CONTEXT_T::NONE;
        }
        else
        {
            partial += c;
        }
    }

    wxString tokens;

    if( context == EXPR_CONTEXT_T::SEXPR_OPEN )
    {
        if( sexprs.empty() )
        {
            // Top level - suggest (rule or (version
            tokens = wxT( "rule|version" );
        }
        else if( sexprs.top() == wxT( "rule" ) )
        {
            // Inside (rule ...) - suggest sub-expressions
            tokens = wxT( "condition|constraint|layer|severity" );
        }
        else if( sexprs.top() == wxT( "constraint" ) )
        {
            // Inside (constraint ...) - suggest constraint parameters
            tokens = wxT( "min|max|opt" );
        }
    }
    else if( context == EXPR_CONTEXT_T::SEXPR_TOKEN )
    {
        if( !sexprs.empty() && sexprs.top() == wxT( "constraint" ) )
        {
            // After (constraint - suggest constraint types
            tokens = wxT( "annular_width|"
                          "assertion|"
                          "clearance|"
                          "connection_width|"
                          "courtyard_clearance|"
                          "creepage|"
                          "diff_pair_gap|"
                          "diff_pair_uncoupled|"
                          "disallow|"
                          "edge_clearance|"
                          "hole_clearance|"
                          "hole_size|"
                          "hole_to_hole|"
                          "length|"
                          "max_error|"
                          "min_resolved_spokes|"
                          "physical_clearance|"
                          "physical_hole_clearance|"
                          "silk_clearance|"
                          "skew|"
                          "text_height|"
                          "text_thickness|"
                          "thermal_relief_gap|"
                          "thermal_spoke_width|"
                          "track_angle|"
                          "track_segment_length|"
                          "track_width|"
                          "via_count|"
                          "via_diameter|"
                          "zone_connection" );
        }
        else if( !sexprs.empty() && sexprs.top() == wxT( "layer" ) )
        {
            // After (layer - suggest layer keywords
            tokens = wxT( "inner|outer" );
        }
        else if( !sexprs.empty() && sexprs.top() == wxT( "severity" ) )
        {
            // After (severity - suggest severity levels
            tokens = wxT( "error|warning|ignore|exclusion" );
        }
    }

    if( !tokens.IsEmpty() )
        m_scintillaTricks->DoAutocomplete( partial, wxSplit( tokens, '|' ) );
}
