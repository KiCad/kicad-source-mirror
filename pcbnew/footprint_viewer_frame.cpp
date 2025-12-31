/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2016 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <3d_viewer/eda_3d_viewer_frame.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <board.h>
#include <footprint.h>
#include <confirm.h>
#include <eda_pattern_match.h>
#include <footprint_info.h>
#include <footprint_viewer_frame.h>
#include <footprint_library_adapter.h>
#include <kiway.h>
#include <kiway_express.h>
#include <netlist_reader/pcb_netlist.h>
#include <widgets/kistatusbar.h>
#include <widgets/msgpanel.h>
#include <widgets/wx_listbox.h>
#include <widgets/wx_aui_utils.h>
#include <gal/graphics_abstraction_layer.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_painter.h>
#include <pcbnew_id.h>
#include <footprint_editor_settings.h>
#include <pgm_base.h>
#include <pcbnew_settings.h>
#include <project_pcb.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <toolbars_footprint_viewer.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/selection.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/zoom_tool.h>
#include <tools/pcb_viewer_tools.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_editor_conditions.h>
#include <tools/pcb_control.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_selection_tool.h>
#include <tools/board_editor_control.h>
#include <wildcards_and_files_ext.h>
#include <lib_tree_model_adapter.h>
#include <wx/srchctrl.h>
#include <wx/tokenzr.h>
#include <wx/choice.h>
#include <wx/hyperlink.h>

#include "invoke_pcb_dialog.h"

using namespace std::placeholders;

BEGIN_EVENT_TABLE( FOOTPRINT_VIEWER_FRAME, PCB_BASE_FRAME )
    // Window events
    EVT_SIZE( FOOTPRINT_VIEWER_FRAME::OnSize )
    EVT_ACTIVATE( FOOTPRINT_VIEWER_FRAME::OnActivate )

    EVT_MENU( wxID_EXIT, FOOTPRINT_VIEWER_FRAME::OnExitKiCad )
    EVT_MENU( wxID_CLOSE, FOOTPRINT_VIEWER_FRAME::CloseFootprintViewer )

    // Toolbar events
    EVT_CHOICE( ID_ON_ZOOM_SELECT, FOOTPRINT_VIEWER_FRAME::OnSelectZoom )
    EVT_CHOICE( ID_ON_GRID_SELECT, FOOTPRINT_VIEWER_FRAME::OnSelectGrid )

    EVT_TEXT( ID_MODVIEW_LIB_FILTER, FOOTPRINT_VIEWER_FRAME::OnLibFilter )
    EVT_TEXT( ID_MODVIEW_FOOTPRINT_FILTER, FOOTPRINT_VIEWER_FRAME::OnFPFilter )

    // listbox events
    EVT_LISTBOX( ID_MODVIEW_LIB_LIST, FOOTPRINT_VIEWER_FRAME::ClickOnLibList )
    EVT_LISTBOX( ID_MODVIEW_FOOTPRINT_LIST, FOOTPRINT_VIEWER_FRAME::ClickOnFootprintList )

END_EVENT_TABLE()


