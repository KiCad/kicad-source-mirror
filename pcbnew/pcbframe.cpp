/******************************************/
/* pcbframe.cpp - PCB editor main window. */
/******************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "pcbstruct.h"      // enum PCB_VISIBLE
#include "collectors.h"
#include "bitmaps.h"
#include "protos.h"
#include "pcbnew_id.h"
#include "drc_stuff.h"
#include "3d_viewer.h"
#include "kbool/include/kbool/booleng.h"

#include "dialog_design_rules.h"

// Keys used in read/write config
#define PCB_MAGNETIC_PADS_OPT   wxT( "PcbMagPadOpt" )
#define PCB_MAGNETIC_TRACKS_OPT wxT( "PcbMagTrackOpt" )
#define SHOW_MICROWAVE_TOOLS    wxT( "ShowMicrowaveTools" )
#define SHOW_LAYER_MANAGER_TOOLS    wxT( "ShowLayerManagerTools" )

BEGIN_EVENT_TABLE( WinEDA_PcbFrame, WinEDA_BasePcbFrame )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT_SERV, WinEDA_PcbFrame::OnSockRequestServer )
    EVT_SOCKET( ID_EDA_SOCKET_EVENT, WinEDA_PcbFrame::OnSockRequest )

    EVT_KICAD_CHOICEBOX( ID_ON_ZOOM_SELECT, WinEDA_PcbFrame::OnSelectZoom )
    EVT_KICAD_CHOICEBOX( ID_ON_GRID_SELECT, WinEDA_PcbFrame::OnSelectGrid )

    EVT_CLOSE( WinEDA_PcbFrame::OnCloseWindow )
    EVT_SIZE( WinEDA_PcbFrame::OnSize )

    EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, WinEDA_PcbFrame::OnZoom )

    EVT_TOOL( ID_LOAD_FILE, WinEDA_PcbFrame::Files_io )
    EVT_TOOL( ID_MENU_READ_LAST_SAVED_VERSION_BOARD, WinEDA_PcbFrame::Files_io )
    EVT_TOOL( ID_MENU_RECOVER_BOARD, WinEDA_PcbFrame::Files_io )
    EVT_TOOL( ID_NEW_BOARD, WinEDA_PcbFrame::Files_io )
    EVT_TOOL( ID_SAVE_BOARD, WinEDA_PcbFrame::Files_io )
    EVT_TOOL( ID_OPEN_MODULE_EDITOR,
              WinEDA_PcbFrame::Process_Special_Functions )

    // Menu Files:
    EVT_MENU( ID_MAIN_MENUBAR, WinEDA_PcbFrame::Process_Special_Functions )

    EVT_MENU( ID_LOAD_FILE, WinEDA_PcbFrame::Files_io )
    EVT_MENU( ID_NEW_BOARD, WinEDA_PcbFrame::Files_io )
    EVT_MENU( ID_SAVE_BOARD, WinEDA_PcbFrame::Files_io )
    EVT_MENU( ID_APPEND_FILE, WinEDA_PcbFrame::Files_io )
    EVT_MENU( ID_SAVE_BOARD_AS, WinEDA_PcbFrame::Files_io )
    EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, WinEDA_PcbFrame::OnFileHistory )

    EVT_MENU( ID_GEN_PLOT, WinEDA_PcbFrame::ToPlotter )

    EVT_MENU( ID_GEN_EXPORT_SPECCTRA, WinEDA_PcbFrame::ExportToSpecctra )
    EVT_MENU( ID_GEN_EXPORT_FILE_GENCADFORMAT, WinEDA_PcbFrame::ExportToGenCAD )
    EVT_MENU( ID_GEN_EXPORT_FILE_MODULE_REPORT,
              WinEDA_PcbFrame::GenModuleReport )

    EVT_MENU( ID_GEN_IMPORT_SPECCTRA_SESSION,
              WinEDA_PcbFrame::ImportSpecctraSession )
    EVT_MENU( ID_GEN_IMPORT_SPECCTRA_DESIGN,
              WinEDA_PcbFrame::ImportSpecctraDesign )

    EVT_MENU( ID_MENU_ARCHIVE_NEW_MODULES,
              WinEDA_PcbFrame::Process_Special_Functions )
    EVT_MENU( ID_MENU_ARCHIVE_ALL_MODULES,
              WinEDA_PcbFrame::Process_Special_Functions )

    EVT_MENU( wxID_EXIT, WinEDA_PcbFrame::OnQuit )

    // menu Config
    EVT_MENU_RANGE( ID_CONFIG_AND_PREFERENCES_START,
                    ID_CONFIG_AND_PREFERENCES_END,
                    WinEDA_PcbFrame::Process_Config )

    EVT_MENU( ID_COLORS_SETUP, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_OPTIONS_SETUP, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_PCB_LAYERS_SETUP, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_PCB_MASK_CLEARANCE, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_PCB_DRAWINGS_WIDTHS_SETUP, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_PCB_PAD_SETUP, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_CONFIG_SAVE, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_CONFIG_READ, WinEDA_PcbFrame::Process_Config )
    EVT_MENU( ID_PCB_DISPLAY_OPTIONS_SETUP,
              WinEDA_PcbFrame::InstallDisplayOptionsDialog )

    EVT_MENU( ID_PCB_USER_GRID_SETUP,
              WinEDA_PcbFrame::Process_Special_Functions )

    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
                    WinEDA_DrawFrame::SetLanguage )

    // menu Postprocess
    EVT_MENU( ID_PCB_GEN_POS_MODULES_FILE, WinEDA_PcbFrame::GenModulesPosition )
    EVT_MENU( ID_PCB_GEN_DRILL_FILE, WinEDA_PcbFrame::InstallDrillFrame )
    EVT_MENU( ID_PCB_GEN_CMP_FILE, WinEDA_PcbFrame::RecreateCmpFileFromBoard )
    EVT_MENU( ID_PCB_GEN_BOM_FILE_FROM_BOARD,
              WinEDA_PcbFrame::RecreateBOMFileFromBoard )

    // menu Miscellaneous
    EVT_MENU( ID_MENU_LIST_NETS, WinEDA_PcbFrame::ListNetsAndSelect )
    EVT_MENU( ID_PCB_GLOBAL_DELETE, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_MENU( ID_MENU_PCB_CLEAN, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_MENU( ID_MENU_PCB_SWAP_LAYERS,
              WinEDA_PcbFrame::Process_Special_Functions )

    // Menu Help
    EVT_MENU( ID_GENERAL_HELP, WinEDA_DrawFrame::GetKicadHelp )
    EVT_MENU( ID_KICAD_ABOUT, WinEDA_BasicFrame::GetKicadAbout )

    // Menu 3D Frame
    EVT_MENU( ID_MENU_PCB_SHOW_3D_FRAME, WinEDA_PcbFrame::Show3D_Frame )

    // Menu Get Design Rules Editor
    EVT_MENU( ID_MENU_PCB_SHOW_DESIGN_RULES_DIALOG,
              WinEDA_PcbFrame::ShowDesignRulesEditor )

    // Horizontal toolbar
    EVT_TOOL( ID_TO_LIBRARY, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_SHEET_SET, WinEDA_DrawFrame::Process_PageSettings )
    EVT_TOOL( wxID_CUT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( wxID_COPY, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( wxID_PASTE, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( wxID_UNDO, WinEDA_PcbFrame::GetBoardFromUndoList )
    EVT_TOOL( wxID_REDO, WinEDA_PcbFrame::GetBoardFromRedoList )
    EVT_TOOL( ID_GEN_PRINT, WinEDA_PcbFrame::ToPrinter )
    EVT_TOOL( ID_GEN_PLOT_SVG, WinEDA_DrawFrame::SVG_Print )
    EVT_TOOL( ID_GEN_PLOT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_FIND_ITEMS, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_GET_NETLIST, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_DRC_CONTROL, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR,
              WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH,
              WinEDA_PcbFrame::Tracks_and_Vias_Size_Event )
    EVT_KICAD_CHOICEBOX( ID_TOOLBARH_PCB_SELECT_LAYER,
                         WinEDA_PcbFrame::Process_Special_Functions )
    EVT_KICAD_CHOICEBOX( ID_AUX_TOOLBAR_PCB_TRACK_WIDTH,
                         WinEDA_PcbFrame::Tracks_and_Vias_Size_Event )
    EVT_KICAD_CHOICEBOX( ID_AUX_TOOLBAR_PCB_VIA_SIZE,
                         WinEDA_PcbFrame::Tracks_and_Vias_Size_Event )
    EVT_TOOL( ID_TOOLBARH_PCB_AUTOPLACE, WinEDA_PcbFrame::AutoPlace )
    EVT_TOOL( ID_TOOLBARH_PCB_AUTOROUTE, WinEDA_PcbFrame::AutoPlace )
    EVT_TOOL( ID_TOOLBARH_PCB_FREEROUTE_ACCESS,
              WinEDA_PcbFrame::Access_to_External_Tool )

    // Option toolbar
    EVT_TOOL_RANGE( ID_TB_OPTIONS_START, ID_TB_OPTIONS_END,
                    WinEDA_PcbFrame::OnSelectOptionToolbar )
    EVT_TOOL( ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR,
                    WinEDA_PcbFrame::OnSelectOptionToolbar)

    // Vertical toolbar:
    EVT_TOOL( ID_NO_SELECT_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_HIGHLIGHT_BUTT,
              WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_COMPONENT_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_TRACK_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_ZONES_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_MIRE_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_ARC_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_CIRCLE_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_ADD_TEXT_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_ADD_LINE_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_COTATION_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_DELETE_ITEM_BUTT,
              WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_SHOW_1_RATSNEST_BUTT,
              WinEDA_PcbFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_PLACE_OFFSET_COORD_BUTT,
              WinEDA_PcbFrame::Process_Special_Functions )

    EVT_TOOL_RANGE( ID_PCB_MUWAVE_START_CMD, ID_PCB_MUWAVE_END_CMD,
                    WinEDA_PcbFrame::ProcessMuWaveFunctions )

    EVT_TOOL_RCLICKED( ID_TRACK_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_CIRCLE_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_ARC_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_ADD_TEXT_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_ADD_LINE_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
    EVT_TOOL_RCLICKED( ID_PCB_COTATION_BUTT, WinEDA_PcbFrame::ToolOnRightClick )

    EVT_MENU_RANGE( ID_POPUP_PCB_AUTOPLACE_START_RANGE,
                    ID_POPUP_PCB_AUTOPLACE_END_RANGE,
                    WinEDA_PcbFrame::AutoPlace )

    EVT_MENU_RANGE( ID_POPUP_PCB_START_RANGE, ID_POPUP_PCB_END_RANGE,
                    WinEDA_PcbFrame::Process_Special_Functions )

    // Tracks and vias sizes general options
    EVT_MENU_RANGE( ID_POPUP_PCB_SELECT_WIDTH_START_RANGE,
                    ID_POPUP_PCB_SELECT_WIDTH_END_RANGE,
                    WinEDA_PcbFrame::Tracks_and_Vias_Size_Event )

    // popup menus
    EVT_MENU( ID_POPUP_PCB_DELETE_TRACKSEG,
              WinEDA_PcbFrame::Process_Special_Functions )
    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    WinEDA_PcbFrame::Process_Special_Functions )

END_EVENT_TABLE()


///////****************************///////////:


