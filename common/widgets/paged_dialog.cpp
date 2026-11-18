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

#include <confirm.h>
#include <core/profile.h>
#include <scoped_set_reset.h>
#include <trace_helpers.h>
#include <widgets/focus_snapshot.h>
#include <widgets/resettable_panel.h>
#include <widgets/wx_infobar.h>
#include <widgets/wx_panel.h>
#include <widgets/paged_dialog.h>
#include <widgets/wx_treebook.h>
#include <pgm_base.h>
#include <project.h>
#include <settings/common_settings.h>
#include <settings/common_settings_internals.h>
#include <set>
#include <widgets/ui_common.h>

#include <wx/app.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/ctrlsub.h>
#include <wx/display.h>
#include <wx/grid.h>
#include <wx/hyperlink.h>
#include <wx/log.h>
#include <wx/radiobox.h>
#include <wx/radiobut.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/srchctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textentry.h>
#include <wx/tokenzr.h>
#include <wx/treebook.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/stc/stc.h>
#include <wx/utils.h>
#include <wx/weakref.h>

#include <utility>

#include <paths.h>

#include <launch_ext.h>

#include <algorithm>

// Maps from dialogTitle <-> pageTitle for keeping track of last-selected pages.
// This is not a simple page index because some dialogs have dynamic page sets.
std::map<wxString, wxString> g_lastPage;
std::map<wxString, wxString> g_lastParentPage;

// Only pages explicitly opted in by their dialog may reuse labels from another instance.
// One entry per dialog, holding the identity its labels were read from so that opening a
// different project replaces them instead of adding to the map for the rest of the session.
struct SEARCH_LABEL_CACHE
{
    wxString                                  identity;
    std::map<wxString, std::vector<wxString>> labels;
};

static std::map<wxString, SEARCH_LABEL_CACHE> s_searchLabels;

// The search box is inset from the left of the tree, and ends where the tree ends
static constexpr int SEARCH_BORDER = 4;


namespace
{
// Controls are rebuilt while the dialog is open, so forget the ones already destroyed
void pruneDestroyed( std::vector<wxWeakRef<wxWindow>>& aWindows )
{
    std::erase_if( aWindows,
                   []( const wxWeakRef<wxWindow>& aRef )
                   {
                       return !aRef;
                   } );
}


wxColour fadedTextColour( const wxTreeCtrl* aTree )
{
#ifdef __WXMAC__
    // GRAYTEXT draws almost as dark as normal text on macOS, so fade the tree's own text halfway
    wxColour fg = aTree->GetForegroundColour();
    wxColour bg = aTree->GetBackgroundColour();

    return wxColour( ( fg.Red() + bg.Red() ) / 2, ( fg.Green() + bg.Green() ) / 2, ( fg.Blue() + bg.Blue() ) / 2 );
#else
    // A high contrast palette picks this colour to stay readable
    return wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT );
#endif
}
} // namespace


PAGED_DIALOG::PAGED_DIALOG( wxWindow* aParent, const wxString& aTitle, bool aShowReset, bool aShowOpenFolder,
                            const wxString& aAuxiliaryAction, const wxSize& aInitialSize ) :
        DIALOG_SHIM( aParent, wxID_ANY, aTitle, wxDefaultPosition, aInitialSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER ),
        m_auxiliaryButton( nullptr ),
        m_resetButton( nullptr ),
        m_openPrefsDirButton( nullptr ),
        m_title( aTitle ),
        m_searchCtrl( nullptr )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( mainSizer );

    m_infoBar = new WX_INFOBAR( this );
    mainSizer->Add( m_infoBar, 0, wxEXPAND, 0 );

    WX_PANEL* treebookPanel = new WX_PANEL( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                            wxBORDER_NONE | wxTAB_TRAVERSAL );
    treebookPanel->SetBorders( false, false, false, true );
    wxBoxSizer* treebookSizer = new wxBoxSizer( wxVERTICAL );
    treebookPanel->SetSizer( treebookSizer );

    m_searchCtrl = new wxSearchCtrl( treebookPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                     wxTE_PROCESS_ENTER );
    m_searchCtrl->ShowCancelButton( true );
    m_searchCtrl->SetDescriptiveText( _( "Search settings" ) );
    treebookSizer->Add( m_searchCtrl, 0, wxTOP | wxBOTTOM | wxLEFT, SEARCH_BORDER );

    // The query is navigation state, not a setting
    ExcludeFromControlUndoRedo( m_searchCtrl );
    OptOut( m_searchCtrl );

    // The sizes callers pass were chosen for a tree that started at the top of the dialog, and
    // DIALOG_SHIM applies that size on the first open, so grow it by the row the search adds
    if( m_initialSize != wxDefaultSize )
        m_initialSize.y += m_searchCtrl->GetBestSize().y + 2 * SEARCH_BORDER;

    m_treebook = new WX_TREEBOOK( treebookPanel, wxID_ANY );
    m_treebook->SetFont( KIUI::GetControlFont( this ) );
    m_treebook->SetFitToCurrentPage( true );

    long treeCtrlFlags = m_treebook->GetTreeCtrl()->GetWindowStyleFlag();
    treeCtrlFlags = ( treeCtrlFlags & ~wxBORDER_MASK ) | wxBORDER_NONE;
    m_treebook->GetTreeCtrl()->SetWindowStyleFlag( treeCtrlFlags );

    treebookSizer->Add( m_treebook, 1, wxEXPAND|wxBOTTOM, 2 );
    mainSizer->Add( treebookPanel, 1, wxEXPAND, 0 );

    m_buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

    if( aShowReset )
    {
        m_resetButton = new wxButton( this, wxID_ANY, _( "Reset to Defaults" ) );
        m_buttonsSizer->Add( m_resetButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
    }

    if( aShowOpenFolder )
    {
#ifdef __WXMAC__
        m_openPrefsDirButton = new wxButton( this, wxID_ANY, _( "Reveal Preferences in Finder" ) );
#else
        m_openPrefsDirButton = new wxButton( this, wxID_ANY, _( "Open Preferences Directory" ) );
#endif
        m_buttonsSizer->Add( m_openPrefsDirButton, 0,
                             wxALIGN_CENTER_VERTICAL | wxRIGHT | wxLEFT, 5 );
    }

    if( !aAuxiliaryAction.IsEmpty() )
    {
        m_auxiliaryButton = new wxButton( this, wxID_ANY, aAuxiliaryAction );
        m_buttonsSizer->Add( m_auxiliaryButton, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );
    }

    m_buttonsSizer->AddStretchSpacer();

    wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
    wxButton* sdbSizerOK = new wxButton( this, wxID_OK );
    sdbSizer->AddButton( sdbSizerOK );
    wxButton* sdbSizerCancel = new wxButton( this, wxID_CANCEL );
    sdbSizer->AddButton( sdbSizerCancel );
    sdbSizer->Realize();

    m_buttonsSizer->Add( sdbSizer, 1, 0, 5 );
    mainSizer->Add( m_buttonsSizer, 0, wxALL|wxEXPAND, 5 );

    SetupStandardButtons();

    // We normally save the dialog size and position based on its class-name.  This class
    // substitutes the title so that each distinctly-titled dialog can have its own saved
    // size and position.
    m_hash_key = aTitle;

    if( m_auxiliaryButton )
        m_auxiliaryButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onAuxiliaryAction, this );

    if( m_resetButton )
        m_resetButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onResetButton, this );

    if( m_openPrefsDirButton )
        m_openPrefsDirButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onOpenPrefsDir, this );

    m_treebook->Bind( wxEVT_CHAR_HOOK, &PAGED_DIALOG::onCharHook, this );
    m_treebook->Bind( wxEVT_TREEBOOK_PAGE_CHANGED, &PAGED_DIALOG::onPageChanged, this );
    m_treebook->Bind( wxEVT_TREEBOOK_PAGE_CHANGING, &PAGED_DIALOG::onPageChanging, this );
    m_treebook->GetTreeCtrl()->Bind( wxEVT_LEFT_DOWN, &PAGED_DIALOG::onTreeLeftDown, this );
    m_treebook->GetTreeCtrl()->Bind( wxEVT_TREE_ITEM_ACTIVATED, &PAGED_DIALOG::onTreeItemActivated, this );

    m_searchCtrl->Bind( wxEVT_TEXT, &PAGED_DIALOG::onSearchText, this );
    m_searchCtrl->Bind( wxEVT_SEARCH_CANCEL, &PAGED_DIALOG::onSearchText, this );
    m_searchCtrl->Bind( wxEVT_SEARCH, &PAGED_DIALOG::onSearchEnter, this );
    m_searchCtrl->Bind( wxEVT_CHAR_HOOK, &PAGED_DIALOG::onSearchCharHook, this );
    Bind( wxEVT_IDLE, &PAGED_DIALOG::onSearchIdle, this );

    // Faded rows hold the colours of the theme they were drawn in, so fade them again after the tree updates
    Bind( wxEVT_SYS_COLOUR_CHANGED,
          [this]( wxSysColourChangedEvent& aEvent )
          {
              aEvent.Skip();
              CallAfter( &PAGED_DIALOG::refadeSearchMisses );
          } );
}


