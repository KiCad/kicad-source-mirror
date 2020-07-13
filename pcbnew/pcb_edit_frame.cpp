/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2013-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <pcb_edit_frame.h>
#include <3d_viewer/eda_3d_viewer.h>
#include <fp_lib_table.h>
#include <bitmaps.h>
#include <trace_helpers.h>
#include <pcbnew_id.h>
#include <drc/drc.h>
#include <pcbnew_settings.h>
#include <layer_widget.h>
#include <pcb_layer_widget.h>
#include <footprint_edit_frame.h>
#include <dialog_plot.h>
#include <dialog_edit_footprint_for_BoardEditor.h>
#include <dialogs/dialog_exchange_footprints.h>
#include <dialog_board_setup.h>
#include <convert_to_biu.h>
#include <view/view_controls.h>
#include <pcb_painter.h>
#include <invoke_pcb_dialog.h>
#include <class_board.h>
#include <class_module.h>
#include <ws_proxy_view_item.h>
#include <connectivity/connectivity_data.h>
#include <wildcards_and_files_ext.h>
#include <pcb_draw_panel_gal.h>
#include <functional>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/zoom_tool.h>
#include <tools/selection_tool.h>
#include <tools/pcbnew_picker_tool.h>
#include <tools/point_editor.h>
#include <tools/edit_tool.h>
#include <tools/global_edit_tool.h>
#include <tools/drawing_tool.h>
#include <tools/pcbnew_control.h>
#include <tools/pcb_editor_control.h>
#include <tools/pcb_inspection_tool.h>
#include <tools/pcb_viewer_tools.h>
#include <tools/placement_tool.h>
#include <tools/pad_tool.h>
#include <microwave/microwave_tool.h>
#include <tools/position_relative_tool.h>
#include <tools/zone_filler_tool.h>
#include <tools/pcb_actions.h>
#include <router/router_tool.h>
#include <router/length_tuner_tool.h>
#include <autorouter/autoplacer_tool.h>
#include <gestfich.h>
#include <executable_names.h>
#include <netlist_reader/board_netlist_updater.h>
#include <netlist_reader/netlist_reader.h>
#include <netlist_reader/pcb_netlist.h>
#include <wx/wupdlock.h>
#include <dialog_drc.h>     // for DIALOG_DRC_WINDOW_NAME definition
#include <ratsnest/ratsnest_viewitem.h>
#include <widgets/panel_selection_filter.h>

#include <widgets/infobar.h>

#if defined(KICAD_SCRIPTING) || defined(KICAD_SCRIPTING_WXPYTHON)
#include <python_scripting.h>
#endif


using namespace std::placeholders;


BEGIN_EVENT_TABLE( PCB_EDIT_FRAME, PCB_BASE_FRAME )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT_SERV, PCB_EDIT_FRAME::OnSockRequestServer )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT, PCB_EDIT_FRAME::OnSockRequest )

    EVT_CHOICE( ID_ON_ZOOM_SELECT, PCB_EDIT_FRAME::OnSelectZoom )
    EVT_CHOICE( ID_ON_GRID_SELECT, PCB_EDIT_FRAME::OnSelectGrid )

    EVT_CLOSE( PCB_EDIT_FRAME::OnCloseWindow )
    EVT_SIZE( PCB_EDIT_FRAME::OnSize )

    EVT_TOOL( ID_MENU_RECOVER_BOARD_AUTOSAVE, PCB_EDIT_FRAME::Files_io )

    // Menu Files:
    EVT_MENU( ID_MAIN_MENUBAR, PCB_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU( ID_IMPORT_NON_KICAD_BOARD, PCB_EDIT_FRAME::Files_io )
    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, PCB_EDIT_FRAME::OnFileHistory )
    EVT_MENU( ID_FILE_LIST_CLEAR, PCB_EDIT_FRAME::OnClearFileHistory )

    EVT_MENU( ID_GEN_EXPORT_FILE_GENCADFORMAT, PCB_EDIT_FRAME::ExportToGenCAD )
    EVT_MENU( ID_GEN_EXPORT_FILE_VRML, PCB_EDIT_FRAME::OnExportVRML )
    EVT_MENU( ID_GEN_EXPORT_FILE_IDF3, PCB_EDIT_FRAME::OnExportIDF3 )
    EVT_MENU( ID_GEN_EXPORT_FILE_STEP, PCB_EDIT_FRAME::OnExportSTEP )
    EVT_MENU( ID_GEN_EXPORT_FILE_HYPERLYNX, PCB_EDIT_FRAME::OnExportHyperlynx )

    EVT_MENU( ID_MENU_ARCHIVE_MODULES_IN_LIBRARY, PCB_EDIT_FRAME::Process_Special_Functions )
    EVT_MENU( ID_MENU_CREATE_LIBRARY_AND_ARCHIVE_MODULES, PCB_EDIT_FRAME::Process_Special_Functions )

    EVT_MENU( wxID_EXIT, PCB_EDIT_FRAME::OnQuit )
    EVT_MENU( wxID_CLOSE, PCB_EDIT_FRAME::OnQuit )

    // menu Config
    EVT_MENU( ID_PCB_3DSHAPELIB_WIZARD, PCB_EDIT_FRAME::On3DShapeLibWizard )
    EVT_MENU( ID_GRID_SETTINGS, PCB_EDIT_FRAME::OnGridSettings )

    // menu Postprocess
    EVT_MENU( ID_PCB_GEN_CMP_FILE, PCB_EDIT_FRAME::RecreateCmpFileFromBoard )

    // Horizontal toolbar
    EVT_TOOL( ID_GEN_PLOT_SVG, PCB_EDIT_FRAME::ExportSVG )
    EVT_TOOL( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH, PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event )
    EVT_COMBOBOX( ID_TOOLBARH_PCB_SELECT_LAYER, PCB_EDIT_FRAME::Process_Special_Functions )
    EVT_CHOICE( ID_AUX_TOOLBAR_PCB_TRACK_WIDTH, PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event )
    EVT_CHOICE( ID_AUX_TOOLBAR_PCB_VIA_SIZE, PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event )


