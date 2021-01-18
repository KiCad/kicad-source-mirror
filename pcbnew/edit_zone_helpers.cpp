/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Some code comes from FreePCB.
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

#include <kiface_i.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <board_commit.h>
#include <zone.h>
#include <zones.h>
#include <zones_functions_for_undo_redo.h>
#include <connectivity/connectivity_data.h>
#include <widgets/progress_reporter.h>
#include <zone_filler.h>


void PCB_EDIT_FRAME::Edit_Zone_Params( ZONE* aZone )
{
    int               dialogResult;
    ZONE_SETTINGS     zoneInfo = GetZoneSettings();
    PICKED_ITEMS_LIST pickedList;    // zones for undo/redo command
    PICKED_ITEMS_LIST deletedList;   // zones that have been deleted when combined
    BOARD_COMMIT      commit( this );

    // Save initial zones configuration, for undo/redo, before adding new zone
    // note the net name and the layer can be changed, so we must save all zones
    deletedList.ClearListAndDeleteItems();
    pickedList.ClearListAndDeleteItems();
    SaveCopyOfZones( pickedList, GetBoard(), -1, UNDEFINED_LAYER );

    if( aZone->GetIsRuleArea() )
    {
        // edit a rule area on a copper layer
        zoneInfo << *aZone;
        dialogResult = InvokeRuleAreaEditor( this, &zoneInfo );
    }
    else if( IsCopperLayer( aZone->GetLayer() ) )
    {
        // edit a zone on a copper layer
        zoneInfo << *aZone;
        dialogResult = InvokeCopperZonesEditor( this, &zoneInfo );
    }
    else
    {
        zoneInfo << *aZone;
        dialogResult = InvokeNonCopperZonesEditor( this, &zoneInfo );
    }

    if( dialogResult == wxID_CANCEL )
    {
        deletedList.ClearListAndDeleteItems();
        pickedList.ClearListAndDeleteItems();
        return;
    }

    SetZoneSettings( zoneInfo );
    OnModify();

    if( dialogResult == ZONE_EXPORT_VALUES )
    {
        UpdateCopyOfZonesList( pickedList, deletedList, GetBoard() );
        commit.Stage( pickedList );
        commit.Push( _( "Modify zone properties" ) );
        pickedList.ClearItemsList(); // s_ItemsListPicker is no more owner of picked items
        return;
    }

    wxBusyCursor dummy;

    // Undraw old zone outlines
    for( ZONE* zone : GetBoard()->Zones() )
        GetCanvas()->GetView()->Update( zone );

    zoneInfo.ExportSetting( *aZone );

    NETINFO_ITEM* net = GetBoard()->FindNet( zoneInfo.m_NetcodeSelection );

    if( net )   // net == NULL should not occur
        aZone->SetNetCode( net->GetNetCode() );

    // Combine zones if possible
    GetBoard()->OnAreaPolygonModified( &deletedList, aZone );

    UpdateCopyOfZonesList( pickedList, deletedList, GetBoard() );

    // refill zones with the new properties applied
    std::vector<ZONE*> zones_to_refill;

    for( unsigned i = 0; i < pickedList.GetCount(); ++i )
    {
        ZONE* zone = dyn_cast<ZONE*>( pickedList.GetPickedItem( i ) );

        if( zone == nullptr )
        {
            wxASSERT_MSG( false, "Expected a zone after zone properties edit" );
            continue;
        }

        // aZone won't be filled if the layer set was modified, but it needs to be updated
        if( zone->IsFilled() || zone == aZone )
            zones_to_refill.push_back( zone );
    }

    commit.Stage( pickedList );

    std::lock_guard<KISPINLOCK> lock( GetBoard()->GetConnectivity()->GetLock() );

    if( zones_to_refill.size() )
    {
        ZONE_FILLER filler( GetBoard(), &commit );
        wxString title = wxString::Format( _( "Refill %d Zones" ), (int) zones_to_refill.size() );
        filler.InstallNewProgressReporter( this, title, 4 );

        if( !filler.Fill( zones_to_refill ) )
        {
            commit.Revert();
            return;
        }
    }

    commit.Push( _( "Modify zone properties" ) );
    GetBoard()->GetConnectivity()->RecalculateRatsnest();

    pickedList.ClearItemsList();  // s_ItemsListPicker is no longer owner of picked items
}