static const wxString TREE_PATH_SEPARATOR = wxS( "  >  " );

static const char* COLLAPSED_NODES_KEY = "__collapsedNodes";


/// Return the pages that have children.
static std::set<size_t> treeNodes( const WX_TREEBOOK* aTreebook )
{
    std::set<size_t> nodes;

    for( size_t page = 0; page < aTreebook->GetPageCount(); ++page )
    {
        int parent = aTreebook->GetPageParent( page );

        if( parent != wxNOT_FOUND )
            nodes.insert( static_cast<size_t>( parent ) );
    }

    return nodes;
}


wxString PAGED_DIALOG::pagePath( size_t aPage ) const
{
    wxString path = m_treebook->GetPageText( aPage );

    for( int parent = m_treebook->GetPageParent( aPage ); parent != wxNOT_FOUND;
         parent = m_treebook->GetPageParent( static_cast<size_t>( parent ) ) )
    {
        path = m_treebook->GetPageText( static_cast<size_t>( parent ) ) + TREE_PATH_SEPARATOR + path;
    }

    return path;
}


std::set<wxString> PAGED_DIALOG::collapsedNodes() const
{
    std::set<wxString> collapsed;

    for( size_t node : treeNodes( m_treebook ) )
    {
        if( !m_treebook->IsNodeExpanded( node ) )
            collapsed.insert( pagePath( node ) );
    }

    return collapsed;
}


void PAGED_DIALOG::saveTreeExpansion() const
{
    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    if( !settings )
        return;

    nlohmann::json collapsed = nlohmann::json::array();

    // Sections a search opened were not opened by the user
    for( const wxString& path : m_collapsedBeforeSearch.value_or( collapsedNodes() ) )
        collapsed.push_back( std::string( path.ToUTF8() ) );

    settings->CsInternals().m_dialogControlValues[GetPersistenceKey()][COLLAPSED_NODES_KEY] = collapsed;
}


void PAGED_DIALOG::restoreTreeExpansion( const std::set<wxString>& aCollapsed )
{
    std::set<int> openAncestors;

    // Collapsing the section holding the open page would select the section instead
    for( int parent = m_treebook->GetSelection(); parent != wxNOT_FOUND; )
    {
        parent = m_treebook->GetPageParent( static_cast<size_t>( parent ) );

        if( parent != wxNOT_FOUND )
            openAncestors.insert( parent );
    }

    for( size_t node : treeNodes( m_treebook ) )
    {
        if( aCollapsed.contains( pagePath( node ) ) && !openAncestors.contains( static_cast<int>( node ) ) )
            m_treebook->CollapseNode( node );
        else
            m_treebook->ExpandNode( node );
    }
}


void PAGED_DIALOG::loadTreeExpansion()
{
    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    if( !settings )
        return;

    auto dialogIt = settings->CsInternals().m_dialogControlValues.find( GetPersistenceKey() );

    if( dialogIt == settings->CsInternals().m_dialogControlValues.end() )
        return;

    auto nodesIt = dialogIt->second.find( COLLAPSED_NODES_KEY );

    if( nodesIt == dialogIt->second.end() || !nodesIt->second.is_array() )
        return;

    std::set<wxString> collapsed;

    for( const nlohmann::json& entry : nodesIt->second )
    {
        if( entry.is_string() )
            collapsed.insert( wxString::FromUTF8( entry.get<std::string>() ) );
    }

    for( size_t node : treeNodes( m_treebook ) )
    {
        if( collapsed.contains( pagePath( node ) ) )
            m_treebook->CollapseNode( node );
    }
}


