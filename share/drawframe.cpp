/******************************************************************/
/* drawframe.cpp - fonctions des classes du type WinEDA_DrawFrame */
/******************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"

#include "common.h"

#ifdef PCBNEW
#include "pcbnew.h"
#endif

#ifdef EESCHEMA
#include "program.h"
#include "libcmp.h"
#include "general.h"
#endif

#ifdef CVPCB
#include "pcbnew.h"
#include "cvpcb.h"
#endif

#include <wx/fontdlg.h>

#include "bitmaps.h"

#include "protos.h"

#include "id.h"


/*******************************************************/
/* Constructeur de WinEDA_DrawFrame: la fenetre generale */
/*******************************************************/

WinEDA_DrawFrame::WinEDA_DrawFrame( wxWindow* father, int idtype,
                                    WinEDA_App* parent, const wxString& title,
                                    const wxPoint& pos, const wxSize& size, long style ) :
    WinEDA_BasicFrame( father, idtype, parent, title, pos, size, style )
{
    wxSize minsize;

    m_VToolBar         = NULL;
    m_AuxVToolBar      = NULL;
    m_OptionsToolBar   = NULL;
    m_AuxiliaryToolBar = NULL;
    m_SelGridBox       = NULL;
    m_SelZoomBox       = NULL;
    m_ZoomMaxValue     = 128;

    DrawPanel = NULL;
    MsgPanel  = NULL;
    m_CurrentScreen = NULL;
    m_MenuBar = NULL;       // main meun frame
    m_ID_current_state    = 0;
    m_HTOOL_current_state = 0;
    m_Draw_Axis           = FALSE;          // TRUE pour avoir les axes dessines
    m_Draw_Grid           = FALSE;          // TRUE pour avoir la axes dessinee
    m_Draw_Sheet_Ref      = FALSE;          // TRUE pour avoir le cartouche dessin�
    m_Print_Sheet_Ref     = TRUE;           // TRUE pour avoir le cartouche imprim�
    m_Draw_Auxiliary_Axis = FALSE;          // TRUE pour avoir les axes auxiliares dessines
    m_UnitType = INTERNAL_UNIT_TYPE;        // Internal unit = inch

    // Internal units per inch
    // = 1000 for schema, = 10000 for PCB
    m_InternalUnits = EESCHEMA_INTERNAL_UNIT;
    if( (m_Ident == PCB_FRAME) || (m_Ident == GERBER_FRAME)
       || (m_Ident == CVPCB_DISPLAY_FRAME)
       || (m_Ident == MODULE_EDITOR_FRAME)
        )
        m_InternalUnits = PCB_INTERNAL_UNIT;

    minsize.x = 470;
    minsize.y = 350 + m_MsgFrameHeight;
    SetSizeHints( minsize.x, minsize.y, -1, -1, -1, -1 );

    /* Verification des parametres de creation */
    if( (size.x < minsize.x) || (size.y < minsize.y) )
        SetSize( 0, 0, minsize.x, minsize.y );

    // Creation de la ligne de status
    #define ZOOM_DISPLAY_SIZE 60
    #define COORD_DISPLAY_SIZE 140
    #define UNITS_DISPLAY_SIZE 50
    #define FUNCTION_DISPLAY_SIZE 100
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

    if( m_Ident != DISPLAY3D_FRAME )
    {
        DrawPanel = new WinEDA_DrawPanel( this, -1, wxPoint( 0, 0 ), m_FrameSize );
        MsgPanel  = new WinEDA_MsgPanel( this, -1, wxPoint( 0, m_FrameSize.y ),
                                        wxSize( m_FrameSize.x, m_MsgFrameHeight ) );
        MsgPanel->SetBackgroundColour( wxColour( ColorRefs[LIGHTGRAY].m_Red,
                                                 ColorRefs[LIGHTGRAY].m_Green,
                                                 ColorRefs[LIGHTGRAY].m_Blue ) );
    }
}


