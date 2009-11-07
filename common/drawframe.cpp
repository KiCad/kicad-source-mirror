/******************************************************************/
/* drawframe.cpp - fonctions des classes du type WinEDA_DrawFrame */
/******************************************************************/

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

#include <wx/fontdlg.h>


/* Configuration entry names. */
static const wxString CursorShapeEntry( wxT( "CuShape" ) );
static const wxString ShowGridEntry( wxT( "ShGrid" ) );
static const wxString LastGridSizeId( wxT( "_LastGridSize" ) );


BEGIN_EVENT_TABLE( WinEDA_DrawFrame, WinEDA_BasicFrame )
    EVT_MOUSEWHEEL( WinEDA_DrawFrame::OnMouseEvent )
    EVT_MENU_OPEN( WinEDA_DrawFrame::OnMenuOpen )
    EVT_ACTIVATE( WinEDA_DrawFrame::OnActivate )
    EVT_MENU_RANGE( ID_POPUP_ZOOM_START_RANGE, ID_POPUP_ZOOM_END_RANGE,
                    WinEDA_DrawFrame::OnZoom )
    EVT_MENU_RANGE( ID_POPUP_GRID_LEVEL_1000, ID_POPUP_GRID_USER,
                    WinEDA_DrawFrame::OnSelectGrid )
END_EVENT_TABLE()


/*******************************************************/
/* Constructeur de WinEDA_DrawFrame: la fenetre generale */
/*******************************************************/

WinEDA_DrawFrame::WinEDA_DrawFrame( wxWindow* father, int idtype,
                                    const wxString& title,
                                    const wxPoint& pos, const wxSize& size,
                                    long style ) :
    WinEDA_BasicFrame( father, idtype, title, pos, size, style )
{
    wxSize minsize;

    m_VToolBar            = NULL;
    m_AuxVToolBar         = NULL;
    m_OptionsToolBar      = NULL;
    m_AuxiliaryToolBar    = NULL;
    m_SelGridBox          = NULL;
    m_SelZoomBox          = NULL;

    DrawPanel             = NULL;
    MsgPanel              = NULL;
    m_CurrentScreen       = NULL;
    m_ID_current_state    = 0;
    m_ID_last_state       = 0;
    m_HTOOL_current_state = 0;
    m_Draw_Axis           = FALSE;  // TRUE pour avoir les axes dessines
    m_Draw_Grid           = FALSE;  // TRUE pour avoir la axes dessinee
    m_Draw_Sheet_Ref      = FALSE;  // TRUE pour avoir le cartouche dessin�
    m_Print_Sheet_Ref     = TRUE;   // TRUE pour avoir le cartouche imprim�
    m_Draw_Auxiliary_Axis = FALSE;  // TRUE pour avoir les axes auxiliares dessines
    m_UnitType            = INTERNAL_UNIT_TYPE;    // Internal unit = inch
    m_CursorShape         = 0;
    m_LastGridSizeId      = 0;

    // Internal units per inch: = 1000 for schema, = 10000 for PCB
    m_InternalUnits       = EESCHEMA_INTERNAL_UNIT;
    minsize.x             = 470;
    minsize.y             = 350 + m_MsgFrameHeight;

    SetSizeHints( minsize.x, minsize.y, -1, -1, -1, -1 );

    /* Verification des parametres de creation */
    if( ( size.x < minsize.x ) || ( size.y < minsize.y ) )
        SetSize( 0, 0, minsize.x, minsize.y );

    // Creation de la ligne de status
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
    GetClientSize( &m_FrameSize.x, &m_FrameSize.y );/* dimx, dimy = dimensions utiles de la
                                                     *  zone utilisateur de la fenetre principale */
    m_FramePos.x   = m_FramePos.y = 0;
    m_FrameSize.y -= m_MsgFrameHeight;

    DrawPanel = new WinEDA_DrawPanel( this, -1, wxPoint( 0, 0 ), m_FrameSize );
    MsgPanel  = new WinEDA_MsgPanel( this, -1, wxPoint( 0, m_FrameSize.y ),
                                     wxSize( m_FrameSize.x, m_MsgFrameHeight ) );

    MsgPanel->SetBackgroundColour( wxColour( ColorRefs[LIGHTGRAY].m_Red,
                                             ColorRefs[LIGHTGRAY].m_Green,
                                             ColorRefs[LIGHTGRAY].m_Blue ) );
}