bool BOARD::OnAreaPolygonModified( PICKED_ITEMS_LIST* aModifiedZonesList, ZONE* modified_area )
{
    // clip polygon against itself
    bool modified = NormalizeAreaPolygon( aModifiedZonesList, modified_area );

    // now see if we need to clip against other areas
    if( TestZoneIntersections( modified_area ) )
    {
        modified = true;
        CombineAllZonesInNet( aModifiedZonesList, modified_area->GetNetCode() );
    }

    // Test for bad areas: all zones must have more than 2 corners:
    // Note: should not happen, but just in case.
    for( ZONE* zone : m_zones )
    {
        if( zone->GetNumCorners() < 3 )
        {
            ITEM_PICKER picker( nullptr, zone, UNDO_REDO::DELETED );
            aModifiedZonesList->PushItem( picker );
            zone->SetFlags( STRUCT_DELETED );
        }
    }

    return modified;
}


bool BOARD::CombineAllZonesInNet( PICKED_ITEMS_LIST* aDeletedList, int aNetCode )
{
    if( m_zones.size() <= 1 )
        return false;

    bool modified = false;

    for( ZONE* zone : m_zones )
        zone->ClearFlags( STRUCT_DELETED );

    // Loop through all combinations
    for( unsigned ia1 = 0; ia1 < m_zones.size() - 1; ia1++ )
    {
        ZONE* refZone = m_zones[ia1];

        if( refZone->GetNetCode() != aNetCode )
            continue;

        // legal polygon
        BOX2I b1 = refZone->Outline()->BBox();
        bool  mod_ia1 = false;

        for( unsigned ia2 = m_zones.size() - 1; ia2 > ia1; ia2-- )
        {
            ZONE* otherZone = m_zones[ia2];

            if( otherZone->HasFlag( STRUCT_DELETED ) )
                continue;

            if( otherZone->GetNetCode() != aNetCode )
                continue;

            if( refZone->GetPriority() != otherZone->GetPriority() )
                continue;

            if( refZone->GetIsRuleArea() != otherZone->GetIsRuleArea() )
                continue;

            if( refZone->GetLayerSet() != otherZone->GetLayerSet() )
                continue;

            BOX2I b2 = otherZone->Outline()->BBox();

            if( b1.Intersects( b2 ) )
            {
                // check otherZone against refZone
                if( refZone->GetLocalFlags() || otherZone->GetLocalFlags() )
                {
                    bool ret = TestZoneIntersection( refZone, otherZone );

                    if( ret )
                        ret = CombineZones( aDeletedList, refZone, otherZone );

                    if( ret )
                    {
                        mod_ia1 = true;
                        modified = true;
                    }
                }
            }
        }

        if( mod_ia1 )
            ia1--;     // if modified, we need to check it again
    }

    return modified;
}


bool BOARD::TestZoneIntersections( ZONE* aZone )
{
    for( ZONE* otherZone : m_zones )
    {
        if( aZone->GetNetCode() != otherZone->GetNetCode() )
            continue;

        if( aZone == otherZone )
            continue;

        // see if areas are on same layers
        if( aZone->GetLayerSet() != otherZone->GetLayerSet() )
            continue;

        // test for different priorities
        if( aZone->GetPriority() != otherZone->GetPriority() )
            continue;

        // test for different types
        if( aZone->GetIsRuleArea() != otherZone->GetIsRuleArea() )
            continue;

        // Keepout area-specific tests
        if( aZone->GetIsRuleArea() )
        {
            if( aZone->GetDoNotAllowCopperPour() != otherZone->GetDoNotAllowCopperPour() )
                continue;

            if( aZone->GetDoNotAllowTracks() != otherZone->GetDoNotAllowTracks() )
                continue;

            if( aZone->GetDoNotAllowVias() != otherZone->GetDoNotAllowVias() )
                continue;

            if( aZone->GetDoNotAllowPads() != otherZone->GetDoNotAllowPads() )
                continue;

            if( aZone->GetDoNotAllowFootprints() != otherZone->GetDoNotAllowFootprints() )
                continue;
        }
        // Filled zone specific tests
        else
        {
            if( aZone->GetLocalClearance() != otherZone->GetLocalClearance() )
                continue;

            if( aZone->GetThermalReliefGap() != otherZone->GetThermalReliefGap() )
                continue;

            if( aZone->GetThermalReliefSpokeWidth() != otherZone->GetThermalReliefSpokeWidth() )
                continue;

            if( aZone->GetLocalClearance() != otherZone->GetLocalClearance() )
                continue;

            if( aZone->GetPadConnection() != otherZone->GetPadConnection() )
                continue;

            if( aZone->GetMinThickness() != otherZone->GetMinThickness() )
                continue;

            if( aZone->GetCornerSmoothingType() != otherZone->GetCornerSmoothingType() )
                continue;

            if( aZone->GetCornerRadius() != otherZone->GetCornerRadius() )
                continue;
        }

        if( TestZoneIntersection( aZone, otherZone ) )
            return true;
    }

    return false;
}


