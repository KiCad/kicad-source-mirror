/**
 * @file zones_test_and_combine_areas.cpp
 * @brief Functions to test, merge and cut polygons used as copper areas outlines
 *        some pieces of code come from FreePCB.
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


static bool bDontShowSelfIntersectionArcsWarning;
static bool bDontShowSelfIntersectionWarning;
static bool bDontShowIntersectionArcsWarning;


/**
 * Function AddArea
 * Add an empty copper area to board areas list
 * @param aNewZonesList = a PICKED_ITEMS_LIST * where to store new areas pickers (useful
 *                        in undo commands) can be NULL
 * @param aNetcode = the necode of the copper area (0 = no net)
 * @param aLayer = the layer of area
 * @param aStartPointPosition = position of the first point of the polygon outline of this area
 * @param aHatch = hacth option
 * @return pointer to the new area
 */
ZONE_CONTAINER* BOARD::AddArea( PICKED_ITEMS_LIST* aNewZonesList, int aNetcode,
                                int aLayer, wxPoint aStartPointPosition, int aHatch )
{
    ZONE_CONTAINER* new_area = InsertArea( aNetcode,
                                           m_ZoneDescriptorList.size( ) - 1,
                                           aLayer, aStartPointPosition.x,
                                           aStartPointPosition.y, aHatch );

    if( aNewZonesList )
    {
        ITEM_PICKER picker( new_area, UR_NEW );
        aNewZonesList->PushItem( picker );
    }
    return new_area;
}


/**
 * Function RemoveArea
 * remove copper area from net, and put it in a deleted list (if exists)
 * @param aDeletedList = a PICKED_ITEMS_LIST * where to store deleted areas (useful in undo
 *                       commands) can be NULL
 * @param  area_to_remove = area to delete or put in deleted list
 */
void BOARD::RemoveArea( PICKED_ITEMS_LIST* aDeletedList, ZONE_CONTAINER* area_to_remove )
{
    if( area_to_remove == NULL )
        return;

    if( aDeletedList )
    {
        ITEM_PICKER picker( area_to_remove, UR_DELETED );
        aDeletedList->PushItem( picker );
        Remove( area_to_remove );   // remove from zone list, but does not delete it
    }
    else
    {
        Delete( area_to_remove );
    }
}


/**
 * Function InsertArea
 * add empty copper area to net, inserting after m_ZoneDescriptorList[iarea]
 * @return pointer to the new area
 */
ZONE_CONTAINER* BOARD::InsertArea( int netcode, int iarea, int layer, int x, int y, int hatch )
{
    ZONE_CONTAINER* new_area = new ZONE_CONTAINER( this );

    new_area->SetNet( netcode );
    new_area->SetLayer( layer );
    new_area->SetTimeStamp( GetNewTimeStamp() );

    if( iarea < (int) ( m_ZoneDescriptorList.size() - 1 ) )
        m_ZoneDescriptorList.insert( m_ZoneDescriptorList.begin() + iarea + 1, new_area );
    else
        m_ZoneDescriptorList.push_back( new_area );

    new_area->m_Poly->Start( layer, x, y, hatch );
    return new_area;
}


/**
 * Function CompleteArea
 * complete copper area contour by adding a line from last to first corner
 * if there is only 1 or 2 corners, remove (delete) the area
 * @param area_to_complete = area to complete or remove
 * @param style = style of last corner
 * @return 1 if Ok, 0 if area removed
 */
int BOARD::CompleteArea( ZONE_CONTAINER* area_to_complete, int style )
{
    if( area_to_complete->m_Poly->GetNumCorners() > 2 )
    {
        area_to_complete->m_Poly->Close( style );
        return 1;
    }
    else
    {
        Delete( area_to_complete );
    }

    return 0;
}


/**
 * Function TestAreaPolygon
 * Test an area for self-intersection.
 *
 * @param CurrArea = copper area to test
 * @return :
 * -1 if arcs intersect other sides
 *  0 if no intersecting sides
 *  1 if intersecting sides, but no intersecting arcs
 * Also sets utility2 flag of area with return value
 */
