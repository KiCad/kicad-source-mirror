#include <common.h>
#include <class_board.h>
#include <class_drawsegment.h>
#include <class_pad.h>

#include <convert_basic_shapes_to_polygon.h>
#include <geometry/polygon_test_point_inside.h>

#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_rect.h>

#include <drc_proto/drc_engine.h>
#include <drc_proto/drc_test_provider.h>
#include <drc_proto/drc_item.h>
#include <drc_proto/drc_rule.h>

namespace test {

class DRC_TEST_PROVIDER_CLEARANCE : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_CLEARANCE ( DRC_ENGINE *aDrc ) :
        DRC_TEST_PROVIDER( aDrc )
        {

        }

    virtual ~DRC_TEST_PROVIDER_CLEARANCE() 
    {

    }

    virtual bool Run() override;

    virtual const wxString GetName() const override { return "clearance"; };
    virtual const wxString GetDescription() const override { return "Tests copper item clearance"; }
    virtual std::set<test::DRC_RULE_ID_T> GetMatchingRuleIds() const override;

private:
    void testPadClearances( );
    bool doPadToPadsDrc( D_PAD* aRefPad, D_PAD** aStart, D_PAD** aEnd, int x_limit );

    bool checkClearanceSegmToPad( const SEG& refSeg, int refSegWidth, const D_PAD* pad,
                                   int minClearance, int* aActualDist );
    bool checkClearancePadToPad( D_PAD* aRefPad, D_PAD* aPad, int aMinClearance, int* aActual );
    bool poly2segmentDRC( wxPoint* aTref, int aTrefCount, wxPoint aSegStart, wxPoint aSegEnd,
                      int aDist, int* aActual );
    bool poly2polyDRC( wxPoint* aTref, int aTrefCount, wxPoint* aTtest, int aTtestCount,
                   int aAllowedDist, int* actualDist );

    BOARD* m_board;
    int m_largestClearance;
    SHAPE_POLY_SET             m_boardOutline;   // The board outline including cutouts
    bool                       m_boardOutlineValid;
    

};

};

bool test::DRC_TEST_PROVIDER_CLEARANCE::Run()
{
    auto bds = m_drcEngine->GetDesignSettings();

    m_board = m_drcEngine->GetBoard();
    m_largestClearance = bds->GetBiggestClearanceValue();

    testPadClearances();

    return false;
}



void test::DRC_TEST_PROVIDER_CLEARANCE::testPadClearances( )
{
    auto bds = m_drcEngine->GetDesignSettings();
    std::vector<D_PAD*>    sortedPads;

    m_board->GetSortedPadListByXthenYCoord( sortedPads );

    if( sortedPads.empty() )
        return;

    // find the max size of the pads (used to stop the pad-to-pad tests)
    int max_size = 0;

    for( D_PAD* pad : sortedPads )
    {
        // GetBoundingRadius() is the radius of the minimum sized circle fully containing the pad
        int radius = pad->GetBoundingRadius();

        if( radius > max_size )
            max_size = radius;
    }

    // Better to be fast than accurate; this keeps us from having to look up / calculate the
    // actual clearances
    max_size += m_largestClearance;

    // Upper limit of pad list (limit not included)
    D_PAD** listEnd = &sortedPads[0] + sortedPads.size();

    // Test the pads
    for( auto& pad : sortedPads )
    {

#if 0
    // fixme: move Board outline clearance to separate provider 
        if( m_boardOutlineValid )
        {
            //int minClearance = bds->m_CopperEdgeClearance;
            //m_clearanceSource = _( "board edge" );

            static DRAWSEGMENT dummyEdge;
            dummyEdge.SetLayer( Edge_Cuts );

            //if( pad->GetRuleClearance( &dummyEdge, &minClearance, &m_clearanceSource ) )
              //  /* minClearance and m_clearanceSource set in GetRuleClearance() */;
            // FIXME
            // auto rule = m_drcEngine->EvalRuleForItems( pad, dummyEdge, DRC_RULE_CLEARANCE );
            // min_clearance = rule->Constraint().Min();
            int minClearance;

            for( auto it = m_boardOutline.IterateSegmentsWithHoles(); it; it++ )
            {
                int actual;

                if( !checkClearanceSegmToPad( *it, 0, pad, minClearance, &actual ) )
                {
                    actual = std::max( 0, actual );
                    DRC_ITEM* drcItem = new DRC_ITEM( DRCE_PAD_NEAR_EDGE );
                    wxString msg;

                    msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                                  /*m_clearanceSource FIXME*/ "" ,
                                  MessageTextFromValue( userUnits(), minClearance, true ),
                                  MessageTextFromValue( userUnits(), actual, true ) );

                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( pad );

                    MARKER_PCB* marker = nullptr; // fixme new MARKER_PCB( drcItem, pad->GetPosition() );
                    AddMarkerToPcb( marker );

                    break;
                }
            }
        }
#endif
        //if( !bds->Ignore( DRCE_PAD_NEAR_PAD ) || !bds->Ignore( DRCE_HOLE_NEAR_PAD ) )
        {
            int x_limit = pad->GetPosition().x + pad->GetBoundingRadius() + max_size;

            doPadToPadsDrc( pad, &pad, listEnd, x_limit );
        }
    }
}

