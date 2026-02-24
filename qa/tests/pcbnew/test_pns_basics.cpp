/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <settings/settings_manager.h>
#include <optional>

#include <pcbnew/board.h>
#include <pcbnew/pad.h>
#include <pcbnew/pcb_track.h>
#include <pcbnew/pcbexpr_evaluator.h>

#include <geometry/shape_circle.h>
#include <router/pns_item.h>
#include <router/pns_kicad_iface.h>
#include <router/pns_node.h>
#include <router/pns_router.h>
#include <router/pns_segment.h>
#include <router/pns_solid.h>
#include <router/pns_via.h>

static bool isCopper( const PNS::ITEM* aItem )
{
    if( !aItem )
        return false;

    BOARD_ITEM* parent = aItem->Parent();

    if( parent && parent->Type() == PCB_PAD_T )
    {
        PAD* pad = static_cast<PAD*>( parent );

        if( pad->IsAperturePad() || pad->IsNPTHWithNoCopper() )
            return false;
    }

    return true;
}


static bool isHole( const PNS::ITEM* aItem )
{
    if( !aItem )
        return false;

    return aItem->OfKind( PNS::ITEM::HOLE_T );
}


static bool isEdge( const PNS::ITEM* aItem )
{
    if( !aItem )
        return false;

    const BOARD_ITEM *parent = aItem->BoardItem();

    return parent && ( parent->IsOnLayer( Edge_Cuts ) || parent->IsOnLayer( Margin ) );
}


class MOCK_RULE_RESOLVER : public PNS::RULE_RESOLVER
{
public:
    MOCK_RULE_RESOLVER() : m_clearanceEpsilon( 10 )
    {
    }

    virtual ~MOCK_RULE_RESOLVER() {}

    virtual int Clearance( const PNS::ITEM* aA, const PNS::ITEM* aB,
                           bool aUseClearanceEpsilon = true ) override
    {
        PNS::CONSTRAINT constraint;
        int             rv = 0;
        PNS_LAYER_RANGE     layers;

        if( !aB )
            layers = aA->Layers();
        else if( isEdge( aA ) )
            layers = aB->Layers();
        else if( isEdge( aB ) )
            layers = aA->Layers();
        else
            layers = aA->Layers().Intersection( aB->Layers() );

        // Normalize layer range (no -1 magic numbers)
        layers = layers.Intersection( PNS_LAYER_RANGE( PCBNEW_LAYER_ID_START, PCB_LAYER_ID_COUNT - 1 ) );

        for( int layer = layers.Start(); layer <= layers.End(); ++layer )
        {
            if( isHole( aA ) && isHole( aB) )
            {
                if( QueryConstraint( PNS::CONSTRAINT_TYPE::CT_HOLE_TO_HOLE, aA, aB, layer, &constraint ) )
                {
                    if( constraint.m_Value.Min() > rv )
                        rv = constraint.m_Value.Min();
                }
            }
            else if( isHole( aA ) || isHole( aB ) )
            {
                if( QueryConstraint( PNS::CONSTRAINT_TYPE::CT_HOLE_CLEARANCE, aA, aB, layer, &constraint ) )
                {
                    if( constraint.m_Value.Min() > rv )
                        rv = constraint.m_Value.Min();
                }
            }
            else if( isCopper( aA ) && ( !aB || isCopper( aB ) ) )
            {
                if( QueryConstraint( PNS::CONSTRAINT_TYPE::CT_CLEARANCE, aA, aB, layer, &constraint ) )
                {
                    if( constraint.m_Value.Min() > rv )
                        rv = constraint.m_Value.Min();
                }
            }
            else if( isEdge( aA ) || ( aB && isEdge( aB ) ) )
            {
                if( QueryConstraint( PNS::CONSTRAINT_TYPE::CT_EDGE_CLEARANCE, aA, aB, layer, &constraint ) )
                {
                    if( constraint.m_Value.Min() > rv )
                        rv = constraint.m_Value.Min();
                }
            }
        }

        return rv;
    }

    virtual PNS::NET_HANDLE DpCoupledNet( PNS::NET_HANDLE aNet ) override { return nullptr; }
    virtual int DpNetPolarity( PNS::NET_HANDLE aNet ) override { return -1; }

