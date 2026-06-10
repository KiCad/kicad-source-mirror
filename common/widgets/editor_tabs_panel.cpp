/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <widgets/editor_tabs_panel.h>

#include <algorithm>

#include <wx/aui/auibook.h>
#include <wx/aui/framemanager.h>
#include <wx/event.h>
#include <wx/intl.h>
#include <wx/menu.h>
#include <wx/sizer.h>

#include <class_draw_panel_gal.h>


// Offset into a private range so these ids never collide with the host frame.
enum
{
    ID_TAB_CLOSE = wxID_HIGHEST + 1,
    ID_TAB_CLOSE_OTHERS,
    ID_TAB_CLOSE_TO_RIGHT,
    ID_TAB_CLOSE_ALL
};


int EDITOR_TABS_MODEL::FindIndex( const wxString& aKey ) const
{
    for( size_t i = 0; i < m_entries.size(); ++i )
    {
        if( m_entries[i].key == aKey )
            return static_cast<int>( i );
    }

    return -1;
}


int EDITOR_TABS_MODEL::PreviewIndex() const
{
    for( size_t i = 0; i < m_entries.size(); ++i )
    {
        const ENTRY& e = m_entries[i];

        if( e.preview && !e.modified )
            return static_cast<int>( i );
    }

    return -1;
}


int EDITOR_TABS_MODEL::OpenDocument( const wxString& aKey, bool aAsPreview )
{
    const int existing = FindIndex( aKey );

    if( existing >= 0 )
    {
        if( !aAsPreview )
            m_entries[existing].preview = false;

        return existing;
    }

    if( aAsPreview )
    {
        const int reuse = PreviewIndex();

        if( reuse >= 0 )
        {
            m_entries[reuse].key = aKey;
            m_entries[reuse].preview = true;
            m_entries[reuse].modified = false;
            return reuse;
        }
    }

    m_entries.push_back( ENTRY{ aKey, aAsPreview, false } );

    return static_cast<int>( m_entries.size() ) - 1;
}


void EDITOR_TABS_MODEL::CloseDocument( const wxString& aKey )
{
    const int idx = FindIndex( aKey );

    if( idx >= 0 )
        m_entries.erase( m_entries.begin() + idx );
}


void EDITOR_TABS_MODEL::MarkModified( const wxString& aKey, bool aModified )
{
    const int idx = FindIndex( aKey );

    if( idx < 0 )
        return;

    m_entries[idx].modified = aModified;

    // Promotion sticks even after the dirty flag later clears.
    if( aModified )
        m_entries[idx].preview = false;
}


void EDITOR_TABS_MODEL::Promote( const wxString& aKey )
{
    const int idx = FindIndex( aKey );

    if( idx >= 0 )
        m_entries[idx].preview = false;
}


bool EDITOR_TABS_MODEL::CanCloseWithoutPrompt( const wxString& aKey ) const
{
    const int idx = FindIndex( aKey );

    if( idx < 0 )
        return true;

    return !m_entries[idx].modified;
}


