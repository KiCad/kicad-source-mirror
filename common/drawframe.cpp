/*****************/
/* drawframe.cpp */
/*****************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "bitmaps.h"
#include "macros.h"
#include "id.h"
#include "class_drawpanel.h"
#include "class_base_screen.h"
#include "wxstruct.h"
#include "confirm.h"
#include "kicad_device_context.h"
#include "dialog_helpers.h"

#include <wx/fontdlg.h>


/* Definitions for enabling and disabling extra debugging output.  Please
 * remember to set these to 0 before committing changes to SVN.
 */
#define DEBUG_DUMP_SCROLLBAR_SETTINGS 0   // Set to 1 to print scroll bar settings.


/* Configuration entry names. */
static const wxString CursorShapeEntryKeyword( wxT( "CursorShape" ) );
static const wxString ShowGridEntryKeyword( wxT( "ShowGrid" ) );
static const wxString GridColorEntryKeyword( wxT( "GridColor" ) );
static const wxString LastGridSizeId( wxT( "_LastGridSize" ) );


BEGIN_EVENT_TABLE( EDA_DRAW_FRAME, EDA_BASE_FRAME )
    EVT_MOUSEWHEEL( EDA_DRAW_FRAME::OnMouseEvent )
    EVT_MENU_OPEN( EDA_DRAW_FRAME::OnMenuOpen )
    EVT_ACTIVATE( EDA_DRAW_FRAME::OnActivate )
    EVT_MENU_RANGE( ID_POPUP_ZOOM_START_RANGE, ID_POPUP_ZOOM_END_RANGE,
                    EDA_DRAW_FRAME::OnZoom )
    EVT_MENU_RANGE( ID_POPUP_GRID_LEVEL_1000, ID_POPUP_GRID_USER,
                    EDA_DRAW_FRAME::OnSelectGrid )
END_EVENT_TABLE()


EDA_DRAW_FRAME::EDA_DRAW_FRAME( wxWindow* father, int idtype,
                                    const wxString& title,
                                    const wxPoint& pos, const wxSize& size,
                                    long style ) :
    EDA_BASE_FRAME( father, idtype, title, pos, size, style )
{
    wxSize minsize;

    m_VToolBar            = NULL;
    m_AuxVToolBar         = NULL;
    m_OptionsToolBar      = NULL;
    m_AuxiliaryToolBar    = NULL;
    m_SelGridBox          = NULL;
    m_SelZoomBox          = NULL;
    m_HotkeysZoomAndGridList = NULL;

    DrawPanel             = NULL;
    MsgPanel              = NULL;
    m_currentScreen       = NULL;
    m_ID_current_state    = 0;
    m_ID_last_state       = 0;
    m_HTOOL_current_state = 0;
    m_Draw_Axis           = FALSE;  // TRUE to draw axis.
    m_Draw_Sheet_Ref      = FALSE;  // TRUE to display reference sheet.
    m_Print_Sheet_Ref     = TRUE;   // TRUE to print reference sheet.
    m_Draw_Auxiliary_Axis = FALSE;  // TRUE draw auxilary axis.
    m_Draw_Grid_Axis      = FALSE;  // TRUE to draw the grid axis
    m_CursorShape         = 0;
    m_LastGridSizeId      = 0;
    m_DrawGrid            = true;   // hide/Show grid. default = show
    m_GridColor           = DARKGRAY;   // Grid color
    m_snapToGrid          = true;

    // Internal units per inch: = 1000 for schema, = 10000 for PCB
    m_InternalUnits       = EESCHEMA_INTERNAL_UNIT;
    minsize.x             = 470;
    minsize.y             = 350 + m_MsgFrameHeight;

    SetSizeHints( minsize.x, minsize.y, -1, -1, -1, -1 );

    /* Make sure window has a sane minimum size. */
    if( ( size.x < minsize.x ) || ( size.y < minsize.y ) )
        SetSize( 0, 0, minsize.x, minsize.y );

    // Pane sizes for status bar.
    #define ZOOM_DISPLAY_SIZE       60
    #define COORD_DISPLAY_SIZE      156
    #define UNITS_DISPLAY_SIZE      50
    #define FUNCTION_DISPLAY_SIZE   100

    static const int dims[6] = { -1, ZOOM_DISPLAY_SIZE,
        COORD_DISPLAY_SIZE, COORD_DISPLAY_SIZE,
        UNITS_DISPLAY_SIZE, FUNCTION_DISPLAY_SIZE };

    CreateStatusBar( 6 );
    SetStatusWidths( 6, dims );

    // Create child subwindows.
    GetClientSize( &m_FrameSize.x, &m_FrameSize.y );
    m_FramePos.x   = m_FramePos.y = 0;
    m_FrameSize.y -= m_MsgFrameHeight;

    DrawPanel = new EDA_DRAW_PANEL( this, -1, wxPoint( 0, 0 ), m_FrameSize );
    MsgPanel  = new WinEDA_MsgPanel( this, -1, wxPoint( 0, m_FrameSize.y ),
                                     wxSize( m_FrameSize.x, m_MsgFrameHeight ) );

    MsgPanel->SetBackgroundColour( wxColour( ColorRefs[LIGHTGRAY].m_Red,
                                             ColorRefs[LIGHTGRAY].m_Green,
                                             ColorRefs[LIGHTGRAY].m_Blue ) );
}


