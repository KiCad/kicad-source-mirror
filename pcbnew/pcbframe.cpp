/****************************************************************/
/* pcbframe.cpp - fonctions des classes du type WinEDA_PcbFrame */
/****************************************************************/

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"
#include "collectors.h"

#include "bitmaps.h"
#include "protos.h"
#include "id.h"
#include "drc_stuff.h"
#include "kbool/include/booleng.h"

/*******************************/
/* class WinEDA_PcbFrame */
/*******************************/

BEGIN_EVENT_TABLE( WinEDA_PcbFrame, WinEDA_BasePcbFrame )

COMMON_EVENTS_DRAWFRAME EVT_SOCKET( ID_EDA_SOCKET_EVENT_SERV, WinEDA_PcbFrame::OnSockRequestServer )
EVT_SOCKET( ID_EDA_SOCKET_EVENT, WinEDA_PcbFrame::OnSockRequest )

EVT_KICAD_CHOICEBOX( ID_ON_ZOOM_SELECT, WinEDA_PcbFrame::OnSelectZoom )
EVT_KICAD_CHOICEBOX( ID_ON_GRID_SELECT, WinEDA_PcbFrame::OnSelectGrid )

EVT_CLOSE( WinEDA_PcbFrame::OnCloseWindow )
EVT_SIZE( WinEDA_PcbFrame::OnSize )

EVT_TOOL_RANGE( ID_ZOOM_IN_BUTT, ID_ZOOM_PAGE_BUTT,
                WinEDA_PcbFrame::Process_Zoom )

EVT_TOOL( ID_LOAD_FILE, WinEDA_PcbFrame::Files_io )
EVT_TOOL( ID_MENU_READ_LAST_SAVED_VERSION_BOARD, WinEDA_PcbFrame::Files_io )
EVT_TOOL( ID_MENU_RECOVER_BOARD, WinEDA_PcbFrame::Files_io )
EVT_TOOL( ID_NEW_BOARD, WinEDA_PcbFrame::Files_io )
EVT_TOOL( ID_SAVE_BOARD, WinEDA_PcbFrame::Files_io )
EVT_TOOL( ID_OPEN_MODULE_EDITOR, WinEDA_PcbFrame::Process_Special_Functions )

EVT_MENU_RANGE( ID_PREFERENCES_FONT_INFOSCREEN, ID_PREFERENCES_FONT_END,
                WinEDA_DrawFrame::ProcessFontPreferences )

// Menu Files:

EVT_MENU( ID_MAIN_MENUBAR, WinEDA_PcbFrame::Process_Special_Functions )

EVT_MENU( ID_MENU_LOAD_FILE, WinEDA_PcbFrame::Files_io )
EVT_MENU( ID_MENU_NEW_BOARD, WinEDA_PcbFrame::Files_io )
EVT_MENU( ID_MENU_SAVE_BOARD, WinEDA_PcbFrame::Files_io )
EVT_MENU( ID_MENU_APPEND_FILE, WinEDA_PcbFrame::Files_io )
EVT_MENU( ID_MENU_SAVE_BOARD_AS, WinEDA_PcbFrame::Files_io )
EVT_MENU( ID_GEN_PLOT, WinEDA_PcbFrame::ToPlotter )
EVT_MENU_RANGE( ID_LOAD_FILE_1, ID_LOAD_FILE_10,
                WinEDA_PcbFrame::Files_io )

EVT_MENU( ID_GEN_EXPORT_SPECCTRA, WinEDA_PcbFrame::ExportToSpecctra )
EVT_MENU( ID_GEN_EXPORT_FILE_GENCADFORMAT, WinEDA_PcbFrame::ExportToGenCAD )
EVT_MENU( ID_GEN_EXPORT_FILE_MODULE_REPORT, WinEDA_PcbFrame::GenModuleReport )

EVT_MENU( ID_GEN_IMPORT_SPECCTRA_SESSION, WinEDA_PcbFrame::ImportSpecctraSession )
EVT_MENU( ID_GEN_IMPORT_SPECCTRA_DESIGN, WinEDA_PcbFrame::ImportSpecctraDesign )

EVT_MENU( ID_MENU_ARCHIVE_NEW_MODULES, WinEDA_PcbFrame::Process_Special_Functions )
EVT_MENU( ID_MENU_ARCHIVE_ALL_MODULES, WinEDA_PcbFrame::Process_Special_Functions )

EVT_MENU( ID_EXIT, WinEDA_PcbFrame::Process_Special_Functions )

