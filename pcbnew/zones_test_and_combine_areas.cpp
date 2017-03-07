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
    LAYER_NUM layer = modified_area->GetLayer();
    bool bCheckAllAreas = TestAreaIntersections( modified_area );

    if( bCheckAllAreas )
    {
        modified = true;
        CombineAllAreasInNet( aModifiedZonesList, modified_area->GetNetCode(), true );
    }

    if( !IsCopperLayer( layer ) )       // Refill non copper zones on this layer
    {
        for( unsigned ia = 0; ia < m_ZoneDescriptorList.size(); ia++ )
            if( m_ZoneDescriptorList[ia]->GetLayer() == layer )
                m_ZoneDescriptorList[ia]->BuildFilledSolidAreasPolygons( this );
    }

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


int BOARD::Test_Drc_Areas_Outlines_To_Areas_Outlines( ZONE_CONTAINER* aArea_To_Examine,
                                                      bool            aCreate_Markers )
{
    int         nerrors = 0;

    // iterate through all areas
    for( int ia = 0; ia < GetAreaCount(); ia++ )
    {
        ZONE_CONTAINER* Area_Ref = GetArea( ia );
        SHAPE_POLY_SET* refSmoothedPoly = Area_Ref->GetSmoothedPoly();

        if( !Area_Ref->IsOnCopperLayer() )
            continue;

        // When testing only a single area, skip all others
        if( aArea_To_Examine && (aArea_To_Examine != Area_Ref) )
            continue;

        for( int ia2 = 0; ia2 < GetAreaCount(); ia2++ )
        {
            ZONE_CONTAINER* area_to_test = GetArea( ia2 );
            SHAPE_POLY_SET* testSmoothedPoly = area_to_test->GetSmoothedPoly();

            if( Area_Ref == area_to_test )
                continue;

            // test for same layer
            if( Area_Ref->GetLayer() != area_to_test->GetLayer() )
                continue;

            // Test for same net
            if( Area_Ref->GetNetCode() == area_to_test->GetNetCode() && Area_Ref->GetNetCode() >= 0 )
                continue;

            // test for different priorities
            if( Area_Ref->GetPriority() != area_to_test->GetPriority() )
                continue;

            // test for different types
            if( Area_Ref->GetIsKeepout() != area_to_test->GetIsKeepout() )
                continue;

            // Examine a candidate zone: compare area_to_test to Area_Ref

            // Get clearance used in zone to zone test.  The policy used to
            // obtain that value is now part of the zone object itself by way of
            // ZONE_CONTAINER::GetClearance().
            int zone2zoneClearance = Area_Ref->GetClearance( area_to_test );

            // Keepout areas have no clearance, so set zone2zoneClearance to 1
            // ( zone2zoneClearance = 0  can create problems in test functions)
            if( Area_Ref->GetIsKeepout() )
                zone2zoneClearance = 1;

            // test for some corners of Area_Ref inside area_to_test
            for( auto iterator = refSmoothedPoly->IterateWithHoles(); iterator; iterator++ )
            {
                VECTOR2I currentVertex = *iterator;

                if( testSmoothedPoly->Contains( currentVertex ) )
                {
                    // COPPERAREA_COPPERAREA error: copper area ref corner inside copper area
                    if( aCreate_Markers )
                    {
                        int x = currentVertex.x;
                        int y = currentVertex.y;

                        wxString msg1   = Area_Ref->GetSelectMenuText();
                        wxString msg2   = area_to_test->GetSelectMenuText();
                        MARKER_PCB*  marker = new MARKER_PCB( COPPERAREA_INSIDE_COPPERAREA,
                                                              wxPoint( x, y ),
                                                              msg1, wxPoint( x, y ),
                                                              msg2, wxPoint( x, y ) );
                        Add( marker );
                    }

                    nerrors++;
                }
            }

            // test for some corners of area_to_test inside Area_Ref
            for( auto iterator = testSmoothedPoly->IterateWithHoles(); iterator; iterator++ )
            {
                VECTOR2I currentVertex = *iterator;

                if( refSmoothedPoly->Contains( currentVertex ) )
                {
                    // COPPERAREA_COPPERAREA error: copper area corner inside copper area ref
                    if( aCreate_Markers )
                    {
                        int x = currentVertex.x;
                        int y = currentVertex.y;

                        wxString msg1   = area_to_test->GetSelectMenuText();
                        wxString msg2   = Area_Ref->GetSelectMenuText();
                        MARKER_PCB*  marker = new MARKER_PCB( COPPERAREA_INSIDE_COPPERAREA,
                                                              wxPoint( x, y ),
                                                              msg1, wxPoint( x, y ),
                                                              msg2, wxPoint( x, y ) );
                        Add( marker );
                    }

                    nerrors++;
                }
            }


            // Iterate through all the segments of refSmoothedPoly
            for( auto refIt = refSmoothedPoly->IterateSegmentsWithHoles(); refIt; refIt++ )
            {
                // Build ref segment
                SEG refSegment = *refIt;

                // Iterate through all the segments in testSmoothedPoly
                for( auto testIt = testSmoothedPoly->IterateSegmentsWithHoles(); testIt; testIt++ )
                {
                    // Build test segment
                    SEG testSegment = *testIt;

                    int x, y;

                    int ax1, ay1, ax2, ay2;
                    ax1 = refSegment.A.x;
                    ay1 = refSegment.A.y;
                    ax2 = refSegment.B.x;
                    ay2 = refSegment.B.y;

                    int bx1, by1, bx2, by2;
                    bx1 = testSegment.A.x;
                    by1 = testSegment.A.y;
                    bx2 = testSegment.B.x;
                    by2 = testSegment.B.y;

                    int d = GetClearanceBetweenSegments( bx1, by1, bx2, by2,
                                                         0,
                                                         ax1, ay1, ax2, ay2,
                                                         0,
                                                         zone2zoneClearance,
                                                         &x, &y );

                    if( d < zone2zoneClearance )
                    {
                        // COPPERAREA_COPPERAREA error : intersect or too close
                        if( aCreate_Markers )
                        {
                            wxString msg1   = Area_Ref->GetSelectMenuText();
                            wxString msg2   = area_to_test->GetSelectMenuText();
                            MARKER_PCB*  marker = new MARKER_PCB( COPPERAREA_CLOSE_TO_COPPERAREA,
                                                                  wxPoint( x, y ),
                                                                  msg1, wxPoint( x, y ),
                                                                  msg2, wxPoint( x, y ) );
                            Add( marker );
                        }

                        nerrors++;
                    }
                }
            }
        }
    }

    return nerrors;
}