FOOTPRINT_VIEWER_FRAME::FOOTPRINT_VIEWER_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    PCB_BASE_FRAME( aKiway, aParent, FRAME_FOOTPRINT_VIEWER, _( "Footprint Library Browser" ),
                    wxDefaultPosition, wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE,
                    FOOTPRINT_VIEWER_FRAME_NAME ),
   m_comp( LIB_ID(), wxEmptyString, wxEmptyString, KIID_PATH(), {} )
{
    m_aboutTitle = _HKI( "KiCad Footprint Library Browser" );

    // Force the items to always snap
    m_magneticItems.pads     = MAGNETIC_OPTIONS::CAPTURE_ALWAYS;
    m_magneticItems.tracks   = MAGNETIC_OPTIONS::CAPTURE_ALWAYS;
    m_magneticItems.graphics = true;

    // Force the frame name used in config. the footprint viewer frame has a name
    // depending on aFrameType (needed to identify the frame by wxWidgets),
    // but only one configuration is preferable.
    m_configName = FOOTPRINT_VIEWER_FRAME_NAME;

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_footprint_browser ) );
    SetIcon( icon );

    m_libListWidth = 200;
    m_fpListWidth = 300;

    wxPanel* libPanel = new wxPanel( this );
    wxSizer* libSizer = new wxBoxSizer( wxVERTICAL );

    m_libFilter = new wxSearchCtrl( libPanel, ID_MODVIEW_LIB_FILTER, wxEmptyString,
                                    wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    m_libFilter->SetDescriptiveText( _( "Filter" ) );
    libSizer->Add( m_libFilter, 0, wxEXPAND, 5 );

    m_libList = new WX_LISTBOX( libPanel, ID_MODVIEW_LIB_LIST, wxDefaultPosition, wxDefaultSize,
                                0, nullptr, wxLB_HSCROLL | wxNO_BORDER );
    libSizer->Add( m_libList, 1, wxEXPAND, 5 );

    libPanel->SetSizer( libSizer );
    libPanel->Fit();

    wxPanel* fpPanel = new wxPanel( this );
    wxSizer* fpSizer = new wxBoxSizer( wxVERTICAL );

    m_fpFilter = new wxSearchCtrl( fpPanel, ID_MODVIEW_FOOTPRINT_FILTER, wxEmptyString,
                                   wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    m_fpFilter->SetDescriptiveText( _( "Filter" ) );
    m_fpFilter->SetToolTip(
            _( "Filter on footprint name, keywords, description and pad count.\n"
               "Search terms are separated by spaces.  All search terms must match.\n"
               "A term which is a number will also match against the pad count." ) );
    fpSizer->Add( m_fpFilter, 0, wxEXPAND, 5 );

#ifdef __WXGTK__
    // wxSearchCtrl vertical height is not calculated correctly on some GTK setups
    // See https://gitlab.com/kicad/code/kicad/-/issues/9019
    m_libFilter->SetMinSize( wxSize( -1, GetTextExtent( wxT( "qb" ) ).y + 10 ) );
    m_fpFilter->SetMinSize( wxSize( -1, GetTextExtent( wxT( "qb" ) ).y + 10 ) );
#endif

    m_fpList = new WX_LISTBOX( fpPanel, ID_MODVIEW_FOOTPRINT_LIST, wxDefaultPosition, wxDefaultSize,
                               0, nullptr, wxLB_HSCROLL | wxNO_BORDER );

    m_fpList->Connect( wxEVT_LEFT_DCLICK,
                       wxMouseEventHandler( FOOTPRINT_VIEWER_FRAME::DClickOnFootprintList ),
                       nullptr, this );
    fpSizer->Add( m_fpList, 1, wxEXPAND, 5 );

    fpPanel->SetSizer( fpSizer );
    fpPanel->Fit();

    // Create GAL canvas
    m_canvasType = loadCanvasTypeSetting();

    PCB_DRAW_PANEL_GAL* drawPanel = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_frameSize,
                                                            GetGalDisplayOptions(), m_canvasType );
    SetCanvas( drawPanel );

    SetBoard( new BOARD() );

    // This board will only be used to hold a footprint for viewing
    GetBoard()->SetBoardUse( BOARD_USE::FPHOLDER );

    // In viewer, the default net clearance is not known (it depends on the actual board).
    // So we do not show the default clearance, by setting it to 0
    // The footprint or pad specific clearance will be shown
    GetBoard()->GetDesignSettings().m_NetSettings->GetDefaultNetclass()->SetClearance( 0 );

    // Don't show the default board solder mask clearance in the footprint viewer.  Only the
    // footprint or pad clearance setting should be shown if it is not 0.
    GetBoard()->GetDesignSettings().m_SolderMaskExpansion = 0;

    // Ensure all layers and items are visible:
    GetBoard()->SetVisibleAlls();
    SetScreen( new PCB_SCREEN( GetPageSizeIU() ) );

    GetScreen()->m_Center = true;      // Center coordinate origins on screen.
    LoadSettings( config() );
    GetGalDisplayOptions().m_axesEnabled = true;

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), drawPanel->GetView(),
                                   drawPanel->GetViewControls(), config(), this );
    m_actions = new PCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );
    drawPanel->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->RegisterTool( new PCB_CONTROL );
    m_toolManager->RegisterTool( new PCB_SELECTION_TOOL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );    // for std context menus (zoom & grid)
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new PCB_PICKER_TOOL ); // for setting grid origin
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new PCB_VIEWER_TOOLS );

    m_toolManager->GetTool<PCB_VIEWER_TOOLS>()->SetFootprintFrame( true );

    m_toolManager->InitTools();
    m_toolManager->InvokeTool( "common.InteractiveSelection" );

    setupUIConditions();

    m_toolbarSettings = GetToolbarSettings<FOOTPRINT_VIEWER_TOOLBAR_SETTINGS>( "fpviewer-toolbars" );
    configureToolbars();
    RecreateToolbars();
    ReCreateMenuBar();

    ReCreateLibraryList();
    UpdateTitle();

    // Call resolveCanvasType after loading settings:
    resolveCanvasType();

    // If a footprint was previously loaded, reload it
    if( getCurNickname().size() && getCurFootprintName().size() )
    {
        LIB_ID id;

        id.SetLibNickname( getCurNickname() );
        id.SetLibItemName( getCurFootprintName() );

        FOOTPRINT* footprint = loadFootprint( id );

        if( footprint )
        {
            GetBoard()->Add( footprint );
            setFPWatcher( footprint );

            m_toolManager->RunAction( PCB_ACTIONS::rehatchShapes );
        }
    }

    drawPanel->DisplayBoard( m_pcb );

    m_auimgr.SetManagedWindow( this );

    // Horizontal items; layers 4 - 6
    m_auimgr.AddPane( m_tbTopMain, EDA_PANE().VToolbar().Name( "TopMainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_tbLeft, EDA_PANE().VToolbar().Name( "LeftToolbar" ).Left().Layer(3) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    // Vertical items; layers 1 - 3
    m_auimgr.AddPane( libPanel, EDA_PANE().Palette().Name( "Libraries" ).Left().Layer(2)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 200, -1 ) );
    m_auimgr.AddPane( fpPanel, EDA_PANE().Palette().Name( "Footprints" ).Left().Layer(1)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 300, -1 ) );

    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    // after changing something to the aui manager call Update() to reflect the changes
    m_auimgr.Update();

    if( m_libListWidth > 0 )
        SetAuiPaneSize( m_auimgr, m_auimgr.GetPane( "Libraries" ), m_libListWidth, -1 );

    if( m_fpListWidth > 0 )
        SetAuiPaneSize( m_auimgr, m_auimgr.GetPane( "Footprints" ), m_fpListWidth, -1 );

    // The canvas should not steal the focus from the list boxes
    GetCanvas()->SetCanFocus( false );
    GetCanvas()->GetGAL()->SetAxesEnabled( true );
    ActivateGalCanvas();

    // Restore last zoom and auto zoom option.  (If auto-zooming we'll adjust when we load the footprint.)
    if( PCBNEW_SETTINGS* cfg = GetPcbNewSettings() )
        GetCanvas()->GetView()->SetScale( cfg->m_FootprintViewerZoom );

    updateView();
    setupUnits( config() );

    ReCreateFootprintList();
    Raise();            // On some window managers, this is needed
    Show( true );
}


