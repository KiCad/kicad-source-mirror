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

#include "kicad_device_context.h"


#define Pad_fill (Pad_Fill_Item.State == RUN)


/**
 * Function ListNetsAndSelect
 * called by a command event
 * displays the sorted list of nets in a dialog frame
 * If a net is selected, it is highlighted
 */
void PCB_EDIT_FRAME::ListNetsAndSelect( wxCommandEvent& event )
{
    NETINFO_ITEM* net;
    wxString      netFilter;
    int           selection;

    netFilter = wxT( "*" );
    wxTextEntryDialog dlg( this, _( "Filter for net names:" ), _( "Net Filter" ), netFilter );
    if( dlg.ShowModal() != wxID_OK )
        return; // cancelled by user

    netFilter = dlg.GetValue( );
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
        INSTALL_UNBUFFERED_DC( dc, DrawPanel );

        if( g_HighLight_Status )
            High_Light( &dc );

        g_HighLight_NetCode = netcode;
        High_Light( &dc );
    }
}


/* Locate track or pad and highlight the corresponding net
 * Returns the Netcode, or -1 if no net located.
 */
int PCB_EDIT_FRAME::Select_High_Light( wxDC* DC )
{
    if( g_HighLight_Status )
        High_Light( DC );

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
            g_HighLight_NetCode = ( (D_PAD*) item )->GetNet();
            High_Light( DC );
            SendMessageToEESCHEMA( item );
            return g_HighLight_NetCode;

        case TYPE_TRACK:
        case TYPE_VIA:
        case TYPE_ZONE:

            // since these classes are all derived from TRACK, use a common
            // GetNet() function:
            g_HighLight_NetCode = ( (TRACK*) item )->GetNet();
            High_Light( DC );
            return g_HighLight_NetCode;

        case TYPE_ZONE_CONTAINER:
            g_HighLight_NetCode = ( (ZONE_CONTAINER*) item )->GetNet();
            High_Light( DC );
            return g_HighLight_NetCode;

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
void PCB_EDIT_FRAME::High_Light( wxDC* DC )
{
    g_HighLight_Status = !g_HighLight_Status;

    GetBoard()->DrawHighLight( DrawPanel, DC, g_HighLight_NetCode );
}
