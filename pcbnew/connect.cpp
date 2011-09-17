/***************************************************************************************/
/* Ratsnest calculations: Function to handle existing tracks in ratsnest calculations */
/***************************************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"

//#include <algorithm>

extern void Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb );
extern void Merge_SubNets_Connected_By_CopperAreas( BOARD* aPcb, int aNetcode );

/* Local functions */
static void Propagate_SubNet( TRACK* pt_start_conn, TRACK* pt_end_conn );
static void Build_Pads_Info_Connections_By_Tracks( TRACK* pt_start_conn, TRACK* pt_end_conn );
static void RebuildTrackChain( BOARD* pcb );

/*..*/


/**
 * Function Merge_Two_SubNets
 * Used by Propagate_SubNet()
 * Change a subnet value to a new value, for tracks ans pads which are connected to
 * corresponding track for pads and tracks, this is the .m_Subnet member that is tested
 * and modified these members are block numbers (or cluster numbers) for a given net
 * The result is merging 2 blocks (or subnets)
 * @return modification count
 * @param old_val = subnet value to modify
 * @param new_val = new subnet value for each item which have old_val as subnet value
 * @param pt_start_conn = first track segment to test
 * @param pt_end_conn = last track segment to test
 * If pt_end_conn = NULL: search is made from pt_start_conn to end of linked list
 */
static int Merge_Two_SubNets( TRACK* pt_start_conn, TRACK* pt_end_conn, int old_val, int new_val )
{
    TRACK* pt_conn;
    int    nb_change = 0;
    D_PAD* pt_pad;

    if( old_val == new_val )
        return 0;

    if( (old_val > 0) && (old_val < new_val) )
        EXCHG( old_val, new_val );

    pt_conn = pt_start_conn;

    for( ; pt_conn != NULL; pt_conn = pt_conn->Next() )
    {
        if( pt_conn->GetSubNet() != old_val )
        {
            if( pt_conn == pt_end_conn )
                break;

            continue;
        }

        nb_change++;
        pt_conn->SetSubNet( new_val );

        if( pt_conn->start && ( pt_conn->start->Type() == TYPE_PAD) )
        {
            pt_pad = (D_PAD*) (pt_conn->start);

            if( pt_pad->GetSubNet() == old_val )
                pt_pad->SetSubNet( pt_conn->GetSubNet() );
        }

        if( pt_conn->end && (pt_conn->end->Type() == TYPE_PAD) )
        {
            pt_pad = (D_PAD*) (pt_conn->end);

            if( pt_pad->GetSubNet() == old_val )
                pt_pad->SetSubNet( pt_conn->GetSubNet() );
        }

        if( pt_conn == pt_end_conn )
            break;
    }

    return nb_change;
}



/**
 * Function Propagate_SubNet
 * Test a list of track segments, to create or propagate a sub netcode to pads and
 * segments connected together the track list must be sorted by nets, and all segments
 * from pt_start_conn to pt_end_conn have the same net when 2 items are connected (a
 * track to a pad, or a track to an other track) they are grouped in a cluster.
 * for pads, this is the .m_physical_connexion member which is a cluster identifier
 * for tracks, this is the .m_Subnet member which is a cluster identifier
 * For a given net, if all tracks are created, there is only one cluster.
 * but if not all tracks are created, there are more than one cluster, and some ratsnest
 * will be shown.
 * @param pt_start_conn = first track to test
 * @param pt_end_conn = last segment to test
 */
