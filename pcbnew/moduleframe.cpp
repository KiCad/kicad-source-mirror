/***************************************************************************/
/* moduleframe.cpp - fonctions de base de la classe WinEDA_ModuleEditFrame */
/***************************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"

#include "pcbnew.h"

#include "bitmaps.h"
#include "protos.h"
#include "id.h"

/********************************/
/* class WinEDA_ModuleEditFrame */
/********************************/
BEGIN_EVENT_TABLE( WinEDA_ModuleEditFrame, wxFrame )
COMMON_EVENTS_DRAWFRAME EVT_MENU_RANGE( ID_POPUP_PCB_ITEM_SELECTION_START,
                                        ID_POPUP_PCB_ITEM_SELECTION_END,
                                        WinEDA_BasePcbFrame::ProcessItemSelection )
COMMON_EVENTS_DRAWFRAME EVT_CLOSE( WinEDA_ModuleEditFrame::OnCloseWindow )
EVT_SIZE( WinEDA_ModuleEditFrame::OnSize )

EVT_KICAD_CHOICEBOX( ID_ON_ZOOM_SELECT, WinEDA_PcbFrame::OnSelectZoom )
EVT_KICAD_CHOICEBOX( ID_ON_GRID_SELECT, WinEDA_PcbFrame::OnSelectGrid )

EVT_TOOL_RANGE( ID_ZOOM_PLUS_BUTT, ID_ZOOM_PAGE_BUTT,
                WinEDA_ModuleEditFrame::Process_Zoom )

EVT_TOOL( ID_LIBEDIT_SELECT_CURRENT_LIB, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_SAVE_LIBMODULE, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_LIBEDIT_DELETE_PART, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_NEW_MODULE, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_LOAD_MODULE, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_LIBEDIT_IMPORT_PART, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_LIBEDIT_EXPORT_PART, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_LIBEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART,
          WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_SHEET_SET, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_GEN_PRINT, WinEDA_DrawFrame::ToPrinter )
EVT_TOOL( ID_MODEDIT_LOAD_MODULE, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_CHECK, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_PAD_SETTINGS, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_INSERT_MODULE_IN_BOARD, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_UPDATE_MODULE_IN_BOARD, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_EDIT_MODULE_PROPERTIES, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_UNDO, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_REDO, WinEDA_ModuleEditFrame::Process_Special_Functions )

