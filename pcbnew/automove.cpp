/****************************************************************/
/* Routines for automatic displacement and rotation of modules. */
/****************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "autorout.h"
#include "cell.h"
#include "pcbnew_id.h"
#include "protos.h"

#include "kicad_device_context.h"

typedef enum {
    FIXE_MODULE,
    FREE_MODULE,
    FIXE_ALL_MODULES,
    FREE_ALL_MODULES
} SelectFixeFct;


static int tri_modules( MODULE** pt_ref, MODULE** pt_compare );


wxString ModulesMaskSelection = wxT( "*" );


/* Called on events (popup menus) relative to automove and autoplace footprints
 */
void WinEDA_PcbFrame::AutoPlace( wxCommandEvent& event )
{
    int        id = event.GetId();
    wxPoint    pos;
    INSTALL_DC( dc, DrawPanel );
    bool       on_state;

    if( m_HToolBar == NULL )
        return;

    wxGetMousePosition( &pos.x, &pos.y );

    switch( id )
    {
    case ID_TOOLBARH_PCB_AUTOPLACE:
    case ID_TOOLBARH_PCB_AUTOROUTE:
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        if( DrawPanel->ManageCurseur
            && DrawPanel->ForceCloseManageCurseur )
        {
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        }
        break;

    default:   // Abort a current command (if any)
        DrawPanel->UnManageCursor( 0, wxCURSOR_ARROW );
        break;
    }

    /* Erase ratsnest if needed */
    if( GetBoard()->IsElementVisible(RATSNEST_VISIBLE) )
        DrawGeneralRatsnest( &dc );
    GetBoard()->m_Status_Pcb |= DO_NOT_SHOW_GENERAL_RASTNEST;

    switch( id )
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
        FixeModule( (MODULE*) GetScreen()->GetCurItem(), TRUE );
        break;

    case ID_POPUP_PCB_AUTOPLACE_FREE_MODULE:
        FixeModule( (MODULE*) GetScreen()->GetCurItem(), FALSE );
        break;

    case ID_POPUP_PCB_AUTOPLACE_FREE_ALL_MODULES:
        FixeModule( NULL, FALSE );
        break;

    case ID_POPUP_PCB_AUTOPLACE_FIXE_ALL_MODULES:
        FixeModule( NULL, TRUE );
        break;

    case ID_POPUP_PCB_AUTOPLACE_CURRENT_MODULE:
        AutoPlaceModule( (MODULE*) GetScreen()->GetCurItem(),
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
        AutoMoveModulesOnPcb( FALSE );
        break;

    case ID_POPUP_PCB_AUTOMOVE_NEW_MODULES:
        AutoMoveModulesOnPcb( TRUE );
        break;

    case ID_POPUP_PCB_REORIENT_ALL_MODULES:
        OnOrientFootprints();
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

    GetBoard()->m_Status_Pcb &= ~DO_NOT_SHOW_GENERAL_RASTNEST;
    Compile_Ratsnest( &dc, true );
    SetToolbars();
}


/* Routine allocation of components in a rectangular format 4 / 3,
 * Starting from the mouse cursor
 * The components with the FIXED status are not normally dives
 * According to the flags:
 * All modules (not fixed) will be left
 * Only PCB modules are not left
 */
void WinEDA_PcbFrame::AutoMoveModulesOnPcb( bool PlaceModulesHorsPcb )
{
    MODULE** pt_Dmod, ** BaseListeModules;
    MODULE*  Module;
    wxPoint  start, current;
    int      Ymax_size, Xsize_allowed;
    int      pas_grille = (int) GetScreen()->GetGridSize().x;
    bool     EdgeExists;
    float    surface;

    if( GetBoard()->m_Modules == NULL )
    {
        DisplayError( this, _( "No modules found!" ) );
        return;
    }

    /* Confirmation */
    if( !IsOK( this, _( "Move modules?" ) ) )
        return;

    EdgeExists = SetBoardBoundaryBoxFromEdgesOnly();

    if( PlaceModulesHorsPcb && !EdgeExists )
    {
        DisplayError( this,
                      _( "Could not automatically place modules. No board outlines detected." ) );
        return;
    }

    Module = GetBoard()->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        Module->Set_Rectangle_Encadrement();
        Module->SetRectangleExinscrit();
    }

    BaseListeModules = GenListeModules( GetBoard(), NULL );

    /* If allocation of modules not PCBs, the cursor is placed below
     * PCB, to avoid placing components in PCB area.
     */
    if( PlaceModulesHorsPcb && EdgeExists )
    {
        if( GetScreen()->m_Curseur.y <
           (GetBoard()->m_BoundaryBox.GetBottom() + 2000) )
            GetScreen()->m_Curseur.y = GetBoard()->m_BoundaryBox.GetBottom() +
                                       2000;
    }

    /* calculating the area occupied by the circuits */
    surface = 0.0;
    for( pt_Dmod = BaseListeModules; *pt_Dmod != NULL; pt_Dmod++ )
    {
        Module = *pt_Dmod;
        if( PlaceModulesHorsPcb && EdgeExists )
        {
            if( GetBoard()->m_BoundaryBox.Inside( Module->m_Pos ) )
                continue;
        }
        surface += Module->m_Surface;
    }

    Xsize_allowed = (int) ( sqrt( surface ) * 4.0 / 3.0 );

    start     = current = GetScreen()->m_Curseur;
    Ymax_size = 0;

    for( pt_Dmod = BaseListeModules; *pt_Dmod != NULL; pt_Dmod++ )
    {
        Module = *pt_Dmod;
        if( Module->IsLocked() )
            continue;

        if( PlaceModulesHorsPcb && EdgeExists )
        {
            if( GetBoard()->m_BoundaryBox.Inside( Module->m_Pos ) )
                continue;
        }

        if( current.x > (Xsize_allowed + start.x) )
        {
            current.x  = start.x;
            current.y += Ymax_size + pas_grille;
            Ymax_size  = 0;
        }

        GetScreen()->m_Curseur.x =
            current.x + Module->m_Pos.x - Module->m_RealBoundaryBox.GetX();
        GetScreen()->m_Curseur.y =
            current.y + Module->m_Pos.y - Module->m_RealBoundaryBox.GetY();
        Ymax_size = MAX( Ymax_size, Module->m_RealBoundaryBox.GetHeight() );

        PutOnGrid( &GetScreen()->m_Curseur );

        Place_Module( Module, NULL, true );

        current.x += Module->m_RealBoundaryBox.GetWidth() + pas_grille;
    }

    MyFree( BaseListeModules );
    DrawPanel->Refresh();
}


