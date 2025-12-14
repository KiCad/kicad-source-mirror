/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <widgets/bitmap_button.h>
#include <core/kicad_algo.h>
#include <algorithm>
#include <macros.h>
#include <bitmaps.h>
#include <dialogs/eda_reorderable_list_dialog.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <tool/action_manager.h>
#include <tool/actions.h>
#include <tool/tool_dispatcher.h>
#include <widgets/wx_dataviewctrl.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/srchctrl.h>
#include <wx/statline.h>
#include <wx/popupwin.h>

#include <eda_doc.h>                    // for GetAssociatedDocument()
#include <pgm_base.h>                   // for PROJECT
#include <settings/settings_manager.h>  // for PROJECT

constexpr int RECENT_SEARCHES_MAX = 10;

std::map<wxString, std::vector<wxString>> g_recentSearches;


LIB_TREE::LIB_TREE( wxWindow* aParent, const wxString& aRecentSearchesKey,
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
        m_filtersSizer( nullptr ),
        m_skipNextRightClick( false ),
        m_previewWindow( nullptr ),
        m_previewDisabled( false )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_hoverTimer.SetOwner( this );
    Bind( wxEVT_TIMER, &LIB_TREE::onHoverTimer, this, m_hoverTimer.GetId() );

    // Search text control
    if( aFlags & SEARCH )
    {
        wxBoxSizer* search_sizer = new wxBoxSizer( wxHORIZONTAL );

        m_query_ctrl = new wxSearchCtrl( this, wxID_ANY );

        m_query_ctrl->ShowCancelButton( true );

        m_debounceTimer = new wxTimer( this );

        search_sizer->Add( m_query_ctrl, 1, wxALIGN_CENTER_VERTICAL|wxRIGHT, 4 );

        wxStaticLine* separator = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL );
        search_sizer->Add( separator, 0, wxEXPAND|wxTOP|wxBOTTOM, 3 );

        m_sort_ctrl = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition,
                                         wxDefaultSize, wxBU_AUTODRAW|0 );
        m_sort_ctrl->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
        m_sort_ctrl->Bind( wxEVT_LEFT_DOWN,
                [&]( wxMouseEvent& aEvent )
                {
                    // Build a pop menu:
                    wxMenu menu;

                    menu.Append( 4201, _( "Sort by Best Match" ), wxEmptyString, wxITEM_CHECK );
                    menu.Append( 4202, _( "Sort Alphabetically" ), wxEmptyString, wxITEM_CHECK );
                    menu.AppendSeparator();
                    menu.Append( 4203, ACTIONS::expandAll.GetMenuItem() );
                    menu.Append( 4204, ACTIONS::collapseAll.GetMenuItem() );

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
                    else if( menu_id == 3 || menu_id == 4203 )
                    {
                        ExpandAll();
                    }
                    else if( menu_id == 4 || menu_id == 4204 )
                    {
                        CollapseAll();
                    }
                } );

        m_sort_ctrl->Bind( wxEVT_CHAR_HOOK, &LIB_TREE::onTreeCharHook, this );
        search_sizer->Add( m_sort_ctrl, 0, wxALIGN_CENTER_VERTICAL, 5 );

        sizer->Add( search_sizer, 0, wxEXPAND, 5 );

        m_query_ctrl->Bind( wxEVT_TEXT, &LIB_TREE::onQueryText, this );

        m_query_ctrl->Bind( wxEVT_SEARCH_CANCEL, &LIB_TREE::onQueryText, this );
        m_query_ctrl->Bind( wxEVT_CHAR_HOOK, &LIB_TREE::onQueryCharHook, this );
        m_query_ctrl->Bind( wxEVT_MOTION, &LIB_TREE::onQueryMouseMoved, this );

#if defined( __WXOSX__ ) // Doesn't work properly on other ports
        m_query_ctrl->Bind( wxEVT_LEAVE_WINDOW,
                            [this]( wxMouseEvent& aEvt )
                            {
                                SetCursor( wxCURSOR_ARROW );
                            } );