WinEDA_DrawFrame::~WinEDA_DrawFrame()
{
    if( m_CurrentScreen != NULL )
        delete m_CurrentScreen;

#if defined(KICAD_AUIMANAGER)
    m_auimgr.UnInit();
#endif
}


/**************************************************************/
void WinEDA_DrawFrame::Affiche_Message( const wxString& message )
/**************************************************************/

/*
 *  Display the message on the bottom the frame
 */
{
    SetStatusText( message, 0 );
}


/****************************************/
void WinEDA_DrawFrame::EraseMsgBox()
/****************************************/
{
    if( MsgPanel )
        MsgPanel->EraseMsgBox();
}


/*******************************************************/
void WinEDA_DrawFrame::OnActivate( wxActivateEvent& event )
/*******************************************************/
{
    m_FrameIsActive = event.GetActive();
    if( DrawPanel )
        DrawPanel->m_CanStartBlock = -1;

    event.Skip();   // required under wxMAC
}


/****************************************************/
void WinEDA_DrawFrame::OnMenuOpen( wxMenuEvent& event )
/****************************************************/
{
    if( DrawPanel )
        DrawPanel->m_CanStartBlock = -1;
    event.Skip();
}


/*******************************************************/
void WinEDA_DrawFrame::ReCreateAuxiliaryToolbar()
/*******************************************************/
// Virtual function
{
}


/********************************************/
void WinEDA_DrawFrame::ReCreateMenuBar()
/********************************************/
// Virtual function
{
}


/****************************************************/
void WinEDA_DrawFrame::OnHotKey( wxDC* DC, int hotkey,
                                 EDA_BaseStruct* DrawStruct )
/****************************************************/
// Virtual function
{
}


/**************************************************************/
void WinEDA_DrawFrame::ToolOnRightClick( wxCommandEvent& event )
/**************************************************************/
// Virtual function
{
}


// Virtual function
void WinEDA_DrawFrame::OnSelectGrid( wxCommandEvent& event )
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
        clientData = (int*) m_SelGridBox->GetClientData( index );

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
                clientData = (int*) m_SelGridBox->GetClientData( i );

                if( clientData && id == *clientData )
                {
                    m_SelGridBox->SetSelection( i );
                    break;
                }
            }
        }
    }

    BASE_SCREEN* screen = GetBaseScreen();

    if( screen->GetGridId() == id )
        return;

    /*
     * This allows for saving non-sequential command ID offsets used that
     * may be used in the grid size combobox.  Do not use the selection
     * index returned by GetSelection().
     */
    m_LastGridSizeId = id - ID_POPUP_GRID_LEVEL_1000;
    screen->m_Curseur = DrawPanel->GetScreenCenterRealPosition();
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
void WinEDA_DrawFrame::OnSelectZoom( wxCommandEvent& event )
{
    if( m_SelZoomBox == NULL )
        return;                        //Ne devrait pas se produire!

    int id = m_SelZoomBox->GetChoice();

    if( id < 0 || !( id < (int)m_SelZoomBox->GetCount() ) )
        return;

    if( id == 0 )           // Auto zoom (Fit in Page)
    {
        Zoom_Automatique( true );
    }
    else
    {
        id--;
        int selectedZoom = GetBaseScreen()->m_ZoomList[id];
        if( GetBaseScreen()->GetZoom() == selectedZoom )
            return;
        GetBaseScreen()->m_Curseur = DrawPanel->GetScreenCenterRealPosition();
        GetBaseScreen()->SetZoom( selectedZoom );
        Recadre_Trace( false );
    }
}

/***********************************/
int WinEDA_DrawFrame::GetZoom(void)
/***********************************/
/* Return the current zoom level */
{
    return GetBaseScreen()->GetZoom();
}


/********************************************************/
void WinEDA_DrawFrame::OnMouseEvent( wxMouseEvent& event )
/********************************************************/
{
    event.Skip();
}


/***********************************************************************/

// Virtuelle
void WinEDA_DrawFrame::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
/***********************************************************************/
{
}


/***************************************/
void WinEDA_DrawFrame::SetToolbars()
/***************************************/
{
    DisplayUnitsMsg();

#if defined(KICAD_AUIMANAGER)
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();
#endif
}