FOOTPRINT_VIEWER_FRAME::~FOOTPRINT_VIEWER_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    GetCanvas()->StopDrawing();
    GetCanvas()->GetView()->Clear();
    // Be sure any event cannot be fired after frame deletion:
    GetCanvas()->SetEvtHandlerEnabled( false );
    m_fpList->Disconnect( wxEVT_LEFT_DCLICK,
                          wxMouseEventHandler( FOOTPRINT_VIEWER_FRAME::DClickOnFootprintList ),
                          nullptr, this );
    setFPWatcher( nullptr );
}


SELECTION& FOOTPRINT_VIEWER_FRAME::GetCurrentSelection()
{
    return m_toolManager->GetTool<PCB_SELECTION_TOOL>()->GetSelection();
}


void FOOTPRINT_VIEWER_FRAME::UpdateMsgPanel()
{
    EDA_DRAW_FRAME::UpdateMsgPanel();

    if( FOOTPRINT* fp = static_cast<FOOTPRINT*>( GetModel() ) )
    {
        std::vector<MSG_PANEL_ITEM> msgItems;
        fp->GetMsgPanelInfo( this, msgItems );
        SetMsgPanel( msgItems );
    }
}


void FOOTPRINT_VIEWER_FRAME::setupUIConditions()
{
    PCB_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*       mgr = m_toolManager->GetActionManager();
    PCB_EDITOR_CONDITIONS cond( this );

    wxASSERT( mgr );

    auto addToBoardCond =
            [this]( const SELECTION& )
            {
                return ( GetBoard()->GetFirstFootprint() != nullptr );
            };

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    mgr->SetConditions( ACTIONS::toggleGrid,        CHECK( cond.GridVisible() ) );
    mgr->SetConditions( ACTIONS::cursorSmallCrosshairs, CHECK( cond.CursorSmallCrosshairs() ) );
    mgr->SetConditions( ACTIONS::cursorFullCrosshairs,  CHECK( cond.CursorFullCrosshairs() ) );
    mgr->SetConditions( ACTIONS::cursor45Crosshairs,    CHECK( cond.Cursor45Crosshairs() ) );

    mgr->SetConditions( PCB_ACTIONS::saveFpToBoard, ENABLE( addToBoardCond ) );

    mgr->SetConditions( ACTIONS::zoomTool,
                        CHECK( cond.CurrentTool( ACTIONS::zoomTool ) ) );
    mgr->SetConditions( ACTIONS::measureTool,
                        CHECK( cond.CurrentTool( ACTIONS::measureTool ) ) );
    mgr->SetConditions( ACTIONS::selectionTool,
                        CHECK( cond.CurrentTool( ACTIONS::selectionTool ) ) );

    mgr->SetConditions( PCB_ACTIONS::showPadNumbers,     CHECK( cond.PadNumbersDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::padDisplayMode,     CHECK( !cond.PadFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::textOutlines,       CHECK( !cond.TextFillDisplay() ) );
    mgr->SetConditions( PCB_ACTIONS::graphicsOutlines,   CHECK( !cond.GraphicsFillDisplay() ) );
    mgr->SetConditions( ACTIONS::toggleBoundingBoxes,    CHECK( cond.BoundingBoxes() ) );

    mgr->SetConditions( PCB_ACTIONS::fpAutoZoom,         CHECK( cond.FootprintViewerAutoZoom() ) );

#undef ENABLE
#undef CHECK
}


void FOOTPRINT_VIEWER_FRAME::doCloseWindow()
{
    // A workaround to avoid flicker, in modal mode when modview frame is destroyed,
    // when the aui toolbar is not docked (i.e. shown in a miniframe)
    // (useful on windows only)
    m_tbTopMain->SetFocus();

    GetCanvas()->StopDrawing();

    Destroy();
}


void FOOTPRINT_VIEWER_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    SizeEv.Skip();
}


void FOOTPRINT_VIEWER_FRAME::ReCreateLibraryList()
{
    m_libList->Clear();

    COMMON_SETTINGS*      cfg = Pgm().GetCommonSettings();
    PROJECT_FILE&         project = Kiway().Prj().GetProjectFile();
    std::vector<wxString> nicknames = PROJECT_PCB::FootprintLibAdapter( &Prj() )->GetLibraryNames();
    std::vector<wxString> pinnedMatches;
    std::vector<wxString> otherMatches;

    auto process =
            [&]( const wxString& aNickname )
            {
                if( alg::contains( project.m_PinnedFootprintLibs, aNickname )
                        || alg::contains( cfg->m_Session.pinned_fp_libs, aNickname ) )
                {
                    pinnedMatches.push_back( aNickname );
                }
                else
                {
                    otherMatches.push_back( aNickname );
                }
            };

    if( m_libFilter->GetValue().IsEmpty() )
    {
        for( const wxString& nickname : nicknames )
            process( nickname );
    }
    else
    {
        wxStringTokenizer tokenizer( m_libFilter->GetValue(), " \t\r\n", wxTOKEN_STRTOK );

        while( tokenizer.HasMoreTokens() )
        {
            const wxString       term = tokenizer.GetNextToken().Lower();
            EDA_COMBINED_MATCHER matcher( term, CTX_LIBITEM );

            for( const wxString& nickname : nicknames )
            {
                if( matcher.Find( nickname.Lower() ) )
                    process( nickname );
            }
        }
    }

    for( const wxString& nickname : pinnedMatches )
        m_libList->Append( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() + nickname );

    for( const wxString& nickname : otherMatches )
        m_libList->Append( nickname );

    // Search for a previous selection:
    int index =  m_libList->FindString( getCurNickname(), true );

    if( index == wxNOT_FOUND )
    {
        if( m_libList->GetCount() > 0 )
        {
            m_libList->SetSelection( 0 );
            wxCommandEvent dummy;
            ClickOnLibList( dummy );
        }
        else
        {
            setCurNickname( wxEmptyString );
            setCurFootprintName( wxEmptyString );
        }
    }
    else
    {
        m_libList->SetSelection( index, true );
        wxCommandEvent dummy;
        ClickOnLibList( dummy );
    }

    GetCanvas()->Refresh();
}


void FOOTPRINT_VIEWER_FRAME::ReCreateFootprintList()
{
    m_fpList->Clear();

    if( !getCurNickname() )
        setCurFootprintName( wxEmptyString );

    auto fp_info_list = FOOTPRINT_LIST::GetInstance( Kiway() );

    wxString nickname = getCurNickname();

    fp_info_list->ReadFootprintFiles( PROJECT_PCB::FootprintLibAdapter( &Prj() ), !nickname ? nullptr : &nickname );

    if( fp_info_list->GetErrorCount() )
    {
        if( KISTATUSBAR* statusBar = dynamic_cast<KISTATUSBAR*>( GetStatusBar() ) )
            statusBar->SetLoadWarningMessages( fp_info_list->GetErrorMessages() );

        // For footprint libraries that support one footprint per file, there may have been
        // valid footprints read so show the footprints that loaded properly.
        if( fp_info_list->GetList().empty() )
            return;
    }

    std::set<wxString> excludes;

    if( !m_fpFilter->GetValue().IsEmpty() )
    {
        wxStringTokenizer tokenizer( m_fpFilter->GetValue(), " \t\r\n", wxTOKEN_STRTOK );

        while( tokenizer.HasMoreTokens() )
        {
            const wxString       filterTerm = tokenizer.GetNextToken().Lower();
            EDA_COMBINED_MATCHER matcher( filterTerm, CTX_LIBITEM );

            for( const std::unique_ptr<FOOTPRINT_INFO>& footprint : fp_info_list->GetList() )
            {
                std::vector<SEARCH_TERM> searchTerms = footprint->GetSearchTerms();
                int                      matched = matcher.ScoreTerms( searchTerms );

                if( filterTerm.IsNumber() && wxAtoi( filterTerm ) == (int)footprint->GetPadCount() )
                    matched++;

                if( !matched )
                    excludes.insert( footprint->GetFootprintName() );
            }
        }
    }

    for( const std::unique_ptr<FOOTPRINT_INFO>& footprint : fp_info_list->GetList() )
    {
        if( !excludes.count( footprint->GetFootprintName() ) )
            m_fpList->Append( footprint->GetFootprintName() );
    }

    int index = wxNOT_FOUND;

    if( !getCurFootprintName().IsEmpty() )
        index = m_fpList->FindString( getCurFootprintName(), true );

    if( index == wxNOT_FOUND )
    {
        if( m_fpList->GetCount() > 0 )
        {
            m_fpList->SetSelection( 0 );
            m_fpList->EnsureVisible( 0 );

            wxCommandEvent dummy;
            ClickOnFootprintList( dummy );
        }
        else
        {
            setCurFootprintName( wxEmptyString );
        }
    }
    else
    {
        m_fpList->SetSelection( index, true );
        m_fpList->EnsureVisible( index );

        wxCommandEvent dummy;
        ClickOnFootprintList( dummy );
    }
}


void FOOTPRINT_VIEWER_FRAME::OnLibFilter( wxCommandEvent& aEvent )
{
    ReCreateLibraryList();

    // Required to avoid interaction with SetHint()
    // See documentation for wxTextEntry::SetHint
    aEvent.Skip();
}


void FOOTPRINT_VIEWER_FRAME::OnFPFilter( wxCommandEvent& aEvent )
{
    ReCreateFootprintList();

    // Required to avoid interaction with SetHint()
    // See documentation for wxTextEntry::SetHint
    aEvent.Skip();
}


void FOOTPRINT_VIEWER_FRAME::OnCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_UP )
    {
        if( m_libFilter->HasFocus() || m_libList->HasFocus() )
            selectPrev( m_libList );
        else
            selectPrev( m_fpList );
    }
    else if( aEvent.GetKeyCode() == WXK_DOWN )
    {
        if( m_libFilter->HasFocus() || m_libList->HasFocus() )
            selectNext( m_libList );
        else
            selectNext( m_fpList );
    }
    else if( aEvent.GetKeyCode() == WXK_TAB && m_libFilter->HasFocus() )
    {
        if( !aEvent.ShiftDown() )
            m_fpFilter->SetFocus();
        else
            aEvent.Skip();
    }
    else if( aEvent.GetKeyCode() == WXK_TAB && m_fpFilter->HasFocus() )
    {
        if( aEvent.ShiftDown() )
            m_libFilter->SetFocus();
        else
            aEvent.Skip();
    }
    else if( ( aEvent.GetKeyCode() == WXK_RETURN || aEvent.GetKeyCode() == WXK_NUMPAD_ENTER )
             && m_fpList->GetSelection() >= 0 )
    {
        AddFootprintToPCB();
    }
    else
    {
        aEvent.Skip();
    }
}


