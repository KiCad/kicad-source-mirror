/****************************************************************/
/* Routines de deplacement automatique et rotation  des MODULES */
/* routines et menu d'autoplacement								*/
/****************************************************************/

/* Fichier automove.cpp */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "cell.h"
#include "id.h"

#include "protos.h"


/* variables locales */

typedef enum {
    FIXE_MODULE,
    FREE_MODULE,
    FIXE_ALL_MODULES,
    FREE_ALL_MODULES
} SelectFixeFct;


/* Fonctions locales */
static int tri_modules( MODULE** pt_ref, MODULE** pt_compare );

/* Variables locales */
wxString ModulesMaskSelection = wxT( "*" );
int      ModulesNewOrient;


/******************************************************/
void WinEDA_PcbFrame::AutoPlace( wxCommandEvent& event )
/******************************************************/

/* Traite les selections d'outils et les commandes appelees du menu POPUP
 */
{
    int        id = event.GetId();
    wxPoint    pos;
    wxClientDC dc( DrawPanel );
    bool       on_state;

    if( m_HToolBar == NULL )
        return;

    DrawPanel->PrepareGraphicContext( &dc );

    wxGetMousePosition( &pos.x, &pos.y );

    switch( id )   // Arret eventuel de la commande de déplacement en cours
    {
    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        if( DrawPanel->ManageCurseur
            && DrawPanel->ForceCloseManageCurseur )
        {
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        }
        break;

    default:        // Arret de la commande de déplacement en cours
        if( DrawPanel->ManageCurseur
            && DrawPanel->ForceCloseManageCurseur )
        {
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        }
        m_ID_current_state = 0;
        DisplayToolMsg( wxEmptyString );
        DrawPanel->SetCursor( wxCursor( wxCURSOR_ARROW ) );
        break;
    }

    /* Erase rastnest if needed */
    if( g_Show_Ratsnest )
        DrawGeneralRatsnest( &dc );
    m_Pcb->m_Status_Pcb |= DO_NOT_SHOW_GENERAL_RASTNEST;

    switch( id )   // Traitement des commandes
    {
    case ID_TOOLBARH_PCB_AUTOPLACE:
        on_state = m_HToolBar->GetToolState( ID_TOOLBARH_PCB_AUTOPLACE );
        if( on_state )
        {
            m_HToolBar->ToggleTool( ID_TOOLBARH_PCB_AUTOROUTE, FALSE );
            m_HTOOL_current_state = ID_TOOLBARH_PCB_AUTOPLACE;
        }
        else
            m_HTOOL_current_state = 0;
        break;

    case ID_TOOLBARH_PCB_AUTOROUTE:
        on_state = m_HToolBar->GetToolState( ID_TOOLBARH_PCB_AUTOROUTE );
        if( on_state )
        {
            m_HToolBar->ToggleTool( ID_TOOLBARH_PCB_AUTOPLACE, FALSE );
            m_HTOOL_current_state = ID_TOOLBARH_PCB_AUTOROUTE;
        }
        else
            m_HTOOL_current_state = 0;
        break;

    case ID_POPUP_PCB_AUTOPLACE_FIXE_MODULE:
        FixeModule( (MODULE*) m_CurrentScreen->GetCurItem(), TRUE );
        break;

    case ID_POPUP_PCB_AUTOPLACE_FREE_MODULE:
        FixeModule( (MODULE*) m_CurrentScreen->GetCurItem(), FALSE );
        break;

    case ID_POPUP_PCB_AUTOPLACE_FREE_ALL_MODULES:
        FixeModule( NULL, FALSE );
        break;

    case ID_POPUP_PCB_AUTOPLACE_FIXE_ALL_MODULES:
        FixeModule( NULL, TRUE );
        break;

    case ID_POPUP_PCB_AUTOPLACE_CURRENT_MODULE:
        AutoPlaceModule( (MODULE*) m_CurrentScreen->GetCurItem(),
                        PLACE_1_MODULE, &dc );
        break;

    case ID_POPUP_PCB_AUTOPLACE_ALL_MODULES:
        AutoPlaceModule( NULL, PLACE_ALL, &dc );
        break;

    case ID_POPUP_PCB_AUTOPLACE_NEW_MODULES:
        AutoPlaceModule( NULL, PLACE_OUT_OF_BOARD, &dc );
        break;

    case ID_POPUP_PCB_AUTOPLACE_NEXT_MODULE:
        AutoPlaceModule( NULL, PLACE_INCREMENTAL, &dc );
        break;

    case ID_POPUP_PCB_AUTOMOVE_ALL_MODULES:
        AutoMoveModulesOnPcb( &dc, FALSE );
        break;

    case ID_POPUP_PCB_AUTOMOVE_NEW_MODULES:
        AutoMoveModulesOnPcb( &dc, TRUE );
        break;

    case ID_POPUP_PCB_REORIENT_ALL_MODULES:
        ReOrientModules( ModulesMaskSelection, ModulesNewOrient, FALSE, &dc );
        break;

    case ID_POPUP_PCB_AUTOROUTE_ALL_MODULES:
        Autoroute( &dc, ROUTE_ALL );
        break;

    case ID_POPUP_PCB_AUTOROUTE_MODULE:
        Autoroute( &dc, ROUTE_MODULE );
        break;

    case ID_POPUP_PCB_AUTOROUTE_PAD:
        Autoroute( &dc, ROUTE_PAD );
        break;

    case ID_POPUP_PCB_AUTOROUTE_NET:
        Autoroute( &dc, ROUTE_NET );
        break;

    case ID_POPUP_PCB_AUTOROUTE_RESET_UNROUTED:
        Reset_Noroutable( &dc );
        break;

    case ID_POPUP_PCB_AUTOROUTE_SELECT_LAYERS:
        break;

    default:
        DisplayError( this, wxT( "AutoPlace command error" ) );
        break;
    }

    m_Pcb->m_Status_Pcb &= ~DO_NOT_SHOW_GENERAL_RASTNEST;
    ReCompile_Ratsnest_After_Changes( &dc );
    SetToolbars();
}


