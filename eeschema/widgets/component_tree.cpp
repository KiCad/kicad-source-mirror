/* -*- c++ -*-
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "component_tree.h"
#include <generate_alias_info.h>
#include <wxdataviewctrl_helpers.h>

#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/html/htmlwin.h>

COMPONENT_TREE::COMPONENT_TREE( wxWindow* aParent,
        CMP_TREE_MODEL_ADAPTER::PTR& aAdapter, WIDGETS aWidgets )
    : wxPanel( aParent ), m_adapter( aAdapter ), m_query_ctrl( nullptr ), m_details_ctrl( nullptr )
{
    auto sizer = new wxBoxSizer( wxVERTICAL );

    // Search text control
    if( aWidgets & SEARCH )
    {
        auto search_sizer = new wxBoxSizer( wxHORIZONTAL );

        m_query_ctrl = new wxTextCtrl(
                this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );

// Additional visual cue for GTK, which hides the placeholder text on focus
#ifdef __WXGTK__
        search_sizer->Add( new wxStaticBitmap( this, wxID_ANY,
                                wxArtProvider::GetBitmap( wxART_FIND, wxART_FRAME_ICON ) ),
                0, wxALIGN_CENTER | wxALL, 5 );
#endif

        search_sizer->Add( m_query_ctrl, 1, wxALIGN_CENTER | wxALL | wxEXPAND, 5 );
        sizer->Add( search_sizer, 0, wxEXPAND, 5 );

        m_query_ctrl->Bind( wxEVT_TEXT, &COMPONENT_TREE::onQueryText, this );
        m_query_ctrl->Bind( wxEVT_TEXT_ENTER, &COMPONENT_TREE::onQueryEnter, this );
        m_query_ctrl->Bind( wxEVT_CHAR_HOOK, &COMPONENT_TREE::onQueryCharHook, this );
    }

    // Component tree
    m_tree_ctrl =
            new wxDataViewCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_SINGLE );
    m_adapter->AttachTo( m_tree_ctrl );

    sizer->Add( m_tree_ctrl, 1, wxALL | wxEXPAND, 5 );

    // Description panel
    if( aWidgets & DETAILS )
    {
        m_details_ctrl = new wxHtmlWindow( this, wxID_ANY, wxDefaultPosition, wxSize( 320, 240 ),
                wxHW_SCROLLBAR_AUTO | wxSUNKEN_BORDER );

        sizer->Add( m_details_ctrl, 1, wxALL | wxEXPAND, 5 );
        m_details_ctrl->Bind( wxEVT_HTML_LINK_CLICKED, &COMPONENT_TREE::onDetailsLink, this );
    }

    SetSizer( sizer );

    Bind( wxEVT_INIT_DIALOG, &COMPONENT_TREE::onInitDialog, this );
    m_tree_ctrl->Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &COMPONENT_TREE::onTreeActivate, this );
    m_tree_ctrl->Bind( wxEVT_DATAVIEW_SELECTION_CHANGED, &COMPONENT_TREE::onTreeSelect, this );

    Layout();
    sizer->Fit( this );
}


LIB_ALIAS* COMPONENT_TREE::GetSelectedAlias( int* aUnit ) const
{
    auto sel = m_tree_ctrl->GetSelection();

    if( aUnit )
        *aUnit = m_adapter->GetUnitFor( sel );

    return m_adapter->GetAliasFor( sel );
}


void COMPONENT_TREE::selectIfValid( const wxDataViewItem& aTreeId )
{
    if( aTreeId.IsOk() )
    {
        m_tree_ctrl->EnsureVisible( aTreeId );
        m_tree_ctrl->Select( aTreeId );
        postSelectEvent();
    }
}


void COMPONENT_TREE::postSelectEvent()
{
    wxDataViewEvent evt( wxEVT_DATAVIEW_SELECTION_CHANGED );
    m_tree_ctrl->GetEventHandler()->ProcessEvent( evt );
}


void COMPONENT_TREE::handleSubtree()
{
    if( !GetSelectedAlias() )
    {
        // Expand library/part units subtree
        auto const sel = m_tree_ctrl->GetSelection();

        if( m_tree_ctrl->IsExpanded( sel ) )
            m_tree_ctrl->Collapse( sel );
        else
            m_tree_ctrl->Expand( sel );
    }
}


void COMPONENT_TREE::onInitDialog( wxInitDialogEvent& aEvent )
{
    // If wxTextCtrl::SetHint() is called before binding wxEVT_TEXT, the event
    // handler will intermittently fire.
    if( m_query_ctrl )
    {
        m_query_ctrl->SetHint( _( "Search" ) );
        m_query_ctrl->SetFocus();
        m_query_ctrl->SetValue( wxEmptyString );
    }

    // There may be a part preselected in the model. Make sure it is displayed.
    postSelectEvent();
}


void COMPONENT_TREE::onQueryText( wxCommandEvent& aEvent )
{
    m_adapter->UpdateSearchString( m_query_ctrl->GetLineText( 0 ) );
    postSelectEvent();

    // Required to avoid interaction with SetHint()
    // See documentation for wxTextEntry::SetHint
    aEvent.Skip();
}


void COMPONENT_TREE::onQueryEnter( wxCommandEvent& aEvent )
{
    handleSubtree();
}


void COMPONENT_TREE::onQueryCharHook( wxKeyEvent& aKeyStroke )
{
    auto const sel = m_tree_ctrl->GetSelection();

    switch( aKeyStroke.GetKeyCode() )
    {
    case WXK_UP: selectIfValid( GetPrevItem( *m_tree_ctrl, sel ) ); break;

    case WXK_DOWN: selectIfValid( GetNextItem( *m_tree_ctrl, sel ) ); break;

    default:
        aKeyStroke.Skip(); // Any other key: pass on to search box directly.
        break;
    }
}


void COMPONENT_TREE::onTreeSelect( wxDataViewEvent& aEvent )
{
    if( m_details_ctrl )
    {
        int unit = 0;
        LIB_ALIAS* alias = GetSelectedAlias( &unit );

        if( alias )
            m_details_ctrl->SetPage( GenerateAliasInfo( alias, unit ) );
        else
            m_details_ctrl->SetPage( wxEmptyString );
    }

    aEvent.Skip();
}


void COMPONENT_TREE::onTreeActivate( wxDataViewEvent& aEvent )
{
    handleSubtree();
    aEvent.Skip();
}


void COMPONENT_TREE::onDetailsLink( wxHtmlLinkEvent& aEvent )
{
    const wxHtmlLinkInfo& info = aEvent.GetLinkInfo();
    ::wxLaunchDefaultBrowser( info.GetHref() );
}