EDITOR_TABS_PANEL::EDITOR_TABS_PANEL( wxWindow* aParent, EDA_DRAW_PANEL_GAL* aSharedCanvas ) :
        wxPanel( aParent ),
        m_sharedCanvas( aSharedCanvas )
{
    // wxWidgets 3.3 made wxAuiTabCtrl an internal class that only works as a child of a notebook, so
    // the strip is a real wxAuiNotebook. Its (empty) page area is collapsed by updateTabStripHeight and
    // the host-owned canvas sits below it as a sibling; documents are tracked by hidden key windows.
    //
    // No wxAUI_NB_TAB_MOVE: the model, m_pageWindows and the host context vectors are index-aligned
    // with the notebook page order, and there is no reorder handler, so user drag-reordering would
    // desync them and target the wrong document.
    m_tabs = new wxAuiNotebook( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxAUI_NB_CLOSE_ON_ALL_TABS | wxAUI_NB_SCROLL_BUTTONS );

    m_tabs->SetArtProvider( new KICAD_TAB_ART( [this]( wxWindow* aPageWindow ) -> TAB_VISUAL_STATE
                                               {
                                                   return visualStateForIndex(
                                                           indexOfWindow( aPageWindow ) );
                                               } ) );

    // Floor the height before first layout so a zero-height paint never trips GTK's invalid bitmap
    // size assert. updateTabStripHeight refines it to the measured tab height once pages exist.
    m_tabs->SetMinSize( wxSize( -1, FromDIP( 28 ) ) );

    m_sizer = new wxBoxSizer( wxVERTICAL );
    m_sizer->Add( m_tabs, 0, wxEXPAND );

    if( m_sharedCanvas )
    {
        // Canvas is host-owned so remember its parent to hand it back and avoid ~wxPanel freeing it.
        // It is reparented in once and never again so a tab switch never disturbs the GL context.
        m_originalCanvasParent = m_sharedCanvas->GetParent();
        m_sharedCanvas->Reparent( this );
        m_sizer->Add( m_sharedCanvas, 1, wxEXPAND );
    }

    SetSizer( m_sizer );

    m_tabs->Bind( wxEVT_AUINOTEBOOK_PAGE_CHANGED, &EDITOR_TABS_PANEL::onPageChanged, this );
    m_tabs->Bind( wxEVT_AUINOTEBOOK_PAGE_CLOSE, &EDITOR_TABS_PANEL::onPageClose, this );
    m_tabs->Bind( wxEVT_AUINOTEBOOK_TAB_RIGHT_DOWN, &EDITOR_TABS_PANEL::onTabRightDown, this );

    // The notebook has no per-tab double-click event, so read the raw double-click on the tab strip
    // itself and hit-test it back to a page for preview promotion. updateTabStripHeight() keeps this
    // binding attached to the live tab-strip control as pages come and go.
    updateTabStripHeight();
}


void EDITOR_TABS_PANEL::ReleaseSharedCanvas()
{
    // Hand the host-owned canvas back so the frame frees it exactly once. Idempotent once null.
    if( !m_sharedCanvas )
        return;

    // Drop the sizer's reference first so no stale wxSizerItem lingers after the reparent.
    if( m_sizer )
        m_sizer->Detach( m_sharedCanvas );

    m_sharedCanvas->Reparent( m_originalCanvasParent ? m_originalCanvasParent : GetParent() );
    m_sharedCanvas = nullptr;
}


EDITOR_TABS_PANEL::~EDITOR_TABS_PANEL()
{
    // Hand the canvas back before child teardown so wx never frees it. Fallback for when no host did.
    ReleaseSharedCanvas();

    // The notebook owns the per-tab key windows and frees them with itself, so just drop our handles.
    m_pageWindows.clear();
}


int EDITOR_TABS_PANEL::indexOfWindow( wxWindow* aWindow ) const
{
    for( size_t i = 0; i < m_pageWindows.size(); ++i )
    {
        if( m_pageWindows[i] == aWindow )
            return static_cast<int>( i );
    }

    return -1;
}


void EDITOR_TABS_PANEL::bindTabDClick()
{
    // wxAuiNotebook owns the tab-strip control and can destroy and recreate it (wx 3.2 frees the tab
    // frame when the last page closes and rebuilds it on the next add), so bind to whatever control is
    // live now. Unbind first so re-binding the same control never stacks a second handler; a control
    // that was never bound makes the Unbind a harmless no-op, and a destroyed control already dropped
    // its binding with itself, so no stale pointer is ever dereferenced.
    wxAuiTabCtrl* tabCtrl = m_tabs->GetActiveTabCtrl();

    if( !tabCtrl )
        return;

    tabCtrl->Unbind( wxEVT_LEFT_DCLICK, &EDITOR_TABS_PANEL::onTabDClick, this );
    tabCtrl->Bind( wxEVT_LEFT_DCLICK, &EDITOR_TABS_PANEL::onTabDClick, this );
}