/********************************************************/
void WinEDA_DrawFrame::DisplayToolMsg( const wxString& msg )
/********************************************************/
{
    SetStatusText( msg, 5 );
}


/*******************************************/
void WinEDA_DrawFrame::DisplayUnitsMsg()
/********************************************/

/* Display current unit Selection on Statusbar
 */
{
    wxString msg;

    switch( g_UnitMetric )
    {
    case INCHES:
        msg = _( "Inch" );
        break;

    case MILLIMETRE:
        msg += _( "mm" );
        break;

    default:
        msg += _( "??" );
        break;
    }

    SetStatusText( msg, 4 );
}


/***************************************/
void WinEDA_DrawFrame::ReDrawPanel()
/***************************************/
{
    if( DrawPanel == NULL )
        return;

    wxClientDC dc( DrawPanel );

    DrawPanel->PrepareGraphicContext( &dc );
    DrawPanel->ReDraw( &dc );
}


/**************************************************/
void WinEDA_DrawFrame::OnSize( wxSizeEvent& SizeEv )
/**************************************************/

/* recalcule les dimensions des toolbars et du panel d'affichage
 */
{
    wxSize size;
    wxSize opt_size;
    wxSize Vtoolbar_size;
    wxSize Auxtoolbar_size;

    GetClientSize( &size.x, &size.y );
    m_FrameSize = size;

#if !defined(KICAD_AUIMANAGER)
    size.y -= m_MsgFrameHeight;
    if( MsgPanel ) // Positionnement en bas d'ecran
    {
        MsgPanel->SetSize( 0, size.y, size.x, m_MsgFrameHeight );
    }

    if( m_AuxiliaryToolBar )            // est sous le m_HToolBar
    {
        Auxtoolbar_size.x = size.x;     // = Largeur de la frame
        Auxtoolbar_size.y = m_AuxiliaryToolBar->GetSize().y;
        m_AuxiliaryToolBar->SetSize( Auxtoolbar_size );
        m_AuxiliaryToolBar->Move( 0, 0 );
        size.y -= Auxtoolbar_size.y;
    }

    if( m_VToolBar )    // Toolbar de droite: hauteur = hauteur utile de la frame-Auxtoolbar
    {
        Vtoolbar_size.x = m_VToolBar->GetSize().x;
        Vtoolbar_size.y = size.y;
        m_VToolBar->SetSize( Vtoolbar_size );
        m_VToolBar->Move( size.x - Vtoolbar_size.x, Auxtoolbar_size.y );
        m_VToolBar->Refresh();
    }

    if( m_AuxVToolBar ) // auxiliary vertical right toolbar, showing tools fo microwave applications
    {
        Vtoolbar_size.x += m_AuxVToolBar->GetSize().x;
        Vtoolbar_size.y  = size.y;
        m_AuxVToolBar->SetSize( m_AuxVToolBar->GetSize().x, Vtoolbar_size.y );
        m_AuxVToolBar->Move( size.x - Vtoolbar_size.x, Auxtoolbar_size.y );
        m_AuxVToolBar->Refresh();
    }
    if( m_OptionsToolBar )
    {
        if( m_OptionsToolBar->m_Horizontal )
        {
            opt_size.x = 0;
            opt_size.y = m_OptionsToolBar->GetSize().y;
            m_OptionsToolBar->SetSize( Auxtoolbar_size.x, 0,
                                       size.x, opt_size.y );
        }
        else
        {
            opt_size.x = m_OptionsToolBar->GetSize().x;
            opt_size.y = 0;
            m_OptionsToolBar->SetSize( 0, Auxtoolbar_size.y,
                                       opt_size.x, size.y );
        }
    }

    if( DrawPanel )
    {
        DrawPanel->SetSize( size.x - Vtoolbar_size.x - opt_size.x,
                            size.y - opt_size.y - 1 );
        DrawPanel->Move( opt_size.x, opt_size.y + Auxtoolbar_size.y + 1 );
    }
#endif
    SizeEv.Skip();
}


/*************************************************************************/
void WinEDA_DrawFrame::SetToolID( int id, int new_cursor_id,
                                  const wxString& title )
/*************************************************************************/

