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

#include "panel_symbol_chooser.h"

#include <pgm_base.h>
#include <kiface_base.h>
#include <sch_base_frame.h>
#include <project_sch.h>
#include <libraries/symbol_library_adapter.h>
#include <widgets/lib_tree.h>
#include <widgets/symbol_preview_widget.h>
#include <widgets/footprint_preview_widget.h>
#include <widgets/footprint_select_widget.h>
#include <settings/settings_manager.h>
#include <project/project_file.h>
#include <eeschema_settings.h>
#include <symbol_editor_settings.h>
#include <symbol_library_common.h>         // For SYMBOL_LIBRARY_FILTER
#include <algorithm>
#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/timer.h>
#include <wx/wxhtml.h>
#include <wx/log.h>


wxString PANEL_SYMBOL_CHOOSER::g_symbolSearchString;
wxString PANEL_SYMBOL_CHOOSER::g_powerSearchString;
SCH_BASE_FRAME* PANEL_SYMBOL_CHOOSER::m_frame = nullptr;

PANEL_SYMBOL_CHOOSER::PANEL_SYMBOL_CHOOSER( SCH_BASE_FRAME* aFrame, wxWindow* aParent,
                                            const SYMBOL_LIBRARY_FILTER* aFilter,
                                            std::vector<PICKED_SYMBOL>&  aHistoryList,
                                            std::vector<PICKED_SYMBOL>&  aAlreadyPlaced,
                                            bool aAllowFieldEdits, bool aShowFootprints, bool& aCancelled,
                                            std::function<void()> aAcceptHandler,
                                            std::function<void()> aEscapeHandler ) :
        wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize ),
        m_symbol_preview( nullptr ),
        m_hsplitter( nullptr ),
        m_vsplitter( nullptr ),
        m_fp_sel_ctrl( nullptr ),
        m_fp_preview( nullptr ),
        m_tree( nullptr ),
        m_details( nullptr ),
        m_acceptHandler( std::move( aAcceptHandler ) ),
        m_escapeHandler( std::move( aEscapeHandler ) ),
        m_showPower( false ),
        m_allow_field_edits( aAllowFieldEdits ),
        m_show_footprints( aShowFootprints )
{
    m_frame = aFrame;

    SYMBOL_LIBRARY_ADAPTER* libmgr = PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() );
    COMMON_SETTINGS::SESSION& session = Pgm().GetCommonSettings()->m_Session;
    PROJECT_FILE&             project = m_frame->Prj().GetProjectFile();

    // Make sure settings are loaded before we start running multi-threaded symbol loaders
    GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );
    GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" );

    m_adapter = SYMBOL_TREE_MODEL_ADAPTER::Create( m_frame, libmgr );
    SYMBOL_TREE_MODEL_ADAPTER* adapter = static_cast<SYMBOL_TREE_MODEL_ADAPTER*>( m_adapter.get() );

    if( aFilter )
    {
        const wxArrayString& liblist = aFilter->GetAllowedLibList();

        for( const wxString& nickname : liblist )
        {
            if( libmgr->HasLibrary( nickname, true ) )
            {
                bool pinned = alg::contains( session.pinned_symbol_libs, nickname )
                                || alg::contains( project.m_PinnedSymbolLibs, nickname );

                if( auto row = libmgr->GetRow( nickname ); row && !( *row )->Hidden()  )
                    adapter->AddLibrary( nickname, pinned );
            }
        }

        adapter->AssignIntrinsicRanks();

        if( aFilter->GetFilterPowerSymbols() )
        {
            static std::function<bool( LIB_TREE_NODE& )> powerFilter =
                    []( LIB_TREE_NODE& aNode ) -> bool
                    {
                        if (PANEL_SYMBOL_CHOOSER::m_frame)
                        {
                            LIB_SYMBOL* symbol = PANEL_SYMBOL_CHOOSER::m_frame->GetLibSymbol(aNode.m_LibId);

                            if (symbol && symbol->IsPower())
                                return true;

                        }

                        return false;
                    };

            adapter->SetFilter( &powerFilter );

            m_showPower = true;
            m_show_footprints = false;
        }
    }

    std::vector<LIB_SYMBOL>     history_list_storage;
    std::vector<LIB_TREE_ITEM*> history_list;
    std::vector<LIB_SYMBOL>     already_placed_storage;
    std::vector<LIB_TREE_ITEM*> already_placed;

    // Lambda to encapsulate the common logic
    auto processList =
            [&]( const std::vector<PICKED_SYMBOL>& inputList,
                 std::vector<LIB_SYMBOL>&          storageList,
                 std::vector<LIB_TREE_ITEM*>&      resultList )
            {
                storageList.reserve( inputList.size() );

                for( const PICKED_SYMBOL& i : inputList )
                {
                    LIB_SYMBOL* symbol = m_frame->GetLibSymbol( i.LibId );

                    if( symbol )
                    {
                        storageList.emplace_back( *symbol );

                        for( const auto& [fieldType, fieldValue] : i.Fields )
                        {
                            SCH_FIELD* field = storageList.back().GetField( fieldType );

                            if( field )
                                field->SetText( fieldValue );
                        }

                        resultList.push_back( &storageList.back() );
                    }
                }
            };

    // Sort the already placed list since it is potentially from multiple sessions,
    // but not the most recent list since we want this listed by most recent usage.
    std::sort( aAlreadyPlaced.begin(), aAlreadyPlaced.end(),
               []( PICKED_SYMBOL const& a, PICKED_SYMBOL const& b )
               {
                   return a.LibId.GetLibItemName() < b.LibId.GetLibItemName();
               } );

    processList( aHistoryList, history_list_storage, history_list );
    processList( aAlreadyPlaced, already_placed_storage, already_placed );

    adapter->DoAddLibrary( wxT( "-- " ) + _( "Recently Used" ) + wxT( " --" ), wxEmptyString,
                           history_list, false, true )
            .m_IsRecentlyUsedGroup = true;

    if( !aHistoryList.empty() )
        adapter->SetPreselectNode( aHistoryList[0].LibId, aHistoryList[0].Unit );

    adapter->DoAddLibrary( wxT( "-- " ) + _( "Already Placed" ) + wxT( " --" ), wxEmptyString,
                           already_placed, false, true )
            .m_IsAlreadyPlacedGroup = true;

    adapter->AddLibraries( m_frame );

    // -------------------------------------------------------------------------------------
    // Construct the actual panel
    //

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    // Use a slightly different layout, with a details pane spanning the entire window,
    // if we're not showing footprints.
    if( m_show_footprints )
    {
        m_hsplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                            wxSP_LIVE_UPDATE | wxSP_NOBORDER | wxSP_3DSASH );

        //Avoid the splitter window being assigned as the Parent to additional windows
        m_hsplitter->SetExtraStyle( wxWS_EX_TRANSIENT );

        sizer->Add( m_hsplitter, 1, wxEXPAND, 5 );
    }
    else
    {
        m_vsplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                            wxSP_LIVE_UPDATE | wxSP_NOBORDER | wxSP_3DSASH );

        m_hsplitter = new wxSplitterWindow( m_vsplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                            wxSP_LIVE_UPDATE | wxSP_NOBORDER | wxSP_3DSASH );

        // Avoid the splitter window being assigned as the parent to additional windows.
        m_vsplitter->SetExtraStyle( wxWS_EX_TRANSIENT );
        m_hsplitter->SetExtraStyle( wxWS_EX_TRANSIENT );

        wxPanel*    detailsPanel = new wxPanel( m_vsplitter );
        wxBoxSizer* detailsSizer = new wxBoxSizer( wxVERTICAL );
        detailsPanel->SetSizer( detailsSizer );

        m_details = new HTML_WINDOW( detailsPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize  );
        detailsSizer->Add( m_details, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
        detailsPanel->Layout();
        detailsSizer->Fit( detailsPanel );

        m_vsplitter->SetSashGravity( 0.5 );
        m_vsplitter->SetMinimumPaneSize( 20 );
        m_vsplitter->SplitHorizontally( m_hsplitter, detailsPanel );

        sizer->Add( m_vsplitter, 1, wxEXPAND | wxBOTTOM, 5 );
    }

    wxPanel*    treePanel = new wxPanel( m_hsplitter );
    wxBoxSizer* treeSizer = new wxBoxSizer( wxVERTICAL );
    treePanel->SetSizer( treeSizer );

    m_tree = new LIB_TREE( treePanel, m_showPower ? wxT( "power" ) : wxT( "symbols" ), m_adapter,
                           LIB_TREE::FLAGS::ALL_WIDGETS, m_details );

    treeSizer->Add( m_tree, 1, wxALL | wxEXPAND, 5 );
    treePanel->Layout();
    treeSizer->Fit( treePanel );

    m_adapter->FinishTreeInitialization();

    if( m_showPower )
        m_tree->SetSearchString( g_powerSearchString );
    else
        m_tree->SetSearchString( g_symbolSearchString );

    m_hsplitter->SetSashGravity( 0.8 );
    m_hsplitter->SetMinimumPaneSize( 20 );
    m_hsplitter->SplitVertically( treePanel, constructRightPanel( m_hsplitter ) );

    m_dbl_click_timer = new wxTimer( this );
    m_open_libs_timer = new wxTimer( this );

    SetSizer( sizer );

    Layout();

    Bind( wxEVT_TIMER, &PANEL_SYMBOL_CHOOSER::onCloseTimer, this, m_dbl_click_timer->GetId() );
    Bind( wxEVT_TIMER, &PANEL_SYMBOL_CHOOSER::onOpenLibsTimer, this, m_open_libs_timer->GetId() );
    Bind( EVT_LIBITEM_SELECTED, &PANEL_SYMBOL_CHOOSER::onSymbolSelected, this );
    Bind( EVT_LIBITEM_CHOSEN, &PANEL_SYMBOL_CHOOSER::onSymbolChosen, this );
    aFrame->Bind( wxEVT_MENU_OPEN, &PANEL_SYMBOL_CHOOSER::onMenuOpen, this );
    aFrame->Bind( wxEVT_MENU_CLOSE, &PANEL_SYMBOL_CHOOSER::onMenuClose, this );

    if( m_fp_sel_ctrl )
        m_fp_sel_ctrl->Bind( EVT_FOOTPRINT_SELECTED, &PANEL_SYMBOL_CHOOSER::onFootprintSelected, this );

    if( m_details )
        m_details->Bind( wxEVT_CHAR_HOOK, &PANEL_SYMBOL_CHOOSER::OnDetailsCharHook, this );

    // Open the user's previously opened libraries on timer expiration.
    // This is done on a timer because we need a gross hack to keep GTK from garbling the
    // display. Must be longer than the search debounce timer.
    m_open_libs_timer->StartOnce( 300 );
}