void FOOTPRINT_VIEWER_FRAME::selectPrev( WX_LISTBOX* aListBox )
{
    int prev = aListBox->GetSelection() - 1;

    if( prev >= 0 )
    {
        aListBox->SetSelection( prev );
        aListBox->EnsureVisible( prev );

        wxCommandEvent dummy;

        if( aListBox == m_libList )
            ClickOnLibList( dummy );
        else
            ClickOnFootprintList( dummy );
    }
}


void FOOTPRINT_VIEWER_FRAME::selectNext( WX_LISTBOX* aListBox )
{
    int next = aListBox->GetSelection() + 1;

    if( next < (int)aListBox->GetCount() )
    {
        aListBox->SetSelection( next );
        aListBox->EnsureVisible( next );

        wxCommandEvent dummy;

        if( aListBox == m_libList )
            ClickOnLibList( dummy );
        else
            ClickOnFootprintList( dummy );
    }
}


void FOOTPRINT_VIEWER_FRAME::ClickOnLibList( wxCommandEvent& aEvent )
{
    int ii = m_libList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_libList->GetBaseString( ii );

    if( getCurNickname() == name )
        return;

    setCurNickname( name );

    // Ensure the displayed footprint is loade/reloaded from the new library
    setCurFootprintName( wxEmptyString );

    ReCreateFootprintList();
    UpdateTitle();
}


