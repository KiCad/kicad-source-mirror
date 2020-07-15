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
#include <drc_proto/drc_item.h>
#include <drc_proto/drc_rule.h>
#include <drc_proto/drc_test_provider_clearance_base.h>

namespace test {

class DRC_TEST_PROVIDER_HOLE_CLEARANCE : public DRC_TEST_PROVIDER_CLEARANCE_BASE
{
public:
    DRC_TEST_PROVIDER_HOLE_CLEARANCE () :
        DRC_TEST_PROVIDER_CLEARANCE_BASE()
        {
        }

    virtual ~DRC_TEST_PROVIDER_HOLE_CLEARANCE() 
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override 
    {
        return "hole_clearance"; 
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests clearance of holes (via/pad drills)";
    }

    virtual std::set<test::DRC_RULE_ID_T> GetMatchingRuleIds() const override;

private:
    bool checkPad( D_PAD* aPad );
    bool checkVia( VIA* aVia );
    bool checkMicroVia( VIA* aVia );

    void addHole( const wxPoint& aLocation, int aRadius, BOARD_ITEM* aOwner );
    bool checkHoles();

    void testPadHoles();
    bool doPadToPadHoleDrc(  D_PAD* aRefPad, D_PAD** aStart, D_PAD** aEnd, int x_limit );

    struct DRILLED_HOLE
    {
        wxPoint     m_location;
        int         m_drillRadius = 0;
        BOARD_ITEM* m_owner = nullptr;
    };

    EDA_UNITS                 m_units;
    BOARD*                    m_board;
    std::vector<DRILLED_HOLE> m_holes;
    int                       m_largestRadius;

};

};


bool test::DRC_TEST_PROVIDER_HOLE_CLEARANCE::Run()
{
    auto bds = m_drcEngine->GetDesignSettings();
    m_board = m_drcEngine->GetBoard();

    m_largestClearance = 0;

    for( auto rule : m_drcEngine->QueryRulesById( test::DRC_RULE_ID_T::DRC_RULE_ID_HOLE_CLEARANCE ) )
    {
        drc_dbg(1, "process rule %p\n", rule );
        if( rule->GetConstraint().m_Value.HasMin() )
        {
            drc_dbg(1, "min-hole-clearance %d\n", rule->GetConstraint().m_Value.Min() );
            m_largestClearance = std::max( m_largestClearance, rule->GetConstraint().m_Value.Min() );
        }
    }

    ReportAux( "Worst hole clearance : %d nm", m_largestClearance );

    ReportStage( ("Testing pad/hole clearances"), 0, 2 );
    testPadHoles();

    return true;
}

void test::DRC_TEST_PROVIDER_HOLE_CLEARANCE::testPadHoles()
{
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
       int x_limit = pad->GetPosition().x + pad->GetBoundingRadius() + max_size;
        drc_dbg(10,"-> %p\n", pad);
       doPadToPadHoleDrc( pad, &pad, listEnd, x_limit );
    }
}


bool test::DRC_TEST_PROVIDER_HOLE_CLEARANCE::doPadToPadHoleDrc(  D_PAD* aRefPad, D_PAD** aStart, D_PAD** aEnd,
                          int x_limit )
{
    const static LSET all_cu = LSET::AllCuMask();

    LSET layerMask = aRefPad->GetLayerSet() & all_cu;


    for( D_PAD** pad_list = aStart;  pad_list<aEnd;  ++pad_list )
    {
        D_PAD* pad = *pad_list;

        if( pad == aRefPad )
            continue;

        

  //      drc_dbg(10," chk against -> %p\n", pad);

        // We can stop the test when pad->GetPosition().x > x_limit
        // because the list is sorted by X values
        if( pad->GetPosition().x > x_limit )
            break;

//        drc_dbg(10," chk2 against -> %p ds %d %d\n", pad, pad->GetDrillSize().x, aRefPad->GetDrillSize().x );
        
        drc_dbg(1," chk1 against -> %p x0 %d x2 %d\n", pad, pad->GetDrillSize().x, aRefPad->GetDrillSize().x );

        // No problem if pads which are on copper layers are on different copper layers,
        // (pads can be only on a technical layer, to build complex pads)
        // but their hole (if any ) can create DRC error because they are on all
        // copper layers, so we test them
        if( ( pad->GetLayerSet() & layerMask ) != 0 &&
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

            drc_dbg(1," chk3 against -> %p x0 %d x2 %d\n", pad, pad->GetDrillSize().x, aRefPad->GetDrillSize().x );

            /* Here, we must test clearance between holes and pads
             * pad size and shape is adjusted to pad drill size and shape
             */
            if( pad->GetDrillSize().x )
            {
                // pad under testing has a hole, test this hole against pad reference

                auto rule = m_drcEngine->EvalRulesForItems( test::DRC_RULE_ID_T::DRC_RULE_ID_HOLE_CLEARANCE, aRefPad, pad );
                auto minClearance = rule->GetConstraint().GetValue().Min();
                int actual;

                drc_dbg(1,"check pad %p rule '%s' cl %d\n", pad, (const char*) rule->GetName().c_str(), minClearance );

                // fixme: pad stacks...
                if( aRefPad->Collide( pad->GetEffectiveHoleShape(), minClearance, &actual ) )
                {
                    DRC_ITEM* drcItem = new DRC_ITEM( DRCE_HOLE_CLEARANCE );

                    wxString msg;

                    msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                                  "",
                                  MessageTextFromValue( userUnits(), minClearance, true ),
                                  MessageTextFromValue( userUnits(), actual, true ) );

                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( pad, aRefPad );
                    drcItem->SetViolatingRule( rule );

                    ReportWithMarker( drcItem, pad->GetPosition() );
                    return false;
                }
            }

            if( aRefPad->GetDrillSize().x ) // pad reference has a hole
            {

                auto rule = m_drcEngine->EvalRulesForItems( test::DRC_RULE_ID_T::DRC_RULE_ID_HOLE_CLEARANCE, aRefPad, pad );
                auto minClearance = rule->GetConstraint().GetValue().Min();
                int actual;

                drc_dbg(1,"check pad %p rule '%s' cl %d\n", aRefPad, (const char*) rule->GetName().c_str(), minClearance );

                if( pad->Collide( aRefPad->GetEffectiveHoleShape(), minClearance, &actual ) )
                {
                    DRC_ITEM* drcItem = new DRC_ITEM( DRCE_HOLE_CLEARANCE );
                    wxString msg;

                    msg.Printf( drcItem->GetErrorText() + _( " (%s clearance %s; actual %s)" ),
                                  "",
                                  MessageTextFromValue( userUnits(), minClearance, true ),
                                  MessageTextFromValue( userUnits(), actual, true ) );

                    drcItem->SetErrorMessage( msg );
                    drcItem->SetItems( aRefPad, pad );
                    drcItem->SetViolatingRule( rule );

                    ReportWithMarker( drcItem, pad->GetPosition() );
                    return false;
                }
            }
        }
    }

    return true;
}

std::set<test::DRC_RULE_ID_T> test::DRC_TEST_PROVIDER_HOLE_CLEARANCE::GetMatchingRuleIds() const
{
    return { DRC_RULE_ID_T::DRC_RULE_ID_HOLE_CLEARANCE };
}


namespace detail
{
    static test::DRC_REGISTER_TEST_PROVIDER<test::DRC_TEST_PROVIDER_HOLE_CLEARANCE> dummy;
}