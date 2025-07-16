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

#include <pgm_base.h>
#include <design_block.h>
#include <design_block_pane.h>
#include <design_block_library_adapter.h>
#include <panel_design_block_chooser.h>
#include <design_block_preview_widget.h>
#include <kiface_base.h>
#include <kiway_holder.h>
#include <eda_draw_frame.h>
#include <widgets/lib_tree.h>
#include <settings/settings_manager.h>
#include <project/project_file.h>
#include <dialogs/html_message_box.h>
#include <settings/app_settings.h>
#include <string_utils.h>
#include <wx/log.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/timer.h>
#include <wx/wxhtml.h>
#include <wx/msgdlg.h>
#include <widgets/wx_progress_reporters.h>


wxString PANEL_DESIGN_BLOCK_CHOOSER::g_designBlockSearchString;


PANEL_DESIGN_BLOCK_CHOOSER::PANEL_DESIGN_BLOCK_CHOOSER( EDA_DRAW_FRAME* aFrame, DESIGN_BLOCK_PANE* aParent,
                                                        std::vector<LIB_ID>&  aHistoryList,
                                                        std::function<void()> aSelectHandler,
                                                        TOOL_INTERACTIVE*     aContextMenuTool ) :

        wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize ),
        m_dbl_click_timer( nullptr ),
        m_open_libs_timer( nullptr ),
        m_vsplitter( nullptr ),
        m_tree( nullptr ),
        m_preview( nullptr ),
        m_parent( aParent ),
        m_frame( aFrame ),
        m_selectHandler( std::move( aSelectHandler ) ),
        m_historyList( aHistoryList )
{
    DESIGN_BLOCK_LIBRARY_ADAPTER* libs = m_frame->Prj().DesignBlockLibs();

    m_adapter = DESIGN_BLOCK_TREE_MODEL_ADAPTER::Create(
            m_frame, libs, m_frame->config()->m_DesignBlockChooserPanel.tree, aContextMenuTool );

    // -------------------------------------------------------------------------------------
    // Construct the actual panel
    //

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_vsplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxSP_LIVE_UPDATE | wxSP_NOBORDER | wxSP_3DSASH );


    // Avoid the splitter window being assigned as the parent to additional windows.
    m_vsplitter->SetExtraStyle( wxWS_EX_TRANSIENT );

    wxPanel*    treePanel = new wxPanel( m_vsplitter );
    wxBoxSizer* treeSizer = new wxBoxSizer( wxVERTICAL );
    treePanel->SetSizer( treeSizer );

    m_detailsPanel = new wxPanel( m_vsplitter );
    m_detailsSizer = new wxBoxSizer( wxVERTICAL );
    m_detailsPanel->SetSizer( m_detailsSizer );

    m_detailsPanel->Layout();
    m_detailsSizer->Fit( m_detailsPanel );

    m_vsplitter->SetSashGravity( 0.5 );
    m_vsplitter->SetMinimumPaneSize( 20 );
    m_vsplitter->SplitHorizontally( treePanel, m_detailsPanel );

    sizer->Add( m_vsplitter, 1, wxEXPAND, 5 );

    m_tree = new LIB_TREE( treePanel, wxT( "design_blocks" ), m_adapter, LIB_TREE::FLAGS::ALL_WIDGETS, nullptr );

    treeSizer->Add( m_tree, 1, wxEXPAND, 5 );
    treePanel->Layout();
    treeSizer->Fit( treePanel );

    RefreshLibs();
    m_adapter->FinishTreeInitialization();

    m_tree->SetSearchString( g_designBlockSearchString );

    m_dbl_click_timer = new wxTimer( this );
    m_open_libs_timer = new wxTimer( this );

    SetSizer( sizer );

    Layout();

    Bind( wxEVT_TIMER, &PANEL_DESIGN_BLOCK_CHOOSER::onCloseTimer, this, m_dbl_click_timer->GetId() );
    Bind( wxEVT_TIMER, &PANEL_DESIGN_BLOCK_CHOOSER::onOpenLibsTimer, this, m_open_libs_timer->GetId() );
    Bind( EVT_LIBITEM_CHOSEN, &PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockChosen, this );
    Bind( wxEVT_CHAR_HOOK, &PANEL_DESIGN_BLOCK_CHOOSER::OnChar, this );

    // Open the user's previously opened libraries on timer expiration.
    // This is done on a timer because we need a gross hack to keep GTK from garbling the
    // display. Must be longer than the search debounce timer.
    m_open_libs_timer->StartOnce( 300 );
}


PANEL_DESIGN_BLOCK_CHOOSER::~PANEL_DESIGN_BLOCK_CHOOSER()
{
    Unbind( wxEVT_TIMER, &PANEL_DESIGN_BLOCK_CHOOSER::onCloseTimer, this );
    Unbind( EVT_LIBITEM_SELECTED, &PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockSelected, this );
    Unbind( EVT_LIBITEM_CHOSEN, &PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockChosen, this );
    Unbind( wxEVT_CHAR_HOOK, &PANEL_DESIGN_BLOCK_CHOOSER::OnChar, this );

    // Stop the timer during destruction early to avoid potential race conditions (that do happen)
    m_dbl_click_timer->Stop();
    m_open_libs_timer->Stop();
    delete m_dbl_click_timer;
    delete m_open_libs_timer;
}