bool BOARD::TestZoneIntersection( ZONE* aZone1, ZONE* aZone2 )
{
    // see if areas are on same layer
    if( aZone1->GetLayer() != aZone2->GetLayer() )
        return false;

    SHAPE_POLY_SET* poly1 = aZone1->Outline();
    SHAPE_POLY_SET* poly2 = aZone2->Outline();

    // test bounding rects
    BOX2I b1 = poly1->BBox();
    BOX2I b2 = poly2->BBox();

    if( ! b1.Intersects( b2 ) )
        return false;

    // Now test for intersecting segments
    for( auto segIterator1 = poly1->IterateSegmentsWithHoles(); segIterator1; segIterator1++ )
    {
        // Build segment
        SEG firstSegment = *segIterator1;

        for( auto segIterator2 = poly2->IterateSegmentsWithHoles(); segIterator2; segIterator2++ )
        {
            // Build second segment
            SEG secondSegment = *segIterator2;

            // Check whether the two segments built collide
            if( firstSegment.Collide( secondSegment, 0 ) )
                return true;
        }
    }

    // If a contour is inside another contour, no segments intersects, but the zones
    // can be combined if a corner is inside an outline (only one corner is enough)
    for( auto iter = poly2->IterateWithHoles(); iter; iter++ )
    {
        if( poly1->Contains( *iter ) )
            return true;
    }

    for( auto iter = poly1->IterateWithHoles(); iter; iter++ )
    {
        if( poly2->Contains( *iter ) )
            return true;
    }

    return false;
}


bool BOARD::CombineZones( PICKED_ITEMS_LIST* aDeletedList, ZONE* aRefZone, ZONE* aZoneToCombine )
{
    if( aRefZone == aZoneToCombine )
    {
        wxASSERT( 0 );
        return false;
    }

    SHAPE_POLY_SET mergedOutlines = *aRefZone->Outline();
    SHAPE_POLY_SET areaToMergePoly = *aZoneToCombine->Outline();

    mergedOutlines.BooleanAdd( areaToMergePoly, SHAPE_POLY_SET::PM_FAST  );
    mergedOutlines.Simplify( SHAPE_POLY_SET::PM_FAST );

    // We should have one polygon with hole
    // We can have 2 polygons with hole, if the 2 initial polygons have only one common corner
    // and therefore cannot be merged (they are dectected as intersecting)
    // but we should never have more than 2 polys
    if( mergedOutlines.OutlineCount() > 2 )
    {
        wxLogMessage( "BOARD::CombineZones error: more than 2 polys after merging" );
        return false;
    }

    if( mergedOutlines.OutlineCount() > 1 )
        return false;

    // Update the area with the new merged outline
    delete aRefZone->Outline();
    aRefZone->SetOutline( new SHAPE_POLY_SET( mergedOutlines ) );

    ITEM_PICKER picker( nullptr, aZoneToCombine, UNDO_REDO::DELETED );
    aDeletedList->PushItem( picker );
    aZoneToCombine->SetFlags( STRUCT_DELETED );

    aRefZone->SetLocalFlags( 1 );
    aRefZone->HatchBorder();

    return true;
}