void EDITOR_TABS_PANEL::updateTabStripHeight()
{
    // The notebook can rebuild its tab-strip control as pages come and go, so keep the double-click
    // handler attached to the live control.
    bindTabDClick();

    // Clamp the notebook to its tab-strip height so its unused page area collapses below the visible
    // region and the shared canvas owns the rest of the pane.
    const bool hasPages = !m_pageWindows.empty();

    int height = hasPages ? m_tabs->GetTabCtrlHeight() : 0;

    // GetTabCtrlHeight reports 0 before the strip has a valid measuring font, so fall back to a floor.
    if( hasPages && height <= 0 )
        height = FromDIP( 28 );

    // Collapse the row when there are no tabs, but keep the control's own min height above zero so a
    // stray paint never sees a zero-height client area and trips the GTK assert.
    if( m_sizer )
        m_sizer->SetItemMinSize( m_tabs, -1, height );

    m_tabs->SetMinSize( wxSize( -1, hasPages ? height : FromDIP( 28 ) ) );
    m_tabs->Show( hasPages );

    if( m_sharedCanvas )
        m_sharedCanvas->Show( hasPages );

    Layout();
}


TAB_VISUAL_STATE EDITOR_TABS_PANEL::visualStateForIndex( int aIdx ) const
{
    if( aIdx < 0 || aIdx >= static_cast<int>( m_model.Entries().size() ) )
        return TAB_VISUAL_STATE{};

    if( onQueryVisualState )
        return onQueryVisualState( aIdx );

    const EDITOR_TABS_MODEL::ENTRY& e = m_model.Entries()[aIdx];

    return ResolveTabVisualState( e.preview, e.modified );
}


int EDITOR_TABS_PANEL::AddTab( const wxString& aKey, const wxString& aLabel, bool aAsPreview )
{
    const int existing = m_model.FindIndex( aKey );

    if( existing >= 0 )
    {
        m_model.OpenDocument( aKey, aAsPreview );
        SelectTab( existing );
        return existing;
    }

    // Capture the preview slot's key before OpenDocument overwrites it, else a reused slot leaves the
    // old key as a dead step in the MRU order.
    wxString reusedOldKey;

    if( aAsPreview )
    {
        const int previewSlot = m_model.PreviewIndex();

        if( previewSlot >= 0 )
            reusedOldKey = m_model.Entries()[previewSlot].key;
    }

    const int reuse = m_model.OpenDocument( aKey, aAsPreview );

    if( reuse < static_cast<int>( m_pageWindows.size() ) )
    {
        m_tabs->SetPageText( reuse, aLabel );

        if( !reusedOldKey.empty() && reusedOldKey != aKey )
            forgetMru( reusedOldKey );
    }
    else
    {
        wxWindow* key = new wxWindow( m_tabs, wxID_ANY );
        key->Hide();

        // The notebook force-selects its first page, so swallow that change event and drive activation
        // explicitly through SelectTab below.
        m_activating = true;
        m_tabs->AddPage( key, aLabel, false );
        m_activating = false;

        m_pageWindows.push_back( key );
    }

    updateTabStripHeight();

    touchMru( aKey );
    SelectTab( reuse );
    m_tabs->Refresh();

    return reuse;
}


void EDITOR_TABS_PANEL::CloseTab( int aIdx )
{
    closeTabInternal( aIdx );
}