bool test::DRC_TEST_PROVIDER_CLEARANCE::doPadToPadsDrc( D_PAD* aRefPad, D_PAD** aStart, D_PAD** aEnd,
                          int x_limit )
{
    const static LSET all_cu = LSET::AllCuMask();

    LSET layerMask = aRefPad->GetLayerSet() & all_cu;


    // For hole testing we use a dummy pad which is given the shape of the hole.  Note that
    // this pad must have a parent because some functions expect a non-null parent to find
    // the pad's board.
    MODULE dummymodule( m_drcEngine->GetBoard() );    // Creates a dummy parent
    D_PAD  dummypad( &dummymodule );

    // Ensure the hole is on all copper layers
    dummypad.SetLayerSet( all_cu | dummypad.GetLayerSet() );

    for( D_PAD** pad_list = aStart;  pad_list<aEnd;  ++pad_list )
    {
        D_PAD* pad = *pad_list;

        if( pad == aRefPad )
            continue;

        // We can stop the test when pad->GetPosition().x > x_limit
        // because the list is sorted by X values
        if( pad->GetPosition().x > x_limit )
            break;

#if 0

// fixme move hole clearance check to another provider

        // No problem if pads which are on copper layers are on different copper layers,
        // (pads can be only on a technical layer, to build complex pads)
        // but their hole (if any ) can create DRC error because they are on all
        // copper layers, so we test them
        if( ( pad->GetLayerSet() & layerMask ) == 0 &&
            ( pad->GetLayerSet() & all_cu ) != 0 &&
            ( aRefPad->GetLayerSet() & all_cu ) != 0 )
        {
            // if holes are in the same location and have the same size and shape,
            // this can be accepted
            if( pad->GetPosition() == aRefPad->GetPosition()
                && pad->GetDrillSize() == aRefPad->GetDrillSize()
                && pad->GetDrillShape() == aRefPad->GetDrillShape() )
            {
                if( aRefPad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
                    continue;

                // for oval holes: must also have the same orientation
                if( pad->GetOrientation() == aRefPad->GetOrientation() )
                    continue;
            }


    // fixme move hole clearance check to another providers

            /* Here, we must test clearance between holes and pads
             * dummy pad size and shape is adjusted to pad drill size and shape
             */
            if( pad->GetDrillSize().x )
            {
                // pad under testing has a hole, test this hole against pad reference
                dummypad.SetPosition( pad->GetPosition() );
                dummypad.SetSize( pad->GetDrillSize() );
                dummypad.SetShape( pad->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG ?
                                                           PAD_SHAPE_OVAL : PAD_SHAPE_CIRCLE );
                dummypad.SetOrientation( pad->GetOrientation() );

                int minClearance = 0; // fixme aRefPad->GetClearance( nullptr, &m_clearanceSource );

                auto rule = m_drcEngine->MatchRulesForItems( DRC_RULE_ID_CLEARANCE, pad, &dummypad );
                int minClearance = rule->m_Value.Min();
                
                int actual;

                if( !checkClearancePadToPad( aRefPad, &dummypad, minClearance, &actual ) )
                {
                    DRC_ITEM* drcItem = new DRC_ITEM( DRCE_HOLE_NEAR_PAD );
                    wxString msg;

                    msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                                  /* fixme m_clearanceSource */"",
                                  MessageTextFromValue( userUnits(), minClearance, true ),
                                  MessageTextFromValue( userUnits(), actual, true ) );

                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( pad, aRefPad );

                    MARKER_PCB* marker = nullptr; // fixme new MARKER_PCB( drcItem, pad->GetPosition() );
                    AddMarkerToPcb( marker );
                    return false;
                }
            }

            if( aRefPad->GetDrillSize().x ) // pad reference has a hole
            {
                dummypad.SetPosition( aRefPad->GetPosition() );
                dummypad.SetSize( aRefPad->GetDrillSize() );
                dummypad.SetShape( aRefPad->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG ?
                                                               PAD_SHAPE_OVAL : PAD_SHAPE_CIRCLE );
                dummypad.SetOrientation( aRefPad->GetOrientation() );

                // FIXME min_clearance = rule->Constraint().Min();
                //int minClearance = pad->GetClearance( nullptr, &m_clearanceSource );
                int minClearance;
                int actual;

                if( !checkClearancePadToPad( pad, &dummypad, minClearance, &actual ) )
                {
                    DRC_ITEM* drcItem = new DRC_ITEM( DRCE_HOLE_NEAR_PAD );
                    wxString msg;

                    msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                                  /*m_clearanceSource FIXME */ "",
                                  MessageTextFromValue( userUnits(), minClearance, true ),
                                  MessageTextFromValue( userUnits(), actual, true ) );

                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( aRefPad, pad );

                    MARKER_PCB* marker = nullptr; // fixme new MARKER_PCB( drcItem, aRefPad->GetPosition() );
                    AddMarkerToPcb( marker );
                    return false;
                }
            }

            continue;
        }
#endif

        // The pad must be in a net (i.e pt_pad->GetNet() != 0 ),
        // But no problem if pads have the same netcode (same net)
        if( pad->GetNetCode() && ( aRefPad->GetNetCode() == pad->GetNetCode() ) )
            continue;

        // if pads are from the same footprint
        if( pad->GetParent() == aRefPad->GetParent() )
        {
            // and have the same pad number ( equivalent pads  )

            // one can argue that this 2nd test is not necessary, that any
            // two pads from a single module are acceptable.  This 2nd test
            // should eventually be a configuration option.
            if( pad->PadNameEqual( aRefPad ) )
                continue;
        }

        // if either pad has no drill and is only on technical layers, not a clearance violation
        if( ( ( pad->GetLayerSet() & layerMask ) == 0 && !pad->GetDrillSize().x ) ||
            ( ( aRefPad->GetLayerSet() & layerMask ) == 0 && !aRefPad->GetDrillSize().x ) )
        {
            continue;
        }

        auto constraint = m_drcEngine->EvalRulesForItems( test::DRC_RULE_ID_T::DRC_RULE_ID_CLEARANCE, aRefPad, pad );

        //int  minClearance = aRefPad->GetClearance( pad, *&m_clearanceSource );
        int minClearance; // fixme

        int  clearanceAllowed = minClearance - m_drcEngine->GetDesignSettings()->GetDRCEpsilon();
        int  actual;

        if( !checkClearancePadToPad( aRefPad, pad, clearanceAllowed, &actual ) )
        {
            DRC_ITEM* drcItem = new DRC_ITEM( DRCE_PAD_NEAR_PAD );
            wxString msg;
            msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                          /*m_clearanceSource fixme*/ "",
                          MessageTextFromValue( userUnits(), minClearance, true ),
                          MessageTextFromValue( userUnits(), actual, true ) );

            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( aRefPad, pad );

            MARKER_PCB* marker = nullptr; // fixme new MARKER_PCB( drcItem, aRefPad->GetPosition() );
            AddMarkerToPcb( marker );

            return false;
        }
    }

    return true;
}