    virtual bool DpNetPair( const PNS::ITEM* aItem, PNS::NET_HANDLE& aNetP,
                            PNS::NET_HANDLE& aNetN ) override
    {
        return false;
    }

    virtual int NetCode( PNS::NET_HANDLE aNet ) override
    {
        return -1;
    }

    virtual wxString NetName( PNS::NET_HANDLE aNet ) override
    {
        return wxEmptyString;
    }

    virtual bool QueryConstraint( PNS::CONSTRAINT_TYPE aType, const PNS::ITEM* aItemA,
                                  const PNS::ITEM* aItemB, int aLayer,
                                  PNS::CONSTRAINT* aConstraint ) override
    {
        ITEM_KEY key;

        key.a = aItemA;
        key.b = aItemB;
        key.type = aType;

        auto it = m_ruleMap.find( key );

        if( it == m_ruleMap.end() )
        {
            int cl;
            switch( aType )
            {
            case PNS::CONSTRAINT_TYPE::CT_CLEARANCE:      cl = m_defaultClearance;   break;
            case PNS::CONSTRAINT_TYPE::CT_HOLE_TO_HOLE:   cl = m_defaultHole2Hole;   break;
            case PNS::CONSTRAINT_TYPE::CT_HOLE_CLEARANCE: cl = m_defaultHole2Copper; break;
            default: return false;
            }

            //printf("GetDef %s %s %d cl %d\n", aItemA->KindStr().c_str(), aItemB->KindStr().c_str(), aType, cl );

            aConstraint->m_Type = aType;
            aConstraint->m_Value.SetMin( cl );

            return true;
        }
        else
        {
            *aConstraint = it->second;
        }

        return true;
    }

    int ClearanceEpsilon() const override { return m_clearanceEpsilon; }

    struct ITEM_KEY
    {
        const PNS::ITEM*     a = nullptr;
        const PNS::ITEM*     b = nullptr;
        PNS::CONSTRAINT_TYPE type;

        bool operator==( const ITEM_KEY& other ) const
        {
            return a == other.a && b == other.b && type == other.type;
        }

        bool operator<( const ITEM_KEY& other ) const
        {
            if( a < other.a )
            {
                return true;
            }
            else if ( a == other.a )
            {
                if( b < other.b )
                    return true;
                else if ( b == other.b )
                    return type < other.type;
            }

            return false;
        }
    };

    bool IsInNetTie( const PNS::ITEM* aA ) override { return false; }

    bool IsNetTieExclusion( const PNS::ITEM* aItem, const VECTOR2I& aCollisionPos,
                            const PNS::ITEM* aCollidingItem ) override
    {
        return false;
    }

    bool IsDrilledHole( const PNS::ITEM* aItem ) override { return false; }

    bool IsNonPlatedSlot( const PNS::ITEM* aItem ) override { return false; }

    bool IsKeepout( const PNS::ITEM* aObstacle, const PNS::ITEM* aItem, bool* aEnforce ) override
    {
        return false;
    }

    void AddMockRule( PNS::CONSTRAINT_TYPE aType, const PNS::ITEM* aItemA, const PNS::ITEM* aItemB,
                      PNS::CONSTRAINT& aConstraint )
    {
        ITEM_KEY key;

        key.a = aItemA;
        key.b = aItemB;
        key.type = aType;

        m_ruleMap[key] = aConstraint;
    }

    int m_defaultClearance = 200000;
    int m_defaultHole2Hole = 220000;
    int m_defaultHole2Copper = 210000;

private:
    std::map<ITEM_KEY, PNS::CONSTRAINT> m_ruleMap;
    int                                 m_clearanceEpsilon;
};

struct PNS_TEST_FIXTURE;

class MOCK_PNS_KICAD_IFACE : public PNS_KICAD_IFACE_BASE
{
public:
    MOCK_PNS_KICAD_IFACE( PNS_TEST_FIXTURE *aFixture ) :
        m_testFixture( aFixture )
    {}

    ~MOCK_PNS_KICAD_IFACE() override {}

