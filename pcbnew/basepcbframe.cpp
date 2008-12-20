/************************************************************************/
/* basepcbframe.cpp - fonctions des classes du type WinEDA_BasePcbFrame */
/************************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"

#include "pcbnew.h"

#include "bitmaps.h"
#include "protos.h"
#include "id.h"

#include "collectors.h"


/*******************************/
/* class WinEDA_BasePcbFrame */
/*******************************/

BEGIN_EVENT_TABLE( WinEDA_BasePcbFrame, WinEDA_DrawFrame )
    COMMON_EVENTS_DRAWFRAME
    EVT_MENU_RANGE( ID_POPUP_PCB_ITEM_SELECTION_START,
                    ID_POPUP_PCB_ITEM_SELECTION_END,
                    WinEDA_BasePcbFrame::ProcessItemSelection )
END_EVENT_TABLE()


/****************/
/* Constructeur */
/****************/

WinEDA_BasePcbFrame::WinEDA_BasePcbFrame( wxWindow*       father,
                                          int             idtype,
                                          const wxString& title,
                                          const wxPoint&  pos,
                                          const wxSize&   size,
                                          long style) :
    WinEDA_DrawFrame( father, idtype, title, pos, size, style )
{
    m_InternalUnits = 10000;        // Internal unit = 1/10000 inch
    m_Pcb = NULL;

    m_DisplayPadFill = TRUE;        // How to draw pads
    m_DisplayPadNum  = TRUE;        // show pads number

    m_DisplayModEdge      = FILLED; // How to show module drawings
    m_DisplayModText      = FILLED; // How to show module texts
    m_DisplayPcbTrackFill = TRUE;   /* FALSE = sketch , TRUE = filled */
    m_Draw3DFrame         = NULL;   // Display Window in 3D mode (OpenGL)
    m_ModuleEditFrame     = NULL;   // Frame for footprint edition

    m_Collector = new GENERAL_COLLECTOR();
}


WinEDA_BasePcbFrame::~WinEDA_BasePcbFrame( void )
{
    delete m_Collector;
}


BASE_SCREEN* WinEDA_BasePcbFrame::GetBaseScreen() const
{
    return GetScreen();
}


void WinEDA_BasePcbFrame::SetBOARD( BOARD* aBoard )
{
    if(m_Pcb != g_ModuleEditor_Pcb)
        delete m_Pcb;
    m_Pcb = aBoard;
}


/**************************************/
int WinEDA_BasePcbFrame::BestZoom( void )
/**************************************/
/**
 * Return the "best" zoom, i.e. the zoom which shows the entire board on screen
 */
{
    int    dx, dy, ii, jj;
    int    bestzoom;
    wxSize size;

    if( m_Pcb == NULL )
        return 32;

    m_Pcb->ComputeBoundaryBox();

    dx = m_Pcb->m_BoundaryBox.GetWidth();
    dy = m_Pcb->m_BoundaryBox.GetHeight();

    size     = DrawPanel->GetClientSize();
    ii       = ( dx + (size.x / 2) ) / size.x;
    jj       = ( dy + (size.y / 2) ) / size.y;
    bestzoom = MAX( ii, jj ) + 1;

    GetScreen()->m_Curseur = m_Pcb->m_BoundaryBox.Centre();

    return bestzoom;
}


/***********************************************************/
void WinEDA_BasePcbFrame::CursorGoto(  const wxPoint& aPos )
/***********************************************************/
{
    // factored out of pcbnew/find.cpp

    PCB_SCREEN* screen = (PCB_SCREEN*)GetScreen();

    wxClientDC dc( DrawPanel );

    /* Il y a peut-etre necessite de recadrer le dessin: */
    if( !DrawPanel->IsPointOnDisplay( aPos ) )
    {
        screen->m_Curseur = aPos;
        Recadre_Trace( TRUE );
    }
    else
    {
        // Put cursor on item position
        DrawPanel->CursorOff( &dc );
        screen->m_Curseur = aPos;
        GRMouseWarp( DrawPanel, screen->m_Curseur );
        DrawPanel->MouseToCursorSchema();
        DrawPanel->CursorOn( &dc );
    }
}


