/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file edit_track_width.cpp
 * @brief Functions to modify sizes of segment, track, net, all vias and/or all tracks.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxPcbStruct.h>

#include <class_board.h>
#include <class_track.h>

#include <pcbnew.h>
#include <drc_stuff.h>
#include <protos.h>


/**
 * Function SetTrackSegmentWidth
 *  Modify one track segment width or one via diameter and drill (using DRC control).
 *  Basic routine used by other routines when editing tracks or vias
 * @param aTrackItem = the track segment or via to modify
 * @param aItemsListPicker = the list picker to use for an undo command (can be NULL)
 * @param aUseNetclassValue = true to use NetClass value, false to use BOARD::m_designSettings value
 * @return  true if done, false if no not change (because DRC error)
 */
bool PCB_EDIT_FRAME::SetTrackSegmentWidth( TRACK*             aTrackItem,
                                           PICKED_ITEMS_LIST* aItemsListPicker,
                                           bool               aUseNetclassValue )
{
    int           initial_width, new_width;
    int           initial_drill = -1,new_drill = -1;
    bool          change_ok = false;
    NETINFO_ITEM* net = NULL;

    if( aUseNetclassValue )
        net = aTrackItem->GetNet();

    initial_width = aTrackItem->GetWidth();

    if( net )
        new_width = net->GetTrackWidth();
    else
        new_width = GetDesignSettings().GetCurrentTrackWidth();

    if( aTrackItem->Type() == PCB_VIA_T )
    {
        const VIA *via = static_cast<const VIA *>( aTrackItem );

        // Micro vias have a size only defined in their netclass
        // (no specific values defined by a table of specific value)
        // Ensure the netclass is accessible:
        if( via->GetViaType() == VIA_MICROVIA && net == NULL )
            net = aTrackItem->GetNet();

        // Get the draill value, regardless it is default or specific
        initial_drill = via->GetDrillValue();

        if( net )
        {
            new_width = net->GetViaSize();
            new_drill = net->GetViaDrillSize();
        }
        else
        {
            new_width = GetDesignSettings().GetCurrentViaSize();
            new_drill = GetDesignSettings().GetCurrentViaDrill();
        }

        if( via->GetViaType() == VIA_MICROVIA )
        {
            if( net )
            {
                new_width = net->GetMicroViaSize();
                new_drill = net->GetMicroViaDrillSize();
            }
            else
            {
                // Should not occur
            }
        }

        // Old versions set a drill value <= 0, when the default netclass it used
        // but it could be better to set the drill value to the actual value
        // to avoid issues for existing vias, if the default drill value is modified
        // in the netclass, and not in current vias.
        if( via->GetDrill() <= 0 )      // means default netclass drill value used
        {
            initial_drill  = -1;        // Force drill vias re-initialization
        }
    }

    aTrackItem->SetWidth( new_width );

    // make a DRC test because the new size is bigger than the old size
    if( initial_width < new_width )
    {
        int diagdrc = OK_DRC;

        if( g_Drc_On )
            diagdrc = m_drc->Drc( aTrackItem, GetBoard()->m_Track );

        if( diagdrc == OK_DRC )
            change_ok = true;
    }
    else if( initial_width > new_width )
    {
        change_ok = true;
    }
    else if( (aTrackItem->Type() == PCB_VIA_T) )
    {
        // if a via has its drill value changed, force change
        if( initial_drill != new_drill )
            change_ok = true;
    }

    if( change_ok )
    {
        OnModify();

        if( aItemsListPicker )
        {
            aTrackItem->SetWidth( initial_width );
            ITEM_PICKER picker( aTrackItem, UR_CHANGED );
            picker.SetLink( aTrackItem->Clone() );
            aItemsListPicker->PushItem( picker );
            aTrackItem->SetWidth( new_width );

            if( aTrackItem->Type() == PCB_VIA_T )
            {
                // Set new drill value. Note: currently microvias have only a default drill value
                VIA *via = static_cast<VIA *>( aTrackItem );
                if( new_drill > 0 )
                    via->SetDrill( new_drill );
                else
                    via->SetDrillDefault();
            }
        }
    }
    else
    {
        aTrackItem->SetWidth( initial_width );
    }

    return change_ok;
}


/**
 * Function Edit_TrackSegm_Width
 * Modify one track segment width or one via diameter (using DRC control).
 * @param aDC = the curred device context (can be NULL)
 * @param aTrackItem = the track segment or via to modify
 */