/*
 *  Active l'icone de l'outil selectionne dans le toolbar Vertical
 *  ( ou l'outil par defaut ID_NO_SELECT_BUTT si pas de nouvelle selection )
 *  if ( id >= 0 )
 *  Met a jour toutes les variables associees:
 *      message, m_ID_current_state, curseur
 *  si ( id < 0 )
 *  Met a jour seulement les variables message et  curseur
 */
{
    // Change Cursor
    if( DrawPanel )
    {
        DrawPanel->m_PanelDefaultCursor = new_cursor_id;
        DrawPanel->SetCursor( new_cursor_id );
    }
    SetCursor( wxCURSOR_ARROW );
    DisplayToolMsg( title );

    if( id < 0 )
        return;

    // Old Tool Inactif ou ID_NO_SELECT_BUTT actif si pas de nouveau Tool
    if( m_ID_current_state )
    {
        if( m_VToolBar )
            m_VToolBar->ToggleTool( m_ID_current_state, FALSE );

        if( m_AuxVToolBar )
            m_AuxVToolBar->ToggleTool( m_ID_current_state, FALSE );
    }
    else
    {
        if( id )
        {
            if( m_VToolBar )
                m_VToolBar->ToggleTool( ID_NO_SELECT_BUTT, FALSE );

            if( m_AuxVToolBar )
                m_AuxVToolBar->ToggleTool( m_ID_current_state, FALSE );
        }
        else if( m_VToolBar )
            m_VToolBar->ToggleTool( ID_NO_SELECT_BUTT, TRUE );
    }

    // New Tool Actif
    if( id )
    {
        if( m_VToolBar )
            m_VToolBar->ToggleTool( id, TRUE );

        if( m_AuxVToolBar )
            m_AuxVToolBar->ToggleTool( id, TRUE );
    }
    else if( m_VToolBar )
        m_VToolBar->ToggleTool( ID_NO_SELECT_BUTT, TRUE );

    m_ID_current_state = id;
}


/*****************************/
/* default virtual functions */
/*****************************/

void WinEDA_DrawFrame::OnGrid( int grid_type )
{
}


int WinEDA_DrawFrame::ReturnBlockCommand( int key )
{
    return 0;
}


void WinEDA_DrawFrame::InitBlockPasteInfos()
{
    GetBaseScreen()->m_BlockLocate.ClearItemsList();
    DrawPanel->ManageCurseur = NULL;
}


void WinEDA_DrawFrame::HandleBlockPlace( wxDC* DC )
{
}


int WinEDA_DrawFrame::HandleBlockEnd( wxDC* DC )
{
    return 0;
}


