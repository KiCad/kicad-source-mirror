/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/lib_tree.h>
#include <widgets/std_bitmap_button.h>
#include <core/kicad_algo.h>
#include <macros.h>
#include <bitmaps.h>
#include <dialogs/eda_reorderable_list_dialog.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <tool/action_manager.h>
#include <tool/actions.h>
#include <widgets/wx_dataviewctrl.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/srchctrl.h>
#include <wx/timer.h>

constexpr int RECENT_SEARCHES_MAX = 10;

std::map<wxString, std::vector<wxString>> g_recentSearches;


LIB_TREE::LIB_TREE( wxWindow* aParent, const wxString& aRecentSearchesKey, LIB_TABLE* aLibTable,
                    wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>& aAdapter, int aFlags,
                    HTML_WINDOW* aDetails ) :
        wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                 wxWANTS_CHARS | wxTAB_TRAVERSAL | wxNO_BORDER ),
        m_adapter( aAdapter ),
        m_query_ctrl( nullptr ),
        m_sort_ctrl( nullptr ),
        m_details_ctrl( nullptr ),
        m_inTimerEvent( false ),
        m_recentSearchesKey( aRecentSearchesKey ),
        m_skipNextRightClick( false )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    // Search text control
    if( aFlags & SEARCH )
    {
        wxBoxSizer* search_sizer = new wxBoxSizer( wxHORIZONTAL );

        m_query_ctrl = new wxSearchCtrl( this, wxID_ANY );

        m_query_ctrl->ShowCancelButton( true );

#ifdef __WXGTK__
        // wxSearchCtrl vertical height is not calculated correctly on some GTK setups
        // See https://gitlab.com/kicad/code/kicad/-/issues/9019
        m_query_ctrl->SetMinSize( wxSize( -1, GetTextExtent( wxT( "qb" ) ).y + 10 ) );
#endif

        m_debounceTimer = new wxTimer( this );

        search_sizer->Add( m_query_ctrl, 1, wxEXPAND, 5 );

        m_sort_ctrl = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition,
                                             wxDefaultSize, wxBU_AUTODRAW|0 );
        m_sort_ctrl->SetBitmap( KiBitmap( BITMAPS::small_sort_desc ) );
        m_sort_ctrl->Bind( wxEVT_LEFT_DOWN,
                [&]( wxMouseEvent& aEvent )
                {
                    // Build a pop menu:
                    wxMenu menu;

                    menu.Append( 4201, _( "Sort by Best Match" ), wxEmptyString, wxITEM_CHECK );
                    menu.Append( 4202, _( "Sort Alphabetically" ), wxEmptyString, wxITEM_CHECK );

                    if( m_adapter->GetSortMode() == LIB_TREE_MODEL_ADAPTER::BEST_MATCH )
                        menu.Check( 4201, true );
                    else
                        menu.Check( 4202, true );

                    // menu_id is the selected submenu id from the popup menu or wxID_NONE
                    int menu_id = m_sort_ctrl->GetPopupMenuSelectionFromUser( menu );

                    if( menu_id == 0 || menu_id == 4201 )
                    {
                        m_adapter->SetSortMode( LIB_TREE_MODEL_ADAPTER::BEST_MATCH );
                        Regenerate( true );
                    }
                    else if( menu_id == 1 || menu_id == 4202 )
                    {
                        m_adapter->SetSortMode( LIB_TREE_MODEL_ADAPTER::ALPHABETIC );
                        Regenerate( true );
                    }
                } );

        search_sizer->Add( m_sort_ctrl, 0, wxEXPAND|wxALL, 1 );

        sizer->Add( search_sizer, 0, wxEXPAND, 5 );

        m_query_ctrl->Bind( wxEVT_TEXT, &LIB_TREE::onQueryText, this );

#if wxCHECK_VERSION( 3, 1, 1 )
        m_query_ctrl->Bind( wxEVT_SEARCH_CANCEL, &LIB_TREE::onQueryText, this );