/*************************************************/
void WinEDA_BasePcbFrame::ReCreateMenuBar( void )
/*************************************************/

// Virtual function
{
}

#ifdef CVPCB
/********************************************************************/
void WinEDA_BasePcbFrame::GeneralControle( wxDC* DC, wxPoint Mouse )
/********************************************************************/

// Virtual function
{
}
#endif

#include "3d_viewer.h"

/***********************************************************/
void WinEDA_BasePcbFrame::Show3D_Frame( wxCommandEvent& event )
/***********************************************************/

/* Creates and shows the 3D frame display
 */
{
#ifndef GERBVIEW

    // Create the main frame window
    if( m_Draw3DFrame )
    {
        DisplayInfo( this, _( "3D Frame already opened" ) );
        return;
    }

#ifdef CVPCB
    m_Draw3DFrame = new WinEDA3D_DrawFrame( this, _( "3D Viewer" ),
    KICAD_DEFAULT_3D_DRAWFRAME_STYLE | wxFRAME_FLOAT_ON_PARENT );
#else
    m_Draw3DFrame = new WinEDA3D_DrawFrame( this, _( "3D Viewer" ) );
#endif
    // Show the frame
    m_Draw3DFrame->Show( TRUE );
#endif
}


/* Virtual functions: Do nothing for WinEDA_BasePcbFrame window */

/***********************************************************************************/
void WinEDA_BasePcbFrame::SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy, int flag )
/***********************************************************************************/
{
}


/********************************************************/
void WinEDA_BasePcbFrame::GetComponentFromUndoList( void )
/********************************************************/
{
}


/********************************************************/
void WinEDA_BasePcbFrame::GetComponentFromRedoList( void )
/********************************************************/
{
}


/****************************************************************/
void WinEDA_BasePcbFrame::SwitchLayer( wxDC* DC, int layer )
/*****************************************************************/

// Note: virtual, overridden in WinEDA_PcbFrame;
{
    int preslayer = ((PCB_SCREEN*)GetScreen())->m_Active_Layer;

    // Check if the specified layer matches the present layer
    if( layer == preslayer )
        return;

    // Copper layers cannot be selected unconditionally; how many
    // of those layers are currently enabled needs to be checked.
    if( (layer >= FIRST_COPPER_LAYER) && (layer <= LAST_COPPER_LAYER) )
    {
        // If only one copper layer is enabled, the only such layer
        // that can be selected to is the "Copper" layer (so the
        // selection of any other copper layer is disregarded).
        if( m_Pcb->m_BoardSettings->m_CopperLayerCount < 2 )
        {
            if( layer != COPPER_LAYER_N )
            {
                return;
            }
        }

        // If more than one copper layer is enabled, the "Copper"
        // and "Component" layers can be selected, but the total
        // number of copper layers determines which internal
        // layers are also capable of being selected.
        else
        {
            if( (layer != COPPER_LAYER_N) && (layer != LAYER_CMP_N)
                && (layer >= m_Pcb->m_BoardSettings->m_CopperLayerCount - 1) )
            {
                return;
            }
        }
    }

    // Is yet more checking required? E.g. when the layer to be selected
    // is a non-copper layer, or when switching between a copper layer
    // and a non-copper layer, or vice-versa?
    // ...

    ((PCB_SCREEN*)GetScreen())->m_Active_Layer = layer;

    if( DisplayOpt.ContrastModeDisplay )
        GetScreen()->SetRefreshReq();
}


/**********************************************************************/
void WinEDA_BasePcbFrame::ProcessItemSelection( wxCommandEvent& event )
/**********************************************************************/
{
    int id = event.GetId();

    // index into the collector list:
    int itemNdx = id - ID_POPUP_PCB_ITEM_SELECTION_START;

    if( id >= ID_POPUP_PCB_ITEM_SELECTION_START
     && id <= ID_POPUP_PCB_ITEM_SELECTION_END )
    {
        BOARD_ITEM* item = (*m_Collector)[itemNdx];
        DrawPanel->m_AbortRequest = false;

#if 0 && defined (DEBUG)
        item->Show( 0, std::cout );
#endif

        SetCurItem( item );
    }
}