void FOOTPRINT_VIEWER_FRAME::ClickOnFootprintList( wxCommandEvent& aEvent )
{
    if( m_fpList->GetCount() == 0 )
        return;

    int ii = m_fpList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_fpList->GetBaseString( ii );

    if( getCurFootprintName().CmpNoCase( name ) != 0 )
    {
        setCurFootprintName( name );
        SelectAndViewFootprint( FPVIEWER_CONSTANTS::NEW_PART );
    }
}


void FOOTPRINT_VIEWER_FRAME::displayFootprint( FOOTPRINT* aFootprint )
{
    for( PAD* pad : aFootprint->Pads() )
    {
        const COMPONENT_NET& net = m_comp.GetNet( pad->GetNumber() );

        if( !net.GetPinFunction().IsEmpty() )
        {
            NETINFO_ITEM* netinfo = new NETINFO_ITEM( GetBoard() );
            netinfo->SetNetname( net.GetPinFunction() );
            GetBoard()->Add( netinfo );
            pad->SetNet( netinfo );
        }
    }

    GetBoard()->Add( aFootprint );

    m_toolManager->RunAction( PCB_ACTIONS::rehatchShapes );
}


void FOOTPRINT_VIEWER_FRAME::DClickOnFootprintList( wxMouseEvent& aEvent )
{
    AddFootprintToPCB();
}


