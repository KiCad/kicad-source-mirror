/***************************************************************************************/
/* Rastnest calculations: Function to handle existing tracks in rastsnest calculations */
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


/**************************************************************************************************/
static int Merge_Two_SubNets( TRACK* pt_start_conn, TRACK* pt_end_conn, int old_val, int new_val )
/**************************************************************************************************/

/**
 * Function Merge_Two_SubNets
 * Used by Propagate_SubNet()
 * Change a subnet value to a new value, for tracks ans pads which are connected to corresponding track
 * for pads and tracks, this is the .m_Subnet member that is tested and modified
 * these members are block numbers (or cluster numbers) for a given net
 * The result is merging 2 blocks (or subnets)
 * @return modification count
 * @param old_val = subnet value to modify
 * @param new_val = new subnet value for each item whith have old_val as subnet value
 * @param pt_start_conn = first track segment to test
 * @param pt_end_conn = last track segment to test
 * If pt_end_conn = NULL: search is made from pt_start_conn to end of linked list
 */
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


/******************************************************************/
static void Propagate_SubNet( TRACK* pt_start_conn, TRACK* pt_end_conn )
/******************************************************************/

/**
 * Function Propagate_SubNet
 * Test a list of track segment, to create or propagate a sub netcode to pads and segments connected together
 * the track list must be sorted by nets, and all segments from pt_start_conn to pt_end_conn have the same net
 * When 2 items are connected (a track to a pad, or a track to an other track) they are grouped in a cluster.
 * for pads, this is the .m_physical_connexion member which is a cluster identifier
 * for tracks, this is the .m_Subnet member which is a cluster identifier
 * For a given net, if all tracks are created, there is only one cluster.
 * but if not all tracks are created, there are more than one cluster, and some ratsnets will be shown.
 * @param pt_start_conn = first track to test
 * @param pt_end_conn = last segment to test
 */
{
    TRACK*      pt_conn;
    int         sub_netcode;
    D_PAD*      pt_pad;
    TRACK*      pt_autre_piste;
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
            if( pt_conn->GetSubNet() )                  /* the track segment is already a cluster member */
            {
                if( pt_pad->GetSubNet() > 0 )           /* The pad is already a cluster member, so we can merge the 2 clusters */
                {
                    Merge_Two_SubNets( pt_start_conn, pt_end_conn,
                                      pt_pad->GetSubNet(), pt_conn->GetSubNet() );
                }
                else  /* The pad is not yet attached to a cluster , so we can add this pad to the cluster */
                    pt_pad->SetSubNet( pt_conn->GetSubNet() );
            }
            else                                        /* the track segment is not attached to a cluster */
            {
                if( pt_pad->GetSubNet() > 0 )           /* it is connected to a pad in a cluster, merge this track */
                {
                    pt_conn->SetSubNet( pt_pad->GetSubNet() );
                }
                else    /* it is connected to a pad not in a cluster, so we must create a new cluster (only with the 2 items: the track and the pad) */
                {
                    sub_netcode++;
                    pt_conn->SetSubNet( sub_netcode );
                    pt_pad->SetSubNet( pt_conn->GetSubNet() );
                }
            }
        }

        PtStruct = pt_conn->end;
        if( PtStruct && (PtStruct->Type() == TYPE_PAD) )
        /* The segment end on a pad */
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
                    pt_pad->SetSubNet( pt_conn->GetSubNet() );
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
            pt_autre_piste = (TRACK*) PtStruct;

            if( pt_conn->GetSubNet() )              /* the track segment is already a cluster member */
            {
                if( pt_autre_piste->GetSubNet() )   /* The other track is already a cluster member, so we can merge the 2 clusters */
                {
                    Merge_Two_SubNets( pt_start_conn, pt_end_conn,
                                      pt_autre_piste->GetSubNet(), pt_conn->GetSubNet() );
                }
                else /* The other track is not yet attached to a cluster , so we can add this other track to the cluster */
                {
                    pt_autre_piste->SetSubNet( pt_conn->GetSubNet() );
                }
            }
            else                                    /* the track segment is not yet attached to a cluster */
            {
                if( pt_autre_piste->GetSubNet() )   /* The other track is already a cluster member, so we can add the segment to the cluster */
                {
                    pt_conn->SetSubNet( pt_autre_piste->GetSubNet() );
                }
                else    /* it is connected to an other segment not in a cluster, so we must create a new cluster (only with the 2 track segments) */
                {
                    sub_netcode++;
                    pt_conn->SetSubNet( sub_netcode );
                    pt_autre_piste->SetSubNet( pt_conn->GetSubNet() );
                }
            }
        }

        PtStruct = pt_conn->end;    // Do the same calculations for the segment end point
        if( PtStruct && (PtStruct->Type() != TYPE_PAD) )
        {
            pt_autre_piste = (TRACK*) PtStruct;

            if( pt_conn->GetSubNet() )  /* the track segment is already a cluster member */
            {
                if( pt_autre_piste->GetSubNet() )
                {
                    Merge_Two_SubNets( pt_start_conn, pt_end_conn,
                                      pt_autre_piste->GetSubNet(), pt_conn->GetSubNet() );
                }
                else
                    pt_autre_piste->SetSubNet( pt_conn->GetSubNet() );
            }
            else      /* the track segment is not yet attached to a cluster */
            {
                if( pt_autre_piste->GetSubNet() )
                {
                    pt_conn->SetSubNet( pt_autre_piste->GetSubNet() );
                }
                else
                {
                    sub_netcode++;
                    pt_conn->SetSubNet( sub_netcode );
                    pt_autre_piste->SetSubNet( pt_conn->GetSubNet() );
                }
            }
        }
        if( pt_conn == pt_end_conn )
            break;
    }
}


