/*******************************************/
/* zones_convert_brd_items_to_polygons.cpp */
/*******************************************/

/* Functions to convert some board items to polygons
 * (pads, tracks ..)
 * This is used to calculate filled areas in copper zones.
 * Filled areas are areas remainder of the full zone area after removed all polygons
 * calculated from these items shapes and the clearance area
 *
 * Important note:
 * Because filled areas must have a minimum thickness to match with Design rule, they are draw in 2 step:
 * 1 - filled polygons are drawn
 * 2 - polygon outlines are drawn with a "minimum thickness width" ( or with a minimum thickness pen )
 * So outlines of filled polygons are calculated with the constraint they match with clearance,
 * taking in account outlines have thickness
 * This ensures:
 *      - areas meet the minimum thickness requirement.
 *      - shapes are smoothed.
 */

#include <math.h>
#include <vector>


#include "fctsys.h"
#include "polygons_defs.h"

#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "trigo.h"

#include "zones.h"
#include "PolyLine.h"


extern void BuildUnconnectedThermalStubsPolygonList( std::vector<CPolyPt>& aCornerBuffer,
                                                     BOARD* aPcb, ZONE_CONTAINER* aZone,
                                                      double aArcCorrection,
                                                      int aRoundPadThermalRotation);
extern void Test_For_Copper_Island_And_Remove( BOARD*          aPcb,
                                               ZONE_CONTAINER* aZone_container );
extern void CreateThermalReliefPadPolygon( std::vector<CPolyPt>& aCornerBuffer,
                                           D_PAD&                aPad,
                                           int                   aThermalGap,
                                           int                   aCopperThickness,
                                           int                   aMinThicknessValue,
                                           int                   aCircleToSegmentsCount,
                                           double                aCorrectionFactor,
                                           int                   aThermalRot );

// Local Functions: helper function to calculate solid areas
static void AddPolygonCornersToKPolygonList( std::vector <CPolyPt>& aCornersBuffer,
                                             KPolygonSet&           aKPolyList );

static int  CopyPolygonsFromKPolygonListToFilledPolysList( ZONE_CONTAINER* aZone,
                                                           KPolygonSet&    aKPolyList );
static int  CopyPolygonsFromFilledPolysListTotKPolygonList( ZONE_CONTAINER* aZone,
                                                            KPolygonSet&    aKPolyList );


// Local Variables:
static int s_thermalRot = 450;  // angle of stubs in thermal reliefs for round pads

/* how many segments are used to create a polygon from a circle: */
static int s_CircleToSegmentsCount = ARC_APPROX_SEGMENTS_COUNT_LOW_DEF;   /* default value. the real value will be changed to
                                                                           * ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF
                                                                           * if m_ArcToSegmentsCount == ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF
                                                                           */
double s_Correction;     /* mult coeff used to enlarge rounded and oval pads (and vias)
                          * because the segment approximation for arcs and circles
                          * create a smaller gap than a true circle
                          */

/**
 * Function AddClearanceAreasPolygonsToPolysList
 * Supports a min thickness area constraint.
 * Add non copper areas polygons (pads and tracks with clearence)
 * to the filled copper area found
 * in BuildFilledPolysListData after calculating filled areas in a zone
 * Non filled copper areas are pads and track and their clearance areas
 * The filled copper area must be computed just before.
 * BuildFilledPolysListData() call this function just after creating the
 *  filled copper area polygon (without clearence areas
 * to do that this function:
 * 1 - Creates the main outline (zone outline) using a correction to shrink the resulting area
 *     with m_ZoneMinThickness/2 value.
 *     The result is areas with a margin of m_ZoneMinThickness/2
 *     When drawing outline with segments having a thickness of m_ZoneMinThickness, the outlines will
 *     match exactly the initial outlines
 * 3 - Add all non filled areas (pads, tracks) in group B with a clearance of m_Clearance + m_ZoneMinThickness/2
 *     in a buffer
 *   - If Thermal shapes are wanted, add non filled area, in order to create these thermal shapes
 * 4 - calculates the polygon A - B
 * 5 - put resulting list of polygons (filled areas) in m_FilledPolysList
 *     This zone contains pads with the same net.
 * 6 - Remove insulated copper islands
 * 7 - If Thermal shapes are wanted, remove unconnected stubs in thermal shapes:
 *      creates a buffer of polygons corresponding to stubs to remove
 *      sub them to the filled areas.
 *      Remove new insulated copper islands
 */