void PAGED_DIALOG::finishInitialization()
{
    for( size_t i = 1; i < m_treebook->GetPageCount(); ++i )
   	    m_macHack.push_back( true );

    // For some reason adding page labels to the treeCtrl doesn't invalidate its bestSize
    // cache so we have to do it by hand
    m_treebook->GetTreeCtrl()->InvalidateBestSize();

    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
        m_treebook->GetPage( i )->Layout();

    m_treebook->Layout();
    m_treebook->Fit();

    // Add a bit of width to the treeCtrl for scrollbars
    wxSize ctrlSize = m_treebook->GetTreeCtrl()->GetSize();
    ctrlSize.x += 20;
    m_treebook->GetTreeCtrl()->SetMinSize( ctrlSize );
    m_treebook->Layout();

#ifdef __WXGTK__
    // wxSearchCtrl vertical height is not calculated correctly on some GTK setups
    // See https://gitlab.com/kicad/code/kicad/-/issues/9019
    m_searchCtrl->SetMinSize( wxSize( ctrlSize.x - SEARCH_BORDER, GetTextExtent( wxT( "qb" ) ).y + 10 ) );
#else
    m_searchCtrl->SetMinSize( wxSize( ctrlSize.x - SEARCH_BORDER, -1 ) );
#endif

    // Note: do NOT call finishDialogSettings() here.  At this point the lazy pages have not
    // yet been resolved, so the sizer minimum is computed from empty placeholder pages.
    // On GTK+X11, finishDialogSettings() stores a "resize to sizer minimum" operation that
    // is deferred until the window is realized by the window manager.  If that deferred
    // operation fires after onPageChanged() has already grown the dialog to fit the real
    // page content, it shrinks the dialog back to the placeholder minimum, cutting off the
    // bottom of the page.  The dialog minimum and size are correctly established by
    // onPageChanged() once the selected page is resolved.
}


void PAGED_DIALOG::SetInitialPage( const wxString& aPage, const wxString& aParentPage )
{
    g_lastPage[ m_title ] = aPage;
    g_lastParentPage[ m_title ] = aParentPage;
}


PAGED_DIALOG::~PAGED_DIALOG()
{
    saveTreeExpansion();

    // Store the current parentPageTitle/pageTitle hierarchy so we can re-select it
    // next time.
    wxString lastPage = wxEmptyString;
    wxString lastParentPage = wxEmptyString;

    int selected = m_treebook->GetSelection();

    if( selected != wxNOT_FOUND )
    {
        lastPage = m_treebook->GetPageText( (unsigned) selected );

        int parent = m_treebook->GetPageParent( (unsigned) selected );

        if( parent != wxNOT_FOUND )
            lastParentPage = m_treebook->GetPageText( (unsigned) parent );
    }

    g_lastPage[ m_title ] = lastPage;
    g_lastParentPage[ m_title ] = lastParentPage;

    if( m_auxiliaryButton )
        m_auxiliaryButton->Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onAuxiliaryAction, this );

    if( m_resetButton )
        m_resetButton->Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onResetButton, this );

    if( m_openPrefsDirButton )
        m_openPrefsDirButton->Unbind( wxEVT_COMMAND_BUTTON_CLICKED, &PAGED_DIALOG::onOpenPrefsDir, this );

    m_treebook->Unbind( wxEVT_CHAR_HOOK, &PAGED_DIALOG::onCharHook, this );
    m_treebook->Unbind( wxEVT_TREEBOOK_PAGE_CHANGED, &PAGED_DIALOG::onPageChanged, this );
    m_treebook->Unbind( wxEVT_TREEBOOK_PAGE_CHANGING, &PAGED_DIALOG::onPageChanging, this );
    m_treebook->GetTreeCtrl()->Unbind( wxEVT_LEFT_DOWN, &PAGED_DIALOG::onTreeLeftDown, this );
    m_treebook->GetTreeCtrl()->Unbind( wxEVT_TREE_ITEM_ACTIVATED, &PAGED_DIALOG::onTreeItemActivated, this );

    m_searchCtrl->Unbind( wxEVT_TEXT, &PAGED_DIALOG::onSearchText, this );
    m_searchCtrl->Unbind( wxEVT_SEARCH_CANCEL, &PAGED_DIALOG::onSearchText, this );
    m_searchCtrl->Unbind( wxEVT_SEARCH, &PAGED_DIALOG::onSearchEnter, this );
    m_searchCtrl->Unbind( wxEVT_CHAR_HOOK, &PAGED_DIALOG::onSearchCharHook, this );
    Unbind( wxEVT_IDLE, &PAGED_DIALOG::onSearchIdle, this );
}


bool PAGED_DIALOG::TransferDataToWindow()
{
    finishInitialization();

    // Call TransferDataToWindow() only once:
    // this is enough on wxWidgets 3.1
    if( !DIALOG_SHIM::TransferDataToWindow() )
        return false;

    loadTreeExpansion();

    // Search for a page matching the lastParentPageTitle/lastPageTitle hierarchy
    wxString lastPage = g_lastPage[ m_title ];
    wxString lastParentPage = g_lastParentPage[ m_title ];
    int lastPageIndex = wxNOT_FOUND;

    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
    {
        if( m_treebook->GetPageText( i ) == lastPage )
        {
            if( lastParentPage.IsEmpty() )
            {
                lastPageIndex = i;
                break;
            }

            if( m_treebook->GetPageParent( i ) >= 0
                && m_treebook->GetPageText( (unsigned) m_treebook->GetPageParent( i ) ) == lastParentPage )
            {
                lastPageIndex = i;
                break;
            }
        }
    }

    lastPageIndex = std::max( 0, lastPageIndex );
    m_treebook->SetSelection( lastPageIndex );
    UpdateResetButton( lastPageIndex );

    return true;
}


bool PAGED_DIALOG::TransferDataFromWindow()
{
    bool ret = true;

    // Call TransferDataFromWindow() only once:
    // this is enough on wxWidgets 3.1
    if( !DIALOG_SHIM::TransferDataFromWindow() )
        ret = false;

    return ret;
}