#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)
    EVT_TOOL( ID_TOOLBARH_PCB_ACTION_PLUGIN_REFRESH, PCB_EDIT_FRAME::OnActionPluginRefresh )
#endif

    // Tracks and vias sizes general options
    EVT_MENU_RANGE( ID_POPUP_PCB_SELECT_WIDTH_START_RANGE, ID_POPUP_PCB_SELECT_WIDTH_END_RANGE,
                    PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event )

    // User interface update event handlers.
    EVT_UPDATE_UI( ID_TOOLBARH_PCB_SELECT_LAYER, PCB_EDIT_FRAME::OnUpdateLayerSelectBox )
    EVT_UPDATE_UI( ID_AUX_TOOLBAR_PCB_TRACK_WIDTH, PCB_EDIT_FRAME::OnUpdateSelectTrackWidth )
    EVT_UPDATE_UI( ID_AUX_TOOLBAR_PCB_VIA_SIZE, PCB_EDIT_FRAME::OnUpdateSelectViaSize )
    EVT_UPDATE_UI_RANGE( ID_POPUP_PCB_SELECT_WIDTH1, ID_POPUP_PCB_SELECT_WIDTH8,
                         PCB_EDIT_FRAME::OnUpdateSelectTrackWidth )
    EVT_UPDATE_UI_RANGE( ID_POPUP_PCB_SELECT_VIASIZE1, ID_POPUP_PCB_SELECT_VIASIZE8,
                         PCB_EDIT_FRAME::OnUpdateSelectViaSize )

    EVT_COMMAND( wxID_ANY, LAYER_WIDGET::EVT_LAYER_COLOR_CHANGE, PCB_EDIT_FRAME::OnLayerColorChange )
END_EVENT_TABLE()


PCB_EDIT_FRAME::PCB_EDIT_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
    PCB_BASE_EDIT_FRAME( aKiway, aParent, FRAME_PCB_EDITOR, wxT( "Pcbnew" ), wxDefaultPosition,
                         wxDefaultSize, KICAD_DEFAULT_DRAWFRAME_STYLE, PCB_EDIT_FRAME_NAME )
{
    m_showBorderAndTitleBlock = true;   // true to display sheet references
    m_SelTrackWidthBox = NULL;
    m_SelViaSizeBox = NULL;
    m_SelLayerBox = NULL;
    m_show_microwave_tools = false;
    m_show_layer_manager_tools = true;
    m_hasAutoSave = true;
    m_microWaveToolBar = NULL;
    m_Layers = nullptr;

    // We don't know what state board was in when it was lasat saved, so we have to
    // assume dirty
    m_ZoneFillsDirty = true;

    m_rotationAngle = 900;
    m_AboutTitle = "Pcbnew";

    // Create GAL canvas
    auto canvas = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_FrameSize,
                                          GetGalDisplayOptions(),
                                          EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO );

    SetCanvas( canvas );

    SetBoard( new BOARD() );

    // Create the PCB_LAYER_WIDGET *after* SetBoard():
    m_Layers = new PCB_LAYER_WIDGET( this, GetCanvas() );

    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( icon_pcbnew_xpm ) );
    SetIcon( icon );

    // LoadSettings() *after* creating m_LayersManager, because LoadSettings()
    // initialize parameters in m_LayersManager
    LoadSettings( config() );

    SetScreen( new PCB_SCREEN( GetPageSettings().GetSizeIU() ) );

    // PCB drawings start in the upper left corner.
    GetScreen()->m_Center = false;

    setupTools();
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateAuxiliaryToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();
    ReCreateMicrowaveVToolbar();

    m_selectionFilterPanel = new PANEL_SELECTION_FILTER( this );

    // Create the infobar
    m_infoBar = new WX_INFOBAR( this, &m_auimgr );

    m_auimgr.SetManagedWindow( this );
    m_auimgr.SetFlags( wxAUI_MGR_LIVE_RESIZE );

    // Horizontal items; layers 4 - 6
    m_auimgr.AddPane( m_mainToolBar,
                      EDA_PANE().HToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_auxiliaryToolBar,
                      EDA_PANE().HToolbar().Name( "AuxToolbar" ).Top().Layer(5) );
    m_auimgr.AddPane( m_messagePanel,
                      EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );
    m_auimgr.AddPane( m_infoBar,
                      EDA_PANE().InfoBar().Name( "InfoBar" ).Top().Layer(1) );

    // Vertical items; layers 1 - 3
    m_auimgr.AddPane( m_optionsToolBar,
                      EDA_PANE().VToolbar().Name( "OptToolbar" ).Left().Layer(3) );

    m_auimgr.AddPane( m_microWaveToolBar,
                      EDA_PANE().VToolbar().Name( "MicrowaveToolbar" ).Right().Layer(2) );
    m_auimgr.AddPane( m_drawToolBar,
                      EDA_PANE().VToolbar().Name( "ToolsToolbar" ).Right().Layer(3) );
    m_auimgr.AddPane( m_Layers,
                      EDA_PANE().Palette().Name( "LayersManager" ).Right().Layer(4)
                      .Caption( _( "Layers Manager" ) ).PaneBorder( false )
                      .MinSize( 80, -1 ).BestSize( m_Layers->GetBestSize() ).Maximize() );
    m_auimgr.AddPane( m_selectionFilterPanel,
                      EDA_PANE().Palette().Name( "SelectionFilter" ).Right().Layer( 4 )
                      .Caption( _( "Selection Filter" ) ).PaneBorder( false ).Position( 2 )
                      .MinSize( 160, -1 ).BestSize( m_selectionFilterPanel->GetBestSize() ) );

    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    m_auimgr.GetPane( "LayersManager" ).Show( m_show_layer_manager_tools );
    m_auimgr.GetPane( "SelectionFilter" ).Show( m_show_layer_manager_tools );
    m_auimgr.GetPane( "MicrowaveToolbar" ).Show( m_show_microwave_tools );

    m_Layers->ReFillRender();   // Update colors in Render after the config is read
    ReFillLayerWidget();        // this is near end and after ReFillRender()
                                // because contents establish size
    syncLayerWidgetLayer();

    // The selection filter doesn't need to grow in the vertical direction when docked
    m_auimgr.GetPane( "SelectionFilter" ).dock_proportion = 0;

    // Call Update() to fix all pane default sizes, especially the "InfoBar" pane before
    // hidding it.
    m_auimgr.Update();

    // We don't want the infobar displayed right away
    m_auimgr.GetPane( "InfoBar" ).Hide();
    m_auimgr.Update();

    GetToolManager()->RunAction( ACTIONS::zoomFitScreen, false );

    m_canvasType = LoadCanvasTypeSetting();

    // Nudge user to switch to OpenGL if they are on Cairo
    if( m_firstRunDialogSetting < 1 )
    {
        if( m_canvasType != EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL )
        {
            wxString msg = _( "KiCad can use your graphics card to give you a smoother "
                              "and faster experience. This option is turned off by "
                              "default since it is not compatible with all computers.\n\n"
                              "Would you like to try enabling graphics acceleration?\n\n"
                              "If you'd like to choose later, select Accelerated Graphics "
                              "in the Preferences menu." );

            wxMessageDialog dlg( this, msg, _( "Enable Graphics Acceleration" ), wxYES_NO );

            dlg.SetYesNoLabels( _( "&Enable Acceleration" ), _( "&No Thanks" ) );

            if( dlg.ShowModal() == wxID_YES )
            {
                // Save Cairo as default in case OpenGL crashes
                saveCanvasTypeSetting( EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO );

                // Switch to OpenGL, which will save the new setting if successful
                GetToolManager()->RunAction( ACTIONS::acceleratedGraphics, true );

                // Switch back to Cairo if OpenGL is not supported
                if( GetCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_NONE )
                    GetToolManager()->RunAction( ACTIONS::standardGraphics, true );
            }
            else
            {
                // If they were on legacy, switch to Cairo
                GetToolManager()->RunAction( ACTIONS::standardGraphics, true );
            }
        }

        m_firstRunDialogSetting = 1;
        SaveSettings( config() );
    }

    InitExitKey();

    GetCanvas()->SwitchBackend( m_canvasType );
    ActivateGalCanvas();

    // Default shutdown reason until a file is loaded
    SetShutdownBlockReason( _( "New PCB file is unsaved" ) );

    // disable Export STEP item if kicad2step does not exist
    wxString strK2S = Pgm().GetExecutablePath();