void PCB_EDIT_FRAME::Edit_TrackSegm_Width( wxDC* aDC, TRACK* aTrackItem )
{
    PICKED_ITEMS_LIST itemsListPicker;
    bool change = SetTrackSegmentWidth( aTrackItem, &itemsListPicker, false );

    if( change == 0 || aTrackItem->GetFlags() )
        return;     // No change

    // The segment has changed: redraw it and save it in undo list
    if( aDC )
    {
        TRACK* oldsegm = (TRACK*) itemsListPicker.GetPickedItemLink( 0 );
        wxASSERT( oldsegm );
        m_canvas->CrossHairOff( aDC );                  // Erase cursor shape
        oldsegm->Draw( m_canvas, aDC, GR_XOR );         // Erase old track shape
        aTrackItem->Draw( m_canvas, aDC, GR_OR );       // Display new track shape
        m_canvas->CrossHairOn( aDC );                   // Display cursor shape
    }

    SaveCopyInUndoList( itemsListPicker, UR_CHANGED );
}


/**
 * Function Edit_Track_Width
 * Modify a full track width (using DRC control).
 * a full track is the set of track segments between 2 ends: pads or a point that has
 * more than 2 segments ends connected
 * @param aDC = the curred device context (can be NULL)
 * @param aTrackSegment = a segment or via on the track to change
 */
void PCB_EDIT_FRAME::Edit_Track_Width( wxDC* aDC, TRACK* aTrackSegment )
{
    TRACK* pt_track;
    int    nb_segm;

    if( aTrackSegment == NULL )
        return;

    pt_track = GetBoard()->MarkTrace( aTrackSegment, &nb_segm, NULL, NULL, true );

    PICKED_ITEMS_LIST itemsListPicker;
    bool change = false;

    for( int ii = 0; ii < nb_segm; ii++, pt_track = pt_track->Next() )
    {
        pt_track->SetState( BUSY, false );

        if( SetTrackSegmentWidth( pt_track, &itemsListPicker, false ) )
            change = true;
    }

    if( !change )
        return;

    // Some segment have changed: redraw them and save in undo list
    if( aDC )
    {
        m_canvas->CrossHairOff( aDC );                     // Erase cursor shape

        for( unsigned ii = 0; ii < itemsListPicker.GetCount(); ii++ )
        {
            TRACK* segm = (TRACK*) itemsListPicker.GetPickedItemLink( ii );
            segm->Draw( m_canvas, aDC, GR_XOR );            // Erase old track shape
            segm = (TRACK*) itemsListPicker.GetPickedItem( ii );
            segm->Draw( m_canvas, aDC, GR_OR );             // Display new track shape
        }

        m_canvas->CrossHairOn( aDC );                   // Display cursor shape
    }

    SaveCopyInUndoList( itemsListPicker, UR_CHANGED );
}


/**
 * Function Change_Net_Tracks_And_Vias_Sizes
 * Reset all tracks width and vias diameters and drill
 * to their default Netclass value or current values
 * @param aNetcode : the netcode of the net to edit
 * @param aUseNetclassValue : bool. True to use netclass values, false to use current values
 */
bool PCB_EDIT_FRAME::Change_Net_Tracks_And_Vias_Sizes( int aNetcode, bool aUseNetclassValue )
{
    TRACK* pt_segm;

    if( aNetcode <= 0 )
        return false;

    // Examine segments
    PICKED_ITEMS_LIST itemsListPicker;
    bool change = false;

    for( pt_segm = GetBoard()->m_Track; pt_segm != NULL; pt_segm = pt_segm->Next() )
    {
        if( aNetcode != pt_segm->GetNetCode() )         // not in net
            continue;

        // we have found a item member of the net
        if( SetTrackSegmentWidth( pt_segm, &itemsListPicker, aUseNetclassValue ) )
            change = true;
    }

    if( !change )
        return false;

    // Some segment have changed: save them in undo list
    SaveCopyInUndoList( itemsListPicker, UR_CHANGED );
    return true;
}


bool PCB_EDIT_FRAME::Reset_All_Tracks_And_Vias_To_Netclass_Values( bool aTrack, bool aVia )
{
    TRACK* pt_segm;

    // read and edit tracks and vias if required
    PICKED_ITEMS_LIST itemsListPicker;
    bool change = false;

    for( pt_segm = GetBoard()->m_Track; pt_segm != NULL; pt_segm = pt_segm->Next() )
    {
        if( (pt_segm->Type() == PCB_VIA_T ) && aVia )
        {
            if( SetTrackSegmentWidth( pt_segm, &itemsListPicker, true ) )
                change = true;
        }

        if( (pt_segm->Type() == PCB_TRACE_T ) && aTrack )
        {
            if( SetTrackSegmentWidth( pt_segm, &itemsListPicker, true ) )
                change = true;
        }
    }

    if( !change )
        return false;

    // Some segment have changed: save them in undo list
    SaveCopyInUndoList( itemsListPicker, UR_CHANGED );

    return true;
}