    void HideItem( PNS::ITEM* aItem ) override {};
    void DisplayItem( const PNS::ITEM* aItem, int aClearance, bool aEdit = false,
                      int aFlags = 0 ) override {};
    PNS::RULE_RESOLVER* GetRuleResolver() override;

    bool TestInheritTrackWidth( PNS::ITEM* aItem, int* aInheritedWidth,
                                const VECTOR2I& aStartPosition = VECTOR2I() )
    {
        m_startLayer = aItem->Layer();
        return inheritTrackWidth( aItem, aInheritedWidth, aStartPosition );
    }

private:
    PNS_TEST_FIXTURE* m_testFixture;
};


struct PNS_TEST_FIXTURE
{
    PNS_TEST_FIXTURE()
    {
        m_router = new PNS::ROUTER;
        m_iface = new MOCK_PNS_KICAD_IFACE( this );
        m_router->SetInterface( m_iface );
    }

    SETTINGS_MANAGER      m_settingsManager;
    PNS::ROUTER*          m_router;
    MOCK_RULE_RESOLVER    m_ruleResolver;
    MOCK_PNS_KICAD_IFACE* m_iface;
    //std::unique_ptr<BOARD> m_board;
};


PNS::RULE_RESOLVER* MOCK_PNS_KICAD_IFACE::GetRuleResolver()
{
    return &m_testFixture->m_ruleResolver;
}

static void dumpObstacles( const PNS::NODE::OBSTACLES &obstacles )
{
    for( const PNS::OBSTACLE& obs : obstacles )
    {
        BOOST_TEST_MESSAGE( wxString::Format( "%p [%s] - %p [%s], clearance %d",
                obs.m_head, obs.m_head->KindStr().c_str(),
                obs.m_item, obs.m_item->KindStr().c_str(),
                obs.m_clearance ) );
    }
}

BOOST_FIXTURE_TEST_CASE( PNSHoleCollisions, PNS_TEST_FIXTURE )
{
    PNS::VIA* v1 = new PNS::VIA( VECTOR2I( 0, 1000000 ), PNS_LAYER_RANGE( F_Cu, B_Cu ), 50000, 10000 );
    PNS::VIA* v2 = new PNS::VIA( VECTOR2I( 0, 2000000 ), PNS_LAYER_RANGE( F_Cu, B_Cu ), 50000, 10000 );

    std::unique_ptr<PNS::NODE> world ( new PNS::NODE );

    v1->SetNet( (PNS::NET_HANDLE) 1 );
    v2->SetNet( (PNS::NET_HANDLE) 2 );

    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    world->AddRaw( v1 );
    world->AddRaw( v2 );

    BOOST_TEST_MESSAGE( "via to via, no violations" );
    {
        PNS::NODE::OBSTACLES obstacles;
        int count = world->QueryColliding( v1, obstacles );
        dumpObstacles( obstacles );
        BOOST_CHECK_EQUAL( obstacles.size(), 0 );
        BOOST_CHECK_EQUAL( count, 0 );
    }

    BOOST_TEST_MESSAGE( "via to via, forced copper to copper violation" );
    {
        PNS::NODE::OBSTACLES obstacles;
        m_ruleResolver.m_defaultClearance = 1000000;
        world->QueryColliding( v1, obstacles );
        dumpObstacles( obstacles );

        BOOST_CHECK_EQUAL( obstacles.size(), 1 );
        const auto& first = *obstacles.begin();

        BOOST_CHECK_EQUAL( first.m_head, v1 );
        BOOST_CHECK_EQUAL( first.m_item, v2 );
        BOOST_CHECK_EQUAL( first.m_clearance, m_ruleResolver.m_defaultClearance );
    }

    BOOST_TEST_MESSAGE( "via to via, forced hole to hole violation" );
    {
        PNS::NODE::OBSTACLES obstacles;
        m_ruleResolver.m_defaultClearance = 200000;
        m_ruleResolver.m_defaultHole2Hole = 1000000;

        world->QueryColliding( v1, obstacles );
        dumpObstacles( obstacles );

        BOOST_CHECK_EQUAL( obstacles.size(), 1 );
        auto iter = obstacles.begin();
        const auto& first = *iter++;

        BOOST_CHECK_EQUAL( first.m_head, v1->Hole() );
        BOOST_CHECK_EQUAL( first.m_item, v2->Hole() );
        BOOST_CHECK_EQUAL( first.m_clearance, m_ruleResolver.m_defaultHole2Hole );
    }

    BOOST_TEST_MESSAGE( "via to via, forced copper to hole violation" );
    {
        PNS::NODE::OBSTACLES obstacles;
        m_ruleResolver.m_defaultHole2Hole = 220000;
        m_ruleResolver.m_defaultHole2Copper = 1000000;

        world->QueryColliding( v1, obstacles );
        dumpObstacles( obstacles );

        BOOST_CHECK_EQUAL( obstacles.size(), 2 );
        auto iter = obstacles.begin();
        const auto& first = *iter++;

        // There is no guarantee on what order the two collisions will be in...
        BOOST_CHECK( ( first.m_head == v1 && first.m_item == v2->Hole() )
                  || ( first.m_head == v1->Hole() && first.m_item == v2 ) );

        BOOST_CHECK_EQUAL( first.m_clearance, m_ruleResolver.m_defaultHole2Copper );
    }
}