/*
 * Test if distance between a segment and a pad is > minClearance.  Return the actual
 * distance if it is less.
 */
bool test::DRC_TEST_PROVIDER_CLEARANCE::checkClearanceSegmToPad( const SEG& refSeg, int refSegWidth, const D_PAD* pad,
                                   int minClearance, int* aActualDist )
{
    if( ( pad->GetShape() == PAD_SHAPE_CIRCLE || pad->GetShape() == PAD_SHAPE_OVAL ) )
    {
        /* Treat an oval pad as a line segment along the hole's major axis,
         * shortened by half its minor axis.
         * A circular pad is just a degenerate case of an oval hole.
         */
        wxPoint padStart, padEnd;
        int     padWidth;

        pad->GetOblongGeometry( pad->GetSize(), &padStart, &padEnd, &padWidth );
        padStart += pad->ShapePos();
        padEnd += pad->ShapePos();

        SEG padSeg( padStart, padEnd );
        int widths = ( padWidth + refSegWidth ) / 2;
        int center2centerAllowed = minClearance + widths;

        // Avoid square-roots if possible (for performance)
        SEG::ecoord center2center_squared = refSeg.SquaredDistance( padSeg );

        if( center2center_squared < SEG::Square( center2centerAllowed ) )
        {
            *aActualDist = std::max( 0.0, sqrt( center2center_squared ) - widths );
            return false;
        }
    }
    else if( ( pad->GetShape() == PAD_SHAPE_RECT || pad->GetShape() == PAD_SHAPE_ROUNDRECT )
            && ( (int) pad->GetOrientation() % 900 == 0 ) )
    {
        EDA_RECT padBBox = pad->GetBoundingBox();
        int     widths = refSegWidth / 2;

        // Note a ROUNDRECT pad with a corner radius = r can be treated as a smaller
        // RECT (size - 2*r) with a clearance increased by r
        if( pad->GetShape() == PAD_SHAPE_ROUNDRECT )
        {
            padBBox.Inflate( - pad->GetRoundRectCornerRadius() );
            widths += pad->GetRoundRectCornerRadius();
        }

        SHAPE_RECT padShape( padBBox.GetPosition(), padBBox.GetWidth(), padBBox.GetHeight() );
        int        actual;

        if( padShape.DoCollide( refSeg, minClearance + widths, &actual ) )
        {
            *aActualDist = std::max( 0, actual - widths );
            return false;
        }
    }
    else        // Convert the rest to polygons
    {
        SHAPE_POLY_SET polyset;

        BOARD* board = pad->GetBoard();
        int    maxError = board ? board->GetDesignSettings().m_MaxError : ARC_HIGH_DEF;

        pad->TransformShapeWithClearanceToPolygon( polyset, 0, maxError );

        const SHAPE_LINE_CHAIN& refpoly = polyset.COutline( 0 );
        int                     widths = refSegWidth / 2;
        int                     actual;

        if( !poly2segmentDRC( (wxPoint*) &refpoly.CPoint( 0 ), refpoly.PointCount(),
                              (wxPoint) refSeg.A, (wxPoint) refSeg.B,
                              minClearance + widths, &actual ) )
        {
            *aActualDist = std::max( 0, actual - widths );
            return false;
        }
    }

    return true;
}