#else
        m_query_ctrl->Bind( wxEVT_SEARCHCTRL_CANCEL_BTN, &LIB_TREE::onQueryText, this );
#endif
        m_query_ctrl->Bind( wxEVT_CHAR_HOOK, &LIB_TREE::onQueryCharHook, this );
        m_query_ctrl->Bind( wxEVT_MOTION, &LIB_TREE::onQueryMouseMoved, this );
        m_query_ctrl->Bind( wxEVT_LEAVE_WINDOW,
                            [this] ( wxMouseEvent& aEvt )
                            {
                                SetCursor( wxCURSOR_ARROW );
                            } );

        m_query_ctrl->Bind( wxEVT_MENU,
                [this]( wxCommandEvent& aEvent )
                {
                    wxString search;
                    size_t   idx = aEvent.GetId() - 1;

                    if( idx < g_recentSearches[ m_recentSearchesKey ].size() )
                        m_query_ctrl->SetValue( g_recentSearches[ m_recentSearchesKey ][idx] );
                },
                1, RECENT_SEARCHES_MAX );

        Bind( wxEVT_TIMER, &LIB_TREE::onDebounceTimer, this, m_debounceTimer->GetId() );
    }

    // Tree control
    int dvFlags = ( aFlags & MULTISELECT ) ? wxDV_MULTIPLE : wxDV_SINGLE;
    m_tree_ctrl = new WX_DATAVIEWCTRL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, dvFlags );
    m_adapter->AttachTo( m_tree_ctrl );

    if( aFlags & DETAILS )
        sizer->AddSpacer( 5 );

    sizer->Add( m_tree_ctrl, 5, wxRIGHT | wxBOTTOM | wxEXPAND, 1 );

    // Description panel
    if( aFlags & DETAILS )
    {
        if( !aDetails )
        {
            wxPoint html_size = ConvertDialogToPixels( wxPoint( 80, 80 ) );

            m_details_ctrl = new HTML_WINDOW( this, wxID_ANY, wxDefaultPosition,
                                              wxSize( html_size.x, html_size.y ),
                                              wxHW_SCROLLBAR_AUTO );

            sizer->Add( m_details_ctrl, 2, wxTOP | wxEXPAND, 5 );
        }
        else
        {
            m_details_ctrl = aDetails;
        }

        m_details_ctrl->Bind( wxEVT_HTML_LINK_CLICKED, &LIB_TREE::onDetailsLink, this );
    }

    SetSizer( sizer );

    m_tree_ctrl->Bind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &LIB_TREE::onTreeActivate, this );
    m_tree_ctrl->Bind( wxEVT_DATAVIEW_SELECTION_CHANGED, &LIB_TREE::onTreeSelect, this );
    m_tree_ctrl->Bind( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &LIB_TREE::onItemContextMenu, this );
    m_tree_ctrl->Bind( wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, &LIB_TREE::onHeaderContextMenu,
                       this );

    // Process hotkeys when the tree control has focus:
    m_tree_ctrl->Bind( wxEVT_CHAR_HOOK, &LIB_TREE::onTreeCharHook, this );

    Bind( SYMBOL_PRESELECTED, &LIB_TREE::onPreselect, this );

    if( m_query_ctrl )
    {
        m_query_ctrl->SetDescriptiveText( _( "Filter" ) );
        m_query_ctrl->SetFocus();
        m_query_ctrl->SetValue( wxEmptyString );
        updateRecentSearchMenu();

        // Force an update of the adapter with the empty text to ensure preselect is done
        Regenerate( false );
    }
    else
    {
        // There may be a part preselected in the model. Make sure it is displayed.
        // Regenerate does this in the other branch
        postPreselectEvent();
    }

    Layout();
    sizer->Fit( this );

