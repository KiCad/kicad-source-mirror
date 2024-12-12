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

#include "drc_re_condition_row_panel.h"

#include <board.h>
#include <widgets/net_selector.h>
#include <widgets/netclass_selector.h>
#include <widgets/area_selector.h>
#include <bitmaps.h>

#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/bmpbuttn.h>
#include <wx/stc/stc.h>
#include <wx/regex.h>
#include <wx/log.h>

static constexpr const wxChar* TRACE_COND = wxT( "KI_TRACE_DRC_RULE_EDITOR" );


DRC_RE_CONDITION_ROW_PANEL::DRC_RE_CONDITION_ROW_PANEL( wxWindow* aParent, BOARD* aBoard,
                                                        bool aShowObjectSelector ) :
        wxPanel( aParent ),
        m_showObjectSelector( aShowObjectSelector )
{
    wxLogTrace( TRACE_COND, wxS( "[DRC_RE_CONDITION_ROW_PANEL] ctor START" ) );

    // Outer horizontal sizer: content on left, delete button on right
    m_mainSizer = new wxBoxSizer( wxHORIZONTAL );

    // Content sizer holds row of dropdowns and custom query text below
    m_contentSizer = new wxBoxSizer( wxVERTICAL );
    m_rowSizer = new wxBoxSizer( wxHORIZONTAL );

    // Object selector (A/B) - only shown for two-object constraints
    wxArrayString objectChoices;
    objectChoices.Add( _( "Object A" ) );
    objectChoices.Add( _( "Object B" ) );

    m_objectChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, objectChoices );
    m_objectChoice->SetSelection( 0 );
    m_rowSizer->Add( m_objectChoice, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2 );

    if( !aShowObjectSelector )
        m_objectChoice->Hide();

    // Condition type dropdown
    wxArrayString conditionChoices;
    conditionChoices.Add( _( "Any" ) );
    conditionChoices.Add( _( "Net" ) );
    conditionChoices.Add( _( "Netclass" ) );
    conditionChoices.Add( _( "Within Area" ) );
    conditionChoices.Add( _( "Custom Query" ) );

    m_conditionChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, conditionChoices );
    m_conditionChoice->SetSelection( 0 );
    m_rowSizer->Add( m_conditionChoice, 0, wxALL | wxALIGN_CENTER_VERTICAL, 2 );

    // Net selector
    m_netSelector = new NET_SELECTOR( this, wxID_ANY );
    m_netSelector->SetNetInfo( &aBoard->GetNetInfo() );
    m_rowSizer->Add( m_netSelector, 1, wxALL | wxEXPAND, 2 );
    m_netSelector->Hide();

    // Netclass selector
    m_netclassSelector = new NETCLASS_SELECTOR( this, wxID_ANY );
    m_netclassSelector->SetBoard( aBoard );
    m_rowSizer->Add( m_netclassSelector, 1, wxALL | wxEXPAND, 2 );
    m_netclassSelector->Hide();

    // Area selector
    m_areaSelector = new AREA_SELECTOR( this, wxID_ANY );
    m_areaSelector->SetBoard( aBoard );
    m_rowSizer->Add( m_areaSelector, 1, wxALL | wxEXPAND, 2 );
    m_areaSelector->Hide();

    m_contentSizer->Add( m_rowSizer, 0, wxEXPAND, 0 );

    // Custom query text control - shown only when Custom Query is selected
    m_customQueryCtrl = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxSize( -1, 60 ) );
    m_customQueryCtrl->SetUseTabs( true );
    m_customQueryCtrl->SetTabWidth( 4 );
    m_customQueryCtrl->SetIndent( 4 );
    m_customQueryCtrl->SetTabIndents( true );
    m_customQueryCtrl->SetBackSpaceUnIndents( true );
    m_customQueryCtrl->SetViewEOL( false );
    m_customQueryCtrl->SetViewWhiteSpace( false );
    m_customQueryCtrl->SetIndentationGuides( true );
    m_customQueryCtrl->SetReadOnly( false );
    m_customQueryCtrl->SetUseHorizontalScrollBar( false );
    m_customQueryCtrl->SetMaxSize( wxSize( -1, 60 ) );
    m_contentSizer->Add( m_customQueryCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
    m_customQueryCtrl->Hide();

    m_mainSizer->Add( m_contentSizer, 1, wxEXPAND, 0 );

    // Delete button on right side, aligned to top
    m_deleteBtn = new wxBitmapButton( this, wxID_ANY, KiBitmapBundle( BITMAPS::trash ) );
    m_deleteBtn->SetToolTip( _( "Remove this condition" ) );
    m_mainSizer->Add( m_deleteBtn, 0, wxALL | wxALIGN_TOP, 2 );

    wxLogTrace( TRACE_COND, wxS( "[DRC_RE_CONDITION_ROW_PANEL] setting sizer" ) );
    SetSizer( m_mainSizer );

    // Bind events
    m_objectChoice->Bind( wxEVT_CHOICE, &DRC_RE_CONDITION_ROW_PANEL::onObjectChoice, this );
    m_conditionChoice->Bind( wxEVT_CHOICE, &DRC_RE_CONDITION_ROW_PANEL::onConditionChoice, this );
    m_deleteBtn->Bind( wxEVT_BUTTON, &DRC_RE_CONDITION_ROW_PANEL::onDeleteClicked, this );
    wxLogTrace( TRACE_COND, wxS( "[DRC_RE_CONDITION_ROW_PANEL] ctor END" ) );
}


