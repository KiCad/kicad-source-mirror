/**
 * @file surbrill.cpp
 * @brief Highlight nets.
 */

#include "fctsys.h"
#include "class_drawpanel.h"
#include "kicad_string.h"
#include "wxPcbStruct.h"
#include "kicad_device_context.h"
#include "macros.h"

#include "class_board.h"
#include "class_track.h"
#include "class_zone.h"

#include "pcbnew.h"
#include "collectors.h"


#define Pad_fill ( Pad_Fill_Item.State == RUN )


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
    wxArrayString list;

    netFilter = wxT( "*" );
    wxTextEntryDialog dlg( this, _( "Filter Net Names" ), _( "Net Filter" ), netFilter );

    if( dlg.ShowModal() != wxID_OK )
        return; // cancelled by user

    netFilter = dlg.GetValue( );

    if( netFilter.IsEmpty() )
        return;

    wxString Line;
    for( unsigned ii = 0; ii < GetBoard()->m_NetInfo->GetCount(); ii++ )
    {
        net = GetBoard()->m_NetInfo->GetNetItem( ii );

        if( !WildCompareString( netFilter, net->GetNetname(), false ) )
            continue;

        Line.Printf( wxT( "net %3.3d:  %s" ), net->GetNet(),
                     GetChars( net->GetNetname() ) );
        list.Add( Line );
    }

    wxSingleChoiceDialog choiceDlg( this, wxEmptyString, _( "Select Net" ), list, NULL );

    if( (choiceDlg.ShowModal() == wxID_CANCEL) || (choiceDlg.GetSelection() == wxNOT_FOUND) )
        return;

    bool     found   = false;
    unsigned netcode = (unsigned) choiceDlg.GetSelection();

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

        if( GetBoard()->IsHighLightNetON() )
            High_Light( &dc );

        GetBoard()->SetHighLightNet( netcode );
        High_Light( &dc );
    }
}


/* Locate track or pad and highlight the corresponding net
 * Returns the Netcode, or -1 if no net located.
 */
int PCB_EDIT_FRAME::Select_High_Light( wxDC* DC )
{
    int netcode = -1;

    if( GetBoard()->IsHighLightNetON() )
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
            netcode = ( (D_PAD*) item )->GetNet();
            SendMessageToEESCHEMA( item );
            break;

        case TYPE_TRACK:
        case TYPE_VIA:
        case TYPE_ZONE:
            // since these classes are all derived from TRACK, use a common
            // GetNet() function:
            netcode = ( (TRACK*) item )->GetNet();
            break;

        case TYPE_ZONE_CONTAINER:
            netcode = ( (ZONE_CONTAINER*) item )->GetNet();
            break;

        default:
            ;   // until somebody changes GENERAL_COLLECTOR::PadsOrTracks,
                // this should not happen.
        }
    }

    if( netcode >= 0 )
    {
        GetBoard()->SetHighLightNet( netcode );
        High_Light( DC );
    }


    return netcode;      // HitTest() failed.
}


/*
 * Highlight command.
 *
 * Show or removes the net at the current cursor position.
 */
void PCB_EDIT_FRAME::High_Light( wxDC* DC )
{
    if( GetBoard()->IsHighLightNetON() )
        GetBoard()->HighLightOFF();
    else
        GetBoard()->HighLightON();

    GetBoard()->DrawHighLight( DrawPanel, DC, GetBoard()->GetHighLightNetCode() );
}