WinEDA_PcbFrame::WinEDA_PcbFrame( wxWindow* father,
                                  const wxString& title,
                                  const wxPoint& pos, const wxSize& size,
                                  long style ) :
    WinEDA_BasePcbFrame( father, PCB_FRAME, title, pos, size, style )
{
    m_FrameName = wxT( "PcbFrame" );
    m_Draw_Sheet_Ref = true;            // true to display sheet references
    m_Draw_Axis = false;                 // true to display X and Y axis
    m_Draw_Auxiliary_Axis = true;
    m_SelTrackWidthBox    = NULL;
    m_SelViaSizeBox = NULL;
    m_SelLayerBox   = NULL;
    m_TrackAndViasSizesList_Changed = false;
    m_show_microwave_tools = false;
    m_show_layer_manager_tools = true;

    m_Layers = new LYRS( this, DrawPanel );

    SetBoard( new BOARD( NULL, this ) );

    m_TrackAndViasSizesList_Changed = true;

    m_drc = new DRC( this );        // these 2 objects point to each other

    m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
    m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    m_DisplayPadNum  = DisplayOpt.DisplayPadNum;

    m_DisplayModEdge = DisplayOpt.DisplayModEdge;
    m_DisplayModText = DisplayOpt.DisplayModText;

    // Give an icon
    SetIcon( wxICON( a_icon_pcbnew ) );

    m_InternalUnits = PCB_INTERNAL_UNIT;    // Unites internes = 1/10000 inch
    SetBaseScreen( ScreenPcb );
    LoadSettings();
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    GetScreen()->AddGrid( m_UserGridSize, m_UserGridUnits, ID_POPUP_GRID_USER );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    if( DrawPanel )
        DrawPanel->m_Block_Enable = true;
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateAuxiliaryToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    ReCreateAuxVToolbar();

    // Fixed "Rendering" tab rows within the LAYER_WIDGET, only the initial color
    // is changed before appending to the LAYER_WIDGET.  This is an automatic variable
    // not a static variable, change the color & state after copying from code to renderRows
    // on the stack.
    LAYER_WIDGET::ROW renderRows[14] = {

#define RR  LAYER_WIDGET::ROW   // Render Row abreviation to reduce source width

             // text                id                      color       tooltip                 checked
        RR( _( "Through Via" ),     VIAS_VISIBLE,           WHITE,      _( "Show through vias" ) ),
        RR( _( "Bl/Buried Via" ),   VIA_MICROVIA_VISIBLE,   WHITE,      _( "Show blind or buried vias" )  ),
        RR( _( "Micro Via" ),       VIA_BBLIND_VISIBLE,     WHITE,      _( "Show micro vias") ),
        RR( _( "Ratsnest" ),        RATSNEST_VISIBLE,       WHITE,      _( "Show unconnected nets as a ratsnest") ),

        RR( _( "Pads Front" ),      PAD_FR_VISIBLE,         WHITE,      _( "Show footprint pads on board's front" ) ),
        RR( _( "Pads Back" ),       PAD_BK_VISIBLE,         WHITE,      _( "Show footprint pads on board's back" ) ),

        RR( _( "Text Front" ),      MOD_TEXT_FR_VISIBLE,    WHITE,      _( "Show footprint text on board's back" ) ),
        RR( _( "Text Back" ),       MOD_TEXT_BK_VISIBLE,    WHITE,      _( "Show footprint text on board's back" ) ),
        RR( _( "Hidden Text" ),     MOD_TEXT_INVISIBLE,     WHITE,      _( "Show footprint text marked as invisible" ) ),

        RR( _( "Anchors" ),         ANCHOR_VISIBLE,         WHITE,      _( "Show footprint and text origins as a cross" ) ),
        RR( _( "Grid" ),            GRID_VISIBLE,           WHITE,      _( "Show the (x,y) grid dots" ) ),
        RR( _( "No-Connects" ),     NO_CONNECTS_VISIBLE,    -1,         _( "Show a marker on pads which have no net connected" ) ),
        RR( _( "Modules Front" ),   MOD_FR_VISIBLE,         -1,         _( "Show footprints that are on board's front") ),
        RR( _( "Modules Back" ),    MOD_BK_VISIBLE,         -1,         _( "Show footprints that are on board's back") ),
    };

    for( unsigned row=0;  row<DIM(renderRows);  ++row )
    {
        if( renderRows[row].color != -1 )       // does this row show a color?
        {
            // this window frame must have an established BOARD, i.e. after SetBoard()
            renderRows[row].color = GetBoard()->GetVisibleElementColor( renderRows[row].id );
        }
        // @todo
        // renderRows[row].state = GetBoard()->IsElementVisible( renderRows[row].id );
    }

    m_Layers->AppendRenderRows( renderRows, DIM(renderRows) );

#if defined(KICAD_AUIMANAGER)
    m_auimgr.SetManagedWindow( this );

    wxAuiPaneInfo horiz;
    horiz.Gripper( false );
    horiz.DockFixed( true );
    horiz.Movable( false );
    horiz.Floatable( false );
    horiz.CloseButton( false );
    horiz.CaptionVisible( false );

    wxAuiPaneInfo vert( horiz );

    vert.TopDockable( false ).BottomDockable( false );
    horiz.LeftDockable( false ).RightDockable( false );

    // LAYER_WIDGET is floatable, but initially docked at far right
    wxAuiPaneInfo   lyrs;
    lyrs.MinSize( m_Layers->GetBestSize() );    // updated in ReFillLayerWidget
    lyrs.BestSize( m_Layers->GetBestSize() );
    lyrs.CloseButton( false );
    lyrs.Caption( _( "Visibles" ) );
    lyrs.IsFloatable();


    if( m_HToolBar )
        m_auimgr.AddPane( m_HToolBar,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top().Row( 0 ) );

    if( m_AuxiliaryToolBar )
        m_auimgr.AddPane( m_AuxiliaryToolBar,
                          wxAuiPaneInfo( horiz ).Name( wxT( "m_AuxiliaryToolBar" ) ).Top().Row( 1 ) );

    if( m_AuxVToolBar )
        m_auimgr.AddPane( m_AuxVToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_AuxVToolBar" ) ).Right().Row( 2 ).Hide() );

    if( m_VToolBar )
        m_auimgr.AddPane( m_VToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_VToolBar" ) ).Right().Row( 1 ) );

    m_auimgr.AddPane( m_Layers, lyrs.Name( wxT( "m_LayersManagerToolBar" ) ).Right().Row( 0 ) );

    if( m_OptionsToolBar )
    {
        m_auimgr.AddPane( m_OptionsToolBar,
                          wxAuiPaneInfo( vert ).Name( wxT( "m_OptionsToolBar" ) ).Left() );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR,
                                      m_show_layer_manager_tools );
        m_auimgr.GetPane( wxT( "m_LayersManagerToolBar" ) ).Show( m_show_layer_manager_tools );
    }

    if( DrawPanel )
        m_auimgr.AddPane( DrawPanel,
                          wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    if( MsgPanel )
        m_auimgr.AddPane( MsgPanel,
                          wxAuiPaneInfo( horiz ).Name( wxT( "MsgPanel" ) ).Bottom() );

    m_auimgr.Update();
#else

    if( m_AuxVToolBar )
        m_AuxVToolBar->Show(m_show_microwave_tools);
#endif

    ReFillLayerWidget();    // this is near end because contents establishes size
}


WinEDA_PcbFrame::~WinEDA_PcbFrame()
{
    extern PARAM_CFG_BASE* ParamCfgList[];

    wxGetApp().SaveCurrentSetupValues( ParamCfgList );
    delete m_drc;
}


//-----<LAYER_WIDGET callbacks>-------------------------------------------

void WinEDA_PcbFrame::LYRS::OnLayerColorChange( int aLayer, int aColor )
{
    myframe->GetBoard()->SetLayerColor( aLayer, aColor );
    myframe->DrawPanel->Refresh();
}

bool WinEDA_PcbFrame::LYRS::OnLayerSelect( int aLayer )
{
    // @todo
    return true;
}

void WinEDA_PcbFrame::LYRS::OnLayerVisible( int aLayer, bool isVisible, bool isFinal )
{
    BOARD* brd = myframe->GetBoard();

    int visibleLayers = brd->GetVisibleLayers();

    if( isVisible )
        visibleLayers |= (1 << aLayer);
    else
        visibleLayers &= ~(1 << aLayer);

    brd->SetVisibleLayers( visibleLayers );

    if( isFinal )
        myframe->DrawPanel->Refresh();
}

void WinEDA_PcbFrame::LYRS::OnRenderColorChange( int aId, int aColor )
{
    myframe->GetBoard()->SetVisibleElementColor( aId, aColor );
    myframe->DrawPanel->Refresh();
}

void WinEDA_PcbFrame::LYRS::OnRenderEnable( int aId, bool isEnabled )
{
    BOARD*  brd = myframe->GetBoard();

    /* @todo:

        1) move:

        RATSNEST_VISIBLE,
        GRID_VISIBLE,   ? maybe not this one

        NO_CONNECTS_VISIBLE,
        MOD_FR_VISIBLE,
        MOD_BK_VISIBLE,

        into m_VisibleElements and get rid of globals.

        2) Add IsElementVisible() & SetVisibleElement() to class BOARD
    */

    switch( aId )
    {
        // see todo above, don't really want anything except IsElementVisible() here.

    case GRID_VISIBLE:
        myframe->m_Draw_Grid = isEnabled;
        break;

    case MOD_FR_VISIBLE:
        DisplayOpt.Show_Modules_Cmp = isEnabled;
        break;

    case MOD_BK_VISIBLE:
        DisplayOpt.Show_Modules_Cu = isEnabled;
        break;

    default:

        int visibleElements = brd->GetVisibleElements();

        if( isEnabled )
            visibleElements |= (1 << aId );
        else
            visibleElements &= ~(1 << aId);

        brd->SetVisibleElements( visibleElements );
    }

    myframe->DrawPanel->Refresh();
}

//-----</LAYER_WIDGET callbacks>------------------------------------------


void WinEDA_PcbFrame::ReFillLayerWidget()
{
    BOARD*  brd = GetBoard();
    int     layer;

    int enabledLayers = brd->GetEnabledLayers();

//    m_Layers->Freeze();     // no screen updates until done modifying

    m_Layers->ClearLayerRows();

    // show all coppers first, with front on top, back on bottom, then technical layers

    layer = LAYER_N_FRONT;
    if( enabledLayers & (1 << layer) )
    {
        m_Layers->AppendLayerRow( LAYER_WIDGET::ROW(
            brd->GetLayerName( layer ), layer, brd->GetLayerColor( layer ), _("Front copper layer"), true ) );
    }

    for( layer = LAYER_N_FRONT-1;  layer >= 1;  --layer )
    {
        if( enabledLayers & (1 << layer) )
        {
            m_Layers->AppendLayerRow( LAYER_WIDGET::ROW(
                brd->GetLayerName( layer ), layer, brd->GetLayerColor( layer ), _("An innner copper layer"), true ) );
        }
    }

    layer = LAYER_N_BACK;
    if( enabledLayers & (1 << layer) )
    {
        m_Layers->AppendLayerRow( LAYER_WIDGET::ROW(
            brd->GetLayerName( layer ), layer, brd->GetLayerColor( layer ), _("Back copper layer"), true ) );
    }

    m_Layers->SelectLayer( LAYER_N_FRONT );

    // technical layers are shown in this order:
    static const struct {
        int         layerId;
        wxString    tooltip;
    } techLayerSeq[] = {

    /* some layers are not visible nor editable, don't show them for now:
     * >> In fact they are useful here because we must be able to change
     * the color and visibility because they can be visible.
     * slikscreen and adhesive layers are visible (adhesive layer is rarely used)
     * Solder mask and solder paste (used for pads) are visible in *Hight Color*
     * mode when they are selected
     * they are now editable because Pcbnew handle parameters (global and local)
     * to calculate pads shapes on these layers
    */
        { ADHESIVE_N_FRONT,     _("Adhesive on board's front")      },
        { ADHESIVE_N_BACK,      _("Adhesive on board's back")       },
        { SOLDERPASTE_N_FRONT,  _("Solder paste on board's front")  },
        { SOLDERPASTE_N_BACK,   _("Solder paste on board's back")   },
        { SILKSCREEN_N_FRONT,   _("Silkscreen on board's front")    },
        { SILKSCREEN_N_BACK,    _("Silkscreen on board's back")     },
        { SOLDERMASK_N_FRONT,   _("Solder mask on board's front")   },
        { SOLDERMASK_N_BACK,    _("Solder mask on board's back")    },
        { DRAW_N,               _( "Explanatory drawings" )         },
        { COMMENT_N,            _( "Explanatory comments" )         },
        { ECO1_N,               _( "TDB" )                          },
        { ECO2_N,               _( "TBD" )                          },
        { EDGE_N,               _( "Board's perimeter definition" ) },
    };

    for( unsigned i=0;  i<DIM(techLayerSeq);  ++i )
    {
        layer = techLayerSeq[i].layerId;

        if( !(enabledLayers & (1 << layer)) )
            continue;

        m_Layers->AppendLayerRow( LAYER_WIDGET::ROW(
            brd->GetLayerName( layer ), layer, brd->GetLayerColor( layer ),
            techLayerSeq[i].tooltip, true ) );
    }

//    m_Layers->Thaw();

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


void WinEDA_PcbFrame::OnQuit( wxCommandEvent & WXUNUSED(event) )
{
    Close(true);
}

void WinEDA_PcbFrame::OnCloseWindow( wxCloseEvent& Event )
{
    DrawPanel->m_AbortRequest = true;

    if( ScreenPcb->IsModify() )
    {
        unsigned        ii;
        wxMessageDialog dialog( this, _( "Board modified, Save before exit ?" ),
                                _( "Confirmation" ),
                                wxYES_NO | wxCANCEL | wxICON_EXCLAMATION |
                                wxYES_DEFAULT );

        ii = dialog.ShowModal();

        switch( ii )
        {
        case wxID_CANCEL:
            Event.Veto();
            return;

        case wxID_NO:
            break;

        case wxID_OK:
        case wxID_YES:
            SavePcbFile( GetScreen()->m_FileName );
            break;
        }
    }

    SaveSettings();

    // do not show the window because ScreenPcb will be deleted and we do not
    // want any paint event
    Show( false );
    ActiveScreen = ScreenPcb;
    Destroy();
}


/**
 * Display 3D frame of current printed circuit board.
 */
void WinEDA_PcbFrame::Show3D_Frame( wxCommandEvent& event )
{
    if( m_Draw3DFrame )
    {
        DisplayInfoMessage( this, _( "3D Frame already opened" ) );
        return;
    }

    m_Draw3DFrame = new WinEDA3D_DrawFrame( this, _( "3D Viewer" ) );
    m_Draw3DFrame->Show( true );
}


/**
 * Display the Design Rules Editor.
 */
void WinEDA_PcbFrame::ShowDesignRulesEditor( wxCommandEvent& event )
{
    DIALOG_DESIGN_RULES dR_editor( this );
    int returncode = dR_editor.ShowModal();

    if( returncode == wxID_OK )     // New rules, or others changes.
    {
        ReCreateLayerBox( NULL );
        // ReFillLayerWidget(); why?
        GetScreen()->SetModify();
    }
}


void WinEDA_PcbFrame::LoadSettings()
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config == NULL )
        return;

    WinEDA_BasePcbFrame::LoadSettings();

    config->Read( PCB_MAGNETIC_PADS_OPT, &g_MagneticPadOption );
    config->Read( PCB_MAGNETIC_TRACKS_OPT, &g_MagneticTrackOption );
    config->Read( SHOW_MICROWAVE_TOOLS, &m_show_microwave_tools );
    config->Read( SHOW_LAYER_MANAGER_TOOLS, &m_show_layer_manager_tools );
}


void WinEDA_PcbFrame::SaveSettings()
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config == NULL )
        return;

    WinEDA_BasePcbFrame::SaveSettings();

    wxRealPoint GridSize = GetScreen()->GetGridSize();

    config->Write( PCB_MAGNETIC_PADS_OPT, (long) g_MagneticPadOption );
    config->Write( PCB_MAGNETIC_TRACKS_OPT, (long) g_MagneticTrackOption );
    config->Write( SHOW_MICROWAVE_TOOLS,
        ( m_AuxVToolBar && m_AuxVToolBar->IsShown() ) ? true : false );
    config->Write( SHOW_LAYER_MANAGER_TOOLS, (long)m_show_layer_manager_tools );

}


/** Function SynchronizeLayersManager( )
 * Must be called when info displayed in the layer manager Toolbar
 * as been changed in the main window ( by hotkey or a tool option.
 * Mainly when the active layer as changed.
 * @param aFlag = flag giving the type of data (layers, checkboxes...)
 */
void WinEDA_PcbFrame::SynchronizeLayersManager( int aFlag )
{
    // Ensure Layer manager synchronization for the active layer
    if( (aFlag & 1) )
        m_Layers->SelectLayer(GetScreen()->m_Active_Layer);
}