PANEL_SYMBOL_CHOOSER::~PANEL_SYMBOL_CHOOSER()
{
    m_frame->Unbind( wxEVT_MENU_OPEN, &PANEL_SYMBOL_CHOOSER::onMenuOpen, this );
    m_frame->Unbind( wxEVT_MENU_CLOSE, &PANEL_SYMBOL_CHOOSER::onMenuClose, this );
    Unbind( wxEVT_TIMER, &PANEL_SYMBOL_CHOOSER::onCloseTimer, this );
    Unbind( EVT_LIBITEM_SELECTED, &PANEL_SYMBOL_CHOOSER::onSymbolSelected, this );
    Unbind( EVT_LIBITEM_CHOSEN, &PANEL_SYMBOL_CHOOSER::onSymbolChosen, this );

    // Stop the timer during destruction early to avoid potential race conditions (that do happen)
    m_dbl_click_timer->Stop();
    m_open_libs_timer->Stop();
    delete m_dbl_click_timer;
    delete m_open_libs_timer;

    if( m_showPower )
        g_powerSearchString = m_tree->GetSearchString();
    else
        g_symbolSearchString = m_tree->GetSearchString();

    if( m_fp_sel_ctrl )
        m_fp_sel_ctrl->Unbind( EVT_FOOTPRINT_SELECTED, &PANEL_SYMBOL_CHOOSER::onFootprintSelected, this );

    if( m_details )
        m_details->Unbind( wxEVT_CHAR_HOOK, &PANEL_SYMBOL_CHOOSER::OnDetailsCharHook, this );

    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        // Save any changes to column widths, etc.
        m_adapter->SaveSettings();

        cfg->m_SymChooserPanel.width = GetParent()->GetSize().x;
        cfg->m_SymChooserPanel.height = GetParent()->GetSize().y;

        cfg->m_SymChooserPanel.sash_pos_h = m_hsplitter->GetSashPosition();

        if( m_vsplitter )
            cfg->m_SymChooserPanel.sash_pos_v = m_vsplitter->GetSashPosition();

        cfg->m_SymChooserPanel.sort_mode = m_tree->GetSortMode();
    }

    m_frame = nullptr;
}


