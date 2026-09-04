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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "panel_symbol_chooser.h"

#include <class_draw_panel_gal.h>
#include <pgm_base.h>
#include <settings/common_settings.h>
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
#include <string_utils.h>
#include <symbol_library_common.h>         // For SYMBOL_LIBRARY_FILTER
#include <mdw/client/mdw_library_sources.h>
#include <mdw/client/mdw_lifecycle.h>
#include <mdw/client/mdw_source.h>
#include <algorithm>
#include <optional>
#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/display.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/timer.h>
#include <wx/wxhtml.h>
#include <wx/log.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <lib_symbol.h>


namespace
{

/// A parts lookup that does not answer by then is placed from its search result
constexpr int WORKSPACE_CONFIRM_TIMEOUT_MS = 3000;


wxString lifecycleText( const MDW_PART_INFO& aPart )
{
    wxString text = aPart.state.not_for_new_designs ? wxString( _( "Not for New Designs" ) )
                                                    : MdwCategoryName( aPart.state.category );

    if( !aPart.revision_label.IsEmpty() )
        text << wxS( " \u00B7 " ) << wxString::Format( _( "Rev %s" ), aPart.revision_label );

    return text;
}


const MDW_PART_MEMBER* findMember( const MDW_PART_INFO& aPart, MDW_MEMBER_KIND aKind )
{
    for( const MDW_PART_MEMBER& member : aPart.members )
    {
        if( member.kind == aKind )
            return &member;
    }

    return nullptr;
}


/// The footprint aPart names, or empty when it names none
wxString footprintHint( const MDW_PART_INFO& aPart )
{
    const MDW_PART_MEMBER* footprint = findMember( aPart, MDW_MEMBER_KIND::FOOTPRINT );

    if( !footprint )
        return wxString();

    std::optional<LIB_ID> id =
            MDW_LIBRARY_SOURCES::Get().LocalLibId( MDW_LIBRARY_TYPE::FOOTPRINT, footprint->member );

    return id ? wxString( id->Format().wx_str() ) : wxString();
}

} // namespace


wxString WORKSPACE_ROW_ITEM::GetFootprint() const
{
    if( m_row.resolution != WORKSPACE_PARTS_SEARCH::RESOLUTION::RESOLVED )
        return wxEmptyString;

    return footprintHint( m_row.part );
}


void WORKSPACE_ROW_ITEM::GetChooserFields( std::map<wxString, wxString>& aColumnMap )
{
    if( m_row.kind == WORKSPACE_PARTS_SEARCH::ROW_KIND::PART )
        aColumnMap[wxS( "Lifecycle" )] = lifecycleText( m_row.part );
}