/****************************************/
WinEDA_DrawFrame::~WinEDA_DrawFrame()
/****************************************/
{
    if( DrawPanel )  // Required: in WinEDA3D_DrawFrame, DrawPanel == NULL !
        m_Parent->m_EDA_Config->Write( wxT( "AutoPAN" ), DrawPanel->m_AutoPAN_Enable );
}


/****************************************************************/
void WinEDA_DrawFrame::AddFontSelectionMenu( wxMenu* main_menu )
/*****************************************************************/

/* create the submenu for fonte selection and setup font size
 */
{
    wxMenu* fontmenu = new wxMenu();

    ADD_MENUITEM( fontmenu, ID_PREFERENCES_FONT_DIALOG, _( "font for dialog boxes" ),
                  fonts_xpm );
    ADD_MENUITEM( fontmenu, ID_PREFERENCES_FONT_INFOSCREEN, _( "font for info display" ),
                  fonts_xpm );
    ADD_MENUITEM( fontmenu, ID_PREFERENCES_FONT_STATUS, _( "font for Status Line" ),
                  fonts_xpm );
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( main_menu, fontmenu,
        ID_PREFERENCES_FONT, _("&Font selection"),
        _("Choose font type and size for dialogs, infos and status box"),
        fonts_xpm );
}


/********************************************************************/
void WinEDA_DrawFrame::ProcessFontPreferences( wxCommandEvent& event )
/********************************************************************/
{
    int    id = event.GetId();
    wxFont font;

    switch( id )
    {
    case ID_PREFERENCES_FONT:
    case ID_PREFERENCES_FONT_DIALOG:
    case ID_PREFERENCES_FONT_STATUS:
        WinEDA_BasicFrame::ProcessFontPreferences( id );
        break;

    case ID_PREFERENCES_FONT_INFOSCREEN:
    {
        font = wxGetFontFromUser( this, *g_MsgFont );
        if( font.Ok() )
        {
            int pointsize = font.GetPointSize();
            *g_MsgFont = font;
            g_MsgFontPointSize = pointsize;
        }
        break;
    }

    default:
        DisplayError( this, wxT( "WinEDA_DrawFrame::ProcessFontPreferences Internal Error" ) );
        break;
    }
}


/**************************************************************/
void WinEDA_DrawFrame::Affiche_Message( const wxString& message )
/**************************************************************/

/*
 *  Dispaly the meesage on yhe bottomon the frame
 */
{
    SetStatusText( message );
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


/********************************************************/
void WinEDA_DrawFrame::OnSelectGrid( wxCommandEvent& event )
/********************************************************/
// Virtual function
{
    if( m_SelGridBox == NULL )
        return;                        // Should not occurs

    int id = m_SelGridBox->GetChoice();
    if( id < 0 )
        return;

    BASE_SCREEN*    screen = GetBaseScreen();

    screen->m_Curseur = DrawPanel->GetScreenCenterRealPosition();
    wxSize grid = screen->GetGrid();
    screen->SetGrid( g_GridList[id] );
    wxSize newgrid = screen->GetGrid();
    if( newgrid.x != grid.x || newgrid.y != grid.y )
        Recadre_Trace( FALSE );
}


/********************************************************/
void WinEDA_DrawFrame::OnSelectZoom( wxCommandEvent& event )  // fonction virtuelle
/********************************************************/

/* Set the zoom when selected by the Zoom List Box
 *  Note:
 *      position 0 = Fit in Page
 *      position >= 1 = zoom (1 to zoom max)
 *      last position : special zoom
 */
{
    if( m_SelZoomBox == NULL )
        return;                        //Ne devrait pas se produire!

    int id = m_SelZoomBox->GetChoice();

    if( id < 0 )
        return;             // No selection

    if( id == 0 )           // Auto zoom (Fit in Page)
    {
        Zoom_Automatique( TRUE );
    }
    else if( id == (int) (m_SelZoomBox->GetCount() - 1) )   // Dummy position: unlisted zoom
        return;
    else                                                    // zooml 1 to zoom max
    {
        id--;
        int zoom = 1 << id;
        if( zoom > m_ZoomMaxValue )
            zoom = m_ZoomMaxValue;
        if( GetBaseScreen()->GetZoom() == zoom )
            return;
        GetBaseScreen()->m_Curseur = DrawPanel->GetScreenCenterRealPosition();
        GetBaseScreen()->SetZoom( zoom );
        Recadre_Trace( FALSE );
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
}


/********************************************************/
void WinEDA_DrawFrame::DisplayToolMsg( const wxString msg )
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
            m_OptionsToolBar->SetSize( Auxtoolbar_size.x, 0, size.x, opt_size.y );
        }
        else
        {
            opt_size.x = m_OptionsToolBar->GetSize().x;
            opt_size.y = 0;
            m_OptionsToolBar->SetSize( 0, Auxtoolbar_size.y, opt_size.x, size.y );
        }
    }

    if( DrawPanel )
    {
        DrawPanel->SetSize( size.x - Vtoolbar_size.x - opt_size.x, size.y - opt_size.y - 1 );
        DrawPanel->Move( opt_size.x, opt_size.y + Auxtoolbar_size.y + 1 );
    }

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
    bool redraw = false;

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