void PANEL_SYMBOL_CHOOSER::onMenuOpen( wxMenuEvent& aEvent )
{
    m_tree->BlockPreview( true );
    aEvent.Skip();
}


void PANEL_SYMBOL_CHOOSER::onMenuClose( wxMenuEvent& aEvent )
{
    m_tree->BlockPreview( false );
    aEvent.Skip();
}


void PANEL_SYMBOL_CHOOSER::OnChar( wxKeyEvent& aEvent )
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
        aEvent.Skip();
    }
}


wxPanel* PANEL_SYMBOL_CHOOSER::constructRightPanel( wxWindow* aParent )
{
    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_TYPE::GAL_TYPE_OPENGL;

    if( m_frame->GetCanvas() )
        backend = m_frame->GetCanvas()->GetBackend();
    else if( COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
        backend = static_cast<EDA_DRAW_PANEL_GAL::GAL_TYPE>( cfg->m_Graphics.canvas_type );

    wxPanel*    panel = new wxPanel( aParent );
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_symbol_preview = new SYMBOL_PREVIEW_WIDGET( panel, &m_frame->Kiway(), true, backend );
    m_symbol_preview->SetLayoutDirection( wxLayout_LeftToRight );

    if( m_show_footprints )
    {
        FOOTPRINT_LIST* fp_list = FOOTPRINT_LIST::GetInstance( m_frame->Kiway() );

        sizer->Add( m_symbol_preview, 11, wxEXPAND | wxALL, 5 );

        if ( fp_list )
        {
            if( m_allow_field_edits )
                m_fp_sel_ctrl = new FOOTPRINT_SELECT_WIDGET( m_frame, panel, fp_list, true );

            m_fp_preview = new FOOTPRINT_PREVIEW_WIDGET( panel, m_frame->Kiway() );
            m_fp_preview->SetUserUnits( m_frame->GetUserUnits() );
        }

        if( m_fp_sel_ctrl )
            sizer->Add( m_fp_sel_ctrl, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

        if( m_fp_preview )
            sizer->Add( m_fp_preview, 10, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
    }
    else
    {
        sizer->Add( m_symbol_preview, 1, wxEXPAND | wxALL, 5 );
    }

    panel->SetSizer( sizer );
    panel->Layout();
    sizer->Fit( panel );

    return panel;
}


void PANEL_SYMBOL_CHOOSER::FinishSetup()
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        auto horizPixelsFromDU =
                [&]( int x ) -> int
                {
                    wxSize sz( x, 0 );
                    return GetParent()->ConvertDialogToPixels( sz ).x;
                };

        EESCHEMA_SETTINGS::PANEL_SYM_CHOOSER& panelCfg = cfg->m_SymChooserPanel;

        int w = panelCfg.width > 40 ? panelCfg.width : horizPixelsFromDU( 440 );
        int h = panelCfg.height > 40 ? panelCfg.height : horizPixelsFromDU( 340 );

        GetParent()->SetSize( wxSize( w, h ) );
        GetParent()->Layout();

        // We specify the width of the right window (m_symbol_view_panel), because specify
        // the width of the left window does not work as expected when SetSashGravity() is called

        if( panelCfg.sash_pos_h < 0 )
            panelCfg.sash_pos_h = horizPixelsFromDU( 220 );

        if( panelCfg.sash_pos_v < 0 )
            panelCfg.sash_pos_v = horizPixelsFromDU( 230 );

        m_hsplitter->SetSashPosition( panelCfg.sash_pos_h );

        if( m_vsplitter )
            m_vsplitter->SetSashPosition( panelCfg.sash_pos_v );

        m_adapter->SetSortMode( (LIB_TREE_MODEL_ADAPTER::SORT_MODE) panelCfg.sort_mode );
    }

    if( m_fp_preview && m_fp_preview->IsInitialized() )
    {
        // This hides the GAL panel and shows the status label
        m_fp_preview->SetStatusText( wxEmptyString );
    }

    if( m_fp_sel_ctrl )
        m_fp_sel_ctrl->Load( m_frame->Kiway(), m_frame->Prj() );
}


void PANEL_SYMBOL_CHOOSER::OnDetailsCharHook( wxKeyEvent& e )
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


void PANEL_SYMBOL_CHOOSER::SetPreselect( const LIB_ID& aPreselect )
{
    m_adapter->SetPreselectNode( aPreselect, 0 );
}


LIB_ID PANEL_SYMBOL_CHOOSER::GetSelectedLibId( int* aUnit ) const
{
    return m_tree->GetSelectedLibId( aUnit );
}


void PANEL_SYMBOL_CHOOSER::ShutdownCanvases()
{
    m_symbol_preview->GetCanvas()->SetEvtHandlerEnabled( false );
    m_symbol_preview->GetCanvas()->StopDrawing();

    if( m_fp_preview )
    {
        m_fp_preview->GetPreviewPanel()->GetCanvas()->SetEvtHandlerEnabled( false );
        m_fp_preview->GetPreviewPanel()->GetCanvas()->StopDrawing();
    }
}


void PANEL_SYMBOL_CHOOSER::onCloseTimer( wxTimerEvent& aEvent )
{
    // Hack because of eaten MouseUp event. See PANEL_SYMBOL_CHOOSER::onSymbolChosen
    // for the beginning of this spaghetti noodle.

    wxMouseState state = wxGetMouseState();

    if( state.LeftIsDown() )
    {
        // Mouse hasn't been raised yet, so fire the timer again. Otherwise the
        // purpose of this timer is defeated.
        m_dbl_click_timer->StartOnce( PANEL_SYMBOL_CHOOSER::DBLCLICK_DELAY );
    }
    else
    {
        m_acceptHandler();
    }
}


void PANEL_SYMBOL_CHOOSER::onOpenLibsTimer( wxTimerEvent& aEvent )
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
        m_adapter->OpenLibs( cfg->m_LibTree.open_libs );
}


