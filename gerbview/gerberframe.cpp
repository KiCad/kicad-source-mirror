/*******************/
/* gerberframe.cpp */
/*******************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"

#include "gerbview.h"
#include "pcbplot.h"
#include "bitmaps.h"
#include "protos.h"


/****************************************/
/* class WinEDA_GerberFrame for GerbView*/
/****************************************/

BEGIN_EVENT_TABLE( WinEDA_GerberFrame, WinEDA_BasePcbFrame )
    EVT_CLOSE( WinEDA_GerberFrame::OnCloseWindow )
    EVT_SIZE( WinEDA_GerberFrame::OnSize )

    EVT_TOOL_RANGE( ID_ZOOM_IN, ID_ZOOM_PAGE, WinEDA_GerberFrame::OnZoom )

    EVT_TOOL( ID_LOAD_FILE, WinEDA_GerberFrame::Files_io )
    EVT_TOOL( ID_APPEND_FILE, WinEDA_GerberFrame::Files_io )
    EVT_TOOL( ID_INC_LAYER_AND_APPEND_FILE, WinEDA_GerberFrame::Files_io )
    EVT_TOOL( ID_GERBVIEW_LOAD_DRILL_FILE, WinEDA_GerberFrame::Files_io )
    EVT_TOOL( ID_GERBVIEW_LOAD_DCODE_FILE, WinEDA_GerberFrame::Files_io )
    EVT_TOOL( ID_NEW_BOARD, WinEDA_GerberFrame::Files_io )
    EVT_TOOL( ID_SAVE_BOARD, WinEDA_GerberFrame::Files_io )

// Menu Files:
    EVT_MENU( ID_LOAD_FILE, WinEDA_GerberFrame::Files_io )
    EVT_MENU( ID_APPEND_FILE, WinEDA_GerberFrame::Files_io )
    EVT_MENU( ID_MENU_INC_LAYER_AND_APPEND_FILE, WinEDA_GerberFrame::Files_io )
    EVT_MENU( ID_NEW_BOARD, WinEDA_GerberFrame::Files_io )
    EVT_MENU( ID_SAVE_BOARD, WinEDA_GerberFrame::Files_io )
    EVT_MENU( ID_SAVE_BOARD_AS, WinEDA_GerberFrame::Files_io )
    EVT_MENU( ID_GEN_PLOT, WinEDA_GerberFrame::ToPlotter )
    EVT_MENU( ID_GERBVIEW_EXPORT_TO_PCBNEW,
              WinEDA_GerberFrame::ExportDataInPcbnewFormat )

    EVT_MENU_RANGE( wxID_FILE1, wxID_FILE9, WinEDA_GerberFrame::OnFileHistory )

    EVT_MENU( ID_EXIT, WinEDA_GerberFrame::Process_Special_Functions )

// menu Preferences
    EVT_MENU_RANGE( ID_CONFIG_AND_PREFERENCES_START,
                    ID_CONFIG_AND_PREFERENCES_END,
                    WinEDA_GerberFrame::Process_Config )

    EVT_MENU( ID_COLORS_SETUP, WinEDA_GerberFrame::Process_Config )
    EVT_MENU( ID_OPTIONS_SETUP, WinEDA_GerberFrame::InstallGerberGeneralOptionsFrame )
    EVT_MENU( ID_PCB_DISPLAY_OPTIONS_SETUP, WinEDA_GerberFrame::InstallGerberDisplayOptionsDialog )

    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
                    WinEDA_DrawFrame::SetLanguage )

// menu Postprocess
    EVT_MENU( ID_GERBVIEW_SHOW_LIST_DCODES,
              WinEDA_GerberFrame::Process_Special_Functions )
    EVT_MENU( ID_GERBVIEW_POPUP_DELETE_DCODE_ITEMS,
              WinEDA_GerberFrame::Process_Special_Functions )
    EVT_MENU( ID_GERBVIEW_SHOW_SOURCE,
              WinEDA_GerberFrame::Process_Special_Functions )


// menu Miscellaneous
    EVT_MENU( ID_PCB_GLOBAL_DELETE,
              WinEDA_GerberFrame::Process_Special_Functions )

