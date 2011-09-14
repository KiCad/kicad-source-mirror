/******************************************/
/* Edit Track: Erase Routines             */
/* Delete segments, tracks, and net areas */
/******************************************/
#include "fctsys.h"

#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "protos.h"


/* Removes 1 segment of track.
 * 2 cases:
 * If There is evidence of new track: delete new segment.
 * Otherwise, delete segment under the cursor.
 */
TRACK* PCB_EDIT_FRAME::Delete_Segment( wxDC* DC, TRACK* aTrack )
{
    int current_net_code;

    if( aTrack == NULL )
        return NULL;

    if( aTrack->IsNew() )  // Trace in progress, erase the last segment
    {
        if( g_CurrentTrackList.GetCount() > 0 )
        {
            int previous_layer = getActiveLayer();

            D( g_CurrentTrackList.VerifyListIntegrity(); )

            // Delete the current trace
            ShowNewTrackWhenMovingCursor( DrawPanel, DC, wxDefaultPosition, false );

            // delete the most recently entered
            delete g_CurrentTrackList.PopBack();

            if( g_TwoSegmentTrackBuild )
            {
                // if in 2 track mode, and the next most recent is a segment
                // not a via, and the one previous to that is a via, then
                // delete up to the via.
                if( g_CurrentTrackList.GetCount() >= 2
                    && g_CurrentTrackSegment->Type() != TYPE_VIA
                    && g_CurrentTrackSegment->Back()->Type() == TYPE_VIA )
                {
                    delete g_CurrentTrackList.PopBack();
                }
            }

            while( g_CurrentTrackSegment && g_CurrentTrackSegment->Type() == TYPE_VIA )
            {
                delete g_CurrentTrackList.PopBack();

                if( g_CurrentTrackSegment && g_CurrentTrackSegment->Type() != TYPE_VIA )
                    previous_layer = g_CurrentTrackSegment->GetLayer();
            }

            // Correct active layer which could change if a via
            // has been erased
            setActiveLayer( previous_layer );

            UpdateStatusBar();

            if( g_TwoSegmentTrackBuild )   // We must have 2 segments or more, or 0
            {
                if( g_CurrentTrackList.GetCount() == 1
                    && g_CurrentTrackSegment->Type() != TYPE_VIA )
                {
                    delete g_CurrentTrackList.PopBack();
                }
            }

            if( g_CurrentTrackList.GetCount() == 0 )
            {
                DrawPanel->SetMouseCapture( NULL, NULL );

                if( GetBoard()->IsHighLightNetON() )
                    High_Light( DC );

                SetCurItem( NULL );
                return NULL;
            }
            else
            {
                if( DrawPanel->IsMouseCaptured() )
                    DrawPanel->m_mouseCaptureCallback( DrawPanel, DC, wxDefaultPosition, false );

                return g_CurrentTrackSegment;
            }
        }
        return NULL;
    }

    current_net_code = aTrack->GetNet();

    DLIST<TRACK>* container = (DLIST<TRACK>*)aTrack->GetList();
    wxASSERT( container );
    container->Remove( aTrack );

    // redraw the area where the track was
    DrawPanel->RefreshDrawingRect( aTrack->GetBoundingBox() );

    SaveCopyInUndoList( aTrack, UR_DELETED );
    OnModify();

    test_1_net_connexion( DC, current_net_code );
    GetBoard()->DisplayInfo( this );
    return NULL;
}


void PCB_EDIT_FRAME::Delete_Track( wxDC* DC, TRACK* aTrack )
{
    if( aTrack != NULL )
    {
        int current_net_code = aTrack->GetNet();
        Remove_One_Track( DC, aTrack );
        OnModify();
        test_1_net_connexion( DC, current_net_code );
    }
}


void PCB_EDIT_FRAME::Delete_net( wxDC* DC, TRACK* aTrack )
{
    if( aTrack == NULL )
        return;

    if( !IsOK( this, _( "Delete NET?" ) ) )
        return;

    PICKED_ITEMS_LIST itemsList;
    ITEM_PICKER       picker( NULL, UR_DELETED );
    int    net_code_delete = aTrack->GetNet();

    /* Search the first item for the given net code */
    TRACK* trackList = GetBoard()->m_Track->GetStartNetCode( net_code_delete );

    /* Remove all segments having the given net code */
    int    ii = 0;
    TRACK* next_track;
    for( TRACK* segm = trackList;  segm; segm = next_track, ++ii )
    {
        next_track = segm->Next();
        if( segm->GetNet() != net_code_delete )
            break;

        GetBoard()->m_Track.Remove( segm );

        // redraw the area where the track was
        DrawPanel->RefreshDrawingRect( segm->GetBoundingBox() );
        picker.m_PickedItem     = segm;
        picker.m_PickedItemType = segm->Type();
        itemsList.PushItem( picker );
    }

    SaveCopyInUndoList( itemsList, UR_DELETED );
    OnModify();
    test_1_net_connexion( DC, net_code_delete );
    GetBoard()->DisplayInfo( this );
}


/* Remove 1 track:
 * The leading segment is removed and all adjacent segments
 * until a pad or a junction point of more than 2 segments is found
 */
void PCB_EDIT_FRAME::Remove_One_Track( wxDC* DC, TRACK* pt_segm )
{
    int segments_to_delete_count;

    if( pt_segm == NULL )
        return;

    TRACK* trackList = MarkTrace( GetBoard(), pt_segm, &segments_to_delete_count,
                                  NULL, NULL, true );

    if( segments_to_delete_count == 0 )
        return;

    int net_code = pt_segm->GetNet();
    PICKED_ITEMS_LIST itemsList;
    ITEM_PICKER       picker( NULL, UR_DELETED );

    int               ii = 0;
    TRACK*            tracksegment = trackList;
    TRACK*            next_track;

    for( ; ii < segments_to_delete_count; ii++, tracksegment = next_track )
    {
        next_track = tracksegment->Next();
        tracksegment->SetState( BUSY, OFF );

        //D( printf( "%s: track %p status=\"%s\"\n", __func__, tracksegment,
        //           TO_UTF8( TRACK::ShowState( tracksegment->GetState( -1 ) ) )
        //          ); )
        D( std::cout << __func__ << ": track " << tracksegment << " status=" \
                     << TO_UTF8( TRACK::ShowState( tracksegment->GetState( -1 ) ) ) \
                     << std::endl; )

        GetBoard()->m_Track.Remove( tracksegment );

        // redraw the area where the track was
        DrawPanel->RefreshDrawingRect( tracksegment->GetBoundingBox() );
        picker.m_PickedItem     = tracksegment;
        picker.m_PickedItemType = tracksegment->Type();
        itemsList.PushItem( picker );
    }

    SaveCopyInUndoList( itemsList, UR_DELETED );

    if( net_code > 0 )
        test_1_net_connexion( DC, net_code );
}
