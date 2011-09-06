/**
 * @file clean.cpp
 * functions to clean tracks: remove null and redundant segments
 */


#include "fctsys.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "protos.h"

/* local functions : */
static void      clean_segments( PCB_EDIT_FRAME* frame );
static void      clean_vias( BOARD* aPcb );
static void     DeleteUnconnectedTracks( PCB_EDIT_FRAME* frame, wxDC* DC );
static TRACK*   AlignSegment( BOARD* Pcb, TRACK* pt_ref, TRACK* pt_segm, int extremite );
static void Clean_Pcb_Items( PCB_EDIT_FRAME* frame, wxDC* DC,
                             bool aCleanVias, bool aMergeSegments,
                             bool aDeleteUnconnectedSegm, bool aConnectToPads );

#include "dialog_cleaning_options.h"

#define CONN2PAD_ENBL

#ifdef CONN2PAD_ENBL
static void ConnectDanglingEndToPad( PCB_EDIT_FRAME* frame, wxDC* DC );
static void ConnectDanglingEndToVia( BOARD* pcb );
//static void Gen_Raccord_Track( PCB_EDIT_FRAME* frame, wxDC* DC );
#endif


/*****************************************/
void PCB_EDIT_FRAME::Clean_Pcb( wxDC* DC )
/*****************************************/
/* Install the track operation dialog frame
*/
{
    DIALOG_CLEANING_OPTIONS::connectToPads = false;
    DIALOG_CLEANING_OPTIONS dlg( this );
    if( dlg.ShowModal() == wxID_OK )
        Clean_Pcb_Items( this, DC, dlg.cleanVias, dlg.mergeSegments,
                        dlg.deleteUnconnectedSegm, dlg.connectToPads );
    DrawPanel->Refresh( true );
}


/* Main cleaning function.
 *  Delete
 * - Redundant points on tracks (merge aligned segments)
 * - vias on pad
 * - null segments
 * - Redundant segments
 *  Create segments when track ends are incorrecty connected:
 *  i.e. when a track end covers a pad or a via but is not exactly on the pad or the via center
 */
void Clean_Pcb_Items( PCB_EDIT_FRAME* frame, wxDC* DC,
                      bool aCleanVias, bool aMergeSegments,
                      bool aDeleteUnconnectedSegm, bool aConnectToPads )
{
    wxBusyCursor( dummy );

    frame->MsgPanel->EraseMsgBox();
    frame->GetBoard()->GetNumSegmTrack();    // update the count

    // Clear undo and redo lists to avoid inconsistencies between lists
    frame->GetScreen()->ClearUndoRedoList();

    /* Rebuild the pad infos (pad list and netcodes) to ensure an up to date info */
    frame->GetBoard()->m_Status_Pcb = 0;
    frame->GetBoard()->m_NetInfo->BuildListOfNets();

    if( aCleanVias )       // delete redundant vias
    {
        frame->SetStatusText( _( "Clean vias" ) );
        clean_vias( frame->GetBoard() );
    }

#ifdef CONN2PAD_ENBL
    /* Create missing segments when a track end covers a pad or a via,
    but is not on the pad  or the via center */
    if( aConnectToPads )
    {
        frame->SetStatusText( _( "Reconnect pads" ) );
        /* Create missing segments when a track end covers a pad, but is not on the pad center */
        ConnectDanglingEndToPad( frame, DC );

        // creation of points of connections at the intersection of tracks
//		Gen_Raccord_Track(frame, DC);

        /* Create missing segments when a track end covers a via, but is not on the via center */
        ConnectDanglingEndToVia( frame->GetBoard() );
    }
#endif

    /* Remove null segments and intermediate points on aligned segments */
    if( aMergeSegments )
    {
        frame->SetStatusText( _( "Merge track segments" ) );
        clean_segments( frame );
    }

    /* Delete dangling tracks */
    if( aDeleteUnconnectedSegm )
    {
        frame->SetStatusText( _( "Delete unconnected tracks" ) );
        DeleteUnconnectedTracks( frame, DC );
    }

    frame->SetStatusText( _( "Cleanup finished" ) );

    frame->Compile_Ratsnest( DC, true );

    frame->OnModify();
}