// Menu Help
    EVT_MENU( ID_GENERAL_HELP, WinEDA_DrawFrame::GetKicadHelp )
    EVT_MENU( ID_KICAD_ABOUT, WinEDA_DrawFrame::GetKicadAbout )

    EVT_TOOL( ID_SHEET_SET, WinEDA_DrawFrame::Process_PageSettings )
    EVT_TOOL( wxID_CUT, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_TOOL( wxID_COPY, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_TOOL( wxID_PASTE, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_TOOL( ID_UNDO_BUTT, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_TOOL( ID_GEN_PRINT, WinEDA_GerberFrame::ToPrinter )
    EVT_TOOL( ID_FIND_ITEMS, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_TOOL( ID_DRC_CONTROL, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_KICAD_CHOICEBOX( ID_TOOLBARH_PCB_SELECT_LAYER,
                         WinEDA_GerberFrame::Process_Special_Functions )

    EVT_KICAD_CHOICEBOX( ID_TOOLBARH_GERBER_SELECT_TOOL,
                         WinEDA_GerberFrame::Process_Special_Functions )

// Vertical toolbar:
    EVT_TOOL( ID_NO_SELECT_BUTT, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_TOOL( ID_TRACK_BUTT, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_ZONES_BUTT, WinEDA_GerberFrame::Process_Special_Functions )
    EVT_TOOL( ID_PCB_DELETE_ITEM_BUTT,
          WinEDA_GerberFrame::Process_Special_Functions )

    EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                    WinEDA_GerberFrame::Process_Special_Functions )

// Pop up menu
    EVT_MENU( ID_GERBVIEW_POPUP_DELETE_DCODE_ITEMS,
              WinEDA_GerberFrame::Process_Special_Functions )

// Option toolbar
    EVT_TOOL_RANGE( ID_TB_OPTIONS_START, ID_TB_OPTIONS_END,
                    WinEDA_GerberFrame::OnSelectOptionToolbar )

END_EVENT_TABLE()

WinEDA_GerberFrame::WinEDA_GerberFrame( wxWindow*       father,
                                        const wxString& title,
                                        const wxPoint&  pos,
                                        const wxSize&   size,
                                        long            style ) :
    WinEDA_BasePcbFrame( father, GERBER_FRAME, title, pos, size, style )
{
    m_FrameName = wxT( "GerberFrame" );

    m_Draw_Axis = true;         // true to show X and Y axis on screen
    m_Draw_Sheet_Ref = FALSE;   // TRUE for reference drawings.
    if( DrawPanel )
        DrawPanel->m_Block_Enable = TRUE;

    // Give an icon
#ifdef __WINDOWS__
    SetIcon( wxICON( a_icon_gerbview ) );
#else
    SetIcon( wxICON( icon_gerbview ) );
#endif

    SetBaseScreen( ScreenPcb );
    ActiveScreen = ScreenPcb;

    LoadSettings();

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId  );

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

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

    m_auimgr.AddPane( m_HToolBar,
                      wxAuiPaneInfo( horiz ).Name( wxT( "m_HToolBar" ) ).Top().Row( 0 ) );

    m_auimgr.AddPane( m_VToolBar,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_VToolBar" ) ).Right() );

    m_auimgr.AddPane( m_OptionsToolBar,
                      wxAuiPaneInfo( vert ).Name( wxT( "m_OptionsToolBar" ) ).Left() );

    m_auimgr.AddPane( DrawPanel,
                      wxAuiPaneInfo().Name( wxT( "DrawFrame" ) ).CentrePane() );

    m_auimgr.AddPane( MsgPanel,
                      wxAuiPaneInfo( horiz ).Name( wxT( "MsgPanel" ) ).Bottom() );

    m_auimgr.Update();
#endif
}


WinEDA_GerberFrame::~WinEDA_GerberFrame()
{
    SetBaseScreen( ScreenPcb );
    extern PARAM_CFG_BASE* ParamCfgList[];
    wxGetApp().SaveCurrentSetupValues( ParamCfgList );
}


void WinEDA_GerberFrame::OnCloseWindow( wxCloseEvent& Event )
{
    PCB_SCREEN* screen = ScreenPcb;

#if 0       // unused currently
    while( screen )
    {
        if( screen->IsModify() )
            break;
        screen = screen->Next();
    }

    if( screen )
    {
        if( !IsOK( this, _( "Layer modified,  Continue ?" ) ) )
        {
            Event.Veto();
            return;
        }
    }
#endif

    while( screen ) // Modify delete flag to prevent further message.
    {
        screen->ClrModify();
        screen = screen->Next();
    }

    SetBaseScreen( ActiveScreen = ScreenPcb );

    SaveSettings();
    Destroy();
}


/** Function SetToolbars()
 * Set the tools state for the toolbars, according to display options
 */
void WinEDA_GerberFrame::SetToolbars()
{
    int     layer  = ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer;
    GERBER* gerber = g_GERBER_List[layer];

    if( m_HToolBar == NULL )
        return;

    if( GetScreen()->m_BlockLocate.m_Command == BLOCK_MOVE )
    {
        m_HToolBar->EnableTool( wxID_CUT, TRUE );
        m_HToolBar->EnableTool( wxID_COPY, TRUE );
    }
    else
    {
        m_HToolBar->EnableTool( wxID_CUT, FALSE );
        m_HToolBar->EnableTool( wxID_COPY, FALSE );
    }

    if( m_SelLayerBox->GetSelection() !=
        ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer )
    {
        m_SelLayerBox->SetSelection(
            ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer );
    }

    if( gerber )
    {
        int sel_index;
        m_SelLayerTool->Enable( TRUE );
        if( gerber->m_Selected_Tool < FIRST_DCODE )  // No tool selected
            sel_index = 0;
        else
            sel_index = gerber->m_Selected_Tool - FIRST_DCODE + 1;

        if( sel_index != m_SelLayerTool->GetSelection() )
        {
            m_SelLayerTool->SetSelection( sel_index );
        }
    }
    else
    {
        m_SelLayerTool->SetSelection( 0 );
        m_SelLayerTool->Enable( FALSE );
    }

    if( m_OptionsToolBar )
    {
        m_OptionsToolBar->ToggleTool(
            ID_TB_OPTIONS_SELECT_UNIT_MM,
            g_UnitMetric ==
            MILLIMETRE ? TRUE : FALSE );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_INCH,
                                      g_UnitMetric == INCHES ? TRUE : FALSE );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                      DisplayOpt.DisplayPolarCood );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_GRID,
                                      m_Draw_Grid );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_CURSOR,
                                      m_CursorShape );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                                      !m_DisplayPadFill );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH,
                                      !m_DisplayPcbTrackFill );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH,
                                      g_DisplayPolygonsModeSketch == 0 ? 0 : 1 );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_DCODES,
                                      DisplayOpt.DisplayPadNum );
    }

    DisplayUnitsMsg();