wxWindow* PAGED_DIALOG::ShowPage( const wxString& aTitle )
{
    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
    {
        if( m_treebook->GetPageText( i ) == aTitle )
        {
            m_treebook->SetSelection( i );

            // The change can be vetoed by the page being left, or redirected away from a branch
            if( m_treebook->GetSelection() != static_cast<int>( i ) )
                return nullptr;

            return m_treebook->GetResolvedPage( i );
        }
    }

    return nullptr;
}


PAGED_DIALOG* PAGED_DIALOG::GetDialog( wxWindow* aParent )
{
    while( aParent )
    {
        if( PAGED_DIALOG* parentDialog = dynamic_cast<PAGED_DIALOG*>( aParent ) )
            return parentDialog;

        aParent = aParent->GetParent();
    }

    return nullptr;
}


void PAGED_DIALOG::SetError( const wxString& aMessage, const wxString& aPageName, int aCtrlId,
                             int aRow, int aCol )
{
    SetError( aMessage, FindWindow( aPageName ), FindWindow( aCtrlId ), aRow, aCol );
}


void PAGED_DIALOG::SetError( const wxString& aMessage, wxWindow* aPage, wxWindow* aCtrl,
                             int aRow, int aCol )
{
    m_infoBar->ShowMessageFor( aMessage, 10000, wxICON_WARNING );

    if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( aCtrl ) )
    {
        textCtrl->SetSelection( -1, -1 );
        textCtrl->SetFocus();
        return;
    }

    if( wxStyledTextCtrl* scintilla = dynamic_cast<wxStyledTextCtrl*>( aCtrl ) )
    {
        if( aRow > 0 )
        {
            int pos = scintilla->PositionFromLine( aRow - 1 ) + ( aCol - 1 );
            scintilla->GotoPos( pos );
        }

        scintilla->SetFocus();
        return;
    }

    if( wxGrid* grid = dynamic_cast<wxGrid*>( aCtrl ) )
    {
        grid->SetFocus();
        grid->MakeCellVisible( aRow, aCol );
        grid->SetGridCursor( aRow, aCol );

        grid->EnableCellEditControl( true );
        grid->ShowCellEditControl();
        return;
    }
}


void PAGED_DIALOG::UpdateResetButton( int aPage )
{
    wxWindow* panel = m_treebook->ResolvePage( aPage );

    // Enable the reset button only if the page is re-settable
    if( m_resetButton )
    {
        if( panel && ( panel->GetWindowStyle() & wxRESETTABLE ) )
        {
            wxString name = m_treebook->GetPageText( aPage );
            name.Replace( wxT( "&" ), wxT( "&&" ) );

            m_resetButton->SetLabel( wxString::Format( _( "Reset %s to Defaults" ), name ) );
            m_resetButton->SetToolTip( panel->GetHelpTextAtPoint( wxPoint( -INT_MAX, INT_MAX ),
                                                                  wxHelpEvent::Origin_Unknown ) );
            m_resetButton->Enable( true );
        }
        else
        {
            m_resetButton->SetLabel( _( "Reset to Defaults" ) );
            m_resetButton->SetToolTip( wxString() );
            m_resetButton->Enable( false );
        }

        m_resetButton->GetParent()->Layout();
    }
}


void PAGED_DIALOG::onCharHook( wxKeyEvent& aEvent )
{
    if( dynamic_cast<wxTextEntry*>( aEvent.GetEventObject() )
            || dynamic_cast<wxStyledTextCtrl*>( aEvent.GetEventObject() )
            || dynamic_cast<wxListView*>( aEvent.GetEventObject() )
            || dynamic_cast<wxTreeCtrl*>( aEvent.GetEventObject() )
            || dynamic_cast<wxGrid*>( FindFocus() ) )
    {
        aEvent.Skip();
        return;
    }

    if( aEvent.GetKeyCode() == WXK_UP )
    {
        int page = m_treebook->GetSelection();

        if( page >= 1 )
        {
            wxWindow* prevPage = m_treebook->GetPage( page - 1 );

            if( !prevPage || prevPage->GetChildren().IsEmpty() )
                m_treebook->SetSelection( std::max( page - 2, 0 ) );
            else
                m_treebook->SetSelection( page - 1 );
        }

        m_treebook->GetTreeCtrl()->SetFocus();     // Don't allow preview canvas to steal gridFocus
    }
    else if( aEvent.GetKeyCode() == WXK_DOWN )
    {
        int page = m_treebook->GetSelection();

        m_treebook->SetSelection( std::min<int>( page + 1, m_treebook->GetPageCount() - 1 ) );

        m_treebook->GetTreeCtrl()->SetFocus();     // Don't allow preview canvas to steal gridFocus
    }
    else
    {
        aEvent.Skip();
    }
}


void PAGED_DIALOG::onTreeLeftDown( wxMouseEvent& aEvent )
{
    aEvent.Skip();

    wxTreeCtrl*  tree = m_treebook->GetTreeCtrl();
    int          flags = 0;
    wxTreeItemId item = tree->HitTest( aEvent.GetPosition(), flags );

    m_clickExpandedItem = wxTreeItemId();

    // A label click opens the branch so onPageChanged() moves on to its first sub-page
    // The expander is left alone so the branch can still be closed
    if( item.IsOk() && ( flags & wxTREE_HITTEST_ONITEMLABEL ) && !tree->IsExpanded( item )
        && tree->ItemHasChildren( item ) )
    {
        tree->Expand( item );
        m_clickExpandedItem = item;

        // The selection does not change, so onPageChanged() will not move on to the first sub-page.
        // Deferred so the tree finishes handling the click first.
        if( tree->GetSelection() == item )
        {
            int page = m_treebook->GetSelection();

            CallAfter(
                    [this, page]()
                    {
                        wxWindow* current = m_treebook->GetCurrentPage();

                        if( m_treebook->GetSelection() == page && current && current->GetChildren().IsEmpty()
                            && page + 1 < (int) m_treebook->GetPageCount() && m_treebook->IsNodeExpanded( page ) )
                        {
                            m_treebook->SetSelection( page + 1 );
                        }
                    } );
        }
    }
}


void PAGED_DIALOG::onTreeItemActivated( wxTreeEvent& aEvent )
{
    // A double-click would otherwise toggle shut the branch its first click just opened
    if( aEvent.GetItem() == m_clickExpandedItem )
    {
        m_clickExpandedItem = wxTreeItemId();
        return;
    }

    aEvent.Skip();
}


