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
    GetBaseScreen()->SetZoom( BestZoom() );
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
    double   scalex, bestscale;
    wxSize size;

    /* Compute the best zoom */
    Rect.Normalize();
    size     = DrawPanel->GetClientSize();
    // Use ceil to at least show the full rect
    scalex       = (double) Rect.GetSize().x / size.x;
    bestscale       = (double)Rect.GetSize().y / size.y;
    bestscale = MAX( bestscale, scalex );

    GetBaseScreen()->SetScalingFactor( bestscale );
    GetBaseScreen()->m_Curseur = Rect.Centre();
    Recadre_Trace( TRUE );
}


/*****************************************************************/
void WinEDA_DrawFrame::OnZoom( wxCommandEvent& event )
{
    if( DrawPanel == NULL )
    {
        wxLogDebug( wxT( "No DrawPanel object defined in " \
                         "WinEDA_DrawFrame::OnZoom()." ) );
        return;
    }

    int          i;
    int          id = event.GetId();
    bool         zoom_at_cursor = false;
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

    default:
        i = id - ID_POPUP_ZOOM_LEVEL_START;

        if( i < 0 )
        {
            wxLogDebug( wxT( "WinEDA_DrawFram::OnZoom() invalid ID %d" ), id );
            return;
        }
        if( !( (size_t) i < screen->m_ZoomList.GetCount()) )
        {
            wxLogDebug( _T( "Requested index %d is outside the bounds of " \
                            "the zoom list." ), i );
            return;
        }
        screen->SetZoom( screen->m_ZoomList[i] );
        Recadre_Trace( true );
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
    size_t      i;
    int         maxZoomIds;
    int         zoom;
    wxSize      grid;
    wxString    msg;
    GRID_TYPE   tmp;
    wxMenu*     gridMenu;
    double      gridValue;

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

    zoom = GetScreen()->GetZoom();
    maxZoomIds = ID_POPUP_ZOOM_LEVEL_END - ID_POPUP_ZOOM_LEVEL_START;
    maxZoomIds = ( (size_t) maxZoomIds < GetScreen()->m_ZoomList.GetCount() ) ?
        maxZoomIds : GetScreen()->m_ZoomList.GetCount();
    wxLogDebug( _T( "%d zoom IDs used." ), maxZoomIds );

    /* Populate zoom submenu. */
    for( i = 0; i < (size_t) maxZoomIds; i++ )
    {
        if ( (GetScreen()->m_ZoomList[i] % GetScreen()->m_ZoomScalar) == 0 )
            msg.Printf( wxT( "%u" ), GetScreen()->m_ZoomList[i] / GetScreen()->m_ZoomScalar);
        else
            msg.Printf(wxT("%.1f"),(float)GetScreen()->m_ZoomList[i] / GetScreen()->m_ZoomScalar );

        zoom_choice->Append( ID_POPUP_ZOOM_LEVEL_START + i, _( "Zoom: " ) + msg,
                             wxEmptyString, wxITEM_CHECK );
        if( zoom == GetScreen()->m_ZoomList[i] )
            zoom_choice->Check( ID_POPUP_ZOOM_LEVEL_START + i, true );
    }

    /* Create grid submenu as required. */
    if( !GetScreen()->m_GridList.IsEmpty() )
    {
        gridMenu = new wxMenu;
        ADD_MENUITEM_WITH_SUBMENU( MasterMenu, gridMenu,
                                   ID_POPUP_GRID_SELECT, _( "Grid Select" ),
                                   grid_select_xpm );

        grid = GetScreen()->GetGrid();

        for( i = 0; i < GetScreen()->m_GridList.GetCount(); i++ )
        {
            tmp = GetScreen()->m_GridList[i];
            gridValue = To_User_Unit( g_UnitMetric, tmp.m_Size.x,
                                      ( (WinEDA_DrawFrame*)m_Parent )->m_InternalUnits );
            if( tmp.m_Id == ID_POPUP_GRID_USER )
            {
                msg = _( "User Grid" );
            }
            else
            {
                if ( g_UnitMetric == 0 )	// inches
                    msg.Printf( wxT( "%g mils" ), gridValue * 1000 );
                else
                    msg.Printf( wxT( "%g mm" ), gridValue );
                msg = _( "Grid: " ) + msg;
            }
            gridMenu->Append( tmp.m_Id, msg, wxEmptyString, true );
            if( grid == tmp.m_Size )
                gridMenu->Check( tmp.m_Id, true );
        }
    }

    MasterMenu->AppendSeparator();
    ADD_MENUITEM( MasterMenu, ID_POPUP_CANCEL, _( "Close" ), cancel_xpm );
}