EDA_DRAW_FRAME::~EDA_DRAW_FRAME()
{
    SAFE_DELETE( m_currentScreen );

    m_auimgr.UnInit();
}


/*
 *  Display the message in the first pane of the status bar.
 */
void EDA_DRAW_FRAME::Affiche_Message( const wxString& message )
{
    SetStatusText( message, 0 );
}


void EDA_DRAW_FRAME::EraseMsgBox()
{
    if( MsgPanel )
        MsgPanel->EraseMsgBox();
}


void EDA_DRAW_FRAME::OnActivate( wxActivateEvent& event )
{
    m_FrameIsActive = event.GetActive();

    if( DrawPanel )
        DrawPanel->m_CanStartBlock = -1;

    event.Skip();   // required under wxMAC
}


void EDA_DRAW_FRAME::OnMenuOpen( wxMenuEvent& event )
{
    if( DrawPanel )
        DrawPanel->m_CanStartBlock = -1;

    event.Skip();
}


// Virtual function
void EDA_DRAW_FRAME::ReCreateAuxiliaryToolbar()
{
}


// Virtual function
void EDA_DRAW_FRAME::ReCreateMenuBar()
{
}


// Virtual function
void EDA_DRAW_FRAME::OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition, EDA_ITEM* aItem )
{
}


// Virtual function
void EDA_DRAW_FRAME::ToolOnRightClick( wxCommandEvent& event )
{
}

/**
 * Function PrintPage (virtual)
 * used to print a page
 * this basic function must be derived to be used for printing
 * because EDA_DRAW_FRAME does not know how to print a page
 * This is the reason it is a virtual function
 */
void EDA_DRAW_FRAME::PrintPage( wxDC* aDC,int aPrintMask,
                                  bool aPrintMirrorMode, void* aData )
{
    wxMessageBox( wxT("EDA_DRAW_FRAME::PrintPage() error"));
}


// Virtual function
void EDA_DRAW_FRAME::OnSelectGrid( wxCommandEvent& event )
{
    int* clientData;
    int  id = ID_POPUP_GRID_LEVEL_100;

    if( event.GetEventType() == wxEVT_COMMAND_COMBOBOX_SELECTED )
    {
        if( m_SelGridBox == NULL )
            return;

        /*
         * Don't use wxCommandEvent::GetClientData() here.  It always
         * returns NULL in GTK.  This solution is not as elegant but
         * it works.
         */
        int index = m_SelGridBox->GetSelection();
        wxASSERT( index != wxNOT_FOUND );
        clientData = (int*) m_SelGridBox->wxItemContainer::GetClientData( index );

        if( clientData != NULL )
            id = *clientData;
    }
    else
    {
        id = event.GetId();

        /* Update the grid select combobox if the grid size was changed
         * by menu event.
         */
        if( m_SelGridBox != NULL )
        {
            for( size_t i = 0; i < m_SelGridBox->GetCount(); i++ )
            {
                clientData = (int*) m_SelGridBox->wxItemContainer::GetClientData( i );

                if( clientData && id == *clientData )
                {
                    m_SelGridBox->SetSelection( i );
                    break;
                }
            }
        }
    }

    BASE_SCREEN* screen = GetScreen();

    if( screen->GetGridId() == id )
        return;

    /*
     * This allows for saving non-sequential command ID offsets used that
     * may be used in the grid size combobox.  Do not use the selection
     * index returned by GetSelection().
     */
    m_LastGridSizeId = id - ID_POPUP_GRID_LEVEL_1000;
    screen->m_Curseur = DrawPanel->GetScreenCenterLogicalPosition();
    screen->SetGrid( id );
    Refresh();
}


