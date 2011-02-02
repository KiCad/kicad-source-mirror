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
#include "kicad_device_context.h"
#include "hotkeys_basic.h"


void EDA_DRAW_FRAME::RedrawScreen( bool aWarpPointer )
{

    PutOnGrid( &(GetBaseScreen()->m_Curseur) );
    AdjustScrollBars();

#if !defined(__WXMAC__)
    /* DrawPanel->Refresh() is not used here because the redraw is delayed and the mouse
     * events (from MouseToCursorSchema ot others) during this delay create problems: the
     * mouse cursor position is false in calculations.  TODO: see exactly how the mouse
     * creates problems when moving during refresh use Refresh() and update() do not change
     * problems
     */
    INSTALL_DC( dc, DrawPanel );
    DrawPanel->SetClipBox( dc );
    DrawPanel->ReDraw( &dc, DrawPanel->m_DisableEraseBG ? false : true );
#else
    DrawPanel->Refresh();
    DrawPanel->Update();
#endif

    /* Move the mouse cursor to the on grid graphic cursor position */
    if( aWarpPointer )
        DrawPanel->MouseToCursorSchema();
 }


/** Adjust the coordinate to the nearest grid value
 * @param aCoord = coordinate to adjust
 * @param aGridSize = pointer to a grid value. if NULL uses the current grid size
 */
void EDA_DRAW_FRAME::PutOnGrid( wxPoint* aCoord , wxRealPoint* aGridSize )
{
    wxCHECK_RET( aCoord != NULL, wxT( "Cannot pull NULL coordinate pointer on grid." ) );

    *aCoord = GetBaseScreen()->GetNearestGridPosition( *aCoord, aGridSize );
}


/** Redraw the screen with best zoom level and the best centering
 * that shows all the page or the board
 */
void EDA_DRAW_FRAME::Zoom_Automatique( bool move_mouse_cursor )
{
    GetBaseScreen()->SetZoom( BestZoom() ); // Set the best zoom
    RedrawScreen( move_mouse_cursor );     // Set the best centering and refresh the screen
}


/** Compute the zoom factor and the new draw offset to draw the
 *  selected area (Rect) in full window screen
 *  @param Rect = selected area to show after zooming
 */
void EDA_DRAW_FRAME::Window_Zoom( EDA_Rect& Rect )
{
    double scalex, bestscale;
    wxSize size;

    /* Compute the best zoom */
    Rect.Normalize();
    size = DrawPanel->GetClientSize();

    // Use ceil to at least show the full rect
    scalex    = (double) Rect.GetSize().x / size.x;
    bestscale = (double) Rect.GetSize().y / size.y;
    bestscale = MAX( bestscale, scalex );

    GetBaseScreen()->SetScalingFactor( bestscale );
    GetBaseScreen()->m_Curseur = Rect.Centre();
    RedrawScreen( TRUE );
}


/**
 * Function OnZoom
 * Called from any zoom event (toolbar , hotkey or popup )
 */
void EDA_DRAW_FRAME::OnZoom( wxCommandEvent& event )
{
    if( DrawPanel == NULL )
        return;

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
            screen->m_Curseur = DrawPanel->GetScreenCenterLogicalPosition();
        if( screen->SetPreviousZoom() )
            RedrawScreen( zoom_at_cursor );
        break;

    case ID_POPUP_ZOOM_OUT:
        zoom_at_cursor = true;

    // fall thru

    case ID_ZOOM_OUT:
        if( id == ID_ZOOM_OUT )
            screen->m_Curseur = DrawPanel->GetScreenCenterLogicalPosition();
        if( screen->SetNextZoom() )
            RedrawScreen( zoom_at_cursor );
        break;

    case ID_ZOOM_REDRAW:
        DrawPanel->Refresh();
        break;

    case ID_POPUP_ZOOM_CENTER:
        RedrawScreen( true );
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

        if( ( i < 0 ) || ( (size_t) i >= screen->m_ZoomList.GetCount() ) )
        {
            wxLogDebug( wxT( "%s %d: index %d is outside the bounds of the zoom list." ),
                        __TFILE__, __LINE__, i );
            return;
        }
        if( screen->SetZoom( screen->m_ZoomList[i] ) )
            RedrawScreen( true );
    }

    UpdateStatusBar();
}


/* add the zoom list menu the the MasterMenu.
 *  used in OnRightClick(wxMouseEvent& event)
 */