bool test::DRC_TEST_PROVIDER_CLEARANCE::checkClearancePadToPad( D_PAD* aRefPad, D_PAD* aPad, int aMinClearance, int* aActual )
{
    // relativePadPos is the aPad shape position relative to the aRefPad shape position
    wxPoint relativePadPos = aPad->ShapePos() - aRefPad->ShapePos();

    int center2center = KiROUND( EuclideanNorm( relativePadPos ) );

    // Quick test: Clearance is OK if the bounding circles are further away than aMinClearance
    if( center2center - aRefPad->GetBoundingRadius() - aPad->GetBoundingRadius() >= aMinClearance )
        return true;

    /* Here, pads are near and DRC depends on the pad shapes.  We must compare distance using
     * a fine shape analysis.
     * Because a circle or oval shape is the easier shape to test, swap pads to have aRefPad be
     * a PAD_SHAPE_CIRCLE or PAD_SHAPE_OVAL.  If aRefPad = TRAPEZOID and aPad = RECT, also swap.
     */
    bool swap_pads;
    swap_pads = false;

    // swap pads to make comparisons easier
    // Note also a ROUNDRECT pad with a corner radius = r can be considered as
    // a smaller RECT (size - 2*r) with a clearance increased by r
    // priority is aRefPad = ROUND then OVAL then RECT/ROUNDRECT then other
    if( aRefPad->GetShape() != aPad->GetShape() && aRefPad->GetShape() != PAD_SHAPE_CIRCLE )
    {
        // pad ref shape is here oval, rect, roundrect, chamfered rect, trapezoid or custom
        switch( aPad->GetShape() )
        {
            case PAD_SHAPE_CIRCLE:
                swap_pads = true;
                break;

            case PAD_SHAPE_OVAL:
                swap_pads = true;
                break;

            case PAD_SHAPE_RECT:
            case PAD_SHAPE_ROUNDRECT:
                if( aRefPad->GetShape() != PAD_SHAPE_OVAL )
                    swap_pads = true;
                break;

            case PAD_SHAPE_TRAPEZOID:
            case PAD_SHAPE_CHAMFERED_RECT:
            case PAD_SHAPE_CUSTOM:
                break;
        }
    }

    if( swap_pads )
    {
        std::swap( aRefPad, aPad );
        relativePadPos = -relativePadPos;
    }

    bool diag = true;

    if( ( aRefPad->GetShape() == PAD_SHAPE_CIRCLE || aRefPad->GetShape() == PAD_SHAPE_OVAL ) )
    {
        /* Treat an oval pad as a line segment along the hole's major axis,
         * shortened by half its minor axis.
         * A circular pad is just a degenerate case of an oval hole.
         */
        wxPoint refPadStart, refPadEnd;
        int     refPadWidth;

        aRefPad->GetOblongGeometry( aRefPad->GetSize(), &refPadStart, &refPadEnd, &refPadWidth );
        refPadStart += aRefPad->ShapePos();
        refPadEnd += aRefPad->ShapePos();

        SEG refPadSeg( refPadStart, refPadEnd );
        diag = checkClearanceSegmToPad( refPadSeg, refPadWidth, aPad, aMinClearance, aActual );
    }
    else
    {
        int dist_extra = 0;

        // corners of aRefPad (used only for rect/roundrect/trap pad)
        wxPoint polyref[4];
        // corners of aRefPad (used only for custom pad)
        SHAPE_POLY_SET polysetref;

        if( aRefPad->GetShape() == PAD_SHAPE_ROUNDRECT )
        {
            int padRadius = aRefPad->GetRoundRectCornerRadius();
            dist_extra = padRadius;
            GetRoundRectCornerCenters( polyref, padRadius, wxPoint( 0, 0 ), aRefPad->GetSize(),
                                       aRefPad->GetOrientation() );
        }
        else if( aRefPad->GetShape() == PAD_SHAPE_CHAMFERED_RECT )
        {
            BOARD* board = aRefPad->GetBoard();
            int maxError = board ? board->GetDesignSettings().m_MaxError : ARC_HIGH_DEF;

            // The reference pad can be rotated.  Calculate the rotated coordinates.
            // (note, the ref pad position is the origin of coordinates for this drc test)
            int padRadius = aRefPad->GetRoundRectCornerRadius();

            TransformRoundChamferedRectToPolygon( polysetref, wxPoint( 0, 0 ), aRefPad->GetSize(),
                                                  aRefPad->GetOrientation(),
                                                  padRadius, aRefPad->GetChamferRectRatio(),
                                                  aRefPad->GetChamferPositions(), maxError );
        }
        else if( aRefPad->GetShape() == PAD_SHAPE_CUSTOM )
        {
            polysetref.Append( aRefPad->GetCustomShapeAsPolygon() );

            // The reference pad can be rotated.  Calculate the rotated coordinates.
            // (note, the ref pad position is the origin of coordinates for this drc test)
            aRefPad->CustomShapeAsPolygonToBoardPosition( &polysetref, wxPoint( 0, 0 ),
                                                          aRefPad->GetOrientation() );
        }
        else
        {
            // BuildPadPolygon has meaning for rect a trapeziod shapes and returns the 4 corners.
            aRefPad->BuildPadPolygon( polyref, wxSize( 0, 0 ), aRefPad->GetOrientation() );
        }

        // corners of aPad (used only for rect/roundrect/trap pad)
        wxPoint polycompare[4];
        // corners of aPad (used only custom pad)
        SHAPE_POLY_SET polysetcompare;

        switch( aPad->GetShape() )
        {
        case PAD_SHAPE_ROUNDRECT:
        case PAD_SHAPE_RECT:
        case PAD_SHAPE_CHAMFERED_RECT:
        case PAD_SHAPE_TRAPEZOID:
        case PAD_SHAPE_CUSTOM:
            if( aPad->GetShape() == PAD_SHAPE_ROUNDRECT )
            {
                int padRadius = aPad->GetRoundRectCornerRadius();
                dist_extra = padRadius;
                GetRoundRectCornerCenters( polycompare, padRadius, relativePadPos, aPad->GetSize(),
                                           aPad->GetOrientation() );
            }
            else if( aPad->GetShape() == PAD_SHAPE_CHAMFERED_RECT )
            {
                BOARD* board = aRefPad->GetBoard();
                int maxError = board ? board->GetDesignSettings().m_MaxError : ARC_HIGH_DEF;

                // The pad to compare can be rotated. Calculate the rotated coordinates.
                // ( note, the pad to compare position is the relativePadPos for this drc test)
                int padRadius = aPad->GetRoundRectCornerRadius();

                TransformRoundChamferedRectToPolygon( polysetcompare, relativePadPos,
                                                      aPad->GetSize(), aPad->GetOrientation(),
                                                      padRadius, aPad->GetChamferRectRatio(),
                                                      aPad->GetChamferPositions(), maxError );
            }
            else if( aPad->GetShape() == PAD_SHAPE_CUSTOM )
            {
                polysetcompare.Append( aPad->GetCustomShapeAsPolygon() );

                // The pad to compare can be rotated. Calculate the rotated coordinates.
                // ( note, the pad to compare position is the relativePadPos for this drc test)
                aPad->CustomShapeAsPolygonToBoardPosition( &polysetcompare, relativePadPos,
                                                           aPad->GetOrientation() );
            }
            else
            {
                aPad->BuildPadPolygon( polycompare, wxSize( 0, 0 ), aPad->GetOrientation() );

                // Move aPad shape to relativePadPos
                for( int ii = 0; ii < 4; ii++ )
                    polycompare[ii] += relativePadPos;
            }

            // And now test polygons: We have 3 cases:
            // one poly is complex and the other is basic (has only 4 corners)
            // both polys are complex
            // both polys are basic (have only 4 corners) the most usual case
            if( polysetref.OutlineCount() && polysetcompare.OutlineCount() == 0)
            {
                const SHAPE_LINE_CHAIN& refpoly = polysetref.COutline( 0 );
                // And now test polygons:
                if( !poly2polyDRC( (wxPoint*) &refpoly.CPoint( 0 ), refpoly.PointCount(),
                                   polycompare, 4, aMinClearance + dist_extra, aActual ) )
                {
                    *aActual = std::max( 0, *aActual - dist_extra );
                    diag = false;
                }
            }
            else if( polysetref.OutlineCount() == 0 && polysetcompare.OutlineCount())
            {
                const SHAPE_LINE_CHAIN& cmppoly = polysetcompare.COutline( 0 );
                // And now test polygons:
                if( !poly2polyDRC((wxPoint*) &cmppoly.CPoint( 0 ), cmppoly.PointCount(),
                                  polyref, 4, aMinClearance + dist_extra, aActual ) )
                {
                    *aActual = std::max( 0, *aActual - dist_extra );
                    diag = false;
                }
            }
            else if( polysetref.OutlineCount() && polysetcompare.OutlineCount() )
            {
                const SHAPE_LINE_CHAIN& refpoly = polysetref.COutline( 0 );
                const SHAPE_LINE_CHAIN& cmppoly = polysetcompare.COutline( 0 );

                // And now test polygons:
                if( !poly2polyDRC((wxPoint*) &refpoly.CPoint( 0 ), refpoly.PointCount(),
                                  (wxPoint*) &cmppoly.CPoint( 0 ), cmppoly.PointCount(),
                                  aMinClearance + dist_extra, aActual ) )
                {
                    *aActual = std::max( 0, *aActual - dist_extra );
                    diag = false;
                }
            }
            else
            {
                if( !poly2polyDRC( polyref, 4, polycompare, 4, aMinClearance + dist_extra, aActual ) )
                {
                    *aActual = std::max( 0, *aActual - dist_extra );
                    diag = false;
                }
            }
            break;

        default:
            wxLogDebug( wxT( "DRC::checkClearancePadToPad: unexpected pad shape %d" ), aPad->GetShape() );
            break;
        }
    }

    return diag;
}