void clean_vias( BOARD * aPcb )
{
    TRACK* track;
    TRACK* next_track;

    for( track = aPcb->m_Track; track; track = track->Next() )
    {
        if( track->Shape() != VIA_THROUGH )
            continue;

        // Search and delete others vias at same location
        TRACK* alt_track = track->Next();
        for( ; alt_track != NULL; alt_track = next_track )
        {
            next_track = alt_track->Next();
            if( alt_track->m_Shape != VIA_THROUGH )
                continue;

            if( alt_track->m_Start != track->m_Start )
                continue;

            /* delete via */
            alt_track->UnLink();
            delete alt_track;
        }
    }

    /* Delete Via on pads at same location */
    for( track = aPcb->m_Track; track != NULL; track = next_track )
    {
        next_track = track->Next();
        if( track->m_Shape != VIA_THROUGH )
            continue;

        D_PAD* pad = Fast_Locate_Pad_Connecte( aPcb, track->m_Start, ALL_CU_LAYERS );
        if( pad && (pad->m_Masque_Layer & EXTERNAL_LAYERS) == EXTERNAL_LAYERS )    // redundant Via
        {
            /* delete via */
            track->UnLink();
            delete track;
        }
    }
}


/*****************************************************************************/
static void DeleteUnconnectedTracks( PCB_EDIT_FRAME* frame, wxDC* DC )
/*****************************************************************************/

/*
 *  Delete dangling tracks
 *  Vias:
 *  If a via is only connected to a dangling track, it also will be removed
 */
{
    TRACK*          segment;
    TRACK*          other;
    TRACK*          startNetcode;
    TRACK*          next;
    ZONE_CONTAINER* zone;
    int             masklayer, oldnetcode;
    int             type_end, flag_erase;

    if( frame->GetBoard()->m_Track == NULL )
        return;

    frame->DrawPanel->m_AbortRequest = FALSE;

    // correct via m_End defects
    for( segment = frame->GetBoard()->m_Track;  segment;  segment = next )
    {
        next = segment->Next();

        if( segment->Type() == TYPE_VIA )
        {
            if( segment->m_Start != segment->m_End )
                segment->m_End = segment->m_Start;
            continue;
        }
    }

    // removal of unconnected tracks
    segment = startNetcode = frame->GetBoard()->m_Track;
    oldnetcode = segment->GetNet();
    for( int ii = 0; segment ; segment = next, ii++ )
    {
        next = segment->Next();

        if( frame->DrawPanel->m_AbortRequest )
            break;

        if( segment->GetNet() != oldnetcode )
        {
            startNetcode = segment;
            oldnetcode = segment->GetNet();
        }

        flag_erase = 0; //Not connected indicator
        type_end = 0;

        /* Is a pad found on a track end ? */

        masklayer = segment->ReturnMaskLayer();

        D_PAD* pad;

        pad = Fast_Locate_Pad_Connecte( frame->GetBoard(), segment->m_Start, masklayer );
        if( pad != NULL )
        {
            segment->start = pad;
            type_end |= START_ON_PAD;
        }

        pad = Fast_Locate_Pad_Connecte( frame->GetBoard(), segment->m_End, masklayer );
        if( pad != NULL )
        {
            segment->end = pad;
            type_end |= END_ON_PAD;
        }

        // if not connected to a pad, test if segment's START is connected to another track
        // For via tests, an enhancement could to test if connected to 2 items on different layers.
        // Currently a via must be connected to 2 items, taht can be on the same layer
        int top_layer, bottom_layer;
        if( (type_end & START_ON_PAD ) == 0 )
        {
            other = Locate_Piste_Connectee( segment, frame->GetBoard()->m_Track, NULL, START );

            if( other == NULL )     // Test a connection to zones
            {
                if( segment->Type() != TYPE_VIA )
                {
                    zone = frame->GetBoard()->HitTestForAnyFilledArea(segment->m_Start, segment->GetLayer() );
                }

                else
                {
                    ((SEGVIA*)segment)->ReturnLayerPair( &top_layer, &bottom_layer );
                    zone = frame->GetBoard()->HitTestForAnyFilledArea(segment->m_Start, top_layer, bottom_layer );
                }
            }

            if( (other == NULL) && (zone == NULL) )
                flag_erase |= 1;

            else    // segment, via or zone connected to this end
            {
                segment->start = other;
                // If a via is connected to this end, test if this via has a second item connected
                // if no, remove it with the current segment
                if( other && other->Type() == TYPE_VIA )
                {
                    // search for another segment following the via

                    segment->SetState( BUSY, ON );

                    SEGVIA* via = (SEGVIA*) other;
                    other = Locate_Piste_Connectee( via, frame->GetBoard()->m_Track,
                                                       NULL, START );
                    if( other == NULL )
                    {
                        via->ReturnLayerPair( &top_layer, &bottom_layer );
                        zone = frame->GetBoard()->HitTestForAnyFilledArea(via->m_Start, bottom_layer, top_layer );
                    }

                    if( (other == NULL) && (zone == NULL) )
                        flag_erase |= 2;

                    segment->SetState( BUSY, OFF );
                }
            }
        }

        // if not connected to a pad, test if segment's END is connected to another track
        if( (type_end & END_ON_PAD ) == 0 )
        {
            other = Locate_Piste_Connectee( segment, frame->GetBoard()->m_Track,
                                               NULL, END );
            if( other == NULL )     // Test a connection to zones
            {
                if( segment->Type() != TYPE_VIA )
                    zone = frame->GetBoard()->HitTestForAnyFilledArea(segment->m_End, segment->GetLayer() );

                else
                {
                    ((SEGVIA*)segment)->ReturnLayerPair( &top_layer, &bottom_layer );
                    zone = frame->GetBoard()->HitTestForAnyFilledArea(segment->m_End,top_layer, bottom_layer  );
                }
            }

            if ( (other == NULL) && (zone == NULL) )
                flag_erase |= 0x10;

            else     // segment, via or zone connected to this end
            {
                segment->end = other;

                // If a via is connected to this end, test if this via has a second item connected
                // if no, remove it with the current segment
                if( other && other->Type() == TYPE_VIA )
                {
                    // search for another segment following the via

                    segment->SetState( BUSY, ON );

                    SEGVIA* via = (SEGVIA*) other;
                    other = Locate_Piste_Connectee( via, frame->GetBoard()->m_Track,
                                                       NULL, END );
                    if( other == NULL )
                    {
                        via->ReturnLayerPair( &top_layer, &bottom_layer );
                        zone = frame->GetBoard()->HitTestForAnyFilledArea(via->m_End, bottom_layer, top_layer );
                    }

                    if( (other == NULL) && (zone == NULL) )
                        flag_erase |= 0x20;

                    segment->SetState( BUSY, OFF );
                }
            }
        }

        if( flag_erase )
        {
            // update the pointer to start of the contiguous netcode group
            if( segment == startNetcode )
            {
                next = segment->Next();
                startNetcode = next;
            }
            else
                next = startNetcode;

            // remove segment from screen and board
            segment->Draw( frame->DrawPanel, DC, GR_XOR );
            segment->DeleteStructure();

            if( next == NULL )
                break;
        }
    }
}