/*****************************************************************************/
void WinEDA_PcbFrame::AutoMoveModulesOnPcb( wxDC* DC, bool PlaceModulesHorsPcb )
/*****************************************************************************/

/* Routine de repartition des composants dans un rectangle de format 4 / 3,
 *  partant du curseur souris
 *  Les composants ayant le status FIXE ne sont normalement pas bouges
 *  Selon les flags:
 *      Tous les modules (non fixes) seront repartis
 *      Seuls les modules Hors PCB seront repartis
 */
{
    MODULE** pt_Dmod, ** BaseListeModules;
    MODULE*  Module;
    wxPoint  start, current;
    int      Ymax_size, Xsize_allowed;
    int      pas_grille = m_CurrentScreen->GetGrid().x;
    bool     EdgeExists;
    float    surface;

    if( m_Pcb->m_Modules == NULL )
    {
        DisplayError( this, _( "No Modules!" ), 10 ); return;
    }

    /* Confirmation */
    if( !IsOK( this, _( "Move Modules ?" ) ) )
        return;

    EdgeExists = SetBoardBoundaryBoxFromEdgesOnly();

    if( PlaceModulesHorsPcb && !EdgeExists )
    {
        DisplayError( this,
                      _(
                          "Autoplace modules: No boad edges detected, unable to place modules" ),
                      20 );
        return;
    }

    Module = m_Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext ) // remise a jour du rect d'encadrement
    {
        Module->Set_Rectangle_Encadrement();
        Module->SetRectangleExinscrit();
    }

    BaseListeModules = GenListeModules( m_Pcb, NULL );

    /* Si repartition de modules Hors PCB, le curseur est mis au dessous
     *  du PCB, pour eviter de placer des composants dans la zone PCB
     */
    if( PlaceModulesHorsPcb && EdgeExists )
    {
        if( m_CurrentScreen->m_Curseur.y < (m_Pcb->m_BoundaryBox.GetBottom() + 2000) )
            m_CurrentScreen->m_Curseur.y = m_Pcb->m_BoundaryBox.GetBottom() + 2000;
    }

    /* calcul de la surface occupee par les circuits */
    surface = 0.0;
    for( pt_Dmod = BaseListeModules; *pt_Dmod != NULL; pt_Dmod++ )
    {
        Module = *pt_Dmod;
        if( PlaceModulesHorsPcb && EdgeExists )
        {
            if( m_Pcb->m_BoundaryBox.Inside( Module->m_Pos ) )
                continue;
        }
        surface += Module->m_Surface;
    }

    Xsize_allowed = (int) (sqrt( surface ) * 4.0 / 3.0);

    /* Placement des modules */
    start     = current = m_CurrentScreen->m_Curseur;
    Ymax_size = 0;

    for( pt_Dmod = BaseListeModules; *pt_Dmod != NULL; pt_Dmod++ )
    {
        Module = *pt_Dmod;
        if( Module->IsLocked() )
            continue;

        if( PlaceModulesHorsPcb && EdgeExists )
        {
            if( m_Pcb->m_BoundaryBox.Inside( Module->m_Pos ) )
                continue;
        }

        if( current.x > (Xsize_allowed + start.x) )
        {
            current.x  = start.x;
            current.y += Ymax_size + pas_grille;
            Ymax_size  = 0;
        }

        m_CurrentScreen->m_Curseur.x =
            current.x + Module->m_Pos.x - Module->m_RealBoundaryBox.GetX();
        m_CurrentScreen->m_Curseur.y =
            current.y + Module->m_Pos.y - Module->m_RealBoundaryBox.GetY();
        Ymax_size = MAX( Ymax_size, Module->m_RealBoundaryBox.GetHeight() );

        PutOnGrid( &m_CurrentScreen->m_Curseur );

        Module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
        Place_Module( Module, DC );   /* positionne Module et recalcule cadre */

        current.x += Module->m_RealBoundaryBox.GetWidth() + pas_grille;
    }

    MyFree( BaseListeModules );
    m_CurrentScreen->SetRefreshReq();
}


