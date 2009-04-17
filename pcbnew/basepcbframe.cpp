/************************************************************************/
/* basepcbframe.cpp - fonctions des classes du type WinEDA_BasePcbFrame */
/************************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "appl_wxstruct.h"

#include "pcbnew.h"
#include "bitmaps.h"
#include "protos.h"
#include "id.h"

#include "collectors.h"
#include "class_drawpanel.h"


/* Configuration entry names. */
static const wxString UserGridSizeXEntry( wxT( "PcbUserGrid_X" ) );
static const wxString UserGridSizeYEntry( wxT( "PcbUserGrid_Y" ) );
static const wxString UserGridUnitsEntry( wxT( "PcbUserGrid_Unit" ) );

/*******************************/
/* class WinEDA_BasePcbFrame */
/*******************************/

BEGIN_EVENT_TABLE( WinEDA_BasePcbFrame, WinEDA_DrawFrame )
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
    m_InternalUnits       = PCB_INTERNAL_UNIT;  // Internal unit = 1/10000 inch
    m_Pcb                 = NULL;

    m_DisplayPadFill      = true;   // How to draw pads
    m_DisplayPadNum       = true;   // show pads number

    m_DisplayModEdge      = FILLED; // How to show module drawings
    m_DisplayModText      = FILLED; // How to show module texts
    m_DisplayPcbTrackFill = true;   /* FALSE = sketch , true = filled */
    m_Draw3DFrame         = NULL;   // Display Window in 3D mode (OpenGL)
    m_ModuleEditFrame     = NULL;   // Frame for footprint edition

    m_UserGridSize        = wxRealPoint( 100.0, 100.0 );
    m_UserGridUnits       = INCHES;

    m_Collector           = new GENERAL_COLLECTOR();
}


WinEDA_BasePcbFrame::~WinEDA_BasePcbFrame()
{
    delete m_Collector;
}


BASE_SCREEN* WinEDA_BasePcbFrame::GetBaseScreen() const
{
    return GetScreen();
}


void WinEDA_BasePcbFrame::SetBoard( BOARD* aBoard )
{
    if( m_Pcb != g_ModuleEditor_Pcb )
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
        return 32 * GetScreen()->m_ZoomScalar;

    m_Pcb->ComputeBoundaryBox();

    dx = m_Pcb->m_BoundaryBox.GetWidth();
    dy = m_Pcb->m_BoundaryBox.GetHeight();

    size     = DrawPanel->GetClientSize();
    ii       = ( dx + (size.x / 2) ) / size.x;
    jj       = ( dy + (size.y / 2) ) / size.y;
    bestzoom = MAX( ii, jj ) + 1;
    GetScreen()->m_Curseur = m_Pcb->m_BoundaryBox.Centre();

    return bestzoom * GetScreen()->m_ZoomScalar;
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
        Recadre_Trace( true );
    }
    else
    {
        // Put cursor on item position
        DrawPanel->CursorOff( &dc );
        screen->m_Curseur = aPos;
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

/* Virtual functions: Do nothing for WinEDA_BasePcbFrame window */

void WinEDA_BasePcbFrame::Show3D_Frame( wxCommandEvent& event )
{
}


void WinEDA_BasePcbFrame::SaveCopyInUndoList( EDA_BaseStruct* ItemToCopy,
                                              int flag )
{
}


void WinEDA_BasePcbFrame::GetComponentFromUndoList( void )
{
}


void WinEDA_BasePcbFrame::GetComponentFromRedoList( void )
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
        aItem->DisplayInfo( this );

#if 0 && defined(DEBUG)
    aItem->Show( 0, std::cout );
#endif

    }
    else
    {
        // we can use either of these two:

        //MsgPanel->EraseMsgBox();
        m_Pcb->DisplayInfo( this );       // show the BOARD stuff

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

void WinEDA_BasePcbFrame::UpdateStatusBar()
/*
 * Update the status bar information.
 */
{
    WinEDA_DrawFrame::UpdateStatusBar();

    if( DisplayOpt.DisplayPolarCood )  // display polar coordinates
    {
        BASE_SCREEN*    screen = GetBaseScreen();
        if( !screen )
            return;

        wxString        Line;
        double          theta, ro;

        int dx = screen->m_Curseur.x - screen->m_O_Curseur.x;
        int dy = screen->m_Curseur.y - screen->m_O_Curseur.y;

        if( dx==0 && dy==0 )
            theta = 0.0;
        else
            theta = atan2( (double) -dy, (double) dx );

        theta = theta * 180.0 / M_PI;

        ro = sqrt( ( (double) dx * dx ) + ( (double) dy * dy ) );
        Line.Printf( g_UnitMetric ? wxT( "Ro %.3f Th %.1f" ) : wxT( "Ro %.4f Th %.1f" ),
                     To_User_Unit( g_UnitMetric, ro, m_InternalUnits ),
                     theta );

        // overwrite the absolute cartesian coordinates
        SetStatusText( Line, 2 );
    }

    /*  not this, because status field no. 0 is reserved for actual fleeting
        status information.  If this is enabled, then that text is erased on
        every DrawPanel redraw.  Field no. 0 is set with Affiche_Message() and it
        should persist until called again.
    SetStatusText( Line, 0 );
    */
}


/**
 * Load PCB base frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get loaded.
 */
void WinEDA_BasePcbFrame::LoadSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    WinEDA_DrawFrame::LoadSettings();
    cfg->Read( m_FrameName + UserGridSizeXEntry, &m_UserGridSize.x, 0.01 );
    cfg->Read( m_FrameName + UserGridSizeYEntry, &m_UserGridSize.y, 0.01 );
    cfg->Read( m_FrameName + UserGridUnitsEntry, &m_UserGridUnits,
               ( long )INCHES );
}


/**
 * Save PCB base frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get saved.
 */
void WinEDA_BasePcbFrame::SaveSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    WinEDA_DrawFrame::SaveSettings();
    cfg->Write( m_FrameName + UserGridSizeXEntry, m_UserGridSize.x );
    cfg->Write( m_FrameName + UserGridSizeYEntry, m_UserGridSize.y );
    cfg->Write( m_FrameName + UserGridUnitsEntry, ( long )m_UserGridUnits );
}
