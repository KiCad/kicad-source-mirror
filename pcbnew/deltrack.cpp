/*********************************************/
/* Edition des pistes: Routines d'effacement */
/* Effacement de segment, piste, net et zone */
/*********************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "protos.h"


/***************************************************************/
TRACK* WinEDA_PcbFrame::Delete_Segment( wxDC* DC, TRACK* aTrack )
/***************************************************************/

/* Supprime 1 segment de piste.
 *  2 Cas possibles:
 *  Si On est en trace de nouvelle piste: Effacement du segment en
 *      cours de trace
 *  Sinon : Effacment du segment sous le curseur.
 */
{
    int current_net_code;

    if( aTrack == NULL )
        return NULL;

    if( aTrack->GetState( DELETED ) )
    {
        D( printf( "WinEDA_PcbFrame::Delete_Segment(): bug deleted already deleted TRACK\n" ); )
        return NULL;
    }

    if( aTrack->m_Flags & IS_NEW )  // Trace in progress, erase the last segment
    {
        if( g_CurrentTrackList.GetCount() > 0 )
        {
            int previous_layer = ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer;

            D( g_CurrentTrackList.VerifyListIntegrity(); )

            // effacement de la piste en cours
            ShowNewTrackWhenMovingCursor( DrawPanel, DC, FALSE );

            // delete the most recently entered
            delete g_CurrentTrackList.PopBack();

            if( g_TwoSegmentTrackBuild )
            {
                // if in 2 track mode, and the next most recent is a segment not a via,
                // and the one previous to that is a via, then delete up to the via.
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

            // Rectification couche active qui a pu changer si une via
            // a ete effacee
            ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer = previous_layer;

            UpdateStatusBar();
            if( g_TwoSegmentTrackBuild )   // We must have 2 segments or more, or 0
            {
                if( g_CurrentTrackList.GetCount()==1 && g_CurrentTrackSegment->Type() != TYPE_VIA )
                {
                    delete g_CurrentTrackList.PopBack();
                }
            }

            if( g_CurrentTrackList.GetCount() == 0 )
            {
                DrawPanel->ManageCurseur = NULL;
                DrawPanel->ForceCloseManageCurseur = NULL;

                if( g_HightLigt_Status )
                    Hight_Light( DC );

                SetCurItem( NULL );
                return NULL;
            }
            else
            {
                if( DrawPanel->ManageCurseur )
                    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

                return g_CurrentTrackSegment;
            }
        }
        return NULL;
    }

    current_net_code = aTrack->GetNet();

    DLIST<TRACK>* container = (DLIST<TRACK>*) aTrack->GetList();
    wxASSERT( container );
    container->Remove( aTrack );
    // redraw the area where the track was
    DrawPanel->PostDirtyRect( aTrack->GetBoundingBox() );

    SaveCopyInUndoList( aTrack, UR_DELETED );
    GetScreen()->SetModify();

    test_1_net_connexion( DC, current_net_code );
    GetBoard()->DisplayInfo( this );
    return NULL;
}


/**********************************************************/
void WinEDA_PcbFrame::Delete_Track( wxDC* DC, TRACK* aTrack )
/**********************************************************/
{
    if( aTrack != NULL )
    {
        int current_net_code = aTrack->GetNet();
        Remove_One_Track( DC, aTrack );
        GetScreen()->SetModify();
        test_1_net_connexion( DC, current_net_code );
    }
}


/********************************************************/
void WinEDA_PcbFrame::Delete_net( wxDC* DC, TRACK* aTrack )
/********************************************************/
{
    if( aTrack == NULL )
        return;

    if( !IsOK( this, _( "Delete NET ?" ) ) )
        return;

    PICKED_ITEMS_LIST      itemsList;
    ITEM_PICKER            picker(NULL,UR_DELETED);
    int    net_code_delete = aTrack->GetNet();

    /* Search the first item for the given net code */
    TRACK* trackList = GetBoard()->m_Track->GetStartNetCode( net_code_delete );

    /* Remove all segments having the given net code */
    int    ii = 0;
    TRACK * next_track;
    for( TRACK* segm = trackList;  segm; segm = next_track, ++ii )
    {
        next_track = segm->Next();
        if( segm->GetNet() != net_code_delete )
            break;

        GetBoard()->m_Track.Remove( segm );
        // redraw the area where the track was
        DrawPanel->PostDirtyRect( segm->GetBoundingBox() );
        picker.m_Item = segm;
        itemsList.PushItem(picker);
    }

    SaveCopyInUndoList( itemsList, UR_DELETED );
    GetScreen()->SetModify();
    test_1_net_connexion( DC, net_code_delete );
    GetBoard()->DisplayInfo( this );

}


/********************************************************************/
void WinEDA_PcbFrame::Remove_One_Track( wxDC* DC, TRACK* pt_segm )
/********************************************************************/

/* Routine de suppression de 1 piste:
 *  le segment pointe est supprime puis les segments adjacents
 *  jusqu'a un pad ou un point de jonction de plus de 2 segments
 */
{
    int     segments_to_delete_count;

    if( pt_segm == NULL )
        return;

    TRACK*  trackList = Marque_Une_Piste( this, DC, pt_segm, &segments_to_delete_count, 0 );
    if( segments_to_delete_count == 0 )
        return;

    int net_code = pt_segm->GetNet();
    PICKED_ITEMS_LIST      itemsList;
    ITEM_PICKER            picker(NULL,UR_DELETED);

    int ii = 0;
    TRACK* tracksegment = trackList;
    TRACK * next_track;
    for( ; ii < segments_to_delete_count; ii++, tracksegment = next_track )
    {
        next_track = tracksegment->Next();
        tracksegment->SetState( BUSY, OFF );

        D(printf("%s: track %p status=\"%s\"\n", __func__, tracksegment,
                 CONV_TO_UTF8( TRACK::ShowState( tracksegment->GetState(-1)) )
                 );)

        GetBoard()->m_Track.Remove( tracksegment );
        // redraw the area where the track was
        DrawPanel->PostDirtyRect( tracksegment->GetBoundingBox() );
        picker.m_Item = tracksegment;
        itemsList.PushItem(picker);
    }

    SaveCopyInUndoList( itemsList, UR_DELETED );
    if ( net_code > 0 )
        test_1_net_connexion( DC, net_code );
}