bool test::DRC_TEST_PROVIDER_CLEARANCE::poly2segmentDRC( wxPoint* aTref, int aTrefCount, wxPoint aSegStart, wxPoint aSegEnd,
                      int aDist, int* aActual )
{
    /* Test if the segment is contained in the polygon.
     * This case is not covered by the following check if the segment is
     * completely contained in the polygon (because edges don't intersect)!
     */
    if( TestPointInsidePolygon( aTref, aTrefCount, aSegStart ) )
    {
        *aActual = 0;
        return false;
    }

    for( int ii = 0, jj = aTrefCount-1; ii < aTrefCount; jj = ii, ii++ )
    {   // for all edges in polygon
        double d;

        if( TestForIntersectionOfStraightLineSegments( aTref[ii].x, aTref[ii].y, aTref[jj].x,
                                                       aTref[jj].y, aSegStart.x, aSegStart.y,
                                                       aSegEnd.x, aSegEnd.y, NULL, NULL, &d ) )
        {
            *aActual = 0;
            return false;
        }

        if( d < aDist )
        {
            *aActual = KiROUND( d );
            return false;
        }
    }

    return true;
}


/**
 * compare 2 convex polygons and return true if distance > aDist (if no error DRC)
 * i.e if for each edge of the first polygon distance from each edge of the other polygon
 * is >= aDist
 */
