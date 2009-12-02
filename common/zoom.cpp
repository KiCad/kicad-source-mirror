/************/
/* zoom.cpp */
/************/

/*
 * Manage zoom, grid step, and auto crop.
 */

#include "fctsys.h"
#include "common.h"
#include "macros.h"
#include "bitmaps.h"
#include "id.h"
#include "class_drawpanel.h"
#include "class_base_screen.h"
#include "wxstruct.h"


/** Compute draw offset (scroll bars and draw parameters)
 *  in order to have the current graphic cursor position at the screen center
 *  @param ToMouse if TRUE, the mouse cursor is moved
 *  to the graphic cursor position (which is usually on grid)
 *
 *  Note: Mac OS ** does not ** allow moving mouse cursor by program.
 */
void WinEDA_DrawFrame::Recadre_Trace( bool ToMouse )
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


/** Adjust the coordinate to the nearest grid value
* @param coord = coordinate to adjust
*/
void WinEDA_DrawFrame::PutOnGrid( wxPoint* coord )
{
    wxRealPoint grid_size = GetBaseScreen()->GetGridSize();

    if( !GetBaseScreen()->m_UserGridIsON )
    {
        int tmp = wxRound( coord->x / grid_size.x );
        coord->x = wxRound( tmp * grid_size.x );

        tmp = wxRound( coord->y / grid_size.y );
        coord->y = wxRound ( tmp * grid_size.y );
    }
}


/** Redraw the screen with the zoom level which shows all the page or the board
 */
void WinEDA_DrawFrame::Zoom_Automatique( bool move_mouse_cursor )
{
    if( GetBaseScreen()->SetZoom( BestZoom() ) )
        Recadre_Trace( move_mouse_cursor );
}


/** Compute the zoom factor and the new draw offset to draw the
 *  selected area (Rect) in full window screen
 *  @param Rect = selected area to show after zooming
 */
void WinEDA_DrawFrame::Window_Zoom( EDA_Rect& Rect )
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


/** Function OnZoom
 * Called from any zoom event (toolbar , hotkey or popup )
 */
void WinEDA_DrawFrame::OnZoom( wxCommandEvent& event )
{
    if( DrawPanel == NULL )
    {
        wxLogDebug( wxT( "%s, %d: DrawPanel object is undefined ." ),
                    __TFILE__, __LINE__ );
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
        if( screen->SetPreviousZoom() )
            Recadre_Trace( zoom_at_cursor );
        break;

    case ID_POPUP_ZOOM_OUT:
        zoom_at_cursor = true;
        // fall thru

    case ID_ZOOM_OUT:
        if( id == ID_ZOOM_OUT )
            screen->m_Curseur = DrawPanel->GetScreenCenterRealPosition();
        if( screen->SetNextZoom() )
            Recadre_Trace( zoom_at_cursor );
        break;

    case ID_ZOOM_REDRAW:
        // DrawPanel->Refresh(); usually good,
        // but does not work under linux, when called from here (wxGTK bug ?)
        ReDrawPanel();
        break;

    case ID_POPUP_ZOOM_CENTER:
        Recadre_Trace( true );
        break;

    case ID_ZOOM_PAGE:
        // With Zoom_Automatique(), the "Zoom Auto" button (and hotkey)
        // does nothing if the view is already at the correct
        // zoom level, but needs to be shifted (centered).
        //Zoom_Automatique( false );
        GetBaseScreen()->SetZoom( BestZoom() );
        Recadre_Trace( false );
        break;

    case ID_POPUP_ZOOM_SELECT:
        break;

    case ID_POPUP_CANCEL:
        DrawPanel->MouseToCursorSchema();
        break;

    default:
        i = id - ID_POPUP_ZOOM_LEVEL_START;

        if( ( i < 0 ) || ( (size_t) i >= screen->m_ZoomList.GetCount() ) )
        {
            wxLogDebug( wxT( "%s %d: index %d is outside the bounds of the zoom list." ),
                        __TFILE__, __LINE__, i );
            return;
        }
        if( screen->SetZoom( screen->m_ZoomList[i] ) )
            Recadre_Trace( true );
    }

    UpdateStatusBar();
}


/* add the zoom list menu the the MasterMenu.
 *  used in OnRightClick(wxMouseEvent& event)
 */
void WinEDA_DrawPanel::AddMenuZoom( wxMenu* MasterMenu )
{
    size_t      i;
    int         maxZoomIds;
    int         zoom;
    wxRealPoint grid;
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

    /* Populate zoom submenu. */
    for( i = 0; i < (size_t) maxZoomIds; i++ )
    {
        if ( ( GetScreen()->m_ZoomList[i] % GetScreen()->m_ZoomScalar ) == 0 )
            msg.Printf( wxT( "%u" ),
                        GetScreen()->m_ZoomList[i] / GetScreen()->m_ZoomScalar );
        else
            msg.Printf( wxT( "%.1f" ),
                        (float) GetScreen()->m_ZoomList[i] /
                        GetScreen()->m_ZoomScalar );

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

        grid = GetScreen()->GetGridSize();

        for( i = 0; i < GetScreen()->m_GridList.GetCount(); i++ )
        {
            tmp = GetScreen()->m_GridList[i];
            gridValue = To_User_Unit( g_UnitMetric, tmp.m_Size.x,
                                      m_Parent->m_InternalUnits );

            if( tmp.m_Id == ID_POPUP_GRID_USER )
            {
                msg = _( "User Grid" );
            }
            else
            {
                if ( g_UnitMetric == 0 )    // inches
                    msg.Printf( wxT( "%.1f mils" ), gridValue * 1000 );
                else
                    msg.Printf( wxT( "%.3f mm" ), gridValue );
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
