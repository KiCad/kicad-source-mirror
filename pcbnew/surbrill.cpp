/*******************/
/* Highlight nets. */
/*******************/

#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "collectors.h"


#define Pad_fill (Pad_Fill_Item.State == RUN)


/** Function ListNetsAndSelect
 * called by a command event
 * displays the sorted list of nets in a dialog frame
 * If a net is selected, it is highlighted
 */
void WinEDA_PcbFrame::ListNetsAndSelect( wxCommandEvent& event )
{
    NETINFO_ITEM* net;
    wxString      netFilter;
    int           selection;

    netFilter = wxT( "*" );
    Get_Message( _( "Filter for net names:" ), _( "Net Filter" ),
                 netFilter, this );
    if( netFilter.IsEmpty() )
        return;

    WinEDA_TextFrame List( this, _( "List Nets" ) );

    for( unsigned ii = 0; ii < GetBoard()->m_NetInfo->GetCount(); ii++ )
    {
        net = GetBoard()->m_NetInfo->GetNetItem( ii );
        wxString Line;
        if( !WildCompareString( netFilter, net->GetNetname(), false ) )
            continue;

        Line.Printf( wxT( "net_code = %3.3d  [%.16s] " ), net->GetNet(),
                     GetChars( net->GetNetname() ) );
        List.Append( Line );
    }

    selection = List.ShowModal();

    if( selection < 0 )
        return;

    bool     found   = false;
    unsigned netcode = (unsigned) selection;

    // Search for the net selected.
    for( unsigned ii = 0; ii < GetBoard()->m_NetInfo->GetCount(); ii++ )
    {
        net = GetBoard()->m_NetInfo->GetNetItem( ii );
        if( !WildCompareString( netFilter, net->GetNetname(), false ) )
            continue;

        if( ii == netcode )
        {
            netcode = net->GetNet();
            found   = true;
            break;
        }
    }

    if( found )
    {
        wxClientDC dc( DrawPanel );

        DrawPanel->PrepareGraphicContext( &dc );

        if( g_HightLigt_Status )
            Hight_Light( &dc );

        g_HightLigth_NetCode = netcode;
        Hight_Light( &dc );
    }
}


/* Locate track or pad and highlight the corresponding net
 * Returns the Netcode, or -1 if no net located.
 */
int WinEDA_PcbFrame::Select_High_Light( wxDC* DC )
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
            g_HightLigth_NetCode = ( (D_PAD*) item )->GetNet();
            Hight_Light( DC );
            SendMessageToEESCHEMA( item );
            return g_HightLigth_NetCode;

        case TYPE_TRACK:
        case TYPE_VIA:
        case TYPE_ZONE:

            // since these classes are all derived from TRACK, use a common
            // GetNet() function:
            g_HightLigth_NetCode = ( (TRACK*) item )->GetNet();
            Hight_Light( DC );
            return g_HightLigth_NetCode;

        case TYPE_ZONE_CONTAINER:
            g_HightLigth_NetCode = ( (ZONE_CONTAINER*) item )->GetNet();
            Hight_Light( DC );
            return g_HightLigth_NetCode;

        default:
            ;   // until somebody changes GENERAL_COLLECTOR::PadsOrTracks,
                // this should not happen.
        }
    }

    return -1;      // HitTest() failed.
}


/*
 * Highlight command.
 *
 * Show or removes the net at the current cursor position.
 */
void WinEDA_PcbFrame::Hight_Light( wxDC* DC )
{
    g_HightLigt_Status = !g_HightLigt_Status;

    GetBoard()->DrawHighLight( DrawPanel, DC, g_HightLigth_NetCode );
}