/***************************************************/
void WinEDA_BasePcbFrame::test_connexions( wxDC* DC )
/***************************************************/

/**
 * Function testing the connections relative to all nets
 *  This function update the status of the ratsnest ( flag CH_ACTIF = 0 if a connection is found, = 1 else)
 * track segments are assumed to be sorted by net codes.
 * This is the case because when a new track is added, it is inserted in the linked list according to its net code.
 * and when nets are changed (when a new netlist is read) tracks are sorted before using this function
 * @param DC = current Device Context
 */
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
    for( TRACK* track = m_Pcb->m_Track;  track;  )
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


/*************************************************************************/
void WinEDA_BasePcbFrame::test_1_net_connexion( wxDC* DC, int net_code )
/*************************************************************************/

/**
 * Function testing the connections relative to a given net
 * track segments are assumed to be sorted by net codes
 * @param DC = current Device Context
 * @param net_code = net code to test
 */
{
    wxString msg;

    if( net_code == 0 )
        return;

    if( (m_Pcb->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK) == 0 )
        Compile_Ratsnest( DC, TRUE );

    for( unsigned i = 0;  i<m_Pcb->GetPadsCount();  ++i )
    {
        D_PAD* pad = m_Pcb->m_NetInfo->GetPad(i);

        int    pad_net_code = pad->GetNet();

        if( pad_net_code < net_code )
            continue;

        if( pad_net_code > net_code )
            break;

        pad->SetSubNet( 0 );
    }

    m_Pcb->Test_Connections_To_Copper_Areas( net_code );

    /* Search for the first and the last segment relative to the given net code */
    if( m_Pcb->m_Track )
    {
        TRACK* pt_start_conn;
        TRACK* pt_end_conn = NULL;
        pt_start_conn = m_Pcb->m_Track.GetFirst()->GetStartNetCode( net_code );

        if( pt_start_conn )
            pt_end_conn = pt_start_conn->GetEndNetCode( net_code );

        if( pt_start_conn && pt_end_conn ) // c.a.d. s'il y a des segments
        {
            Build_Pads_Info_Connections_By_Tracks( pt_start_conn, pt_end_conn );
        }
    }
    Merge_SubNets_Connected_By_CopperAreas( m_Pcb, net_code );

    /* Test the rastnest for this net */
    int nb_net_noconnect = Test_1_Net_Ratsnest( DC, net_code );

    /* Display results */
    msg.Printf( wxT( "links %d nc %d  net:nc %d" ),
                m_Pcb->GetRatsnestsCount(), m_Pcb->GetNoconnectCount(),
                nb_net_noconnect );

    SetStatusText( msg );
    return;
}