BOOST_FIXTURE_TEST_CASE( PNSViaBackdrillRetention, PNS_TEST_FIXTURE )
{
    PNS::VIA via( VECTOR2I( 1000, 2000 ), PNS_LAYER_RANGE( F_Cu, B_Cu ), 40000, 20000, nullptr,
                  VIATYPE::THROUGH );
    via.SetHoleLayers( PNS_LAYER_RANGE( F_Cu, In2_Cu ) );
    via.SetHolePostMachining( std::optional<PAD_DRILL_POST_MACHINING_MODE>( PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK ) );
    via.SetSecondaryDrill( std::optional<int>( 12000 ) );
    via.SetSecondaryHoleLayers( std::optional<PNS_LAYER_RANGE>( PNS_LAYER_RANGE( F_Cu, In1_Cu ) ) );
    via.SetSecondaryHolePostMachining( std::optional<PAD_DRILL_POST_MACHINING_MODE>( PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED ) );

    PNS::VIA viaCopy( via );
    std::unique_ptr<PNS::VIA> viaClone( via.Clone() );

    auto checkVia = [&]( const PNS::VIA& candidate )
    {
        BOOST_CHECK_EQUAL( candidate.HoleLayers().Start(), via.HoleLayers().Start() );
        BOOST_CHECK_EQUAL( candidate.HoleLayers().End(), via.HoleLayers().End() );
        BOOST_CHECK( candidate.HolePostMachining().has_value() );
        BOOST_CHECK( candidate.HolePostMachining().value() == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK );
        BOOST_CHECK( candidate.SecondaryDrill().has_value() );
        BOOST_CHECK_EQUAL( candidate.SecondaryDrill().value(), via.SecondaryDrill().value() );
        BOOST_CHECK( candidate.SecondaryHoleLayers().has_value() );
        BOOST_CHECK_EQUAL( candidate.SecondaryHoleLayers()->Start(),
                           via.SecondaryHoleLayers()->Start() );
        BOOST_CHECK_EQUAL( candidate.SecondaryHoleLayers()->End(),
                           via.SecondaryHoleLayers()->End() );
        BOOST_CHECK( candidate.SecondaryHolePostMachining().has_value() );

        // run this BOOST_CHECK only if possible to avoid crash
        if( candidate.SecondaryHolePostMachining().has_value() )
            BOOST_CHECK( candidate.SecondaryHolePostMachining().value() == via.SecondaryHolePostMachining().value() );
    };

    checkVia( viaCopy );
    checkVia( *viaClone );
}