/**
 * Set the zoom when selected by the Zoom List Box
 *  Note:
 *      position 0 = Fit in Page
 *      position >= 1 = zoom (1 to zoom max)
 *      last position : special zoom
 *      virtual function
 */
void EDA_DRAW_FRAME::OnSelectZoom( wxCommandEvent& event )
{
    if( m_SelZoomBox == NULL )
        return;                        // Should not happen!

    int id = m_SelZoomBox->GetChoice();

    if( id < 0 || !( id < (int)m_SelZoomBox->GetCount() ) )
        return;

    if( id == 0 )                      // Auto zoom (Fit in Page)
    {
        Zoom_Automatique( true );
    }
    else
    {
        id--;
        int selectedZoom = GetScreen()->m_ZoomList[id];

        if( GetScreen()->GetZoom() == selectedZoom )
            return;

        GetScreen()->m_Curseur = DrawPanel->GetScreenCenterLogicalPosition();
        GetScreen()->SetZoom( selectedZoom );
        RedrawScreen( false );
    }
}


/* Return the current zoom level */
int EDA_DRAW_FRAME::GetZoom(void)
{
    return GetScreen()->GetZoom();
}


void EDA_DRAW_FRAME::OnMouseEvent( wxMouseEvent& event )
{
    event.Skip();
}


// Virtual
void EDA_DRAW_FRAME::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
{
}


void EDA_DRAW_FRAME::SetToolbars()
{
    DisplayUnitsMsg();

    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();
}


void EDA_DRAW_FRAME::DisplayToolMsg( const wxString& msg )
{
    SetStatusText( msg, 5 );
}


/* Display current unit Selection on Statusbar
 */
void EDA_DRAW_FRAME::DisplayUnitsMsg()
{
    wxString msg;

    switch( g_UserUnit )
    {
    case INCHES:
        msg = _( "Inches" );
        break;

    case MILLIMETRES:
        msg += _( "mm" );
        break;

    default:
        msg += _( "Units" );
        break;
    }

    SetStatusText( msg, 4 );
}



/* Recalculate the size of toolbars and display panel.
 */
void EDA_DRAW_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    m_FrameSize = GetClientSize( );

    SizeEv.Skip();
}


/**
 * Function SetToolID
 * Enables the icon of the selected tool in the vertical toolbar.
 * (Or tool ID_NO_SELECT_BUTT default if no new selection)
 * @param aId = new m_ID_current_state value (if aId >= 0)
 * @param aCursor = the new cursor shape (0 = default cursor)
 * @param aToolMsg = tool message in status bar
 * if (aId >= 0)
 * Updates all variables related:
 *      m_ID_current_state, cursor shape and message in status bar
 * If (aId < 0)
 *      Only updates the cursor shape and message in status bar
 *      (does not the current m_ID_current_state value
 */