void EDITOR_TABS_PANEL::closeTabInternal( int aIdx )
{
    if( aIdx < 0 || aIdx >= static_cast<int>( m_pageWindows.size() ) )
        return;

    // Remember the active tab as a key, not an index, since indices shift on removal.
    const int      activeBefore = GetActiveTab();
    const wxString activeBeforeKey =
            ( activeBefore >= 0 && activeBefore < static_cast<int>( m_model.Entries().size() ) )
                    ? m_model.Entries()[activeBefore].key
                    : wxString();

    // Every close entry point funnels through here so the user is prompted exactly once.
    if( onCloseTabRequested && !onCloseTabRequested( aIdx ) )
        return;

    const wxString key = m_model.Entries()[aIdx].key;

    // DeletePage frees the key window and reselects a neighbour; swallow that change event since the
    // successor selection is chosen explicitly below.
    m_activating = true;
    m_tabs->DeletePage( static_cast<size_t>( aIdx ) );
    m_activating = false;

    m_pageWindows.erase( m_pageWindows.begin() + aIdx );
    m_model.CloseDocument( key );
    forgetMru( key );

    updateTabStripHeight();

    const int newCount = static_cast<int>( m_pageWindows.size() );

    if( newCount <= 0 )
    {
        m_tabs->Refresh();
        return;
    }

    if( m_suppressActivateOnClose )
    {
        // The host already installed the successor document, so select the page visually only and
        // keep the MRU consistent. Re-entering activation would double-install or hit a freed index.
        int visualIdx;

        if( activeBeforeKey.empty() || activeBeforeKey == key )
            visualIdx = std::min( aIdx, newCount - 1 );
        else
            visualIdx = m_model.FindIndex( activeBeforeKey );

        if( visualIdx >= 0 && visualIdx < newCount )
        {
            // ChangeSelection updates the strip without firing a change event, so the host is not
            // re-notified to install a document it already installed.
            m_tabs->ChangeSelection( static_cast<size_t>( visualIdx ) );
            touchMru( m_model.Entries()[visualIdx].key );
        }
    }
    else
    {
        SelectTab( std::min( aIdx, newCount - 1 ) );
    }

    m_tabs->Refresh();
}


void EDITOR_TABS_PANEL::CloseOthers( int aKeepIdx )
{
    if( aKeepIdx < 0 || aKeepIdx >= static_cast<int>( m_pageWindows.size() ) )
        return;

    const wxString keepKey = m_model.Entries()[aKeepIdx].key;

    // Close from the back so earlier indices stay valid.
    for( int i = static_cast<int>( m_pageWindows.size() ) - 1; i >= 0; --i )
    {
        if( m_model.Entries()[i].key != keepKey )
            CloseTab( i );
    }
}


void EDITOR_TABS_PANEL::CloseToRight( int aIdx )
{
    // A negative anchor would otherwise close every tab.
    if( aIdx < 0 || aIdx >= static_cast<int>( m_pageWindows.size() ) )
        return;

    for( int i = static_cast<int>( m_pageWindows.size() ) - 1; i > aIdx; --i )
        CloseTab( i );
}


void EDITOR_TABS_PANEL::CloseAll()
{
    for( int i = static_cast<int>( m_pageWindows.size() ) - 1; i >= 0; --i )
        CloseTab( i );
}


void EDITOR_TABS_PANEL::MarkModified( int aIdx, bool aModified )
{
    if( aIdx < 0 || aIdx >= static_cast<int>( m_model.Entries().size() ) )
        return;

    // Clearing the preview flag stops the next library-open from replacing this edited document.
    m_model.MarkModified( m_model.Entries()[aIdx].key, aModified );
    RefreshTabLabels();
}


void EDITOR_TABS_PANEL::PromoteTab( int aIdx )
{
    if( aIdx < 0 || aIdx >= static_cast<int>( m_model.Entries().size() ) )
        return;

    if( !m_model.Entries()[aIdx].preview )
        return;

    m_model.Promote( m_model.Entries()[aIdx].key );
    RefreshTabLabels();
}


void EDITOR_TABS_PANEL::SelectTab( int aIdx )
{
    if( aIdx < 0 || aIdx >= static_cast<int>( m_pageWindows.size() ) )
        return;

    // ChangeSelection moves the strip without firing a change event, so activation is driven once,
    // here, rather than also arriving through onPageChanged.
    m_tabs->ChangeSelection( static_cast<size_t>( aIdx ) );
    m_tabs->Refresh();
    activateTab( aIdx );
}


void EDITOR_TABS_PANEL::activateTab( int aIdx )
{
    if( aIdx < 0 || aIdx >= static_cast<int>( m_model.Entries().size() ) )
        return;

    // Guard against re-entrancy from a programmatic SetActivePage so the host is notified once.
    if( m_activating )
        return;

    m_activating = true;
    touchMru( m_model.Entries()[aIdx].key );

    if( onActivateTab )
        onActivateTab( aIdx );

    m_activating = false;
}


