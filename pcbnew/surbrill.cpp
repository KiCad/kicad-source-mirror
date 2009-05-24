/****************************/
/* affichage des empreintes */
/****************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"

#include "pcbnew.h"
#include "protos.h"
#include "collectors.h"


#define Pad_fill (Pad_Fill_Item.State == RUN)


/*********************************************************/
void WinEDA_PcbFrame::Liste_Equipot( wxCommandEvent& event )
/*********************************************************/

/* Display a filtered list of equipot names
 *  if an equipot is selected the corresponding tracks and pads are highlighted
 */
{
    NETINFO_ITEM*          net;
    wxString          msg;
    WinEDA_TextFrame* List;
    unsigned ii;

    msg = wxT( "*" );
    Get_Message( _( "Filter for net names:" ), _("Net Filter"), msg, this );
    if( msg.IsEmpty() )
        return;

    List = new WinEDA_TextFrame( this, _( "List Nets" ) );

    for( ii = 0; ii < GetBoard()->m_NetInfo->GetCount() ; ii++ )
    {
        net =  GetBoard()->m_NetInfo->GetItem( ii );
        wxString Line;
        if( !WildCompareString( msg, net->GetNetname(), false ) )
            continue;

        Line.Printf( wxT( "net_code = %3.3d  [%.16s] " ), net->GetNet(),
                    net->GetNetname().GetData() );
        List->Append( Line );
    }

    ii = List->ShowModal();

    List->Destroy();

    if( ii < 0 )
        return;

    for( unsigned jj = 0; jj < GetBoard()->m_NetInfo->GetCount() ; jj++ )
    {
        net =  GetBoard()->m_NetInfo->GetItem( ii );
        if( !WildCompareString( msg, net->GetNetname(), false ) )
            continue;

        if( ii == jj )
        {
            ii = net->GetNet();
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

    // use this scheme because a pad is a higher priority than a track in the
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

    GetBoard()->DrawHighLight( DrawPanel, DC, g_HightLigth_NetCode );
}