// menu Config
EVT_MENU_RANGE( ID_CONFIG_AND_PREFERENCES_START, ID_CONFIG_AND_PREFERENCES_END,
                WinEDA_PcbFrame::Process_Config )

EVT_MENU( ID_COLORS_SETUP, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_OPTIONS_SETUP, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_PCB_TRACK_SIZE_SETUP, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_PCB_DRAWINGS_WIDTHS_SETUP, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_PCB_PAD_SETUP, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_PCB_LOOK_SETUP, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_CONFIG_SAVE, WinEDA_PcbFrame::Process_Config )
EVT_MENU( ID_CONFIG_READ, WinEDA_PcbFrame::Process_Config )

EVT_MENU( ID_PCB_USER_GRID_SETUP, WinEDA_PcbFrame::Process_Special_Functions )

EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
                WinEDA_DrawFrame::SetLanguage )

// menu Postprocess
EVT_MENU( ID_PCB_GEN_POS_MODULES_FILE, WinEDA_PcbFrame::GenModulesPosition )
EVT_MENU( ID_PCB_GEN_DRILL_FILE, WinEDA_PcbFrame::InstallDrillFrame )
EVT_MENU( ID_PCB_GEN_CMP_FILE, WinEDA_PcbFrame::Files_io )

// menu Miscellaneous
EVT_MENU( ID_MENU_LIST_NETS, WinEDA_PcbFrame::Liste_Equipot )
EVT_MENU( ID_PCB_GLOBAL_DELETE, WinEDA_PcbFrame::Process_Special_Functions )
EVT_MENU( ID_MENU_PCB_CLEAN, WinEDA_PcbFrame::Process_Special_Functions )
EVT_MENU( ID_MENU_PCB_SWAP_LAYERS, WinEDA_PcbFrame::Process_Special_Functions )

// Menu Help
EVT_MENU( ID_GENERAL_HELP, WinEDA_DrawFrame::GetKicadHelp )
EVT_MENU( ID_KICAD_ABOUT, WinEDA_BasicFrame::GetKicadAbout )

// Menu 3D Frame
EVT_MENU( ID_MENU_PCB_SHOW_3D_FRAME, WinEDA_PcbFrame::Show3D_Frame )