#if defined(KICAD_AUIMANAGER)
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();
#endif
}


int WinEDA_GerberFrame::BestZoom()
{
    double x, y;
    wxSize size;

    GetBoard()->ComputeBoundaryBox();
    size = DrawPanel->GetClientSize();
    x = ( (double) GetBoard()->m_BoundaryBox.GetWidth() +
          GetScreen()->GetGridSize().x ) / (double) size.x;
    y = ( (double) GetBoard()->m_BoundaryBox.GetHeight() +
          GetScreen()->GetGridSize().y ) / (double) size.y;
    GetScreen()->m_Curseur = GetBoard()->m_BoundaryBox.Centre();

    return wxRound( MAX( x, y ) * (double) GetScreen()->m_ZoomScalar );
}

/**************************************/
void WinEDA_GerberFrame::LoadSettings()
/**************************************/
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config == NULL )
        return;

    WinEDA_BasePcbFrame::LoadSettings();
    long pageSize_opt;
    config->Read( GerbviewShowPageSizeOption, &pageSize_opt, 0l );
    int imax = 0;
    for( ; g_GerberPageSizeList[imax] != NULL; imax++ );
    if( pageSize_opt < 0 || pageSize_opt >= imax )
        pageSize_opt = 0;
    GetScreen()->m_CurrentSheetDesc = g_GerberPageSizeList[pageSize_opt];
    if ( pageSize_opt > 0 )
    {
        m_Draw_Sheet_Ref = true;
    }
}

/**************************************/
void WinEDA_GerberFrame::SaveSettings()
/**************************************/
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( config == NULL )
        return;

    WinEDA_BasePcbFrame::SaveSettings();

    wxRealPoint GridSize = GetScreen()->GetGridSize();

    long pageSize_opt = 0;
    if( m_Draw_Sheet_Ref )
    {
        for( int ii = 1; g_GerberPageSizeList[ii] != NULL; ii++ )
        {
            if( GetScreen()->m_CurrentSheetDesc == g_GerberPageSizeList[ii] )
            {
                pageSize_opt = ii;
                break;
            }
        }
    }
    config->Write( GerbviewShowPageSizeOption, pageSize_opt );
}
