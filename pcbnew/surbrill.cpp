/****************************/
/* affichage des empreintes */
/****************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"
#include "collectors.h"


#define Pad_fill (Pad_Fill_Item.State == RUN)

static void Pad_Surbrillance( WinEDA_DrawPanel* panel, wxDC* DC, MODULE* Module, int NetCode );

/* variables locales : */
static int draw_mode;


/*********************************************************/
void WinEDA_PcbFrame::Liste_Equipot( wxCommandEvent& event )
/*********************************************************/

/* Display a filtered list of equipot names
 *  if an equipot is selected the corresponding tracks and pads are highlighted
 */
{
    EQUIPOT*          Equipot;
    wxString          msg;
    WinEDA_TextFrame* List;
    int ii, jj;

    msg = wxT( "*" );
    Get_Message( _( "Filter for net names:" ), _("Net Filter"), msg, this );
    if( msg.IsEmpty() )
        return;

    List = new WinEDA_TextFrame( this, _( "List Nets" ) );

    Equipot = (EQUIPOT*) GetBoard()->m_Equipots;
    for( ; Equipot != NULL; Equipot = (EQUIPOT*) Equipot->Next() )
    {
        wxString Line;
        /* calcul adr relative du nom de la pastille reference de la piste */
        if( !WildCompareString( msg, Equipot->GetNetname(), FALSE ) )
            continue;

        Line.Printf( wxT( "net_code = %3.3d  [%.16s] " ), Equipot->GetNet(),
                    Equipot->GetNetname().GetData() );
        List->Append( Line );
    }

    ii = List->ShowModal();

    List->Destroy();

    if( ii < 0 )
        return;

    /* Recherche du numero de net rellement selectionn�*/
    Equipot = (EQUIPOT*) GetBoard()->m_Equipots;
    for( jj = 0; Equipot != NULL; Equipot = (EQUIPOT*) Equipot->Next() )
    {
        /* calcul adr relative du nom de la pastille reference de la piste */
        if( !WildCompareString( msg, Equipot->GetNetname(), FALSE ) )
            continue;

        if( ii == jj )
        {
            ii = Equipot->GetNet();
            break;
        }
        jj++;
    }

    wxClientDC dc( DrawPanel );

    DrawPanel->PrepareGraphicContext( &dc );

    if( g_HightLigt_Status )
        Hight_Light( &dc );

    g_HightLigth_NetCode = ii;
    Hight_Light( &dc );
}


/**************************************************/
int WinEDA_PcbFrame::Select_High_Light( wxDC* DC )
/**************************************************/

/* Localise track ou pad et met en surbrillance le net correspondant
 *  Retourne le netcode, ou -1 si pas de net localis�*/
{
    if( g_HightLigt_Status )
        Hight_Light( DC );

    // use this scheme because of pad is higher priority than tracks in the
    // search, and finding a pad, instead of a track on a pad,
    // allows us to fire a message to eeschema.

    GENERAL_COLLECTORS_GUIDE guide = GetCollectorsGuide();


    // optionally, modify the "guide" here as needed using its member functions


    m_Collector->Collect( GetBoard(), GENERAL_COLLECTOR::PadsTracksOrZones,
                         GetScreen()->RefPos( true ), guide );

    BOARD_ITEM* item = (*m_Collector)[0];

    if( item )
    {
        switch( item->Type() )
        {
        case TYPE_PAD:
            g_HightLigth_NetCode = ((D_PAD*)item)->GetNet();
            Hight_Light( DC );
            SendMessageToEESCHEMA( item );
            return g_HightLigth_NetCode;

        case TYPE_TRACK:
        case TYPE_VIA:
        case TYPE_ZONE:
            // since these classes are all derived from TRACK, use a common
            // GetNet() function:
            g_HightLigth_NetCode = ((TRACK*)item)->GetNet();
            Hight_Light( DC );
            return g_HightLigth_NetCode;

        case TYPE_ZONE_CONTAINER:
            g_HightLigth_NetCode = ((ZONE_CONTAINER*)item)->GetNet();
            Hight_Light( DC );
            return g_HightLigth_NetCode;

        default:
            ;   // until somebody changes GENERAL_COLLECTOR::PadsOrTracks,
                // this should not happen.
        }
    }

    return -1;      // HitTest() failed.
}


/*******************************************/
void WinEDA_PcbFrame::Hight_Light( wxDC* DC )
/*******************************************/

/*
 *  fonction d'appel de Surbrillance a partir du menu
 *  Met ou supprime la surbrillance d'un net pointe par la souris
 */
{
    g_HightLigt_Status = !g_HightLigt_Status;
    DrawHightLight( DC, g_HightLigth_NetCode );
}


/****************************************************************/
void WinEDA_PcbFrame::DrawHightLight( wxDC* DC, int NetCode )
/****************************************************************/

/* Turn On or OFF the HightLight for trcak and pads with the netcode "NetCode'
 */
{
    if( g_HightLigt_Status )
        draw_mode = GR_SURBRILL | GR_OR;
    else
        draw_mode = GR_AND | GR_SURBRILL;

#if 0   // does not unhighlight properly
    // redraw the zones with the NetCode
    for( SEGZONE* zone = GetBoard()->m_Zone;   zone;   zone = zone->Next() )
    {
        if( zone->GetNet() == NetCode )
        {
            zone->Draw( DrawPanel, DC, draw_mode );
        }
    }
#endif

    // Redraw ZONE_CONTAINERS
    BOARD::ZONE_CONTAINERS& zones = GetBoard()->m_ZoneDescriptorList;
    for( BOARD::ZONE_CONTAINERS::iterator zc = zones.begin();  zc!=zones.end();  ++zc )
    {
        if( (*zc)->GetNet() == NetCode )
        {
            (*zc)->Draw( DrawPanel, DC, draw_mode );
        }
    }

    /* Redraw pads */
    for( MODULE* module = GetBoard()->m_Modules;  module;   module = module->Next() )
    {
        Pad_Surbrillance( DrawPanel, DC, module, NetCode );
    }

    /* Redraw track and vias: */
    for( TRACK* pts = GetBoard()->m_Track;   pts;   pts = pts->Next() )
    {
        if( pts->GetNet() == NetCode )
        {
            pts->Draw( DrawPanel, DC, draw_mode );
        }
    }
}


/*******************************************************/
static void Pad_Surbrillance( WinEDA_DrawPanel* panel,
                              wxDC* DC, MODULE* Module, int NetCode )
/*******************************************************/
/* Mise en Surbrillance des Pads */
{
    D_PAD* pt_pad;

    /* trace des pastilles */
    for( pt_pad = Module->m_Pads;  pt_pad;  pt_pad = pt_pad->Next() )
    {
        if( pt_pad->GetNet() == NetCode )
        {
            pt_pad->Draw( panel, DC, draw_mode );
        }
    }
}
