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
#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "trigo.h"

#include "zones.h"

#include "PolyLine.h"

// Kbool 1.9 and before had sometimes problemes when calculating thermal shapes as polygons (this is the best solution)
// Kbool 2.0 has solved some problems, but not all
// Kbool 2.1 has solved some others problems, but not all

// Used to create data files to debug Kbool
#include "debug_kbool_key_file_fct.h"

// Also we can create test files for Kbool bebug purposes
// when CREATE_KBOOL_KEY_FILES is defined
// See debug_kbool_key_file_fct.h


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

// Local Functions:
static void AddPolygonCornersToBoolengine( std::vector <CPolyPt>& aCornersBuffer,
                                           Bool_Engine* aBoolengine, GroupType aGroup );

// Local Variables:
#ifdef CREATE_KBOOL_KEY_FILES_WITH_0_DEG
static int s_thermalRot = 0;
#else
static int s_thermalRot = 450;  // angle of stubs in thermal reliefs for round pads
#endif

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
 * 1 - creates a Bool_Engine,with option: holes are linked to outer contours by double overlapping segments
 *       this means the created polygons have no holes (hole are linked to outer outline by double overlapped segments
 *       and are therefore compatible with draw functions (DC draw polygons and Gerber or PS outputs)
 * 2 - Add the main outline (zone outline) in group A
 * 3 - Creates a correction using BOOL_CORRECTION operation to shrink the resulting area
 *     with m_ZoneMinThickness/2 value.
 *     The result is areas with a margin of m_ZoneMinThickness/2
 *     When drawing outline with segments having a thickness of m_ZoneMinThickness, the outlines will
 *     match exactly the initial outlines
 * 4 - recreates the same Bool_Engine, with no correction
 * 5 - Add the main modified outline (zone outline) in group A
 * 6 - Add all non filled areas (pads, tracks) in group B with a clearance of m_Clearance + m_ZoneMinThickness/2
 * 7 - calculates the polygon A - B
 * 8 - put resulting list of polygons (filled areas) in m_FilledPolysList
 *     This zone contains pads with the same net.
 * 9 - Remove insulated copper islands
 * 10 - If Thermal shapes are wanted, remove copper around pads in zone, in order to create thes thermal shapes
 *      a - Creates a bool engine and add the last copper areas in group A
 *      b - Add thermal shapes (non copper ares in group B
 *      c - Calculates the polygon A - B
 * 11 - Remove new insulated copper islands
 */

/* Important note:
 * One can add thermal areas in the step 6, with others items to substract.
 * It is faster.
 * But :
 *     kbool fails sometimes in this case
 *     The separate step to make thermal shapes allows a more sophisticated algorith (todo)
 *     like remove thermal copper bridges in thermal shapes that are not connected to an area
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

    Bool_Engine* booleng = new Bool_Engine();
    ArmBoolEng( booleng, true );

    /* First, Add the main polygon (i.e. the filled area using only one outline)
     * in GroupA in Bool_Engine to do a BOOL_CORRECTION operation
     * to reserve a m_ZoneMinThickness/2 margind around the outlines and holes
     * the margin will be filled when redraw outilnes with segments having a width set to
     * m_ZoneMinThickness
     * so m_ZoneMinThickness is the min thickness of the filled zones areas
     */
    CopyPolygonsFromFilledPolysListToBoolengine( booleng, GROUP_A );
    booleng->SetCorrectionFactor( (double) -m_ZoneMinThickness / 2 );
    booleng->Do_Operation( BOOL_CORRECTION );

    /* Now copy the new outline in m_FilledPolysList */
    m_FilledPolysList.clear();
    CopyPolygonsFromBoolengineToFilledPolysList( booleng );
    delete booleng;

    if( m_FilledPolysList.size() == 0 )
        return;

    /* Second, Add the main (corrected) polygon (i.e. the filled area using only one outline)
     * in GroupA in Bool_Engine to do a BOOL_A_SUB_B operation
     * All areas to remove will be put in GroupB in Bool_Engine
     */
    booleng = new Bool_Engine();
    ArmBoolEng( booleng, true );

    /* Calculates the clearance value that meet DRC requirements
     * from m_ZoneClearance and clearance from the corresponding netclass
     * We have a "local" clearance in zones because most of time
     * clearance between a zone and others items is bigger than the netclass clearance
     * this is more true for small clearance values
     * Note also the "local" clearance is used for clearance between non copper items
     *    or items like texts on copper layers
     */
    int         margin = m_ZoneMinThickness / 2;
    int zone_clearance = max( m_ZoneClearance, GetClearance() );
    zone_clearance += margin;

    /* Add holes (i.e. tracks and pads areas as polygons outlines)
     * in GroupB in Bool_Engine
     */

    /* items ouside the zone bounding box are skipped
     * the bounding box is the zone bounding box + the biggest clearance found in Netclass list
     */
    EDA_Rect item_boundingbox;
    EDA_Rect zone_boundingbox  = GetBoundingBox();
    int      biggest_clearance = aPcb->GetBiggestClearanceValue();
    biggest_clearance = MAX( biggest_clearance, zone_clearance );
    zone_boundingbox.Inflate( biggest_clearance );

#ifdef CREATE_KBOOL_KEY_FILES_FIRST_PASS
    CreateKeyFile();
    OpenKeyFileEntity( "Layer_fp" );
    CopyPolygonsFromFilledPolysListToKeyFile( this, 0 );
#endif

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
                /* Test fo pads that are on top or bottom only and have a hole.
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
     * Pcbnew allows these items to be on copper layers in microwvae applictions
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

    // cornerBufferPolysToSubstract contains polygons to substract,
    // prepare booleng to do that:
    AddPolygonCornersToBoolengine( cornerBufferPolysToSubstract,
                                   booleng, GROUP_B );

#ifdef CREATE_KBOOL_KEY_FILES_FIRST_PASS
    for( unsigned icnt = 0; icnt < cornerBuffer.size(); )
    {
        StartKeyFilePolygon( 1 );
        for( ii = icnt; ii < cornerBuffer.size(); ii++ )
        {
            AddKeyFilePointXY( cornerBufferPolysToSubstract[ii].x,
                               cornerBufferPolysToSubstract[ii].y );
            if( cornerBufferPolysToSubstract[ii].end_contour )
                break;
        }

        EndKeyFilePolygon();
        icnt = ii + 1;
    }

    CloseKeyFileEntity();
    CloseKeyFile();
#endif
/* calculates copper areas */

    if( cornerBufferPolysToSubstract.size() > 0 )
    {
        /* Add the main corrected polygon (i.e. the filled area using only one outline)
         * in GroupA in Bool_Engine
         */
        CopyPolygonsFromFilledPolysListToBoolengine( booleng, GROUP_A );

        booleng->Do_Operation( BOOL_A_SUB_B );

        /* put these areas in m_FilledPolysList */
        m_FilledPolysList.clear();
        CopyPolygonsFromBoolengineToFilledPolysList( booleng );
    }
    delete booleng;

// Remove insulated islands:
    if( GetNet() > 0 )
        Test_For_Copper_Island_And_Remove_Insulated_Islands( aPcb );

    // remove thermal gaps if required:
    if( m_PadOption != THERMAL_PAD || aPcb->m_Modules == NULL )
        return;

// Remove thermal symbols
    cornerBufferPolysToSubstract.clear();
    if( m_PadOption == THERMAL_PAD )
    {
        booleng = new Bool_Engine();
        ArmBoolEng( booleng, true );

#ifdef CREATE_KBOOL_KEY_FILES
        CreateKeyFile();
        OpenKeyFileEntity( "Layer" );
        CopyPolygonsFromFilledPolysListToKeyFile( this, 0 );
#endif

        for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
        {
            for( D_PAD* pad = module->m_Pads; pad != NULL; pad = pad->Next() )
            {
                if( !pad->IsOnLayer( GetLayer() ) )
                    continue;

                if( pad->GetNet() != GetNet() )
                    continue;
                item_boundingbox = pad->GetBoundingBox();
                item_boundingbox.Inflate( m_ThermalReliefGapValue );
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

        if( cornerBufferPolysToSubstract.size() )
        {
            /* Add the main corrected polygon (i.e. the filled area using only one outline)
             * in GroupA in Bool_Engine
             */
            CopyPolygonsFromFilledPolysListToBoolengine( booleng, GROUP_A );

            // cornerBufferPolysToSubstract contains polygons to substract,
            // prepare booleng to do that:
            AddPolygonCornersToBoolengine( cornerBufferPolysToSubstract,
                                           booleng, GROUP_B );

            /* remove thermal areas (non copper areas) */
            booleng->Do_Operation( BOOL_A_SUB_B );
            /* put these areas in m_FilledPolysList */
            m_FilledPolysList.clear();
            CopyPolygonsFromBoolengineToFilledPolysList( booleng );
        }
        delete booleng;

        // Remove insulated islands:
        if( GetNet() > 0 )
            Test_For_Copper_Island_And_Remove_Insulated_Islands( aPcb );
#ifdef CREATE_KBOOL_KEY_FILES
        for( unsigned icnt = 0; icnt < cornerBufferPolysToSubstract.size(); )
        {
            StartKeyFilePolygon( 1 );
            for( ii = icnt; ii < cornerBufferPolysToSubstract.size(); ii++ )
            {
                AddKeyFilePointXY( cornerBufferPolysToSubstract[ii].x,
                                   cornerBufferPolysToSubstract[ii].y );
                if( cornerBufferPolysToSubstract[ii].end_contour )
                    break;
            }

            EndKeyFilePolygon();
            icnt = ii + 1;
        }
    }
    CloseKeyFileEntity();
    CloseKeyFile();
#endif
    }

// Now we remove all unused thermal stubs.
#define REMOVE_UNUSED_THERMAL_STUBS // Can be commented to skip unused thermal stubs calculations
#ifdef REMOVE_UNUSED_THERMAL_STUBS

    booleng = new Bool_Engine();
    ArmBoolEng( booleng, true );
    cornerBufferPolysToSubstract.clear();

// Test thermal stubs connections and add polygons to remove unconnected stubs.
    for( MODULE* module = aPcb->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad != NULL; pad = pad->Next() )
        {
            // check
            if( !pad->IsOnLayer( GetLayer() ) )
                continue;
            if( pad->GetNet() != GetNet() )
                continue;

            item_boundingbox = pad->GetBoundingBox();
            item_boundingbox.Inflate( m_ThermalReliefGapValue );
            if( !( item_boundingbox.Intersects( zone_boundingbox ) ) )
                continue;

            // test point
            int dx =
                ( pad->m_Size.x / 2 ) + m_ThermalReliefGapValue;
            int dy =
                ( pad->m_Size.y / 2 ) + m_ThermalReliefGapValue;

            // This is CIRCLE pad tweak (for circle pads the thermal stubs are at 45 deg)
            int fAngle = pad->m_Orient;
            if( pad->m_PadShape == PAD_CIRCLE )
            {
                dx     = (int) ( dx * s_Correction );
                dy     = dx;
                fAngle = s_thermalRot;
            }

            // compute north, south, west and east points for zone connection.
            // Add a small value to ensure point is inside (or outside) zone, not on an edge
            wxPoint ptTest[4];
            ptTest[0] = wxPoint( 0, 3 + dy + margin );
            ptTest[1] = wxPoint( 0, -(3 + dy + margin) );
            ptTest[2] = wxPoint( 3 + dx + margin, 0 );
            ptTest[3] = wxPoint( -(3 + dx + margin), 0 );


            // Test all sides
            for( int i = 0; i<4; i++ )
            {
                // rotate point
                RotatePoint( &ptTest[i], fAngle );

                // translate point
                ptTest[i] += pad->ReturnShapePos();
                bool inside = HitTestFilledArea( ptTest[i] );

                if( inside == false )
                {
                    // polygon buffer
                    std::vector<wxPoint> corners_buffer;

                    // polygons are rectangles with width of copper bridge value
                    // contour line width has to be taken into calculation to avoid "thermal stub bleed"
                    const int iDTRC =
                        ( m_ThermalReliefCopperBridgeValue - m_ZoneMinThickness ) / 2;

                    switch( i )
                    {
                    case 0:
                        corners_buffer.push_back( wxPoint( -iDTRC, dy ) );
                        corners_buffer.push_back( wxPoint( +iDTRC, dy ) );
                        corners_buffer.push_back( wxPoint( +iDTRC, iDTRC ) );
                        corners_buffer.push_back( wxPoint( -iDTRC, iDTRC ) );
                        break;

                    case 1:
                        corners_buffer.push_back( wxPoint( -iDTRC, -dy ) );
                        corners_buffer.push_back( wxPoint( +iDTRC, -dy ) );
                        corners_buffer.push_back( wxPoint( +iDTRC, -iDTRC ) );
                        corners_buffer.push_back( wxPoint( -iDTRC, -iDTRC ) );
                        break;

                    case 2:
                        corners_buffer.push_back( wxPoint( dx, -iDTRC ) );
                        corners_buffer.push_back( wxPoint( dx, iDTRC ) );
                        corners_buffer.push_back( wxPoint( +iDTRC, iDTRC ) );
                        corners_buffer.push_back( wxPoint( +iDTRC, -iDTRC ) );
                        break;

                    case 3:
                        corners_buffer.push_back( wxPoint( -dx, -iDTRC ) );
                        corners_buffer.push_back( wxPoint( -dx, iDTRC ) );
                        corners_buffer.push_back( wxPoint( -iDTRC, iDTRC ) );
                        corners_buffer.push_back( wxPoint( -iDTRC, -iDTRC ) );
                        break;
                    }


                    // add computed polygon to list
                    for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
                    {
                        wxPoint cpos = corners_buffer[ic];
                        RotatePoint( &cpos, fAngle );                           // Rotate according to module orientation
                        cpos += pad->ReturnShapePos();                          // Shift origin to position
                        CPolyPt corner;
                        corner.x = cpos.x;
                        corner.y = cpos.y;
                        corner.end_contour = ( ic < (corners_buffer.size() - 1) ) ? 0 : 1;
                        cornerBufferPolysToSubstract.push_back( corner );
                    }
                }
            }
        }
    }

/* compute copper areas */
    if( cornerBufferPolysToSubstract.size() )
    {
        /* Add the main corrected polygon (i.e. the filled area using only one outline)
         * in GroupA in Bool_Engine
         */
        CopyPolygonsFromFilledPolysListToBoolengine( booleng, GROUP_A );
        AddPolygonCornersToBoolengine( cornerBufferPolysToSubstract, booleng, GROUP_B );

        booleng->Do_Operation( BOOL_A_SUB_B );

        /* put these areas in m_FilledPolysList */
        m_FilledPolysList.clear();
        CopyPolygonsFromBoolengineToFilledPolysList( booleng );

        // Remove insulated islands, if any:
        if( GetNet() > 0 )
            Test_For_Copper_Island_And_Remove_Insulated_Islands( aPcb );
    }

    delete booleng;

#endif  // REMOVE_UNUSED_THERMAL_STUBS
}


/** AddPolygonCornersToBoolengine
 * copy a set of polygons in a kbool engine
 * @param aCornersBuffer = list of polygons defined by corners.
 * @ param aBoolengine = the kbool engine to populate
 * @ param aGroup = GROUP_A or GROUP_B
 */
void AddPolygonCornersToBoolengine( std::vector <CPolyPt>& aCornersBuffer,
                                    Bool_Engine* aBoolengine, GroupType aGroup )
{
    unsigned ii;

    for( unsigned icnt = 0; icnt < aCornersBuffer.size(); )
    {
        aBoolengine->StartPolygonAdd( aGroup );
        for( ii = icnt; ii < aCornersBuffer.size(); ii++ )
        {
            aBoolengine->AddPoint( aCornersBuffer[ii].x, aCornersBuffer[ii].y );
            if( aCornersBuffer[ii].end_contour )
                break;
        }

        aBoolengine->EndPolygonAdd();
        icnt = ii + 1;
    }
}


/***********************************************************************************************************/
int ZONE_CONTAINER::CopyPolygonsFromFilledPolysListToBoolengine( Bool_Engine* aBoolengine,
                                                                 GroupType    aGroup )
/************************************************************************************************************/

/**
 * Function CopyPolygonsFromFilledPolysListToBoolengine
 * Copy (Add) polygons found in m_FilledPolysList to kbool BoolEngine
 * m_FilledPolysList may have more than one polygon
 * @param aBoolengine = kbool engine
 * @param aGroup = group in kbool engine (GROUP_A or GROUP_B only)
 * @return the corner count
 */
{
    unsigned corners_count = m_FilledPolysList.size();
    int count   = 0;
    unsigned ic = 0;

    while( ic < corners_count )
    {
        if( aBoolengine->StartPolygonAdd( aGroup ) )
        {
            for( ; ic < corners_count; ic++ )
            {
                CPolyPt* corner = &m_FilledPolysList[ic];
                aBoolengine->AddPoint( corner->x, corner->y );
                count++;
                if( corner->end_contour )
                {
                    ic++;
                    break;
                }
            }

            aBoolengine->EndPolygonAdd();
        }
    }

    return count;
}


/*****************************************************************************************/
int ZONE_CONTAINER::CopyPolygonsFromBoolengineToFilledPolysList( Bool_Engine* aBoolengine )
/*****************************************************************************************/

/**
 * Function CopyPolygonsFromBoolengineToFilledPolysList
 * Copy (Add) polygons created by kbool (after Do_Operation) to m_FilledPolysList
 * @param aBoolengine = kbool engine
 * @return the corner count
 */
{
    int count = 0;

    while( aBoolengine->StartPolygonGet() )
    {
        CPolyPt corner( 0, 0, false );
        while( aBoolengine->PolygonHasMorePoints() )
        {
            corner.x = (int) aBoolengine->GetPolygonXPoint();
            corner.y = (int) aBoolengine->GetPolygonYPoint();
            corner.end_contour = false;

            // Flag this corner if starting a hole connection segment:
            // This is used by draw functions to draw only useful segments (and not extra segments)
            corner.utility = (aBoolengine->GetPolygonPointEdgeType() == KB_FALSE_EDGE) ? 1 : 0;
            m_FilledPolysList.push_back( corner );
            count++;
        }

        corner.end_contour = true;
        m_FilledPolysList.pop_back();
        m_FilledPolysList.push_back( corner );
        aBoolengine->EndPolygonGet();
    }

    return count;
}
