/************/
/* zoom.cpp */
/************/

/*
 *  Fonctions de gestion du zoom, du pas de grille et du
 *  recadrage automatique
 */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "macros.h"

#ifdef EESCHEMA
#include "program.h"
#include "libcmp.h"
#include "general.h"
#endif

#ifdef PCBNEW
#include "pcbnew.h"
#endif

#ifdef CVPCB
#include "cvpcb.h"
#endif

#include "bitmaps.h"

#include "protos.h"

#include "id.h"


/* Fonctions locales: */


/**************************************************/
void WinEDA_DrawFrame::Recadre_Trace( bool ToMouse )
/**************************************************/

/* Calcule les offsets de trace.
 *  Les offsets sont ajustés a un multiple du pas de grille
 *  si ToMouse == TRUE, le curseur souris (curseur "systeme") est replace
 *  en position curseur graphique (curseur kicad)
 * 
 *  Note: Mac OS ** does not ** allow moving mouse cursor by program.
 */
{
    /* decalages a apporter au trace en coordonnees ecran */
    PutOnGrid( &m_CurrentScreen->m_Curseur );
    AdjustScrollBars();

    ReDrawPanel();

    /* Place le curseur souris sur le curseur SCHEMA*/
    if( ToMouse == TRUE )
        DrawPanel->MouseToCursorSchema();
}


/************************************************/
void WinEDA_DrawFrame::PutOnGrid( wxPoint* coord )
/************************************************/
/* retourne la valeur de la coordonnee coord sur le point de grille le plus proche */
{
    double ftmp;

    if( !m_CurrentScreen->m_UserGridIsON )
    {
        wxSize grid_size = m_CurrentScreen->GetGrid();
        
        ftmp     = (double) coord->x / grid_size.x;
        coord->x = ( (int) round( ftmp ) ) * grid_size.x;

        ftmp     = (double) coord->y / grid_size.y;
        coord->y = ( (int) round( ftmp ) ) * grid_size.y;
    }
    else
    {
        double pasx = m_CurrentScreen->m_UserGrid.x * m_InternalUnits;
        double pasy = m_CurrentScreen->m_UserGrid.y * m_InternalUnits;
        
        if( m_CurrentScreen->m_UserGridUnit != INCHES )
        {
            pasx /= 25.4; 
            pasy /= 25.4;
        }
        
        int    nn = (int) round( coord->x / pasx );
        coord->x = (int) round( pasx * nn );
        
        nn = (int) round( coord->y / pasy );
        coord->y = (int) round( pasy * nn );
    }
}


/**************************************************************/
void WinEDA_DrawFrame::Zoom_Automatique( bool move_mouse_cursor )
/**************************************************************/

/* Affiche le Schema au meilleur zoom au meilleur centrage pour le dessin
 *  de facon a avoir toute la feuille affichee a l'ecran
 */
{
    int bestzoom;

    bestzoom = BestZoom();
    m_CurrentScreen->SetZoom( bestzoom );
    Recadre_Trace( move_mouse_cursor );
}


/*************************************************/
void WinEDA_DrawFrame::Window_Zoom( EDA_Rect& Rect )
/*************************************************/

/* Compute the zoom factor and the new draw offset to draw the
 *  selected area (Rect) in full window screen
 */
{
    int    ii, jj;
    int    bestzoom;
    wxSize size;

    /* Compute the best zoom */
    Rect.Normalize();
    size     = DrawPanel->GetClientSize();
    ii       = Rect.GetSize().x / size.x;
    jj       = Rect.GetSize().y / size.y;
    bestzoom = MAX( ii, jj );
    if( bestzoom <= 0 )
        bestzoom = 1;

    m_CurrentScreen->SetZoom( bestzoom );

    m_CurrentScreen->m_Curseur = Rect.Centre();
    Recadre_Trace( TRUE );
}


/*****************************************************************/
void WinEDA_DrawPanel::Process_Popup_Zoom( wxCommandEvent& event )
/*****************************************************************/