/*********************************************/
void WinEDA_DrawFrame::AdjustScrollBars()
/*********************************************/
{
    int     xUnit, yUnit;
    wxSize  draw_size, panel_size;
    wxSize  scrollbar_number;
    wxPoint scrollbar_pos;
    BASE_SCREEN* screen = GetBaseScreen();

    if( screen == NULL || DrawPanel == NULL )
        return;

    // The drawing size is twice the current page size.
    draw_size = screen->ReturnPageSize() * 2;

    // Calculate the portion of the drawing that can be displayed in the
    // client area at the current zoom level.
    panel_size = DrawPanel->GetClientSize();
    screen->Unscale( panel_size );

    /* Adjust drawing size when zooming way out to prevent centering around
     * cursor problems. */
    if( panel_size.x > draw_size.x || panel_size.y > draw_size.y )
        draw_size = panel_size;

    draw_size += panel_size / 2;

    if( screen->m_Center )
    {
        screen->m_DrawOrg.x = -draw_size.x / 2;
        screen->m_DrawOrg.y = -draw_size.y / 2;
    }
    else
    {
        screen->m_DrawOrg.x = -panel_size.x / 2;
        screen->m_DrawOrg.y = -panel_size.y / 2;
    }

#ifndef WX_ZOOM
    // Calculate the number of scroll bar units for the given zoom level. */
    scrollbar_number.x =
        wxRound( (double) draw_size.x /
                 (double) screen->Unscale( screen->m_ZoomScalar ) );
    scrollbar_number.y =
        wxRound( (double) draw_size.y /
                 (double) screen->Unscale( screen->m_ZoomScalar ) );

    xUnit = yUnit = screen->m_ZoomScalar;

    if( xUnit <= 1 )
        xUnit = 1;
    if( yUnit <= 1 )
        yUnit = 1;
    xUnit = screen->Unscale( xUnit );
    yUnit = screen->Unscale( yUnit );

    // Calcul de la position, curseur place au centre d'ecran
    scrollbar_pos = screen->m_Curseur - screen->m_DrawOrg;

    scrollbar_pos.x -= panel_size.x / 2;
    scrollbar_pos.y -= panel_size.y / 2;

    if( scrollbar_pos.x <= 0 )
        scrollbar_pos.x = 0;
    if( scrollbar_pos.y <= 0 )
        scrollbar_pos.y = 0;

    scrollbar_pos.x = wxRound( (double) scrollbar_pos.x / (double) xUnit );
    scrollbar_pos.y = wxRound( (double) scrollbar_pos.y / (double) yUnit );
    screen->m_ScrollbarPos    = scrollbar_pos;
    screen->m_ScrollbarNumber = scrollbar_number;

    DrawPanel->SetScrollbars( screen->m_ZoomScalar,
                              screen->m_ZoomScalar,
                              screen->m_ScrollbarNumber.x,
                              screen->m_ScrollbarNumber.y,
                              screen->m_ScrollbarPos.x,
                              screen->m_ScrollbarPos.y, TRUE );
#else
    int x, y, scroll_x, scroll_y;
    double scale_x, scale_y;

    wxClientDC DC( this );

    DrawPanel->PrepareGraphicContext( &DC );
    x = DC.LogicalToDeviceXRel( draw_size.GetWidth() );
    y = DC.LogicalToDeviceYRel( draw_size.GetHeight() );

    scrollbar_pos = screen->m_Curseur - screen->m_DrawOrg;
    scrollbar_pos.x -= panel_size.x / 2;
    scrollbar_pos.y -= panel_size.y / 2;
    scroll_x = DC.LogicalToDeviceXRel( scrollbar_pos.x );
    scroll_y = DC.LogicalToDeviceYRel( scrollbar_pos.y );

    wxLogDebug( wxT( "SetScrollbars(1, 1, %d, %d, %d, %d)" ),
                x, y, scroll_x, scroll_y );

    DrawPanel->SetScrollbars( 1, 1, x, y, scroll_x, scroll_y );
#endif
}


/****************************************************/
void WinEDA_DrawFrame::SetDrawBgColor( int color_num )
/****************************************************/

/* met a jour la couleur de fond pour les trac�s
 *  seules les couleurs BLACK ou WHITE sont autoris�es
 *  le parametre XorMode est mis a jour selon la couleur du fond
 */
{
    if( ( color_num != WHITE ) && ( color_num != BLACK ) )
        color_num = BLACK;
    g_DrawBgColor = color_num;
    if( color_num == WHITE )
    {
        g_XorMode    = GR_NXOR;
        g_GhostColor = BLACK;
    }
    else
    {
        g_XorMode    = GR_XOR;
        g_GhostColor = WHITE;
    }

    if( DrawPanel )
        DrawPanel->SetBackgroundColour( wxColour( ColorRefs[g_DrawBgColor].m_Red,
                                                  ColorRefs[g_DrawBgColor].m_Green,
                                                  ColorRefs[g_DrawBgColor].m_Blue ) );
}