// Horizontal toolbar
EVT_TOOL( ID_TO_LIBRARY, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_SHEET_SET, WinEDA_DrawFrame::Process_PageSettings )
EVT_TOOL( wxID_CUT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( wxID_COPY, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( wxID_PASTE, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_UNDO_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_GEN_PRINT, WinEDA_DrawFrame::ToPrinter )
EVT_TOOL( ID_GEN_PLOT, WinEDA_DrawFrame::Process_Special_Functions )
EVT_TOOL( ID_FIND_ITEMS, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_GET_NETLIST, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_DRC_CONTROL, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH, WinEDA_PcbFrame::Process_Special_Functions )
EVT_KICAD_CHOICEBOX( ID_TOOLBARH_PCB_SELECT_LAYER,
                     WinEDA_PcbFrame::Process_Special_Functions )
EVT_KICAD_CHOICEBOX( ID_AUX_TOOLBAR_PCB_TRACK_WIDTH,
                     WinEDA_PcbFrame::Process_Special_Functions )
EVT_KICAD_CHOICEBOX( ID_AUX_TOOLBAR_PCB_VIA_SIZE,
                     WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_TOOLBARH_PCB_AUTOPLACE, WinEDA_PcbFrame::AutoPlace )
EVT_TOOL( ID_TOOLBARH_PCB_AUTOROUTE, WinEDA_PcbFrame::AutoPlace )
EVT_TOOL( ID_TOOLBARH_PCB_FREEROUTE_ACCESS, WinEDA_PcbFrame::Access_to_External_Tool )

// Option toolbar
EVT_TOOL_RANGE( ID_TB_OPTIONS_START, ID_TB_OPTIONS_END,
                WinEDA_PcbFrame::OnSelectOptionToolbar )

// Vertical toolbar:
EVT_TOOL( ID_NO_SELECT_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_HIGHLIGHT_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_COMPONENT_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_TRACK_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_ZONES_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_MIRE_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_ARC_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_CIRCLE_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_TEXT_COMMENT_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_LINE_COMMENT_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_COTATION_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_DELETE_ITEM_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_SHOW_1_RATSNEST_BUTT, WinEDA_PcbFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_PLACE_OFFSET_COORD_BUTT, WinEDA_PcbFrame::Process_Special_Functions )

EVT_TOOL_RANGE( ID_PCB_MUWAVE_START_CMD, ID_PCB_MUWAVE_END_CMD,
                WinEDA_PcbFrame::ProcessMuWaveFunctions )

EVT_TOOL_RCLICKED( ID_TRACK_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_PCB_CIRCLE_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_PCB_ARC_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_TEXT_COMMENT_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_LINE_COMMENT_BUTT, WinEDA_PcbFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_PCB_COTATION_BUTT, WinEDA_PcbFrame::ToolOnRightClick )

EVT_MENU_RANGE( ID_POPUP_PCB_AUTOPLACE_START_RANGE,
                ID_POPUP_PCB_AUTOPLACE_END_RANGE,
                WinEDA_PcbFrame::AutoPlace )

EVT_MENU_RANGE( ID_POPUP_PCB_START_RANGE, ID_POPUP_PCB_END_RANGE,
                WinEDA_PcbFrame::Process_Special_Functions )

// Annulation de commande en cours
EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                WinEDA_PcbFrame::Process_Special_Functions )

// PopUp Menus pour Zooms traites dans drawpanel.cpp

END_EVENT_TABLE()


///////****************************///////////:

/****************/
/* Constructeur */
/****************/

WinEDA_PcbFrame::WinEDA_PcbFrame( wxWindow* father, WinEDA_App* parent,
                                  const wxString& title,
                                  const wxPoint& pos, const wxSize& size,
                                  long style ) :
    WinEDA_BasePcbFrame( father, parent, PCB_FRAME, title, pos, size, style )
{
    m_FrameName                = wxT( "PcbFrame" );
    //m_AboutTitle               = g_PcbnewAboutTitle;
    m_Draw_Axis                = TRUE;          // TRUE pour avoir les axes dessines
    m_Draw_Grid                = g_ShowGrid;    // TRUE pour avoir la grille dessinee
    m_Draw_Sheet_Ref           = TRUE;          // TRUE pour avoir le cartouche dessine
    m_Draw_Auxiliary_Axis      = TRUE;
    m_SelTrackWidthBox         = NULL;
    m_SelViaSizeBox            = NULL;
    m_SelLayerBox              = NULL;
    m_ZoomMaxValue             = 2048;
    m_SelTrackWidthBox_Changed = FALSE;
    m_SelViaSizeBox_Changed    = FALSE;

    m_drc = new DRC( this );        // these 2 objects point to each other

    SetBOARD( new BOARD( NULL, this ) );

    m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
    m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    m_DisplayPadNum  = DisplayOpt.DisplayPadNum;

    m_DisplayModEdge = DisplayOpt.DisplayModEdge;
    m_DisplayModText = DisplayOpt.DisplayModText;

    // Give an icon
    SetIcon( wxICON( a_icon_pcbnew ) );

    m_InternalUnits = PCB_INTERNAL_UNIT;    // Unites internes = 1/10000 inch
    SetBaseScreen( ScreenPcb );
    GetSettings();
    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );

    wxSize GridSize( 500, 500 );

    if( m_Parent && m_Parent->m_EDA_Config )
    {
        long SizeX, SizeY;

        if( m_Parent->m_EDA_Config->Read( wxT( "PcbEditGrid_X" ), &SizeX )
           && m_Parent->m_EDA_Config->Read( wxT( "PcbEditGrid_Y" ), &SizeY ) )
        {
            GridSize.x = SizeX;
            GridSize.y = SizeY;
        }
        m_Parent->m_EDA_Config->Read( wxT( "PcbMagPadOpt" ), &g_MagneticPadOption );
        m_Parent->m_EDA_Config->Read( wxT( "PcbMagTrackOpt" ), &g_MagneticTrackOption );
    }
    GetScreen()->SetGrid( GridSize );

    if( DrawPanel )
        DrawPanel->m_Block_Enable = TRUE;
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateAuxiliaryToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();
}


/************************************/
WinEDA_PcbFrame::~WinEDA_PcbFrame()
/************************************/
{
    m_Parent->m_PcbFrame = NULL;
    SetBaseScreen( ScreenPcb );

    delete m_drc;

    if( m_Pcb != g_ModuleEditor_Pcb )
        delete m_Pcb;
}