#endif

        m_query_ctrl->Bind( wxEVT_MENU,
                [this]( wxCommandEvent& aEvent )
                {
                    size_t idx = aEvent.GetId() - 1;

                    if( idx < g_recentSearches[ m_recentSearchesKey ].size() )
                        m_query_ctrl->SetValue( g_recentSearches[ m_recentSearchesKey ][idx] );
                },
                1, RECENT_SEARCHES_MAX );

        Bind( wxEVT_TIMER, &LIB_TREE::onDebounceTimer, this, m_debounceTimer->GetId() );
    }

    if( aFlags & FILTERS )
    {
        m_filtersSizer = new wxBoxSizer( wxVERTICAL );
        sizer->Add( m_filtersSizer, 0, wxEXPAND | wxLEFT, 4 );
    }

    // Tree control
    int dvFlags = ( aFlags & MULTISELECT ) ? wxDV_MULTIPLE : wxDV_SINGLE;
    m_tree_ctrl = new WX_DATAVIEWCTRL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, dvFlags );
    m_adapter->AttachTo( m_tree_ctrl );

#ifdef __WXGTK__
    // The GTK renderer seems to calculate row height incorrectly sometimes; but can be overridden
    int rowHeight = FromDIP( 6 ) + GetTextExtent( wxS( "pdI" ) ).y;
    m_tree_ctrl->SetRowHeight( rowHeight );