#ifdef __WXGTK__
    // Scrollbars must be always enabled to prevent an infinite event loop
    // more details: http://trac.wxwidgets.org/ticket/18141
    if( m_details_ctrl )
        m_details_ctrl->ShowScrollbars( wxSHOW_SB_ALWAYS, wxSHOW_SB_ALWAYS );
#endif /* __WXGTK__ */
}


LIB_TREE::~LIB_TREE()
{
    // Stop the timer during destruction early to avoid potential race conditions (that do happen)
    m_debounceTimer->Stop();
}


void LIB_TREE::ShowChangedLanguage()
{
    if( m_query_ctrl )
        m_query_ctrl->SetDescriptiveText( _( "Filter" ) );

    if( m_adapter )
        m_adapter->ShowChangedLanguage();
}


LIB_ID LIB_TREE::GetSelectedLibId( int* aUnit ) const
{
    wxDataViewItem sel = m_tree_ctrl->GetSelection();

    if( !sel )
        return LIB_ID();

    if( aUnit )
        *aUnit = m_adapter->GetUnitFor( sel );

    return m_adapter->GetAliasFor( sel );
}


int LIB_TREE::GetSelectedLibIds( std::vector<LIB_ID>& aSelection, std::vector<int>* aUnit ) const
{
    wxDataViewItemArray selection;
    int count = m_tree_ctrl->GetSelections( selection );

    for( const wxDataViewItem& item : selection )
    {
        aSelection.emplace_back( m_adapter->GetAliasFor( item ) );

        if( aUnit )
            aUnit->emplace_back( m_adapter->GetUnitFor( item ) );
    }

    return count;
}


LIB_TREE_NODE* LIB_TREE::GetCurrentTreeNode() const
{
    wxDataViewItem sel = m_tree_ctrl->GetSelection();

    if( !sel )
        return nullptr;

    return m_adapter->GetTreeNodeFor( sel );
}


void LIB_TREE::SelectLibId( const LIB_ID& aLibId )
{
    selectIfValid( m_adapter->FindItem( aLibId ) );
}


void LIB_TREE::CenterLibId( const LIB_ID& aLibId )
{
    centerIfValid( m_adapter->FindItem( aLibId ) );
}


void LIB_TREE::Unselect()
{
    m_tree_ctrl->UnselectAll();
}


void LIB_TREE::ExpandLibId( const LIB_ID& aLibId )
{
    expandIfValid( m_adapter->FindItem( aLibId ) );
}


void LIB_TREE::SetSearchString( const wxString& aSearchString )
{
    m_query_ctrl->ChangeValue( aSearchString );
}


wxString LIB_TREE::GetSearchString() const
{
    return m_query_ctrl->GetValue();
}


void LIB_TREE::updateRecentSearchMenu()
{
    wxString newEntry = GetSearchString();

    std::vector<wxString>& recents = g_recentSearches[ m_recentSearchesKey ];

    if( !newEntry.IsEmpty() )
    {
        if( alg::contains( recents, newEntry ) )
            alg::delete_matching( recents, newEntry );

        if( recents.size() >= RECENT_SEARCHES_MAX )
            recents.pop_back();

        recents.insert( recents.begin(), newEntry );
    }

    wxMenu* menu = new wxMenu();

    for( const wxString& recent : recents )
        menu->Append( menu->GetMenuItemCount() + 1, recent );

    if( recents.empty() )
        menu->Append( wxID_ANY, _( "recent searches" ) );

    m_query_ctrl->SetMenu( menu );
}


void LIB_TREE::Regenerate( bool aKeepState )
{
    STATE current;

    // Store the state
    if( aKeepState )
        current = getState();

    wxString filter = m_query_ctrl->GetValue();
    m_adapter->UpdateSearchString( filter, aKeepState );
    postPreselectEvent();

    // Restore the state
    if( aKeepState )
        setState( current );
}


void LIB_TREE::RefreshLibTree()
{
    m_adapter->RefreshTree();
}


wxWindow* LIB_TREE::GetFocusTarget()
{
    if( m_query_ctrl )
        return m_query_ctrl;
    else
        return m_tree_ctrl;
}