/*******************************************************************************************/
static void Build_Pads_Info_Connections_By_Tracks( TRACK* pt_start_conn, TRACK* pt_end_conn )
/*******************************************************************************************/

/**  Used after a track change (delete a track ou add a track)
 * Compute connections (initialize the .start and .end members) for a single net.
 * tracks must be sorted by net, as usual
 *  @param pt_start_conn = first segment of the net
 *  @param pt_end_conn = last segment of the net
 *  Connections to pads are assumed to be already initialized.
 *  If a track is deleted, the other pointers to pads do not change.
 *  When a track is added, its pointers to pads are already initialized
 */
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
        if( Track->Type() == TYPE_VIA )  // A via can connect many tracks, we must search for all track segments in this net
        {
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
            Track->start = Locate_Piste_Connectee( Track, Track, pt_end_conn, START );
        }

        if( Track->end == NULL )    // end track not already connected, search a connection
        {
            Track->end = Locate_Piste_Connectee( Track, Track, pt_end_conn, END );
        }

        if( Track == pt_end_conn )
            break;
    }

    /* Creates sub nets (cluster) for the current net: */
    Propagate_SubNet( pt_start_conn, pt_end_conn );
}


#define POS_AFF_CHREF 62

/**
 * Function SuperFast_Locate_Pad_Connecte
 * locates the pad connected to a track ended at coord px, py.
 * A track is seen as connected if the px, py position is same as the pad position.
 *
 * @param aPcb = the board.
 * @param pt_liste = Pointers to pads buffer
 *      This buffer is a list like the list created by build_liste_pad, but sorted by increasing X pad coordinate
 * @param posref = reference coordinate
 * @param masque_layer = Layers (bit to bit) to consider
 * @return : pointer on the connected pad
 *  This function uses a fast search in this sorted pad list and it is faster than Fast_Locate_Pad_connecte(),
 *  But this sorted pad list must be built before calling this function.
 *
 *  (Note: The usual pad list (created by build_liste_pad) m_Pcb->m_Pads is sorted by increasing netcodes )
 */
static D_PAD* SuperFast_Locate_Pad_Connecte( BOARD* aPcb, LISTE_PAD* pt_liste,
                                             const wxPoint& posref, int masque_layer )
{
    D_PAD*     pad;
    int        ii;

    int        nb_pad  = aPcb->GetPadsCount();
    LISTE_PAD* ptr_pad = pt_liste;
    LISTE_PAD* lim = pt_liste + nb_pad - 1;

    ptr_pad = pt_liste;
    while( nb_pad )
    {
        pad      = *ptr_pad;
        ii       = nb_pad;
        nb_pad >>= 1;

        if( (ii & 1) && ( ii > 1 ) )
            nb_pad++;

        if( pad->m_Pos.x < posref.x ) /* Must search after this item */
        {
            ptr_pad += nb_pad;
            if( ptr_pad > lim )
                ptr_pad = lim;
            continue;
        }
        if( pad->m_Pos.x > posref.x ) /* Must search before this item */
        {
            ptr_pad -= nb_pad;
            if( ptr_pad < pt_liste )
                ptr_pad = pt_liste;
            continue;
        }

        if( pad->m_Pos.x == posref.x )  /* A suitable block is found (X coordinate matches the px reference: but wue must matches the Y coordinate */
        {
            /* Search the beginning of the block */
            while( ptr_pad >= pt_liste )
            {
                pad = *ptr_pad;
                if( pad->m_Pos.x == posref.x )
                    ptr_pad--;
                else
                    break;
            }

            ptr_pad++;  /* ptr_pad = first pad which have pad->m_Pos.x = px */

            for( ; ; ptr_pad++ )
            {
                if( ptr_pad > lim )
                    return NULL; /* outside suitable block */

                pad = *ptr_pad;
                if( pad->m_Pos.x != posref.x )
                    return NULL; /* outside suitable block */

                if( pad->m_Pos.y != posref.y )
                    continue;

                /* A Pad if found here: but it must mach the layer */
                if( pad->m_Masque_Layer & masque_layer )  // Matches layer => a connected pad is found !
                    return pad;
            }
        }
    }

    return NULL;
}