BOOST_AUTO_TEST_CASE( PCBViaBackdrillCloneRetainsData )
{
    BOARD board;
    PCB_VIA via( &board );

    via.SetPrimaryDrillStartLayer( F_Cu );
    via.SetPrimaryDrillEndLayer( B_Cu );
    via.SetFrontPostMachining( std::optional<PAD_DRILL_POST_MACHINING_MODE>( PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK ) );
    via.SetSecondaryDrillSize( std::optional<int>( 15000 ) );
    via.SetSecondaryDrillStartLayer( F_Cu );
    via.SetSecondaryDrillEndLayer( In2_Cu );

    via.SetBackPostMachining( std::optional<PAD_DRILL_POST_MACHINING_MODE>( PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE ) );
    via.SetTertiaryDrillSize( std::optional<int>( 8000 ) );
    via.SetTertiaryDrillStartLayer( B_Cu );
    via.SetTertiaryDrillEndLayer( In4_Cu );

    PCB_VIA viaCopy( via );
    std::unique_ptr<PCB_VIA> viaClone( static_cast<PCB_VIA*>( via.Clone() ) );

    auto checkVia = [&]( const PCB_VIA& candidate )
    {
        BOOST_CHECK_EQUAL( candidate.GetPrimaryDrillStartLayer(), via.GetPrimaryDrillStartLayer() );
        BOOST_CHECK_EQUAL( candidate.GetPrimaryDrillEndLayer(), via.GetPrimaryDrillEndLayer() );
        BOOST_CHECK( candidate.GetFrontPostMachining().has_value() );
        BOOST_CHECK_EQUAL( static_cast<int>( candidate.GetFrontPostMachining().value() ),
                           static_cast<int>( via.GetFrontPostMachining().value() ) );
        BOOST_CHECK( candidate.GetSecondaryDrillSize().has_value() );
        BOOST_CHECK_EQUAL( candidate.GetSecondaryDrillSize().value(),
                           via.GetSecondaryDrillSize().value() );
        BOOST_CHECK_EQUAL( candidate.GetSecondaryDrillStartLayer(),
                           via.GetSecondaryDrillStartLayer() );
        BOOST_CHECK_EQUAL( candidate.GetSecondaryDrillEndLayer(),
                           via.GetSecondaryDrillEndLayer() );

        BOOST_CHECK( candidate.GetBackPostMachining().has_value() );
        BOOST_CHECK_EQUAL( static_cast<int>( candidate.GetBackPostMachining().value() ),
                           static_cast<int>( via.GetBackPostMachining().value() ) );
        BOOST_CHECK( candidate.GetTertiaryDrillSize().has_value() );
        BOOST_CHECK_EQUAL( candidate.GetTertiaryDrillSize().value(),
                           via.GetTertiaryDrillSize().value() );
        BOOST_CHECK_EQUAL( candidate.GetTertiaryDrillStartLayer(),
                           via.GetTertiaryDrillStartLayer() );
        BOOST_CHECK_EQUAL( candidate.GetTertiaryDrillEndLayer(),
                           via.GetTertiaryDrillEndLayer() );
    };

    checkVia( viaCopy );
    checkVia( *viaClone );
}


/**
 * Test that PNS_LAYER_RANGE(1, 0) is swapped to (0, 1).
 *
 * This is a minimal regression test for https://gitlab.com/kicad/code/kicad/-/issues/20355
 * The actual fix is in pns_kicad_iface.cpp syncPad() which skips creating an
 * INNER_LAYERS SOLID on 2-layer boards. This test verifies the layer range behavior
 * that motivated the fix.
 */
BOOST_AUTO_TEST_CASE( PNSLayerRangeSwapBehavior )
{
    // On a 2-layer board with FRONT_INNER_BACK mode, BoardCopperLayerCount() returns 2.
    // The code would calculate PNS_LAYER_RANGE(1, 2 - 2) = PNS_LAYER_RANGE(1, 0)
    // Since start > end, the constructor swaps them to (0, 1), which would span
    // both F_Cu and B_Cu incorrectly.

    PNS_LAYER_RANGE innerLayersRange2Layer( 1, 0 );  // What would happen on 2-layer board

    // Verify the swap behavior that causes the bug
    BOOST_CHECK_EQUAL( innerLayersRange2Layer.Start(), 0 );
    BOOST_CHECK_EQUAL( innerLayersRange2Layer.End(), 1 );
    BOOST_CHECK( innerLayersRange2Layer.Overlaps( 0 ) );  // F_Cu
    BOOST_CHECK( innerLayersRange2Layer.Overlaps( 1 ) );  // B_Cu

    // On a 4-layer board, inner layers are 1 and 2, so PNS_LAYER_RANGE(1, 4-2) = (1, 2)
    PNS_LAYER_RANGE innerLayersRange4Layer( 1, 2 );  // Correct for 4-layer board

    BOOST_CHECK_EQUAL( innerLayersRange4Layer.Start(), 1 );
    BOOST_CHECK_EQUAL( innerLayersRange4Layer.End(), 2 );
    BOOST_CHECK( !innerLayersRange4Layer.Overlaps( 0 ) ); // F_Cu - should not overlap
    BOOST_CHECK( innerLayersRange4Layer.Overlaps( 1 ) );  // In1_Cu
    BOOST_CHECK( innerLayersRange4Layer.Overlaps( 2 ) );  // In2_Cu
    BOOST_CHECK( !innerLayersRange4Layer.Overlaps( 3 ) ); // B_Cu - should not overlap
}