#endif

    sizer->Add( m_tree_ctrl, 5, wxEXPAND, 5 );

    // Description panel
    if( aFlags & DETAILS )
    {
        if( !aDetails )
        {
            wxPoint html_size = ConvertDialogToPixels( wxPoint( 80, 80 ) );

            m_details_ctrl = new HTML_WINDOW( this, wxID_ANY, wxDefaultPosition,
                                              wxSize( html_size.x, html_size.y ) );

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

    // wxDataViewCtrl eats its mouseMoved events, so we're forced to use idle events to track
    // the hover state
    Bind( wxEVT_IDLE, &LIB_TREE::onIdle, this );

    // Process hotkeys when the tree control has focus:
    m_tree_ctrl->Bind( wxEVT_CHAR_HOOK, &LIB_TREE::onTreeCharHook, this );

    Bind( EVT_LIBITEM_SELECTED, &LIB_TREE::onPreselect, this );

    if( m_query_ctrl )
    {
        m_query_ctrl->SetDescriptiveText( _( "Filter" ) );
        m_query_ctrl->SetFocus();
        m_query_ctrl->ChangeValue( wxEmptyString );
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
    Unbind( wxEVT_TIMER, &LIB_TREE::onHoverTimer, this, m_hoverTimer.GetId() );

    m_tree_ctrl->Unbind( wxEVT_DATAVIEW_ITEM_ACTIVATED, &LIB_TREE::onTreeActivate, this );
    m_tree_ctrl->Unbind( wxEVT_DATAVIEW_SELECTION_CHANGED, &LIB_TREE::onTreeSelect, this );
    m_tree_ctrl->Unbind( wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &LIB_TREE::onItemContextMenu, this );
    m_tree_ctrl->Unbind( wxEVT_DATAVIEW_COLUMN_HEADER_RIGHT_CLICK, &LIB_TREE::onHeaderContextMenu,
                         this );

    Unbind( wxEVT_IDLE, &LIB_TREE::onIdle, this );
    m_tree_ctrl->Unbind( wxEVT_CHAR_HOOK, &LIB_TREE::onTreeCharHook, this );
    Unbind( EVT_LIBITEM_SELECTED, &LIB_TREE::onPreselect, this );

    if( m_query_ctrl )
    {
        m_query_ctrl->Unbind( wxEVT_TEXT, &LIB_TREE::onQueryText, this );

        m_query_ctrl->Unbind( wxEVT_SEARCH_CANCEL, &LIB_TREE::onQueryText, this );
        m_query_ctrl->Unbind( wxEVT_CHAR_HOOK, &LIB_TREE::onQueryCharHook, this );
        m_query_ctrl->Unbind( wxEVT_MOTION, &LIB_TREE::onQueryMouseMoved, this );
    }

    // Stop the timer during destruction early to avoid potential race conditions (that do happen)]
    if( m_debounceTimer )
    {
        m_debounceTimer->Stop();
        Unbind( wxEVT_TIMER, &LIB_TREE::onDebounceTimer, this, m_debounceTimer->GetId() );
    }

    if( m_details_ctrl )
        m_details_ctrl->Unbind( wxEVT_HTML_LINK_CLICKED, &LIB_TREE::onDetailsLink, this );

    m_hoverTimer.Stop();
}


void LIB_TREE::ShutdownPreviews()
{
    m_hoverTimer.Stop();
    m_previewDisabled = true;

    if( m_previewWindow )
    {
        // Shutdown the preview window's canvas
        m_adapter->ShutdownPreview( m_previewWindow );

        m_previewWindow->Hide();
        m_previewWindow->Destroy();
        m_previewWindow = nullptr;
    }
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

    if( m_adapter->GetTreeNodeFor( sel )->m_IsAlreadyPlacedGroup
            || m_adapter->GetTreeNodeFor( sel )->m_IsRecentlyUsedGroup )
    {
        return LIB_ID();
    }

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

int LIB_TREE::GetSelectedTreeNodes( std::vector<LIB_TREE_NODE*>& aSelection ) const
{
    wxDataViewItemArray selection;
    int                 count = m_tree_ctrl->GetSelections( selection );

    for( const wxDataViewItem& item : selection )
    {
        aSelection.push_back( m_adapter->GetTreeNodeFor( item ) );
    }

    return count;
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
    m_tree_ctrl->Freeze();
    m_tree_ctrl->UnselectAll();
    m_tree_ctrl->Thaw();
}


void LIB_TREE::ExpandLibId( const LIB_ID& aLibId )
{
    expandIfValid( m_adapter->FindItem( aLibId ) );
}


void LIB_TREE::ExpandAll()
{
    m_tree_ctrl->ExpandAll();
}


void LIB_TREE::CollapseAll()
{
    m_tree_ctrl->CollapseAll();
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
            std::erase( recents, newEntry );

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
    wxCommandEvent event( EVT_LIBITEM_SELECTED );
    wxPostEvent( this, event );
}


void LIB_TREE::postSelectEvent()
{
    wxCommandEvent event( EVT_LIBITEM_CHOSEN );
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

    state.scrollpos = {
        m_tree_ctrl->HasScrollbar( wxHORIZONTAL ) ? m_tree_ctrl->GetScrollPos( wxHORIZONTAL ) : 0,
        m_tree_ctrl->HasScrollbar( wxVERTICAL ) ? m_tree_ctrl->GetScrollPos( wxVERTICAL ) : 0
    };

    return state;
}


void LIB_TREE::setState( const STATE& aState )
{
    m_tree_ctrl->Freeze();

    for( const wxDataViewItem& item : aState.expanded )
        m_tree_ctrl->Expand( item );

    // TODO(JE) probably remove this; it fights with centerIfValid
    // m_tree_ctrl->SetScrollPos( wxHORIZONTAL, aState.scrollpos.x );
    // m_tree_ctrl->SetScrollPos( wxVERTICAL, aState.scrollpos.y );

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
    int hotkey = aKeyStroke.GetKeyCode();

    int mods = aKeyStroke.GetModifiers();

    // the flag wxMOD_ALTGR is defined in wxWidgets as wxMOD_CONTROL|wxMOD_ALT
    // So AltGr key cannot used as modifier key because it is the same as Alt key + Ctrl key.
#if CAN_USE_ALTGR_KEY
    if( wxmods & wxMOD_ALTGR )
        mods |= MD_ALTGR;
    else
#endif
    {
        if( mods & wxMOD_CONTROL )
            hotkey += MD_CTRL;

        if( mods & wxMOD_ALT )
            hotkey += MD_ALT;
    }

    if( mods & wxMOD_SHIFT )
        hotkey += MD_SHIFT;

#ifdef wxMOD_META
    if( mods & wxMOD_META )
        hotkey += MD_META;
#endif

#ifdef wxMOD_WIN
    if( mods & wxMOD_WIN )
        hotkey += MD_SUPER;
#endif

    if( hotkey == ACTIONS::expandAll.GetHotKey()
        || hotkey == ACTIONS::expandAll.GetHotKeyAlt() )
    {
        m_tree_ctrl->ExpandAll();
        return;
    }
    else if( hotkey == ACTIONS::collapseAll.GetHotKey()
             || hotkey == ACTIONS::collapseAll.GetHotKeyAlt() )
    {
        m_tree_ctrl->CollapseAll();
        return;
    }

    wxDataViewItem sel = m_tree_ctrl->GetSelection();

    if( !sel.IsOk() )
        sel = m_adapter->GetCurrentDataViewItem();

    LIB_TREE_NODE::TYPE type = sel.IsOk() ? m_adapter->GetTypeFor( sel )
                                          : LIB_TREE_NODE::TYPE::INVALID;

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

        if( type == LIB_TREE_NODE::TYPE::LIBRARY )
            m_tree_ctrl->Expand( sel );

        break;

    case WXK_SUBTRACT:
        updateRecentSearchMenu();

        if( type == LIB_TREE_NODE::TYPE::LIBRARY )
            m_tree_ctrl->Collapse( sel );

        break;

    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
        updateRecentSearchMenu();

        if( GetSelectedLibId().IsValid() )
            postSelectEvent();
        else if( type == LIB_TREE_NODE::TYPE::LIBRARY )
            toggleExpand( sel );

        break;

    default:
        aKeyStroke.Skip();      // Any other key: pass on to search box directly.
        break;
    }
}


void LIB_TREE::onQueryMouseMoved( wxMouseEvent& aEvent )
{
#if defined( __WXOSX__ ) // Doesn't work properly on other ports
    wxPoint pos = aEvent.GetPosition();
    wxRect  ctrlRect = m_query_ctrl->GetScreenRect();
    int     buttonWidth = ctrlRect.GetHeight();         // Presume buttons are square

    if( m_query_ctrl->IsSearchButtonVisible() && pos.x < buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else if( m_query_ctrl->IsCancelButtonVisible() && pos.x > ctrlRect.GetWidth() - buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else
        SetCursor( wxCURSOR_IBEAM );
#endif
}


#define PREVIEW_SIZE wxSize( 240, 200 )
#define HOVER_TIMER_MILLIS 400


void LIB_TREE::showPreview( wxDataViewItem aItem )
{
    if( aItem.IsOk() && m_adapter->HasPreview( aItem ) )
    {
        m_previewItem = aItem;
        m_previewItemRect = m_tree_ctrl->GetItemRect( m_previewItem );

        wxWindow* topLevelParent = wxGetTopLevelParent( m_parent );

        if( !m_previewWindow )
            m_previewWindow = new wxPopupWindow( topLevelParent );

        m_previewWindow->SetSize( PREVIEW_SIZE );

        m_adapter->ShowPreview( m_previewWindow, aItem );

        m_previewWindow->SetPosition( wxPoint( m_tree_ctrl->GetScreenRect().GetRight() - 10,
                                               wxGetMousePosition().y - PREVIEW_SIZE.y / 2 ) );

        m_previewWindow->Show();
    }
}


void LIB_TREE::hidePreview()
{
    m_previewItem = wxDataViewItem();

    if( m_previewWindow )
        m_previewWindow->Hide();
}


void LIB_TREE::onIdle( wxIdleEvent& aEvent )
{
    // The wxDataViewCtrl won't give us its mouseMoved events so we're forced to use idle
    // events to track the hover state

    // The dang thing won't give us scroll events either, so we implement a poor-man's
    // scroll-checker using the last-known positions of the preview or hover items.

    wxWindow* topLevelParent = wxGetTopLevelParent( m_parent );
    wxWindow* topLevelFocus = wxGetTopLevelParent( wxWindow::FindFocus() );

    bool    mouseOverWindow = false;
    wxPoint screenPos = wxGetMousePosition();

    if( m_tree_ctrl->IsShownOnScreen() )
        mouseOverWindow |= m_tree_ctrl->GetScreenRect().Contains( screenPos );

    if( m_previewDisabled || topLevelFocus != topLevelParent || !mouseOverWindow )
    {
        m_hoverTimer.Stop();
        hidePreview();
        return;
    }

    wxPoint           clientPos = m_tree_ctrl->ScreenToClient( screenPos );
    wxDataViewItem    item;
    wxDataViewColumn* col = nullptr;

    m_tree_ctrl->HitTest( clientPos, item, col );

    if( m_previewItem.IsOk() )
    {
        if( item != m_previewItem )
        {
#ifdef __WXGTK__
            // Hide the preview, because Wayland can't move windows.
            if( wxGetDisplayInfo().type == wxDisplayType::wxDisplayWayland )
                hidePreview();
#endif
            showPreview( item );
        }

        return;
    }

    if( m_hoverPos != clientPos )
    {
        m_hoverPos = clientPos;
        m_hoverItem = item;
        m_hoverItemRect = m_tree_ctrl->GetItemRect( m_hoverItem );
        m_hoverTimer.StartOnce( HOVER_TIMER_MILLIS );
    }
}


void LIB_TREE::onHoverTimer( wxTimerEvent& aEvent )
{
    hidePreview();

    if( !m_tree_ctrl->IsShownOnScreen() || m_previewDisabled )
        return;

    wxDataViewItem    item;
    wxDataViewColumn* col = nullptr;
    m_tree_ctrl->HitTest( m_hoverPos, item, col );

    if( item == m_hoverItem && m_tree_ctrl->GetItemRect( item ) == m_hoverItemRect )
    {
        if( item != m_tree_ctrl->GetSelection() )
            showPreview( item );
    }
    else // view must have been scrolled
    {
        m_hoverItem = item;
        m_hoverItemRect = m_tree_ctrl->GetItemRect( m_hoverItem );
        m_hoverTimer.StartOnce( HOVER_TIMER_MILLIS );
    }
}


void LIB_TREE::onTreeCharHook( wxKeyEvent& aKeyStroke )
{
    onQueryCharHook( aKeyStroke );

    if( aKeyStroke.GetSkipped() )
    {
        if( TOOL_INTERACTIVE* tool = m_adapter->GetContextMenuTool() )
        {
            int hotkey = aKeyStroke.GetKeyCode();

            int mods = aKeyStroke.GetModifiers();

            if( mods & wxMOD_ALTGR )
                hotkey |= MD_ALTGR;
            else
            {
                if( mods & wxMOD_ALT )
                    hotkey |= MD_ALT;

                if( mods & wxMOD_CONTROL )
                    hotkey |= MD_CTRL;
            }

            if( mods & wxMOD_SHIFT )
                hotkey |= MD_SHIFT;

#ifdef wxMOD_META
            if( mods & wxMOD_META )
                hotkey |= MD_META;
#endif

#ifdef wxMOD_WIN
            if( mods & wxMOD_WIN )
                hotkey |= MD_SUPER;
#endif

            if( tool->GetManager()->GetActionManager()->RunHotKey( hotkey ) )
                aKeyStroke.Skip( false );
        }
    }
}


void LIB_TREE::onTreeSelect( wxDataViewEvent& aEvent )
{
    if( m_tree_ctrl->IsFrozen() )
        return;

    if( !m_inTimerEvent )
        updateRecentSearchMenu();

    postPreselectEvent();
}


void LIB_TREE::onTreeActivate( wxDataViewEvent& aEvent )
{
    hidePreview();

    if( !m_inTimerEvent )
        updateRecentSearchMenu();

    if( !GetSelectedLibId().IsValid() )
        toggleExpand( m_tree_ctrl->GetSelection() );    // Expand library/part units subtree
    else
        postSelectEvent();                              // Open symbol/footprint
}


void LIB_TREE::onDetailsLink( wxHtmlLinkEvent& aEvent )
{
    const wxHtmlLinkInfo& info = aEvent.GetLinkInfo();
    wxString docname = info.GetHref();
    PROJECT& prj = Pgm().GetSettingsManager().Prj();

    GetAssociatedDocument( this, docname, &prj );
}


void LIB_TREE::onPreselect( wxCommandEvent& aEvent )
{
    hidePreview();

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
    hidePreview();

    if( m_skipNextRightClick )
    {
        m_skipNextRightClick = false;
        return;
    }

    m_previewDisabled = true;

    if( TOOL_INTERACTIVE* tool = m_adapter->GetContextMenuTool() )
    {
        if( !GetCurrentTreeNode() )
        {
            wxPoint pos = m_tree_ctrl->ScreenToClient( wxGetMousePosition() );

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

        if( current && current->m_Type == LIB_TREE_NODE::TYPE::LIBRARY )
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

    m_previewDisabled = false;
}


void LIB_TREE::onHeaderContextMenu( wxDataViewEvent& aEvent )
{
    hidePreview();
    m_previewDisabled = true;

    ACTION_MENU menu( true, nullptr );

    menu.Add( ACTIONS::selectLibTreeColumns );

    if( GetPopupMenuSelectionFromUser( menu ) != wxID_NONE )
    {
        EDA_REORDERABLE_LIST_DIALOG dlg( m_parent, _( "Select Columns" ),
                                         m_adapter->GetAvailableColumns(),
                                         m_adapter->GetShownColumns() );

        if( dlg.ShowModal() == wxID_OK )
        {
            m_adapter->SetShownColumns( dlg.EnabledList() );
            Regenerate( true );
        }
    }

    m_previewDisabled = false;
}


wxDEFINE_EVENT( EVT_LIBITEM_SELECTED, wxCommandEvent );
wxDEFINE_EVENT( EVT_LIBITEM_CHOSEN, wxCommandEvent );