void FOOTPRINT_VIEWER_FRAME::AddFootprintToPCB()
{
    if( GetBoard()->GetFirstFootprint() )
    {
        PCB_EDIT_FRAME*  pcbframe = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB_EDITOR, false );

        if( pcbframe == nullptr )      // happens when the board editor is not active (or closed)
        {
            DisplayErrorMessage( this, _( "No board currently open." ) );
            return;
        }

        PCBNEW_SETTINGS* cfg = pcbframe->GetPcbNewSettings();
        TOOL_MANAGER*    toolMgr = pcbframe->GetToolManager();

        if( toolMgr->GetTool<BOARD_EDITOR_CONTROL>()->PlacingFootprint() )
        {
            DisplayError( this, _( "Previous footprint placement still in progress." ) );
            return;
        }

        wxWindow* blocking_dialog = pcbframe->Kiway().GetBlockingDialog();

        if( blocking_dialog )
            blocking_dialog->Close( true );

        toolMgr->RunAction( ACTIONS::selectionClear );
        BOARD_COMMIT commit( pcbframe );

        // Create the "new" footprint
        FOOTPRINT* newFootprint = (FOOTPRINT*) GetBoard()->GetFirstFootprint()->Duplicate( IGNORE_PARENT_GROUP );
        newFootprint->SetParent( pcbframe->GetBoard() );
        newFootprint->SetLink( niluuid );
        newFootprint->SetFlags(IS_NEW ); // whatever

        for( PAD* pad : newFootprint->Pads() )
        {
            // Set the pads ratsnest settings to the global settings
            pad->SetLocalRatsnestVisible( cfg->m_Display.m_ShowGlobalRatsnest );

            // Pads in the library all have orphaned nets.  Replace with Default.
            pad->SetNetCode( 0 );
        }

        // Put it on FRONT layer,
        // (Can be stored flipped if the lib is an archive built from a board)
        if( newFootprint->IsFlipped() )
            newFootprint->Flip( newFootprint->GetPosition(), cfg->m_FlipDirection );

        KIGFX::VIEW_CONTROLS* viewControls = pcbframe->GetCanvas()->GetViewControls();
        VECTOR2D              cursorPos = viewControls->GetCursorPosition();

        commit.Add( newFootprint );
        viewControls->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
        pcbframe->PlaceFootprint( newFootprint );

        newFootprint->SetPosition( VECTOR2I( 0, 0 ) );
        viewControls->SetCrossHairCursorPosition( cursorPos, false );
        commit.Push( _( "Insert Footprint" ) );

        pcbframe->Raise();
        toolMgr->PostAction( PCB_ACTIONS::placeFootprint, newFootprint );

        newFootprint->ClearFlags();
    }
}