static void Propagate_SubNet( TRACK* pt_start_conn, TRACK* pt_end_conn )
{
    TRACK*      pt_conn;
    int         sub_netcode;
    D_PAD*      pt_pad;
    TRACK*      pt_other_trace;
    BOARD_ITEM* PtStruct;

    /* Clear variables used in computations */
    pt_conn = pt_start_conn;

    for( ; pt_conn != NULL; pt_conn = pt_conn->Next() )
    {
        pt_conn->SetSubNet( 0 );
        PtStruct = pt_conn->start;

        if( PtStruct && (PtStruct->Type() == TYPE_PAD) )
            ( (D_PAD*) PtStruct )->SetSubNet( 0 );

        PtStruct = pt_conn->end;

        if( PtStruct && (PtStruct->Type() == TYPE_PAD) )
            ( (D_PAD*) PtStruct )->SetSubNet( 0 );

        if( pt_conn == pt_end_conn )
            break;
    }

    sub_netcode = 1;
    pt_start_conn->SetSubNet( sub_netcode );

    /* Start of calculation */
    pt_conn = pt_start_conn;

    for( ; pt_conn != NULL; pt_conn = pt_conn->Next() )
    {
        /* First: handling connections to pads */
        PtStruct = pt_conn->start;

        /* The segment starts on a pad */
        if( PtStruct && (PtStruct->Type() == TYPE_PAD) )
        {
            pt_pad = (D_PAD*) PtStruct;

            if( pt_conn->GetSubNet() )        /* the track segment is already a cluster member */
            {
                if( pt_pad->GetSubNet() > 0 )
                {
                    /* The pad is already a cluster member, so we can merge the 2 clusters */
                    Merge_Two_SubNets( pt_start_conn, pt_end_conn,
                                       pt_pad->GetSubNet(), pt_conn->GetSubNet() );
                }
                else
                {
                    /* The pad is not yet attached to a cluster , so we can add this pad to
                     * the cluster */
                    pt_pad->SetSubNet( pt_conn->GetSubNet() );
                }
            }
            else                              /* the track segment is not attached to a cluster */
            {
                if( pt_pad->GetSubNet() > 0 )
                {
                    /* it is connected to a pad in a cluster, merge this track */
                    pt_conn->SetSubNet( pt_pad->GetSubNet() );
                }
                else
                {
                    /* it is connected to a pad not in a cluster, so we must create a new
                     * cluster (only with the 2 items: the track and the pad) */
                    sub_netcode++;
                    pt_conn->SetSubNet( sub_netcode );
                    pt_pad->SetSubNet( pt_conn->GetSubNet() );
                }
            }
        }

        PtStruct = pt_conn->end;

        /* The segment end on a pad */
        if( PtStruct && (PtStruct->Type() == TYPE_PAD) )
        {
            pt_pad = (D_PAD*) PtStruct;

            if( pt_conn->GetSubNet() )
            {
                if( pt_pad->GetSubNet() > 0 )
                {
                    Merge_Two_SubNets( pt_start_conn, pt_end_conn,
                                       pt_pad->GetSubNet(), pt_conn->GetSubNet() );
                }
                else
                {
                    pt_pad->SetSubNet( pt_conn->GetSubNet() );
                }
            }
            else
            {
                if( pt_pad->GetSubNet() > 0 )
                {
                    pt_conn->SetSubNet( pt_pad->GetSubNet() );
                }
                else
                {
                    sub_netcode++;
                    pt_conn->SetSubNet( sub_netcode );
                    pt_pad->SetSubNet( pt_conn->GetSubNet() );
                }
            }
        }


        /* Test connections between segments */
        PtStruct = pt_conn->start;

        if( PtStruct && (PtStruct->Type() != TYPE_PAD) )
        {
            /* The segment starts on an other track */
            pt_other_trace = (TRACK*) PtStruct;

            /* the track segment is already a cluster member */
            if( pt_conn->GetSubNet() )
            {
                /* The other track is already a cluster member, so we can merge the 2 clusters */
                if( pt_other_trace->GetSubNet() )
                {
                    Merge_Two_SubNets( pt_start_conn, pt_end_conn,
                                       pt_other_trace->GetSubNet(), pt_conn->GetSubNet() );
                }
                else
                {
                    /* The other track is not yet attached to a cluster , so we can add this
                     * other track to the cluster */
                    pt_other_trace->SetSubNet( pt_conn->GetSubNet() );
                }
            }
            else
            {
                /* the track segment is not yet attached to a cluster */
                if( pt_other_trace->GetSubNet() )
                {
                    /* The other track is already a cluster member, so we can add the segment
                     * to the cluster */
                    pt_conn->SetSubNet( pt_other_trace->GetSubNet() );
                }
                else
                {
                    /* it is connected to an other segment not in a cluster, so we must
                     * create a new cluster (only with the 2 track segments) */
                    sub_netcode++;
                    pt_conn->SetSubNet( sub_netcode );
                    pt_other_trace->SetSubNet( pt_conn->GetSubNet() );
                }
            }
        }

        PtStruct = pt_conn->end;    // Do the same calculations for the segment end point

        if( PtStruct && (PtStruct->Type() != TYPE_PAD) )
        {
            pt_other_trace = (TRACK*) PtStruct;

            if( pt_conn->GetSubNet() )  /* the track segment is already a cluster member */
            {
                if( pt_other_trace->GetSubNet() )
                {
                    Merge_Two_SubNets( pt_start_conn, pt_end_conn,
                                       pt_other_trace->GetSubNet(), pt_conn->GetSubNet() );
                }
                else
                {
                    pt_other_trace->SetSubNet( pt_conn->GetSubNet() );
                }
            }
            else
            {
                /* the track segment is not yet attached to a cluster */
                if( pt_other_trace->GetSubNet() )
                {
                    pt_conn->SetSubNet( pt_other_trace->GetSubNet() );
                }
                else
                {
                    sub_netcode++;
                    pt_conn->SetSubNet( sub_netcode );
                    pt_other_trace->SetSubNet( pt_conn->GetSubNet() );
                }
            }
        }

        if( pt_conn == pt_end_conn )
            break;
    }
}