#ifdef PCBNEW
    // handle color changes for transitions in and out of ID_TRACK_BUTT
    if( (m_ID_current_state==ID_TRACK_BUTT && id!=ID_TRACK_BUTT)
     || (m_ID_current_state!=ID_TRACK_BUTT && id==ID_TRACK_BUTT) )
    {
        if( DisplayOpt.ContrastModeDisplay )
            redraw = true;
    }
#endif

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

    // must do this after the tool has been set, otherwise pad::Draw() does
    // not show proper color when DisplayOpt.ContrastModeDisplay is true.
    if( redraw )
        ReDrawPanel();
}


/********************************************/
void WinEDA_DrawFrame::OnZoom( int zoom_type )
/********************************************/

/* Fonction de traitement du zoom
 *  Modifie le facteur de zoom et reaffiche l'ecran
 *  Pour les commandes par menu Popup ou par le clavier, le curseur est
 *  replac� au centre de l'ecran
 */
{
    if( DrawPanel == NULL )
        return;

    BASE_SCREEN*    screen = GetBaseScreen();
    bool    move_mouse_cursor = FALSE;
    int     x, y;
    wxPoint old_pos;

    DrawPanel->GetViewStart( &x, &y );
    old_pos = GetBaseScreen()->m_Curseur;

    switch( zoom_type )
    {
    case ID_POPUP_ZOOM_IN:
    case ID_ZOOM_IN_KEY:
        move_mouse_cursor = TRUE;
        // fall thru

    case ID_ZOOM_IN_BUTT:
        if( zoom_type == ID_ZOOM_IN_BUTT )
            GetBaseScreen()->m_Curseur = DrawPanel->GetScreenCenterRealPosition();

        screen->SetPreviousZoom();

        Recadre_Trace( move_mouse_cursor );
        break;

    case ID_POPUP_ZOOM_OUT:
    case ID_ZOOM_OUT_KEY:
        move_mouse_cursor = TRUE;
        // fall thru

    case ID_ZOOM_OUT_BUTT:
        if( zoom_type == ID_ZOOM_OUT_BUTT )
            screen->m_Curseur = DrawPanel->GetScreenCenterRealPosition();
        screen->SetNextZoom();
        Recadre_Trace( move_mouse_cursor );
        break;

    case ID_POPUP_ZOOM_REDRAW:
    case ID_ZOOM_REDRAW_KEY:
    case ID_ZOOM_REDRAW_BUTT:
        ReDrawPanel();
        break;

    case ID_POPUP_ZOOM_CENTER:
    case ID_ZOOM_CENTER_KEY:
        Recadre_Trace( TRUE );
        break;

    case ID_ZOOM_PAGE_BUTT:
    case ID_ZOOM_AUTO:
    case ID_POPUP_ZOOM_AUTO:
        Zoom_Automatique( FALSE );
        break;

    case ID_ZOOM_PANNING_UP:
        OnPanning( ID_ZOOM_PANNING_UP );
        break;

    case ID_ZOOM_PANNING_DOWN:
        OnPanning( ID_ZOOM_PANNING_DOWN );
        break;

    case ID_ZOOM_PANNING_LEFT:
        OnPanning( ID_ZOOM_PANNING_LEFT );
        DrawPanel->CursorOn( NULL );
        break;

    case ID_ZOOM_PANNING_RIGHT:
        OnPanning( ID_ZOOM_PANNING_RIGHT );
        break;


    default:
        wxMessageBox( wxT( "WinEDA_DrawFrame::OnZoom switch Error" ) );
        break;
    }

    Affiche_Status_Box();
}


