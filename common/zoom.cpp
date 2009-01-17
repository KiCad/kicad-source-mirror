/************/
/* zoom.cpp */
/************/

/*
 *  Fonctions de gestion du zoom, du pas de grille et du
 *  recadrage automatique
 */

#include "fctsys.h"

#include "common.h"
#include "macros.h"

#include "bitmaps.h"

#include "id.h"


/**************************************************/
void WinEDA_DrawFrame::Recadre_Trace( bool ToMouse )
/**************************************************/

/** Compute draw offset (scroll bars and draw parameters)
 *  in order to have the current graphic cursor position at the screen center
 *  @param ToMouse if TRUE, the mouse cursor is moved
 *  to the graphic cursor position (which is usually on grid)
 *
 *  Note: Mac OS ** does not ** allow moving mouse cursor by program.
 */
{
    PutOnGrid( &(GetBaseScreen()->m_Curseur) );

    AdjustScrollBars();

    ReDrawPanel();

    /* Move the mouse cursor to the on grid graphic cursor position */
    if( ToMouse == TRUE )
    {
        DrawPanel->MouseToCursorSchema();
    }
}


/************************************************/
void WinEDA_DrawFrame::PutOnGrid( wxPoint* coord )
/************************************************/
/** Adjust the coordinate to the nearest grid value
* @param coord = coordinate to adjust
*/
{
    wxSize grid_size = GetBaseScreen()->GetGrid();

    if( !GetBaseScreen()->m_UserGridIsON )
    {
        coord->x = ( (int) round( (double) coord->x /
                                  (double) grid_size.x ) ) * grid_size.x;
        coord->y = ( (int) round( (double) coord->y /
                                  (double) grid_size.y ) ) * grid_size.y;
    }
}


/**************************************************************/
void WinEDA_DrawFrame::Zoom_Automatique( bool move_mouse_cursor )
/**************************************************************/

/** Redraw the screen with the zoom level which shows all the page or the board
 */
{
    int bestzoom;

    bestzoom = BestZoom();
    GetBaseScreen()->SetZoom( bestzoom );
    Recadre_Trace( move_mouse_cursor );
}


/*************************************************/
void WinEDA_DrawFrame::Window_Zoom( EDA_Rect& Rect )
/*************************************************/

/** Compute the zoom factor and the new draw offset to draw the
 *  selected area (Rect) in full window screen
 *  @param Rect = selected area to show after zooming
 */
{
    int    ii, jj;
    int    bestzoom;
    wxSize size;

    /* Compute the best zoom */
    Rect.Normalize();
    size     = DrawPanel->GetClientSize();
    // Use ceil to at least show the full rect
    ii       = static_cast<int>( ceil(1.0 * Rect.GetSize().x / size.x) );
    jj       = static_cast<int>( ceil(1.0 * Rect.GetSize().y / size.y) );
    bestzoom = MAX( ii, jj );
    if( bestzoom <= 0 )
        bestzoom = 1;

    GetBaseScreen()->SetZoom( bestzoom );

    GetBaseScreen()->m_Curseur = Rect.Centre();
    Recadre_Trace( TRUE );
}


/*****************************************************************/
void WinEDA_DrawFrame::OnZoom( wxCommandEvent& event )
{
    if( DrawPanel == NULL )
    {
        wxLogDebug( wxT( "No DrawPanel object definedin " \
                         "WinEDA_DrawFrame::OnZoom()." ) );
        return;
    }

    bool         zoom_at_cursor = false;
    int          id = event.GetId();
    BASE_SCREEN* screen = GetBaseScreen();

    switch( id )
    {
    case ID_POPUP_ZOOM_IN:
        zoom_at_cursor = true;
        // fall thru

    case ID_ZOOM_IN:
        if( id == ID_ZOOM_IN )
            screen->m_Curseur = DrawPanel->GetScreenCenterRealPosition();
        screen->SetPreviousZoom();
        Recadre_Trace( zoom_at_cursor );
        break;

    case ID_POPUP_ZOOM_OUT:
        zoom_at_cursor = true;
        // fall thru

    case ID_ZOOM_OUT:
        if( id == ID_ZOOM_OUT )
            screen->m_Curseur = DrawPanel->GetScreenCenterRealPosition();
        screen->SetNextZoom();
        Recadre_Trace( zoom_at_cursor );
        break;

    case ID_ZOOM_REDRAW:
        DrawPanel->Refresh();
        break;

    case ID_POPUP_ZOOM_CENTER:
        Recadre_Trace( true );
        break;

    case ID_ZOOM_PAGE:
        Zoom_Automatique( false );
        break;

    case ID_POPUP_ZOOM_SELECT:
        break;

    case ID_POPUP_CANCEL:
        DrawPanel->MouseToCursorSchema();
        break;

    case ID_POPUP_ZOOM_LEVEL_1:
        screen->SetZoom( 1 );
        Recadre_Trace( true );
        break;

    case ID_POPUP_ZOOM_LEVEL_2:
        screen->SetZoom( 2 );
        Recadre_Trace( true );
        break;

    case ID_POPUP_ZOOM_LEVEL_4:
        screen->SetZoom( 4 );
        Recadre_Trace( true );
        break;

    case ID_POPUP_ZOOM_LEVEL_8:
        screen->SetZoom( 8 );
        Recadre_Trace( true );
        break;

    case ID_POPUP_ZOOM_LEVEL_16:
        screen->SetZoom( 16 );
        Recadre_Trace( true );
        break;

    case ID_POPUP_ZOOM_LEVEL_32:
        screen->SetZoom( 32 );
        Recadre_Trace( true );
        break;

    case ID_POPUP_ZOOM_LEVEL_64:
        screen->SetZoom( 64 );
        Recadre_Trace( true );
        break;

    case ID_POPUP_ZOOM_LEVEL_128:
        screen->SetZoom( 128 );
        Recadre_Trace( true );
        break;

    case ID_POPUP_ZOOM_LEVEL_256:
        screen->SetZoom( 256 );
        Recadre_Trace( true );
        break;

    case ID_POPUP_ZOOM_LEVEL_512:
        screen->SetZoom( 512 );
        Recadre_Trace( true );
        break;

    case ID_POPUP_ZOOM_LEVEL_1024:
        screen->SetZoom( 1024 );
        Recadre_Trace( true );
        break;

    case ID_POPUP_ZOOM_LEVEL_2048:
        screen->SetZoom( 2048 );
        Recadre_Trace( true );
        break;

    default:
        wxLogDebug( wxT( "WinEDA_DrawFram::OnZoom() unhandled ID %d" ), id );
        return;
    }

    Affiche_Status_Box();
}

