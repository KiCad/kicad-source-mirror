/**
 * @file zones_functions_for_undo_redo.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras <jp.charras@wanadoo.fr>
 * Copyright (C) 2007-2015 KiCad Developers, see change_log.txt for contributors.
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


/* These functions are relative to undo redo function, when zones are involved.
 * When a zone outline is modified (or created) this zone, or others zones on the same layer
 * and with the same netcode can change or can be deleted
 * This is due to the fact overlapping zones are merged
 * Also, when a zone outline is modified by adding a cutout area,
 * this zone can be converted to more than one area, if the outline is break to 2 or more outlines
 * and therefore new zones are created
 *
 * Due to the complexity of potential changes, and the fact there are only few zones
 * in a board, and a zone has only few segments outlines, the more easy way to
 * undo redo changes is to make a copy of all zones that can be changed
 * and see after zone edition or creation what zones that are really modified,
 * and ones they are modified (changes, deletion or addition)
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>

#include <class_board.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <zones.h>
#include <zones_functions_for_undo_redo.h>

/**
 * Function IsSame
 * test is 2 zones are equivalent:
 * 2 zones are equivalent if they have same parameters and same outlines
 * info relative to filling is not take in account
 * @param aZoneToCompare = zone to compare with "this"
 */
bool ZONE_CONTAINER::IsSame( const ZONE_CONTAINER& aZoneToCompare )
{
    // compare basic parameters:
    if( GetLayer() != aZoneToCompare.GetLayer() )
        return false;

    if( GetNetCode() != aZoneToCompare.GetNetCode() )
        return false;

    if( GetPriority() != aZoneToCompare.GetPriority() )
        return false;

    // Compare zone specific parameters
    if( GetIsKeepout() != aZoneToCompare.GetIsKeepout() )
        return false;

    if( GetIsKeepout() )
    {
        if(  GetDoNotAllowCopperPour() != aZoneToCompare.GetDoNotAllowCopperPour() )
            return false;

        if(  GetDoNotAllowVias() != aZoneToCompare.GetDoNotAllowVias() )
            return false;

        if(  GetDoNotAllowTracks() != aZoneToCompare.GetDoNotAllowTracks() )
            return false;
    }

    if( m_ArcToSegmentsCount != aZoneToCompare.GetArcSegmentCount() )
        return false;

    if( m_ZoneClearance != aZoneToCompare.m_ZoneClearance )
        return false;

    if( m_ZoneMinThickness != aZoneToCompare.GetMinThickness() )
        return false;

    if( m_FillMode != aZoneToCompare.GetFillMode() )
        return false;

    if( m_PadConnection != aZoneToCompare.m_PadConnection )
        return false;

    if( m_ThermalReliefGap != aZoneToCompare.m_ThermalReliefGap )
        return false;

    if( m_ThermalReliefCopperBridge != aZoneToCompare.m_ThermalReliefCopperBridge )
        return false;


    // Compare outlines
    wxASSERT( m_Poly );                                      // m_Poly == NULL Should never happen
    wxASSERT( aZoneToCompare.Outline() );

    if( Outline()->m_CornersList.GetList() !=
        aZoneToCompare.Outline()->m_CornersList.GetList() )    // Compare vector
        return false;

    return true;
}


/**
 * Function SaveCopyOfZones
 * creates a copy of zones having a given netcode on a given layer,
 * and fill a pick list with pickers to handle these copies
 * the UndoRedo status is set to UR_CHANGED for all items in list
 * Later, UpdateCopyOfZonesList will change and update these pickers after a zone edition
 * @param aPickList = the pick list
 * @param aPcb = the Board
 * @param aNetCode = the reference netcode. if aNetCode < 0 all netcodes are used
 * @param aLayer = the layer of zones. if aLayer < 0, all layers are used
 * @return the count of saved copies
 */
int SaveCopyOfZones( PICKED_ITEMS_LIST& aPickList, BOARD* aPcb, int aNetCode, LAYER_NUM aLayer )
{
    int copyCount = 0;

    for( unsigned ii = 0; ; ii++ )
    {
        ZONE_CONTAINER* zone = aPcb->GetArea( ii );

        if( zone == NULL )      // End of list
            break;

        if( aNetCode >= 0 && aNetCode != zone->GetNetCode() )
            continue;

        if( aLayer >= 0 && aLayer != zone->GetLayer() )
            continue;

        ZONE_CONTAINER* zoneDup = new ZONE_CONTAINER( *zone );
        zoneDup->SetParent( aPcb );
        ITEM_PICKER picker( zone, UR_CHANGED );
        picker.SetLink( zoneDup );
        aPickList.PushItem( picker );
        copyCount++;
    }

    return copyCount;
}