/************************************************************/
static void clean_segments( PCB_EDIT_FRAME* frame )
/************************************************************/
/* Delete null lenght segments, and intermediate points .. */
{
    TRACK*          segment, * nextsegment;
    TRACK*          other;
    int             ii;
    int             flag, no_inc;
    wxString        msg;

    frame->DrawPanel->m_AbortRequest = FALSE;

    // Delete null segments
    for( segment = frame->GetBoard()->m_Track;  segment;  segment = nextsegment )
    {
        nextsegment = segment->Next();
        if( !segment->IsNull() )
            continue;

        /* Length segment = 0; delete it */
        segment->DeleteStructure();
    }

    /* Delete redundant segments */
    for( segment  = frame->GetBoard()->m_Track, ii = 0;  segment;  segment = segment->Next(), ii++ )
    {
        for( other = segment->Next(); other; other = nextsegment )
        {
            nextsegment = other->Next();
            int erase = 0;

            if( segment->Type() != other->Type() )
                continue;

            if( segment->GetLayer() != other->GetLayer() )
                continue;

            if( segment->GetNet() != other->GetNet() )
                break;

            if( segment->m_Start == other->m_Start )
            {
                if( segment->m_End == other->m_End )
                    erase = 1;
            }

            if( segment->m_Start == other->m_End )
            {
                if( segment->m_End == other->m_Start )
                    erase = 1;
            }

            /* Delete redundant point */
            if( erase )
            {
                ii--;
                other->DeleteStructure();
            }
        }
    }

    /* delete intermediate points  */
    ii = 0;
    for( segment = frame->GetBoard()->m_Track;  segment;  segment = nextsegment )
    {
        TRACK*  segStart;
        TRACK*  segEnd;
        TRACK*  segDelete;

        nextsegment = segment->Next();
        if( frame->DrawPanel->m_AbortRequest )
            return;

        if( segment->Type() != TYPE_TRACK )
            continue;

        flag = no_inc = 0;

        // search for a possible point that connects on the START point of the segment
        for( segStart = segment->Next(); ; )
        {
            segStart = Locate_Piste_Connectee( segment, segStart,
                                                NULL, START );
            if( segStart )
            {
                // the two segments must have the same width
                if( segment->m_Width != segStart->m_Width )
                    break;

                // it cannot be a via
                if( segStart->Type() != TYPE_TRACK )
                    break;

                /* We must have only one segment connected */
                segStart->SetState( BUSY, ON );
                other = Locate_Piste_Connectee( segment, frame->GetBoard()->m_Track,
                                                 NULL, START );
                segStart->SetState( BUSY, OFF );

                if( other == NULL )
                    flag = 1;           /* OK */

                break;
            }
            break;
        }

        if( flag )    /* We have the starting point of the segment is connecte to an other segment */
        {
            segDelete = AlignSegment( frame->GetBoard(), segment, segStart, START );
            if( segDelete )
            {
                no_inc = 1;
                segDelete->DeleteStructure();
            }
        }

        /* search for a possible point that connects on the END point of the segment: */
        for( segEnd = segment->Next(); ; )
        {
            segEnd = Locate_Piste_Connectee( segment, segEnd, NULL, END );
            if( segEnd )
            {
                if( segment->m_Width != segEnd->m_Width )
                    break;

                if( segEnd->Type() != TYPE_TRACK )
                    break;

                /* We must have only one segment connected */
                segEnd->SetState( BUSY, ON );
                other = Locate_Piste_Connectee( segment, frame->GetBoard()->m_Track,
                                                 NULL, END );
                segEnd->SetState( BUSY, OFF );

                if( other == NULL )
                    flag |= 2;          /* Ok */

                break;
            }
            else
                break;
        }

        if( flag & 2 )    /* We have the ending point of the segment is connecte to an other segment */
        {
            segDelete = AlignSegment( frame->GetBoard(), segment, segEnd, END );
            if( segDelete )
            {
                no_inc = 1;
                segDelete->DeleteStructure();
            }
        }

        if( no_inc ) /* The current segment was modified, retry to merge it */
            nextsegment = segment->Next();
    }

   return;
}