void PANEL_SYMBOL_CHOOSER::showFootprintFor( LIB_ID const& aLibId )
{
    if( !m_fp_preview || !m_fp_preview->IsInitialized() )
        return;

    LIB_SYMBOL* symbol = nullptr;

    try
    {
        symbol = PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() )->LoadSymbol( aLibId );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogError( _( "Error loading symbol %s from library '%s'." ) + wxS( "\n%s" ),
                    aLibId.GetLibItemName().wx_str(),
                    aLibId.GetLibNickname().wx_str(),
                    ioe.What() );
    }

    if( !symbol )
        return;

    SCH_FIELD* fp_field = symbol->GetField( FIELD_T::FOOTPRINT );
    wxString   fp_name = fp_field ? fp_field->GetFullText() : wxString( "" );

    showFootprint( fp_name );
}


void PANEL_SYMBOL_CHOOSER::showFootprint( wxString const& aFootprint )
{
    if( !m_fp_preview || !m_fp_preview->IsInitialized() )
        return;

    if( aFootprint == wxEmptyString )
    {
        m_fp_preview->SetStatusText( _( "No footprint specified" ) );
    }
    else
    {
        LIB_ID lib_id;

        if( lib_id.Parse( aFootprint ) == -1 && lib_id.IsValid() )
        {
            m_fp_preview->ClearStatus();
            m_fp_preview->DisplayFootprint( lib_id );
        }
        else
        {
            m_fp_preview->SetStatusText( _( "Invalid footprint specified" ) );
        }
    }
}