void DRC_RE_CONDITION_ROW_PANEL::SetObjectTarget( OBJECT_TARGET aTarget )
{
    m_objectChoice->SetSelection( static_cast<int>( aTarget ) );
}


DRC_RE_CONDITION_ROW_PANEL::OBJECT_TARGET DRC_RE_CONDITION_ROW_PANEL::GetObjectTarget() const
{
    return static_cast<OBJECT_TARGET>( m_objectChoice->GetSelection() );
}


void DRC_RE_CONDITION_ROW_PANEL::SetConditionType( CONDITION_TYPE aType )
{
    m_conditionChoice->SetSelection( static_cast<int>( aType ) );
    updateVisibility();
}


DRC_RE_CONDITION_ROW_PANEL::CONDITION_TYPE DRC_RE_CONDITION_ROW_PANEL::GetConditionType() const
{
    return static_cast<CONDITION_TYPE>( m_conditionChoice->GetSelection() );
}


void DRC_RE_CONDITION_ROW_PANEL::SetValue( const wxString& aValue )
{
    switch( GetConditionType() )
    {
    case CONDITION_TYPE::NET:
        m_netSelector->SetSelectedNet( aValue );
        break;

    case CONDITION_TYPE::NETCLASS:
        m_netclassSelector->SetSelectedNetclass( aValue );
        break;

    case CONDITION_TYPE::WITHIN_AREA:
        m_areaSelector->SetSelectedArea( aValue );
        break;

    case CONDITION_TYPE::CUSTOM:
        m_customQueryCtrl->SetValue( aValue );
        break;

    default:
        break;
    }
}


wxString DRC_RE_CONDITION_ROW_PANEL::GetValue() const
{
    switch( GetConditionType() )
    {
    case CONDITION_TYPE::NET:
        return m_netSelector->GetSelectedNetname();

    case CONDITION_TYPE::NETCLASS:
        return m_netclassSelector->GetSelectedNetclass();

    case CONDITION_TYPE::WITHIN_AREA:
        return m_areaSelector->GetSelectedArea();

    case CONDITION_TYPE::CUSTOM:
        return m_customQueryCtrl->GetText();

    default:
        return wxString();
    }
}