/****************************************************************************/
static TRACK* AlignSegment( BOARD* Pcb, TRACK* pt_ref, TRACK* pt_segm, int extremite )
/****************************************************************************/
/* Function used by clean_segments.
 *  Test alignement of pt_segm and pt_ref (which must have acommon end).
 *  and see if the common point is not on a pad (i.e. if this common point can be removed).
 *  the ending point of pt_ref is the start point (extremite == START)
 *  or the end point (extremite == FIN)
 *  if the common end can be deleted, this function
 *    change the common point coordinate of the pt_ref segm
 *   (and therefore connect the 2 other ending points)
 *    and return pt_segm (which can be deleted).
 *  else return NULL
 */
{
    int flag = 0;

    int refdx = pt_ref->m_End.x - pt_ref->m_Start.x;
    int refdy = pt_ref->m_End.y - pt_ref->m_Start.y;

    int segmdx = pt_segm->m_End.x - pt_segm->m_Start.x;
    int segmdy = pt_segm->m_End.y - pt_segm->m_Start.y;

    // test for vertical alignment (easy to handle)
    if( refdx == 0 )
    {
        if( segmdx != 0 )
            return NULL;
        else
            flag = 1;
    }

    // test for horizontal alignment (easy to handle)
    if( refdy == 0 )
    {
        if( segmdy != 0 )
            return NULL;
        else
            flag = 2;
    }

    /* tst if alignement in other cases
     *  We must have refdy/refdx == (+/-)segmdy/segmdx, (i.e. same orientation) */
    if( flag == 0 )
    {
        if( (refdy * segmdx !=  refdx * segmdy)
         && (refdy * segmdx != -refdx * segmdy) )
            return NULL;
        flag = 4;
    }

    /* Here we have 2 aligned segments:
    We must change the pt_ref common point only if not on a pad
    (this function) is called when thre is only 2 connected segments,
    and if this point is not on a pad, it can be removed and the 2 segments will be merged
    */
    if( extremite == START )
    {
        /* We do not have a pad */
        if( Fast_Locate_Pad_Connecte( Pcb, pt_ref->m_Start,
                                      g_TabOneLayerMask[pt_ref->GetLayer()] ) )
            return NULL;

        /* change the common point coordinate of pt_segm tu use the other point
        of pt_segm (pt_segm will be removed later) */
        if( pt_ref->m_Start == pt_segm->m_Start )
        {
            pt_ref->m_Start = pt_segm->m_End;
            return pt_segm;
        }
        else
        {
            pt_ref->m_Start = pt_segm->m_Start;
            return pt_segm;
        }
    }
    else    /* extremite == END */
    {
        /* We do not have a pad */
        if( Fast_Locate_Pad_Connecte( Pcb, pt_ref->m_End,
                                     g_TabOneLayerMask[pt_ref->GetLayer()] ) )
            return NULL;

        /* change the common point coordinate of pt_segm tu use the other point
        of pt_segm (pt_segm will be removed later) */
        if( pt_ref->m_End == pt_segm->m_Start )
        {
            pt_ref->m_End = pt_segm->m_End;
            return pt_segm;
        }
        else
        {
            pt_ref->m_End = pt_segm->m_Start;
            return pt_segm;
        }
    }
    return NULL;
}