void PANEL_SYMBOL_CHOOSER::populateFootprintSelector( LIB_ID const& aLibId )
{
    if( !m_fp_sel_ctrl )
        return;

    m_fp_sel_ctrl->ClearFilters();

    LIB_SYMBOL* symbol = nullptr;

    if( aLibId.IsValid() )
    {
        try
        {
            symbol = PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() )->LoadSymbol( aLibId );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( _( "Error loading symbol %s from library '%s'." ) + wxS( "\n%s" ),
                        aLibId.GetLibItemName().wx_str(),
                        aLibId.GetLibNickname().wx_str(),
                        ioe.What() );
        }
    }

    if( symbol != nullptr )
    {
    int        pinCount = symbol->GetGraphicalPins( 0 /* all units */, 1 /* single bodyStyle */ ).size();
        SCH_FIELD* fp_field = symbol->GetField( FIELD_T::FOOTPRINT );
        wxString   fp_name = fp_field ? fp_field->GetFullText() : wxString( "" );

        m_fp_sel_ctrl->FilterByPinCount( pinCount );
        m_fp_sel_ctrl->FilterByFootprintFilters( symbol->GetFPFilters(), true );
        m_fp_sel_ctrl->SetDefaultFootprint( fp_name );
        m_fp_sel_ctrl->UpdateList();
        m_fp_sel_ctrl->Enable();
    }
    else
    {
        m_fp_sel_ctrl->UpdateList();
        m_fp_sel_ctrl->Disable();
    }
}