void PANEL_DESIGN_BLOCK_CHOOSER::SaveSettings()
{
    g_designBlockSearchString = m_tree->GetSearchString();

    if( APP_SETTINGS_BASE* cfg = m_frame->config() )
    {
        // Save any changes to column widths, etc.
        m_adapter->SaveSettings();

        cfg->m_DesignBlockChooserPanel.width = GetParent()->GetSize().x;
        cfg->m_DesignBlockChooserPanel.height = GetParent()->GetSize().y;
        cfg->m_DesignBlockChooserPanel.sash_pos_v = m_vsplitter->GetSashPosition();
        cfg->m_DesignBlockChooserPanel.sort_mode = m_tree->GetSortMode();
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::ShowChangedLanguage()
{
    if( m_tree )
        m_tree->ShowChangedLanguage();
}


void PANEL_DESIGN_BLOCK_CHOOSER::SetPreviewWidget( DESIGN_BLOCK_PREVIEW_WIDGET* aPreview )
{
    m_preview = aPreview;
    m_detailsSizer->Add( m_preview, 1, wxEXPAND, 5 );
    Layout();
}


void PANEL_DESIGN_BLOCK_CHOOSER::OnChar( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_ESCAPE )
    {
        wxObject* eventSource = aEvent.GetEventObject();

        if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( eventSource ) )
        {
            // First escape cancels search string value
            if( textCtrl->GetValue() == m_tree->GetSearchString() && !m_tree->GetSearchString().IsEmpty() )
            {
                m_tree->SetSearchString( wxEmptyString );
                return;
            }
        }
    }
    else
    {
        aEvent.Skip();
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::FinishSetup()
{
    if( APP_SETTINGS_BASE* cfg = m_frame->config() )
    {
        auto horizPixelsFromDU =
                [&]( int x ) -> int
                {
                    wxSize sz( x, 0 );
                    return GetParent()->ConvertDialogToPixels( sz ).x;
                };

        APP_SETTINGS_BASE::PANEL_DESIGN_BLOCK_CHOOSER& panelCfg = cfg->m_DesignBlockChooserPanel;

        int w = panelCfg.width > 40 ? panelCfg.width : horizPixelsFromDU( 440 );
        int h = panelCfg.height > 40 ? panelCfg.height : horizPixelsFromDU( 340 );

        GetParent()->SetSize( wxSize( w, h ) );
        GetParent()->Layout();

        // We specify the width of the right window (m_design_block_view_panel), because specify
        // the width of the left window does not work as expected when SetSashGravity() is called

        if( panelCfg.sash_pos_h < 0 )
            panelCfg.sash_pos_h = horizPixelsFromDU( 220 );

        if( panelCfg.sash_pos_v < 0 )
            panelCfg.sash_pos_v = horizPixelsFromDU( 230 );

        if( m_vsplitter )
            m_vsplitter->SetSashPosition( panelCfg.sash_pos_v );

        m_adapter->SetSortMode( (LIB_TREE_MODEL_ADAPTER::SORT_MODE) panelCfg.sort_mode );
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::RefreshLibs( bool aProgress )
{
    // Unselect before syncing to avoid null reference in the adapter
    // if a selected item is removed during the sync
    m_tree->Unselect();

    DESIGN_BLOCK_TREE_MODEL_ADAPTER* adapter = static_cast<DESIGN_BLOCK_TREE_MODEL_ADAPTER*>( m_adapter.get() );

    // Clear all existing libraries then re-add
    adapter->ClearLibraries();

    rebuildHistoryNode();

    if( !m_historyList.empty() )
        adapter->SetPreselectNode( m_historyList[0], 0 );

    adapter->AddLibraries( m_frame );

    m_tree->Regenerate( true );
    Refresh();
}


void PANEL_DESIGN_BLOCK_CHOOSER::SetPreselect( const LIB_ID& aPreselect )
{
    m_adapter->SetPreselectNode( aPreselect, 0 );
}


LIB_ID PANEL_DESIGN_BLOCK_CHOOSER::GetSelectedLibId( int* aUnit ) const
{
    return m_tree->GetSelectedLibId( aUnit );
}


void PANEL_DESIGN_BLOCK_CHOOSER::SelectLibId( const LIB_ID& aLibId )
{
    m_tree->CenterLibId( aLibId );
    m_tree->SelectLibId( aLibId );
}


void PANEL_DESIGN_BLOCK_CHOOSER::onCloseTimer( wxTimerEvent& aEvent )
{
    // Hack because of eaten MouseUp event. See PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockChosen
    // for the beginning of this spaghetti noodle.

    wxMouseState state = wxGetMouseState();

    if( state.LeftIsDown() )
    {
        // Mouse hasn't been raised yet, so fire the timer again. Otherwise the
        // purpose of this timer is defeated.
        m_dbl_click_timer->StartOnce( PANEL_DESIGN_BLOCK_CHOOSER::DBLCLICK_DELAY );
    }
    else
    {
        m_frame->GetCanvas()->SetFocus();
        m_selectHandler();
        addDesignBlockToHistory( m_tree->GetSelectedLibId() );
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::onOpenLibsTimer( wxTimerEvent& aEvent )
{
    if( APP_SETTINGS_BASE* cfg = m_frame->config() )
        m_adapter->OpenLibs( cfg->m_LibTree.open_libs );

    // Bind this now se we don't spam the event queue with EVT_LIBITEM_SELECTED events during
    // the initial load.
    Bind( EVT_LIBITEM_SELECTED, &PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockSelected, this );
}


void PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockSelected( wxCommandEvent& aEvent )
{
    if( GetSelectedLibId().IsValid() )
    {
        std::unique_ptr<DESIGN_BLOCK> designBlock( m_parent->GetDesignBlock( GetSelectedLibId(), true, true ) );
        m_preview->DisplayDesignBlock( designBlock.get() );
    }
}


void PANEL_DESIGN_BLOCK_CHOOSER::onDesignBlockChosen( wxCommandEvent& aEvent )
{
    if( m_tree->GetSelectedLibId().IsValid() )
    {
        // Got a selection. We can't just end the modal dialog here, because wx leaks some events
        // back to the parent window (in particular, the MouseUp following a double click).
        //
        // NOW, here's where it gets really fun. wxTreeListCtrl eats MouseUp.  This isn't really
        // feasible to bypass without a fully custom wxDataViewCtrl implementation, and even then
        // might not be fully possible (docs are vague). To get around this, we use a one-shot
        // timer to schedule the dialog close.
        //
        // See PANEL_DESIGN_BLOCK_CHOOSER::onCloseTimer for the other end of this spaghetti noodle.
        m_dbl_click_timer->StartOnce( PANEL_DESIGN_BLOCK_CHOOSER::DBLCLICK_DELAY );
    }
}

void PANEL_DESIGN_BLOCK_CHOOSER::addDesignBlockToHistory( const LIB_ID& aLibId )
{
    LIB_ID savedId = GetSelectedLibId();

    m_tree->Unselect();

    // Remove duplicates
    for( int i = (int) m_historyList.size() - 1; i >= 0; --i )
    {
        if( m_historyList[i] == aLibId )
            m_historyList.erase( m_historyList.begin() + i );
    }

    // Add the new name at the beginning of the history list
    m_historyList.insert( m_historyList.begin(), aLibId );

    // Remove extra names
    while( m_historyList.size() >= 8 )
        m_historyList.pop_back();

    rebuildHistoryNode();
    m_tree->Regenerate( true );

    SelectLibId( savedId );
}


void PANEL_DESIGN_BLOCK_CHOOSER::rebuildHistoryNode()
{
    m_adapter->RemoveGroup( true, false );

    // Build the history list
    std::vector<LIB_TREE_ITEM*> historyInfos;

    DESIGN_BLOCK_LIBRARY_ADAPTER* adapter = m_frame->Prj().DesignBlockLibs();

    for( const LIB_ID& lib : m_historyList )
    {
        LIB_TREE_ITEM* info = adapter->LoadDesignBlock( lib.GetLibNickname(), lib.GetLibItemName() );

        // this can be null, for example, if the design block has been deleted from a library.
        if( info != nullptr )
            historyInfos.push_back( info );
    }

    m_adapter->DoAddLibrary( wxT( "-- " ) + _( "Recently Used" ) + wxT( " --" ), wxEmptyString,
                             historyInfos, false, true )
            .m_IsRecentlyUsedGroup = true;
}


void PANEL_DESIGN_BLOCK_CHOOSER::displayErrors( wxTopLevelWindow* aWindow )
{
    // @todo: go to a more HTML !<table>! ? centric output, possibly with recommendations
    // for remedy of errors.  Add numeric error codes to PARSE_ERROR, and switch on them for
    // remedies, etc.  Full access is provided to everything in every exception!

    HTML_MESSAGE_BOX dlg( aWindow, _( "Load Error" ) );

    dlg.MessageSet( _( "Errors were encountered loading design blocks:" ) );

    wxString msg;
    // TODO(JE) library tables - this function isn't even called, but would need fixup if so
#if 0
    while( std::unique_ptr<IO_ERROR> error = DESIGN_BLOCK_LIB_TABLE::GetGlobalList().PopError() )
    {
        wxString tmp = EscapeHTML( error->Problem() );

        // Preserve new lines in error messages so queued errors don't run together.
        tmp.Replace( wxS( "\n" ), wxS( "<BR>" ) );
        msg += wxT( "<p>" ) + tmp + wxT( "</p>" );
    }
#endif
    dlg.AddHTML_Text( msg );

    dlg.ShowModal();
}
