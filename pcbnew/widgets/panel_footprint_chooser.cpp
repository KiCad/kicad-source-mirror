/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2023 CERN
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

#include <widgets/panel_footprint_chooser.h>
#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/log.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/timer.h>
#include <wx/wxhtml.h>
#include <pcb_base_frame.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <fp_lib_table.h>
#include <settings/settings_manager.h>
#include <widgets/lib_tree.h>
#include <widgets/footprint_preview_widget.h>
#include <widgets/wx_progress_reporters.h>
#include <footprint_info_impl.h>
#include <project_pcb.h>
#include <kiface_base.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>

// When a new footprint is selected, a custom event is sent, for instance to update
// 3D viewer. So define a FP_SELECTION_EVENT event
wxDEFINE_EVENT(FP_SELECTION_EVENT, wxCommandEvent);

PANEL_FOOTPRINT_CHOOSER::PANEL_FOOTPRINT_CHOOSER( PCB_BASE_FRAME* aFrame, wxTopLevelWindow* aParent,
                                                  const wxArrayString& aFootprintHistoryList,
                                                  std::function<bool( LIB_TREE_NODE& )> aFilter,
                                                  std::function<void()> aAcceptHandler,
                                                  std::function<void()> aEscapeHandler ) :
        wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize ),
        m_hsplitter( nullptr ),
        m_vsplitter( nullptr ),
        m_frame( aFrame ),
        m_filter( std::move( aFilter ) ),
        m_acceptHandler( std::move( aAcceptHandler ) ),
        m_escapeHandler( std::move( aEscapeHandler ) )
{
    m_CurrFootprint = nullptr;
    FP_LIB_TABLE*   fpTable = PROJECT_PCB::PcbFootprintLibs( &aFrame->Prj() );

    // Load footprint files:
    auto* progressReporter = new WX_PROGRESS_REPORTER( aParent, _( "Load Footprint Libraries" ), 1,
                                                       PR_CAN_ABORT );
    GFootprintList.ReadFootprintFiles( fpTable, nullptr, progressReporter );

    // Force immediate deletion of the WX_PROGRESS_REPORTER.  Do not use Destroy(), or use
    // Destroy() followed by wxSafeYield() because on Windows, APP_PROGRESS_DIALOG and
    // WX_PROGRESS_REPORTER have some side effects on the event loop manager.  For instance, a
    // subsequent call to ShowModal() or ShowQuasiModal() for a dialog following the use of a
    // WX_PROGRESS_REPORTER results in incorrect modal or quasi modal behavior.
    delete progressReporter;

    if( GFootprintList.GetErrorCount() )
        GFootprintList.DisplayErrors( aParent );

    m_adapter = FP_TREE_MODEL_ADAPTER::Create( aFrame, fpTable );
    FP_TREE_MODEL_ADAPTER* adapter = static_cast<FP_TREE_MODEL_ADAPTER*>( m_adapter.get() );

    std::vector<LIB_TREE_ITEM*> historyInfos;

    for( const wxString& item : aFootprintHistoryList )
    {
        LIB_TREE_ITEM* fp_info = GFootprintList.GetFootprintInfo( item );

        // this can be null, for example, if the footprint has been deleted from a library.
        if( fp_info != nullptr )
            historyInfos.push_back( fp_info );
    }

    adapter->DoAddLibrary( wxT( "-- " ) + _( "Recently Used" ) + wxT( " --" ), wxEmptyString,
                           historyInfos, false, true )
            .m_IsRecentlyUsedGroup = true;

    if( historyInfos.size() )
        adapter->SetPreselectNode( historyInfos[0]->GetLIB_ID(), 0 );

    adapter->SetFilter( &m_filter );
    adapter->AddLibraries( m_frame );

    // -------------------------------------------------------------------------------------
    // Construct the actual panel
    //

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_vsplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxSP_LIVE_UPDATE | wxSP_NOBORDER | wxSP_3DSASH );

    m_hsplitter = new wxSplitterWindow( m_vsplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxSP_LIVE_UPDATE | wxSP_NOBORDER | wxSP_3DSASH );

    //Avoid the splitter window being assigned as the Parent to additional windows
    m_vsplitter->SetExtraStyle( wxWS_EX_TRANSIENT );
    m_hsplitter->SetExtraStyle( wxWS_EX_TRANSIENT );

    m_detailsPanel = new wxPanel( m_vsplitter );
    auto detailsSizer = new wxBoxSizer( wxVERTICAL );
    m_detailsPanel->SetSizer( detailsSizer );

    m_details = new HTML_WINDOW( m_detailsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize );
    detailsSizer->Add( m_details, 1, wxEXPAND, 5 );
    m_detailsPanel->Layout();
    detailsSizer->Fit( m_detailsPanel );

    m_vsplitter->SetSashGravity( 0.5 );
    // Ensure splitted areas are always shown (i.e. 0 size not allowed) when m_detailsPanel is shown
    m_vsplitter->SetMinimumPaneSize( 80 );  // arbitrary value but reasonable min size
    m_vsplitter->SplitHorizontally( m_hsplitter, m_detailsPanel );

    sizer->Add( m_vsplitter, 1, wxEXPAND, 5 );

    m_tree = new LIB_TREE( m_hsplitter, wxT( "footprints" ), m_adapter,
                           LIB_TREE::FLAGS::ALL_WIDGETS, m_details );

    m_hsplitter->SetSashGravity( 0.8 );
    m_hsplitter->SetMinimumPaneSize( 20 );

    m_RightPanel = new wxPanel( m_hsplitter );
    m_RightPanelSizer = new wxBoxSizer( wxVERTICAL );

    m_preview_ctrl = new FOOTPRINT_PREVIEW_WIDGET( m_RightPanel, m_frame->Kiway() );
    m_preview_ctrl->SetUserUnits( m_frame->GetUserUnits() );
    m_RightPanelSizer->Add( m_preview_ctrl, 1, wxEXPAND, 5 );

    m_RightPanel->SetSizer( m_RightPanelSizer );
    m_RightPanel->Layout();
    m_RightPanelSizer->Fit( m_RightPanel );

    m_hsplitter->SplitVertically( m_tree, m_RightPanel );

    m_dbl_click_timer = new wxTimer( this );
    m_open_libs_timer = new wxTimer( this );

    SetSizer( sizer );

    m_adapter->FinishTreeInitialization();

    Bind( wxEVT_TIMER, &PANEL_FOOTPRINT_CHOOSER::onCloseTimer, this, m_dbl_click_timer->GetId() );
    Bind( wxEVT_TIMER, &PANEL_FOOTPRINT_CHOOSER::onOpenLibsTimer, this, m_open_libs_timer->GetId() );
    Bind( EVT_LIBITEM_SELECTED, &PANEL_FOOTPRINT_CHOOSER::onFootprintSelected, this );
    Bind( EVT_LIBITEM_CHOSEN, &PANEL_FOOTPRINT_CHOOSER::onFootprintChosen, this );
    m_frame->Bind( wxEVT_MENU_OPEN, &PANEL_FOOTPRINT_CHOOSER::onMenuOpen, this );
    m_frame->Bind( wxEVT_MENU_CLOSE, &PANEL_FOOTPRINT_CHOOSER::onMenuClose, this );

    m_details->Connect( wxEVT_CHAR_HOOK,
                        wxKeyEventHandler( PANEL_FOOTPRINT_CHOOSER::OnDetailsCharHook ),
                        nullptr, this );

    Bind( wxEVT_CHAR_HOOK,
            [&]( wxKeyEvent& aEvent )
            {
                if( aEvent.GetKeyCode() == WXK_ESCAPE )
                {
                    wxObject* eventSource = aEvent.GetEventObject();

                    if( wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( eventSource ) )
                    {
                        // First escape cancels search string value
                        if( textCtrl->GetValue() == m_tree->GetSearchString()
                                && !m_tree->GetSearchString().IsEmpty() )
                        {
                            m_tree->SetSearchString( wxEmptyString );
                            return;
                        }
                    }

                    m_escapeHandler();
                }
                else
                {
                    // aEvent.Skip() should be sufficient to allow the normal key events to be
                    // generated (at least according to the wxWidgets documentation).  And yet,
                    // here we are.
                    aEvent.DoAllowNextEvent();

                    aEvent.Skip();
                }
            } );

    Layout();

    // Open the user's previously opened libraries on timer expiration.
    // This is done on a timer because we need a gross hack to keep GTK from garbling the
    // display. Must be longer than the search debounce timer.
    m_open_libs_timer->StartOnce( 300 );
}


