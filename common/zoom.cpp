/************/
/* zoom.cpp */
/************/

/*
 * Manage zoom, grid step, and auto crop.
 */

#include "fctsys.h"
#include "macros.h"
#include "id.h"
#include "class_drawpanel.h"
#include "class_base_screen.h"
#include "wxstruct.h"
#include "kicad_device_context.h"
#include "hotkeys_basic.h"


void EDA_DRAW_FRAME::RedrawScreen( const wxPoint& aCenterPoint, bool aWarpPointer )
{
    AdjustScrollBars( aCenterPoint );

    // Move the mouse cursor to the on grid graphic cursor position
    if( aWarpPointer )
        DrawPanel->MoveCursorToCrossHair();

    DrawPanel->Refresh();
    DrawPanel->Update();
 }


/** Redraw the screen with best zoom level and the best centering
 * that shows all the page or the board
 */
void EDA_DRAW_FRAME::Zoom_Automatique( bool aWarpPointer )
{
    BASE_SCREEN * screen = GetScreen();
    screen->SetZoom( BestZoom() ); // Set the best zoom and get center point.

    if( screen->m_FirstRedraw )
        screen->SetCrossHairPosition( screen->GetScrollCenterPosition() );

    RedrawScreen( screen->GetScrollCenterPosition(), aWarpPointer );
}


/** Compute the zoom factor and the new draw offset to draw the
 *  selected area (Rect) in full window screen
 *  @param Rect = selected area to show after zooming
 */
void EDA_DRAW_FRAME::Window_Zoom( EDA_RECT& Rect )
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

    GetScreen()->SetScalingFactor( bestscale );
    RedrawScreen( Rect.Centre(), true );
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
    BASE_SCREEN* screen = GetScreen();
    wxPoint      center = screen->GetScrollCenterPosition();

    switch( id )
    {
    case ID_POPUP_ZOOM_IN:
        zoom_at_cursor = true;
        center = screen->GetCrossHairPosition();

    // fall thru
    case ID_ZOOM_IN:
        if( screen->SetPreviousZoom() )
            RedrawScreen( center, zoom_at_cursor );
        break;

    case ID_POPUP_ZOOM_OUT:
        zoom_at_cursor = true;
        center = screen->GetCrossHairPosition();

    // fall thru
    case ID_ZOOM_OUT:
        if( screen->SetNextZoom() )
            RedrawScreen( center, zoom_at_cursor );
        break;

    case ID_ZOOM_REDRAW:
        DrawPanel->Refresh();
        break;

    case ID_POPUP_ZOOM_CENTER:
        center = screen->GetCrossHairPosition();
        RedrawScreen( center, true );
        break;

    case ID_ZOOM_PAGE:
        Zoom_Automatique( false );
        break;

    case ID_POPUP_ZOOM_SELECT:
        break;

    case ID_POPUP_CANCEL:
        DrawPanel->MoveCursorToCrossHair();
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
            RedrawScreen( center, true );
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
    AddMenuItem( MasterMenu, ID_POPUP_ZOOM_CENTER, msg, KiBitmap( zoom_center_on_screen_xpm ) );
    msg = AddHotkeyName( _( "Zoom in" ), m_HotkeysZoomAndGridList, HK_ZOOM_IN );
    AddMenuItem( MasterMenu, ID_POPUP_ZOOM_IN, msg, KiBitmap( zoom_in_xpm ) );
    msg = AddHotkeyName( _( "Zoom out" ), m_HotkeysZoomAndGridList, HK_ZOOM_OUT );
    AddMenuItem( MasterMenu, ID_POPUP_ZOOM_OUT, msg, KiBitmap( zoom_out_xpm ) );
    msg = AddHotkeyName( _( "Redraw view" ), m_HotkeysZoomAndGridList, HK_ZOOM_REDRAW );
    AddMenuItem( MasterMenu, ID_ZOOM_REDRAW, msg, KiBitmap( zoom_redraw_xpm ) );
    msg = AddHotkeyName( _( "Zoom auto" ), m_HotkeysZoomAndGridList, HK_ZOOM_AUTO );
    AddMenuItem( MasterMenu, ID_ZOOM_PAGE, msg, KiBitmap( zoom_fit_in_page_xpm ) );


    wxMenu* zoom_choice = new wxMenu;
    AddMenuItem( MasterMenu, zoom_choice,
                 ID_POPUP_ZOOM_SELECT, _( "Zoom select" ),
                 KiBitmap( zoom_selection_xpm ) );

    zoom = screen->GetZoom();
    maxZoomIds = ID_POPUP_ZOOM_LEVEL_END - ID_POPUP_ZOOM_LEVEL_START;
    maxZoomIds = ( (size_t) maxZoomIds < screen->m_ZoomList.GetCount() ) ?
                 maxZoomIds : screen->m_ZoomList.GetCount();

    /* Populate zoom submenu. */
    for( int i = 0; i < maxZoomIds; i++ )
    {
        msg.Printf( wxT( "%g" ), screen->m_ZoomList[i] );

        zoom_choice->Append( ID_POPUP_ZOOM_LEVEL_START + i, _( "Zoom: " ) + msg,
                             wxEmptyString, wxITEM_CHECK );
        if( zoom == screen->m_ZoomList[i] )
            zoom_choice->Check( ID_POPUP_ZOOM_LEVEL_START + i, true );
    }

    /* Create grid submenu as required. */
    if( screen->GetGridCount() )
    {
        wxMenu* gridMenu = new wxMenu;
        AddMenuItem( MasterMenu, gridMenu, ID_POPUP_GRID_SELECT,
                     _( "Grid Select" ), KiBitmap( grid_select_xpm ) );

        GRID_TYPE   tmp;
        wxRealPoint grid = screen->GetGridSize();

        for( size_t i = 0; i < screen->GetGridCount(); i++ )
        {
            tmp = screen->GetGrid( i );
            double gridValueInch = To_User_Unit( INCHES, tmp.m_Size.x, m_internalUnits );
            double gridValue_mm = To_User_Unit( MILLIMETRES, tmp.m_Size.x, m_internalUnits );

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
    AddMenuItem( MasterMenu, ID_POPUP_CANCEL, _( "Close" ), KiBitmap( cancel_xpm ) );
}