bool DRC::doEdgeZoneDrc( ZONE_CONTAINER* aArea, int aCornerIndex )
{
    if( !aArea->IsOnCopperLayer() )    // Cannot have a Drc error if not on copper layer
        return true;
    // Get polygon, contour and vertex index.
    SHAPE_POLY_SET::VERTEX_INDEX index;

    // If the vertex does not exist, there is no conflict
    if( !aArea->Outline()->GetRelativeIndices( aCornerIndex, &index ) )
        return true;

    // Retrieve the selected contour
    SHAPE_LINE_CHAIN contour;
    contour = aArea->Outline()->Polygon( index.m_polygon )[index.m_contour];

    // Retrieve the segment that starts at aCornerIndex-th corner.
    SEG selectedSegment = contour.Segment( index.m_vertex );

    VECTOR2I start = selectedSegment.A;
    VECTOR2I end = selectedSegment.B;

    // iterate through all areas
    for( int ia2 = 0; ia2 < m_pcb->GetAreaCount(); ia2++ )
    {
        ZONE_CONTAINER* area_to_test   = m_pcb->GetArea( ia2 );
        int             zone_clearance = std::max( area_to_test->GetZoneClearance(),
                                                   aArea->GetZoneClearance() );

        // test for same layer
        if( area_to_test->GetLayer() != aArea->GetLayer() )
            continue;

        // Test for same net
        if( ( aArea->GetNetCode() == area_to_test->GetNetCode() ) && (aArea->GetNetCode() >= 0) )
            continue;

        // test for same priority
        if( area_to_test->GetPriority() != aArea->GetPriority() )
            continue;

        // test for same type
        if( area_to_test->GetIsKeepout() != aArea->GetIsKeepout() )
            continue;

        // For keepout, there is no clearance, so use a minimal value for it
        // use 1, not 0 as value to avoid some issues in tests
        if( area_to_test->GetIsKeepout() )
            zone_clearance = 1;

        // test for ending line inside area_to_test
        if( area_to_test->Outline()->Contains( end ) )
        {
            // COPPERAREA_COPPERAREA error: corner inside copper area
            m_currentMarker = fillMarker( aArea, static_cast<wxPoint>( end ),
                                          COPPERAREA_INSIDE_COPPERAREA,
                                          m_currentMarker );
            return false;
        }

        // now test spacing between areas
        int ax1    = start.x;
        int ay1    = start.y;
        int ax2    = end.x;
        int ay2    = end.y;

        // Iterate through all edges in the polygon.
        SHAPE_POLY_SET::SEGMENT_ITERATOR iterator;
        for( iterator = area_to_test->Outline()->IterateSegmentsWithHoles(); iterator; iterator++ )
        {
            SEG segment = *iterator;

            int bx1 = segment.A.x;
            int by1 = segment.A.y;
            int bx2 = segment.B.x;
            int by2 = segment.B.y;

            int x, y;   // variables containing the intersecting point coordinates
            int d = GetClearanceBetweenSegments( bx1, by1, bx2, by2,
                                                 0,
                                                 ax1, ay1, ax2, ay2,
                                                 0,
                                                 zone_clearance,
                                                 &x, &y );

            if( d < zone_clearance )
            {
                // COPPERAREA_COPPERAREA error : edge intersect or too close
                m_currentMarker = fillMarker( aArea, wxPoint( x, y ),
                                              COPPERAREA_CLOSE_TO_COPPERAREA,
                                              m_currentMarker );
                return false;
            }

        }
    }

    return true;
}