void FOOTPRINT_VIEWER_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg ) )
    {
        // We don't allow people to change this right now, so make sure it's on
        GetWindowSettings( cfg )->cursor.always_show_cursor = true;

        PCB_BASE_FRAME::LoadSettings( aCfg );

        // Fetch display and grid settings from Footprint Editor
        if( FOOTPRINT_EDITOR_SETTINGS* fpedit = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" ) )
        {
            m_displayOptions = fpedit->m_Display;
            GetGalDisplayOptions().ReadWindowSettings( fpedit->m_Window );
        }

        m_libListWidth = cfg->m_FootprintViewerLibListWidth;
        m_fpListWidth = cfg->m_FootprintViewerFPListWidth;

        // Set parameters to a reasonable value.
        int maxWidth = cfg->m_FootprintViewer.state.size_x - 80;

        if( m_libListWidth + m_fpListWidth > maxWidth )
        {
            m_libListWidth = maxWidth * ( m_libListWidth / ( m_libListWidth + m_fpListWidth ) );
            m_fpListWidth = maxWidth - m_libListWidth;
        }
    }
}


void FOOTPRINT_VIEWER_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxCHECK( cfg, /*void*/ );

    GetGalDisplayOptions().m_axesEnabled = true;

    // We don't want to store anything other than the window settings
    PCB_BASE_FRAME::SaveSettings( cfg );

    if( GetCanvas() && GetCanvas()->GetView() )
        cfg->m_FootprintViewerZoom = GetCanvas()->GetView()->GetScale();

    cfg->m_FootprintViewerLibListWidth = m_libList->GetSize().x;
    cfg->m_FootprintViewerFPListWidth = m_fpList->GetSize().x;

}


WINDOW_SETTINGS* FOOTPRINT_VIEWER_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    if( PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg ) )
        return &cfg->m_FootprintViewer;

    wxFAIL_MSG( wxT( "FOOTPRINT_CHOOSER not running with PCBNEW_SETTINGS" ) );
    return &aCfg->m_Window;     // non-null fail-safe
}


COLOR_SETTINGS* FOOTPRINT_VIEWER_FRAME::GetColorSettings( bool aForceRefresh ) const
{
    FOOTPRINT_EDITOR_SETTINGS* cfg = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" );
    return ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );
}


void FOOTPRINT_VIEWER_FRAME::CommonSettingsChanged( int aFlags )
{
    PCB_BASE_FRAME::CommonSettingsChanged( aFlags );

    GetCanvas()->ForceRefresh();

    if( aFlags & ENVVARS_CHANGED )
        ReCreateLibraryList();
}


const wxString FOOTPRINT_VIEWER_FRAME::getCurNickname()
{
    return Prj().GetRString( PROJECT::PCB_FOOTPRINT_VIEWER_LIB_NICKNAME );
}


void FOOTPRINT_VIEWER_FRAME::setCurNickname( const wxString& aNickname )
{
    Prj().SetRString( PROJECT::PCB_FOOTPRINT_VIEWER_LIB_NICKNAME, aNickname );
}


const wxString FOOTPRINT_VIEWER_FRAME::getCurFootprintName()
{
    return Prj().GetRString( PROJECT::PCB_FOOTPRINT_VIEWER_FP_NAME );
}


void FOOTPRINT_VIEWER_FRAME::setCurFootprintName( const wxString& aName )
{
    Prj().SetRString( PROJECT::PCB_FOOTPRINT_VIEWER_FP_NAME, aName );
}


void FOOTPRINT_VIEWER_FRAME::OnActivate( wxActivateEvent& event )
{
    if( event.GetActive() )
    {
        // Ensure we have the right library list:
        std::vector< wxString > libNicknames = PROJECT_PCB::FootprintLibAdapter( &Prj() )->GetLibraryNames();
        bool                    stale = false;

        if( libNicknames.size() != m_libList->GetCount() )
            stale = true;
        else
        {
            for( unsigned ii = 0;  ii < libNicknames.size();  ii++ )
            {
                if( libNicknames[ii] != m_libList->GetBaseString( ii ) )
                {
                    stale = true;
                    break;
                }
            }
        }

        if( stale )
        {
            ReCreateLibraryList();
            UpdateTitle();
        }
    }

    event.Skip();    // required under wxMAC
}