void PAGED_DIALOG::ExcludeFromSearchIndex( wxWindow* aWindow )
{
    if( PAGED_DIALOG* dialog = GetDialog( aWindow ) )
        dialog->excludeFromSearchIndex( aWindow );
}


void PAGED_DIALOG::excludeFromSearchIndex( wxWindow* aWindow )
{
    pruneDestroyed( m_searchExcludedControls );
    m_searchExcludedControls.emplace_back( aWindow );
    InvalidateSearchIndex( aWindow );
}


bool PAGED_DIALOG::isExcludedFromSearch( const wxWindow* aWindow ) const
{
    return std::ranges::any_of( m_searchExcludedControls,
                                [aWindow]( const wxWeakRef<wxWindow>& aRef )
                                {
                                    return aRef.get() == aWindow;
                                } );
}


void PAGED_DIALOG::ReuseSearchLabels( const wxString& aScope, const wxString& aIdentity,
                                      const std::set<size_t>& aPages )
{
    m_searchReuseScope = aScope;
    m_reusableSearchPaths.clear();

    SEARCH_LABEL_CACHE& cache = s_searchLabels[aScope];

    // Labels describe the project they were read from, so another one starts over rather than
    // reading the last one's wording.
    if( cache.identity != aIdentity )
    {
        cache.identity = aIdentity;
        cache.labels.clear();
    }

    // Kept as tree paths, not indices: a page added or moved later would leave an index
    // pointing at whatever took its place.
    for( size_t page : aPages )
    {
        wxCHECK2_MSG( page < m_treebook->GetPageCount(), continue,
                      wxS( "ReuseSearchLabels() takes pages of a tree that is already built" ) );

        m_reusableSearchPaths.insert( pagePath( page ) );
    }
}


void PAGED_DIALOG::ExcludeBookTitlesFromSearchIndex( wxBookCtrlBase* aBook )
{
    if( PAGED_DIALOG* dialog = GetDialog( aBook ) )
        dialog->excludeBookTitlesFromSearchIndex( aBook );
}


void PAGED_DIALOG::excludeBookTitlesFromSearchIndex( wxBookCtrlBase* aBook )
{
    pruneDestroyed( m_searchExcludedBookTitles );
    m_searchExcludedBookTitles.emplace_back( aBook );
    InvalidateSearchIndex( aBook );
}


void PAGED_DIALOG::InvalidateSearchIndex( wxWindow* aWindow )
{
    if( PAGED_DIALOG* dialog = GetDialog( aWindow ) )
        dialog->invalidateSearchIndex( aWindow );
}


void PAGED_DIALOG::invalidateSearchIndex( wxWindow* aWindow )
{
    // A page under construction has not been read yet, and its labels are read once it is built
    if( m_treebook->IsBuildingPage() )
        return;

    for( size_t ii = 0; ii < m_treebook->GetPageCount(); ++ii )
    {
        wxWindow* page = m_treebook->GetPage( ii );

        if( page != aWindow && !page->IsDescendant( aWindow ) )
            continue;

        if( const wxString path = pagePath( ii ); m_reusableSearchPaths.contains( path ) )
            s_searchLabels[m_searchReuseScope].labels.erase( path );

        bool indexed = ii < m_searchLabelsPending.size() && !m_searchLabelsPending[ii];

        if( indexed )
        {
            m_searchIndex[ii] = { pageSearchTitle( ii ) };
            m_searchLabelsPending[ii] = true;
            CallAfter( &PAGED_DIALOG::refreshSearch );
        }

        return;
    }
}


void PAGED_DIALOG::collectSearchText( wxWindow* aWindow, std::vector<wxString>& aTexts ) const
{
    if( isExcludedFromSearch( aWindow ) )
        return;

    // Include inactive book pages, but not windows removed from the book.
    if( wxBookCtrlBase* book = dynamic_cast<wxBookCtrlBase*>( aWindow ) )
    {
        bool titlesExcluded = std::ranges::any_of( m_searchExcludedBookTitles,
                                                   [book]( const wxWeakRef<wxWindow>& aRef )
                                                   {
                                                       return aRef.get() == book;
                                                   } );

        for( size_t ii = 0; ii < book->GetPageCount(); ++ii )
        {
            if( !titlesExcluded )
                aTexts.push_back( book->GetPageText( ii ).Lower() );

            collectSearchText( book->GetPage( ii ), aTexts );
        }

        return;
    }

    for( wxWindow* child : aWindow->GetChildren() )
    {
        if( isExcludedFromSearch( child ) )
            continue;

        if( !child->IsShown() )
            continue;

        // wxRadioBox derives from wxStaticBox on MSW, so test it first
        if( wxRadioBox* radioBox = dynamic_cast<wxRadioBox*>( child ) )
        {
            aTexts.push_back( radioBox->GetLabelText().Lower() );

            for( unsigned int ii = 0; ii < radioBox->GetCount(); ++ii )
                aTexts.push_back( wxStripMenuCodes( radioBox->GetString( ii ), wxStrip_Mnemonics ).Lower() );
        }
        else if( dynamic_cast<wxStaticText*>( child ) || dynamic_cast<wxCheckBox*>( child )
                 || dynamic_cast<wxRadioButton*>( child ) || dynamic_cast<wxButton*>( child )
                 || dynamic_cast<wxStaticBox*>( child ) || dynamic_cast<wxHyperlinkCtrl*>( child ) )
        {
            aTexts.push_back( static_cast<wxControl*>( child )->GetLabelText().Lower() );
        }
        else if( wxGrid* grid = dynamic_cast<wxGrid*>( child ) )
        {
            for( int col = 0; col < grid->GetNumberCols(); ++col )
                aTexts.push_back( grid->GetColLabelValue( col ).Lower() );
        }

        collectSearchText( child, aTexts );
    }
}


