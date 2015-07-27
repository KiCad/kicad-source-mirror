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
        EDA_RECT b1 = curr_area->Outline()->GetBoundingBox();
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

            EDA_RECT b2 = area2->Outline()->GetBoundingBox();

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

    CPolyLine* poly1 = area_ref->Outline();
    CPolyLine* poly2 = area_to_test->Outline();

    // test bounding rects
    EDA_RECT      b1 = poly1->GetBoundingBox();
    EDA_RECT      b2 = poly2->GetBoundingBox();

    if( ! b1.Intersects( b2 ) )
        return false;

    // now test for intersecting segments
    for( int icont1 = 0; icont1<poly1->GetContoursCount(); icont1++ )
    {
        int is1 = poly1->GetContourStart( icont1 );
        int ie1 = poly1->GetContourEnd( icont1 );

        for( int ic1 = is1; ic1<=ie1; ic1++ )
        {
            int xi1 = poly1->GetX( ic1 );
            int yi1 = poly1->GetY( ic1 );
            int xf1, yf1;

            if( ic1 < ie1 )
            {
                xf1 = poly1->GetX( ic1 + 1 );
                yf1 = poly1->GetY( ic1 + 1 );
            }
            else
            {
                xf1 = poly1->GetX( is1 );
                yf1 = poly1->GetY( is1 );
            }

            for( int icont2 = 0; icont2<poly2->GetContoursCount(); icont2++ )
            {
                int is2 = poly2->GetContourStart( icont2 );
                int ie2 = poly2->GetContourEnd( icont2 );

                for( int ic2 = is2; ic2<=ie2; ic2++ )
                {
                    int xi2 = poly2->GetX( ic2 );
                    int yi2 = poly2->GetY( ic2 );
                    int xf2, yf2;

                    if( ic2 < ie2 )
                    {
                        xf2 = poly2->GetX( ic2 + 1 );
                        yf2 = poly2->GetY( ic2 + 1 );
                    }
                    else
                    {
                        xf2 = poly2->GetX( is2 );
                        yf2 = poly2->GetY( is2 );
                    }

                    bool intersect = FindSegmentIntersections( xi1, yi1, xf1, yf1,
                                                               xi2, yi2, xf2, yf2 );
                    if( intersect )
                        return true;
                }
            }
        }
    }

    // If a contour is inside an other contour, no segments intersects, but the zones
    // can be combined if a corner is inside an outline (only one corner is enought)
    for( int ic2 = 0; ic2 < poly2->GetCornersCount(); ic2++ )
    {
        int x = poly2->GetX( ic2 );
        int y = poly2->GetY( ic2 );

        if( poly1->TestPointInside( x, y ) )
        {
            return true;
        }
    }

    for( int ic1 = 0; ic1 < poly1->GetCornersCount(); ic1++ )
    {
        int x = poly1->GetX( ic1 );
        int y = poly1->GetY( ic1 );

        if( poly2->TestPointInside( x, y ) )
        {
            return true;
        }
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

    SHAPE_POLY_SET mergedOutlines = ConvertPolyListToPolySet( area_ref->Outline()->m_CornersList );
    SHAPE_POLY_SET areaToMergePoly = ConvertPolyListToPolySet( area_to_combine->Outline()->m_CornersList );

    mergedOutlines.BooleanAdd( areaToMergePoly );
    mergedOutlines.Simplify();

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

    area_ref->Outline()->m_CornersList = ConvertPolySetToPolyList( mergedOutlines );

    RemoveArea( aDeletedList, area_to_combine );

    area_ref->SetLocalFlags( 1 );
    area_ref->Outline()->Hatch();

    return true;
}


