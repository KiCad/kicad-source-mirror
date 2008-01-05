/****************************************************************************/
/* functions to test, merges and cut polygons used as copper areas outlines */
/****************************************************************************/

#include <vector>

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"

using namespace std;

#undef ASSERT
#define ASSERT wxASSERT

bool bDontShowSelfIntersectionArcsWarning;
bool bDontShowSelfIntersectionWarning;
bool bDontShowIntersectionArcsWarning;
bool bDontShowIntersectionWarning;


/**
 * Function AddArea
 * add empty copper area to net
 * @return pointer to the new area
 */
ZONE_CONTAINER* BOARD::AddArea( int netcode, int layer, int x, int y, int hatch )
{
    ZONE_CONTAINER* new_area = InsertArea( netcode, m_ZoneDescriptorList.size(
                                               ) - 1, layer, x, y, hatch );

    return new_area;
}


/**
 * remove copper area from net
 * @param  area = area to remove
 * @return 0
 */
int BOARD::RemoveArea( ZONE_CONTAINER* area_to_remove )
{
    Delete( area_to_remove );
    return 0;
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
    new_area->m_TimeStamp = GetTimeStamp();
    if( iarea < (int) ( m_ZoneDescriptorList.size() - 1) )
        m_ZoneDescriptorList.insert( m_ZoneDescriptorList.begin() + iarea + 1, new_area );
    else
        m_ZoneDescriptorList.push_back( new_area );

    new_area->m_Poly->Start( layer, 1, 10 * NM_PER_MIL, x, y,
                           hatch );
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
        RemoveArea( area_to_complete );
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
                            int ret    = FindSegmentIntersections( x1i,
                                                                   y1i,
                                                                   x1f,
                                                                   y1f,
                                                                   style,
                                                                   x2i,
                                                                   y2i,
                                                                   x2f,
                                                                   y2f,
                                                                   style2 );
                            if( ret )
                            {
                                // intersection between non-adjacent sides
                                bInt = TRUE;
                                if( style != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
                                {
                                    bArcInt = TRUE;
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
 * @param bMessageBoxInt == TRUE, shows message when clipping occurs.
 * @param  bMessageBoxArc == TRUE, shows message when clipping can't be done due to arcs.
 * @return:
 *	-1 if arcs intersect other sides, so polygon can't be clipped
 *	 0 if no intersecting sides
 *	 1 if intersecting sides
 * Also sets areas->utility1 flags if areas are modified
 */
int BOARD::ClipAreaPolygon( ZONE_CONTAINER* CurrArea,
                            bool bMessageBoxArc, bool bMessageBoxInt, bool bRetainArcs )
{
    CPolyLine* p    = CurrArea->m_Poly;
    int        test = TestAreaPolygon( CurrArea ); // this sets utility2 flag

    if( test == -1 && !bRetainArcs )
        test = 1;
    if( test == -1 )
    {
        // arc intersections, don't clip unless bRetainArcs == false
        if( bMessageBoxArc && bDontShowSelfIntersectionArcsWarning == false )
        {
            wxString str;
            str.Printf( wxT( "Area %X of net \"%s\" has arcs intersecting other sides.\n" ),
                       CurrArea->m_TimeStamp, CurrArea->m_Netname.GetData() );
            str += wxT( "This may cause problems with other editing operations,\n" );
            str += wxT( "such as adding cutouts. It can't be fixed automatically.\n" );
            str += wxT( "Manual correction is recommended." );
            wxMessageBox( str );

//            bDontShowSelfIntersectionArcsWarning = dlg.bDontShowBoxState;
        }
        return -1;  // arcs intersect with other sides, error
    }

    // mark all areas as unmodified except this one
    for( unsigned ia = 0; ia < m_ZoneDescriptorList.size(); ia++ )
        m_ZoneDescriptorList[ia]->utility = 0;

    CurrArea->utility = 1;

    if( test == 1 )
    {
        // non-arc intersections, clip the polygon
        if( bMessageBoxInt && bDontShowSelfIntersectionWarning == false )
        {
            wxString str;
            str.Printf( wxT( "Area %d of net \"%s\" is self-intersecting and will be clipped.\n" ),
                       CurrArea->m_TimeStamp, CurrArea->m_Netname.GetData() );
            str += wxT( "This may result in splitting the area.\n" );
            str += wxT( "If the area is complex, this may take a few seconds." );
            wxMessageBox( str );

//			bDontShowSelfIntersectionWarning = dlg.bDontShowBoxState;
        }
    }

//** TODO test for cutouts outside of area
//**	if( test == 1 )
    {
        std::vector<CPolyLine*> * pa = new std::vector<CPolyLine*>;
        p->Undraw();
        int n_poly = CurrArea->m_Poly->NormalizeWithGpc( pa, bRetainArcs );
        if( n_poly > 1 )    // i.e if clippinf has created some polygons, we must add these new copper areas
        {
            for( int ip = 1; ip < n_poly; ip++ )
            {
                // create new copper area and copy poly into it
                CPolyLine* new_p = (*pa)[ip - 1];
                CurrArea = AddArea( CurrArea->GetNet(), CurrArea->GetLayer(), 0, 0, 0 );

                // remove the poly that was automatically created for the new area
                // and replace it with a poly from NormalizeWithGpc
                delete CurrArea->m_Poly;
                CurrArea->m_Poly = new_p;
                CurrArea->m_Poly->Draw();
                CurrArea->utility = 1;
            }
        }
        p->Draw();
        delete pa;
    }
    return test;
}


/**
 * Process an area that has been modified, by clipping its polygon against
 * itself and the polygons for any other areas on the same net.
 * This may change the number and order of copper areas in the net.
 * @param modified_area = area to test
 * @param bMessageBox : if TRUE, shows message boxes when clipping occurs.
 * @return :
 * -1 if arcs intersect other sides, so polygon can't be clipped
 *  0 if no intersecting sides
 *  1 if intersecting sides, polygon clipped
 */
int BOARD::AreaPolygonModified( ZONE_CONTAINER* modified_area,
                                bool            bMessageBoxArc,
                                bool            bMessageBoxInt )
{
    // clip polygon against itself
    int test = ClipAreaPolygon( modified_area, bMessageBoxArc, bMessageBoxInt );

    if( test == -1 )
        return test;

    // now see if we need to clip against other areas
    bool bCheckAllAreas = false;
    if( test == 1 )
        bCheckAllAreas = TRUE;
    else
        bCheckAllAreas = TestAreaIntersections( modified_area );
    if( bCheckAllAreas )
        CombineAllAreasInNet( modified_area->GetNet(), bMessageBoxInt, TRUE );

    return test;
}


/**
 * Function CombineAllAreasInNet
 * Checks all copper areas in net for intersections, combining them if found
 * @param aNetCode = net to consider
 * @param bMessageBox : if true display warning message box
 * @param bUseUtility : if true, don't check areas if both utility flags are 0
 * Sets utility flag = 1 for any areas modified
 * If an area has self-intersecting arcs, doesn't try to combine it
 */
int BOARD::CombineAllAreasInNet( int aNetCode, bool bMessageBox, bool bUseUtility )
{
    if( m_ZoneDescriptorList.size() > 1 )
    {
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
            CRect b1      = curr_area->m_Poly->GetCornerBounds();
            bool  mod_ia1 = false;
            for( unsigned ia2 = m_ZoneDescriptorList.size() - 1; ia2 > ia1; ia2-- )
            {
                ZONE_CONTAINER* area2 = m_ZoneDescriptorList[ia2];
                if( curr_area->GetLayer() == area2->GetLayer()
                    && curr_area->utility2 != -1 && area2->utility2 != -1 )
                {
                    CRect b2 = area2->m_Poly->GetCornerBounds();
                    if( !( b1.left > b2.right || b1.right < b2.left
                           || b1.bottom > b2.top || b1.top < b2.bottom ) )
                    {
                        // check ia2 against 1a1
                        if( curr_area->utility || area2->utility || bUseUtility ==
                            false )
                        {
                            int ret = TestAreaIntersection( curr_area, area2 );
                            if( ret == 1 )
                                ret = CombineAreas( curr_area, area2 );
                            if( ret == 1 )
                            {
                                if( bMessageBox && bDontShowIntersectionWarning == false )
                                {
                                    wxString str;
                                    str.Printf(
                                        wxT(
                                            "Areas %d and %d of net \"%s\" intersect and will be combined.\n" ),
                                        ia1 + 1,
                                        ia2 + 1,
                                        curr_area->m_Netname.GetData() );
                                    str += wxT(
                                        "If they are complex, this may take a few seconds." );
                                    wxMessageBox( str );

//                                    bDontShowIntersectionWarning = dlg.bDontShowBoxState;
                                }
                                mod_ia1 = TRUE;
                            }
                            else if( ret == 2 )
                            {
                                if( bMessageBox && bDontShowIntersectionArcsWarning == false )
                                {
                                    wxString str;
                                    str.Printf(
                                        wxT(
                                            "Areas %d and %d of net \"%s\" intersect, but some of the intersecting sides are arcs.\n" ),
                                        ia1 + 1,
                                        ia2 + 1,
                                        curr_area->m_Netname.GetData() );
                                    str += wxT( "Therefore, these areas can't be combined." );
                                    wxMessageBox( str );

//                                    bDontShowIntersectionArcsWarning = dlg.bDontShowBoxState;
                                }
                            }
                        }
                    }
                }
            }

            if( mod_ia1 )
                ia1--; // if modified, we need to check it again
        }
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
        if( area_to_test->GetNet() != area_to_test->GetNet() )
            continue;
        if( area_to_test != area2 )
        {
            // see if areas are on same layer
            if( area_to_test->GetLayer() != area2->GetLayer() )
                continue;

            CPolyLine* poly2 = area2->m_Poly;

            // test bounding rects
            CRect      b1 = poly1->GetCornerBounds();
            CRect      b2 = poly2->GetCornerBounds();
            if(    b1.bottom > b2.top
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
                                return TRUE;
                        }
                    }
                }
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
    if(    b1.bottom > b2.top
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
                        bInt = TRUE;
                        if( style1 != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
                            bArcInt = TRUE;
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
        return 0;
    if( bArcInt )
        return 2;
    return 1;
}


/**
 * Function CombineAreas
 * If possible, combine 2 copper areas
 * area_ref must be BEFORE area_to_combine in m_ZoneDescriptorList
 * area_to_combine will be deleted, if areas are combined
 * @return : 0 if no intersection
 *         1 if intersection
 *         2 if arcs intersect
 */
int BOARD::CombineAreas( ZONE_CONTAINER* area_ref, ZONE_CONTAINER* area_to_combine )
{
    if( area_ref == area_to_combine )
        ASSERT( 0 );
#if 0

    // test for intersection
    int test = TestAreaIntersection( area_ref, area_to_combine );
    if( test != 1 )
        return test; // no intersection
#endif

    // polygons intersect, combine them
    CPolyLine*        poly1 = area_ref->m_Poly;
    CPolyLine*        poly2 = area_to_combine->m_Poly;
    std::vector<CArc> arc_array1;
    std::vector<CArc> arc_array2;
    poly1->MakeGpcPoly( -1, &arc_array1 );
    poly2->MakeGpcPoly( -1, &arc_array2 );
    int n_ext_cont1 = 0;
    for( int ic = 0; ic<poly1->GetGpcPoly()->num_contours; ic++ )
        if( !( (poly1->GetGpcPoly()->hole)[ic] ) )
            n_ext_cont1++;

    int n_ext_cont2 = 0;
    for( int ic = 0; ic<poly2->GetGpcPoly()->num_contours; ic++ )
        if( !( (poly2->GetGpcPoly()->hole)[ic] ) )
            n_ext_cont2++;

    gpc_polygon* union_gpc = new gpc_polygon;
    gpc_polygon_clip( GPC_UNION, poly1->GetGpcPoly(), poly2->GetGpcPoly(), union_gpc );

    // get number of outside contours
    int n_union_ext_cont = 0;
    for( int ic = 0; ic<union_gpc->num_contours; ic++ )
        if( !( (union_gpc->hole)[ic] ) )
            n_union_ext_cont++;

    // if no intersection, free new gpc and return
    if( n_union_ext_cont == n_ext_cont1 + n_ext_cont2 )
    {
        gpc_free_polygon( union_gpc );
        delete union_gpc;
        return 0;
    }

    // intersection, replace area_ref->m_Poly with combined areas and remove area_to_combine
    RemoveArea( area_to_combine );
    area_ref->m_Poly->RemoveAllContours();

    // create area with external contour
    for( int ic = 0; ic<union_gpc->num_contours; ic++ )
    {
        if( !(union_gpc->hole)[ic] )
        {
            // external contour, replace this poly
            for( int i = 0; i<union_gpc->contour[ic].num_vertices; i++ )
            {
                int x = ( (union_gpc->contour)[ic].vertex )[i].x;
                int y = ( (union_gpc->contour)[ic].vertex )[i].y;
                if( i==0 )
                {
                    area_ref->m_Poly->Start( area_ref->GetLayer(), 0, 0, x, y, area_ref->m_Poly->GetHatchStyle() );
                }
                else
                    area_ref->m_Poly->AppendCorner( x, y );
            }

            area_ref->m_Poly->Close();
        }
    }

    // add holes
    for( int ic = 0; ic<union_gpc->num_contours; ic++ )
    {
        if( (union_gpc->hole)[ic] )
        {
            // hole
            for( int i = 0; i<union_gpc->contour[ic].num_vertices; i++ )
            {
                int x = ( (union_gpc->contour)[ic].vertex )[i].x;
                int y = ( (union_gpc->contour)[ic].vertex )[i].y;
                area_ref->m_Poly->AppendCorner( x, y );
            }

            area_ref->m_Poly->Close();
        }
    }

    area_ref->utility = 1;
    area_ref->m_Poly->RestoreArcs( &arc_array1 );
    area_ref->m_Poly->RestoreArcs( &arc_array2 );
    area_ref->m_Poly->Draw();
    gpc_free_polygon( union_gpc );
    delete union_gpc;
    return 1;
}


#if 0
void dra_areas( CDlgLog* log, int copper_layers,
                int units, BOOL check_unrouted,
                CArray<CPolyLine> * board_outline,
                DesignRules* dr, DRErrorList* drelist )
{
    CString d_str, x_str, y_str;
    CString str;
    CString str2;
    long    nerrors = 0;

    // now iterate through all areas
    for( int ia = 0; ia<net->nareas; ia++ )
    {
        ZONE_CONTAINER*   a = &net->area[ia];

        // iterate through all nets again
        POSITION pos2 = pos;
        void*    ptr2;
        CString  name2;
        while( pos2 != NULL )
        {
            m_nlist->m_map.GetNextAssoc( pos2, name2, ptr2 );
            cnet* net2 = (cnet*) ptr2;
            for( int ia2 = 0; ia2<net2->nareas; ia2++ )
            {
                ZONE_CONTAINER* a2 = &net2->area[ia2];

                // test for same layer
                if( a->m_Poly->GetLayer() == a2->m_Poly->GetLayer() )
                {
                    // test for points inside one another
                    for( int ic = 0; ic<a->m_Poly->GetNumCorners(); ic++ )
                    {
                        int x = a->m_Poly->GetX( ic );
                        int y = a->m_Poly->GetY( ic );
                        if( a2->m_Poly->TestPointInside( x, y ) )
                        {
                            // COPPERAREA_COPPERAREA error
                            id id_a = net->id;
                            id_a.st  = ID_AREA;
                            id_a.i   = ia;
                            id_a.sst = ID_SEL_CORNER;
                            id_a.ii  = ic;
                            str.Format(
                                "%ld: \"%s\" copper area inside \"%s\" inside copper area\r\n",
                                nerrors + 1,
                                net->name,
                                net2->name );
                            DRError* dre = drelist->Add( nerrors,
                                                         DRError::COPPERAREA_INSIDE_COPPERAREA,
                                                         &str,
                                                         &net->name,
                                                         &net2->name,
                                                         id_a,
                                                         id_a,
                                                         x,
                                                         y,
                                                         x,
                                                         y,
                                                         0,
                                                         0 );
                            if( dre )
                            {
                                nerrors++;
                                if( log )
                                    log->AddLine( str );
                            }
                        }
                    }

                    for( int ic2 = 0; ic2<a2->m_Poly->GetNumCorners(); ic2++ )
                    {
                        int x = a2->m_Poly->GetX( ic2 );
                        int y = a2->m_Poly->GetY( ic2 );
                        if( a->m_Poly->TestPointInside( x, y ) )
                        {
                            // COPPERAREA_COPPERAREA error
                            id id_a = net2->id;
                            id_a.st  = ID_AREA;
                            id_a.i   = ia2;
                            id_a.sst = ID_SEL_CORNER;
                            id_a.ii  = ic2;
                            str.Format( "%ld: \"%s\" copper area inside \"%s\" copper area\r\n",
                                        nerrors + 1, net2->name, net->name );
                            DRError* dre = drelist->Add( nerrors,
                                                         DRError::COPPERAREA_INSIDE_COPPERAREA,
                                                         &str,
                                                         &net2->name,
                                                         &net->name,
                                                         id_a,
                                                         id_a,
                                                         x,
                                                         y,
                                                         x,
                                                         y,
                                                         0,
                                                         0 );
                            if( dre )
                            {
                                nerrors++;
                                if( log )
                                    log->AddLine( str );
                            }
                        }
                    }

                    // now test spacing between areas
                    for( int icont = 0; icont<a->m_Poly->GetNumContours(); icont++ )
                    {
                        int ic_start = a->m_Poly->GetContourStart( icont );
                        int ic_end   = a->m_Poly->GetContourEnd( icont );
                        for( int ic = ic_start; ic<=ic_end; ic++ )
                        {
                            id id_a = net->id;
                            id_a.st  = ID_AREA;
                            id_a.i   = ia;
                            id_a.sst = ID_SIDE;
                            id_a.ii  = ic;
                            int ax1 = a->m_Poly->GetX( ic );
                            int ay1 = a->m_Poly->GetY( ic );
                            int ax2, ay2;
                            if( ic == ic_end )
                            {
                                ax2 = a->m_Poly->GetX( ic_start );
                                ay2 = a->m_Poly->GetY( ic_start );
                            }
                            else
                            {
                                ax2 = a->m_Poly->GetX( ic + 1 );
                                ay2 = a->m_Poly->GetY( ic + 1 );
                            }
                            int astyle = a->m_Poly->GetSideStyle( ic );
                            for( int icont2 = 0; icont2<a2->m_Poly->GetNumContours(); icont2++ )
                            {
                                int ic_start2 = a2->m_Poly->GetContourStart( icont2 );
                                int ic_end2   = a2->m_Poly->GetContourEnd( icont2 );
                                for( int ic2 = ic_start2; ic2<=ic_end2; ic2++ )
                                {
                                    id id_b = net2->id;
                                    id_b.st  = ID_AREA;
                                    id_b.i   = ia2;
                                    id_b.sst = ID_SIDE;
                                    id_b.ii  = ic2;
                                    int bx1 = a2->m_Poly->GetX( ic2 );
                                    int by1 = a2->m_Poly->GetY( ic2 );
                                    int bx2, by2;
                                    if( ic2 == ic_end2 )
                                    {
                                        bx2 = a2->m_Poly->GetX( ic_start2 );
                                        by2 = a2->m_Poly->GetY( ic_start2 );
                                    }
                                    else
                                    {
                                        bx2 = a2->m_Poly->GetX( ic2 + 1 );
                                        by2 = a2->m_Poly->GetY( ic2 + 1 );
                                    }
                                    int bstyle = a2->m_Poly->GetSideStyle( ic2 );
                                    int x, y;
                                    int d = ::GetClearanceBetweenSegments( bx1,
                                                                           by1,
                                                                           bx2,
                                                                           by2,
                                                                           bstyle,
                                                                           0,
                                                                           ax1,
                                                                           ay1,
                                                                           ax2,
                                                                           ay2,
                                                                           astyle,
                                                                           0,
                                                                           dr->copper_copper,
                                                                           &x,
                                                                           &y );
                                    if( d < dr->copper_copper )
                                    {
                                        // COPPERAREA_COPPERAREA error
                                        ::MakeCStringFromDimension( &d_str,
                                                                    d,
                                                                    units,
                                                                    TRUE,
                                                                    TRUE,
                                                                    TRUE,
                                                                    1 );
                                        ::MakeCStringFromDimension( &x_str,
                                                                    x,
                                                                    units,
                                                                    FALSE,
                                                                    TRUE,
                                                                    TRUE,
                                                                    1 );
                                        ::MakeCStringFromDimension( &y_str,
                                                                    y,
                                                                    units,
                                                                    FALSE,
                                                                    TRUE,
                                                                    TRUE,
                                                                    1 );
                                        str.Format(
                                            "%ld: \"%s\" copper area to \"%s\" copper area = %s, x=%s, y=%s\r\n",
                                            nerrors + 1,
                                            net->name,
                                            net2->name,
                                            d_str,
                                            x_str,
                                            y_str );
                                        DRError* dre = drelist->Add(
                                            nerrors,
                                            DRError::
                                            COPPERAREA_COPPERAREA,
                                            &str,
                                            &net->name,
                                            &net2->name,
                                            id_a,
                                            id_b,
                                            x,
                                            y,
                                            x,
                                            y,
                                            0,
                                            0 );
                                        if( dre )
                                        {
                                            nerrors++;
                                            if( log )
                                                log->AddLine( str );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}


#endif