/**
 * Function UpdateCopyOfZonesList
 * check a pick list to remove zones identical to their copies
 * and set the type of operation in picker (UR_DELETED, UR_CHANGED)
 * if an item is deleted, the initial values are retrievered,
 * because they can have changed in edition
 * @param aPickList = the main pick list
 * @param aAuxiliaryList = the list of deleted or added (new created) items after calculations
 * @param aPcb = the Board
 *
 * aAuxiliaryList is a list of pickers updated by zone algorithms:
 *  This list contains zones which were added or deleted during the zones combine process
 * aPickList :is a list of zones that can be modified (changed or deleted, or not modified)
 *  Typically, this is the list of existing zones on the layer of the edited zone,
 *  before any change.
 *  >> if the picked zone is not changed, it is removed from list
 *  >> if the picked zone was deleted (i.e. not found in board list), the picker is modified:
 *          its status becomes UR_DELETED
 *          the aAuxiliaryList corresponding picker is removed (if not found : set an error)
 *  >> if the picked zone was flagged as UR_NEW, and was after deleted ,
 *  perhaps combined with an other zone  (i.e. not found in board list):
 *          the picker is removed
 *          the zone itself if really deleted
 *          the aAuxiliaryList corresponding picker is removed (if not found : set an error)
 * After aPickList is cleaned, the aAuxiliaryList is read
 *  All pickers flagged UR_NEW are moved to aPickList
 * (the corresponding zones are zone that were created by the zone normalize and combine process,
 * mainly when adding cutout areas, or creating self intersecting contours)
 *  All pickers flagged UR_DELETED are removed, and the coresponding zones actually deleted
 * (the corresponding zones are new zone that were created by the zone normalize process,
 * when creating self intersecting contours, and after combined with an existing zone.
 * At the end of the update process the aAuxiliaryList must be void,
 *  because all pickers created by the combine process
 *  must have been removed (removed for new and deleted zones, or moved in aPickList.)
 * If not an error is set.
 */
void UpdateCopyOfZonesList( PICKED_ITEMS_LIST& aPickList,
                            PICKED_ITEMS_LIST& aAuxiliaryList,
                            BOARD*             aPcb )
{
    for( unsigned kk = 0; kk < aPickList.GetCount(); kk++ )
    {
        UNDO_REDO_T  status = aPickList.GetPickedItemStatus( kk );

        ZONE_CONTAINER* ref = (ZONE_CONTAINER*) aPickList.GetPickedItem( kk );

        for( unsigned ii = 0; ; ii++ )  // analyse the main picked list
        {
            ZONE_CONTAINER* zone = aPcb->GetArea( ii );

            if( zone == NULL )
            {
                /* End of list: the stored item is not found:
                 * it must be in aDeletedList:
                 * search it and restore initial values
                 * or
                 * if flagged UR_NEW: remove it definitively
                 */
                if( status == UR_NEW )
                {
                    delete ref;
                    ref = NULL;
                    aPickList.RemovePicker( kk );
                    kk--;
                }
                else
                {
                    ZONE_CONTAINER* zcopy = (ZONE_CONTAINER*) aPickList.GetPickedItemLink( kk );
                    aPickList.SetPickedItemStatus( UR_DELETED, kk );

                    wxASSERT_MSG( zcopy != NULL,
                                  wxT( "UpdateCopyOfZonesList() error: link = NULL" ) );

                    ref->Copy( zcopy );

                    // the copy was deleted; the link does not exists now.
                    aPickList.SetPickedItemLink( NULL, kk );
                    delete zcopy;
                }

                // Remove this item from aAuxiliaryList, mainly for tests purpose
                bool notfound = true;

                for( unsigned nn = 0; nn < aAuxiliaryList.GetCount(); nn++ )
                {
                    if( ref != NULL && aAuxiliaryList.GetPickedItem( nn ) == ref )
                    {
                        aAuxiliaryList.RemovePicker( nn );
                        notfound = false;
                        break;
                    }
                }

                if( notfound )  // happens when the new zone overlaps an existing zone
                                // and these zones are combined
                {
                    DBG( printf(
                        "UpdateCopyOfZonesList(): item not found in aAuxiliaryList,"
                        "combined with an other zone\n" ) );
                }
                break;
            }

            if( zone == ref )      // picked zone found
            {
                if( aPickList.GetPickedItemStatus( kk ) != UR_NEW )
                {
                    ZONE_CONTAINER* zcopy = (ZONE_CONTAINER*) aPickList.GetPickedItemLink( kk );

                    if( zone->IsSame( *zcopy ) )    // Remove picked, because no changes
                    {
                        delete zcopy;               // Delete copy
                        aPickList.RemovePicker( kk );
                        kk--;
                    }
                }

                break;
            }
        }
    }

    // Add new zones in main pick list, and remove pickers from Auxiliary List
    for( unsigned ii = 0; ii < aAuxiliaryList.GetCount(); )
    {
        if( aAuxiliaryList.GetPickedItemStatus( ii ) == UR_NEW )
        {
            ITEM_PICKER picker = aAuxiliaryList.GetItemWrapper( ii );
            aPickList.PushItem( picker );
            aAuxiliaryList.RemovePicker( ii );
        }
        else if( aAuxiliaryList.GetPickedItemStatus( ii ) == UR_DELETED )
        {
            delete aAuxiliaryList.GetPickedItemLink( ii );
            aAuxiliaryList.RemovePicker( ii );
        }
        else
            ii++;
    }

    // Should not occur:
    wxASSERT_MSG( aAuxiliaryList.GetCount() == 0,
                  wxT( "UpdateCopyOfZonesList() error: aAuxiliaryList not empty." ) );
}