#ifdef __WXMAC__
    if (strK2S.find( "pcbnew.app" ) != wxNOT_FOUND )
    {
        // On macOS, we have standalone applications inside the main bundle, so we handle that here:
        strK2S += "../../";
    }

    strK2S += "Contents/MacOS/";
#endif

    wxFileName appK2S( strK2S, "kicad2step" );

    #ifdef _WIN32
    appK2S.SetExt( "exe" );
    #endif

    // Ensure the window is on top
    Raise();

//    if( !appK2S.FileExists() )
 //       GetMenuBar()->FindItem( ID_GEN_EXPORT_FILE_STEP )->Enable( false );
}


PCB_EDIT_FRAME::~PCB_EDIT_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    delete m_selectionFilterPanel;
    delete m_Layers;
}


void PCB_EDIT_FRAME::SetBoard( BOARD* aBoard )
{
    if( m_Pcb )
        m_Pcb->ClearProject();

    PCB_BASE_EDIT_FRAME::SetBoard( aBoard );

    aBoard->SetProject( &Prj() );
    aBoard->GetConnectivity()->Build( aBoard );

    // reload the worksheet
    SetPageSettings( aBoard->GetPageSettings() );
}


BOARD_ITEM_CONTAINER* PCB_EDIT_FRAME::GetModel() const
{
    return m_Pcb;
}


void PCB_EDIT_FRAME::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    PCB_BASE_FRAME::SetPageSettings( aPageSettings );

    // Prepare worksheet template
    KIGFX::WS_PROXY_VIEW_ITEM* worksheet;
    worksheet = new KIGFX::WS_PROXY_VIEW_ITEM( IU_PER_MILS ,&m_Pcb->GetPageSettings(),
                                               m_Pcb->GetProject(), &m_Pcb->GetTitleBlock() );
    worksheet->SetSheetName( std::string( GetScreenDesc().mb_str() ) );

    BASE_SCREEN* screen = GetScreen();

    if( screen != NULL )
    {
        worksheet->SetSheetNumber( screen->m_ScreenNumber );
        worksheet->SetSheetCount( screen->m_NumberOfScreens );
    }

    if( auto board = GetBoard() )
        worksheet->SetFileName(  TO_UTF8( board->GetFileName() ) );

    // PCB_DRAW_PANEL_GAL takes ownership of the worksheet
    GetCanvas()->SetWorksheet( worksheet );
}


bool PCB_EDIT_FRAME::IsContentModified()
{
    return GetScreen() && GetScreen()->IsModify() && GetBoard() && !GetBoard()->IsEmpty();
}


bool PCB_EDIT_FRAME::isAutoSaveRequired() const
{
    if( GetScreen() )
        return GetScreen()->IsSave();

    return false;
}