/***********************************************************/
void WinEDA_PcbFrame::FixeModule( MODULE* Module, bool Fixe )
/***********************************************************/

/* Met a jour (FALSE ou TRUE) l'attribut FIXE sur le module Module,
 *  ou sur tous les modules si Modulle == NULL
 */
{
    if( Module )    /* Traitement du module */
    {
        Module->SetLocked( Fixe );
        
        Module->Display_Infos( this );
        GetScreen()->SetModify();
    }
    else
    {
        Module = m_Pcb->m_Modules;
        for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
        {
            if( WildCompareString( ModulesMaskSelection, Module->m_Reference->m_Text ) )
            {
                Module->SetLocked( Fixe );
                GetScreen()->SetModify();
            }
        }
    }
}


/*******************************************************************/
void WinEDA_PcbFrame::ReOrientModules( const wxString& ModuleMask,
                                       int Orient, bool include_fixe, wxDC* DC )
/*******************************************************************/

/*
 *  Reoriente tous les modules selon masque et attribut, avec la nouvelle
 *  orientation selectionnee
 */
{
    MODULE*  Module;
    wxString line;

    line.Printf( _( "Ok to set module orientation to %d degrees ?" ), Orient / 10 );
    if( !IsOK( this, line ) )
        return;

    Module = m_Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
    {
        if( Module->IsLocked() && !include_fixe )
            continue;

        if( WildCompareString( ModuleMask, Module->m_Reference->m_Text, FALSE ) )
        {
            m_CurrentScreen->SetModify();
            Module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
            Rotate_Module( NULL, Module, Orient, FALSE );
            Module->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
        }
    }
}


/*********************************************************/
MODULE** GenListeModules( BOARD* Pcb, int* NbModules )
/*********************************************************/

/* Cree par allocation memoire la liste ordonnee des structures D_MODULES
 *  decrivant les modules a deplacer
 *  la fin de la liste est signalee par NULL
 *  Retourne egalement le nombre de modules par *NbModules
 *  Penser a desallouer la memoire apres usage
 */
{
    MODULE*  Module;
    MODULE** ListeMod, ** PtList;
    int      NbMod;

    /* Reservation de la memoire pour description des modules que l'on
     *  peut deplacer */
    Module = Pcb->m_Modules;
    NbMod  = 0;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
        NbMod++;

    ListeMod = (MODULE**) MyZMalloc( (NbMod + 1) * sizeof(MODULE *) );
    if( ListeMod == NULL )
    {
        if( NbModules != NULL )
            *NbModules = 0;
        return NULL;
    }

    PtList = ListeMod;
    Module = Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
    {
        *PtList = Module; PtList++;
        Module->SetRectangleExinscrit();
    }

    /* Tri par surface decroissante des modules ( on place les plus gros en 1er) */
    qsort( ListeMod, NbMod, sizeof(MODULE * *),
           ( int( * ) ( const void*, const void* ) )tri_modules );

    if( NbModules != NULL )
        *NbModules = NbMod;
    return ListeMod;
}


/**************************************************/
/* Routine de tri de modules, utilisee par qsort: */
/**************************************************/

static int tri_modules( MODULE** pt_ref, MODULE** pt_compare )
{
    float ff;

    ff = (*pt_ref)->m_Surface - (*pt_compare)->m_Surface;
    if( ff < 0 )
        return 1;
    if( ff > 0 )
        return -1;
    return 0;
}