// Vertical toolbar (left click):
EVT_TOOL( ID_NO_SELECT_BUTT, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_ADD_PAD, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_ARC_BUTT, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_PCB_CIRCLE_BUTT, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_TEXT_COMMENT_BUTT, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_LINE_COMMENT_BUTT, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_DELETE_ITEM_BUTT, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_TOOL( ID_MODEDIT_PLACE_ANCHOR, WinEDA_ModuleEditFrame::Process_Special_Functions )

// Vertical toolbar (right click):
EVT_TOOL_RCLICKED( ID_MODEDIT_ADD_PAD, WinEDA_ModuleEditFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_TRACK_BUTT, WinEDA_ModuleEditFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_PCB_CIRCLE_BUTT, WinEDA_ModuleEditFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_PCB_ARC_BUTT, WinEDA_ModuleEditFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_TEXT_COMMENT_BUTT, WinEDA_ModuleEditFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_LINE_COMMENT_BUTT, WinEDA_ModuleEditFrame::ToolOnRightClick )
EVT_TOOL_RCLICKED( ID_PCB_COTATION_BUTT, WinEDA_ModuleEditFrame::ToolOnRightClick )

// Options Toolbar
EVT_TOOL_RANGE( ID_TB_OPTIONS_START, ID_TB_OPTIONS_END,
                WinEDA_ModuleEditFrame::OnSelectOptionToolbar )

EVT_MENU_RANGE( ID_POPUP_PCB_START_RANGE, ID_POPUP_PCB_END_RANGE,
                WinEDA_ModuleEditFrame::Process_Special_Functions )

// Annulation de commande en cours
EVT_MENU_RANGE( ID_POPUP_GENERAL_START_RANGE, ID_POPUP_GENERAL_END_RANGE,
                WinEDA_PcbFrame::Process_Special_Functions )

// Transformations du module
EVT_MENU( ID_MODEDIT_MODULE_ROTATE, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_MENU( ID_MODEDIT_MODULE_MIRROR, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_MENU( ID_MODEDIT_MODULE_SCALE, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_MENU( ID_MODEDIT_MODULE_SCALEX, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_MENU( ID_MODEDIT_MODULE_SCALEY, WinEDA_ModuleEditFrame::Process_Special_Functions )

EVT_MENU( ID_PCB_DRAWINGS_WIDTHS_SETUP, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_MENU( ID_PCB_PAD_SETUP, WinEDA_ModuleEditFrame::Process_Special_Functions )
EVT_MENU( ID_PCB_USER_GRID_SETUP, WinEDA_PcbFrame::Process_Special_Functions )

// Menu 3D Frame
EVT_MENU( ID_MENU_PCB_SHOW_3D_FRAME, WinEDA_ModuleEditFrame::Show3D_Frame )

// PopUp Menu Zoom trait�s dans drawpanel.cpp

END_EVENT_TABLE()


/****************/
/* Constructeur */
/****************/

WinEDA_ModuleEditFrame::WinEDA_ModuleEditFrame( wxWindow* father, WinEDA_App* parent,
                                                const wxString& title,
                                                const wxPoint& pos, const wxSize& size,
												long style ) :
    WinEDA_BasePcbFrame( father, parent, MODULE_EDITOR_FRAME, wxEmptyString, pos, size, style )
{
    m_FrameName      = wxT( "ModEditFrame" );
    m_Draw_Axis      = TRUE;    // TRUE pour avoir les axes dessines
    m_Draw_Grid      = TRUE;    // TRUE pour avoir la axes dessinee
    m_Draw_Sheet_Ref = FALSE;   // TRUE pour avoir le cartouche dessin�
    m_ZoomMaxValue   = 1024;

    // Give an icon
    SetIcon( wxICON( icon_modedit ) );

    SetTitle( wxT( "Module Editor (lib: " ) + m_CurrentLib + wxT( ")" ) );

    if( ScreenModule == NULL )
    {
        ScreenModule = new PCB_SCREEN( PCB_FRAME );
        ActiveScreen = ScreenModule;
    }
    ScreenModule->m_UndoRedoCountMax = 10;

    if( g_ModuleEditor_Pcb == NULL )
        g_ModuleEditor_Pcb = new BOARD( NULL, this );
    m_Pcb = g_ModuleEditor_Pcb;

    m_Pcb->m_PcbFrame = this;
    m_CurrentScreen   = ScreenModule;
    SetCurItem( NULL );
    GetSettings();

    wxSize GridSize( 500, 500 );
    if( m_Parent && m_Parent->m_EDA_Config )
    {
        long SizeX, SizeY;
        if( m_Parent->m_EDA_Config->Read( wxT( "ModEditGrid_X" ), &SizeX )
           && m_Parent->m_EDA_Config->Read( wxT( "ModEditGrid_Y" ), &SizeY ) )
        {
            GridSize.x = SizeX;
            GridSize.y = SizeY;
        }
    }
    GetScreen()->SetGrid( GridSize );

    SetSize( m_FramePos.x, m_FramePos.y, m_FrameSize.x, m_FrameSize.y );
    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateAuxiliaryToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    if( DrawPanel )
        DrawPanel->m_Block_Enable = TRUE;
}


/****************************************************/
WinEDA_ModuleEditFrame::~WinEDA_ModuleEditFrame()
/****************************************************/
{
    m_Parent->m_ModuleEditFrame = NULL;
    m_CurrentScreen = ScreenPcb;
}


/**************************************************************/
void WinEDA_ModuleEditFrame::OnCloseWindow( wxCloseEvent& Event )
/**************************************************************/
{
    if( GetScreen()->IsModify() )
    {
        if( !IsOK( this, _( "Module Editor: module modified!, Continue ?" ) ) )
        {
            Event.Veto(); return;
        }
    }

    SaveSettings();
    if( m_Parent && m_Parent->m_EDA_Config )
    {
        wxSize GridSize = GetScreen()->GetGrid();
        m_Parent->m_EDA_Config->Write( wxT( "ModEditGrid_X" ), (long) GridSize.x );
        m_Parent->m_EDA_Config->Write( wxT( "ModEditGrid_Y" ), (long) GridSize.y );
    }
    Destroy();
}


/*********************************************/
void WinEDA_ModuleEditFrame::SetToolbars()
/*********************************************/
{
    bool active, islib = TRUE;

    if( m_HToolBar == NULL )
        return;

    if( m_CurrentLib == wxEmptyString )
        islib = FALSE;

    m_HToolBar->EnableTool( ID_MODEDIT_SAVE_LIBMODULE, islib );
    m_HToolBar->EnableTool( ID_LIBEDIT_DELETE_PART, islib );

    if( m_Pcb->m_Modules == NULL )
        active = FALSE;
    else
        active = TRUE;

    m_HToolBar->EnableTool( ID_LIBEDIT_EXPORT_PART, active );
    m_HToolBar->EnableTool( ID_LIBEDIT_CREATE_NEW_LIB_AND_SAVE_CURRENT_PART, active );
    m_HToolBar->EnableTool( ID_MODEDIT_SAVE_LIBMODULE, active && islib );
    MODULE* module_in_edit = m_Pcb->m_Modules;
    if( module_in_edit && module_in_edit->m_Link ) // this is not a new module ...
    {
        WinEDA_PcbFrame* pcbframe      = m_Parent->m_PcbFrame;
        BOARD*           mainpcb       = pcbframe->m_Pcb;
        MODULE*          source_module = mainpcb->m_Modules;

        // search if the source module was not deleted:
        for(  ; source_module != NULL; source_module = (MODULE*) source_module->Pnext )
        {
            if( module_in_edit->m_Link == source_module->m_TimeStamp )
                break;
        }

        if( source_module )
        {
            m_HToolBar->EnableTool( ID_MODEDIT_INSERT_MODULE_IN_BOARD, false );
            m_HToolBar->EnableTool( ID_MODEDIT_UPDATE_MODULE_IN_BOARD, true );
        }
        else    // The source was deleted, therefore we can insert but not update the module
        {
            m_HToolBar->EnableTool( ID_MODEDIT_INSERT_MODULE_IN_BOARD, true );
            m_HToolBar->EnableTool( ID_MODEDIT_UPDATE_MODULE_IN_BOARD, false );
        }
    }
    else
    {
        m_HToolBar->EnableTool( ID_MODEDIT_INSERT_MODULE_IN_BOARD, active );
        m_HToolBar->EnableTool( ID_MODEDIT_UPDATE_MODULE_IN_BOARD, false );
    }

    if( GetScreen() )
    {
        m_HToolBar->EnableTool( ID_MODEDIT_UNDO, GetScreen()->m_UndoList && active );
        m_HToolBar->EnableTool( ID_MODEDIT_REDO, GetScreen()->m_RedoList && active );
    }

    if( m_Parent->m_PcbFrame->m_Pcb->m_Modules )
    {
        m_HToolBar->EnableTool( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, TRUE );
    }
    else
    {
        m_HToolBar->EnableTool( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, FALSE );
    }


    if( m_VToolBar )
    {
        m_VToolBar->EnableTool( ID_MODEDIT_ADD_PAD, active );
        m_VToolBar->EnableTool( ID_LINE_COMMENT_BUTT, active );
        m_VToolBar->EnableTool( ID_PCB_CIRCLE_BUTT, active );
        m_VToolBar->EnableTool( ID_PCB_ARC_BUTT, active );
        m_VToolBar->EnableTool( ID_TEXT_COMMENT_BUTT, active );
        m_VToolBar->EnableTool( ID_MODEDIT_PLACE_ANCHOR, active );
        m_VToolBar->EnableTool( ID_PCB_DELETE_ITEM_BUTT, active );
    }

    if( m_OptionsToolBar )
    {
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_MM,
                                      g_UnitMetric == MILLIMETRE ? TRUE : FALSE );
        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_UNIT_INCH,
                                      g_UnitMetric == INCHES ? TRUE : FALSE );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                                      DisplayOpt.DisplayPolarCood );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_GRID,
                                      m_Draw_Grid );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SELECT_CURSOR,
                                      g_CursorShape );

        m_OptionsToolBar->ToggleTool( ID_TB_OPTIONS_SHOW_PADS_SKETCH,
                                      !m_DisplayPadFill );
    }

    if( m_AuxiliaryToolBar )
    {
        int ii, jj;
        if( m_SelZoomBox )
        {
            int old_choice = m_SelZoomBox->GetChoice();
            int new_choice = 1;
            int zoom;
            for( jj = 1, zoom = 1; zoom <= m_ZoomMaxValue; zoom <<= 1, jj++ )
            {
                if( m_CurrentScreen && (m_CurrentScreen->GetZoom() == zoom) )
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
                m_SelGridBox->SetSelection( ii );/* User Grid */
        }
    }

    DisplayUnitsMsg();
}