/**********************************************/
void WinEDA_DrawFrame::OnPanning( int direction )
/**********************************************/

/* Fonction de traitement du zoom
 *  Modifie le facteur de zoom et reaffiche l'ecran
 *  Pour les commandes par menu Popup ou par le clavier, le curseur est
 *  replac� au centre de l'ecran
 */
{
    if( DrawPanel == NULL )
        return;

    int        delta;
    wxClientDC dc( DrawPanel );

    int        x, y;


    DrawPanel->PrepareGraphicContext( &dc );
    DrawPanel->GetViewStart( &x, &y );  // x and y are in scroll unit, not in pixels
    delta = DrawPanel->m_ScrollButt_unit;

    switch( direction )
    {
    case ID_ZOOM_PANNING_UP:
        y -= delta;
        break;

    case ID_ZOOM_PANNING_DOWN:
        y += delta;
        break;

    case ID_ZOOM_PANNING_LEFT:
        x -= delta;
        break;

    case ID_ZOOM_PANNING_RIGHT:
        x += delta;
        break;

    default:
        wxMessageBox( wxT( "WinEDA_DrawFrame::OnPanning Error" ) );
        break;
    }

    DrawPanel->Scroll( x, y );

    /* Place le curseur souris sur le curseur SCHEMA*/
    DrawPanel->MouseToCursorSchema();
}


/*****************************/
/* default virtual fonctions */
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
    GetBaseScreen()->BlockLocate.m_BlockDrawStruct = NULL;
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
    wxSize  draw_size, panel_size;
    wxSize  scrollbar_number;
    wxPoint scrollbar_pos;

    BASE_SCREEN*    screen = GetBaseScreen();

    int     zoom = screen->GetZoom();
    int     xUnit, yUnit;

    if( screen == NULL )
        return;
    if( DrawPanel == NULL )
        return;

    draw_size = screen->ReturnPageSize();

    // La zone d'affichage est reglee a une taille double de la feuille de travail:
    draw_size.x *= 2; draw_size.y *= 2;

    // On utilise le centre de l'ecran comme position de reference, donc
    // la surface de trace doit etre augmentee
    panel_size    = DrawPanel->GetClientSize();
    panel_size.x *= zoom; panel_size.y *= zoom;
    draw_size.x  += panel_size.x / 2;
    draw_size.y  += panel_size.y / 2;


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

    // DrawOrg est rendu multiple du zoom min :
    screen->m_DrawOrg.x -= screen->m_DrawOrg.x % 256;
    screen->m_DrawOrg.y -= screen->m_DrawOrg.y % 256;

    // Calcul du nombre de scrolls  (en unites de scrool )
    scrollbar_number.x = draw_size.x / (DrawPanel->m_Scroll_unit * zoom);
    scrollbar_number.y = draw_size.y / (DrawPanel->m_Scroll_unit * zoom);

    xUnit = yUnit = DrawPanel->m_Scroll_unit;

    if( xUnit <= 1 )
        xUnit = 1;
    if( yUnit <= 1 )
        yUnit = 1;
    xUnit *= zoom; yUnit *= zoom;

    // Calcul de la position, curseur place au centre d'ecran
    scrollbar_pos = screen->m_Curseur;

    scrollbar_pos.x -= screen->m_DrawOrg.x;
    scrollbar_pos.y -= screen->m_DrawOrg.y;

    scrollbar_pos.x -= panel_size.x / 2;
    scrollbar_pos.y -= panel_size.y / 2;

    if( scrollbar_pos.x < 0 )
        scrollbar_pos.x = 0;
    if( scrollbar_pos.y < 0 )
        scrollbar_pos.y = 0;

    scrollbar_pos.x /= xUnit;
    scrollbar_pos.y /= yUnit;
    screen->m_ScrollbarPos    = scrollbar_pos;
    screen->m_ScrollbarNumber = scrollbar_number;

    DrawPanel->SetScrollbars( DrawPanel->m_Scroll_unit,
                              DrawPanel->m_Scroll_unit,
                              screen->m_ScrollbarNumber.x,
                            screen->m_ScrollbarNumber.y,
                            screen->m_ScrollbarPos.x,
                            screen->m_ScrollbarPos.y, TRUE );
}