int BOARD::TestAreaPolygon( ZONE_CONTAINER* CurrArea )
{
    CPolyLine*         p = CurrArea->m_Poly;

    // first, check for sides intersecting other sides, especially arcs
    bool               bInt    = false;
    bool               bArcInt = false;
    int                n_cont  = p->GetNumContours();

    // make bounding rect for each contour
    std::vector<CRect> cr;
    cr.reserve( n_cont );

    for( int icont = 0; icont<n_cont; icont++ )
        cr.push_back( p->GetCornerBounds( icont ) );

    for( int icont = 0; icont<n_cont; icont++ )
    {
        int is_start = p->GetContourStart( icont );
        int is_end   = p->GetContourEnd( icont );

        for( int is = is_start; is<=is_end; is++ )
        {
            int is_prev = is - 1;

            if( is_prev < is_start )
                is_prev = is_end;

            int is_next = is + 1;

            if( is_next > is_end )
                is_next = is_start;

            int style = p->GetSideStyle( is );
            int x1i   = p->GetX( is );
            int y1i   = p->GetY( is );
            int x1f   = p->GetX( is_next );
            int y1f   = p->GetY( is_next );

            // check for intersection with any other sides
            for( int icont2 = icont; icont2<n_cont; icont2++ )
            {
                if( cr[icont].left > cr[icont2].right
                    || cr[icont].bottom > cr[icont2].top
                    || cr[icont2].left > cr[icont].right
                    || cr[icont2].bottom > cr[icont].top )
                {
                    // rectangles don't overlap, do nothing
                }
                else
                {
                    int is2_start = p->GetContourStart( icont2 );
                    int is2_end   = p->GetContourEnd( icont2 );

                    for( int is2 = is2_start; is2<=is2_end; is2++ )
                    {
                        int is2_prev = is2 - 1;

                        if( is2_prev < is2_start )
                            is2_prev = is2_end;

                        int is2_next = is2 + 1;

                        if( is2_next > is2_end )
                            is2_next = is2_start;

                        if( icont != icont2
                           || (is2 != is && is2 != is_prev && is2 != is_next && is != is2_prev
                               && is !=
                               is2_next ) )
                        {
                            int style2 = p->GetSideStyle( is2 );
                            int x2i    = p->GetX( is2 );
                            int y2i    = p->GetY( is2 );
                            int x2f    = p->GetX( is2_next );
                            int y2f    = p->GetY( is2_next );
                            int ret    = FindSegmentIntersections( x1i, y1i, x1f, y1f, style,
                                                                   x2i, y2i, x2f, y2f, style2 );
                            if( ret )
                            {
                                // intersection between non-adjacent sides
                                bInt = true;

                                if( style != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
                                {
                                    bArcInt = true;
                                    break;
                                }
                            }
                        }
                    }
                }

                if( bArcInt )
                    break;
            }

            if( bArcInt )
                break;
        }

        if( bArcInt )
            break;
    }

    if( bArcInt )
        CurrArea->utility2 = -1;
    else if( bInt )
        CurrArea->utility2 = 1;
    else
        CurrArea->utility2 = 0;

    return CurrArea->utility2;
}


/**
 * Function ClipAreaPolygon
 * Process an area that has been modified, by clipping its polygon against itself.
 * This may change the number and order of copper areas in the net.
 * @param aNewZonesList = a PICKED_ITEMS_LIST * where to store new areas pickers (useful in
 *                        undo commands) can be NULL
 * @param aCurrArea = the zone to process
 * @param bMessageBoxInt == true, shows message when clipping occurs.
 * @param  bMessageBoxArc == true, shows message when clipping can't be done due to arcs.
 * @param bRetainArcs = true to handle arcs (not really used in KiCad)
 * @return:
 *  -1 if arcs intersect other sides, so polygon can't be clipped
 *   0 if no intersecting sides
 *   1 if intersecting sides
 * Also sets areas->utility1 flags if areas are modified
 */
int BOARD::ClipAreaPolygon( PICKED_ITEMS_LIST * aNewZonesList,
                            ZONE_CONTAINER* aCurrArea,
                            bool bMessageBoxArc, bool bMessageBoxInt, bool bRetainArcs )
{
    CPolyLine* curr_polygon = aCurrArea->m_Poly;
    int        test = TestAreaPolygon( aCurrArea ); // this sets utility2 flag

    if( test == -1 && !bRetainArcs )
        test = 1;

    if( test == -1 )
    {
        // arc intersections, don't clip unless bRetainArcs == false
        if( bMessageBoxArc && bDontShowSelfIntersectionArcsWarning == false )
        {
            wxString str;
            str.Printf( wxT( "Area %08lX of net \"%s\" has arcs intersecting other sides.\n" ),
                        aCurrArea->GetTimeStamp(), GetChars( aCurrArea->m_Netname ) );
            str += wxT( "This may cause problems with other editing operations,\n" );
            str += wxT( "such as adding cutouts. It can't be fixed automatically.\n" );
            str += wxT( "Manual correction is recommended." );
            wxMessageBox( str );
        }

        return -1;  // arcs intersect with other sides, error
    }

    // mark all areas as unmodified except this one
    for( unsigned ia = 0; ia < m_ZoneDescriptorList.size(); ia++ )
        m_ZoneDescriptorList[ia]->utility = 0;

    aCurrArea->utility = 1;

    if( test == 1 )
    {
        // non-arc intersections, clip the polygon
        if( bMessageBoxInt && bDontShowSelfIntersectionWarning == false )
        {
            wxString str;
            str.Printf( wxT( "Area %08lX of net \"%s\" is self-intersecting and will be clipped.\n" ),
                        aCurrArea->GetTimeStamp(), GetChars( aCurrArea->m_Netname ) );
            str += wxT( "This may result in splitting the area.\n" );
            str += wxT( "If the area is complex, this may take a few seconds." );
            wxMessageBox( str );

//          bDontShowSelfIntersectionWarning = dlg.bDontShowBoxState;
        }
    }

//** TODO test for cutouts outside of area
//**    if( test == 1 )
    {
        std::vector<CPolyLine*>* pa = new std::vector<CPolyLine*>;
        curr_polygon->UnHatch();
        int n_poly = aCurrArea->m_Poly->NormalizeAreaOutlines( pa, bRetainArcs );

        // i.e if clipping has created some polygons, we must add these new copper areas.
        if( n_poly > 1 )
        {
            ZONE_CONTAINER* NewArea;

            for( int ip = 1; ip < n_poly; ip++ )
            {
                // create new copper area and copy poly into it
                CPolyLine* new_p = (*pa)[ip - 1];
                NewArea = AddArea( aNewZonesList, aCurrArea->GetNet(), aCurrArea->GetLayer(),
                                   wxPoint(0, 0), CPolyLine::NO_HATCH );

                // remove the poly that was automatically created for the new area
                // and replace it with a poly from NormalizeAreaOutlines
                delete NewArea->m_Poly;
                NewArea->m_Poly = new_p;
                NewArea->m_Poly->Hatch();
                NewArea->utility = 1;
            }
        }

        curr_polygon->Hatch();
        delete pa;
    }

    return test;
}


/**
 * Process an area that has been modified, by clipping its polygon against
 * itself and the polygons for any other areas on the same net.
 * This may change the number and order of copper areas in the net.
 * @param aModifiedZonesList = a PICKED_ITEMS_LIST * where to store deleted or added areas
 *                             (useful in undo commands can be NULL
 * @param modified_area = area to test
 * @param  bMessageBoxArc if true, shows message when clipping can't be done due to arcs.
 * @param bMessageBoxInt == true, shows message when clipping occurs.
 * @return :
 * -1 if arcs intersect other sides, so polygon can't be clipped
 *  0 if no intersecting sides
 *  1 if intersecting sides, polygon clipped
 */
int BOARD::AreaPolygonModified( PICKED_ITEMS_LIST* aModifiedZonesList,
                                ZONE_CONTAINER* modified_area,
                                bool            bMessageBoxArc,
                                bool            bMessageBoxInt )
{
    // clip polygon against itself
    int test = ClipAreaPolygon( aModifiedZonesList, modified_area, bMessageBoxArc, bMessageBoxInt );

    if( test == -1 )
        return test;

    // now see if we need to clip against other areas
    int  layer = modified_area->GetLayer();
    bool bCheckAllAreas = false;

    if( test == 1 )
        bCheckAllAreas = true;
    else
        bCheckAllAreas = TestAreaIntersections( modified_area );

    if( bCheckAllAreas )
        CombineAllAreasInNet( aModifiedZonesList, modified_area->GetNet(), bMessageBoxInt, true );

    if( layer >= FIRST_NO_COPPER_LAYER )    // Refill non copper zones on this layer
    {
        if( m_ZoneDescriptorList.size() > 0 )
        {
            for( unsigned ia = 0; ia < m_ZoneDescriptorList.size(); ia++ )
                if( m_ZoneDescriptorList[ia]->GetLayer() == layer )
                    m_ZoneDescriptorList[ia]->BuildFilledPolysListData( this );
        }
    }

    // Test for bad areas: all zones must have more than 2 corners:
    // Note: should not happen, but just in case.
    for( unsigned ia1 = 0; ia1 < m_ZoneDescriptorList.size() - 1; )
    {
        ZONE_CONTAINER* zone = m_ZoneDescriptorList[ia1];

        if( zone->GetNumCorners() >= 3 )
            ia1++;
        else               // Remove zone because it is incorrect:
            RemoveArea( aModifiedZonesList, zone );
    }

    return test;
}


/**
 * Function CombineAllAreasInNet
 * Checks all copper areas in net for intersections, combining them if found
 * @param aDeletedList = a PICKED_ITEMS_LIST * where to store deleted areas (useful in
 *                       undo commands can be NULL
 * @param aNetCode = net to consider
 * @param bMessageBox : if true display warning message box
 * @param bUseUtility : if true, don't check areas if both utility flags are 0
 * Sets utility flag = 1 for any areas modified
 * If an area has self-intersecting arcs, doesn't try to combine it
 */
int BOARD::CombineAllAreasInNet( PICKED_ITEMS_LIST* aDeletedList, int aNetCode,
                                 bool bMessageBox, bool bUseUtility )
{
    if( m_ZoneDescriptorList.size() <= 1 )
        return 0;

    // start by testing all area polygons to set utility2 flags
    for( unsigned ia = 0; ia < m_ZoneDescriptorList.size(); ia++ )
        if( m_ZoneDescriptorList[ia]->GetNet() == aNetCode )
            TestAreaPolygon( m_ZoneDescriptorList[ia] );

    // now loop through all combinations
    for( unsigned ia1 = 0; ia1 < m_ZoneDescriptorList.size() - 1; ia1++ )
    {
        ZONE_CONTAINER* curr_area = m_ZoneDescriptorList[ia1];
        if( curr_area->GetNet() != aNetCode )
            continue;

        // legal polygon
        CRect b1 = curr_area->m_Poly->GetCornerBounds();
        bool  mod_ia1 = false;

        for( unsigned ia2 = m_ZoneDescriptorList.size() - 1; ia2 > ia1; ia2-- )
        {
            ZONE_CONTAINER* area2 = m_ZoneDescriptorList[ia2];

            if( area2->GetNet() != aNetCode )
                continue;
            if( curr_area->GetPriority() != area2->GetPriority() )
                continue;

            if( curr_area->GetLayer() == area2->GetLayer()
                && curr_area->utility2 != -1 && area2->utility2 != -1 )
            {
                CRect b2 = area2->m_Poly->GetCornerBounds();
                if( !( b1.left > b2.right || b1.right < b2.left
                       || b1.bottom > b2.top || b1.top < b2.bottom ) )
                {
                    // check area2 against curr_area
                    if( curr_area->utility || area2->utility || bUseUtility == false )
                    {
                        int ret = TestAreaIntersection( curr_area, area2 );

                        if( ret == 1 )
                            ret = CombineAreas( aDeletedList, curr_area, area2 );

                        if( ret == 1 )
                        {
                            mod_ia1 = true;
                        }
                        else if( ret == 2 )
                        {
                            if( bMessageBox && bDontShowIntersectionArcsWarning == false )
                            {
                                wxString str;
                                str.Printf( wxT( "Areas %d and %d of net \"%s\" intersect, but some of the intersecting sides are arcs.\n" ),
                                            ia1 + 1,
                                            ia2 + 1,
                                            GetChars( curr_area->m_Netname ) );
                                str += wxT( "Therefore, these areas can't be combined." );
                                wxMessageBox( str );
                            }
                        }
                    }
                }
            }
        }

        if( mod_ia1 )
            ia1--;     // if modified, we need to check it again
    }

    return 0;
}


/**
 * Function TestAreaIntersections
 * Check for intersection of a given copper area with other areas in same net
 * @param area_to_test = area to compare to all other areas in the same net
 */
bool BOARD::TestAreaIntersections( ZONE_CONTAINER* area_to_test )
{
    CPolyLine* poly1 = area_to_test->m_Poly;

    for( unsigned ia2 = 0; ia2 < m_ZoneDescriptorList.size(); ia2++ )
    {
        ZONE_CONTAINER* area2 = m_ZoneDescriptorList[ia2];

        if( area_to_test->GetNet() != area2->GetNet() )
            continue;

        if( area_to_test == area2 )
            continue;

        // see if areas are on same layer
        if( area_to_test->GetLayer() != area2->GetLayer() )
            continue;

        if( area_to_test->GetPriority() != area2->GetPriority() )
            continue;

        CPolyLine* poly2 = area2->m_Poly;

        // test bounding rects
        CRect      b1 = poly1->GetCornerBounds();
        CRect      b2 = poly2->GetCornerBounds();

        if(  b1.bottom > b2.top
          || b1.top < b2.bottom
          || b1.left > b2.right
          || b1.right < b2.left )
            continue;

        // test for intersecting segments
        for( int icont1 = 0; icont1<poly1->GetNumContours(); icont1++ )
        {
            int is1 = poly1->GetContourStart( icont1 );
            int ie1 = poly1->GetContourEnd( icont1 );

            for( int ic1 = is1; ic1<=ie1; ic1++ )
            {
                int xi1 = poly1->GetX( ic1 );
                int yi1 = poly1->GetY( ic1 );
                int xf1, yf1, style1;

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

                style1 = poly1->GetSideStyle( ic1 );

                for( int icont2 = 0; icont2 < poly2->GetNumContours(); icont2++ )
                {
                    int is2 = poly2->GetContourStart( icont2 );
                    int ie2 = poly2->GetContourEnd( icont2 );

                    for( int ic2 = is2; ic2<=ie2; ic2++ )
                    {
                        int xi2 = poly2->GetX( ic2 );
                        int yi2 = poly2->GetY( ic2 );
                        int xf2, yf2, style2;

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

                        style2 = poly2->GetSideStyle( ic2 );
                        int n_int = FindSegmentIntersections( xi1, yi1, xf1, yf1, style1,
                                                              xi2, yi2, xf2, yf2, style2 );
                        if( n_int )
                            return true;
                    }
                }
            }
        }

        // If a contour is inside an other contour, no segments intersects, but the zones can
        // be combined test a corner inside an outline (only one corner is enought)
        for( int ic2 = 0; ic2 < poly2->GetNumCorners(); ic2++ )
        {
            int x = poly2->GetX( ic2 );
            int y = poly2->GetY( ic2 );

            if( poly1->TestPointInside( x, y ) )
            {
                return true;
            }
        }

        for( int ic1 = 0; ic1 < poly1->GetNumCorners(); ic1++ )
        {
            int x = poly1->GetX( ic1 );
            int y = poly1->GetY( ic1 );

            if( poly2->TestPointInside( x, y ) )
            {
                return true;
            }
        }
    }

    return false;
}


/**
 * Function TestAreaIntersection
 * Test for intersection of 2 copper areas
 * area_to_test must be after area_ref in m_ZoneDescriptorList
 * @param area_ref = area reference
 * @param area_to_test = area to compare for intersection calculations
 * @return : 0 if no intersection
 *         1 if intersection
 *         2 if arcs intersect
 */
int BOARD::TestAreaIntersection( ZONE_CONTAINER* area_ref, ZONE_CONTAINER* area_to_test )
{
    // see if areas are on same layer
    if( area_ref->GetLayer() != area_to_test->GetLayer() )
        return 0;

    CPolyLine* poly1 = area_ref->m_Poly;
    CPolyLine* poly2 = area_to_test->m_Poly;

    // test bounding rects
    CRect      b1 = poly1->GetCornerBounds();
    CRect      b2 = poly2->GetCornerBounds();

    if(  b1.bottom > b2.top
      || b1.top < b2.bottom
      || b1.left > b2.right
      || b1.right < b2.left )
        return 0;

    // now test for intersecting segments
    bool bInt    = false;
    bool bArcInt = false;

    for( int icont1 = 0; icont1<poly1->GetNumContours(); icont1++ )
    {
        int is1 = poly1->GetContourStart( icont1 );
        int ie1 = poly1->GetContourEnd( icont1 );

        for( int ic1 = is1; ic1<=ie1; ic1++ )
        {
            int xi1 = poly1->GetX( ic1 );
            int yi1 = poly1->GetY( ic1 );
            int xf1, yf1, style1;

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

            style1 = poly1->GetSideStyle( ic1 );

            for( int icont2 = 0; icont2<poly2->GetNumContours(); icont2++ )
            {
                int is2 = poly2->GetContourStart( icont2 );
                int ie2 = poly2->GetContourEnd( icont2 );

                for( int ic2 = is2; ic2<=ie2; ic2++ )
                {
                    int xi2 = poly2->GetX( ic2 );
                    int yi2 = poly2->GetY( ic2 );
                    int xf2, yf2, style2;

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

                    style2 = poly2->GetSideStyle( ic2 );
                    int n_int = FindSegmentIntersections( xi1, yi1, xf1, yf1, style1,
                                                          xi2, yi2, xf2, yf2, style2 );
                    if( n_int )
                    {
                        bInt = true;

                        if( style1 != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
                            bArcInt = true;

                        break;
                    }
                }

                if( bArcInt )
                    break;
            }

            if( bArcInt )
                break;
        }

        if( bArcInt )
            break;
    }

    if( !bInt )
    {
        if( bArcInt )
            return 0;

        // If a contour is inside an other contour, no segments intersects, but the zones
        // can be combined test a corner inside an outline (only one corner is enought)
        for( int ic2 = 0; ic2 < poly2->GetNumCorners(); ic2++ )
        {
            int x = poly2->GetX( ic2 );
            int y = poly2->GetY( ic2 );

            if( poly1->TestPointInside( x, y ) )
            {
                return 1;
            }
        }

        for( int ic1 = 0; ic1 < poly1->GetNumCorners(); ic1++ )
        {
            int x = poly1->GetX( ic1 );
            int y = poly1->GetY( ic1 );

            if( poly2->TestPointInside( x, y ) )
            {
                return 1;
            }
        }

        return 0;
    }

    if( bArcInt )
        return 2;

    return 1;
}


/**
 * Function CombineAreas
 * If possible, combine 2 copper areas
 * @param aDeletedList = a PICKED_ITEMS_LIST * where to store deleted areas (useful in undo
 *                       commands can be NULL
 * @param area_ref = tje main area (zone)
 * @param area_to_combine = the zone that can be merged with area_ref
 * area_ref must be BEFORE area_to_combine
 * area_to_combine will be deleted, if areas are combined
 * @return : 0 if no intersection
 *         1 if intersection
 *         2 if arcs intersect
 */
int BOARD::CombineAreas( PICKED_ITEMS_LIST* aDeletedList, ZONE_CONTAINER* area_ref,
                         ZONE_CONTAINER* area_to_combine )
{
    if( area_ref == area_to_combine )
    {
        wxASSERT( 0 );
    }

    // polygons intersect, combine them
    std::vector<CArc> arc_array1;
    std::vector<CArc> arc_array2;
    bool keep_area_to_combine = false;

    Bool_Engine*      booleng = new Bool_Engine();
    ArmBoolEng( booleng );

    area_ref->m_Poly->AddPolygonsToBoolEng( booleng, GROUP_A, -1, -1 );
    area_to_combine->m_Poly->AddPolygonsToBoolEng( booleng, GROUP_B, -1, -1 );
    booleng->Do_Operation( BOOL_OR );

    // create area with external contour: Recreate only area edges, NOT holes
    if( booleng->StartPolygonGet() )
    {
        if( booleng->GetPolygonPointEdgeType() == KB_INSIDE_EDGE )
        {
            DisplayError( NULL, wxT( "BOARD::CombineAreas() error: unexpected hole descriptor" ) );
        }

        area_ref->m_Poly->RemoveAllContours();

        // foreach point in the polygon
        bool first = true;

        while( booleng->PolygonHasMorePoints() )
        {
            int x = (int) booleng->GetPolygonXPoint();
            int y = (int) booleng->GetPolygonYPoint();

            if( first )
            {
                first = false;
                area_ref->m_Poly->Start( area_ref->GetLayer(
                                            ), x, y, area_ref->m_Poly->GetHatchStyle() );
            }
            else
            {
                area_ref->m_Poly->AppendCorner( x, y );
            }
        }

        booleng->EndPolygonGet();
        area_ref->m_Poly->Close();
    }

    // Recreate the area_to_combine if a second polygon exists
    // if not exists , the first poly contains the 2 initial polygons
#if 0   // TestAreaIntersection must be called before combine areas, so
        // 2 intersecting areas are expected, and only one outline contour after combining areas
    else
    {
        area_to_combine->m_Poly->RemoveAllContours();
        keep_area_to_combine = true;

        // create area with external contour: Recreate only area edges, NOT holes (todo..)
        {
            // foreach point in the polygon
            bool first = true;
            while( booleng->PolygonHasMorePoints() )
            {
                int x = booleng->GetPolygonXPoint();
                int y = booleng->GetPolygonYPoint();

                if( first )
                {
                    first = false;
                    area_to_combine->m_Poly->Start( area_ref->GetLayer(), x, y,
                                                    area_ref->m_Poly->GetHatchStyle() );
                }
                else
                {
                    area_to_combine->m_Poly->AppendCorner( x, y );
                }
            }

            booleng->EndPolygonGet();
            area_to_combine->m_Poly->Close();
        }
    }
#endif

    // add holes
    bool show_error = true;

    while( booleng->StartPolygonGet() )
    {
        // we expect all vertex are holes inside the main outline
        if( booleng->GetPolygonPointEdgeType() != KB_INSIDE_EDGE )
        {
            if( show_error )    // show this error only once, if happens
                DisplayError( NULL,
                              wxT( "BOARD::CombineAreas() error: unexpected outside contour descriptor" ) );

            show_error = false;
            continue;
        }

        while( booleng->PolygonHasMorePoints() )
        {
            int x = (int) booleng->GetPolygonXPoint();
            int y = (int) booleng->GetPolygonYPoint();
            area_ref->m_Poly->AppendCorner( x, y );
        }

        area_ref->m_Poly->Close();
        booleng->EndPolygonGet();
    }

    if( !keep_area_to_combine )
        RemoveArea( aDeletedList, area_to_combine );

    area_ref->utility = 1;
    area_ref->m_Poly->RestoreArcs( &arc_array1 );
    area_ref->m_Poly->RestoreArcs( &arc_array2 );
    area_ref->m_Poly->Hatch();
    delete booleng;
    return 1;
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

        // If testing only a single area, then skip all others
        if( aArea_To_Examine && (aArea_To_Examine != Area_Ref) )
            continue;

        for( int ia2 = 0; ia2 < GetAreaCount(); ia2++ )
        {
            ZONE_CONTAINER* Area_To_Test = GetArea( ia2 );
            CPolyLine*      testSmoothedPoly = Area_To_Test->GetSmoothedPoly();

            if( Area_Ref == Area_To_Test )
                continue;

            // test for same layer
            if( Area_Ref->GetLayer() != Area_To_Test->GetLayer() )
                continue;

            // Test for same net
            if( Area_Ref->GetNet() == Area_To_Test->GetNet() && Area_Ref->GetNet() >= 0 )
                continue;

            // test for different priorities
            if( Area_Ref->GetPriority() != Area_To_Test->GetPriority() )
                continue;

            // Examine a candidate zone: compare Area_To_Test to Area_Ref

            // Get clearance used in zone to zone test.  The policy used to
            // obtain that value is now part of the zone object itself by way of
            // ZONE_CONTAINER::GetClearance().
            int zone2zoneClearance = Area_Ref->GetClearance( Area_To_Test );

            // test for some corners of Area_Ref inside Area_To_Test
            for( int ic = 0; ic < refSmoothedPoly->GetNumCorners(); ic++ )
            {
                int x = refSmoothedPoly->GetX( ic );
                int y = refSmoothedPoly->GetY( ic );

                if( testSmoothedPoly->TestPointInside( x, y ) )
                {
                    // COPPERAREA_COPPERAREA error: copper area ref corner inside copper area
                    if( aCreate_Markers )
                    {
                        wxString msg1   = Area_Ref->GetSelectMenuText();
                        wxString msg2   = Area_To_Test->GetSelectMenuText();
                        MARKER_PCB*  marker = new MARKER_PCB( COPPERAREA_INSIDE_COPPERAREA,
                                                              wxPoint( x, y ),
                                                              msg1, wxPoint( x, y ),
                                                              msg2, wxPoint( x, y ) );
                        Add( marker );
                    }

                    nerrors++;
                }
            }

            // test for some corners of Area_To_Test inside Area_Ref
            for( int ic2 = 0; ic2 < testSmoothedPoly->GetNumCorners(); ic2++ )
            {
                int x = testSmoothedPoly->GetX( ic2 );
                int y = testSmoothedPoly->GetY( ic2 );

                if( refSmoothedPoly->TestPointInside( x, y ) )
                {
                    // COPPERAREA_COPPERAREA error: copper area corner inside copper area ref
                    if( aCreate_Markers )
                    {
                        wxString msg1   = Area_To_Test->GetSelectMenuText();
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
            for( int icont = 0; icont < refSmoothedPoly->GetNumContours(); icont++ )
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

                    int astyle = refSmoothedPoly->GetSideStyle( ic );

                    for( int icont2 = 0; icont2 < testSmoothedPoly->GetNumContours(); icont2++ )
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

                            int bstyle = testSmoothedPoly->GetSideStyle( ic2 );
                            int x, y;

                            int d = GetClearanceBetweenSegments( bx1, by1, bx2, by2, bstyle,
                                                                 0,
                                                                 ax1, ay1, ax2,
                                                                 ay2, astyle,
                                                                 0,
                                                                 zone2zoneClearance,
                                                                 &x, &y );

                            if( d < zone2zoneClearance )
                            {
                                // COPPERAREA_COPPERAREA error : intersect or too close
                                if( aCreate_Markers )
                                {
                                    wxString msg1   = Area_Ref->GetSelectMenuText();
                                    wxString msg2   = Area_To_Test->GetSelectMenuText();
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


/**
 * Function doEdgeZoneDrc
 * tests a segment in ZONE_CONTAINER * aArea:
 *      Test Edge inside other areas
 *      Test Edge too close other areas
 * @param aArea The current area.
 * @param aCornerIndex The first corner of the segment to test.
 * @return bool - false if DRC error  or true if OK
 */

bool DRC::doEdgeZoneDrc( ZONE_CONTAINER* aArea, int aCornerIndex )
{
    if( !aArea->IsOnCopperLayer() )    // Cannot have a Drc error if not on copper layer
        return true;

    wxString str;

    wxPoint  start = aArea->GetCornerPosition( aCornerIndex );
    wxPoint  end;

    // Search the end point of the edge starting at aCornerIndex
    if( aArea->m_Poly->corner[aCornerIndex].end_contour == false
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
            if( aArea->m_Poly->corner[ii].end_contour )
                break;

            end = aArea->GetCornerPosition( ii );
            ii--;
        }
    }

    // iterate through all areas
    for( int ia2 = 0; ia2 < m_pcb->GetAreaCount(); ia2++ )
    {
        ZONE_CONTAINER* Area_To_Test   = m_pcb->GetArea( ia2 );
        int             zone_clearance = max( Area_To_Test->m_ZoneClearance,
                                              aArea->m_ZoneClearance );

        // test for same layer
        if( Area_To_Test->GetLayer() != aArea->GetLayer() )
            continue;

        // Test for same net
        if( ( aArea->GetNet() == Area_To_Test->GetNet() ) && (aArea->GetNet() >= 0) )
            continue;

        // test for same priority
        if( Area_To_Test->GetPriority() != aArea->GetPriority() )
            continue;

        // test for ending line inside Area_To_Test
        int x = end.x;
        int y = end.y;

        if( Area_To_Test->m_Poly->TestPointInside( x, y ) )
        {
            // COPPERAREA_COPPERAREA error: corner inside copper area
            m_currentMarker = fillMarker( aArea, wxPoint( x, y ),
                                          COPPERAREA_INSIDE_COPPERAREA,
                                          m_currentMarker );
            return false;
        }

        // now test spacing between areas
        int astyle = CPolyLine::STRAIGHT;
        int ax1    = start.x;
        int ay1    = start.y;
        int ax2    = end.x;
        int ay2    = end.y;

        for( int icont2 = 0; icont2 < Area_To_Test->m_Poly->GetNumContours(); icont2++ )
        {
            int ic_start2 = Area_To_Test->m_Poly->GetContourStart( icont2 );
            int ic_end2   = Area_To_Test->m_Poly->GetContourEnd( icont2 );

            for( int ic2 = ic_start2; ic2<=ic_end2; ic2++ )
            {
                int bx1 = Area_To_Test->m_Poly->GetX( ic2 );
                int by1 = Area_To_Test->m_Poly->GetY( ic2 );
                int bx2, by2;

                if( ic2 == ic_end2 )
                {
                    bx2 = Area_To_Test->m_Poly->GetX( ic_start2 );
                    by2 = Area_To_Test->m_Poly->GetY( ic_start2 );
                }
                else
                {
                    bx2 = Area_To_Test->m_Poly->GetX( ic2 + 1 );
                    by2 = Area_To_Test->m_Poly->GetY( ic2 + 1 );
                }

                int bstyle = Area_To_Test->m_Poly->GetSideStyle( ic2 );
                int x, y;
                int d = GetClearanceBetweenSegments( bx1, by1, bx2, by2, bstyle,
                                                     0,
                                                     ax1, ay1, ax2, ay2, astyle,
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