void PCB_EDIT_FRAME::setupTools()
{
    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( m_Pcb, GetCanvas()->GetView(),
                                   GetCanvas()->GetViewControls(), config(), this );
    m_actions = new PCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );
    m_toolManager->RegisterTool( new SELECTION_TOOL );
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new PCBNEW_PICKER_TOOL );
    m_toolManager->RegisterTool( new ROUTER_TOOL );
    m_toolManager->RegisterTool( new LENGTH_TUNER_TOOL );
    m_toolManager->RegisterTool( new EDIT_TOOL );
    m_toolManager->RegisterTool( new GLOBAL_EDIT_TOOL );
    m_toolManager->RegisterTool( new PAD_TOOL );
    m_toolManager->RegisterTool( new DRAWING_TOOL );
    m_toolManager->RegisterTool( new POINT_EDITOR );
    m_toolManager->RegisterTool( new PCBNEW_CONTROL );
    m_toolManager->RegisterTool( new PCB_EDITOR_CONTROL );
    m_toolManager->RegisterTool( new PCB_INSPECTION_TOOL );
    m_toolManager->RegisterTool( new ALIGN_DISTRIBUTE_TOOL );
    m_toolManager->RegisterTool( new MICROWAVE_TOOL );
    m_toolManager->RegisterTool( new POSITION_RELATIVE_TOOL );
    m_toolManager->RegisterTool( new ZONE_FILLER_TOOL );
    m_toolManager->RegisterTool( new AUTOPLACE_TOOL );
    m_toolManager->RegisterTool( new DRC );
    m_toolManager->RegisterTool( new PCB_VIEWER_TOOLS );
    m_toolManager->InitTools();

    // Run the selection tool, it is supposed to be always active
    m_toolManager->InvokeTool( "pcbnew.InteractiveSelection" );
}


void PCB_EDIT_FRAME::ReFillLayerWidget()
{
    wxWindowUpdateLocker no_update( m_Layers );
    m_Layers->ReFill();

    wxAuiPaneInfo& lyrs = m_auimgr.GetPane( m_Layers );

    wxSize bestz = m_Layers->GetBestSize();

    lyrs.MinSize( bestz );
    lyrs.BestSize( bestz );
    lyrs.FloatingSize( bestz );

    if( lyrs.IsDocked() )
        m_auimgr.Update();
    else
        m_Layers->SetSize( bestz );
}


void PCB_EDIT_FRAME::OnQuit( wxCommandEvent& event )
{
    if( event.GetId() == wxID_EXIT )
        Kiway().OnKiCadExit();

    if( event.GetId() == wxID_CLOSE || Kiface().IsSingle() )
        Close( false );
}


void PCB_EDIT_FRAME::RecordDRCExclusions()
{
    BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();
    bds.m_DrcExclusions.clear();

    for( MARKER_PCB* marker : GetBoard()->Markers() )
    {
        if( marker->IsExcluded() )
            bds.m_DrcExclusions.insert( marker->Serialize() );
    }
}


void PCB_EDIT_FRAME::ResolveDRCExclusions()
{
    BOARD_DESIGN_SETTINGS& bds = GetBoard()->GetDesignSettings();

    for( MARKER_PCB* marker : GetBoard()->Markers() )
    {
        auto i = bds.m_DrcExclusions.find( marker->Serialize() );

        if( i != bds.m_DrcExclusions.end() )
        {
            marker->SetExcluded( true );
            bds.m_DrcExclusions.erase( i );
        }
    }

    BOARD_COMMIT commit( this );

    for( const wxString& exclusionData : bds.m_DrcExclusions )
    {
        MARKER_PCB* marker = MARKER_PCB::Deserialize( exclusionData );

        if( marker )
        {
            marker->SetExcluded( true );
            commit.Add( marker );
        }
    }

    bds.m_DrcExclusions.clear();

    commit.Push( wxEmptyString, false, false );
}


void PCB_EDIT_FRAME::OnCloseWindow( wxCloseEvent& aEvent )
{
    // Shutdown blocks must be determined and vetoed as early as possible
    if( SupportsShutdownBlockReason() && aEvent.GetId() == wxEVT_QUERY_END_SESSION
            && IsContentModified() )
    {
        aEvent.Veto();
        return;
    }

    // First close the DRC dialog.  For some reason, if the board editor frame is destroyed
    // when the DRC dialog currently open, Pcbnew crashes, at least on Windows.
    DIALOG_DRC* open_dlg = static_cast<DIALOG_DRC*>(
                                        wxWindow::FindWindowByName( DIALOG_DRC_WINDOW_NAME ) );

    if( open_dlg )
        open_dlg->Close( true );

    if( IsContentModified() )
    {
        wxFileName fileName = GetBoard()->GetFileName();
        wxString msg = _( "Save changes to \"%s\" before closing?" );

        if( !HandleUnsavedChanges( this, wxString::Format( msg, fileName.GetFullName() ),
                                   [&]()->bool { return Files_io_from_id( ID_SAVE_BOARD ); } ) )
        {
            aEvent.Veto();
            return;
        }
    }

    // On Windows 7 / 32 bits, on OpenGL mode only, Pcbnew crashes
    // when closing this frame if a footprint was selected, and the footprint editor called
    // to edit this footprint, and when closing pcbnew if this footprint is still selected
    // See https://bugs.launchpad.net/kicad/+bug/1655858
    // I think this is certainly a OpenGL event fired after frame deletion, so this workaround
    // avoid the crash (JPC)
    GetCanvas()->SetEvtHandlerEnabled( false );

    GetCanvas()->StopDrawing();

    // Delete the auto save file if it exists.
    wxFileName fn = GetBoard()->GetFileName();

    // Auto save file name is the normal file name prefixed with 'GetAutoSaveFilePrefix()'.
    fn.SetName( GetAutoSaveFilePrefix() + fn.GetName() );

    // When the auto save feature does not have write access to the board file path, it falls
    // back to a platform specific user temporary file path.
    if( !fn.IsOk() || !fn.IsDirWritable() )
        fn.SetPath( wxFileName::GetTempDir() );

    wxLogTrace( traceAutoSave, "Deleting auto save file <" + fn.GetFullPath() + ">" );

    // Remove the auto save file on a normal close of Pcbnew.
    if( fn.FileExists() && !wxRemoveFile( fn.GetFullPath() ) )
    {
        wxString msg = wxString::Format( _( "The auto save file \"%s\" could not be removed!" ),
                                         fn.GetFullPath() );
        wxMessageBox( msg, Pgm().App().GetAppName(), wxOK | wxICON_ERROR, this );
    }

    // Make sure local settings are persisted
    SaveProjectSettings();

    // Do not show the layer manager during closing to avoid flicker
    // on some platforms (Windows) that generate useless redraw of items in
    // the Layer Manger
    if( m_show_layer_manager_tools )
        m_auimgr.GetPane( "LayersManager" ).Show( false );

    // Delete board structs and undo/redo lists, to avoid crash on exit
    // when deleting some structs (mainly in undo/redo lists) too late
    Clear_Pcb( false, true );

    // do not show the window because ScreenPcb will be deleted and we do not
    // want any paint event
    Show( false );

    // Close frame:
    aEvent.Skip();
}