/**
 * Function RemoveMisConnectedTracks
 * finds all track segments which are mis-connected (to more than one net).
 * When such a bad segment is found, mark it as needing to be removed.
 * and remove all tracks having at least one flagged segment.
 * @param aDC = the current device context (can be NULL)
 * @return true if any change is made
 */
bool PCB_EDIT_FRAME::RemoveMisConnectedTracks( wxDC* aDC )
{
    TRACK*          segment;
    TRACK*          other;
    TRACK*          next;
    int             net_code_s, net_code_e;
    bool            isModified = false;

    for( segment = GetBoard()->m_Track;  segment;  segment = (TRACK*) segment->Next() )
    {
        segment->SetState( FLAG0, OFF );

        // find the netcode for segment using anything connected to the "start" of "segment"
        net_code_s = -1;
        if( segment->start  &&  segment->start->Type()==TYPE_PAD )
        {
            // get the netcode of the pad to propagate.
            net_code_s = ((D_PAD*)(segment->start))->GetNet();
        }
        else
        {
            other = Locate_Piste_Connectee( segment, GetBoard()->m_Track,
                                             NULL, START );
            if( other )
                net_code_s = other->GetNet();
        }

        if( net_code_s < 0 )
            continue;           // the "start" of segment is not connected

        // find the netcode for segment using anything connected to the "end" of "segment"
        net_code_e = -1;
        if( segment->end  &&  segment->end->Type()==TYPE_PAD )
        {
            net_code_e = ((D_PAD*)(segment->end))->GetNet();
        }
        else
        {
            other = Locate_Piste_Connectee( segment, GetBoard()->m_Track,
                                             NULL, END );
            if( other )
                net_code_e = other->GetNet();
        }

        if( net_code_e < 0 )
            continue;           // the "end" of segment is not connected

        // Netcodes do not agree, so mark the segment as needed to be removed
        if( net_code_s != net_code_e )
        {
            segment->SetState( FLAG0, ON );
        }
    }

    // Remove flagged segments
    for( segment = GetBoard()->m_Track;  segment;  segment = next )
    {
        next = (TRACK*) segment->Next();

        if( segment->GetState( FLAG0 ) )    // Ssegment is flagged to be removed
        {
            segment->SetState( FLAG0, OFF );
            isModified = true;
            GetBoard()->m_Status_Pcb = 0;
            Remove_One_Track( aDC, segment );
            // the current segment could be deleted, so restart to the beginning
            next = GetBoard()->m_Track;
        }
    }

    return isModified;
}


#if 0

/***************************************************************/
static void Gen_Raccord_Track( PCB_EDIT_FRAME* frame, wxDC* DC )
/***************************************************************/