void PCB_BASE_FRAME::TestConnections( wxDC* aDC )
{
    // Clear the cluster identifier for all pads
    for( unsigned i = 0;  i< m_Pcb->GetPadsCount();  ++i )
    {
        D_PAD* pad = m_Pcb->m_NetInfo->GetPad(i);

        pad->SetZoneSubNet( 0 );
        pad->SetSubNet( 0 );
    }

    m_Pcb->Test_Connections_To_Copper_Areas();

    // Test existing connections net by net
    for( TRACK* track = m_Pcb->m_Track;  track; )
    {
        // this is the current net because pt_start_conn is the first segment of the net
        int    current_net_code = track->GetNet();

        // this is the last segment of the current net
        TRACK* pt_end_conn = track->GetEndNetCode( current_net_code );

        Build_Pads_Info_Connections_By_Tracks( track, pt_end_conn );

        track = pt_end_conn->Next();    // this is now the first segment of the next net
    }

    Merge_SubNets_Connected_By_CopperAreas( m_Pcb );

    return;
}


void PCB_BASE_FRAME::TestNetConnection( wxDC* aDC, int aNetCode )
{
    wxString msg;

    if( aNetCode == 0 )
        return;

    if( (m_Pcb->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK) == 0 )
        Compile_Ratsnest( aDC, true );

    for( unsigned i = 0;  i<m_Pcb->GetPadsCount();  ++i )
    {
        D_PAD* pad = m_Pcb->m_NetInfo->GetPad(i);

        int    pad_net_code = pad->GetNet();

        if( pad_net_code < aNetCode )
            continue;

        if( pad_net_code > aNetCode )
            break;

        pad->SetSubNet( 0 );
    }

    m_Pcb->Test_Connections_To_Copper_Areas( aNetCode );

    /* Search for the first and the last segment relative to the given net code */
    if( m_Pcb->m_Track )
    {
        TRACK* pt_start_conn;
        TRACK* pt_end_conn = NULL;
        pt_start_conn = m_Pcb->m_Track.GetFirst()->GetStartNetCode( aNetCode );

        if( pt_start_conn )
            pt_end_conn = pt_start_conn->GetEndNetCode( aNetCode );

        if( pt_start_conn && pt_end_conn ) // c.a.d. if there are segments
        {
            Build_Pads_Info_Connections_By_Tracks( pt_start_conn, pt_end_conn );
        }
    }

    Merge_SubNets_Connected_By_CopperAreas( m_Pcb, aNetCode );

    /* Test the ratsnest for this net */
    int nb_net_noconnect = TestOneRatsNest( aDC, aNetCode );

    /* Display results */
    msg.Printf( wxT( "links %d nc %d  net:nc %d" ),
                m_Pcb->GetRatsnestsCount(), m_Pcb->GetNoconnectCount(),
                nb_net_noconnect );

    SetStatusText( msg );
    return;
}


/**  Used after a track change (delete a track ou add a track)
 * Compute connections (initialize the .start and .end members) for a single net.
 * tracks must be sorted by net, as usual
 *  @param pt_start_conn = first segment of the net
 *  @param pt_end_conn = last segment of the net
 *  Connections to pads are assumed to be already initialized.
 *  If a track is deleted, the other pointers to pads do not change.
 *  When a track is added, its pointers to pads are already initialized
 */