void LIB_TREE::FocusSearchFieldIfExists()
{
    if( m_query_ctrl )
        m_query_ctrl->SetFocus();
}


void LIB_TREE::toggleExpand( const wxDataViewItem& aTreeId )
{
    if( !aTreeId.IsOk() )
        return;

    if( m_tree_ctrl->IsExpanded( aTreeId ) )
        m_tree_ctrl->Collapse( aTreeId );
    else
        m_tree_ctrl->Expand( aTreeId );
}


void LIB_TREE::selectIfValid( const wxDataViewItem& aTreeId )
{
    if( aTreeId.IsOk() )
    {
        m_tree_ctrl->EnsureVisible( aTreeId );
        m_tree_ctrl->UnselectAll();
        m_tree_ctrl->Select( aTreeId );
        postPreselectEvent();
    }
}


void LIB_TREE::centerIfValid( const wxDataViewItem& aTreeId )
{
    /*
     * This doesn't actually center because the wxWidgets API is poorly suited to that (and
     * it might be too noisy as well).
     *
     * It does try to keep the given item a bit off the top or bottom of the window.
     */

    if( aTreeId.IsOk() )
    {
        LIB_TREE_NODE* node = m_adapter->GetTreeNodeFor( aTreeId );
        LIB_TREE_NODE* parent = node->m_Parent;
        LIB_TREE_NODE* grandParent = parent ? parent->m_Parent : nullptr;

        if( parent )
        {
            wxDataViewItemArray siblings;
            m_adapter->GetChildren( wxDataViewItem( parent ), siblings );

            int idx = siblings.Index( aTreeId );

            if( idx + 5 < (int) siblings.GetCount() )
            {
                m_tree_ctrl->EnsureVisible( siblings.Item( idx + 5 ) );
            }
            else if( grandParent )
            {
                wxDataViewItemArray parentsSiblings;
                m_adapter->GetChildren( wxDataViewItem( grandParent ), parentsSiblings );

                int p_idx = parentsSiblings.Index( wxDataViewItem( parent ) );

                if( p_idx + 1 < (int) parentsSiblings.GetCount() )
                    m_tree_ctrl->EnsureVisible( parentsSiblings.Item( p_idx + 1 ) );
            }

            if( idx - 5 >= 0 )
                m_tree_ctrl->EnsureVisible( siblings.Item( idx - 5 ) );
            else
                m_tree_ctrl->EnsureVisible( wxDataViewItem( parent ) );
        }

        m_tree_ctrl->EnsureVisible( aTreeId );
    }
}


void LIB_TREE::expandIfValid( const wxDataViewItem& aTreeId )
{
    if( aTreeId.IsOk() && !m_tree_ctrl->IsExpanded( aTreeId ) )
        m_tree_ctrl->Expand( aTreeId );
}


void LIB_TREE::postPreselectEvent()
{
    wxCommandEvent event( SYMBOL_PRESELECTED );
    wxPostEvent( this, event );
}


void LIB_TREE::postSelectEvent()
{
    wxCommandEvent event( SYMBOL_SELECTED );
    wxPostEvent( this, event );
}


LIB_TREE::STATE LIB_TREE::getState() const
{
    STATE state;
    wxDataViewItemArray items;
    m_adapter->GetChildren( wxDataViewItem( nullptr ), items );

    for( const wxDataViewItem& item : items )
    {
        if( m_tree_ctrl->IsExpanded( item ) )
            state.expanded.push_back( item );
    }

    state.selection = GetSelectedLibId();

    return state;
}


void LIB_TREE::setState( const STATE& aState )
{
    m_tree_ctrl->Freeze();

    for( const wxDataViewItem& item : aState.expanded )
        m_tree_ctrl->Expand( item );

    // wxDataViewCtrl cannot be frozen when a selection
    // command is issued, otherwise it selects a random item (Windows)
    m_tree_ctrl->Thaw();

    if( !aState.selection.GetLibItemName().empty() || !aState.selection.GetLibNickname().empty() )
        SelectLibId( aState.selection );
}