void EDA_DRAW_FRAME::SetToolID( int aId, int aCursor, const wxString& aToolMsg )
{
    // Keep default cursor in toolbars
    SetCursor( wxNullCursor );

    // Change Cursor in DrawPanel only
    if( DrawPanel )
    {
        DrawPanel->m_PanelDefaultCursor = aCursor;
        DrawPanel->SetCursor( aCursor );
    }

    DisplayToolMsg( aToolMsg );

    if( aId < 0 )
        return;

    // Old Tool ID_NO_SELECT_BUTT active or inactive if no new tool.
    if( m_ID_current_state )
    {
        if( m_VToolBar )
            m_VToolBar->ToggleTool( m_ID_current_state, FALSE );

        if( m_AuxVToolBar )
            m_AuxVToolBar->ToggleTool( m_ID_current_state, FALSE );
    }
    else
    {
        if( aId )
        {
            if( m_VToolBar )
                m_VToolBar->ToggleTool( ID_NO_SELECT_BUTT, FALSE );

            if( m_AuxVToolBar )
                m_AuxVToolBar->ToggleTool( m_ID_current_state, FALSE );
        }
        else if( m_VToolBar )
            m_VToolBar->ToggleTool( ID_NO_SELECT_BUTT, TRUE );
    }

    if( aId )
    {
        if( m_VToolBar )
            m_VToolBar->ToggleTool( aId, TRUE );

        if( m_AuxVToolBar )
            m_AuxVToolBar->ToggleTool( aId, TRUE );
    }
    else if( m_VToolBar )
        m_VToolBar->ToggleTool( ID_NO_SELECT_BUTT, TRUE );

    m_ID_current_state = aId;

    if( m_VToolBar )
        m_VToolBar->Refresh( );
}


/*****************************/
/* default virtual functions */
/*****************************/

void EDA_DRAW_FRAME::OnGrid( int grid_type )
{
}


wxPoint EDA_DRAW_FRAME::GetGridPosition( const wxPoint& aPosition )
{
    wxPoint pos = aPosition;

    if( m_currentScreen != NULL && m_snapToGrid )
        pos = m_currentScreen->GetNearestGridPosition( aPosition );

    return pos;
}


int EDA_DRAW_FRAME::ReturnBlockCommand( int key )
{
    return 0;
}


void EDA_DRAW_FRAME::InitBlockPasteInfos()
{
    GetScreen()->m_BlockLocate.ClearItemsList();
    DrawPanel->ManageCurseur = NULL;
}


void EDA_DRAW_FRAME::HandleBlockPlace( wxDC* DC )
{
}


bool EDA_DRAW_FRAME::HandleBlockEnd( wxDC* DC )
{
    return false;
}


void EDA_DRAW_FRAME::AdjustScrollBars()
{
    int     unitsX, unitsY, posX, posY;
    wxSize  drawingSize, clientSize;
    BASE_SCREEN* screen = GetScreen();
    bool noRefresh = true;

    if( screen == NULL || DrawPanel == NULL )
        return;

    // The drawing size is twice the current page size.
    drawingSize = screen->ReturnPageSize() * 2;

    // Calculate the portion of the drawing that can be displayed in the
    // client area at the current zoom level.
    clientSize = DrawPanel->GetClientSize();

    double scalar = screen->GetScalingFactor();
    clientSize.x = wxRound( (double) clientSize.x / scalar );
    clientSize.y = wxRound( (double) clientSize.y / scalar );

    /* Adjust drawing size when zooming way out to prevent centering around
     * cursor problems. */
    if( clientSize.x > drawingSize.x || clientSize.y > drawingSize.y )
        drawingSize = clientSize;

    drawingSize.x += wxRound( (double) clientSize.x / 2.0 );
    drawingSize.y += wxRound( (double) clientSize.y / 2.0 );

    if( screen->m_Center )
    {
        screen->m_DrawOrg.x = -wxRound( (double) drawingSize.x / 2.0 );
        screen->m_DrawOrg.y = -wxRound( (double) drawingSize.y / 2.0 );
    }
    else
    {
        screen->m_DrawOrg.x = -wxRound( (double) clientSize.x / 2.0 );
        screen->m_DrawOrg.y = -wxRound( (double) clientSize.y / 2.0 );
    }

    /* Always set scrollbar pixels per unit to 1 unless you want the zoom
     * around cursor to jump around.  This reported problem occurs when the
     * zoom point is not on a pixel per unit increment.  If you set the
     * pixels per unit to 10, you have potential for the zoom point to
     * jump around +/-5 pixels from the nearest grid point.
     */
    screen->m_ScrollPixelsPerUnitX = screen->m_ScrollPixelsPerUnitY = 1;

    // Calculate the number of scroll bar units for the given zoom level. */
    unitsX = wxRound( (double) drawingSize.x * scalar );
    unitsY = wxRound( (double) drawingSize.y * scalar );

    // Calculate the position, place the cursor at the center of screen.
    posX = screen->m_Curseur.x - screen->m_DrawOrg.x;
    posY = screen->m_Curseur.y - screen->m_DrawOrg.y;

    posX -= wxRound( (double) clientSize.x / 2.0 );
    posY -= wxRound( (double) clientSize.y / 2.0 );

    if( posX < 0 )
        posX = 0;

    if( posY < 0 )
        posY = 0;

    posX = wxRound( (double) posX * scalar );
    posY = wxRound( (double) posY * scalar );

    screen->m_ScrollbarPos = wxPoint( posX, posY );
    screen->m_ScrollbarNumber = wxSize( unitsX, unitsY );

#if DEBUG_DUMP_SCROLLBAR_SETTINGS
    wxLogDebug( wxT( "SetScrollbars(%d, %d, %d, %d, %d, %d)" ),
                screen->m_ScrollPixelsPerUnitX, screen->m_ScrollPixelsPerUnitY,
                screen->m_ScrollbarNumber.x, screen->m_ScrollbarNumber.y,
                screen->m_ScrollbarPos.x, screen->m_ScrollbarPos.y );
#endif

    DrawPanel->SetScrollbars( screen->m_ScrollPixelsPerUnitX,
                              screen->m_ScrollPixelsPerUnitY,
                              screen->m_ScrollbarNumber.x,
                              screen->m_ScrollbarNumber.y,
                              screen->m_ScrollbarPos.x,
                              screen->m_ScrollbarPos.y, noRefresh );
}