/****************************************************/
void WinEDA_DrawFrame::SetDrawBgColor( int color_num )
/****************************************************/

/* met a jour la couleur de fond pour les trac�s
 *  seules les couleurs BLACK ou WHITE sont autoris�es
 *  le parametre XorMode est mis a jour selon la couleur du fond
 */
{
    if( (color_num != WHITE) && (color_num != BLACK) )
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

    m_Parent->SetLanguageIdentifier( id );
    m_Parent->SetLanguage();
}


/***********************************************/
void WinEDA_DrawFrame::Affiche_Status_Box()
/***********************************************/

/* Routine d'affichage du zoom et des coord curseur.
 */
{
    wxString        Line;
    int             dx, dy;
    BASE_SCREEN*    screen = GetBaseScreen();

    if( !screen )
        return;

    /* affichage Zoom et coordonnees absolues */
    Line.Printf( wxT( "Z %d" ), screen->GetZoom() );
    SetStatusText( Line, 1 );

    Line.Printf( g_UnitMetric ? wxT( "X %.3f  Y %.3f" ) : wxT( "X %.4f  Y %.4f" ),
                To_User_Unit( g_UnitMetric, screen->m_Curseur.x,
                              m_InternalUnits ),
                To_User_Unit( g_UnitMetric, screen->m_Curseur.y,
                              m_InternalUnits ) );
    SetStatusText( Line, 2 );

    /* affichage des coordonnees relatives  */
    dx = screen->m_Curseur.x - screen->m_O_Curseur.x;
    dy = screen->m_Curseur.y - screen->m_O_Curseur.y;

    Line.Printf( g_UnitMetric ? wxT( "x %.3f  y %.3f" ) : wxT( "x %.4f  y %.4f" ),
                To_User_Unit( g_UnitMetric, dx, m_InternalUnits ),
                To_User_Unit( g_UnitMetric, dy, m_InternalUnits ) );

    SetStatusText( Line, 3 );

#ifdef PCBNEW
    if( DisplayOpt.DisplayPolarCood )  /* Display coordonnee polaire */
    {
        double theta, ro;
        if( (dx == 0) && (dy == 0) )
            theta = 0.0;
        else
            theta = atan2( (double) -dy, (double) dx );
        theta = theta * 180 / M_PI;

        ro = sqrt( ( (double) dx * dx ) + ( (double) dy * dy ) );
        Line.Printf( g_UnitMetric ? wxT( "Ro %.3f Th %.1f" ) : wxT( "Ro %.4f Th %.1f" ),
                     To_User_Unit( g_UnitMetric, (int) round( ro ), m_InternalUnits ),
                     theta );
        SetStatusText( Line, 0 );
    }
#endif
}