/* Update (TRUE or FALSE) FIXED attribute on the module Module
 * or all the modules if Module == NULL
 */
void WinEDA_PcbFrame::FixeModule( MODULE* Module, bool Fixe )
{
    if( Module )
    {
        Module->SetLocked( Fixe );

        Module->DisplayInfo( this );
        OnModify();
    }
    else
    {
        Module = GetBoard()->m_Modules;
        for( ; Module != NULL; Module = Module->Next() )
        {
            if( WildCompareString( ModulesMaskSelection,
                                   Module->m_Reference->m_Text ) )
            {
                Module->SetLocked( Fixe );
                OnModify();
            }
        }
    }
}


/* Create memory allocation by the ordered list of structures D_MODULES
 * Describing the module to move
 * The end of the list is indicated by NULL
 * Also returns the number of modules per NbModules *
 * Deallocates memory after use
 */
MODULE** GenListeModules( BOARD* Pcb, int* NbModules )
{
    MODULE*  Module;
    MODULE** ListeMod, ** PtList;
    int      NbMod;

    /* Reserve memory for descriptions of modules that are to be moved. */
    Module = Pcb->m_Modules;
    NbMod  = 0;
    for( ; Module != NULL; Module = Module->Next() )
        NbMod++;

    ListeMod = (MODULE**) MyZMalloc( (NbMod + 1) * sizeof(MODULE*) );
    if( ListeMod == NULL )
    {
        if( NbModules != NULL )
            *NbModules = 0;
        return NULL;
    }

    PtList = ListeMod;
    Module = Pcb->m_Modules;
    for( ; Module != NULL; Module = Module->Next() )
    {
        *PtList = Module; PtList++;
        Module->SetRectangleExinscrit();
    }

    /* Sort by surface area module largest to smallest */
    qsort( ListeMod, NbMod, sizeof(MODULE * *),
           ( int ( * )( const void*, const void* ) )tri_modules );

    if( NbModules != NULL )
        *NbModules = NbMod;
    return ListeMod;
}


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