void LIB_TREE::onQueryText( wxCommandEvent& aEvent )
{
    m_debounceTimer->StartOnce( 200 );

    // Required to avoid interaction with SetHint()
    // See documentation for wxTextEntry::SetHint
    aEvent.Skip();
}


void LIB_TREE::onDebounceTimer( wxTimerEvent& aEvent )
{
    m_inTimerEvent = true;
    Regenerate( false );
    m_inTimerEvent = false;
}


void LIB_TREE::onQueryCharHook( wxKeyEvent& aKeyStroke )
{
    wxDataViewItem sel = m_tree_ctrl->GetSelection();

    if( !sel.IsOk() )
        sel = m_adapter->GetCurrentDataViewItem();

    LIB_TREE_NODE::TYPE  type = sel.IsOk() ? m_adapter->GetTypeFor( sel ) : LIB_TREE_NODE::INVALID;

    switch( aKeyStroke.GetKeyCode() )
    {
    case WXK_UP:
        updateRecentSearchMenu();
        selectIfValid( m_tree_ctrl->GetPrevItem( sel ) );
        break;

    case WXK_DOWN:
        updateRecentSearchMenu();
        selectIfValid( m_tree_ctrl->GetNextItem( sel ) );
        break;

    case WXK_ADD:
        updateRecentSearchMenu();

        if( type == LIB_TREE_NODE::LIB )
            m_tree_ctrl->Expand( sel );

        break;

    case WXK_SUBTRACT:
        updateRecentSearchMenu();

        if( type == LIB_TREE_NODE::LIB )
            m_tree_ctrl->Collapse( sel );

        break;

    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
        updateRecentSearchMenu();

        if( GetSelectedLibId().IsValid() )
            postSelectEvent();
        else if( type == LIB_TREE_NODE::LIB )
            toggleExpand( sel );

        break;

    default:
        aKeyStroke.Skip();      // Any other key: pass on to search box directly.
        break;
    }
}