/********************************************************/
void WinEDA_DrawFrame::SetLanguage( wxCommandEvent& event )
/********************************************************/
{
    int id = event.GetId();

    wxGetApp().SetLanguageIdentifier( id );
    if ( wxGetApp().SetLanguage() )
    {
        wxLogDebug( wxT( "Recreating menu bar due to language change." ) );
        ReCreateMenuBar();
        Refresh();
    }
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
 * Function UpdateStatusBar()
 * Displays in the bottom of the main window a stust:
 *  - Absolute Cursor coordinates
 *  - Relative Cursor coordinates (relative to the last coordinate stored
 *     when actiavte the space bar)
 * ( in this status is also displayed the zoom level, but this is not made
 *   by this function )
 */
void WinEDA_DrawFrame::UpdateStatusBar()
{
    wxString        Line;
    int             dx, dy;
    BASE_SCREEN*    screen = GetBaseScreen();

    if( !screen )
        return;

    /* Display Zoom level: zoom = zoom_coeff/ZoomScalar */
    if ( (screen->GetZoom() % screen->m_ZoomScalar) == 0 )
        Line.Printf( wxT( "Z %d" ), screen->GetZoom() / screen->m_ZoomScalar );
    else
        Line.Printf( wxT( "Z %.1f" ),
                     (float)screen->GetZoom() / screen->m_ZoomScalar );
    SetStatusText( Line, 1 );

    /* Display absolute coordinates:  */
    double dXpos = To_User_Unit( g_UnitMetric, screen->m_Curseur.x,
                                 m_InternalUnits );
    double dYpos = To_User_Unit( g_UnitMetric, screen->m_Curseur.y,
                                 m_InternalUnits );
    /*
     * Converting from inches to mm can give some coordinates due to
     * float point precision rounding errors, like 1.999 or 2.001 so
     * round to the nearest drawing precision required by the application.
    */
    if ( g_UnitMetric )
    {
        dXpos = RoundTo0( dXpos, (double)( m_InternalUnits / 10 ) );
        dYpos = RoundTo0( dYpos, (double)( m_InternalUnits / 10 ) );
    }
    if( m_InternalUnits == EESCHEMA_INTERNAL_UNIT )
        Line.Printf( g_UnitMetric ? wxT( "X %.2f  Y %.2f" ) :
                     wxT( "X %.3f  Y %.3f" ), dXpos, dYpos );
    else
        Line.Printf( g_UnitMetric ? wxT( "X %.3f  Y %.3f" ) :
                     wxT( "X %.4f  Y %.4f" ), dXpos, dYpos );
    SetStatusText( Line, 2 );

    /* Display relative coordinates:  */
    dx = screen->m_Curseur.x - screen->m_O_Curseur.x;
    dy = screen->m_Curseur.y - screen->m_O_Curseur.y;
    dXpos = To_User_Unit( g_UnitMetric, dx, m_InternalUnits );
    dYpos = To_User_Unit( g_UnitMetric, dy, m_InternalUnits );
    if ( g_UnitMetric )
    {
        dXpos = RoundTo0( dXpos, (double)( m_InternalUnits / 10 ) );
        dYpos = RoundTo0( dYpos, (double)( m_InternalUnits / 10 ) );
    }
    if( m_InternalUnits == EESCHEMA_INTERNAL_UNIT )
        Line.Printf( g_UnitMetric ? wxT( "X %.2f  Y %.2f" ) :
                     wxT( "X %.3f  Y %.3f" ), dXpos, dYpos );
    else
        Line.Printf( g_UnitMetric ? wxT( "x %.3f  y %.3f" ) :
                     wxT( "x %.4f  y %.4f" ), dXpos, dYpos );

    SetStatusText( Line, 3 );
}

/**
 * Load draw frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get loaded.
 */
void WinEDA_DrawFrame::LoadSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    WinEDA_BasicFrame::LoadSettings();
    cfg->Read( m_FrameName + CursorShapeEntry, &m_CursorShape, ( long )0 );
    cfg->Read( m_FrameName + ShowGridEntry, &m_Draw_Grid, true );
    cfg->Read( m_FrameName + LastGridSizeId, &m_LastGridSizeId, 0L );
}


/**
 * Save draw frame specific configuration settings.
 *
 * Don't forget to call this base method from any derived classes or the
 * settings will not get saved.
 */
void WinEDA_DrawFrame::SaveSettings()
{
    wxASSERT( wxGetApp().m_EDA_Config != NULL );

    wxConfig* cfg = wxGetApp().m_EDA_Config;

    WinEDA_BasicFrame::SaveSettings();
    cfg->Write( m_FrameName + CursorShapeEntry, m_CursorShape );
    cfg->Write( m_FrameName + ShowGridEntry, m_Draw_Grid );
    cfg->Write( m_FrameName + LastGridSizeId, ( long ) m_LastGridSizeId );
}


void WinEDA_DrawFrame::AppendMsgPanel( const wxString& textUpper,
                                       const wxString& textLower,
                                       int color, int pad )
{
    if( MsgPanel == NULL )
        return;

    MsgPanel->AppendMessage( textUpper, textLower, color, pad );
}


void WinEDA_DrawFrame::ClearMsgPanel( void )
{
    if( MsgPanel == NULL )
        return;

    MsgPanel->EraseMsgBox();
}