void ZONE_CONTAINER::AddClearanceAreasPolygonsToPolysList( BOARD* aPcb )
{
    // Set the number of segments in arc approximations
    if( m_ArcToSegmentsCount == ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF  )
        s_CircleToSegmentsCount = ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF;
    else
        s_CircleToSegmentsCount = ARC_APPROX_SEGMENTS_COUNT_LOW_DEF;

    /* calculates the coeff to compensate radius reduction of holes clearance
     * due to the segment approx.
     * For a circle the min radius is radius * cos( 2PI / s_CircleToSegmentsCount / 2)
     * s_Correction is 1 /cos( PI/s_CircleToSegmentsCount  )
     */
    s_Correction = 1.0 / cos( 3.14159265 / s_CircleToSegmentsCount );

    /* Uses a kbool engine to add holes in the m_FilledPolysList polygon.
     * Because this function is called just after creating the m_FilledPolysList,
     * only one polygon is in list.
     * (initial holes in zones are linked into outer contours by double overlapping segments).
     * because after adding holes, many polygons could be exist in this list.
     */

    // This polygon set is the area(s) to fill, with m_ZoneMinThickness/2
    KPolygonSet polyset_zone_solid_areas;
    int         margin = m_ZoneMinThickness / 2;

    /* First, creates the main polygon (i.e. the filled area using only one outline)
     * to reserve a m_ZoneMinThickness/2 margin around the outlines and holes
     * this margin is the room to redraw outlines with segments having a width set to
     * m_ZoneMinThickness
     * so m_ZoneMinThickness is the min thickness of the filled zones areas
     * the main polygon is stored in polyset_zone_solid_areas
     */

    CopyPolygonsFromFilledPolysListTotKPolygonList( this,
                                                    polyset_zone_solid_areas );
    polyset_zone_solid_areas -= margin;

    if( polyset_zone_solid_areas.size() == 0 )
        return;

    /* Calculates the clearance value that meet DRC requirements
     * from m_ZoneClearance and clearance from the corresponding netclass
     * We have a "local" clearance in zones because most of time
     * clearance between a zone and others items is bigger than the netclass clearance
     * this is more true for small clearance values
     * Note also the "local" clearance is used for clearance between non copper items
     *    or items like texts on copper layers
     */
    int zone_clearance = max( m_ZoneClearance, GetClearance() );
    zone_clearance += margin;

    /* store holes (i.e. tracks and pads areas as polygons outlines)
     * in a polygon list
     */

    /* items ouside the zone bounding box are skipped
     * the bounding box is the zone bounding box + the biggest clearance found in Netclass list
     */
    EDA_Rect item_boundingbox;
    EDA_Rect zone_boundingbox  = GetBoundingBox();
    int      biggest_clearance = aPcb->GetBiggestClearanceValue();
    biggest_clearance = MAX( biggest_clearance, zone_clearance );
    zone_boundingbox.Inflate( biggest_clearance );

    /*
     * First : Add pads. Note: pads having the same net as zone are left in zone.
     * Thermal shapes will be created later if necessary
     */
    int item_clearance;

    // static to avoid unnecessary memory allocation when filling many zones.
    static std::vector <CPolyPt> cornerBufferPolysToSubstract;
    cornerBufferPolysToSubstract.clear();

    /* Use a dummy pad to calculate hole clerance when a pad is not on all copper layers
     * and this pad has a hole
     * This dummy pad has the size and shape of the hole
    * Therefore, this dummy pad is a circle or an oval.
     * A pad must have a parent because some functions expect a non null parent
     * to find the parent board, and some other data
     */
    MODULE dummymodule( aPcb );    // Creates a dummy parent
    D_PAD dummypad( &dummymodule );
    D_PAD* nextpad;
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad != NULL; pad = nextpad )
        {
            nextpad = pad->Next();  // pad pointer can be modified by next code, so calculate the next pad here
            if( !pad->IsOnLayer( GetLayer() ) )
            {
                /* Test for pads that are on top or bottom only and have a hole.
                 * There are curious pads but they can be used for some components that are inside the
                 * board (in fact inside the hole. Some photo diodes and Leds are like this)
                 */
                if( (pad->m_Drill.x == 0) && (pad->m_Drill.y == 0) )
                    continue;

                // Use a dummy pad to calculate a hole shape that have the same dimension as the pad hole
                dummypad.m_Size     = pad->m_Drill;
                dummypad.m_Orient   = pad->m_Orient;
                dummypad.m_PadShape = pad->m_DrillShape;
                dummypad.m_Pos = pad->m_Pos;
                pad = &dummypad;
            }

            if( pad->GetNet() != GetNet() )
            {
                item_clearance   = pad->GetClearance() + margin;
                item_boundingbox = pad->GetBoundingBox();
                if( item_boundingbox.Intersects( zone_boundingbox ) )
                {
                    int clearance = MAX( zone_clearance, item_clearance );
                    pad->TransformShapeWithClearanceToPolygon( cornerBufferPolysToSubstract,
                                                               clearance,
                                                               s_CircleToSegmentsCount,
                                                               s_Correction );
                }
                continue;
            }

            int gap = zone_clearance;
            if( (m_PadOption == PAD_NOT_IN_ZONE)
               || (GetNet() == 0) || pad->m_PadShape == PAD_TRAPEZOID )

            // PAD_TRAPEZOID shapes are not in zones because they are used in microwave apps
            // and i think it is good that shapes are not changed by thermal pads or others
            {
                item_boundingbox = pad->GetBoundingBox();
                if( item_boundingbox.Intersects( zone_boundingbox ) )
                {
                    pad->TransformShapeWithClearanceToPolygon( cornerBufferPolysToSubstract,
                                                               gap,
                                                               s_CircleToSegmentsCount,
                                                               s_Correction );
                }
            }
        }
    }

    /* Add holes (i.e. tracks and vias areas as polygons outlines)
     * in cornerBufferPolysToSubstract
     */
    for( TRACK* track = aPcb->m_Track;  track;  track = track->Next() )
    {
        if( !track->IsOnLayer( GetLayer() ) )
            continue;
        if( track->GetNet() == GetNet()  && (GetNet() != 0) )
            continue;

        item_clearance   = track->GetClearance() + margin;
        item_boundingbox = track->GetBoundingBox();
        if( item_boundingbox.Intersects( zone_boundingbox ) )
        {
            int clearance = MAX( zone_clearance, item_clearance );
            track->TransformShapeWithClearanceToPolygon( cornerBufferPolysToSubstract,
                                                         clearance,
                                                         s_CircleToSegmentsCount,
                                                         s_Correction );
        }
    }

    /* Add module edge items that are on copper layers
     * Pcbnew allows these items to be on copper layers in microwave applictions
     * This is a bad thing, but must be handle here, until a better way is found
     */
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( BOARD_ITEM* item = module->m_Drawings;  item;  item = item->Next() )
        {
            if( !item->IsOnLayer( GetLayer() ) )
                continue;
            if( item->Type() != TYPE_EDGE_MODULE )
                continue;
            item_boundingbox = item->GetBoundingBox();
            if( item_boundingbox.Intersects( zone_boundingbox ) )
            {
                ( (EDGE_MODULE*) item )->TransformShapeWithClearanceToPolygon(
                    cornerBufferPolysToSubstract, m_ZoneClearance,
                    s_CircleToSegmentsCount, s_Correction );
            }
        }
    }

    // Add graphic items (copper texts) and board edges
    for( BOARD_ITEM* item = aPcb->m_Drawings;  item;  item = item->Next() )
    {
        if( item->GetLayer() != GetLayer() && item->GetLayer() != EDGE_N )
            continue;

        switch( item->Type() )
        {
        case TYPE_DRAWSEGMENT:
            ( (DRAWSEGMENT*) item )->TransformShapeWithClearanceToPolygon(
                cornerBufferPolysToSubstract,
                m_ZoneClearance,
                s_CircleToSegmentsCount,
                s_Correction );
            break;


        case TYPE_TEXTE:
            ( (TEXTE_PCB*) item )->TransformShapeWithClearanceToPolygon(
                cornerBufferPolysToSubstract,
                m_ZoneClearance,
                s_CircleToSegmentsCount,
                s_Correction );
            break;

        default:
            break;
        }
    }

    // Remove thermal symbols
    if( m_PadOption == THERMAL_PAD )
    {
        for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
        {
            for( D_PAD* pad = module->m_Pads; pad != NULL; pad = pad->Next() )
            {
                if( !pad->IsOnLayer( GetLayer() ) )
                    continue;

                if( pad->GetNet() != GetNet() )
                    continue;
                item_boundingbox = pad->GetBoundingBox();
                item_boundingbox.Inflate( m_ThermalReliefGapValue, m_ThermalReliefGapValue );
                if( item_boundingbox.Intersects( zone_boundingbox ) )
                {
                    CreateThermalReliefPadPolygon( cornerBufferPolysToSubstract,
                                                   *pad, m_ThermalReliefGapValue,
                                                   m_ThermalReliefCopperBridgeValue,
                                                   m_ZoneMinThickness,
                                                   s_CircleToSegmentsCount,
                                                   s_Correction, s_thermalRot );
                }
            }
        }
    }

    // cornerBufferPolysToSubstract contains polygons to substract.
    // Calculate now actual solid areas
    if( cornerBufferPolysToSubstract.size() > 0 )
    {
        KPolygonSet polyset_holes;
        AddPolygonCornersToKPolygonList( cornerBufferPolysToSubstract,
                                         polyset_holes );
        // Remove holes from initial area.:
        polyset_zone_solid_areas -= polyset_holes;

        /* put these areas in m_FilledPolysList */
        m_FilledPolysList.clear();
        CopyPolygonsFromKPolygonListToFilledPolysList( this, polyset_zone_solid_areas );
    }

    // Remove insulated islands:
    if( GetNet() > 0 )
        Test_For_Copper_Island_And_Remove_Insulated_Islands( aPcb );