static void Build_Pads_Info_Connections_By_Tracks( TRACK* pt_start_conn, TRACK* pt_end_conn )
{
    TRACK* Track;

    /* Reset the old connections type track to track */
    for( Track = pt_start_conn; Track != NULL; Track = Track->Next() )
    {
        Track->SetSubNet( 0 );

        if( Track->GetState( BEGIN_ONPAD ) == 0 )
            Track->start = NULL;

        if( Track->GetState( END_ONPAD ) == 0 )
            Track->end = NULL;

        if( Track == pt_end_conn )
            break;
    }

    /* Update connections type track to track */
    for( Track = pt_start_conn; Track != NULL; Track = Track->Next() )
    {
        if( Track->Type() == TYPE_VIA )
        {
            // A via can connect many tracks, we must search for all track segments in this net
            TRACK* pt_segm;
            int    layermask = Track->ReturnMaskLayer();

            for( pt_segm = pt_start_conn; pt_segm != NULL; pt_segm = pt_segm->Next() )
            {
                int curlayermask = pt_segm->ReturnMaskLayer();

                if( !pt_segm->start && (pt_segm->m_Start == Track->m_Start)
                   && ( layermask & curlayermask ) )
                {
                    pt_segm->start = Track;
                }

                if( !pt_segm->end && (pt_segm->m_End == Track->m_Start)
                   && (layermask & curlayermask) )
                {
                    pt_segm->end = Track;
                }

                if( pt_segm == pt_end_conn )
                    break;
            }
        }

        if( Track->start == NULL )  // end track not already connected, search a connection
        {
            Track->start = Track->GetTrace( Track, pt_end_conn, START );
        }

        if( Track->end == NULL )    // end track not already connected, search a connection
        {
            Track->end = Track->GetTrace( Track, pt_end_conn, END );
        }

        if( Track == pt_end_conn )
            break;
    }

    /* Creates sub nets (cluster) for the current net: */
    Propagate_SubNet( pt_start_conn, pt_end_conn );
}


#define POS_AFF_CHREF 62


void PCB_BASE_FRAME::RecalculateAllTracksNetcode()
{
    TRACK*              pt_trace;
    TRACK*              pt_next;
    char                new_passe_request = 1;

    std::vector<D_PAD*> sortedPads;
    BOARD_ITEM*         PtStruct;
    int                 layerMask;
    wxString            msg;

    // Build the net info list
    GetBoard()->m_NetInfo->BuildListOfNets();

    if( m_Pcb->GetPadsCount() == 0 ) // If no pad, reset pointers and netcode, and do nothing else
    {
        pt_trace = m_Pcb->m_Track;

        for( ; pt_trace != NULL; pt_trace = pt_trace->Next() )
        {
            pt_trace->start = NULL;
            pt_trace->SetState( BEGIN_ONPAD | END_ONPAD, OFF );
            pt_trace->SetNet( 0 );
            pt_trace->end = NULL;
        }

        return;
    }

    /**************************************************************/
    /* Pass 1: search the connections between track ends and pads */
    /**************************************************************/
    m_Pcb->GetSortedPadListByXCoord( sortedPads );

    /* Reset variables and flags used in computation */
    pt_trace = m_Pcb->m_Track;

    for( ; pt_trace != NULL; pt_trace = pt_trace->Next() )
    {
        pt_trace->SetState( BUSY | IN_EDIT | BEGIN_ONPAD | END_ONPAD, OFF );
        pt_trace->SetZoneSubNet( 0 );
        pt_trace->SetNet( 0 );  // net code = 0 means not connected
    }

    /* First pass: search connection between a track segment and a pad.
     * if found, set the track net code to the pad netcode
     */
    pt_trace = m_Pcb->m_Track;

    for( ; pt_trace != NULL; pt_trace = pt_trace->Next() )
    {
        layerMask = g_TabOneLayerMask[pt_trace->GetLayer()];

        /* Search for a pad on the segment starting point */
        pt_trace->start = m_Pcb->GetPad( &sortedPads[0], pt_trace->m_Start, layerMask );

        if( pt_trace->start != NULL )
        {
            pt_trace->SetState( BEGIN_ONPAD, ON );
            pt_trace->SetNet( ( (D_PAD*) (pt_trace->start) )->GetNet() );
        }

        /* Search for a pad on the segment ending point */
        pt_trace->end = m_Pcb->GetPad( &sortedPads[0], pt_trace->m_End, layerMask );

        if( pt_trace->end != NULL )
        {
            pt_trace->SetState( END_ONPAD, ON );
            pt_trace->SetNet( ( (D_PAD*) (pt_trace->end) )->GetNet() );
        }
    }


    /*****************************************************/
    /* Pass 2: search the connections between track ends */
    /*****************************************************/

    /*  the .start and .end member pointers are updated, only if NULLs
     * (if not nuls, the end is already connected to a pad).
     * the connection (if found) is between segments
     * when a track has a net code and the other has a null net code, the null net code is changed
     */

    for( pt_trace = m_Pcb->m_Track; pt_trace != NULL; pt_trace = pt_trace->Next() )
    {
        if( pt_trace->start == NULL )
        {
            pt_trace->start = pt_trace->GetTrace( m_Pcb->m_Track, NULL, START );
        }

        if( pt_trace->end == NULL )
        {
            pt_trace->end = pt_trace->GetTrace( m_Pcb->m_Track, NULL, END );
        }
    }

    /**********************************************************/
    /* Propagate net codes from a segment to an other segment */
    /**********************************************************/

    while( new_passe_request )
    {
        bool reset_flag = false;
        new_passe_request = 0;

        /* look for vias which could be connect many tracks */
        for( TRACK* via = m_Pcb->m_Track; via != NULL; via = via->Next() )
        {
            if( via->Type() != TYPE_VIA )
                continue;

            if( via->GetNet() > 0 )
                continue; // Netcode already known

            // Lock for a connection to a track with a known netcode
            pt_next = m_Pcb->m_Track;

            while( ( pt_next = via->GetTrace( pt_next, NULL, START ) ) != NULL )
            {
                if( pt_next->GetNet() )
                {
                    via->SetNet( pt_next->GetNet() );
                    break;
                }

                pt_next->SetState( BUSY, ON );
                reset_flag = true;
            }
        }

        if( reset_flag )
        {
            for( pt_trace = m_Pcb->m_Track; pt_trace != NULL; pt_trace = pt_trace->Next() )
            {
                pt_trace->SetState( BUSY, OFF );
            }
        }

        /* set the netcode of connected tracks: if at track is connected to a pad, its net
         * code is already set.
         * if the current track is connected to an other track:
         * if a track has a net code, it is used for the other track.
         * Thus there is a propagation of the netcode from a track to an other.
         * if none of the 2 track has a net code we do nothing
         * the iteration is stopped when no new change occurs
         */
        for( pt_trace = m_Pcb->m_Track; pt_trace != NULL; pt_trace = pt_trace->Next() )
        {
            /* look for the connection to the current segment starting point */
            PtStruct = (BOARD_ITEM*) pt_trace->start;

            if( PtStruct && (PtStruct->Type() != TYPE_PAD) )
            {
                // Begin on an other track segment
                pt_next = (TRACK*) PtStruct;

                if( pt_trace->GetNet() )
                {
                    if( pt_next->GetNet() == 0 )
                    {
                        // the current track has a netcode, we use it for the other track
                        // A change is made: a new iteration is requested.
                        new_passe_request = 1;
                        pt_next->SetNet( pt_trace->GetNet() );
                    }
                }
                else
                {
                    if( pt_next->GetNet() != 0 )
                    {
                        // the other track has a netcode, we use it for the current track
                        pt_trace->SetNet( pt_next->GetNet() );
                        new_passe_request = 1;
                    }
                }
            }

            /* look for the connection to the current segment ending point */
            PtStruct = pt_trace->end;

            if( PtStruct && (PtStruct->Type() != TYPE_PAD) )
            {
                pt_next = (TRACK*) PtStruct;

                // End on an other track: propagate netcode if possible
                if( pt_trace->GetNet() )
                {
                    if( pt_next->GetNet() == 0 )
                    {
                        new_passe_request = 1;
                        pt_next->SetNet( pt_trace->GetNet() );
                    }
                }
                else
                {
                    if( pt_next->GetNet() != 0 )
                    {
                        pt_trace->SetNet( pt_next->GetNet() );
                        new_passe_request = 1;
                    }
                }
            }
        }
    }

    /* Sort the track list by net codes: */
    RebuildTrackChain( m_Pcb );
}


