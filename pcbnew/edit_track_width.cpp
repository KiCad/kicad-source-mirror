/***************************************************************/
/* Edition des pistes: Routines de modification de dimensions: */
/* Modif de largeurs de segment, piste, net , zone et diam Via */
/***************************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "protos.h"



/** Function SetTrackSegmentWidth
 *  Modify one track segment width or one via diameter (using DRC control).
 *  Basic routine used by other routines when editing tracks or vias
 * @param aTrackItem = the track segment or via to modify
 * @param aItemsListPicker = the list picker to use for an undo command (can be NULL)
 * @return  true if done, false if no not change (because DRC error)
 */
bool WinEDA_PcbFrame::SetTrackSegmentWidth( TRACK*             aTrackItem,
                                            PICKED_ITEMS_LIST* aItemsListPicker )
{
    int  initial_width, new_width;
    bool change_ok = false;

    initial_width = aTrackItem->m_Width;
    new_width = aTrackItem->m_Width = g_DesignSettings.m_CurrentTrackWidth;
    if( aTrackItem->Type() == TYPE_VIA )
    {
        new_width = aTrackItem->m_Width = g_DesignSettings.m_CurrentViaSize;
        if( aTrackItem->m_Shape == VIA_MICROVIA )
            new_width = aTrackItem->m_Width = g_DesignSettings.m_CurrentMicroViaSize;
    }

    if( initial_width < new_width )     /* make a DRC test because the new size is bigger than the old size */
    {
        int diagdrc = OK_DRC;
        if( Drc_On )
            diagdrc = m_drc->Drc( aTrackItem, GetBoard()->m_Track );
        if( diagdrc == OK_DRC )
            change_ok = true;
    }
    else if( initial_width > new_width )
        change_ok = true;
    // if new width == initial_width: do nothing

    if( change_ok )
    {
        GetScreen()->SetModify();
        if( aItemsListPicker )
        {
            aTrackItem->m_Width = initial_width;
            ITEM_PICKER picker( aTrackItem, UR_CHANGED );
            picker.m_Link = aTrackItem->Copy();
            aItemsListPicker->PushItem( picker );
            aTrackItem->m_Width = new_width;
        }
    }
    else
        aTrackItem->m_Width = initial_width;

    return change_ok;
}


/** Function Edit_TrackSegm_Width
 * Modify one track segment width or one via diameter (using DRC control).
 * @param  DC = the curred device context (can be NULL)
 * @param aTrackItem = the track segment or via to modify
 */
void WinEDA_PcbFrame::Edit_TrackSegm_Width( wxDC* DC, TRACK* aTrackItem )
{
    PICKED_ITEMS_LIST itemsListPicker;
    bool change = SetTrackSegmentWidth( aTrackItem, &itemsListPicker );

    if( change == 0 || aTrackItem->m_Flags )
        return;     // No change

    // The segment has changed: redraw it and save it in undo list
    TRACK* oldsegm = (TRACK*) itemsListPicker.GetPickedItem( 0 );
    if( DC )
    {
        DrawPanel->CursorOff( DC );                     // Erase cursor shape
        oldsegm->Draw( DrawPanel, DC, GR_XOR );         // Erase old track shape
        aTrackItem->Draw( DrawPanel, DC, GR_OR );       // Display new track shape
        DrawPanel->CursorOn( DC );                      // Display cursor shape
    }
    SaveCopyInUndoList( itemsListPicker, UR_CHANGED );
}

/** Function Edit_Track_Width
 * Modify a full track width (using DRC control).
 * a full track is the set of track segments between 2 ends: pads or a point that has more than 2 segments ends connected
 * @param  DC = the curred device context (can be NULL)
 * @param aTrackSegment = a segment or via on the track to change
 * @return  none
 */
void WinEDA_PcbFrame::Edit_Track_Width( wxDC* DC, TRACK* aTrackSegment )
{
    TRACK* pt_track;
    int    nb_segm;

    if( aTrackSegment == NULL )
        return;

    pt_track = Marque_Une_Piste( this, DC, aTrackSegment, &nb_segm, 0 );

    PICKED_ITEMS_LIST itemsListPicker;
    bool change = false;
    for( int ii = 0; ii < nb_segm; ii++, pt_track = pt_track->Next() )
    {
        pt_track->SetState( BUSY, OFF );
        if( SetTrackSegmentWidth( pt_track, &itemsListPicker ) )
            change = true;
    }

    if( !change )
        return;

    // Some segment have changed: redraw them and save in undo list
    if( DC )
    {
        DrawPanel->CursorOff( DC );                     // Erase cursor shape
        for( unsigned ii = 0; ii < itemsListPicker.GetCount(); ii++ )
        {
            TRACK* segm = (TRACK*) itemsListPicker.GetPickedItemLink( ii );
            segm->Draw( DrawPanel, DC, GR_XOR );            // Erase old track shape
            segm = (TRACK*) itemsListPicker.GetPickedItem( ii );
            segm->Draw( DrawPanel, DC, GR_OR );             // Display new track shape
        }

        DrawPanel->CursorOn( DC );                   // Display cursor shape
    }

    SaveCopyInUndoList( itemsListPicker, UR_CHANGED );
}