void EDITOR_TABS_PANEL::AdvanceTab( bool aForward )
{
    if( m_pageWindows.size() < 2 )
        return;

    // Cycle the MRU order so repeated Ctrl+Tab walks recently visited tabs first.
    const int active = GetActiveTab();

    if( active < 0 )
        return;

    const wxString activeKey = m_model.Entries()[active].key;
    int            pos = -1;

    for( size_t i = 0; i < m_mru.size(); ++i )
    {
        if( m_mru[i] == activeKey )
        {
            pos = static_cast<int>( i );
            break;
        }
    }

    if( pos < 0 )
        return;

    const int count = static_cast<int>( m_mru.size() );
    const int next = aForward ? ( pos + 1 ) % count : ( pos - 1 + count ) % count;
    const int idx = m_model.FindIndex( m_mru[next] );

    if( idx >= 0 )
        SelectTab( idx );
}


int EDITOR_TABS_PANEL::GetActiveTab() const
{
    return m_tabs->GetSelection();
}


int EDITOR_TABS_PANEL::FindTab( const wxString& aKey ) const
{
    return m_model.FindIndex( aKey );
}


void EDITOR_TABS_PANEL::RefreshTabLabels()
{
    m_tabs->Refresh();
}


void EDITOR_TABS_PANEL::touchMru( const wxString& aKey )
{
    forgetMru( aKey );
    m_mru.insert( m_mru.begin(), aKey );
}


void EDITOR_TABS_PANEL::forgetMru( const wxString& aKey )
{
    m_mru.erase( std::remove( m_mru.begin(), m_mru.end(), aKey ), m_mru.end() );
}


void EDITOR_TABS_PANEL::onPageChanged( wxAuiNotebookEvent& aEvent )
{
    // The notebook drives its own active page, so a user tab switch surfaces here as the activation
    // path. Programmatic selection uses ChangeSelection and is fired explicitly, not through here.
    const int idx = aEvent.GetSelection();

    if( idx >= 0 && idx < static_cast<int>( m_model.Entries().size() ) )
        activateTab( idx );

    aEvent.Skip();
}


void EDITOR_TABS_PANEL::onPageClose( wxAuiNotebookEvent& aEvent )
{
    const int idx = aEvent.GetSelection();

    // Veto the control's own removal and route through CloseTab, which owns the single close prompt.
    aEvent.Veto();
    CloseTab( idx );
}


void EDITOR_TABS_PANEL::onTabRightDown( wxAuiNotebookEvent& aEvent )
{
    m_contextMenuIdx = aEvent.GetSelection();

    if( m_contextMenuIdx < 0 )
        return;

    wxMenu menu;
    menu.Append( ID_TAB_CLOSE, _( "Close Tab" ) );
    menu.Append( ID_TAB_CLOSE_OTHERS, _( "Close Other Tabs" ) );
    menu.Append( ID_TAB_CLOSE_TO_RIGHT, _( "Close Tabs to the Right" ) );
    menu.Append( ID_TAB_CLOSE_ALL, _( "Close All Tabs" ) );

    menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &EDITOR_TABS_PANEL::onContextMenu, this );

    PopupMenu( &menu );
}


void EDITOR_TABS_PANEL::onTabDClick( wxMouseEvent& aEvent )
{
    aEvent.Skip();

    // The handler is bound on the tab strip control, so the click is in its coordinates and hit-tests
    // against the same control.
    wxAuiTabCtrl* tabCtrl = m_tabs->GetActiveTabCtrl();

    if( !tabCtrl )
        return;

    wxWindow* page = nullptr;

    if( tabCtrl->TabHitTest( aEvent.GetX(), aEvent.GetY(), &page ) )
        PromoteTab( indexOfWindow( page ) );
}


void EDITOR_TABS_PANEL::onContextMenu( wxCommandEvent& aEvent )
{
    const int idx = m_contextMenuIdx;

    if( idx < 0 )
        return;

    switch( aEvent.GetId() )
    {
    case ID_TAB_CLOSE:          CloseTab( idx );                                break;
    case ID_TAB_CLOSE_OTHERS:   CloseOthers( idx );                             break;
    case ID_TAB_CLOSE_TO_RIGHT: CloseToRight( idx );                            break;
    case ID_TAB_CLOSE_ALL:      CloseAll();                                     break;
    default:                                                                    break;
    }
}