/**
 * Function SortPadsByXCoord
 * is used to Sort a pad list by x coordinate value.
 */
static int SortPadsByXCoord( const void* pt_ref, const void* pt_comp )
{
    D_PAD* ref  = *(LISTE_PAD*) pt_ref;
    D_PAD* comp = *(LISTE_PAD*) pt_comp;

    return ref->m_Pos.x - comp->m_Pos.x;
}


/*****************************************************************************/
void CreateSortedPadListByXCoord( BOARD* aBoard, std::vector<D_PAD*>* aVector )
/*****************************************************************************/
{
    aVector->insert( aVector->end(), aBoard->m_NetInfo->m_PadsFullList.begin(), aBoard->m_NetInfo->m_PadsFullList.end() );

    qsort( &(*aVector)[0], aBoard->GetPadsCount(), sizeof( D_PAD*), SortPadsByXCoord );
}


/********************************************************************/
void WinEDA_BasePcbFrame::RecalculateAllTracksNetcode( )
/********************************************************************/

/* search connections between tracks and pads, and propagate pad net codes to the track segments
 * This is a 2 pass computation.
 * First:
 * We search a connection between a track segment and a pad: if found : this segment  netcode is set to the pad netcode
 */
{
    TRACK*              pt_piste;
    TRACK*              pt_next;
    int                 a_color;
    char                new_passe_request = 1, flag;

    std::vector<D_PAD*> sortedPads;
    BOARD_ITEM*         PtStruct;
    int                 masque_layer;
    wxString            msg;

    // Build the net info list
    GetBoard()->m_NetInfo->BuildListOfNets();

    if( m_Pcb->GetPadsCount() == 0 )     // If no pad, reset pointers and netcode, and do nothing else
    {
        pt_piste = m_Pcb->m_Track;
        for( ; pt_piste != NULL; pt_piste = pt_piste->Next() )
        {
            pt_piste->start = NULL;
            pt_piste->SetState( BEGIN_ONPAD | END_ONPAD, OFF );
            pt_piste->SetNet( 0 );
            pt_piste->end = NULL;
        }
        return;
    }

    /**************************************************************/
    /* Pass 1: search the connections between track ends and pads */
    /**************************************************************/
    CreateSortedPadListByXCoord( m_Pcb, &sortedPads );

    /* Reset variables and flags used in computation */
    pt_piste = m_Pcb->m_Track;
    for( ; pt_piste != NULL; pt_piste = pt_piste->Next() )
    {
        pt_piste->SetState( BUSY | EDIT | BEGIN_ONPAD | END_ONPAD, OFF );
        pt_piste->SetZoneSubNet( 0 );
        pt_piste->SetNet( 0 );  // net code = 0 means not connected
    }

    /* First pass: search connection between a track segment and a pad.
     * if found, set the track net code to the pad netcode
     */
    pt_piste = m_Pcb->m_Track;
    for( ; pt_piste != NULL; pt_piste = pt_piste->Next() )
    {
        flag = 0;
        masque_layer = g_TabOneLayerMask[pt_piste->GetLayer()];

        /* Search for a pad on the segment starting point */
        pt_piste->start = SuperFast_Locate_Pad_Connecte( m_Pcb,
                                                         &sortedPads[0],
                                                         pt_piste->m_Start,
                                                         masque_layer );
        if( pt_piste->start != NULL )
        {
            pt_piste->SetState( BEGIN_ONPAD, ON );
            pt_piste->SetNet( ( (D_PAD*) (pt_piste->start) )->GetNet() );
        }

        /* Search for a pad on the segment ending point */
        pt_piste->end = SuperFast_Locate_Pad_Connecte( m_Pcb,
                                                       &sortedPads[0],
                                                       pt_piste->m_End,
                                                       masque_layer );

        if( pt_piste->end != NULL )
        {
            pt_piste->SetState( END_ONPAD, ON );
            pt_piste->SetNet( ( (D_PAD*) (pt_piste->end) )->GetNet() );
        }
    }


    /*****************************************************/
    /* Pass 2: search the connections between track ends */
    /*****************************************************/

    /*  the .start et .end member pointers are updated, only if NULLs
     * (if not nuls, the end is already connected to a pad).
     * the connection (if found) is between segments
     * when a track has a net code and the other has a null net code, the null net code is changed
     */

    for( pt_piste = m_Pcb->m_Track; pt_piste != NULL; pt_piste = pt_piste->Next() )
    {
        if( pt_piste->start == NULL )
        {
            pt_piste->start = Locate_Piste_Connectee( pt_piste, m_Pcb->m_Track, NULL, START );
        }

        if( pt_piste->end == NULL )
        {
            pt_piste->end = Locate_Piste_Connectee( pt_piste, m_Pcb->m_Track, NULL, END );
        }
    }

    /**********************************************************/
    /* Propagate net codes from a segment to an other segment */
    /**********************************************************/

    a_color = YELLOW;

    while( new_passe_request )
    {
        bool reset_flag = FALSE;
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
            while( ( pt_next = Locate_Piste_Connectee( via, pt_next, NULL, START ) ) != NULL )
            {
                if( pt_next->GetNet() )
                {
                    via->SetNet( pt_next->GetNet() );
                    break;
                }
                pt_next->SetState( BUSY, ON );
                reset_flag = TRUE;
            }
        }

        if( reset_flag )
            for( pt_piste = m_Pcb->m_Track; pt_piste != NULL; pt_piste = pt_piste->Next() )
            {
                pt_piste->SetState( BUSY, OFF );
            }

        /* set the netcode of connected tracks: if at track is connected to a pad, its net code is already set.
         * if the current track is connected to an other track:
         * if a track has a net code, it is used for the other track.
         * Thus there is a propagation of the netcode from a track to an other.
         * if none of the 2 track has a net code we do nothing
         * the iteration is stopped when no new change occurs
         */
        for( pt_piste = m_Pcb->m_Track; pt_piste != NULL; pt_piste = pt_piste->Next() )
        {
            /* look for the connection to the current segment starting point */
            PtStruct = (BOARD_ITEM*) pt_piste->start;
            if( PtStruct && (PtStruct->Type() != TYPE_PAD) )
            {
                // Begin on an other track segment
                pt_next = (TRACK*) PtStruct;
                if( pt_piste->GetNet() )
                {
                    if( pt_next->GetNet() == 0 )    // the current track has a netcode, we use it for the other track
                    {
                        new_passe_request = 1;      // A change is made: a new iteration is requested.
                        pt_next->SetNet( pt_piste->GetNet() );
                    }
                }
                else
                {
                    if( pt_next->GetNet() != 0 )    // the other track has a netcode, we use it for the current track
                    {
                        pt_piste->SetNet( pt_next->GetNet() );
                        new_passe_request = 1;
                    }
                }
            }

            /* look for the connection to the current segment ending point */
            PtStruct = pt_piste->end;
            if( PtStruct &&(PtStruct->Type() != TYPE_PAD) )
            {
                pt_next = (TRACK*) PtStruct;

                // End on an other track: propagate netcode if possible
                if( pt_piste->GetNet() )
                {
                    if( pt_next->GetNet() == 0 )
                    {
                        new_passe_request = 1;
                        pt_next->SetNet( pt_piste->GetNet() );
                    }
                }
                else
                {
                    if( pt_next->GetNet() != 0 )
                    {
                        pt_piste->SetNet( pt_next->GetNet() );
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