/***********************************************************/
void WinEDA_PcbFrame::Edit_Net_Width( wxDC* DC, int Netcode )
/***********************************************************/
{
    TRACK* pt_segm;

    if( Netcode <= 0 )
        return;

    if( !IsOK( this, _( "Change track width (entire NET) ?" ) ) )
        return;

    /* balayage des segments */
    PICKED_ITEMS_LIST itemsListPicker;
    bool change = false;
    for( pt_segm = GetBoard()->m_Track; pt_segm != NULL; pt_segm = pt_segm->Next() )
    {
        if( Netcode != pt_segm->GetNet() )         /* mauvaise piste */
            continue;
        /* piste d'un net trouvee */
        if( SetTrackSegmentWidth( pt_segm, &itemsListPicker ) )
            change = true;
    }

    if( !change )
        return;

    // Some segment have changed: redraw them and save in undo list
    if( DC )
    {
        DrawPanel->CursorOff( DC );                     // Erase cursor shape
        for( unsigned ii = 0; ii < itemsListPicker.GetCount(); ii++ )
        {
            TRACK* segm = (TRACK*) itemsListPicker.GetPickedItemLink( ii );
            segm->Draw( DrawPanel, DC, GR_XOR );            // Erase old track shape
            segm = (TRACK*) itemsListPicker.GetPickedItem( ii );
            segm->Draw( DrawPanel, DC, GR_OR );             // Display new track shape
        }

        DrawPanel->CursorOn( DC );                  // Display cursor shape
    }

    SaveCopyInUndoList( itemsListPicker, UR_CHANGED );
}


/*************************************************************************/
bool WinEDA_PcbFrame::Resize_Pistes_Vias( wxDC* DC, bool Track, bool Via )
/*************************************************************************/

/* remet a jour la largeur des pistes et/ou le diametre des vias
 *  Si piste == 0 , pas de cht sur les pistes
 *  Si via == 0 , pas de cht sur les vias
 */
{
    TRACK* pt_segm;

    if( Track && Via )
    {
        if( !IsOK( this, _( "Edit All Tracks and Vias Sizes" ) ) )
            return FALSE;
    }
    else if( Via )
    {
        if( !IsOK( this, _( "Edit All Via Sizes" ) ) )
            return FALSE;
    }
    else if( Track )
    {
        if( !IsOK( this, _( "Edit All Track Sizes" ) ) )
            return FALSE;
    }

    /* balayage des segments */
    PICKED_ITEMS_LIST itemsListPicker;
    bool change = false;
    for( pt_segm = GetBoard()->m_Track; pt_segm != NULL; pt_segm = pt_segm->Next() )
    {
        if( (pt_segm->Type() == TYPE_VIA ) && Via )
        {
            if( SetTrackSegmentWidth( pt_segm, &itemsListPicker ) )
                change = true;
        }
    }

    if( (pt_segm->Type() == TYPE_TRACK ) && Track )
    {
        if( SetTrackSegmentWidth( pt_segm, &itemsListPicker ) )
            change = true;;
    }
    if( !change )
        return false;

    // Some segment have changed: redraw them and save in undo list
    if( DC )
    {
        DrawPanel->CursorOff( DC );                     // Erase cursor shape
        for( unsigned ii = 0; ii < itemsListPicker.GetCount(); ii++ )
        {
            TRACK* segm = (TRACK*) itemsListPicker.GetPickedItemLink( ii );
            segm->Draw( DrawPanel, DC, GR_XOR );            // Erase old track shape
            segm = (TRACK*) itemsListPicker.GetPickedItem( ii );
            segm->Draw( DrawPanel, DC, GR_OR );             // Display new track shape
        }

        DrawPanel->CursorOn( DC );                  // Display cursor shape
    }

    SaveCopyInUndoList( itemsListPicker, UR_CHANGED );

    return true;
}