void PCB_EDIT_FRAME::ActivateGalCanvas()
{
    PCB_BASE_EDIT_FRAME::ActivateGalCanvas();
    GetCanvas()->UpdateColors();
    GetCanvas()->Refresh();
}


void PCB_EDIT_FRAME::ShowBoardSetupDialog( const wxString& aInitialPage, const wxString& aErrorMsg,
                                           int aErrorCtrlId, int aErrorLine, int aErrorCol )
{
    // Make sure everything's up-to-date
    GetBoard()->BuildListOfNets();

    DIALOG_BOARD_SETUP dlg( this );

    if( !aInitialPage.IsEmpty() )
        dlg.SetInitialPage( aInitialPage, wxEmptyString );

    if( !aErrorMsg.IsEmpty() )
        dlg.SetError( aErrorMsg, aInitialPage, aErrorCtrlId, aErrorLine, aErrorCol );

    if( dlg.ShowQuasiModal() == wxID_OK )
    {
        Prj().GetProjectFile().NetSettings().ResolveNetClassAssignments();

        GetBoard()->SynchronizeNetsAndNetClasses();
        GetBoard()->GetDesignSettings().SetCurrentNetClass( NETCLASS::Default );
        SaveProjectSettings();

        UpdateUserInterface();
        ReCreateAuxiliaryToolbar();

        for( auto module : GetBoard()->Modules() )
            GetCanvas()->GetView()->Update( module );

        GetCanvas()->Refresh();

        m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

        //this event causes the routing tool to reload its design rules information
        TOOL_EVENT toolEvent( TC_COMMAND, TA_MODEL_CHANGE, AS_ACTIVE );
        toolEvent.SetHasPosition( false );
        m_toolManager->ProcessEvent( toolEvent );

        OnModify();
    }
}


void PCB_EDIT_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    PCB_BASE_FRAME::LoadSettings( aCfg );

    PCBNEW_SETTINGS* cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxASSERT( cfg );

    if( cfg )
    {
        m_rotationAngle            = cfg->m_RotationAngle;
        g_DrawDefaultLineThickness = Millimeter2iu( cfg->m_PlotLineWidth );
        m_show_microwave_tools     = cfg->m_AuiPanels.show_microwave_tools;
        m_show_layer_manager_tools = cfg->m_AuiPanels.show_layer_manager;
        m_showPageLimits           = cfg->m_ShowPageLimits;
    }
}


void PCB_EDIT_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    PCB_BASE_FRAME::SaveSettings( aCfg );

    auto cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxASSERT( cfg );

    if( cfg )
    {
        cfg->m_RotationAngle                  = m_rotationAngle;
        cfg->m_PlotLineWidth                  = Iu2Millimeter( g_DrawDefaultLineThickness );
        cfg->m_AuiPanels.show_microwave_tools = m_show_microwave_tools;
        cfg->m_AuiPanels.show_layer_manager   = m_show_layer_manager_tools;
        cfg->m_ShowPageLimits                 = m_showPageLimits;
    }
}


COLOR4D PCB_EDIT_FRAME::GetGridColor()
{
    return GetColorSettings()->GetColor( LAYER_GRID );
}


void PCB_EDIT_FRAME::SetGridColor( COLOR4D aColor )
{

    GetColorSettings()->SetColor( LAYER_GRID, aColor );
    GetCanvas()->GetGAL()->SetGridColor( aColor );
}


void PCB_EDIT_FRAME::SetActiveLayer( PCB_LAYER_ID aLayer )
{
    PCB_BASE_FRAME::SetActiveLayer( aLayer );

    syncLayerWidgetLayer();

    m_toolManager->RunAction( PCB_ACTIONS::layerChanged );  // notify other tools
    GetCanvas()->SetFocus();                             // allow capture of hotkeys
    GetCanvas()->SetHighContrastLayer( aLayer );
    GetCanvas()->Refresh();
}


void PCB_EDIT_FRAME::onBoardLoaded()
{
    UpdateTitle();

    wxFileName fn = GetBoard()->GetFileName();
    m_infoBar->Dismiss();

    // Display a warning that the file is read only
    if( fn.FileExists() && !fn.IsFileWritable() )
    {
        m_infoBar->RemoveAllButtons();
        m_infoBar->AddCloseButton();
        m_infoBar->ShowMessage( "Board file is read only.", wxICON_WARNING );
    }

    // Re-create layers manager based on layer info in board
    ReFillLayerWidget();
    ReCreateLayerBox();

    // Sync layer and item visibility
    syncLayerVisibilities();
    syncLayerWidgetLayer();
    SyncRenderStates();

    SetElementVisibility( LAYER_RATSNEST, GetDisplayOptions().m_ShowGlobalRatsnest );

    // Update the tracks / vias available sizes list:
    ReCreateAuxiliaryToolbar();

    // Display the loaded board:
    Zoom_Automatique( false );

    Refresh();

    SetMsgPanel( GetBoard() );
    SetStatusText( wxEmptyString );

    m_toolManager->GetTool<DRC>()->LoadRules();

    SetShutdownBlockReason( _( "PCB file changes are unsaved" ) );
}


void PCB_EDIT_FRAME::syncLayerWidgetLayer()
{
    m_Layers->SelectLayer( GetActiveLayer() );
    m_Layers->OnLayerSelected();
}


void PCB_EDIT_FRAME::SyncRenderStates()
{
    m_Layers->ReFillRender();
}