/**
 * Test that splitting a locked PNS segment preserves the locked marker on both halves.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/21564
 * When a locked track is split by a new route connecting to its middle, both resulting
 * segments must retain the locked state.
 */
BOOST_FIXTURE_TEST_CASE( PNSSegmentSplitPreservesLockedState, PNS_TEST_FIXTURE )
{
    std::unique_ptr<PNS::NODE> world( new PNS::NODE );
    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    PNS::NET_HANDLE net = (PNS::NET_HANDLE) 1;

    VECTOR2I segStart( 0, 0 );
    VECTOR2I segEnd( 10000000, 0 );
    VECTOR2I splitPt( 5000000, 0 );

    PNS::SEGMENT* lockedSeg = new PNS::SEGMENT( SEG( segStart, segEnd ), net );
    lockedSeg->SetWidth( 250000 );
    lockedSeg->SetLayers( PNS_LAYER_RANGE( F_Cu ) );
    lockedSeg->Mark( PNS::MK_LOCKED );

    BOOST_CHECK( lockedSeg->IsLocked() );

    world->AddRaw( lockedSeg );

    // Clone the locked segment and set up two halves (simulating SplitAdjacentSegments)
    std::unique_ptr<PNS::SEGMENT> clone1( PNS::Clone( *lockedSeg ) );
    std::unique_ptr<PNS::SEGMENT> clone2( PNS::Clone( *lockedSeg ) );

    clone1->SetEnds( segStart, splitPt );
    clone2->SetEnds( splitPt, segEnd );

    BOOST_CHECK_MESSAGE( clone1->IsLocked(),
                         "First half of split locked segment must retain locked state" );
    BOOST_CHECK_MESSAGE( clone2->IsLocked(),
                         "Second half of split locked segment must retain locked state" );
    BOOST_CHECK_EQUAL( clone1->Width(), lockedSeg->Width() );
    BOOST_CHECK_EQUAL( clone2->Width(), lockedSeg->Width() );
}


/**
 * Test that inheritTrackWidth selects the correct track based on cursor proximity.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/19123
 * When a pad has multiple tracks of different widths, the router should inherit
 * the width of the track closest to the cursor, not just the minimum width.
 */