PANEL_FOOTPRINT_CHOOSER::~PANEL_FOOTPRINT_CHOOSER()
{
    m_frame->Unbind( wxEVT_MENU_OPEN, &PANEL_FOOTPRINT_CHOOSER::onMenuOpen, this );
    m_frame->Unbind( wxEVT_MENU_CLOSE, &PANEL_FOOTPRINT_CHOOSER::onMenuClose, this );
    Unbind( wxEVT_TIMER, &PANEL_FOOTPRINT_CHOOSER::onCloseTimer, this );
    Unbind( EVT_LIBITEM_SELECTED, &PANEL_FOOTPRINT_CHOOSER::onFootprintSelected, this );
    Unbind( EVT_LIBITEM_CHOSEN, &PANEL_FOOTPRINT_CHOOSER::onFootprintChosen, this );

    m_details->Disconnect( wxEVT_CHAR_HOOK, wxKeyEventHandler( PANEL_FOOTPRINT_CHOOSER::OnDetailsCharHook ),
                           nullptr, this );

    // I am not sure the following two lines are necessary, but they will not hurt anyone
    m_dbl_click_timer->Stop();
    m_open_libs_timer->Stop();
    delete m_dbl_click_timer;
    delete m_open_libs_timer;

    if( PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
    {
        // Save any changes to column widths, etc.
        m_adapter->SaveSettings();

        cfg->m_FootprintChooser.width = GetParent()->GetSize().x;
        cfg->m_FootprintChooser.height = GetParent()->GetSize().y;
        cfg->m_FootprintChooser.sash_h = m_hsplitter->GetSashPosition();

        if( m_vsplitter )
            cfg->m_FootprintChooser.sash_v = m_vsplitter->GetSashPosition();

        cfg->m_FootprintChooser.sort_mode = m_tree->GetSortMode();
    }
}


void PANEL_FOOTPRINT_CHOOSER::onMenuOpen( wxMenuEvent& aEvent )
{
    m_tree->BlockPreview( true );
    aEvent.Skip();
}


void PANEL_FOOTPRINT_CHOOSER::onMenuClose( wxMenuEvent& aEvent )
{
    m_tree->BlockPreview( false );
    aEvent.Skip();
}


void PANEL_FOOTPRINT_CHOOSER::FinishSetup()
{
    if( PCBNEW_SETTINGS* settings = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
    {
        auto horizPixelsFromDU =
                [&]( int x ) -> int
                {
                    wxSize sz( x, 0 );
                    return GetParent()->ConvertDialogToPixels( sz ).x;
                };

        PCBNEW_SETTINGS::FOOTPRINT_CHOOSER& cfg = settings->m_FootprintChooser;

        int w = cfg.width < 40 ? horizPixelsFromDU( 440 ) : cfg.width;
        int h = cfg.height < 40 ? horizPixelsFromDU( 340 ) : cfg.height;

        GetParent()->SetSize( wxSize( w, h ) );
        GetParent()->Layout();

        // We specify the width of the right window (m_symbol_view_panel), because specify
        // the width of the left window does not work as expected when SetSashGravity() is called
        if( cfg.sash_h < 0 )
            cfg.sash_h = horizPixelsFromDU( 220 );

        m_hsplitter->SetSashPosition( cfg.sash_h );

        if( cfg.sash_v < 0 )
            cfg.sash_v = horizPixelsFromDU( 230 );

        if( m_vsplitter )
            m_vsplitter->SetSashPosition( cfg.sash_v );

        m_adapter->SetSortMode( (LIB_TREE_MODEL_ADAPTER::SORT_MODE) cfg.sort_mode );
    }
}


void PANEL_FOOTPRINT_CHOOSER::SetPreselect( const LIB_ID& aPreselect )
{
    m_adapter->SetPreselectNode( aPreselect, 0 );
}


LIB_ID PANEL_FOOTPRINT_CHOOSER::GetSelectedLibId() const
{
    return m_tree->GetSelectedLibId();
}


void PANEL_FOOTPRINT_CHOOSER::onCloseTimer( wxTimerEvent& aEvent )
{
    // Hack because of eaten MouseUp event. See PANEL_FOOTPRINT_CHOOSER::onFootprintChosen
    // for the beginning of this spaghetti noodle.

    auto state = wxGetMouseState();

    if( state.LeftIsDown() )
    {
        // Mouse hasn't been raised yet, so fire the timer again. Otherwise the
        // purpose of this timer is defeated.
        m_dbl_click_timer->StartOnce( PANEL_FOOTPRINT_CHOOSER::DblClickDelay );
    }
    else
    {
        m_acceptHandler();
    }
}

#include <footprint_preview_panel.h>
void PANEL_FOOTPRINT_CHOOSER::onOpenLibsTimer( wxTimerEvent& aEvent )
{
    if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( Kiface().KifaceSettings() ) )
        m_adapter->OpenLibs( cfg->m_LibTree.open_libs );
}


void PANEL_FOOTPRINT_CHOOSER::onFootprintSelected( wxCommandEvent& aEvent )
{
    if( !m_preview_ctrl || !m_preview_ctrl->IsInitialized() )
        return;

    LIB_ID lib_id = m_tree->GetSelectedLibId();

    if( !lib_id.IsValid() )
    {
        m_preview_ctrl->SetStatusText( _( "No footprint selected" ) );
    }
    else
    {
        m_preview_ctrl->ClearStatus();
        m_preview_ctrl->DisplayFootprint( lib_id );
    }

    m_CurrFootprint = static_cast<FOOTPRINT_PREVIEW_PANEL*>(m_preview_ctrl->GetPreviewPanel())->GetCurrentFootprint();

    // Send a FP_SELECTION_EVENT event after a footprint change
    wxCommandEvent event( FP_SELECTION_EVENT, GetId() );
    event.SetEventObject( this );

    ProcessWindowEvent( event );
}


void PANEL_FOOTPRINT_CHOOSER::onFootprintChosen( wxCommandEvent& aEvent )
{
    if( m_tree->GetSelectedLibId().IsValid() )
    {
        // Got a selection. We can't just end the modal dialog here, because wx leaks some
        // events back to the parent window (in particular, the MouseUp following a double click).
        //
        // NOW, here's where it gets really fun. wxTreeListCtrl eats MouseUp.  This isn't really
        // feasible to bypass without a fully custom wxDataViewCtrl implementation, and even then
        // might not be fully possible (docs are vague). To get around this, we use a one-shot
        // timer to schedule the dialog close.
        //
        // See PANEL_FOOTPRINT_CHOOSER::onCloseTimer for the other end of this spaghetti noodle.
        m_dbl_click_timer->StartOnce( PANEL_FOOTPRINT_CHOOSER::DblClickDelay );
    }
}


void PANEL_FOOTPRINT_CHOOSER::OnDetailsCharHook( wxKeyEvent& e )
{
    if( m_details && e.GetKeyCode() == 'C' && e.ControlDown() &&
        !e.AltDown() && !e.ShiftDown() && !e.MetaDown() )
    {
        wxString txt = m_details->SelectionToText();
        wxLogNull doNotLog; // disable logging of failed clipboard actions

        if( wxTheClipboard->Open() )
        {
            wxTheClipboard->SetData( new wxTextDataObject( txt ) );
            wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
            wxTheClipboard->Close();
        }
    }
    else
    {
        e.Skip();
    }
}