void PAGED_DIALOG::AppendControlSearchTerms( wxWindow* aWindow, std::vector<wxString>& aTerms, bool aIncludeValues )
{
    if( wxBookCtrlBase* book = dynamic_cast<wxBookCtrlBase*>( aWindow ) )
    {
        for( size_t ii = 0; ii < book->GetPageCount(); ++ii )
            AppendControlSearchTerms( book->GetPage( ii ), aTerms, aIncludeValues );

        return;
    }

    if( wxGrid* grid = dynamic_cast<wxGrid*>( aWindow ) )
    {
        if( !aIncludeValues )
            return;

        const int editRow = grid->GetGridCursorRow();
        const int editCol = grid->GetGridCursorCol();

        for( int row = 0; row < grid->GetNumberRows(); ++row )
        {
            for( int col = 0; col < grid->GetNumberCols(); ++col )
            {
                if( grid->IsCellEditControlEnabled() && row == editRow && col == editCol )
                {
                    wxGridCellEditor* editor = grid->GetCellEditor( row, col );
                    aTerms.push_back( editor->GetValue() );
                    editor->DecRef();
                }
                else
                {
                    aTerms.push_back( grid->GetCellValue( row, col ) );
                }
            }
        }

        return;
    }

    if( wxItemContainerImmutable* choices = dynamic_cast<wxItemContainerImmutable*>( aWindow ) )
    {
        for( unsigned int ii = 0; ii < choices->GetCount(); ++ii )
            aTerms.push_back( choices->GetString( ii ) );
    }

    if( aIncludeValues )
    {
        // Spin controls share no text entry base on every port, and wxMSW's integer one not even wxSpinCtrlBase
        if( wxSpinCtrl* spin = dynamic_cast<wxSpinCtrl*>( aWindow ) )
        {
            aTerms.push_back( spin->GetTextValue() );
            return;
        }

        if( wxSpinCtrlDouble* spin = dynamic_cast<wxSpinCtrlDouble*>( aWindow ) )
        {
            aTerms.push_back( spin->GetTextValue() );
            return;
        }

        if( wxTextEntryBase* entry = dynamic_cast<wxTextEntryBase*>( aWindow ) )
        {
            if( !aWindow->HasFlag( wxTE_PASSWORD ) )
                aTerms.push_back( entry->GetValue() );

            return;
        }
    }

    for( wxWindow* child : aWindow->GetChildren() )
    {
        if( child->IsShown() )
            AppendControlSearchTerms( child, aTerms, aIncludeValues );
    }
}


void PAGED_DIALOG::AppendSearchTerms( size_t aPage, std::vector<wxString>& aTerms ) const
{
    if( wxWindow* page = m_treebook->GetResolvedPage( aPage ) )
        AppendControlSearchTerms( page, aTerms );
}


void PAGED_DIALOG::ReplaceSearchTerm( std::vector<wxString>& aTerms, size_t aFirst, const wxString& aShown,
                                      const wxString& aCurrent )
{
    auto it = std::find( aTerms.begin() + aFirst, aTerms.end(), aShown );

    if( it != aTerms.end() )
        *it = aCurrent;
    else
        aTerms.push_back( aCurrent );
}


void PAGED_DIALOG::AppendTextVarSearchTerms( std::vector<wxString>& aTerms ) const
{
    for( const auto& [name, value] : Prj().GetTextVars() )
    {
        aTerms.push_back( name );
        aTerms.push_back( value );
    }
}


wxString PAGED_DIALOG::pageSearchTitle( size_t aPage ) const
{
    wxString title = m_treebook->GetPageText( aPage );
    int      parent = m_treebook->GetPageParent( aPage );

    // Lets "schematic colors" find the Colors page under Schematic Editor
    if( parent != wxNOT_FOUND )
        title = m_treebook->GetPageText( parent ) + wxS( " " ) + title;

    return title.Lower();
}


std::vector<wxString> PAGED_DIALOG::buildPageSearchIndex( size_t aPage )
{
    std::vector<wxString> texts;

    texts.push_back( pageSearchTitle( aPage ) );
    collectSearchText( m_treebook->ResolvePageForSearch( aPage ), texts );
    return texts;
}


void PAGED_DIALOG::indexPage( size_t aPage )
{
    m_searchIndex[aPage] = buildPageSearchIndex( aPage );
    m_searchLabelsPending[aPage] = false;

    if( const wxString path = pagePath( aPage ); m_reusableSearchPaths.contains( path ) )
        s_searchLabels[m_searchReuseScope].labels[path] = m_searchIndex[aPage];
}


void PAGED_DIALOG::onSearchIdle( wxIdleEvent& aEvent )
{
    aEvent.Skip();

    // A page build can yield, and indexing another page inside it would restore focus out of order
    if( !m_indexingSearch || m_indexingPage )
        return;

    auto missing = std::ranges::find( m_searchLabelsPending, true );

    if( missing != m_searchLabelsPending.end() )
    {
        size_t           page = std::distance( m_searchLabelsPending.begin(), missing );
        SCOPED_SET_RESET indexing( m_indexingPage, true );
        FOCUS_SNAPSHOT   focus;
        PROF_TIMER       timer;
        bool             wasBuilt = m_treebook->GetResolvedPage( page ) != nullptr;

        // Build one page per idle event so input can be handled between page constructors.
        indexPage( page );

        // Keeping every page built can exhaust the GDI handles of a Windows process
        if( m_releaseIndexedPages && !wasBuilt && !m_treebook->IsPageActive( page ) )
        {
            std::vector<wxString>& values = m_releasedPageValues[page];

            values.clear();
            AppendSearchTerms( page, values );

            for( wxString& value : values )
                value.MakeLower();

            m_treebook->ReleasePageBuiltForSearch( page );
        }

        focus.Restore();
        timer.Stop();
        wxLogTrace( traceUiProfile, wxS( "Indexing page '%s' of '%s' for search took %s." ),
                    m_treebook->GetPageText( page ), m_title, timer.to_string() );
    }

    if( std::ranges::find( m_searchLabelsPending, true ) != m_searchLabelsPending.end() )
    {
        aEvent.RequestMore();
        return;
    }

    m_indexingSearch = false;
    updateSearch( m_pendingSearchAction );
}