/**
 * Function Sort_By_NetCode
 * sorts track segments used in RebuildTrackChain() (for the qsort C function)
 * The sorting is made by net code.
 */
static int Sort_By_NetCode( const void* left, const void* right )
{
    TRACK* pt_ref     = *(TRACK**) left;
    TRACK* pt_compare = *(TRACK**) right;

    int    ret = pt_ref->GetNet() - pt_compare->GetNet();

    return ret;
}


/**
 * Function RebuildTrackChain
 * rebuilds the track segment linked list in order to have a chain
 * sorted by increasing netcodes.
 * @param pcb = board to rebuild
 */
static void RebuildTrackChain( BOARD* pcb )
{
    if( pcb->m_Track == NULL )
        return;

    int     nbsegm = pcb->m_Track.GetCount();

    TRACK** array = (TRACK**) MyZMalloc( nbsegm * sizeof(TRACK*) );

    for( int i = 0;  i<nbsegm;  ++i )
    {
        array[i] = pcb->m_Track.PopFront();
        wxASSERT( array[i] );
    }

    // the list is empty now
    wxASSERT( pcb->m_Track == NULL && pcb->m_Track.GetCount()==0 );

    qsort( array, nbsegm, sizeof(TRACK*), Sort_By_NetCode );

    // add them back to the list
    for( int i = 0;  i<nbsegm;  ++i )
    {
        pcb->m_Track.PushBack( array[i] );
    }

    MyFree( array );
}