/* Gere les commandes de zoom appelées par le menu Popup
 *  Toute autre commande est transmise a Parent->Process_Special_Functions(event)
 */
{
    int        id = event.GetId();
    wxClientDC dc( this );

    switch( id )
    {
    case ID_POPUP_ZOOM_PLUS:
    case ID_POPUP_ZOOM_MOINS:
    case ID_POPUP_ZOOM_CENTER:
    case ID_POPUP_ZOOM_AUTO:
    case ID_POPUP_ZOOM_REDRAW:
        m_Parent->OnZoom( id );
        break;

    case ID_POPUP_ZOOM_SELECT:
        break;

    case ID_POPUP_CANCEL:
        MouseToCursorSchema();
        break;

    case ID_POPUP_ZOOM_LEVEL_1:
        m_Parent->m_CurrentScreen->SetZoom( 1 );
        m_Parent->Recadre_Trace( TRUE );
        break;

    case ID_POPUP_ZOOM_LEVEL_2:
        m_Parent->m_CurrentScreen->SetZoom( 2 );
        m_Parent->Recadre_Trace( TRUE );
        break;

    case ID_POPUP_ZOOM_LEVEL_4:
        m_Parent->m_CurrentScreen->SetZoom( 4 );
        m_Parent->Recadre_Trace( TRUE );
        break;

    case ID_POPUP_ZOOM_LEVEL_8:
        m_Parent->m_CurrentScreen->SetZoom( 8 );
        m_Parent->Recadre_Trace( TRUE );
        break;

    case ID_POPUP_ZOOM_LEVEL_16:
        m_Parent->m_CurrentScreen->SetZoom( 16 );
        m_Parent->Recadre_Trace( TRUE );
        break;

    case ID_POPUP_ZOOM_LEVEL_32:
        m_Parent->m_CurrentScreen->SetZoom( 32 );
        m_Parent->Recadre_Trace( TRUE );
        break;

    case ID_POPUP_ZOOM_LEVEL_64:
        m_Parent->m_CurrentScreen->SetZoom( 64 );
        m_Parent->Recadre_Trace( TRUE );
        break;

    case ID_POPUP_ZOOM_LEVEL_128:
        m_Parent->m_CurrentScreen->SetZoom( 128 );
        m_Parent->Recadre_Trace( TRUE );
        break;

    case ID_POPUP_ZOOM_LEVEL_256:
        m_Parent->m_CurrentScreen->SetZoom( 256 );
        m_Parent->Recadre_Trace( TRUE );
        break;

    case ID_POPUP_ZOOM_LEVEL_512:
        m_Parent->m_CurrentScreen->SetZoom( 512 );
        m_Parent->Recadre_Trace( TRUE );
        break;

    case ID_POPUP_ZOOM_LEVEL_1024:
        m_Parent->m_CurrentScreen->SetZoom( 1024 );
        m_Parent->Recadre_Trace( TRUE );
        break;

    case ID_POPUP_ZOOM_LEVEL_2048:
        m_Parent->m_CurrentScreen->SetZoom( 2048 );
        m_Parent->Recadre_Trace( TRUE );
        break;

    case ID_POPUP_GRID_LEVEL_1:
        m_Parent->m_CurrentScreen->SetGrid( wxSize( 1, 1 ) );
        m_Parent->ReDrawPanel();
        break;

    case ID_POPUP_GRID_LEVEL_2:
        m_Parent->m_CurrentScreen->SetGrid( wxSize( 2, 2 ) );
        m_Parent->ReDrawPanel();
        break;

    case ID_POPUP_GRID_LEVEL_5:
        m_Parent->m_CurrentScreen->SetGrid( wxSize( 5, 5 ) );
        m_Parent->ReDrawPanel();
        break;

    case ID_POPUP_GRID_LEVEL_10:
        m_Parent->m_CurrentScreen->SetGrid( wxSize( 10, 10 ) );
        m_Parent->ReDrawPanel();
        break;

    case ID_POPUP_GRID_LEVEL_20:
        m_Parent->m_CurrentScreen->SetGrid( wxSize( 20, 20 ) );
        m_Parent->ReDrawPanel();
        break;

    case ID_POPUP_GRID_LEVEL_25:
        m_Parent->m_CurrentScreen->SetGrid( wxSize( 25, 25 ) );
        m_Parent->ReDrawPanel();
        break;

    case ID_POPUP_GRID_LEVEL_50:
        m_Parent->m_CurrentScreen->SetGrid( wxSize( 50, 50 ) );
        m_Parent->ReDrawPanel();
        break;

    case ID_POPUP_GRID_LEVEL_100:
        m_Parent->m_CurrentScreen->SetGrid( wxSize( 100, 100 ) );
        m_Parent->ReDrawPanel();
        break;

    case ID_POPUP_GRID_LEVEL_200:
        m_Parent->m_CurrentScreen->SetGrid( wxSize( 200, 200 ) );
        m_Parent->ReDrawPanel();
        break;

    case ID_POPUP_GRID_LEVEL_250:
        m_Parent->m_CurrentScreen->SetGrid( wxSize( 250, 250 ) );
        m_Parent->ReDrawPanel();
        break;

    case ID_POPUP_GRID_LEVEL_500:
        m_Parent->m_CurrentScreen->SetGrid( wxSize( 500, 500 ) );
        m_Parent->ReDrawPanel();
        break;

    case ID_POPUP_GRID_LEVEL_1000:
        m_Parent->m_CurrentScreen->SetGrid( wxSize( 1000, 1000 ) );
        m_Parent->ReDrawPanel();
        break;

    case ID_POPUP_GRID_USER:
        m_Parent->m_CurrentScreen->SetGrid( wxSize( -1, -1 ) );
        m_Parent->ReDrawPanel();
        break;

    default:
        DisplayError( this, wxT( "WinEDA_DrawPanel::Process_Popup_Zoom() ID error" ) );
        break;
    }

    m_Parent->Affiche_Status_Box();
}