/**
 * Function Gen_Raccord_Track
 * tests the ends of segments.  If and end is on a segment of other track, but not
 * on other's end, the other segment is cut into 2, the point of cut being the end of
 * segment first being operated on.  This is done so that the subsequent tests
 * of connection, which do not test segment overlaps, will see this continuity.
 */
{
    TRACK*          segment;
    TRACK*          other;
    int             nn = 0;
    int             masquelayer;
    int             ii, percent, oldpercent;
    wxString        msg;

    frame->Affiche_Message( wxT( "Gen Raccords sur Pistes:" ) );
    if( frame->GetBoard()->GetNumSegmTrack() == 0 )
        return;

    frame->DrawPanel->m_AbortRequest = FALSE;

    oldpercent = -1; ii = 0;
    for( segment = frame->GetBoard()->m_Track;  segment;  segment = segment->Next() )
    {
        // display activity
        ii++;
        percent = (100 * ii) / frame->GetBoard()->m_Track.GetCount();
        if( percent != oldpercent )
        {
            frame->DisplayActivity( percent, wxT( "Tracks: " ) );
            oldpercent = percent;

            msg.Printf( wxT( "%d" ), frame->GetBoard()->m_Track.GetCount() );
            frame->MsgPanel->SetMessage( POS_AFF_MAX, wxT( "Max" ), msg, GREEN );

            msg.Printf( wxT( "%d" ), ii );
            frame->MsgPanel->SetMessage( POS_AFF_NUMSEGM, wxT( "Segm" ), msg, CYAN );
        }

        if( frame->DrawPanel->m_AbortRequest )
            return;

        masquelayer = segment->ReturnMaskLayer();

        // look at the "start" of the "segment"
        for( other = frame->GetBoard()->m_Track;  other;  other = other->Next() )
        {
            TRACK* newTrack;

            other = Locate_Pistes( other, segment->m_Start, masquelayer );
            if( other == NULL )
                break;

            if( other == segment )
                continue;

            if( other->Type() == TYPE_VIA )
                continue;

            if( segment->m_Start == other->m_Start )
                continue;

            if( segment->m_Start == other->m_End )
                continue;

            // Test if the "end" of this segment is already connected to other
            if( segment->m_End == other->m_Start )
                continue;

            if( segment->m_End == other->m_End )
                continue;

            other->Draw( frame->DrawPanel, DC, GR_XOR );

            nn++;
            msg.Printf( wxT( "%d" ), nn );
            frame->MsgPanel->SetMessage( POS_AFF_VAR, wxT( "New <" ), msg, YELLOW );

            // create a new segment and insert it next to "other", then shorten other.
            newTrack = other->Copy();

            frame->GetBoard()->m_Track.Insert( newTrack, other->Next() );

            other->m_End      = segment->m_Start;
            newTrack->m_Start = segment->m_Start;

            Trace_Une_Piste( frame->DrawPanel, DC, other, 2, GR_OR );

            // skip forward one, skipping the newTrack
            other = newTrack;
        }

        // look at the "end" of the "segment"
        for( other = frame->GetBoard()->m_Track;  other;  other = other->Next() )
        {
            TRACK* newTrack;

            other = Locate_Pistes( other, segment->m_End, masquelayer );
            if( other == NULL )
                break;

            if( other == segment )
                continue;

            if( other->Type() == TYPE_VIA )
                continue;

            if( segment->m_End == other->m_Start )
                continue;

            if( segment->m_End == other->m_End )
                continue;

            if( segment->m_Start == other->m_Start )
                continue;

            if( segment->m_Start == other->m_End )
                continue;

            other->Draw( frame->DrawPanel, DC, GR_XOR );

            nn++;
            msg.Printf( wxT( "%d" ), nn );
            frame->MsgPanel->SetMessage( POS_AFF_VAR, wxT( "New >" ), msg, YELLOW );

            // create a new segment and insert it next to "other", then shorten other.
            newTrack = other->Copy();
            frame->GetBoard()->m_Track.Insert( newTrack, other->Next() );

            other->m_End      = segment->m_End;
            newTrack->m_Start = segment->m_End;

            Trace_Une_Piste( frame->DrawPanel, DC, other, 2, GR_OR );

            // skip forward one, skipping the newTrack
            other = newTrack;
        }
    }
}

#endif


#if defined(CONN2PAD_ENBL)