int PAGED_DIALOG::applySearch()
{
    wxTreeCtrl*           tree = m_treebook->GetTreeCtrl();
    size_t                count = m_treebook->GetPageCount();
    std::vector<wxString> terms;
    wxStringTokenizer     tokenizer( m_searchCtrl->GetValue().Lower(), wxS( " \t" ) );

    while( tokenizer.HasMoreTokens() )
        terms.push_back( tokenizer.GetNextToken() );

    if( terms.empty() )
    {
        m_indexingSearch = false;

        for( size_t ii = 0; ii < count; ++ii )
        {
            tree->SetItemTextColour( m_treebook->GetPageTreeItem( ii ), wxNullColour );
            tree->SetItemBold( m_treebook->GetPageTreeItem( ii ), false );
        }

        if( m_collapsedBeforeSearch )
        {
            restoreTreeExpansion( *m_collapsedBeforeSearch );
            m_collapsedBeforeSearch.reset();
        }

        return wxNOT_FOUND;
    }

    if( !m_collapsedBeforeSearch )
        m_collapsedBeforeSearch = collapsedNodes();

    if( m_searchIndex.size() != count )
    {
        m_searchIndex.clear();
        m_searchIndex.resize( count );
        m_releasedPageValues.clear();
        m_releasedPageValues.resize( count );

        m_searchLabelsPending.assign( count, true );

        for( size_t page = 0; page < count; ++page )
        {
            const wxString path = pagePath( page );

            if( !m_reusableSearchPaths.contains( path ) )
                continue;

            auto cached = s_searchLabels[m_searchReuseScope].labels.find( path );

            if( cached != s_searchLabels[m_searchReuseScope].labels.end() )
            {
                m_searchIndex[page] = cached->second;
                m_searchLabelsPending[page] = false;
            }
        }

        // Built pages are cheap to read, and reading the open page now keeps it from being left
        // on the first key only because its labels were not read yet
        for( size_t page = 0; page < count; ++page )
        {
            if( m_searchLabelsPending[page] && m_treebook->GetResolvedPage( page ) )
                indexPage( page );
        }

        // Seeding each page with its own title answers the first keystroke before any page is read
        for( size_t page = 0; page < count; ++page )
        {
            if( m_searchLabelsPending[page] )
                m_searchIndex[page].push_back( pageSearchTitle( page ) );
        }
    }

    if( std::ranges::find( m_searchLabelsPending, true ) != m_searchLabelsPending.end() )
    {
        m_indexingSearch = true;
        wxWakeUpIdle();
    }

    BeginSearchPass();

    std::vector<bool> matches( count, false );
    std::vector<bool> found( terms.size(), false );

    auto takeTerms = [&]( const wxString& aText )
    {
        for( size_t jj = 0; jj < terms.size(); ++jj )
        {
            if( !found[jj] && aText.Contains( terms[jj] ) )
                found[jj] = true;
        }
    };

    auto foundEveryTerm = [&]()
    {
        return std::ranges::find( found, false ) == found.end();
    };

    for( size_t ii = 0; ii < count; ++ii )
    {
        // A page matches when every word is found somewhere on it. Testing each label for the
        // whole query would miss words that are both on the page but in different labels.
        std::fill( found.begin(), found.end(), false );

        for( const wxString& text : m_searchIndex[ii] )
            takeTerms( text );

        // A page still waiting for its labels is matched on its title alone. Reading its
        // values here would build it, which is what the idle pass exists to spread out.
        if( !foundEveryTerm() && !m_searchLabelsPending[ii] )
        {
            // Project values can change after the label index is built.
            std::vector<wxString> live;

            AppendSearchTerms( ii, live );

            for( const wxString& term : live )
                takeTerms( term.Lower() );

            if( !m_treebook->GetResolvedPage( ii ) )
            {
                for( const wxString& value : m_releasedPageValues[ii] )
                    takeTerms( value );
            }
        }

        matches[ii] = foundEveryTerm();
    }

    for( size_t ii = 0; ii < count; ++ii )
    {
        if( !matches[ii] )
            continue;

        for( int parent = m_treebook->GetPageParent( ii ); parent != wxNOT_FOUND;
             parent = m_treebook->GetPageParent( parent ) )
        {
            matches[parent] = true;
            m_treebook->ExpandNode( parent );
        }
    }

    wxColour greyed = fadedTextColour( tree );
    int      current = m_treebook->GetSelection();
    int      target = wxNOT_FOUND;

    for( size_t ii = 0; ii < count; ++ii )
    {
        tree->SetItemBold( m_treebook->GetPageTreeItem( ii ), matches[ii] );
        tree->SetItemTextColour( m_treebook->GetPageTreeItem( ii ), matches[ii] ? wxNullColour : greyed );

        // Keep the current match selected and skip empty category pages.
        if( matches[ii] && ( m_treebook->IsLazyPage( ii ) || !m_treebook->GetPage( ii )->GetChildren().IsEmpty() )
            && ( target == wxNOT_FOUND || static_cast<int>( ii ) == current ) )
        {
            target = static_cast<int>( ii );
        }
    }

    return target;
}


bool PAGED_DIALOG::showHistoryControl( wxWindow* aCtrl )
{
    for( size_t ii = 0; ii < m_treebook->GetPageCount(); ++ii )
    {
        wxWindow* page = m_treebook->GetResolvedPage( ii );

        if( !page || ( page != aCtrl && !page->IsDescendant( aCtrl ) ) )
            continue;

        if( static_cast<int>( ii ) != m_treebook->GetSelection() )
            m_treebook->SetSelection( ii );

        // Leaving the open page can be refused
        return static_cast<int>( ii ) == m_treebook->GetSelection();
    }

    return true;
}


void PAGED_DIALOG::refadeSearchMisses()
{
    wxTreeCtrl* tree = m_treebook->GetTreeCtrl();
    wxColour    greyed = fadedTextColour( tree );

    // Rerunning the search would also reopen sections the user collapsed since
    for( size_t ii = 0; ii < m_treebook->GetPageCount(); ++ii )
    {
        wxTreeItemId item = m_treebook->GetPageTreeItem( ii );

        if( tree->GetItemTextColour( item ).IsOk() )
            tree->SetItemTextColour( item, greyed );
    }
}


void PAGED_DIALOG::refreshSearch()
{
    if( !m_searchCtrl->IsEmpty() )
        updateSearch( SEARCH_ACTION::REFRESH );
}


void PAGED_DIALOG::onSearchText( wxCommandEvent& aEvent )
{
    // Consuming the cancel event stops the generic control from clearing itself
    if( aEvent.GetEventType() == wxEVT_SEARCH_CANCEL )
    {
        m_searchCtrl->Clear();
        return;
    }

    // Required to avoid interaction with SetHint()
    aEvent.Skip();

    updateSearch( SEARCH_ACTION::SELECT_PAGE );
}


