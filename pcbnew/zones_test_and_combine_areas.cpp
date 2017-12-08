/**
 * @file zones_test_and_combine_areas.cpp
 * @brief Functions to test, merge and cut polygons used as copper areas outlines
 *        some pieces of code come from FreePCB.
 */

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

#include <fctsys.h>
#include <common.h>
#include <confirm.h>
#include <class_undoredo_container.h>

#include <class_board.h>
#include <class_zone.h>
#include <class_marker_pcb.h>

#include <pcbnew.h>
#include <drc_stuff.h>
#include <math_for_graphics.h>

#define STRAIGHT 0      // To be remove after math_for_graphics code cleanup


bool BOARD::OnAreaPolygonModified( PICKED_ITEMS_LIST* aModifiedZonesList,
                                   ZONE_CONTAINER* modified_area )
{
    // clip polygon against itself
    bool modified = NormalizeAreaPolygon( aModifiedZonesList, modified_area );

    // now see if we need to clip against other areas
    /*
    LAYER_NUM layer = modified_area->GetLayer();
    */

    bool bCheckAllAreas = TestAreaIntersections( modified_area );

    if( bCheckAllAreas )
    {
        modified = true;
        CombineAllAreasInNet( aModifiedZonesList, modified_area->GetNetCode(), true );
    }

    /*

    FIXME : do we really need this?

    if( !IsCopperLayer( layer ) )       // Refill non copper zones on this layer
    {
        for( unsigned ia = 0; ia < m_ZoneDescriptorList.size(); ia++ )
            if( m_ZoneDescriptorList[ia]->GetLayer() == layer )
                m_ZoneDescriptorList[ia]->BuildFilledSolidAreasPolygons( this );
    }

    */

    // Test for bad areas: all zones must have more than 2 corners:
    // Note: should not happen, but just in case.
    for( unsigned ii = 0; ii < m_ZoneDescriptorList.size(); )
    {
        ZONE_CONTAINER* zone = m_ZoneDescriptorList[ii];

        if( zone->GetNumCorners() >= 3 )
            ii++;
        else               // Remove zone because it is incorrect:
            RemoveArea( aModifiedZonesList, zone );
    }

    return modified;
}


bool BOARD::CombineAllAreasInNet( PICKED_ITEMS_LIST* aDeletedList, int aNetCode,
                                  bool aUseLocalFlags )
{
    if( m_ZoneDescriptorList.size() <= 1 )
        return false;

    bool modified = false;

    // Loop through all combinations
    for( unsigned ia1 = 0; ia1 < m_ZoneDescriptorList.size() - 1; ia1++ )
    {
        ZONE_CONTAINER* curr_area = m_ZoneDescriptorList[ia1];

        if( curr_area->GetNetCode() != aNetCode )
            continue;

        // legal polygon
        BOX2I b1 = curr_area->Outline()->BBox();
        bool  mod_ia1 = false;

        for( unsigned ia2 = m_ZoneDescriptorList.size() - 1; ia2 > ia1; ia2-- )
        {
            ZONE_CONTAINER* area2 = m_ZoneDescriptorList[ia2];

            if( area2->GetNetCode() != aNetCode )
                continue;

            if( curr_area->GetPriority() != area2->GetPriority() )
                continue;

            if( curr_area->GetIsKeepout() != area2->GetIsKeepout() )
                continue;

            if( curr_area->GetLayer() != area2->GetLayer() )
                continue;

            BOX2I b2 = area2->Outline()->BBox();

            if( b1.Intersects( b2 ) )
            {
                // check area2 against curr_area
                if( curr_area->GetLocalFlags() || area2->GetLocalFlags()
                    || aUseLocalFlags == false )
                {
                    bool ret = TestAreaIntersection( curr_area, area2 );

                    if( ret )
                        ret = CombineAreas( aDeletedList, curr_area, area2 );

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


bool BOARD::TestAreaIntersections( ZONE_CONTAINER* area_to_test )
{
    for( unsigned ia2 = 0; ia2 < m_ZoneDescriptorList.size(); ia2++ )
    {
        ZONE_CONTAINER* area2 = m_ZoneDescriptorList[ia2];

        if( area_to_test->GetNetCode() != area2->GetNetCode() )
            continue;

        if( area_to_test == area2 )
            continue;

        // see if areas are on same layer
        if( area_to_test->GetLayer() != area2->GetLayer() )
            continue;

        // test for different priorities
        if( area_to_test->GetPriority() != area2->GetPriority() )
            continue;

        // test for different types
        if( area_to_test->GetIsKeepout() != area2->GetIsKeepout() )
            continue;

        if( TestAreaIntersection( area_to_test, area2 ) )
            return true;
    }

    return false;
}


bool BOARD::TestAreaIntersection( ZONE_CONTAINER* area_ref, ZONE_CONTAINER* area_to_test )
{
    // see if areas are on same layer
    if( area_ref->GetLayer() != area_to_test->GetLayer() )
        return false;

    SHAPE_POLY_SET* poly1 = area_ref->Outline();
    SHAPE_POLY_SET* poly2 = area_to_test->Outline();

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


bool BOARD::CombineAreas( PICKED_ITEMS_LIST* aDeletedList, ZONE_CONTAINER* area_ref,
                          ZONE_CONTAINER* area_to_combine )
{
    if( area_ref == area_to_combine )
    {
        wxASSERT( 0 );
        return false;
    }

    SHAPE_POLY_SET mergedOutlines = *area_ref->Outline();
    SHAPE_POLY_SET areaToMergePoly = *area_to_combine->Outline();

    mergedOutlines.BooleanAdd( areaToMergePoly, SHAPE_POLY_SET::PM_FAST  );
    mergedOutlines.Simplify( SHAPE_POLY_SET::PM_FAST );

    // We should have one polygon with hole
    // We can have 2 polygons with hole, if the 2 initial polygons have only one common corner
    // and therefore cannot be merged (they are dectected as intersecting)
    // but we should never have more than 2 polys
    if( mergedOutlines.OutlineCount() > 2 )
    {
        wxLogMessage(wxT("BOARD::CombineAreas error: more than 2 polys after merging") );
        return false;
    }

    if( mergedOutlines.OutlineCount() > 1 )
        return false;

    // Update the area with the new merged outline
    delete area_ref->Outline();
    area_ref->SetOutline( new SHAPE_POLY_SET( mergedOutlines ) );

    RemoveArea( aDeletedList, area_to_combine );

    area_ref->SetLocalFlags( 1 );
    area_ref->Hatch();

    return true;
}