wxString PANEL_SYMBOL_CHOOSER::g_symbolSearchString;
wxString PANEL_SYMBOL_CHOOSER::g_powerSearchString;

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
        m_frame( aFrame ),
        m_acceptHandler( std::move( aAcceptHandler ) ),
        m_escapeHandler( std::move( aEscapeHandler ) ),
        m_showPower( false ),
        m_allow_field_edits( aAllowFieldEdits ),
        m_show_footprints( aShowFootprints ),
        m_bodyStyleLabel( nullptr ),
        m_bodyStyleChoice( nullptr ),
        m_selectedBodyStyle( 1 ),
        m_confirmTimer( nullptr )
{
    SYMBOL_LIBRARY_ADAPTER*   libmgr = PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() );
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

                std::optional<LIBRARY_TABLE_ROW*> row = libmgr->GetRow( nickname );

                if( row.has_value() && !row.value()->Hidden()  )
                    adapter->AddLibrary( nickname, pinned );
            }
        }

        adapter->AssignIntrinsicRanks();

        if( aFilter->GetFilterPowerSymbols() )
        {
            static std::function<bool( LIB_TREE_NODE& )> powerFilter =
                    []( LIB_TREE_NODE& aNode ) -> bool
                    {
                        return aNode.m_IsPower;
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

    for( const PICKED_SYMBOL& picked : aHistoryList )
    {
        if( picked.PartIdentity )
            m_recentParts[picked.LibId.Format().wx_str()] = *picked.PartIdentity;
    }

    adapter->DoAddLibrary( wxT( "-- " ) + _( "Recently Used" ) + wxT( " --" ), wxEmptyString, history_list,
                           false, true )
            .m_IsRecentlyUsedGroup = true;

    if( !aHistoryList.empty() )
        adapter->SetPreselectNode( aHistoryList[0].LibId, aHistoryList[0].Unit );

    adapter->DoAddLibrary( wxT( "-- " ) + _( "Already Placed" ) + wxT( " --" ), wxEmptyString, already_placed,
                           false, true )
            .m_IsAlreadyPlacedGroup = true;

    if( !m_showPower )
    {
        std::vector<std::shared_ptr<MDW_SOURCE>> partSources = WORKSPACE_PARTS_SEARCH::PartSources();

        if( !partSources.empty() )
        {
            m_workspaceParts = std::make_unique<WORKSPACE_PARTS_SEARCH>(
                    std::move( partSources ),
                    [this]( const MDW_PART_INFO& aPart, LIB_ID& aSymbol )
                    {
                        return resolveWorkspacePart( aPart, aSymbol );
                    },
                    [this]( size_t aGroup )
                    {
                        refillWorkspaceGroup( aGroup );
                    } );

            for( const WORKSPACE_PARTS_SEARCH::GROUP& group : m_workspaceParts->Groups() )
            {
                LIB_TREE_NODE_LIBRARY& node = adapter->DoAddLibrary( group.name, wxEmptyString, {}, false, true );
                node.m_IsExternalResultsGroup = true;
                m_workspaceGroups.push_back( &node );
            }
        }
    }

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

    if( m_workspaceParts )
    {
        m_tree->SetQueryChangedHandler(
                [this]( const wxString& aQuery )
                {
                    m_workspaceParts->SetQuery( aQuery );
                } );
    }

    if( m_showPower )
        m_tree->SetSearchString( g_powerSearchString );
    else
        m_tree->SetSearchString( g_symbolSearchString );

    m_hsplitter->SetSashGravity( 0.8 );
    m_hsplitter->SetMinimumPaneSize( 20 );
    m_hsplitter->SplitVertically( treePanel, constructRightPanel( m_hsplitter ) );

    m_dbl_click_timer = new wxTimer( this );
    m_open_libs_timer = new wxTimer( this );
    m_confirmTimer = new wxTimer( this );

    SetSizer( sizer );

    Layout();

    Bind( wxEVT_TIMER, &PANEL_SYMBOL_CHOOSER::onCloseTimer, this, m_dbl_click_timer->GetId() );
    Bind( wxEVT_TIMER, &PANEL_SYMBOL_CHOOSER::onOpenLibsTimer, this, m_open_libs_timer->GetId() );

    Bind(
            wxEVT_TIMER,
            [this]( wxTimerEvent& )
            {
                m_confirmOp.Cancel();

                if( m_pickedPart )
                    acceptWorkspacePart( *m_pickedPart, m_pickedSymbol );
            },
            m_confirmTimer->GetId() );
    Bind( EVT_LIBITEM_SELECTED, &PANEL_SYMBOL_CHOOSER::onSymbolSelected, this );
    Bind( EVT_LIBITEM_CHOSEN, &PANEL_SYMBOL_CHOOSER::onSymbolChosen, this );
    aFrame->Bind( wxEVT_MENU_OPEN, &PANEL_SYMBOL_CHOOSER::onMenuOpen, this );
    aFrame->Bind( wxEVT_MENU_CLOSE, &PANEL_SYMBOL_CHOOSER::onMenuClose, this );

    if( m_fp_sel_ctrl )
        m_fp_sel_ctrl->Bind( EVT_FOOTPRINT_SELECTED, &PANEL_SYMBOL_CHOOSER::onFootprintSelected, this );

    if( m_bodyStyleChoice )
        m_bodyStyleChoice->Bind( wxEVT_CHOICE, &PANEL_SYMBOL_CHOOSER::onSelectBodyStyle, this );

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
    m_confirmTimer->Stop();
    m_confirmOp.Cancel();
    delete m_dbl_click_timer;
    delete m_open_libs_timer;
    delete m_confirmTimer;

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

        cfg->m_SymChooserPanel.width = GetParent()->ToDIP( GetParent()->GetSize().x );
        cfg->m_SymChooserPanel.height = GetParent()->ToDIP( GetParent()->GetSize().y );

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
            if( textCtrl->GetValue() == m_tree->GetSearchString() && !m_tree->GetSearchString().IsEmpty() )
            {
                m_tree->SetSearchString( wxEmptyString );
                Regenerate();
                GetFocusTarget()->SetFocus();
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
    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_TYPE::GAL_TYPE_MODERN;

    if( m_frame->GetCanvas() )
        backend = m_frame->GetCanvas()->GetBackend();
    else if( COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
        backend = EDA_DRAW_PANEL_GAL::ResolveStoredCanvasType( cfg->m_Graphics.canvas_type );

    wxPanel*    panel = new wxPanel( aParent );
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_symbol_preview = new SYMBOL_PREVIEW_WIDGET( panel, &m_frame->Kiway(), true, backend );
    m_symbol_preview->SetLayoutDirection( wxLayout_LeftToRight );

    wxBoxSizer* bodyStyleSizer = new wxBoxSizer( wxHORIZONTAL );
    m_bodyStyleLabel = new wxStaticText( panel, wxID_ANY, _( "Body style:" ) );
    m_bodyStyleChoice = new wxChoice( panel, wxID_ANY );
    m_bodyStyleChoice->Enable( false );
    bodyStyleSizer->Add( m_bodyStyleLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxTOP | wxBOTTOM, 5 );
    bodyStyleSizer->Add( m_bodyStyleChoice, 1, wxEXPAND | wxALL, 5 );
    sizer->Add( bodyStyleSizer, 0, wxEXPAND | wxBOTTOM, 5 );

    if( m_show_footprints )
    {
        sizer->Add( m_symbol_preview, 11, wxEXPAND | wxALL, 5 );

        m_fp_sel_ctrl = new FOOTPRINT_SELECT_WIDGET( m_frame, panel );
        sizer->Add( m_fp_sel_ctrl, 0, wxEXPAND | wxLEFT | wxRIGHT, 5 );

        m_fp_preview = new FOOTPRINT_PREVIEW_WIDGET( panel, m_frame->Kiway() );
        m_fp_preview->SetUserUnits( m_frame->GetUserUnits() );

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

        // The persisted size is stored in DIP so it is independent of the monitor it was saved
        // on. Restoring raw pixels would scale the window by the DPI ratio when reopened on a
        // different-scale display, producing a window that spills across monitors.
        int w = panelCfg.width > 40 ? GetParent()->FromDIP( panelCfg.width ) : horizPixelsFromDU( 440 );
        int h = panelCfg.height > 40 ? GetParent()->FromDIP( panelCfg.height ) : horizPixelsFromDU( 340 );

        // Cap to the work area so a stale pre-DIP setting cannot reopen the window across monitors.
        if( int display = wxDisplay::GetFromWindow( GetParent() ); display != wxNOT_FOUND )
        {
            wxRect workArea = wxDisplay( display ).GetClientArea();
            w = std::min( w, workArea.GetWidth() );
            h = std::min( h, workArea.GetHeight() );
        }

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


void PANEL_SYMBOL_CHOOSER::OnDetailsCharHook( wxKeyEvent& aEvent )
{
    if( m_details
        && aEvent.GetKeyCode() == 'C'
        && aEvent.ControlDown()
        && !aEvent.AltDown()
        && !aEvent.ShiftDown()
        && !aEvent.MetaDown() )
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
        aEvent.Skip();
    }
}


void PANEL_SYMBOL_CHOOSER::SetCompatibilityCallback( SYMBOL_COMPAT_FUNC aFunc )
{
    m_compatCallback = std::move( aFunc );

    if( SYMBOL_TREE_MODEL_ADAPTER* symAdapter = dynamic_cast<SYMBOL_TREE_MODEL_ADAPTER*>( m_adapter.get() ) )
        symAdapter->SetCompatibilityCallback( m_compatCallback );
}


void PANEL_SYMBOL_CHOOSER::SetPreselect( const LIB_ID& aPreselect )
{
    m_adapter->SetPreselectNode( aPreselect, 0 );

    if( m_tree && aPreselect.IsValid() )
        m_tree->SelectLibId( aPreselect );
}


LIB_ID PANEL_SYMBOL_CHOOSER::GetSelectedLibId( int* aUnit, int* aBodyStyle ) const
{
    LIB_ID id = m_tree->GetSelectedLibId( aUnit );

    // The lookup at accept time may have moved the part to another symbol
    if( m_pickedPart )
        id = m_pickedSymbol;

    if( aBodyStyle )
        *aBodyStyle = m_selectedBodyStyle;

    return id;
}


void PANEL_SYMBOL_CHOOSER::updateBodyStyleChoice( LIB_SYMBOL* aSymbol )
{
    int bodyStyleCount = aSymbol ? std::max( aSymbol->GetBodyStyleCount(), 1 ) : 1;

    m_bodyStyleChoice->Enable( bodyStyleCount > 1 );
    m_bodyStyleChoice->Clear();

    if( bodyStyleCount > 1 )
    {
        for( int i = 1; i <= bodyStyleCount; i++ )
            m_bodyStyleChoice->Append( aSymbol->GetBodyStyleDescription( i, false ) );

        if( m_selectedBodyStyle > static_cast<int>( m_bodyStyleChoice->GetCount() ) )
            m_selectedBodyStyle = 1;

        m_bodyStyleChoice->SetSelection( m_selectedBodyStyle - 1 );
    }
    else
    {
        m_selectedBodyStyle = 1;
    }
}


void PANEL_SYMBOL_CHOOSER::onSelectBodyStyle( wxCommandEvent& aEvent )
{
    int selection = m_bodyStyleChoice->GetSelection();

    if( selection < 0 )
        return;

    m_selectedBodyStyle = selection + 1;

    LIB_TREE_NODE* node = m_tree->GetCurrentTreeNode();

    if( node && node->m_LibId.IsValid() )
        m_symbol_preview->DisplaySymbol( node->m_LibId, node->m_Unit, m_selectedBodyStyle );
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
        AcceptSelection();
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

    m_fp_override.Empty();
    showFootprint( symbol->GetFootprint() );
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
        int      pinCount = symbol->GetGraphicalPins( 0 /* all units */, 1 /* single bodyStyle */ ).size();
        wxString fp_name = symbol->GetFootprint();

        if( !m_fp_override.IsEmpty() )
            fp_name = m_fp_override;

        // Explicitly associated footprints (issue #2282) are listed ahead of the glob matches in
        // written order and bypass the pin-count filter; a mapped EP/NC footprint legally has more
        // pads than the symbol has pins.
        for( const ASSOCIATED_FOOTPRINT& assoc : symbol->GetEffectiveAssociatedFootprints() )
            m_fp_sel_ctrl->AddAlwaysIncludedFootprint( assoc.m_FootprintLibId );

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

    std::erase_if( m_field_edits,
            []( std::pair<FIELD_T, wxString> const& i )
            {
                return i.first == FIELD_T::FOOTPRINT;
            } );

    m_field_edits.emplace_back( std::make_pair( FIELD_T::FOOTPRINT, m_fp_override ) );

    showFootprint( m_fp_override );
}


void PANEL_SYMBOL_CHOOSER::onSymbolSelected( wxCommandEvent& aEvent )
{
    LIB_TREE_NODE*                     node = m_tree->GetCurrentTreeNode();
    size_t                             group = 0;
    const WORKSPACE_PARTS_SEARCH::ROW* workspace = workspaceRow( node, &group );
    bool                               sameSymbolDifferentUnit = false;

    // A pending accept was for the previous selection
    if( m_confirmTimer && m_confirmTimer->IsRunning() )
    {
        m_confirmTimer->Stop();
        m_confirmOp.Cancel();
        m_pickedPart.reset();
    }

    if( workspace
        && ( workspace->kind == WORKSPACE_PARTS_SEARCH::ROW_KIND::MORE
             || workspace->kind == WORKSPACE_PARTS_SEARCH::ROW_KIND::RETRY ) )
    {
        // This event is posted, so refilling the group cannot pull the row out from under the tree
        m_workspaceParts->Activate( group, m_workspaceRows.at( node ).second );
        return;
    }

    MDW_ITEM_REF part;

    if( workspace && workspace->kind == WORKSPACE_PARTS_SEARCH::ROW_KIND::PART )
        part = workspace->part.part;

    // Two parts can share a symbol yet not a footprint
    bool otherPart = part.item_id != m_curPart.item_id || part.source != m_curPart.source;
    m_curPart = part;

    if( node && node->m_LibId.IsValid() )
    {
        if( node->m_LibId == m_curSymbolId && !otherPart )
            sameSymbolDifferentUnit = true;
        else
            m_field_edits.clear();

        if( otherPart )
            m_fp_override.Empty();

        m_curSymbolId = node->m_LibId;
        LIB_SYMBOL* symbol = m_frame->GetLibSymbol( m_curSymbolId );

        updateBodyStyleChoice( symbol );

        m_symbol_preview->DisplaySymbol( m_curSymbolId, node->m_Unit, m_selectedBodyStyle );

        if( !node->m_Footprint.IsEmpty() )
        {
            wxCommandEvent evt( EVT_FOOTPRINT_SELECTED );
            evt.SetString( node->m_Footprint);
            onFootprintSelected( evt );
        }
        else if( sameSymbolDifferentUnit && !m_fp_override.IsEmpty())
        {
            showFootprint( m_fp_override );
        }
        else
        {
            showFootprintFor( m_curSymbolId );
        }

        populateFootprintSelector( m_curSymbolId );

        if( m_compatCallback && m_details )
        {
            std::vector<VARIANT_COMPAT_RESULT> issues = m_compatCallback( m_curSymbolId );

            if( !issues.empty() )
            {
                wxString html = wxS( "<br><b>" ) + _( "Compatibility Warnings:" ) + wxS( "</b><ul>" );

                for( const VARIANT_COMPAT_RESULT& issue : issues )
                    html += wxS( "<li>" ) + EscapeHTML( issue.detail ) + wxS( "</li>" );

                html += wxS( "</ul>" );
                m_details->AppendToPage( html );
            }
        }
    }
    else
    {
        m_curSymbolId = LIB_ID();
        m_symbol_preview->SetStatusText( _( "No symbol selected" ) );

        if( m_fp_preview && m_fp_preview->IsInitialized() )
            m_fp_preview->SetStatusText( wxEmptyString );

        m_field_edits.clear();
        updateBodyStyleChoice( nullptr );
        populateFootprintSelector( m_curSymbolId );
    }

    if( workspace && workspace->kind == WORKSPACE_PARTS_SEARCH::ROW_KIND::PART )
    {
        if( HTML_WINDOW* details = m_tree->GetDetailsCtrl() )
            details->SetPage( workspacePartDetails( group, *workspace, wxEmptyString ) );
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
    if( m_workspaceParts )
        m_workspaceParts->Reresolve();

    LIB_ID savedSelection = m_tree->GetSelectedLibId();
    m_tree->Regenerate( true );

    if( savedSelection.IsValid() )
        m_tree->CenterLibId( savedSelection );
}


void PANEL_SYMBOL_CHOOSER::AcceptSelection()
{
    LIB_TREE_NODE*                     node = m_tree->GetCurrentTreeNode();
    size_t                             group = 0;
    const WORKSPACE_PARTS_SEARCH::ROW* row = workspaceRow( node, &group );

    if( !row )
    {
        m_pickedPart.reset();

        const LIB_TREE_NODE* library = node;

        while( library && library->m_Type != LIB_TREE_NODE::TYPE::LIBRARY )
            library = library->m_Parent;

        // A recently used workspace part is placed as that part again
        if( library && library->m_IsRecentlyUsedGroup )
        {
            if( auto it = m_recentParts.find( node->m_LibId.Format().wx_str() ); it != m_recentParts.end() )
            {
                m_pickedPart.emplace();
                m_pickedPart->part = it->second.part;
                m_pickedPart->revision_id = it->second.revision_id;
                m_pickedSymbol = node->m_LibId;
            }
        }

        m_acceptHandler();
        return;
    }

    if( row->kind != WORKSPACE_PARTS_SEARCH::ROW_KIND::PART
        || row->resolution != WORKSPACE_PARTS_SEARCH::RESOLUTION::RESOLVED || m_confirmTimer->IsRunning() )
    {
        return;
    }

    // Placed as searched unless the lookup below answers in time
    m_pickedPart = row->part;
    m_pickedSymbol = row->symbol;

    const WORKSPACE_PARTS_SEARCH::ROW searched = *row;
    const wxString                    source = m_workspaceParts->Groups()[group].source_name;

    m_confirmTimer->StartOnce( WORKSPACE_CONFIRM_TIMEOUT_MS );
    m_confirmOp = m_workspaceParts->Source( group )->Parts().Get(
            searched.part.part, wxEmptyString,
            [this, group, searched, source]( const MDW_RESULT& aResult, const MDW_PART_INFO& aPart )
            {
                m_confirmTimer->Stop();

                if( aResult.IsOk() )
                {
                    LIB_ID symbol;

                    if( resolveWorkspacePart( aPart, symbol ) == WORKSPACE_PARTS_SEARCH::RESOLUTION::RESOLVED )
                    {
                        acceptWorkspacePart( aPart, symbol );
                        return;
                    }

                    m_pickedPart.reset();

                    if( HTML_WINDOW* details = m_tree->GetDetailsCtrl() )
                    {
                        details->SetPage( workspacePartDetails(
                                group, searched, _( "The part changed and its symbol is not available here." ) ) );
                    }

                    return;
                }

                switch( aResult.error )
                {
                case MDW_ERROR_CODE::CANCELLED:
                    return;

                case MDW_ERROR_CODE::ITEM_NOT_FOUND:
                    m_pickedPart.reset();

                    if( HTML_WINDOW* details = m_tree->GetDetailsCtrl() )
                    {
                        details->SetPage( workspacePartDetails(
                                group, searched, wxString::Format( _( "This part is no longer in %s." ), source ) ) );
                    }

                    return;

                default:
                    m_frame->SetStatusText( wxString::Format( _( "Could not check %s with %s; placed as found." ),
                                                              searched.text, source ) );
                    acceptWorkspacePart( searched.part, searched.symbol );
                    return;
                }
            } );
}


void PANEL_SYMBOL_CHOOSER::acceptWorkspacePart( const MDW_PART_INFO& aPart, const LIB_ID& aSymbol )
{
    m_confirmTimer->Stop();

    const wxString searched = m_pickedPart ? footprintHint( *m_pickedPart ) : wxString();
    const wxString confirmed = footprintHint( aPart );

    // The searched revision chose the footprint unless the user picked another, so a newer one may replace it
    if( confirmed != searched )
    {
        auto edit = std::find_if( m_field_edits.begin(), m_field_edits.end(),
                                  []( const std::pair<FIELD_T, wxString>& aEdit )
                                  {
                                      return aEdit.first == FIELD_T::FOOTPRINT;
                                  } );

        if( edit == m_field_edits.end() || edit->second == searched )
        {
            if( edit != m_field_edits.end() )
                m_field_edits.erase( edit );

            if( !confirmed.IsEmpty() )
                m_field_edits.emplace_back( FIELD_T::FOOTPRINT, confirmed );
        }
    }

    updateBodyStyleChoice( m_frame->GetLibSymbol( aSymbol ) );

    m_pickedPart = aPart;
    m_pickedSymbol = aSymbol;
    m_acceptHandler();
}


void PANEL_SYMBOL_CHOOSER::refillWorkspaceGroup( size_t aGroup )
{
    if( !m_tree || aGroup >= m_workspaceGroups.size() )
        return;

    std::erase_if( m_workspaceRows,
                   [aGroup]( const auto& aEntry )
                   {
                       return aEntry.second.first == aGroup;
                   } );

    const std::vector<WORKSPACE_PARTS_SEARCH::ROW>& rows = m_workspaceParts->Groups()[aGroup].rows;

    m_tree->RefillGroup( *m_workspaceGroups[aGroup],
                         [&]( LIB_TREE_NODE_LIBRARY& aNode )
                         {
                             for( size_t ii = 0; ii < rows.size(); ++ii )
                             {
                                 const WORKSPACE_PARTS_SEARCH::ROW& row = rows[ii];
                                 WORKSPACE_ROW_ITEM                 item( row );
                                 LIB_TREE_NODE_ITEM&                node = aNode.AddItem( &item );
                                 bool resolved = row.resolution == WORKSPACE_PARTS_SEARCH::RESOLUTION::RESOLVED;

                                 // The node names its library after the item, but a part names its symbol
                                 node.m_LibId = resolved ? row.symbol : LIB_ID();
                                 node.m_Unavailable =
                                         row.kind == WORKSPACE_PARTS_SEARCH::ROW_KIND::STATUS
                                         || ( row.kind == WORKSPACE_PARTS_SEARCH::ROW_KIND::PART && !resolved );

                                 m_workspaceRows[&node] = { aGroup, ii };
                             }
                         } );
}


const WORKSPACE_PARTS_SEARCH::ROW* PANEL_SYMBOL_CHOOSER::workspaceRow( const LIB_TREE_NODE* aNode,
                                                                       size_t*              aGroup ) const
{
    auto it = aNode ? m_workspaceRows.find( aNode ) : m_workspaceRows.end();

    if( it == m_workspaceRows.end() )
        return nullptr;

    const std::vector<WORKSPACE_PARTS_SEARCH::GROUP>& groups = m_workspaceParts->Groups();
    auto [group, row] = it->second;

    if( group >= groups.size() || row >= groups[group].rows.size() )
        return nullptr;

    if( aGroup )
        *aGroup = group;

    return &groups[group].rows[row];
}


WORKSPACE_PARTS_SEARCH::RESOLUTION PANEL_SYMBOL_CHOOSER::resolveWorkspacePart( const MDW_PART_INFO& aPart,
                                                                               LIB_ID&              aSymbol ) const
{
    const MDW_PART_MEMBER* member = findMember( aPart, MDW_MEMBER_KIND::SYMBOL );

    if( !member )
        return WORKSPACE_PARTS_SEARCH::RESOLUTION::NO_SYMBOL;

    if( member->member.item_id.IsEmpty() )
        return WORKSPACE_PARTS_SEARCH::RESOLUTION::UNRESOLVED;

    // The item id finds the member in the local copies, which know the nickname this computer registered it under
    std::optional<LIB_ID> id = MDW_LIBRARY_SOURCES::Get().LocalLibId( MDW_LIBRARY_TYPE::SYMBOL, member->member );
    SYMBOL_LIBRARY_ADAPTER* libs = PROJECT_SCH::SymbolLibAdapter( &m_frame->Prj() );

    if( !id || !id->IsValid() || !libs->HasLibrary( id->GetLibNickname(), true ) || !m_frame->GetLibSymbol( *id ) )
        return WORKSPACE_PARTS_SEARCH::RESOLUTION::NOT_SET_UP;

    aSymbol = *id;
    return WORKSPACE_PARTS_SEARCH::RESOLUTION::RESOLVED;
}


wxString PANEL_SYMBOL_CHOOSER::workspacePartDetails( size_t aGroup, const WORKSPACE_PARTS_SEARCH::ROW& aRow,
                                                     const wxString& aNote ) const
{
    const MDW_PART_INFO&   part = aRow.part;
    const wxString&        source = m_workspaceParts->Groups()[aGroup].source_name;
    const MDW_PART_MEMBER* symbol = findMember( part, MDW_MEMBER_KIND::SYMBOL );

    wxString html = wxS( "<b>" ) + EscapeHTML( aRow.text ) + wxS( "</b>" );

    if( !part.description.IsEmpty() )
        html << wxS( "<br>" ) << EscapeHTML( part.description );

    auto line =
            [&]( const wxString& aLabel, const wxString& aValue )
            {
                if( !aValue.IsEmpty() )
                {
                    html << wxS( "<tr><td>" ) << EscapeHTML( aLabel ) << wxS( "</td><td>" ) << EscapeHTML( aValue )
                         << wxS( "</td></tr>" );
                }
            };

    html << wxS( "<table>" );
    line( _( "Revision" ), part.revision_label );
    line( _( "State" ), lifecycleText( part ) );
    line( _( "Source" ), source );
    line( _( "Symbol" ), symbol ? symbol->member.path_hint : wxString() );
    line( _( "Footprint" ), footprintHint( part ) );
    html << wxS( "</table>" );

    if( aRow.resolution == WORKSPACE_PARTS_SEARCH::RESOLUTION::NOT_SET_UP )
    {
        html << wxS( "<p><i>" )
             << _( "The library for this part is not set up on this computer, or its copy does not hold the part "
                   "yet. Updating the library may bring it." )
             << wxS( "</i></p>" );
    }
    else if( aRow.resolution == WORKSPACE_PARTS_SEARCH::RESOLUTION::NO_SYMBOL )
        html << wxS( "<p><i>" ) << _( "This part has no KiCad symbol." ) << wxS( "</i></p>" );
    else if( aRow.resolution == WORKSPACE_PARTS_SEARCH::RESOLUTION::UNRESOLVED && symbol )
    {
        html << wxS( "<p><i>" )
             << EscapeHTML( wxString::Format( _( "%s names symbol %s, which none of its libraries holds." ), source,
                                              symbol->member.path_hint ) )
             << wxS( "</i></p>" );
    }

    if( !aNote.IsEmpty() )
        html << wxS( "<p><b>" ) << EscapeHTML( aNote ) << wxS( "</b></p>" );

    if( !part.url.IsEmpty() )
    {
        html << wxS( "<p><a href=\"" ) << EscapeHTML( part.url ) << wxS( "\">" )
             << EscapeHTML( wxString::Format( _( "Open in %s" ), source ) ) << wxS( "</a></p>" );
    }

    return html;
}