void PANEL_SYMBOL_CHOOSER::onFootprintSelected( wxCommandEvent& aEvent )
{
    m_fp_override = aEvent.GetString();

    std::erase_if( m_field_edits, []( std::pair<FIELD_T, wxString> const& i )
                                   {
                                       return i.first == FIELD_T::FOOTPRINT;
                                   } );

    m_field_edits.emplace_back( std::make_pair( FIELD_T::FOOTPRINT, m_fp_override ) );

    showFootprint( m_fp_override );
}


void PANEL_SYMBOL_CHOOSER::onSymbolSelected( wxCommandEvent& aEvent )
{
    LIB_TREE_NODE* node = m_tree->GetCurrentTreeNode();

    if( node && node->m_LibId.IsValid() )
    {
        m_symbol_preview->DisplaySymbol( node->m_LibId, node->m_Unit );

        if( !node->m_Footprint.IsEmpty() )
            showFootprint( node->m_Footprint );
        else
            showFootprintFor( node->m_LibId );

        populateFootprintSelector( node->m_LibId );
    }
    else
    {
        m_symbol_preview->SetStatusText( _( "No symbol selected" ) );

        if( m_fp_preview && m_fp_preview->IsInitialized() )
            m_fp_preview->SetStatusText( wxEmptyString );

        populateFootprintSelector( LIB_ID() );
    }
}


void PANEL_SYMBOL_CHOOSER::onSymbolChosen( wxCommandEvent& aEvent )
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
        // See PANEL_SYMBOL_CHOOSER::onCloseTimer for the other end of this spaghetti noodle.
        m_dbl_click_timer->StartOnce( PANEL_SYMBOL_CHOOSER::DBLCLICK_DELAY );
    }
}


void PANEL_SYMBOL_CHOOSER::Regenerate()
{
    LIB_ID savedSelection = m_tree->GetSelectedLibId();
    m_tree->Regenerate( true );

    if( savedSelection.IsValid() )
        m_tree->CenterLibId( savedSelection );
}