void PCB_EDIT_FRAME::syncLayerVisibilities()
{
    m_Layers->SyncLayerVisibilities();
    GetCanvas()->SyncLayersVisibility( m_Pcb );
}


void PCB_EDIT_FRAME::OnUpdateLayerAlpha( wxUpdateUIEvent & )
{
    m_Layers->SyncLayerAlphaIndicators();
}


bool PCB_EDIT_FRAME::IsElementVisible( GAL_LAYER_ID aElement ) const
{
    return GetBoard()->IsElementVisible( aElement );
}


void PCB_EDIT_FRAME::SetElementVisibility( GAL_LAYER_ID aElement, bool aNewState )
{
    // Force the RATSNEST visible
    if( aElement == LAYER_RATSNEST )
        GetCanvas()->GetView()->SetLayerVisible( aElement, true );
    else
        GetCanvas()->GetView()->SetLayerVisible( aElement , aNewState );

    GetBoard()->SetElementVisibility( aElement, aNewState );
    m_Layers->SetRenderState( aElement, aNewState );
}


void PCB_EDIT_FRAME::SetVisibleAlls()
{
    GetBoard()->SetVisibleAlls();

    for( GAL_LAYER_ID ii = GAL_LAYER_ID_START; ii < GAL_LAYER_ID_BITMASK_END; ++ii )
        m_Layers->SetRenderState( ii, true );
}


void PCB_EDIT_FRAME::ShowChangedLanguage()
{
    // call my base class
    PCB_BASE_EDIT_FRAME::ShowChangedLanguage();

    // update the layer manager
    m_Layers->Freeze();

    wxAuiPaneInfo& pane_info = m_auimgr.GetPane( m_Layers );
    pane_info.Caption( _( "Visibles" ) );
    m_auimgr.Update();

    m_Layers->SetLayersManagerTabsText();
    ReFillLayerWidget();
    // m_Layers->ReFillRender();  // SyncRenderStates() does this

    // upate the layer widget to match board visibility states, both layers and render columns.
    syncLayerVisibilities();
    syncLayerWidgetLayer();
    SyncRenderStates();

    m_Layers->Thaw();

    // pcbnew-specific toolbars
    ReCreateMicrowaveVToolbar();
}


wxString PCB_EDIT_FRAME::GetLastPath( LAST_PATH_TYPE aType )
{
    PROJECT_FILE& project = Prj().GetProjectFile();

    if( project.m_PcbLastPath[ aType ].IsEmpty() )
        return wxEmptyString;

    wxFileName absoluteFileName = project.m_PcbLastPath[ aType ];
    wxFileName pcbFileName = GetBoard()->GetFileName();

    absoluteFileName.MakeAbsolute( pcbFileName.GetPath() );
    return absoluteFileName.GetFullPath();
}


void PCB_EDIT_FRAME::SetLastPath( LAST_PATH_TYPE aType, const wxString& aLastPath )
{
    PROJECT_FILE& project = Prj().GetProjectFile();

    wxFileName relativeFileName = aLastPath;
    wxFileName pcbFileName = GetBoard()->GetFileName();

    relativeFileName.MakeRelativeTo( pcbFileName.GetPath() );

    if( relativeFileName.GetFullPath() != project.m_PcbLastPath[ aType ] )
    {
        project.m_PcbLastPath[ aType ] = relativeFileName.GetFullPath();
        SaveProjectSettings();
    }
}


void PCB_EDIT_FRAME::OnModify( )
{
    PCB_BASE_FRAME::OnModify();

    Update3DView( false );

    m_ZoneFillsDirty = true;
}


void PCB_EDIT_FRAME::ExportSVG( wxCommandEvent& event )
{
    InvokeExportSVG( this, GetBoard() );
}


void PCB_EDIT_FRAME::UpdateTitle()
{
    wxFileName fileName = GetBoard()->GetFileName();
    wxString fileinfo;

    if( fileName.IsOk() && fileName.FileExists() )
        fileinfo = fileName.IsFileWritable() ? wxString( wxEmptyString ) : _( " [Read Only]" );
    else
        fileinfo = _( " [Unsaved]" );

    SetTitle( wxString::Format( _( "Pcbnew" ) + wxT( " \u2014 %s%s" ),
                                fileName.GetFullPath(),
                                fileinfo ) );
}


void PCB_EDIT_FRAME::UpdateUserInterface()
{
    // Update the layer manager and other widgets from the board setup
    // (layer and items visibility, colors ...)

    // Rebuild list of nets (full ratsnest rebuild)
    GetBoard()->BuildConnectivity();
    Compile_Ratsnest( true );

    // Update info shown by the horizontal toolbars
    ReCreateLayerBox();

    // Update the layer manager
    m_Layers->Freeze();
    ReFillLayerWidget();
    // m_Layers->ReFillRender();  // SyncRenderStates() does this

    // upate the layer widget to match board visibility states, both layers and render columns.
    syncLayerVisibilities();
    syncLayerWidgetLayer();
    SyncRenderStates();

    m_Layers->Thaw();
}


#if defined( KICAD_SCRIPTING_WXPYTHON )

void PCB_EDIT_FRAME::ScriptingConsoleEnableDisable()
{
    wxWindow * pythonPanelFrame = findPythonConsole();
    bool pythonPanelShown = true;

    if( pythonPanelFrame == NULL )
        pythonPanelFrame = CreatePythonShellWindow( this, pythonConsoleNameId() );
    else
        pythonPanelShown = ! pythonPanelFrame->IsShown();

    if( pythonPanelFrame )
        pythonPanelFrame->Show( pythonPanelShown );
    else
        wxMessageBox( wxT( "Error: unable to create the Python Console" ) );
}
#endif


void PCB_EDIT_FRAME::OnLayerColorChange( wxCommandEvent& aEvent )
{
    ReCreateLayerBox();
}