/********************************************************/
void WinEDA_PcbFrame::OnCloseWindow( wxCloseEvent& Event )
/********************************************************/
{
    PCB_SCREEN* screen;

    DrawPanel->m_AbortRequest = TRUE;

    screen = ScreenPcb;
    while( screen )
    {
        if( screen->IsModify() )
            break;
        screen = screen->Next();
    }

    if( screen )
    {
        unsigned        ii;
        wxMessageDialog dialog( this, _( "Board modified, Save before exit ?" ),
                                _( "Confirmation" ), wxYES_NO | wxCANCEL | wxICON_EXCLAMATION |
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

    while( screen ) // suppression flag modify pour eviter d'autres message
    {
        screen->ClrModify();
        screen = screen->Next();
    }

    /* Reselection de l'ecran de base,
     *  pour les evenements de refresh generes par wxWindows */
    SetBaseScreen( ActiveScreen = ScreenPcb );

    SaveSettings();
    if( m_Parent && m_Parent->m_EDA_Config )
    {
        wxSize GridSize = GetScreen()->GetGrid();
        m_Parent->m_EDA_Config->Write( wxT( "PcbEditGrid_X" ), (long) GridSize.x );
        m_Parent->m_EDA_Config->Write( wxT( "PcbEditGrid_Y" ), (long) GridSize.y );
        m_Parent->m_EDA_Config->Write( wxT( "PcbMagPadOpt" ), (long) g_MagneticPadOption );
        m_Parent->m_EDA_Config->Write( wxT( "PcbMagTrackOpt" ), (long) g_MagneticTrackOption );
    }
    Destroy();
}


/***************************************/
void WinEDA_PcbFrame::SetToolbars()
/***************************************/

/*
 *  Active ou desactive les tools des toolbars, en fonction des commandes
 *  en cours
 */
{
    int ii, jj;

    if( m_ID_current_state == ID_TRACK_BUTT )
    {
        if( Drc_On )
            DrawPanel->SetCursor( wxCursor( wxCURSOR_PENCIL ) );
        else
            DrawPanel->SetCursor( wxCursor( wxCURSOR_QUESTION_ARROW ) );
    }


    if( m_HToolBar == NULL )
        return;

    m_HToolBar->EnableTool( ID_SAVE_BOARD, GetScreen()->IsModify() );

    if( GetScreen()->BlockLocate.m_Command == BLOCK_MOVE )
    {
        m_HToolBar->EnableTool( wxID_CUT, TRUE );
        m_HToolBar->EnableTool( wxID_COPY, TRUE );
    }
    else
    {
        m_HToolBar->EnableTool( wxID_CUT, FALSE );
        m_HToolBar->EnableTool( wxID_COPY, FALSE );
    }

    if( g_UnDeleteStackPtr )
    {
        m_HToolBar->EnableTool( wxID_PASTE, TRUE );
    }
    else
    {
        m_HToolBar->EnableTool( wxID_PASTE, FALSE );
    }

    if( g_UnDeleteStackPtr )
    {
        m_HToolBar->EnableTool( ID_UNDO_BUTT, TRUE );
    }
    else
        m_HToolBar->EnableTool( ID_UNDO_BUTT, FALSE );

    if( m_OptionsToolBar )
    {
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_DRC_OFF,
                                      !Drc_On );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_DRC_OFF, Drc_On ?
                                           _(
                                               "DRC Off (Disable !!!), Currently: DRC is active" )
                                           :
                                           _( "DRC On (Currently: DRC is inactive !!!)" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_MM,
                                      g_UnitMetric == MILLIMETRE ? TRUE : FALSE );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_INCH,
                                      g_UnitMetric == INCHES ? TRUE : FALSE );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                      DisplayOpt.DisplayPolarCood );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                           DisplayOpt.DisplayPolarCood ?
                                           _( "Polar Coords not show" ) :
                                           _( "Display Polar Coords" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_GRID,
                                      m_Draw_Grid );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_GRID,
                                           m_Draw_Grid ? _( "Grid not show" ) : _( "Show Grid" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_CURSOR,
                                      g_CursorShape );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_RATSNEST,
                                      g_Show_Ratsnest );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_RATSNEST,
                                           g_Show_Ratsnest ?
                                           _( "General ratsnest not show" ) : _(
                                               "Show General ratsnest" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_MODULE_RATSNEST,
                                      g_Show_Module_Ratsnest );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_MODULE_RATSNEST,
                                           g_Show_Module_Ratsnest ?
                                           _( "Module ratsnest not show" ) :
                                           _( "Show Module ratsnest" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_AUTO_DEL_TRACK,
                                      g_AutoDeleteOldTrack );

        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_AUTO_DEL_TRACK,
                                           g_AutoDeleteOldTrack ?
                                           _( "Disable Auto Delete old Track" ) :
                                           _( "Enable Auto Delete old Track" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_ZONES,
                                      DisplayOpt.DisplayZones );

        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_ZONES,
                                           DisplayOpt.DisplayZones ?
                                           _( "Do not Show Zones" ) : _( "Show Zones" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                                      !m_DisplayPadFill );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                                           m_DisplayPadFill ?
                                           _( "Show Pads Sketch mode" ) :
                                           _( "Show pads filled mode" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH,
                                      !m_DisplayPcbTrackFill );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_TRACKS_SKETCH,
                                           m_DisplayPcbTrackFill ?
                                           _( "Show Tracks Sketch mode" ) :
                                           _( "Show Tracks filled mode" ) );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_HIGHT_CONTRAST_MODE,
                                      DisplayOpt.ContrastModeDisplay );
        m_OptionsToolBar->SetToolShortHelp( ID_TB_OPTIONS_SHOW_HIGHT_CONTRAST_MODE,
                                           DisplayOpt.ContrastModeDisplay ?
                                           _( "Normal Contrast Mode Display" ) :
                                           _( "Hight Contrast Mode Display" ) );
    }

    if( m_AuxiliaryToolBar )
    {
        wxString msg;
        m_AuxiliaryToolBar->ToggleTool( ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH,
                                        g_DesignSettings.m_UseConnectedTrackWidth );
        if( m_SelTrackWidthBox && m_SelTrackWidthBox_Changed )
        {
            m_SelTrackWidthBox_Changed = FALSE;
            m_SelTrackWidthBox->Clear();
            wxString format = _( "Track" );

            if( g_UnitMetric == INCHES )
                format += wxT( " %.1f" );
            else
                format += wxT( " %.3f" );

            for( ii = 0; ii < HISTORY_NUMBER; ii++ )
            {
                if( g_DesignSettings.m_TrackWidthHistory[ii] == 0 )
                    break; // Fin de liste
                double value = To_User_Unit( g_UnitMetric,
                                             g_DesignSettings.m_TrackWidthHistory[ii],
                                             PCB_INTERNAL_UNIT );

                if( g_UnitMetric == INCHES )
                    msg.Printf( format.GetData(), value * 1000 );
                else
                    msg.Printf( format.GetData(), value );

                m_SelTrackWidthBox->Append( msg );

                if( g_DesignSettings.m_TrackWidthHistory[ii] ==
                    g_DesignSettings.m_CurrentTrackWidth )
                    m_SelTrackWidthBox->SetSelection( ii );
            }
        }

        if( m_SelViaSizeBox && m_SelViaSizeBox_Changed )
        {
            m_SelViaSizeBox_Changed = FALSE;
            m_SelViaSizeBox->Clear();
            wxString format = _( "Via" );

            if( g_UnitMetric == INCHES )
                format += wxT( " %.1f" );
            else
                format += wxT( " %.3f" );

            for( ii = 0; ii < HISTORY_NUMBER; ii++ )
            {
                if( g_DesignSettings.m_ViaSizeHistory[ii] == 0 )
                    break; // Fin de liste

                double value = To_User_Unit( g_UnitMetric,
                                             g_DesignSettings.m_ViaSizeHistory[ii],
                                             PCB_INTERNAL_UNIT );

                if( g_UnitMetric == INCHES )
                    msg.Printf( format.GetData(), value * 1000 );
                else
                    msg.Printf( format.GetData(), value );

                m_SelViaSizeBox->Append( msg );
                if( g_DesignSettings.m_ViaSizeHistory[ii] == g_DesignSettings.m_CurrentViaSize )
                    m_SelViaSizeBox->SetSelection( ii );
            }
        }

        if( m_SelZoomBox )
        {
            int old_choice = m_SelZoomBox->GetChoice();
            int new_choice = 1;
            int zoom;

            for( jj = 1, zoom = 1; zoom <= m_ZoomMaxValue; zoom <<= 1, jj++ )
            {
                if( GetScreen() && (GetScreen()->GetZoom() == zoom) )
                    break;
                new_choice++;
            }

            if( old_choice != new_choice )
                m_SelZoomBox->SetSelection( new_choice );
        }

        if( m_SelGridBox && GetScreen() )
        {
            int kk = m_SelGridBox->GetChoice();

            for( ii = 0; g_GridList[ii].x > 0; ii++ )
            {
                if( !GetScreen()->m_UserGridIsON
                   && (GetScreen()->GetGrid().x == g_GridList[ii].x)
                   && (GetScreen()->GetGrid().y == g_GridList[ii].y) )
                {
                    if( kk != ii )
                        m_SelGridBox->SetSelection( ii );
                    kk = ii;
                    break;
                }
            }

            if( kk != ii )
                m_SelGridBox->SetSelection( ii ); /* User Grid */
        }
    }

    UpdateToolbarLayerInfo();

    PrepareLayerIndicator();

    DisplayUnitsMsg();
}