// Now we remove all unused thermal stubs.
    if( m_PadOption == THERMAL_PAD )
    {
        cornerBufferPolysToSubstract.clear();
        // Test thermal stubs connections and add polygons to remove unconnected stubs.
        BuildUnconnectedThermalStubsPolygonList( cornerBufferPolysToSubstract, aPcb, this, s_Correction, s_thermalRot );

        /* remove copper areas */
        if( cornerBufferPolysToSubstract.size() )
        {
            KPolygonSet polyset_holes;
            AddPolygonCornersToKPolygonList( cornerBufferPolysToSubstract, polyset_holes );
            polyset_zone_solid_areas -= polyset_holes;

            /* put these areas in m_FilledPolysList */
            m_FilledPolysList.clear();
            CopyPolygonsFromKPolygonListToFilledPolysList( this, polyset_zone_solid_areas );
            if( GetNet() > 0 )
                Test_For_Copper_Island_And_Remove_Insulated_Islands( aPcb );
        }
    }

    cornerBufferPolysToSubstract.clear();
}

void AddPolygonCornersToKPolygonList( std::vector <CPolyPt>&
                                                   aCornersBuffer,
                                      KPolygonSet& aKPolyList )
{
    unsigned ii;

    std::vector<KPolyPoint> cornerslist;

    int polycount = 0;
    for( unsigned ii = 0; ii < aCornersBuffer.size(); ii++ )
    {
        if( aCornersBuffer[ii].end_contour )
            polycount++;
    }

    aKPolyList.reserve( polycount );

    for( unsigned icnt = 0; icnt < aCornersBuffer.size(); )
    {
        KPolygon poly;
        cornerslist.clear();
        for( ii = icnt; ii < aCornersBuffer.size(); ii++ )
        {
            cornerslist.push_back( KPolyPoint( aCornersBuffer[ii].x,
                                               aCornersBuffer[ii].y ) );
            if( aCornersBuffer[ii].end_contour )
                break;
        }

        bpl::set_points( poly, cornerslist.begin(), cornerslist.end() );
        aKPolyList.push_back( poly );
        icnt = ii + 1;
    }
}


