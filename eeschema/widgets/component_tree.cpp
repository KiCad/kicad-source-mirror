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
#include <wx/wupdlock.h>

#include <symbol_lib_table.h>


COMPONENT_TREE::COMPONENT_TREE( wxWindow* aParent, SYMBOL_LIB_TABLE* aSymLibTable,
        CMP_TREE_MODEL_ADAPTER_BASE::PTR& aAdapter, WIDGETS aWidgets )
    : wxPanel( aParent ),
      m_sym_lib_table( aSymLibTable ),
      m_adapter( aAdapter ),
      m_query_ctrl( nullptr ),
      m_details_ctrl( nullptr ),
      m_filtering( false )
{
    // create space for context menu pointers, INVALID is the max value
    m_menus.resize( CMP_TREE_NODE::TYPE::INVALID + 1 );

    auto sizer = new wxBoxSizer( wxVERTICAL );

    // Search text control
    if( aWidgets & SEARCH )
    {
        auto search_sizer = new wxBoxSizer( wxHORIZONTAL );

        m_query_ctrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                       wxDefaultSize, wxTE_PROCESS_ENTER );

// Additional visual cue for GTK, which hides the placeholder text on focus
#ifdef __WXGTK__
        search_sizer->Add( new wxStaticBitmap( this, wxID_ANY,
                                wxArtProvider::GetBitmap( wxART_FIND, wxART_FRAME_ICON ) ),
                0, wxALIGN_CENTER | wxALL, 5 );
#endif

        search_sizer->Add( m_query_ctrl, 1, wxALL | wxEXPAND, 5 );
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

    m_tree_ctrl->Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &COMPONENT_TREE::onTreeActivate, this );
    m_tree_ctrl->Bind( wxEVT_DATAVIEW_SELECTION_CHANGED, &COMPONENT_TREE::onTreeSelect, this );
    m_tree_ctrl->Bind( wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, &COMPONENT_TREE::onContextMenu, this );

    Bind( COMPONENT_PRESELECTED, &COMPONENT_TREE::onPreselect, this );

    // If wxTextCtrl::SetHint() is called before binding wxEVT_TEXT, the event
    // handler will intermittently fire.
    if( m_query_ctrl )
    {
        m_query_ctrl->SetHint( _( "Search" ) );
        m_query_ctrl->SetFocus();
        m_query_ctrl->SetValue( wxEmptyString );
    }

    // There may be a part preselected in the model. Make sure it is displayed.
    postPreselectEvent();

    Layout();
    sizer->Fit( this );
}


LIB_ID COMPONENT_TREE::GetSelectedLibId( int* aUnit ) const
{
    auto sel = m_tree_ctrl->GetSelection();

    if( !sel )
    {
        LIB_ID emptyId;

        return emptyId;
    }

    if( aUnit )
        *aUnit = m_adapter->GetUnitFor( sel );

    return m_adapter->GetAliasFor( sel );
}


void COMPONENT_TREE::SelectLibId( const LIB_ID& aLibId )
{
    selectIfValid( m_adapter->FindItem( aLibId ) );
}


void COMPONENT_TREE::Regenerate()
{
    STATE current;

    // Store the state
    if( !m_filtering )
        m_unfilteredState = getState();
    else
        current = getState();

    wxString filter = m_query_ctrl->GetValue();
    m_adapter->UpdateSearchString( filter );
    m_filtering = !filter.IsEmpty();
    postPreselectEvent();

    // Restore the state
    setState( m_filtering ? current : m_unfilteredState );
}


void COMPONENT_TREE::selectIfValid( const wxDataViewItem& aTreeId )
{
    if( aTreeId.IsOk() )
    {
        m_tree_ctrl->EnsureVisible( aTreeId );
        m_tree_ctrl->Select( aTreeId );
        postPreselectEvent();
    }
}


void COMPONENT_TREE::postPreselectEvent()
{
    wxCommandEvent event( COMPONENT_PRESELECTED );
    wxPostEvent( this, event );
}


void COMPONENT_TREE::postSelectEvent()
{
    wxCommandEvent event( COMPONENT_SELECTED );
    wxPostEvent( this, event );
}


COMPONENT_TREE::STATE COMPONENT_TREE::getState() const
{
    STATE state;
    wxDataViewItemArray items;
    m_adapter->GetChildren( wxDataViewItem( nullptr ), items );

    for( const auto& item : items )
    {
        if( m_tree_ctrl->IsExpanded( item ) )
            state.expanded.push_back( item );
    }

    state.selection = GetSelectedLibId();

    return state;
}


void COMPONENT_TREE::setState( const STATE& aState )
{
    wxWindowUpdateLocker updateLock( m_tree_ctrl );

    for( const auto& item : aState.expanded )
        m_tree_ctrl->Expand( item );

    if( aState.selection.IsValid() )
        SelectLibId( aState.selection );
}


void COMPONENT_TREE::onQueryText( wxCommandEvent& aEvent )
{
    Regenerate();

    // Required to avoid interaction with SetHint()
    // See documentation for wxTextEntry::SetHint
    aEvent.Skip();
}


void COMPONENT_TREE::onQueryEnter( wxCommandEvent& aEvent )
{
    if( GetSelectedLibId().IsValid() )
        postSelectEvent();
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
    postPreselectEvent();
}


void COMPONENT_TREE::onTreeActivate( wxDataViewEvent& aEvent )
{
    if( !GetSelectedLibId().IsValid() )
    {
        // Expand library/part units subtree
        auto const sel = m_tree_ctrl->GetSelection();

        if( m_tree_ctrl->IsExpanded( sel ) )
            m_tree_ctrl->Collapse( sel );
        else
            m_tree_ctrl->Expand( sel );
    }
    else
    {
        postSelectEvent();
    }
}


void COMPONENT_TREE::onDetailsLink( wxHtmlLinkEvent& aEvent )
{
    const wxHtmlLinkInfo& info = aEvent.GetLinkInfo();
    ::wxLaunchDefaultBrowser( info.GetHref() );
}


void COMPONENT_TREE::onPreselect( wxCommandEvent& aEvent )
{
    if( m_details_ctrl )
    {
        int unit = 0;
        LIB_ID id = GetSelectedLibId( &unit );

        if( id.IsValid() )
            m_details_ctrl->SetPage( GenerateAliasInfo( m_sym_lib_table, id, unit ) );
        else
            m_details_ctrl->SetPage( wxEmptyString );
    }

    aEvent.Skip();
}


void COMPONENT_TREE::onContextMenu( wxDataViewEvent& aEvent )
{
    auto const sel = m_tree_ctrl->GetSelection();
    auto type = sel.IsOk() ? m_adapter->GetTypeFor( sel ) : CMP_TREE_NODE::INVALID;

    if( m_menus[type] )
    {
        m_menuActive = true;
        PopupMenu( m_menus[type].get() );
        m_menuActive = false;
    }
}


wxDEFINE_EVENT( COMPONENT_PRESELECTED, wxCommandEvent );
wxDEFINE_EVENT( COMPONENT_SELECTED, wxCommandEvent );