/**
 * Function ConnectDanglingEndToPad
 * looks for vias which have no netcode and which are in electrical contact
 * with a track to the degree that the track's end point falls on the via.
 * Note that this is not a rigorous electrical check, but is better than
 * testing for the track endpoint equaling the via center.  When such a via
 * is found, then add a small track to bridge from the overlapping track to
 * the via and change the via's netcode so that subsequent continuity checks
 * can be done with the faster equality algorithm.
 */
static void ConnectDanglingEndToVia( BOARD* pcb )
{
    for( TRACK* track = pcb->m_Track;  track;  track = track->Next() )
    {
        SEGVIA* via;

        if( track->Type()!=TYPE_VIA  || (via = (SEGVIA*)track)->GetNet()!=0 )
            continue;

        for( TRACK* other = pcb->m_Track;  other;  other = other->Next() )
        {
            if( other == track )
                continue;

            if( !via->IsOnLayer( other->GetLayer() ) )
                continue;

            // if the other track's m_End does not match the via position, and the track's m_Start is
            // within the bounds of the via, and the other track has no start
            if( other->m_End!=via->GetPosition() && via->HitTest( other->m_Start ) && !other->start )
            {
                TRACK* newTrack = other->Copy();

                pcb->m_Track.Insert( newTrack, other->Next() );

                newTrack->m_End = via->GetPosition();

                newTrack->start = other;
                newTrack->end   = via;
                other->start = newTrack;

                via->SetNet( other->GetNet() );

                if( !via->start )
                    via->start = other;

                if( !via->end )
                    via->end = other;
            }

            // if the other track's m_Start does not match the via position, and the track's m_End is
            // within the bounds of the via, and the other track has no end
            else if( other->m_Start!=via->GetPosition() && via->HitTest( other->m_End ) && !other->end )
            {
                TRACK* newTrack = other->Copy();

                pcb->m_Track.Insert( newTrack, other->Next() );

                newTrack->m_Start = via->GetPosition();

                newTrack->start = via;
                newTrack->end   = other;
                other->end = newTrack;

                via->SetNet( other->GetNet() );

                if( !via->start )
                    via->start = other;

                if( !via->end )
                    via->end = other;
            }
        }
    }
}


/***************************************************************/
void ConnectDanglingEndToPad( PCB_EDIT_FRAME* frame, wxDC* DC )
/**************************************************************/

/**
 * Function ConnectDanglingEndToPad
 * possibly adds a segment to the end of any and all tracks if their end is not exactly
 * connected into the center of the pad.  This allows faster control of
 * connections.
 */
{
    TRACK*          segment;
    int             nb_new_piste = 0;
    wxString        msg;

    frame->DrawPanel->m_AbortRequest = FALSE;

    for( segment = frame->GetBoard()->m_Track;  segment;  segment = segment->Next() )
    {
        D_PAD*          pad;

        if( frame->DrawPanel->m_AbortRequest )
            return;

        pad = Locate_Pad_Connecte( frame->GetBoard(), segment, START );
        if( pad )
        {
            // test if the track is not precisely starting on the found pad
            if( segment->m_Start != pad->m_Pos )
            {
                if( Locate_Piste_Connectee( segment, frame->GetBoard()->m_Track,
                                            NULL, START ) == NULL )
                {
                    TRACK* newTrack = segment->Copy();

                    frame->GetBoard()->m_Track.Insert( newTrack, segment->Next() );

                    newTrack->m_End = pad->m_Pos;

                    newTrack->start = segment;
                    newTrack->end   = pad;

                    nb_new_piste++;

                    newTrack->Draw( frame->DrawPanel, DC, GR_OR );
                }
            }
        }

        pad = Locate_Pad_Connecte( frame->GetBoard(), segment, END );
        if( pad )
        {
            // test if the track is not precisely ending on the found pad
            if( segment->m_End != pad->m_Pos )
            {
                if( Locate_Piste_Connectee( segment, frame->GetBoard()->m_Track,
                                            NULL, END ) == NULL )
                {
                    TRACK* newTrack = segment->Copy();

                    frame->GetBoard()->m_Track.Insert( newTrack, segment->Next() );

                    newTrack->m_Start = pad->m_Pos;

                    newTrack->start = pad;
                    newTrack->end   = segment;
                    nb_new_piste++;
                }
            }
        }
    }
}

#endif
