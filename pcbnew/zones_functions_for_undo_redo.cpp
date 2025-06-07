/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras <jp.charras@wanadoo.fr>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 *
 * When a zone outline is modified (or created) this zone, or others zones on the same layer
 * and with the same netcode can change or can be deleted due to the fact overlapping zones are
 * merged.  Also, when a zone outline is modified by adding a cutout area, this zone can be
 * converted to more than one area, if the outline is break to 2 or more outlines and therefore
 * new zones are created
 *
 * Due to the complexity of potential changes, and the fact there are only few zones in a board,
 * and a zone has only few segments outlines, the more easy way to undo redo changes is to make
 * a copy of all zones that can be changed and see after zone editing or creation what zones that
 * are really modified, and ones they are modified (changes, deletion or addition)
 */

#include <pcb_edit_frame.h>

#include <board.h>
#include <zone.h>
#include <zones.h>
#include <zones_functions_for_undo_redo.h>

/**
 * Function IsSame
 * test is 2 zones are equivalent:
 * 2 zones are equivalent if they have same parameters and same outlines
 * info relative to filling is not take in account
 * @param aZoneToCompare = zone to compare with "this"
 */
bool ZONE::IsSame( const ZONE& aZoneToCompare )
{
    // compare basic parameters:
    if( GetLayerSet() != aZoneToCompare.GetLayerSet() )
        return false;

    if( GetNetCode() != aZoneToCompare.GetNetCode() )
        return false;

    if( GetAssignedPriority() != aZoneToCompare.GetAssignedPriority() )
        return false;

    // Compare zone specific parameters
    if( GetIsRuleArea() != aZoneToCompare.GetIsRuleArea() )
        return false;

    if( GetDoNotAllowZoneFills() != aZoneToCompare.GetDoNotAllowZoneFills() )
        return false;

    if( GetDoNotAllowVias() != aZoneToCompare.GetDoNotAllowVias() )
        return false;

    if( GetDoNotAllowTracks() != aZoneToCompare.GetDoNotAllowTracks() )
        return false;

    if( GetDoNotAllowPads() != aZoneToCompare.GetDoNotAllowPads() )
        return false;

    if( GetDoNotAllowFootprints() != aZoneToCompare.GetDoNotAllowFootprints() )
        return false;

    if( GetPlacementAreaEnabled() != aZoneToCompare.GetPlacementAreaEnabled() )
        return false;

    if( GetPlacementAreaSourceType() != aZoneToCompare.GetPlacementAreaSourceType() )
        return false;

    if( GetPlacementAreaSource() != aZoneToCompare.GetPlacementAreaSource() )
        return false;

    if( m_ZoneClearance != aZoneToCompare.m_ZoneClearance )
        return false;

    if( m_ZoneMinThickness != aZoneToCompare.GetMinThickness() )
        return false;

    if( m_fillMode != aZoneToCompare.GetFillMode() )
        return false;

    if( m_PadConnection != aZoneToCompare.m_PadConnection )
        return false;

    if( m_thermalReliefGap != aZoneToCompare.m_thermalReliefGap )
        return false;

    if( m_thermalReliefSpokeWidth != aZoneToCompare.m_thermalReliefSpokeWidth )
        return false;

    if( m_zoneName != aZoneToCompare.m_zoneName )
        return false;

    if( m_islandRemovalMode != aZoneToCompare.m_islandRemovalMode )
        return false;

    if( m_minIslandArea != aZoneToCompare.m_minIslandArea )
        return false;


    // Compare outlines
    wxASSERT( m_Poly );                                      // m_Poly == NULL Should never happen
    wxASSERT( aZoneToCompare.Outline() );

    if( Outline() != aZoneToCompare.Outline() )    // Compare vector
        return false;

    return true;
}


/**
 * Function SaveCopyOfZones
 * creates a copy of zones having a given netcode on a given layer,
 * and fill a pick list with pickers to handle these copies
 * the UndoRedo status is set to CHANGED for all items in list
 * Later, UpdateCopyOfZonesList will change and update these pickers after a zone editing
 * @param aPickList = the pick list
 * @param aPcb = the Board
 */
void SaveCopyOfZones( PICKED_ITEMS_LIST& aPickList, BOARD* aPcb )
{
    for( ZONE* zone : aPcb->Zones() )
    {
        ZONE* zoneDup = new ZONE( *zone );
        zoneDup->SetParent( aPcb );
        zoneDup->SetParentGroup( nullptr );

        ITEM_PICKER picker( nullptr, zone, UNDO_REDO::CHANGED );
        picker.SetLink( zoneDup );
        aPickList.PushItem( picker );
    }
}