/**
 * Function SetLanguage
 * called on a language menu selection
 * when using a derived function, do not forget to call this one
 */
void EDA_DRAW_FRAME::SetLanguage( wxCommandEvent& event )
{
    EDA_BASE_FRAME::SetLanguage( event );
}


/**
 * Round to the nearest precision.
 *
 * Try to approximate a coordinate using a given precision to prevent
 * rounding errors when converting from inches to mm.
 *
 * ie round the unit value to 0 if unit is 1 or 2, or 8 or 9
 */
double RoundTo0( double x, double precision )
{
    assert( precision != 0 );

    long long ix = wxRound( x * precision );

    if ( x < 0.0 )
        NEGATE( ix );

    int remainder = ix % 10;   // remainder is in precision mm

    if ( remainder <= 2 )
        ix -= remainder;       // truncate to the near number
    else if (remainder >= 8 )
        ix += 10 - remainder;  // round to near number

    if ( x < 0 )
        NEGATE( ix );

    return (double) ix / precision;
}

/**
 * Function UpdateStatusBar
 * Displays in the bottom of the main window a stust:
 *  - Absolute Cursor coordinates
 *  - Relative Cursor coordinates (relative to the last coordinate stored
 *     when actiavte the space bar)
 * ( in this status is also displayed the zoom level, but this is not made
 *   by this function )
 */