/*****************************************************************/
void WinEDA_BasePcbFrame::SetCurItem( BOARD_ITEM* aItem )
/*****************************************************************/
{
    GetScreen()->SetCurItem( aItem );

    if( aItem )
    {
        aItem->Display_Infos( this );

#if 0 && defined(DEBUG)
    aItem->Show( 0, std::cout );
#endif

    }
    else
    {
        // we can use either of these two:

        //MsgPanel->EraseMsgBox();
        m_Pcb->Display_Infos( this );       // show the BOARD stuff

#if 0 && defined(DEBUG)
        std::cout << "SetCurItem(NULL)\n";
#endif

    }
}


/*****************************************************************/
BOARD_ITEM* WinEDA_BasePcbFrame::GetCurItem()
/*****************************************************************/
{
    return GetScreen()->GetCurItem();
}


/****************************************************************/
GENERAL_COLLECTORS_GUIDE WinEDA_BasePcbFrame::GetCollectorsGuide()
/****************************************************************/
{
    GENERAL_COLLECTORS_GUIDE guide( m_Pcb->m_BoardSettings->GetVisibleLayers(),
                                    ((PCB_SCREEN*)GetScreen())->m_Active_Layer );

    // account for the globals
    guide.SetIgnoreMTextsMarkedNoShow( g_ModuleTextNOVColor & ITEM_NOT_SHOW );
    guide.SetIgnoreMTextsOnCopper( g_ModuleTextCUColor & ITEM_NOT_SHOW );
    guide.SetIgnoreMTextsOnCmp( g_ModuleTextCMPColor & ITEM_NOT_SHOW );
    guide.SetIgnoreModulesOnCu( !DisplayOpt.Show_Modules_Cu );
    guide.SetIgnoreModulesOnCmp( !DisplayOpt.Show_Modules_Cmp );

    return guide;
}

void WinEDA_BasePcbFrame::SetToolID( int id, int new_cursor_id,
                                     const wxString& title )
{
    bool redraw = false;

    WinEDA_DrawFrame::SetToolID( id, new_cursor_id, title );

    if( id < 0 )
        return;

    // handle color changes for transitions in and out of ID_TRACK_BUTT
    if( ( m_ID_current_state == ID_TRACK_BUTT && id != ID_TRACK_BUTT )
        || ( m_ID_current_state != ID_TRACK_BUTT && id == ID_TRACK_BUTT ) )
    {
        if( DisplayOpt.ContrastModeDisplay )
            redraw = true;
    }

    // must do this after the tool has been set, otherwise pad::Draw() does
    // not show proper color when DisplayOpt.ContrastModeDisplay is true.
    if( redraw && DrawPanel)
        DrawPanel->Refresh();
}

void WinEDA_BasePcbFrame::Affiche_Status_Box()
{
    wxString        Line;
    int             dx, dy;
    double          theta, ro;
    BASE_SCREEN*    screen = GetBaseScreen();

    if( !screen )
        return;

    WinEDA_DrawFrame::Affiche_Status_Box();

    dx = screen->m_Curseur.x - screen->m_O_Curseur.x;
    dy = screen->m_Curseur.y - screen->m_O_Curseur.y;

    if( DisplayOpt.DisplayPolarCood )  /* Display coordonnee polaire */
    {
        if( (dx == 0) && (dy == 0) )
            theta = 0.0;
        else
            theta = atan2( (double) -dy, (double) dx );

        theta = theta * 180.0 / M_PI;

        ro = sqrt( ( (double) dx * dx ) + ( (double) dy * dy ) );
        Line.Printf( g_UnitMetric ? wxT( "Ro %.3f Th %.1f" ) : wxT( "Ro %.4f Th %.1f" ),
                     To_User_Unit( g_UnitMetric, (int) round( ro ), m_InternalUnits ),
                     theta );
    }

    SetStatusText( Line, 0 );
}