/**
 * Function UpdateCopyOfZonesList
 * Check a pick list to remove zones identical to their copies and set the type of operation in
 * picker (DELETED, CHANGED).  If an item is deleted, the initial values are retrievered,
 * because they can have changed during editing.
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
 *          its status becomes DELETED
 *          the aAuxiliaryList corresponding picker is removed (if not found : set an error)
 *  >> if the picked zone was flagged as NEWITEM, and was after deleted ,
 *  perhaps combined with another zone  (i.e. not found in board list):
 *          the picker is removed
 *          the zone itself if really deleted
 *          the aAuxiliaryList corresponding picker is removed (if not found : set an error)
 * After aPickList is cleaned, the aAuxiliaryList is read
 *  All pickers flagged NEWITEM are moved to aPickList
 * (the corresponding zones are zone that were created by the zone normalize and combine process,
 * mainly when adding cutout areas, or creating self intersecting contours)
 *  All pickers flagged DELETED are removed, and the corresponding zones actually deleted
 * (the corresponding zones are new zone that were created by the zone normalize process,
 * when creating self intersecting contours, and after combined with an existing zone.
 * At the end of the update process the aAuxiliaryList must be void,
 *  because all pickers created by the combine process
 *  must have been removed (removed for new and deleted zones, or moved in aPickList.)
 * If not an error is set.
 */
void UpdateCopyOfZonesList( PICKED_ITEMS_LIST& aPickList, PICKED_ITEMS_LIST& aAuxiliaryList,
                            BOARD* aPcb )
{
    for( unsigned kk = 0; kk < aPickList.GetCount(); kk++ )
    {
        UNDO_REDO status = aPickList.GetPickedItemStatus( kk );

        ZONE* ref = (ZONE*) aPickList.GetPickedItem( kk );

        for( unsigned ii = 0; ; ii++ )  // analyse the main picked list
        {
            ZONE* zone = aPcb->GetArea( ii );

            if( zone == nullptr )
            {
                /* End of list: the stored item is not found:
                 * it must be in aDeletedList:
                 * search it and restore initial values
                 * or
                 * if flagged NEWITEM: remove it definitively
                 */
                if( status == UNDO_REDO::NEWITEM )
                {
                    delete ref;
                    ref = nullptr;
                    aPickList.RemovePicker( kk );
                    kk--;
                }
                else
                {
                    ZONE* zcopy = (ZONE*) aPickList.GetPickedItemLink( kk );
                    aPickList.SetPickedItemStatus( UNDO_REDO::DELETED, kk );

                    wxASSERT_MSG( zcopy != nullptr,
                                  wxT( "UpdateCopyOfZonesList() error: link = NULL" ) );

                    ref->SwapItemData( zcopy );

                    // the copy was deleted; the link does not exists now.
                    aPickList.SetPickedItemLink( nullptr, kk );
                    delete zcopy;
                }

                // Remove this item from aAuxiliaryList, mainly for tests purpose
                bool notfound = true;

                for( unsigned nn = 0; nn < aAuxiliaryList.GetCount(); nn++ )
                {
                    if( ref != nullptr && aAuxiliaryList.GetPickedItem( nn ) == ref )
                    {
                        aAuxiliaryList.RemovePicker( nn );
                        notfound = false;
                        break;
                    }
                }

                if( notfound )  // happens when the new zone overlaps an existing zone
                                // and these zones are combined
                {
#if  defined(DEBUG)
                    printf( "UpdateCopyOfZonesList(): item not found in aAuxiliaryList,"
                            "combined with another zone\n" );
                    fflush(nullptr);
#endif
                }

                break;
            }

            if( zone == ref )      // picked zone found
            {
                if( aPickList.GetPickedItemStatus( kk ) != UNDO_REDO::NEWITEM )
                {
                    ZONE* zcopy = (ZONE*) aPickList.GetPickedItemLink( kk );

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
        if( aAuxiliaryList.GetPickedItemStatus( ii ) == UNDO_REDO::NEWITEM )
        {
            ITEM_PICKER picker = aAuxiliaryList.GetItemWrapper( ii );
            aPickList.PushItem( picker );
            aAuxiliaryList.RemovePicker( ii );
        }
        else if( aAuxiliaryList.GetPickedItemStatus( ii ) == UNDO_REDO::DELETED )
        {
            delete aAuxiliaryList.GetPickedItemLink( ii );
            aAuxiliaryList.RemovePicker( ii );
        }
        else
        {
            ii++;
        }
    }

    // Should not occur:
    wxASSERT_MSG( aAuxiliaryList.GetCount() == 0,
                  wxT( "UpdateCopyOfZonesList() error: aAuxiliaryList not empty." ) );
}