void PCB_EDIT_FRAME::SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType )
{
    // switches currently used canvas (Cairo / OpenGL).
    PCB_BASE_FRAME::SwitchCanvas( aCanvasType );

    // The base class method *does not* reinitialize the layers manager. We must upate the
    // layer widget to match board visibility states, both layers and render columns.
    syncLayerVisibilities();
    syncLayerWidgetLayer();
    SyncRenderStates();
}


void PCB_EDIT_FRAME::ToPlotter( int aID )
{
    PCB_PLOT_PARAMS plotSettings = GetPlotSettings();

    switch( aID )
    {
    case ID_GEN_PLOT_GERBER:
        plotSettings.SetFormat( PLOT_FORMAT::GERBER );
        break;
    case ID_GEN_PLOT_DXF:
        plotSettings.SetFormat( PLOT_FORMAT::DXF );
        break;
    case ID_GEN_PLOT_HPGL:
        plotSettings.SetFormat( PLOT_FORMAT::HPGL );
        break;
    case ID_GEN_PLOT_PDF:
        plotSettings.SetFormat( PLOT_FORMAT::PDF );
        break;
    case ID_GEN_PLOT_PS:
        plotSettings.SetFormat( PLOT_FORMAT::POST );
        break;
    case ID_GEN_PLOT:        /* keep the previous setup */                   break;
    default:
        wxFAIL_MSG( "ToPlotter(): unexpected plot type" ); break;
        break;
    }

    SetPlotSettings( plotSettings );

    // Force rebuild the dialog if currently open because the old dialog can be not up to date
    // if the board (or units) has changed
    wxWindow* dlg =  wxWindow::FindWindowByName( DLG_WINDOW_NAME );

    if( dlg )
        dlg->Destroy();

    dlg = new DIALOG_PLOT( this );
    dlg->Show( true );
}


bool PCB_EDIT_FRAME::SetCurrentNetClass( const wxString& aNetClassName )
{
    bool change = GetDesignSettings().SetCurrentNetClass( aNetClassName );

    if( change )
        ReCreateAuxiliaryToolbar();

    return change;
}


bool PCB_EDIT_FRAME::FetchNetlistFromSchematic( NETLIST& aNetlist, FETCH_NETLIST_MODE aMode )
{
    if( Kiface().IsSingle() )
    {
        DisplayError( this, _( "Cannot update the PCB because Pcbnew is opened in stand-alone "
                               "mode. In order to create or update PCBs from schematics, you "
                               "must launch the KiCad project manager and create a project." ) );
        return false;
    }

    // Update PCB requires a netlist. Therefore the schematic editor must be running
    // If this is not the case, open the schematic editor
    KIWAY_PLAYER* frame = Kiway().Player( FRAME_SCH, true );

    if( !frame->IsShown() )
    {
        wxFileName fn( Prj().GetProjectPath(), Prj().GetProjectName(),
                KiCadSchematicFileExtension );

        // Maybe the file hasn't been converted to the new s-expression file format so
        // see if the legacy schematic file is still in play.
        if( !fn.FileExists() )
        {
            fn.SetExt( LegacySchematicFileExtension );

            if( !fn.FileExists() )
            {
                DisplayError( this, _( "The schematic for this board cannot be found." ) );
                return false;
            }
        }

        frame->OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ) );

        // we show the schematic editor frame, because do not show is seen as
        // a not yet opened schematic by Kicad manager, which is not the case
        frame->Show( true );

        // bring ourselves back to the front
        Raise();
    }

    std::string payload;

    if( aMode == NO_ANNOTATION )
        payload = "no-annotate";
    else if( aMode == QUIET_ANNOTATION )
        payload = "quiet-annotate";

    Kiway().ExpressMail( FRAME_SCH, MAIL_SCH_GET_NETLIST, payload, this );

    try
    {
        auto lineReader = new STRING_LINE_READER( payload, _( "Eeschema netlist" ) );
        KICAD_NETLIST_READER netlistReader( lineReader, &aNetlist );
        netlistReader.LoadNetlist();
    }
    catch( const IO_ERROR& )
    {
        assert( false ); // should never happen
        return false;
    }

    return true;
}


void PCB_EDIT_FRAME::DoUpdatePCBFromNetlist( NETLIST& aNetlist, bool aUseTimestamps )
{
    BOARD_NETLIST_UPDATER updater( this, GetBoard() );
    updater.SetLookupByTimestamp( aUseTimestamps );
    updater.SetDeleteUnusedComponents( false );
    updater.SetReplaceFootprints( true );
    updater.SetDeleteSinglePadNets( false );
    updater.SetWarnPadNoNetInNetlist( false );
    updater.UpdateNetlist( aNetlist );
}


void PCB_EDIT_FRAME::RunEeschema()
{
    wxString   msg;
    wxFileName schfn( Prj().GetProjectPath(), Prj().GetProjectName(),
                      KiCadSchematicFileExtension );

    if( !schfn.FileExists() )
    {
        msg.Printf( _( "Schematic file \"%s\" not found." ), schfn.GetFullPath() );
        wxMessageBox( msg, _( "KiCad Error" ), wxOK | wxICON_ERROR, this );
        return;
    }

    if( Kiface().IsSingle() )
    {
        wxString filename = wxT( "\"" ) + schfn.GetFullPath( wxPATH_NATIVE ) + wxT( "\"" );
        ExecuteFile( this, EESCHEMA_EXE, filename );
    }
    else
    {
        KIWAY_PLAYER* frame = Kiway().Player( FRAME_SCH, false );

        // Please: note: DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::initBuffers() calls
        // Kiway.Player( FRAME_SCH, true )
        // therefore, the schematic editor is sometimes running, but the schematic project
        // is not loaded, if the library editor was called, and the dialog field editor was used.
        // On linux, it happens the first time the schematic editor is launched, if
        // library editor was running, and the dialog field editor was open
        // On Windows, it happens always after the library editor was called,
        // and the dialog field editor was used
        if( !frame )
        {
            try
            {
                frame = Kiway().Player( FRAME_SCH, true );
            }
            catch( const IO_ERROR& err )
            {
                wxMessageBox( _( "Eeschema failed to load:\n" ) + err.What(),
                              _( "KiCad Error" ), wxOK | wxICON_ERROR, this );
                return;
            }
        }

        if( !frame->IsShown() ) // the frame exists, (created by the dialog field editor)
                                // but no project loaded.
        {
            frame->OpenProjectFiles( std::vector<wxString>( 1, schfn.GetFullPath() ) );
            frame->Show( true );
        }

        // On Windows, Raise() does not bring the window on screen, when iconized or not shown
        // On linux, Raise() brings the window on screen, but this code works fine
        if( frame->IsIconized() )
        {
            frame->Iconize( false );
            // If an iconized frame was created by Pcbnew, Iconize( false ) is not enough
            // to show the frame at its normal size: Maximize should be called.
            frame->Maximize( false );
        }

        frame->Raise();
    }
}