void EDA_DRAW_FRAME::UpdateStatusBar()
{
    wxString        Line;
    int             dx, dy;
    BASE_SCREEN*    screen = GetScreen();

    if( !screen )
        return;

    /* Display Zoom level: zoom = zoom_coeff/ZoomScalar */
    if ( (screen->GetZoom() % screen->m_ZoomScalar) == 0 )
        Line.Printf( wxT( "Z %d" ), screen->GetZoom() / screen->m_ZoomScalar );
    else
        Line.Printf( wxT( "Z %.1f" ), (float)screen->GetZoom() / screen->m_ZoomScalar );

    SetStatusText( Line, 1 );

    /* Display absolute coordinates:  */
    double dXpos = To_User_Unit( g_UserUnit, screen->m_Curseur.x, m_InternalUnits );
    double dYpos = To_User_Unit( g_UserUnit, screen->m_Curseur.y, m_InternalUnits );

    /*
     * Converting from inches to mm can give some coordinates due to
     * float point precision rounding errors, like 1.999 or 2.001 so
     * round to the nearest drawing precision required by the application.
    */
    if ( g_UserUnit == MILLIMETRES )
    {
        dXpos = RoundTo0( dXpos, (double)( m_InternalUnits / 10 ) );
        dYpos = RoundTo0( dYpos, (double)( m_InternalUnits / 10 ) );
    }

    /* The following sadly is an if eeschema/if pcbnew */
    wxString absformatter;
    wxString locformatter;
    switch( g_UserUnit )
    {
    case INCHES:
        if( m_InternalUnits == EESCHEMA_INTERNAL_UNIT )
        {
            absformatter = wxT( "X %.3f  Y %.3f" );
            locformatter = wxT( "dx %.3f  dy %.3f" );
        }
        else
        {
            absformatter = wxT( "X %.4f  Y %.4f" );
            locformatter = wxT( "dx %.4f  dy %.4f" );
        }
        break;

    case MILLIMETRES:
        if( m_InternalUnits == EESCHEMA_INTERNAL_UNIT )
        {
            absformatter = wxT( "X %.2f  Y %.2f" );
            locformatter = wxT( "dx %.2f  dy %.2f" );
        }
        else
        {
            absformatter = wxT( "X %.3f  Y %.3f" );
            locformatter = wxT( "dx %.3f  dy %.3f" );
        }
        break;

    case UNSCALED_UNITS:
        absformatter = wxT( "X %f  Y %f" );
        locformatter = wxT( "dx %f  dy %f" );
        break;
    }

    Line.Printf( absformatter, dXpos, dYpos );
    SetStatusText( Line, 2 );

    /* Display relative coordinates:  */
    dx = screen->m_Curseur.x - screen->m_O_Curseur.x;
    dy = screen->m_Curseur.y - screen->m_O_Curseur.y;
    dXpos = To_User_Unit( g_UserUnit, dx, m_InternalUnits );
    dYpos = To_User_Unit( g_UserUnit, dy, m_InternalUnits );

    if( g_UserUnit == MILLIMETRES )
    {
        dXpos = RoundTo0( dXpos, (double) ( m_InternalUnits / 10 ) );
        dYpos = RoundTo0( dYpos, (double) ( m_InternalUnits / 10 ) );
    }

    /* We already decided the formatter above */
    Line.Printf( locformatter, dXpos, dYpos );
    SetStatusText( Line, 3 );
}

/**
 * Load draw frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get loaded.
 */
void EDA_DRAW_FRAME::LoadSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    EDA_BASE_FRAME::LoadSettings();
    cfg->Read( m_FrameName + CursorShapeEntryKeyword, &m_CursorShape, ( long )0 );
    bool btmp;

    if ( cfg->Read( m_FrameName + ShowGridEntryKeyword, &btmp ) )
        SetGridVisibility( btmp);

    int itmp;

    if( cfg->Read( m_FrameName + GridColorEntryKeyword, &itmp ) )
        SetGridColor(itmp);

    cfg->Read( m_FrameName + LastGridSizeId, &m_LastGridSizeId, 0L );
}


/**
 * Save draw frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get saved.
 */
void EDA_DRAW_FRAME::SaveSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    EDA_BASE_FRAME::SaveSettings();
    cfg->Write( m_FrameName + CursorShapeEntryKeyword, m_CursorShape );
    cfg->Write( m_FrameName + ShowGridEntryKeyword, IsGridVisible() );
    cfg->Write( m_FrameName + GridColorEntryKeyword, GetGridColor() );
    cfg->Write( m_FrameName + LastGridSizeId, ( long ) m_LastGridSizeId );
}


void EDA_DRAW_FRAME::AppendMsgPanel( const wxString& textUpper,
                                     const wxString& textLower,
                                     int color, int pad )
{
    if( MsgPanel == NULL )
        return;

    MsgPanel->AppendMessage( textUpper, textLower, color, pad );
}


void EDA_DRAW_FRAME::ClearMsgPanel( void )
{
    if( MsgPanel == NULL )
        return;

    MsgPanel->EraseMsgBox();
}