void EDA_DRAW_FRAME::AddMenuZoomAndGrid( wxMenu* MasterMenu )
{
    int         maxZoomIds;
    int         zoom;
    wxString    msg;
    BASE_SCREEN * screen = DrawPanel->GetScreen();

    msg = AddHotkeyName( _( "Center" ), m_HotkeysZoomAndGridList, HK_ZOOM_CENTER );
    ADD_MENUITEM( MasterMenu, ID_POPUP_ZOOM_CENTER, msg, zoom_center_xpm );
    msg = AddHotkeyName( _( "Zoom in" ), m_HotkeysZoomAndGridList, HK_ZOOM_IN );
    ADD_MENUITEM( MasterMenu, ID_POPUP_ZOOM_IN, msg, zoom_in_xpm );
    msg = AddHotkeyName( _( "Zoom out" ), m_HotkeysZoomAndGridList, HK_ZOOM_OUT );
    ADD_MENUITEM( MasterMenu, ID_POPUP_ZOOM_OUT, msg, zoom_out_xpm );
    msg = AddHotkeyName( _( "Redraw view" ), m_HotkeysZoomAndGridList, HK_ZOOM_REDRAW );
    ADD_MENUITEM( MasterMenu, ID_ZOOM_REDRAW, msg, zoom_redraw_xpm );
    msg = AddHotkeyName( _( "Zoom auto" ), m_HotkeysZoomAndGridList, HK_ZOOM_AUTO );
    ADD_MENUITEM( MasterMenu, ID_ZOOM_PAGE, msg, zoom_auto_xpm );


    wxMenu* zoom_choice = new wxMenu;
    ADD_MENUITEM_WITH_SUBMENU( MasterMenu, zoom_choice,
                               ID_POPUP_ZOOM_SELECT, _( "Zoom select" ),
                               zoom_select_xpm );

    zoom = screen->GetZoom();
    maxZoomIds = ID_POPUP_ZOOM_LEVEL_END - ID_POPUP_ZOOM_LEVEL_START;
    maxZoomIds = ( (size_t) maxZoomIds < screen->m_ZoomList.GetCount() ) ?
                 maxZoomIds : screen->m_ZoomList.GetCount();

    /* Populate zoom submenu. */
    for( int i = 0; i < maxZoomIds; i++ )
    {
        if( ( screen->m_ZoomList[i] % screen->m_ZoomScalar ) == 0 )
            msg.Printf( wxT( "%u" ),
                        screen->m_ZoomList[i] / screen->m_ZoomScalar );
        else
            msg.Printf( wxT( "%.1f" ),
                        (float) screen->m_ZoomList[i] /
                        screen->m_ZoomScalar );

        zoom_choice->Append( ID_POPUP_ZOOM_LEVEL_START + i, _( "Zoom: " ) + msg,
                             wxEmptyString, wxITEM_CHECK );
        if( zoom == screen->m_ZoomList[i] )
            zoom_choice->Check( ID_POPUP_ZOOM_LEVEL_START + i, true );
    }

    /* Create grid submenu as required. */
    if( screen->GetGridCount() )
    {
        wxMenu* gridMenu = new wxMenu;
        ADD_MENUITEM_WITH_SUBMENU( MasterMenu, gridMenu, ID_POPUP_GRID_SELECT,
                                   _( "Grid Select" ), grid_select_xpm );

        GRID_TYPE   tmp;
        wxRealPoint grid = screen->GetGridSize();

        for( size_t i = 0; i < screen->GetGridCount(); i++ )
        {
            tmp = screen->GetGrid( i );
            double gridValueInch = To_User_Unit( INCHES, tmp.m_Size.x, m_InternalUnits );
            double gridValue_mm = To_User_Unit( MILLIMETRES, tmp.m_Size.x, m_InternalUnits );

            if( tmp.m_Id == ID_POPUP_GRID_USER )
            {
                msg = _( "User Grid" );
            }
            else
            {
                switch( g_UserUnit )
                {
                case INCHES:
                    msg.Printf( wxT( "%.1f mils, (%.3f mm)" ),
                                gridValueInch * 1000, gridValue_mm );
                    break;

                case MILLIMETRES:
                    msg.Printf( wxT( "%.3f mm, (%.1f mils)" ),
                                gridValue_mm, gridValueInch * 1000 );
                    break;

                case UNSCALED_UNITS:
                    msg = wxT( "???" );
                    break;
                }
            }

            gridMenu->Append( tmp.m_Id, msg, wxEmptyString, true );

            if( grid == tmp.m_Size )
                gridMenu->Check( tmp.m_Id, true );
        }
    }

    MasterMenu->AppendSeparator();
    ADD_MENUITEM( MasterMenu, ID_POPUP_CANCEL, _( "Close" ), cancel_xpm );
}