void PAGED_DIALOG::updateSearch( SEARCH_ACTION aAction )
{
    FOCUS_SNAPSHOT focus;
    int            target = applySearch();

    // Pages still being read can match later, so the query acts again once they are all read
    if( !m_indexingSearch )
        m_pendingSearchAction = SEARCH_ACTION::REFRESH;
    else if( aAction != SEARCH_ACTION::REFRESH )
        m_pendingSearchAction = aAction;

    if( aAction != SEARCH_ACTION::REFRESH && target != wxNOT_FOUND && target != m_treebook->GetSelection() )
    {
        SCOPED_SET_RESET selecting( m_selectingSearchMatch, true );
        m_treebook->SetSelection( target );
    }

    if( aAction == SEARCH_ACTION::FOCUS_TREE && target != wxNOT_FOUND && target == m_treebook->GetSelection() )
    {
        m_treebook->GetTreeCtrl()->SetFocus();
        return;
    }

    // Page construction can move focus. Keep focus on any error that prevents a page change.
    bool vetoed = target != wxNOT_FOUND && target != m_treebook->GetSelection();

    if( focus.IsIn( m_searchCtrl ) && !vetoed )
        focus.Restore();
}


void PAGED_DIALOG::onSearchEnter( wxCommandEvent& aEvent )
{
    updateSearch( SEARCH_ACTION::FOCUS_TREE );
}


void PAGED_DIALOG::onSearchCharHook( wxKeyEvent& aEvent )
{
    // First Escape clears the query. A second closes the dialog.
    // Handle this here so DIALOG_SHIM does not restore the old query.
    if( aEvent.GetKeyCode() == WXK_ESCAPE )
    {
        if( !m_searchCtrl->IsEmpty() )
            m_searchCtrl->Clear();
        else
            EmulateButtonClickIfPresent( wxID_CANCEL );

        return;
    }

    int key = aEvent.GetKeyCode();

    // The dialog's undo would revert a control on another page, so the box keeps these keys
    if( aEvent.ControlDown() && !aEvent.AltDown() && ( key == 'Z' || key == 'Y' ) )
    {
        aEvent.DoAllowNextEvent();
        return;
    }

    aEvent.Skip();
}


void PAGED_DIALOG::onPageChanged( wxBookCtrlEvent& event )
{
    int sel = event.GetSelection();

    if( sel == wxNOT_FOUND )
        return;

    size_t page = static_cast<size_t>( sel );

    // Use the first sub-page when a tree level node is selected, but only if the node is
    // expanded. When the user collapses a branch, wxTreebook auto-selects the parent node.
    // Redirecting to the first child here would undo the collapse.
    wxWindow* currentPage = m_treebook->GetCurrentPage();

    if( currentPage && currentPage->GetChildren().IsEmpty()
            && page + 1 < m_treebook->GetPageCount()
            && m_treebook->IsNodeExpanded( page ) )
    {
        m_treebook->ChangeSelection( page + 1 );
    }

    page = m_treebook->GetSelection();

    UpdateResetButton( page );

    // Make sure the dialog size is enough to fit the page content.
    m_treebook->InvalidateBestSize();

    SetMinSize( wxDefaultSize );
    wxSize minSize = GetBestSize();
    minSize.IncTo( FromDIP( wxSize( 600, 500 ) ) );
    minSize.DecTo( FromDIP( wxSize( 1500, 900 ) ) ); // Failsafe

    // Clamp to the current display so the resize border stays reachable (#24408).
    int displayIdx = wxDisplay::GetFromWindow( this );

    if( displayIdx == wxNOT_FOUND )
        displayIdx = 0;

    wxRect displayArea = wxDisplay( (unsigned int) displayIdx ).GetClientArea();
    wxSize maxSize( std::max( 0, displayArea.width  - FromDIP( 40 ) ),
                    std::max( 0, displayArea.height - FromDIP( 80 ) ) );
    minSize.DecTo( maxSize );

    SetMinSize( minSize );

    wxSize currentSize = GetSize();
    wxSize newSize = currentSize;
    newSize.IncTo( minSize );

    if( newSize != currentSize )
        SetSize( newSize );

#ifdef __WXMAC__
    // Work around an OSX wxWidgets issue where the wxGrid children don't get placed correctly
    // until the first resize event
    if( page < m_macHack.size() && m_macHack[ page ] )
    {
        wxSize pageSize = m_treebook->GetPage( page )->GetSize();
        pageSize.x += 1;
        pageSize.y += 2;

        m_treebook->GetPage( page )->SetSize( pageSize );
        m_macHack[ page ] = false;
    }
#else
    wxSizeEvent evt( wxDefaultSize );
    wxQueueEvent( m_treebook, evt.Clone() );
#endif

    // A page the user chose must not be left again when the last labels are read
    if( !m_selectingSearchMatch )
        m_pendingSearchAction = SEARCH_ACTION::REFRESH;

    // Refresh after derived page handlers have updated their project data.
    CallAfter( &PAGED_DIALOG::refreshSearch );
}


void PAGED_DIALOG::onPageChanging( wxBookCtrlEvent& aEvent )
{
    int currentPage = aEvent.GetOldSelection();

    if( currentPage == wxNOT_FOUND )
        return;

    wxWindow* page = m_treebook->GetPage( currentPage );

    wxCHECK( page, /* void */ );

    // If there is a validation error on the current page, don't allow the page change.
    if( !page->Validate() || !page->TransferDataFromWindow() )
    {
        aEvent.Veto();
        return;
    }
}


void PAGED_DIALOG::onResetButton( wxCommandEvent& aEvent )
{
    int sel = m_treebook->GetSelection();

    if( sel == wxNOT_FOUND )
        return;

    // NB: dynamic_cast doesn't work over Kiway
    wxWindow* panel = m_treebook->ResolvePage( sel );

    if( panel )
    {
        wxCommandEvent resetCommand( wxEVT_COMMAND_BUTTON_CLICKED, ID_RESET_PANEL );
        panel->ProcessWindowEvent( resetCommand );
    }
}

void PAGED_DIALOG::onOpenPrefsDir( wxCommandEvent& aEvent )
{
    wxString dir( PATHS::GetUserSettingsPath() );
    LaunchExternal( dir );
}