void FOOTPRINT_VIEWER_FRAME::ReloadFootprint( FOOTPRINT* aFootprint )
{
    setCurNickname( aFootprint->GetFPID().GetLibNickname() );
    setCurFootprintName( aFootprint->GetFPID().GetLibItemName() );
    SelectAndViewFootprint( FPVIEWER_CONSTANTS::RELOAD_PART );
}


void FOOTPRINT_VIEWER_FRAME::HardRedraw()
{
    ReCreateLibraryList();
    ReCreateFootprintList();
    ReloadFootprint( GetBoard()->GetFirstFootprint() );
}

void FOOTPRINT_VIEWER_FRAME::KiwayMailIn( KIWAY_EXPRESS& mail )
{
    switch( mail.Command() )
    {
    case MAIL_RELOAD_LIB:
        ReCreateLibraryList();
        break;

    default:
        break;
    }
}


void FOOTPRINT_VIEWER_FRAME::Update3DView( bool aMarkDirty, bool aRefresh, const wxString* aTitle )
{
    wxString title = _( "3D Viewer" ) + wxT( " \u2014 " ) + getCurFootprintName();
    PCB_BASE_FRAME::Update3DView( aMarkDirty, aRefresh, &title );
}


COLOR4D FOOTPRINT_VIEWER_FRAME::GetGridColor()
{
    return GetColorSettings()->GetColor( LAYER_GRID );
}


void FOOTPRINT_VIEWER_FRAME::UpdateTitle()
{
    wxString title;
    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();

    if( !getCurNickname().IsEmpty() )
    {
        if( std::optional<wxString> optUri = manager.GetFullURI( LIBRARY_TABLE_TYPE::FOOTPRINT, getCurNickname(), true ) )
            title = getCurNickname() + wxT( " \u2014 " ) + *optUri;
        else
            title = _( "[no library selected]" );
    }
    else
    {
        title = _( "[no library selected]" );
    }

    title += wxT( " \u2014 " ) + _( "Footprint Library Browser" );

    SetTitle( title );
}


void FOOTPRINT_VIEWER_FRAME::SelectAndViewFootprint( FPVIEWER_CONSTANTS aMode )
{
    if( !getCurNickname() )
        return;

    int selection = m_fpList->FindString( getCurFootprintName(), true );

    if( aMode == FPVIEWER_CONSTANTS::NEXT_PART )
    {
        if( selection != wxNOT_FOUND && selection < (int)m_fpList->GetCount() - 1 )
            selection++;
    }

    if( aMode == FPVIEWER_CONSTANTS::PREVIOUS_PART )
    {
        if( selection != wxNOT_FOUND && selection > 0 )
            selection--;
    }

    if( selection != wxNOT_FOUND )
    {
        m_fpList->SetSelection( selection );
        m_fpList->EnsureVisible( selection );

        setCurFootprintName( m_fpList->GetBaseString( selection ) );

        // Delete the current footprint
        if( PCB_SELECTION_TOOL* selectionTool = m_toolManager->GetTool<PCB_SELECTION_TOOL>() )
            selectionTool->ClearSelection( true /* quiet mode */ );

        GetBoard()->DeleteAllFootprints();
        GetBoard()->RemoveUnusedNets( nullptr );

        FOOTPRINT* footprint = PROJECT_PCB::FootprintLibAdapter( &Prj() )->LoadFootprint( getCurNickname(),
                                                                                          getCurFootprintName(), false );

        if( footprint )
            displayFootprint( footprint );

        if( aMode != FPVIEWER_CONSTANTS::RELOAD_PART )
            setFPWatcher( footprint );

        Update3DView( true, true );
        updateView();
    }

    UpdateTitle();

    GetCanvas()->Refresh();
}


void FOOTPRINT_VIEWER_FRAME::updateView()
{
    GetCanvas()->UpdateColors();
    GetCanvas()->DisplayBoard( GetBoard() );

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( config() );
    wxCHECK( cfg, /* void */ );

    if( cfg->m_FootprintViewerAutoZoomOnSelect )
        m_toolManager->RunAction( ACTIONS::zoomFitScreen );
    else
        m_toolManager->RunAction( ACTIONS::centerContents );


    UpdateMsgPanel();
}


void FOOTPRINT_VIEWER_FRAME::OnExitKiCad( wxCommandEvent& event )
{
    Kiway().OnKiCadExit();
}


void FOOTPRINT_VIEWER_FRAME::CloseFootprintViewer( wxCommandEvent& event )
{
    Close( false );
}


BOARD_ITEM_CONTAINER* FOOTPRINT_VIEWER_FRAME::GetModel() const
{
    return GetBoard()->GetFirstFootprint();
}