bool test::DRC_TEST_PROVIDER_CLEARANCE::poly2polyDRC( wxPoint* aTref, int aTrefCount, wxPoint* aTtest, int aTtestCount,
                   int aAllowedDist, int* actualDist )
{
    /* Test if one polygon is contained in the other and thus the polygon overlap.
     * This case is not covered by the following check if one polygone is
     * completely contained in the other (because edges don't intersect)!
     */
    if( TestPointInsidePolygon( aTref, aTrefCount, aTtest[0] ) )
    {
        *actualDist = 0;
        return false;
    }

    if( TestPointInsidePolygon( aTtest, aTtestCount, aTref[0] ) )
    {
        *actualDist = 0;
        return false;
    }

    for( int ii = 0, jj = aTrefCount - 1; ii < aTrefCount; jj = ii, ii++ )
    {
        // for all edges in aTref
        for( int kk = 0, ll = aTtestCount - 1; kk < aTtestCount; ll = kk, kk++ )
        {
            // for all edges in aTtest
            double d;
            int    intersect = TestForIntersectionOfStraightLineSegments(
                                        aTref[ii].x, aTref[ii].y, aTref[jj].x, aTref[jj].y,
                                        aTtest[kk].x, aTtest[kk].y, aTtest[ll].x, aTtest[ll].y,
                                        nullptr, nullptr, &d );

            if( intersect )
            {
                *actualDist = 0;
                return false;
            }

            if( d < aAllowedDist )
            {
                *actualDist = KiROUND( d );
                return false;
            }
        }
    }

    return true;
}



std::set<test::DRC_RULE_ID_T> test::DRC_TEST_PROVIDER_CLEARANCE::GetMatchingRuleIds() const
{
    return { DRC_RULE_ID_T::DRC_RULE_ID_CLEARANCE };
}

test::DRC_TEST_PROVIDER *drcCreateClearanceTestProvider( test::DRC_ENGINE *engine )
{
    return new test::DRC_TEST_PROVIDER_CLEARANCE( engine );
}