bool DRC_RE_CONDITION_ROW_PANEL::ParseExpression( const wxString& aExpr )
{
    wxLogTrace( TRACE_COND, wxS( "[ParseExpression] expr='%s'" ), aExpr );

    if( aExpr.IsEmpty() )
    {
        wxLogTrace( TRACE_COND, wxS( "[ParseExpression] empty -> ANY" ) );
        SetConditionType( CONDITION_TYPE::ANY );
        return true;
    }

    wxString expr = aExpr;

    // Check for object prefix (A. or B.)
    if( expr.StartsWith( "A." ) )
    {
        SetObjectTarget( OBJECT_TARGET::OBJECT_A );
        expr = expr.Mid( 2 );
    }
    else if( expr.StartsWith( "B." ) )
    {
        SetObjectTarget( OBJECT_TARGET::OBJECT_B );
        expr = expr.Mid( 2 );
    }

    // Try to match known patterns
    wxRegEx netRe( wxT( "^NetName\\s*==\\s*'([^']*)'" ) );
    wxRegEx netclassRe1( wxT( "^hasNetclass\\('([^']*)'\\)" ) );
    wxRegEx netclassRe2( wxT( "^NetClass\\s*==\\s*'([^']*)'" ) );
    wxRegEx areaRe( wxT( "^(?:enclosedByArea|intersectsArea)\\('([^']*)'\\)" ) );

    if( netRe.Matches( expr ) )
    {
        wxLogTrace( TRACE_COND, wxS( "[ParseExpression] matched NET pattern" ) );
        SetConditionType( CONDITION_TYPE::NET );
        m_netSelector->SetSelectedNet( netRe.GetMatch( expr, 1 ) );
        return true;
    }
    else if( netclassRe1.Matches( expr ) )
    {
        wxLogTrace( TRACE_COND, wxS( "[ParseExpression] matched NETCLASS (hasNetclass) pattern" ) );
        SetConditionType( CONDITION_TYPE::NETCLASS );
        m_netclassSelector->SetSelectedNetclass( netclassRe1.GetMatch( expr, 1 ) );
        return true;
    }
    else if( netclassRe2.Matches( expr ) )
    {
        wxLogTrace( TRACE_COND, wxS( "[ParseExpression] matched NETCLASS (NetClass==) pattern" ) );
        SetConditionType( CONDITION_TYPE::NETCLASS );
        m_netclassSelector->SetSelectedNetclass( netclassRe2.GetMatch( expr, 1 ) );
        return true;
    }
    else if( areaRe.Matches( expr ) )
    {
        wxLogTrace( TRACE_COND, wxS( "[ParseExpression] matched WITHIN_AREA pattern" ) );
        SetConditionType( CONDITION_TYPE::WITHIN_AREA );
        m_areaSelector->SetSelectedArea( areaRe.GetMatch( expr, 1 ) );
        return true;
    }

    // Fallback to custom query
    wxLogTrace( TRACE_COND, wxS( "[ParseExpression] no match -> CUSTOM" ) );
    SetConditionType( CONDITION_TYPE::CUSTOM );
    m_customQueryCtrl->SetValue( aExpr );

    return true;
}


wxString DRC_RE_CONDITION_ROW_PANEL::BuildExpression() const
{
    wxString prefix = m_showObjectSelector
                          ? ( GetObjectTarget() == OBJECT_TARGET::OBJECT_A ? "A" : "B" )
                          : "A";

    switch( GetConditionType() )
    {
    case CONDITION_TYPE::NET:
    {
        wxString net = m_netSelector->GetSelectedNetname();

        if( net.IsEmpty() )
            return wxEmptyString;

        return prefix + wxString::Format( ".NetName == '%s'", net );
    }

    case CONDITION_TYPE::NETCLASS:
    {
        wxString netclass = m_netclassSelector->GetSelectedNetclass();

        if( netclass.IsEmpty() )
            return wxEmptyString;

        return prefix + wxString::Format( ".hasNetclass('%s')", netclass );
    }

    case CONDITION_TYPE::WITHIN_AREA:
    {
        wxString area = m_areaSelector->GetSelectedArea();

        if( area.IsEmpty() )
            return wxEmptyString;

        return prefix + wxString::Format( ".intersectsArea('%s')", area );
    }

    case CONDITION_TYPE::CUSTOM:
        return m_customQueryCtrl->GetText();

    default:
        return wxEmptyString;
    }
}


void DRC_RE_CONDITION_ROW_PANEL::ShowDeleteButton( bool aShow )
{
    m_deleteBtn->Show( aShow );
    Layout();
}


bool DRC_RE_CONDITION_ROW_PANEL::HasCustomQuerySelected() const
{
    return GetConditionType() == CONDITION_TYPE::CUSTOM;
}


void DRC_RE_CONDITION_ROW_PANEL::onObjectChoice( wxCommandEvent& aEvent )
{
    if( m_changeCallback )
        m_changeCallback();
}


void DRC_RE_CONDITION_ROW_PANEL::onConditionChoice( wxCommandEvent& aEvent )
{
    updateVisibility();

    if( m_changeCallback )
        m_changeCallback();
}


void DRC_RE_CONDITION_ROW_PANEL::onDeleteClicked( wxCommandEvent& aEvent )
{
    if( m_deleteCallback )
        m_deleteCallback();
}


void DRC_RE_CONDITION_ROW_PANEL::updateVisibility()
{
    m_netSelector->Hide();
    m_netclassSelector->Hide();
    m_areaSelector->Hide();
    m_customQueryCtrl->Hide();

    switch( GetConditionType() )
    {
    case CONDITION_TYPE::NET:
        m_netSelector->Show();
        break;

    case CONDITION_TYPE::NETCLASS:
        m_netclassSelector->Show();
        break;

    case CONDITION_TYPE::WITHIN_AREA:
        m_areaSelector->Show();
        break;

    case CONDITION_TYPE::CUSTOM:
        m_customQueryCtrl->Show();
        break;

    default:
        break;
    }

    Layout();

    // Notify parent to update layout since our size may have changed
    if( wxWindow* parent = GetParent() )
        parent->Layout();
}