int CopyPolygonsFromKPolygonListToFilledPolysList( ZONE_CONTAINER*
                                                   aZone, KPolygonSet&
                                                   aKPolyList )
{
    int count = 0;

    for( unsigned ii = 0; ii < aKPolyList.size(); ii++ )
    {
        KPolygon& poly = aKPolyList[ii];
        CPolyPt   corner( 0, 0, false );
        for( unsigned jj = 0; jj < poly.size(); jj++ )
        {
            KPolyPoint point = *(poly.begin() + jj);
            corner.x = point.x();
            corner.y = point.y();
            corner.end_contour = false;

            // Flag this corner if starting a hole connection segment:
            // This is used by draw functions to draw only useful segments (and not extra segments)
            // corner.utility = (aBoolengine->GetPolygonPointEdgeType() == KB_FALSE_EDGE) ? 1 : 0;
            aZone->m_FilledPolysList.push_back( corner );
            count++;
        }

        corner.end_contour = true;
        aZone->m_FilledPolysList.pop_back();
        aZone->m_FilledPolysList.push_back( corner );
    }

    return count;
}


int CopyPolygonsFromFilledPolysListTotKPolygonList( ZONE_CONTAINER*
                                                    aZone, KPolygonSet&
                                                    aKPolyList )
{
    unsigned corners_count = aZone->m_FilledPolysList.size();
    int      count = 0;
    unsigned ic    = 0;

    int      polycount = 0;

    for( unsigned ii = 0; ii < corners_count; ii++ )
    {
        CPolyPt* corner = &aZone->m_FilledPolysList[ic];
        if( corner->end_contour )
            polycount++;
    }

    aKPolyList.reserve( polycount );
    std::vector<KPolyPoint> cornerslist;
    while( ic < corners_count )
    {
        cornerslist.clear();
        KPolygon poly;
        {
            for( ; ic < corners_count; ic++ )
            {
                CPolyPt* corner = &aZone->m_FilledPolysList[ic];
                cornerslist.push_back( KPolyPoint( corner->x, corner->y ) );
                count++;
                if( corner->end_contour )
                {
                    ic++;
                    break;
                }
            }

            bpl::set_points( poly, cornerslist.begin(), cornerslist.end() );
            aKPolyList.push_back( poly );
        }
    }

    return count;
}