void PCB_EDIT_FRAME::PythonPluginsReload()
{
    // Reload Python plugins if they are newer than
    // the already loaded, and load new plugins
#if defined(KICAD_SCRIPTING)
    //Reload plugin list: reload Python plugins if they are newer than
    // the already loaded, and load new plugins
    PythonPluginsReloadBase();

    #if defined(KICAD_SCRIPTING_ACTION_MENU)
        // Action plugins can be modified, therefore the plugins menu
        // must be updated:
        ReCreateMenuBar();
        // Recreate top toolbar to add action plugin buttons
        ReCreateHToolbar();
    #endif
#endif
}


void PCB_EDIT_FRAME::InstallFootprintPropertiesDialog( MODULE* Module )
{
    if( Module == NULL )
        return;

    DIALOG_FOOTPRINT_BOARD_EDITOR* dlg = new DIALOG_FOOTPRINT_BOARD_EDITOR( this, Module );

    int retvalue = dlg->ShowModal();
    /* retvalue =
     *  FP_PRM_EDITOR_RETVALUE::PRM_EDITOR_WANT_UPDATE_FP if update footprint
     *  FP_PRM_EDITOR_RETVALUE::PRM_EDITOR_WANT_EXCHANGE_FP if change footprint
     *  FP_PRM_EDITOR_RETVALUE::PRM_EDITOR_WANT_MODEDIT for a goto editor command
     *  FP_PRM_EDITOR_RETVALUE::PRM_EDITOR_EDIT_OK for normal edit
     */

    dlg->Close();
    dlg->Destroy();

    if( retvalue == DIALOG_FOOTPRINT_BOARD_EDITOR::PRM_EDITOR_EDIT_OK )
    {
        // If something edited, push a refresh request
        GetCanvas()->Refresh();
    }
    else if( retvalue == DIALOG_FOOTPRINT_BOARD_EDITOR::PRM_EDITOR_EDIT_BOARD_FOOTPRINT )
    {
        auto editor = (FOOTPRINT_EDIT_FRAME*) Kiway().Player( FRAME_FOOTPRINT_EDITOR, true );

        editor->Load_Module_From_BOARD( Module );

        editor->Show( true );
        editor->Raise();        // Iconize( false );
    }

    else if( retvalue == DIALOG_FOOTPRINT_BOARD_EDITOR::PRM_EDITOR_EDIT_LIBRARY_FOOTPRINT )
    {
        auto editor = (FOOTPRINT_EDIT_FRAME*) Kiway().Player( FRAME_FOOTPRINT_EDITOR, true );

        editor->LoadModuleFromLibrary( Module->GetFPID() );

        editor->Show( true );
        editor->Raise();        // Iconize( false );
    }

    else if( retvalue == DIALOG_FOOTPRINT_BOARD_EDITOR::PRM_EDITOR_WANT_UPDATE_FP )
    {
        InstallExchangeModuleFrame( Module, true, true );
    }

    else if( retvalue == DIALOG_FOOTPRINT_BOARD_EDITOR::PRM_EDITOR_WANT_EXCHANGE_FP )
    {
        InstallExchangeModuleFrame( Module, false, true );
    }
}


int PCB_EDIT_FRAME::InstallExchangeModuleFrame( MODULE* aModule, bool updateMode,
                                                bool selectedMode )
{
    DIALOG_EXCHANGE_FOOTPRINTS dialog( this, aModule, updateMode, selectedMode );

    return dialog.ShowQuasiModal();
}


void PCB_EDIT_FRAME::CommonSettingsChanged( bool aEnvVarsChanged )
{
    PCB_BASE_EDIT_FRAME::CommonSettingsChanged( aEnvVarsChanged );

    ReCreateMicrowaveVToolbar();

    Layout();
    SendSizeEvent();
}


void PCB_EDIT_FRAME::LockModule( MODULE* aModule, bool aLocked )
{
    const wxString ModulesMaskSelection = wxT( "*" );
    if( aModule )
    {
        aModule->SetLocked( aLocked );
        SetMsgPanel( aModule );
        OnModify();
    }
    else
    {
        for( auto mod : GetBoard()->Modules() )
        {
            if( WildCompareString( ModulesMaskSelection, mod->GetReference() ) )
            {
                mod->SetLocked( aLocked );
                OnModify();
            }
        }
    }
}


bool ExportBoardToHyperlynx( BOARD* aBoard, const wxFileName& aPath );


void PCB_EDIT_FRAME::OnExportHyperlynx( wxCommandEvent& event )
{
    wxString    wildcard =  wxT( "*.hyp" );
    wxFileName  fn = GetBoard()->GetFileName();

    fn.SetExt( wxT("hyp") );

    wxFileDialog dlg( this, _( "Export Hyperlynx Layout" ), fn.GetPath(), fn.GetFullName(),
                      wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return;

    fn = dlg.GetPath();

    // always enforce filename extension, user may not have entered it.
    fn.SetExt( wxT( "hyp" ) );

    ExportBoardToHyperlynx( GetBoard(), fn );
}


wxString PCB_EDIT_FRAME::GetCurrentFileName() const
{
    return GetBoard()->GetFileName();
}