void WinEDA_DrawPanel::OnPopupGridSelect( wxCommandEvent& event )
{
    GetScreen()->SetGrid( event.GetId() );
    Refresh();
}

/*************************************************************/
void WinEDA_DrawPanel::AddMenuZoom( wxMenu* MasterMenu )
/*************************************************************/

/* add the zoom list menu the the MasterMenu.
 *  used in OnRightClick(wxMouseEvent& event)
 */
{
    size_t           i;
    int              zoom;
    wxSize           grid;
    int              zoom_value;
    wxString         msg;
    int              ii;
    wxString         line;
    GRID_TYPE        tmp;

    ADD_MENUITEM( MasterMenu, ID_POPUP_ZOOM_CENTER, _( "Center" ),
                  zoom_center_xpm );
    ADD_MENUITEM( MasterMenu, ID_POPUP_ZOOM_IN, _( "Zoom in" ), zoom_in_xpm );
    ADD_MENUITEM( MasterMenu, ID_POPUP_ZOOM_OUT, _( "Zoom out" ), zoom_out_xpm );
    ADD_MENUITEM( MasterMenu, ID_ZOOM_PAGE, _( "Zoom auto" ), zoom_auto_xpm );

    wxMenu* zoom_choice = new wxMenu;
    ADD_MENUITEM_WITH_SUBMENU( MasterMenu, zoom_choice,
                               ID_POPUP_ZOOM_SELECT, _( "Zoom select" ),
                               zoom_select_xpm );

    ADD_MENUITEM( MasterMenu, ID_ZOOM_REDRAW, _( "Redraw view" ),
                  zoom_redraw_xpm );

    /* Create the basic zoom list: */
    zoom = GetScreen()->GetZoom();
    zoom_value = 1;
    for( ii = 0; zoom_value <= m_Parent->m_ZoomMaxValue; zoom_value <<= 1, ii++ ) // Create zoom choice 1 .. zoom max
    {
        line.Printf( wxT( "%u" ), zoom_value );
        zoom_choice->Append( ID_POPUP_ZOOM_LEVEL_1 + ii,
                             _( "Zoom: " ) + line, wxEmptyString, TRUE );
        if( zoom == zoom_value )
            zoom_choice->Check( ID_POPUP_ZOOM_LEVEL_1 + ii, TRUE );
    }

    /* Create grid submenu as required. */
    if( !GetScreen()->m_GridList.IsEmpty() )
    {
        wxMenu* grid_choice = new wxMenu;
        ADD_MENUITEM_WITH_SUBMENU( MasterMenu, grid_choice,
                                   ID_POPUP_GRID_SELECT, _( "Grid Select" ),
                                   grid_select_xpm );

        grid = GetScreen()->GetGrid();

        for( i = 0; i < GetScreen()->m_GridList.GetCount(); i++ )
        {
            tmp = GetScreen()->m_GridList[i];
            double grid_value = To_User_Unit( g_UnitMetric,
                                              tmp.m_Size.x,
                                              ( (WinEDA_DrawFrame*)m_Parent )->m_InternalUnits );
            if( tmp.m_Id == ID_POPUP_GRID_USER )
            {
                msg = _( "User Grid" );
            }
            else
            {
                if ( g_UnitMetric == 0 )	// inches
                    line.Printf( wxT( "%g mils" ), grid_value*1000 );
                else
                    line.Printf( wxT( "%g mm" ), grid_value );
                msg = _( "Grid: " ) + line;
            }
            grid_choice->Append( tmp.m_Id, msg, wxEmptyString, true );
            if( grid == tmp.m_Size )
                grid_choice->Check( tmp.m_Id, true );
        }
    }

    MasterMenu->AppendSeparator();
    ADD_MENUITEM( MasterMenu, ID_POPUP_CANCEL, _( "Close" ), cancel_xpm );
}