int BOARD::Test_Drc_Areas_Outlines_To_Areas_Outlines( ZONE_CONTAINER* aArea_To_Examine,
                                                      bool            aCreate_Markers )
{
    wxString    str;
    int         nerrors = 0;

    // iterate through all areas
    for( int ia = 0; ia < GetAreaCount(); ia++ )
    {
        ZONE_CONTAINER* Area_Ref = GetArea( ia );
        CPolyLine*      refSmoothedPoly = Area_Ref->GetSmoothedPoly();

        if( !Area_Ref->IsOnCopperLayer() )
            continue;

        // When testing only a single area, skip all others
        if( aArea_To_Examine && (aArea_To_Examine != Area_Ref) )
            continue;

        for( int ia2 = 0; ia2 < GetAreaCount(); ia2++ )
        {
            ZONE_CONTAINER* area_to_test = GetArea( ia2 );
            CPolyLine*      testSmoothedPoly = area_to_test->GetSmoothedPoly();

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
            for( int ic = 0; ic < refSmoothedPoly->GetCornersCount(); ic++ )
            {
                int x = refSmoothedPoly->GetX( ic );
                int y = refSmoothedPoly->GetY( ic );

                if( testSmoothedPoly->TestPointInside( x, y ) )
                {
                    // COPPERAREA_COPPERAREA error: copper area ref corner inside copper area
                    if( aCreate_Markers )
                    {
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
            for( int ic2 = 0; ic2 < testSmoothedPoly->GetCornersCount(); ic2++ )
            {
                int x = testSmoothedPoly->GetX( ic2 );
                int y = testSmoothedPoly->GetY( ic2 );

                if( refSmoothedPoly->TestPointInside( x, y ) )
                {
                    // COPPERAREA_COPPERAREA error: copper area corner inside copper area ref
                    if( aCreate_Markers )
                    {
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

            // now test spacing between areas
            for( int icont = 0; icont < refSmoothedPoly->GetContoursCount(); icont++ )
            {
                int ic_start = refSmoothedPoly->GetContourStart( icont );
                int ic_end   = refSmoothedPoly->GetContourEnd( icont );

                for( int ic = ic_start; ic<=ic_end; ic++ )
                {
                    int ax1 = refSmoothedPoly->GetX( ic );
                    int ay1 = refSmoothedPoly->GetY( ic );
                    int ax2, ay2;

                    if( ic == ic_end )
                    {
                        ax2 = refSmoothedPoly->GetX( ic_start );
                        ay2 = refSmoothedPoly->GetY( ic_start );
                    }
                    else
                    {
                        ax2 = refSmoothedPoly->GetX( ic + 1 );
                        ay2 = refSmoothedPoly->GetY( ic + 1 );
                    }

                    for( int icont2 = 0; icont2 < testSmoothedPoly->GetContoursCount(); icont2++ )
                    {
                        int ic_start2 = testSmoothedPoly->GetContourStart( icont2 );
                        int ic_end2   = testSmoothedPoly->GetContourEnd( icont2 );

                        for( int ic2 = ic_start2; ic2<=ic_end2; ic2++ )
                        {
                            int bx1 = testSmoothedPoly->GetX( ic2 );
                            int by1 = testSmoothedPoly->GetY( ic2 );
                            int bx2, by2;

                            if( ic2 == ic_end2 )
                            {
                                bx2 = testSmoothedPoly->GetX( ic_start2 );
                                by2 = testSmoothedPoly->GetY( ic_start2 );
                            }
                            else
                            {
                                bx2 = testSmoothedPoly->GetX( ic2 + 1 );
                                by2 = testSmoothedPoly->GetY( ic2 + 1 );
                            }

                            int x, y;

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
        }
    }

    return nerrors;
}


bool DRC::doEdgeZoneDrc( ZONE_CONTAINER* aArea, int aCornerIndex )
{
    if( !aArea->IsOnCopperLayer() )    // Cannot have a Drc error if not on copper layer
        return true;

    wxString str;

    wxPoint  start = aArea->GetCornerPosition( aCornerIndex );
    wxPoint  end;

    // Search the end point of the edge starting at aCornerIndex
    if( aArea->Outline()->m_CornersList[aCornerIndex].end_contour == false
       && aCornerIndex < (aArea->GetNumCorners() - 1) )
    {
        end = aArea->GetCornerPosition( aCornerIndex + 1 );
    }
    else    // aCornerIndex is the last corner of an outline.
            // the corresponding end point of the segment is the first corner of the outline
    {
        int ii = aCornerIndex - 1;
        end = aArea->GetCornerPosition( ii );

        while( ii >= 0 )
        {
            if( aArea->Outline()->m_CornersList[ii].end_contour )
                break;

            end = aArea->GetCornerPosition( ii );
            ii--;
        }
    }

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
        if( area_to_test->Outline()->TestPointInside( end.x, end.y ) )
        {
            // COPPERAREA_COPPERAREA error: corner inside copper area
            m_currentMarker = fillMarker( aArea, end,
                                          COPPERAREA_INSIDE_COPPERAREA,
                                          m_currentMarker );
            return false;
        }

        // now test spacing between areas
        int ax1    = start.x;
        int ay1    = start.y;
        int ax2    = end.x;
        int ay2    = end.y;

        for( int icont2 = 0; icont2 < area_to_test->Outline()->GetContoursCount(); icont2++ )
        {
            int ic_start2 = area_to_test->Outline()->GetContourStart( icont2 );
            int ic_end2   = area_to_test->Outline()->GetContourEnd( icont2 );

            for( int ic2 = ic_start2; ic2<=ic_end2; ic2++ )
            {
                int bx1 = area_to_test->Outline()->GetX( ic2 );
                int by1 = area_to_test->Outline()->GetY( ic2 );
                int bx2, by2;

                if( ic2 == ic_end2 )
                {
                    bx2 = area_to_test->Outline()->GetX( ic_start2 );
                    by2 = area_to_test->Outline()->GetY( ic_start2 );
                }
                else
                {
                    bx2 = area_to_test->Outline()->GetX( ic2 + 1 );
                    by2 = area_to_test->Outline()->GetY( ic2 + 1 );
                }

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
    }

    return true;
}