void LIB_TREE::onQueryMouseMoved( wxMouseEvent& aEvent )
{
    wxPoint pos = aEvent.GetPosition();
    wxRect  ctrlRect = m_query_ctrl->GetScreenRect();
    int     buttonWidth = ctrlRect.GetHeight();         // Presume buttons are square

    if( m_query_ctrl->IsSearchButtonVisible() && pos.x < buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else if( m_query_ctrl->IsCancelButtonVisible() && pos.x > ctrlRect.GetWidth() - buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else
        SetCursor( wxCURSOR_IBEAM );
}


void LIB_TREE::onTreeCharHook( wxKeyEvent& aKeyStroke )
{
    onQueryCharHook( aKeyStroke );

    if( aKeyStroke.GetSkipped() )
    {
        if( TOOL_INTERACTIVE* tool = m_adapter->GetContextMenuTool() )
        {
            int hotkey = aKeyStroke.GetKeyCode();

            if( aKeyStroke.ShiftDown() )
                hotkey |= MD_SHIFT;

            if( aKeyStroke.AltDown() )
                hotkey |= MD_ALT;

            if( aKeyStroke.ControlDown() )
                hotkey |= MD_CTRL;

            if( tool->GetManager()->GetActionManager()->RunHotKey( hotkey ) )
                aKeyStroke.Skip( false );
        }
    }
}


void LIB_TREE::onTreeSelect( wxDataViewEvent& aEvent )
{
    if( !m_inTimerEvent )
        updateRecentSearchMenu();

    if( !m_tree_ctrl->IsFrozen() )
        postPreselectEvent();
}


void LIB_TREE::onTreeActivate( wxDataViewEvent& aEvent )
{
    if( !GetSelectedLibId().IsValid() )
        toggleExpand( m_tree_ctrl->GetSelection() );    // Expand library/part units subtree
    else
        postSelectEvent();                              // Open symbol/footprint
}


void LIB_TREE::onDetailsLink( wxHtmlLinkEvent& aEvent )
{
    const wxHtmlLinkInfo& info = aEvent.GetLinkInfo();
    ::wxLaunchDefaultBrowser( info.GetHref() );
}


void LIB_TREE::onPreselect( wxCommandEvent& aEvent )
{
    if( m_details_ctrl )
    {
        int unit = 0;
        LIB_ID id = GetSelectedLibId( &unit );

        if( id.IsValid() )
            m_details_ctrl->SetPage( m_adapter->GenerateInfo( id, unit ) );
        else
            m_details_ctrl->SetPage( wxEmptyString );
    }

    aEvent.Skip();
}


void LIB_TREE::onItemContextMenu( wxDataViewEvent& aEvent )
{
    if( m_skipNextRightClick )
    {
        m_skipNextRightClick = false;
        return;
    }

    if( TOOL_INTERACTIVE* tool = m_adapter->GetContextMenuTool() )
    {
        if( !GetCurrentTreeNode() )
        {
            wxPoint pos = m_tree_ctrl->ScreenToClient( wxGetMousePosition() );

            // What we actually want is the height of the column header, but wxWidgets gives us
            // no way to get that, so we use the height of the search ctrl as a proxy.  And it's
            // not even a very good proxy on Mac....
            int headerHeight = m_tree_ctrl->GetPosition().y;
#ifdef __WXMAC__
            headerHeight += 5;
#endif

            pos.y -= headerHeight;

            wxDataViewItem    item;
            wxDataViewColumn* col;
            m_tree_ctrl->HitTest( pos, item, col );

            if( item.IsOk() )
            {
                m_tree_ctrl->SetFocus();
                m_tree_ctrl->Select( item );
                wxSafeYield();
            }
        }

        tool->Activate();
        tool->GetManager()->VetoContextMenuMouseWarp();
        tool->GetToolMenu().ShowContextMenu();

        TOOL_EVENT evt( TC_MOUSE, TA_MOUSE_CLICK, BUT_RIGHT );
        tool->GetManager()->DispatchContextMenu( evt );
    }
    else
    {
        LIB_TREE_NODE* current = GetCurrentTreeNode();

        if( current && current->m_Type == LIB_TREE_NODE::LIB )
        {
            ACTION_MENU menu( true, nullptr );

            if( current->m_Pinned )
            {
                menu.Add( ACTIONS::unpinLibrary );

                if( GetPopupMenuSelectionFromUser( menu ) != wxID_NONE )
                    m_adapter->UnpinLibrary( current );
            }
            else
            {
                menu.Add( ACTIONS::pinLibrary );

                if( GetPopupMenuSelectionFromUser( menu ) != wxID_NONE )
                    m_adapter->PinLibrary( current );
            }
        }
    }
}


void LIB_TREE::onHeaderContextMenu( wxDataViewEvent& aEvent )
{
    ACTION_MENU menu( true, nullptr );

    menu.Add( ACTIONS::selectColumns );

    if( GetPopupMenuSelectionFromUser( menu ) != wxID_NONE )
    {
        EDA_REORDERABLE_LIST_DIALOG dlg( m_parent, _( "Select Columns" ),
                                         m_adapter->GetAvailableColumns(),
                                         m_adapter->GetShownColumns() );

        if( dlg.ShowModal() == wxID_OK )
            m_adapter->SetShownColumns( dlg.EnabledList() );
    }

#if !wxCHECK_VERSION( 3, 1, 0 )
    // wxGTK 3.0 sends item right click events for header right clicks
    m_skipNextRightClick = true;
#endif
}


wxDEFINE_EVENT( SYMBOL_PRESELECTED, wxCommandEvent );
wxDEFINE_EVENT( SYMBOL_SELECTED, wxCommandEvent );