class grid_list_struct
{
public:
    int           m_Value;
    int           m_Id;
};


/*************************************************************/
void WinEDA_DrawPanel::AddMenuZoom( wxMenu* MasterMenu )
/*************************************************************/

/* add the zoom list menu the the MasterMenu.
 *  used in OnRightClick(wxMouseEvent& event)
 */
{
    int              zoom;
    wxSize           grid;
    int              zoom_value;
    wxString         zoom_msg = _( "Zoom: " );
    wxString         grid_msg = _( "Grid: " ), msg;
    int              ii;
    wxString         line;

    static const grid_list_struct grid_list_pcb[] =
    {
        { 1000, ID_POPUP_GRID_LEVEL_1000 },
        { 500,  ID_POPUP_GRID_LEVEL_500  },
        { 250,  ID_POPUP_GRID_LEVEL_250  },
        { 200,  ID_POPUP_GRID_LEVEL_200  },
        { 100,  ID_POPUP_GRID_LEVEL_100  },
        { 50,   ID_POPUP_GRID_LEVEL_50   },
        { 25,   ID_POPUP_GRID_LEVEL_25   },
        { 20,   ID_POPUP_GRID_LEVEL_20   },
        { 10,   ID_POPUP_GRID_LEVEL_10   },
        { 5,    ID_POPUP_GRID_LEVEL_5    },
        { 2,    ID_POPUP_GRID_LEVEL_2    },
        { 1,    ID_POPUP_GRID_LEVEL_1    },
        { 0,    ID_POPUP_GRID_USER       }
    };

    static const grid_list_struct grid_list_schematic[] =
    {
        { 50, ID_POPUP_GRID_LEVEL_50 },
        { 25, ID_POPUP_GRID_LEVEL_25 },
        { 10, ID_POPUP_GRID_LEVEL_10 },
        { 5,  ID_POPUP_GRID_LEVEL_5  },
        { 2,  ID_POPUP_GRID_LEVEL_2  },
        { 1,  ID_POPUP_GRID_LEVEL_1  },
        { 0,  ID_POPUP_GRID_USER     }
    };


    ADD_MENUITEM( MasterMenu, ID_POPUP_ZOOM_CENTER, _( "Center" ), zoom_center_xpm );
    ADD_MENUITEM( MasterMenu, ID_POPUP_ZOOM_PLUS, _( "Zoom +" ), zoom_in_xpm );
    ADD_MENUITEM( MasterMenu, ID_POPUP_ZOOM_MOINS, _( "Zoom -" ), zoom_out_xpm );

    wxMenu* zoom_choice = new wxMenu;
    ADD_MENUITEM_WITH_SUBMENU( MasterMenu, zoom_choice,
                               ID_POPUP_ZOOM_SELECT, _( "Zoom Select" ), zoom_select_xpm );

    ADD_MENUITEM( MasterMenu, ID_POPUP_ZOOM_AUTO, _( "Auto" ), zoom_optimal_xpm );
    ADD_MENUITEM( MasterMenu, ID_POPUP_ZOOM_REDRAW, _( "Redraw" ), repaint_xpm );

    /* Create the basic zoom list: */
    zoom = m_Parent->m_CurrentScreen->GetZoom();
    zoom_value = 1;
    for( ii = 0; zoom_value <= m_Parent->m_ZoomMaxValue; zoom_value <<= 1, ii++ ) // Create zoom choice 1 .. zoom max
    {
        line.Printf( wxT( "%u" ), zoom_value );
        zoom_choice->Append( ID_POPUP_ZOOM_LEVEL_1 + ii,
                             zoom_msg + line, wxEmptyString, TRUE );
        if( zoom == zoom_value )
            zoom_choice->Check( ID_POPUP_ZOOM_LEVEL_1 + ii, TRUE );
    }

    wxMenu* grid_choice = new wxMenu;
    ADD_MENUITEM_WITH_SUBMENU( MasterMenu, grid_choice,
                               ID_POPUP_GRID_SELECT, _( "Grid Select" ), grid_select_xpm );

    grid = m_Parent->m_CurrentScreen->GetGrid();

	// Create grid list
    switch( m_Parent->m_Ident )
    {
    case MODULE_EDITOR_FRAME:
    case GERBER_FRAME:
    case PCB_FRAME:
    case CVPCB_DISPLAY_FRAME:
        for( ii = 0; ; ii++ )
        {
			if ( grid_list_pcb[ii].m_Value )
			{
				double grid_value = To_User_Unit(g_UnitMetric, grid_list_pcb[ii].m_Value,
					((WinEDA_DrawFrame*)m_Parent)->m_InternalUnits);
				if ( g_UnitMetric == 0)	// inches
					line.Printf(wxT("%g mils"), grid_value*1000);
				else
					line.Printf(wxT("%g mm"), grid_value);
			}
			else line = _("grid user");
            msg = grid_msg + line;
            grid_choice->Append( grid_list_pcb[ii].m_Id, msg, wxEmptyString, TRUE );
            if( grid_list_pcb[ii].m_Value <= 0 )
            {
                if( m_Parent->m_CurrentScreen->m_UserGridIsON )
                    grid_choice->Check( grid_list_pcb[ii].m_Id, TRUE );
                break;
            }
            if( grid.x == grid_list_pcb[ii].m_Value )
                grid_choice->Check( grid_list_pcb[ii].m_Id, TRUE );
        }

        break;

    case SCHEMATIC_FRAME:
    case LIBEDITOR_FRAME:
        for( ii = 0; ; ii++ )
        {
            if( grid_list_schematic[ii].m_Value <= 0 )
                break;
			double grid_value = To_User_Unit(g_UnitMetric, grid_list_schematic[ii].m_Value,
				((WinEDA_DrawFrame*)m_Parent)->m_InternalUnits);
			if ( g_UnitMetric == 0)	// inches
				line.Printf(wxT("%g mils"), grid_value*1000);
			else
				line.Printf(wxT("%g mm"), grid_value);

            msg = grid_msg + line;
            grid_choice->Append( grid_list_schematic[ii].m_Id,
                                 msg, wxEmptyString, TRUE );
            if( grid.x == grid_list_schematic[ii].m_Value )
                grid_choice->Check( grid_list_schematic[ii].m_Id, TRUE );
        }

        break;

    case VIEWER_FRAME:
        break;
    }

    MasterMenu->AppendSeparator();
    ADD_MENUITEM( MasterMenu, ID_POPUP_CANCEL, _( "Close" ), cancel_xpm );
}


/**********************************************************/
void WinEDA_DrawFrame::Process_Zoom( wxCommandEvent& event )
/**********************************************************/

/* fonction de traitement des boutons de Zoom.
 *  Appelle simplement la fonction de traitement du Zoom de la
 *  fenetre active.
 */
{
    int id = event.GetId();

    switch( id )
    {
    case ID_ZOOM_PLUS_BUTT:
    case ID_ZOOM_MOINS_BUTT:
    case ID_ZOOM_REDRAW_BUTT:
    case ID_ZOOM_PAGE_BUTT:
        OnZoom( id );
        break;

    default:
        DisplayError( this, wxT( "WinEDA_DrawFrame::Process_Zoom id Error" ) );
        break;
    }
}