BOOST_FIXTURE_TEST_CASE( PNSInheritTrackWidthCursorProximity, PNS_TEST_FIXTURE )
{
    std::unique_ptr<PNS::NODE> world( new PNS::NODE );
    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    VECTOR2I padPos( 0, 0 );
    PNS::NET_HANDLE net = (PNS::NET_HANDLE) 1;

    // Pad at origin with a small circular shape
    PNS::SOLID* pad = new PNS::SOLID;
    pad->SetShape( new SHAPE_CIRCLE( padPos, 500000 ) );
    pad->SetPos( padPos );
    pad->SetLayers( PNS_LAYER_RANGE( F_Cu ) );
    pad->SetNet( net );
    world->AddRaw( pad );

    // Narrow track going right (250um width)
    int narrowWidth = 250000;
    PNS::SEGMENT* narrowSeg = new PNS::SEGMENT( SEG( padPos, VECTOR2I( 5000000, 0 ) ), net );
    narrowSeg->SetWidth( narrowWidth );
    narrowSeg->SetLayers( PNS_LAYER_RANGE( F_Cu ) );
    world->AddRaw( narrowSeg );

    // Wide track going up (500um width)
    int wideWidth = 500000;
    PNS::SEGMENT* wideSeg = new PNS::SEGMENT( SEG( padPos, VECTOR2I( 0, -5000000 ) ), net );
    wideSeg->SetWidth( wideWidth );
    wideSeg->SetLayers( PNS_LAYER_RANGE( F_Cu ) );
    world->AddRaw( wideSeg );

    int inherited = 0;

    // Without cursor position, should fall back to minimum width
    BOOST_CHECK( m_iface->TestInheritTrackWidth( pad, &inherited ) );
    BOOST_CHECK_EQUAL( inherited, narrowWidth );

    // Cursor near the narrow track (to the right) should select narrow width
    inherited = 0;
    BOOST_CHECK( m_iface->TestInheritTrackWidth( pad, &inherited, VECTOR2I( 2000000, 0 ) ) );
    BOOST_CHECK_EQUAL( inherited, narrowWidth );

    // Cursor near the wide track (upward) should select wide width
    inherited = 0;
    BOOST_CHECK( m_iface->TestInheritTrackWidth( pad, &inherited, VECTOR2I( 0, -2000000 ) ) );
    BOOST_CHECK_EQUAL( inherited, wideWidth );

    // Cursor slightly offset toward narrow track should still select narrow
    inherited = 0;
    BOOST_CHECK( m_iface->TestInheritTrackWidth( pad, &inherited, VECTOR2I( 100000, 50000 ) ) );
    BOOST_CHECK_EQUAL( inherited, narrowWidth );

    // Cursor slightly offset toward wide track should select wide
    inherited = 0;
    BOOST_CHECK( m_iface->TestInheritTrackWidth( pad, &inherited, VECTOR2I( 50000, -100000 ) ) );
    BOOST_CHECK_EQUAL( inherited, wideWidth );
}


/**
 * Test that PCBEXPR_UCODE correctly identifies geometry-dependent functions during compilation.
 *
 * Regression test for PNS track drag performance. The geometry-dependent flag controls whether
 * segment-by-segment DRC evaluation is performed during routing. When no geometry-dependent
 * functions exist in clearance rules, the expensive per-segment evaluation is skipped.
 */
BOOST_AUTO_TEST_CASE( PCBExprGeometryDependentFunctionDetection )
{
    PCBEXPR_COMPILER compiler( new PCBEXPR_UNIT_RESOLVER() );

    auto compileAndCheck = [&]( const wxString& aExpr, bool aExpectGeometry )
    {
        PCBEXPR_UCODE   ucode;
        PCBEXPR_CONTEXT ctx( 0, F_Cu );

        bool ok = compiler.Compile( aExpr.ToUTF8().data(), &ucode, &ctx );
        BOOST_CHECK_MESSAGE( ok, "Failed to compile: " + aExpr );

        if( ok )
        {
            BOOST_CHECK_MESSAGE( ucode.HasGeometryDependentFunctions() == aExpectGeometry,
                                 wxString::Format( "Expression '%s': expected geometry=%s, got %s",
                                                   aExpr,
                                                   aExpectGeometry ? "true" : "false",
                                                   ucode.HasGeometryDependentFunctions()
                                                           ? "true" : "false" ) );
        }
    };

    // Property-based conditions should NOT be geometry-dependent
    compileAndCheck( wxT( "A.NetClass == 'Power'" ), false );
    compileAndCheck( wxT( "A.Type == 'via'" ), false );
    compileAndCheck( wxT( "A.NetName == '/VCC'" ), false );

    // Geometry-dependent functions SHOULD be detected
    compileAndCheck( wxT( "A.intersectsCourtyard('U1')" ), true );
    compileAndCheck( wxT( "A.intersectsArea('Zone1')" ), true );
    compileAndCheck( wxT( "A.enclosedByArea('Zone1')" ), true );
    compileAndCheck( wxT( "A.intersectsFrontCourtyard('U1')" ), true );
    compileAndCheck( wxT( "A.intersectsBackCourtyard('U1')" ), true );

    // Deprecated aliases should also be detected
    compileAndCheck( wxT( "A.insideCourtyard('U1')" ), true );
    compileAndCheck( wxT( "A.insideArea('Zone1')" ), true );
}

