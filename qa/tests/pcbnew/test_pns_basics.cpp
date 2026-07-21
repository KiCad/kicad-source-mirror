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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <settings/settings_manager.h>
#include <optional>

#include <pcbnew/board.h>
#include <pcbnew/pad.h>
#include <pcbnew/pcb_track.h>
#include <pcbnew/pcbexpr_evaluator.h>

#include <geometry/shape_circle.h>
#include <geometry/shape_arc.h>
#include <geometry/eda_angle.h>
#include <router/pns_component_dragger.h>
#include <router/pns_dragger.h>
#include <router/pns_routing_settings.h>
#include <router/pns_arc.h>
#include <router/pns_line.h>
#include <router/pns_item.h>
#include <router/pns_kicad_iface.h>
#include <router/pns_node.h>
#include <router/pns_router.h>
#include <router/pns_segment.h>
#include <router/pns_shove.h>
#include <router/pns_sizes_settings.h>
#include <router/pns_solid.h>
#include <router/pns_topology.h>
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

        // electrical clearances are net-aware; physical clearances are net-blind; same-net or
        // free-pad pairs with no positive physical rule fall back to -1.
        const bool sameNet = aA && aB && aA->Net() && aA->Net() == aB->Net();
        const bool freePad = aA && aB && ( aA->IsFreePad() || aB->IsFreePad() );

        for( int layer = layers.Start(); layer <= layers.End(); ++layer )
        {
            if( !sameNet && !freePad )
            {
                if( isHole( aA ) && isHole( aB ) )
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

            if( isHole( aA ) || isHole( aB ) )
            {
                if( QueryConstraint( PNS::CONSTRAINT_TYPE::CT_PHYSICAL_HOLE_CLEARANCE, aA, aB, layer, &constraint ) )
                {
                    if( constraint.m_Value.Min() > rv )
                        rv = constraint.m_Value.Min();
                }
            }

            if( QueryConstraint( PNS::CONSTRAINT_TYPE::CT_PHYSICAL_CLEARANCE, aA, aB, layer, &constraint ) )
            {
                if( constraint.m_Value.Min() > rv )
                    rv = constraint.m_Value.Min();
            }
        }

        if( ( sameNet || freePad ) && rv == 0 )
            rv = -1;

        return rv;
    }

    bool HasUserDefinedPhysicalConstraint() override { return m_hasUserPhysicalRules; }

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
            case PNS::CONSTRAINT_TYPE::CT_PHYSICAL_CLEARANCE:
                if( m_defaultPhysicalClearance == 0 )
                    return false;
                cl = m_defaultPhysicalClearance;
                break;
            case PNS::CONSTRAINT_TYPE::CT_PHYSICAL_HOLE_CLEARANCE:
                if( m_defaultPhysicalHoleClearance == 0 )
                    return false;
                cl = m_defaultPhysicalHoleClearance;
                break;
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

    int  m_defaultClearance = 200000;
    int  m_defaultHole2Hole = 220000;
    int  m_defaultHole2Copper = 210000;
    int  m_defaultPhysicalClearance = 0;     // 0 means "rule does not match this pair"
    int  m_defaultPhysicalHoleClearance = 0; // 0 means "rule does not match this pair"
    bool m_hasUserPhysicalRules = false;

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

    ~PNS_TEST_FIXTURE()
    {
        delete m_router;
        delete m_iface;
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


BOOST_FIXTURE_TEST_CASE( PNSShoveOwnsRootLineHistory, PNS_TEST_FIXTURE )
{
    PNS::NODE world;
    world.SetRuleResolver( &m_ruleResolver );

    PNS::SEGMENT segment( SEG( VECTOR2I( 0, 0 ), VECTOR2I( 1000000, 0 ) ),
                          reinterpret_cast<PNS::NET_HANDLE>( 1 ) );
    segment.SetLayers( PNS_LAYER_RANGE( F_Cu ) );

    PNS::LINE line;
    line.Line().Append( VECTOR2I( 0, 0 ) );
    line.Line().Append( VECTOR2I( 1000000, 0 ) );
    line.SetLayers( PNS_LAYER_RANGE( F_Cu ) );

    PNS::SHOVE shove( &world, m_router );
    shove.SetShovePolicy( &segment, PNS::SHOVE::SHP_SHOVE );
    shove.SetShovePolicy( line, PNS::SHOVE::SHP_SHOVE );
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


/**
 * Test that collideSimple handles items with null shapes gracefully.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23851
 * When ITEM::Shape() returns nullptr, collision checking must not crash.
 * The syncPad fix prevents null-shape SOLIDs from entering the PNS world,
 * but collideSimple should also be defensive.
 */
BOOST_FIXTURE_TEST_CASE( PNSCollideSimpleNullShapeGuard, PNS_TEST_FIXTURE )
{
    std::unique_ptr<PNS::NODE> world( new PNS::NODE );
    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    PNS::NET_HANDLE net1 = (PNS::NET_HANDLE) 1;
    PNS::NET_HANDLE net2 = (PNS::NET_HANDLE) 2;

    PNS::SOLID* solid1 = new PNS::SOLID;
    solid1->SetShape( new SHAPE_CIRCLE( VECTOR2I( 0, 0 ), 500000 ) );
    solid1->SetPos( VECTOR2I( 0, 0 ) );
    solid1->SetLayers( PNS_LAYER_RANGE( F_Cu ) );
    solid1->SetNet( net1 );

    PNS::SOLID* solid2 = new PNS::SOLID;
    solid2->SetShape( new SHAPE_CIRCLE( VECTOR2I( 100000, 0 ), 500000 ) );
    solid2->SetPos( VECTOR2I( 100000, 0 ) );
    solid2->SetLayers( PNS_LAYER_RANGE( F_Cu ) );
    solid2->SetNet( net2 );

    world->AddRaw( solid1 );
    world->AddRaw( solid2 );

    // Verify collision works normally with valid shapes
    PNS::NODE::OBSTACLES obstacles;
    int count = world->QueryColliding( solid1, obstacles );
    BOOST_CHECK( count > 0 );

    // Exercise the null-shape guard in collideSimple by calling Collide() directly with a
    // null-shape solid as the head item. This bypasses the spatial index (which requires a
    // valid shape for bounding-box computation) and hits the exact code path the guard protects.
    PNS::SOLID nullShapeSolid;
    nullShapeSolid.SetPos( VECTOR2I( 0, 0 ) );
    nullShapeSolid.SetLayers( PNS_LAYER_RANGE( F_Cu ) );
    nullShapeSolid.SetNet( net2 );

    bool collided = solid1->Collide( &nullShapeSolid, world.get(), F_Cu, nullptr );
    BOOST_CHECK( !collided );
}


/**
 * Test that COMPONENT_DRAGGER works correctly with basic solid dragging.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23851
 * Exercises the component dragger Start() and multiple Drag() calls to ensure
 * no crashes occur during the drag sequence. This simulates the core PNS
 * operations that happen during an InlineDrag (G key) of a footprint.
 */
BOOST_FIXTURE_TEST_CASE( PNSComponentDraggerBasicDrag, PNS_TEST_FIXTURE )
{
    std::unique_ptr<PNS::NODE> world( new PNS::NODE );
    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    PNS::NET_HANDLE net1 = (PNS::NET_HANDLE) 1;

    VECTOR2I pad1Pos( 0, 0 );

    PNS::SOLID* pad1 = new PNS::SOLID;
    pad1->SetShape( new SHAPE_CIRCLE( pad1Pos, 500000 ) );
    pad1->SetPos( pad1Pos );
    pad1->SetLayers( PNS_LAYER_RANGE( F_Cu ) );
    pad1->SetNet( net1 );
    pad1->SetRoutable( true );

    VECTOR2I traceEnd( 2500000, 2500000 );
    PNS::SEGMENT* trace = new PNS::SEGMENT( SEG( pad1Pos, traceEnd ), net1 );
    trace->SetWidth( 250000 );
    trace->SetLayers( PNS_LAYER_RANGE( F_Cu ) );

    world->AddRaw( pad1 );
    world->AddRaw( trace );

    PNS::COMPONENT_DRAGGER dragger( m_router );
    dragger.SetWorld( world.get() );

    PNS::ITEM_SET itemsToDrag;
    itemsToDrag.Add( pad1 );

    bool started = dragger.Start( pad1Pos, itemsToDrag );
    BOOST_REQUIRE( started );

    // Simulate multiple drag events (mimics mouse movement during drag)
    VECTOR2I dragPositions[] = {
        VECTOR2I( 100000, 100000 ),
        VECTOR2I( 500000, 500000 ),
        VECTOR2I( 1000000, 1000000 ),
        VECTOR2I( 500000, 200000 ),
        VECTOR2I( 0, 0 )
    };

    for( const VECTOR2I& pos : dragPositions )
    {
        bool dragOk = dragger.Drag( pos );
        BOOST_CHECK( dragOk );

        PNS::NODE* currentNode = dragger.CurrentNode();
        BOOST_CHECK( currentNode != nullptr );
    }

    // Verify the dragged items set is populated
    PNS::ITEM_SET traces = dragger.Traces();
    BOOST_CHECK( traces.Size() > 0 );

    // Clean up branch nodes before the world is destroyed
    world->KillChildren();
}


// Dragging the apex of an isolated arc outward keeps a single arc in the chain and
// changes its radius. Exercises the LINE::DragArc geometric core added for in-router
// arc dragging.
BOOST_AUTO_TEST_CASE( PNSLineDragArcResize )
{
    // 90-degree CCW arc, centre at origin, radius 1mm.
    SHAPE_ARC arc( VECTOR2I( 0, 0 ), VECTOR2I( 1000000, 0 ), EDA_ANGLE( 90, DEGREES_T ), 250000 );

    SHAPE_LINE_CHAIN chain;
    chain.SetWidth( 250000 );
    chain.Append( arc );

    PNS::LINE line;
    line.SetWidth( 250000 );
    line.Line() = chain;

    BOOST_REQUIRE_EQUAL( line.CLine().ArcCount(), 1 );

    double oldRadius = line.CLine().CArcs()[0].GetRadius();
    int    apexIdx = line.CLine().PointCount() / 2;

    // Pull the apex toward the tangent corner at (1mm, 1mm) to grow the radius.
    line.DragArc( VECTOR2I( 950000, 950000 ), apexIdx );

    BOOST_CHECK_EQUAL( line.CLine().ArcCount(), 1 );
    BOOST_CHECK( line.CLine().PointCount() >= 2 );
    BOOST_CHECK( line.CLine().CArcs()[0].GetRadius() != oldRadius );
}


// Driving the arc endpoints together collapses the arc out of the chain (the
// "collapse to nothing drops it from the route" path in LINE::DragArc).
BOOST_AUTO_TEST_CASE( PNSLineDragArcCollapse )
{
    SHAPE_ARC arc( VECTOR2I( 0, 0 ), VECTOR2I( 1000000, 0 ), EDA_ANGLE( 90, DEGREES_T ), 250000 );

    SHAPE_LINE_CHAIN chain;
    chain.SetWidth( 250000 );
    chain.Append( arc );

    PNS::LINE line;
    line.SetWidth( 250000 );
    line.Line() = chain;

    BOOST_REQUIRE_EQUAL( line.CLine().ArcCount(), 1 );

    // Drag almost onto the tangent corner at (1mm, 1mm), shrinking the arc until its
    // endpoints fall within the keep-track threshold and the arc is dropped. Staying a
    // hair off the corner keeps the constructed radius positive.
    line.DragArc( VECTOR2I( 999950, 999950 ), line.CLine().PointCount() / 2 );

    BOOST_CHECK_EQUAL( line.CLine().ArcCount(), 0 );
}


// An arc whose central angle reaches 180 degrees cannot be dragged, and the refusal
// is reported through the router's failure reason so the UI can surface it.
BOOST_FIXTURE_TEST_CASE( PNSDragArcRejectsNear180, PNS_TEST_FIXTURE )
{
    PNS::ROUTING_SETTINGS settings( nullptr, "" );
    m_router->LoadSettings( &settings );

    PNS::NET_HANDLE net = (PNS::NET_HANDLE) 1;

    auto makeArc = [&]( const EDA_ANGLE& aAngle ) -> PNS::ARC*
    {
        SHAPE_ARC sa( VECTOR2I( 0, 0 ), VECTOR2I( 1000000, 0 ), aAngle, 250000 );
        PNS::ARC* a = new PNS::ARC( sa, net );
        a->SetLayers( PNS_LAYER_RANGE( F_Cu ) );
        return a;
    };

    // Shallow arc: drag starts and no failure is reported.
    {
        std::unique_ptr<PNS::NODE> world( new PNS::NODE );
        world->SetMaxClearance( 10000000 );
        world->SetRuleResolver( &m_ruleResolver );

        PNS::ARC* arc = makeArc( EDA_ANGLE( 90, DEGREES_T ) );
        world->AddRaw( arc );

        PNS::DRAGGER dragger( m_router );
        dragger.SetWorld( world.get() );
        dragger.SetMode( PNS::DM_ARC );

        PNS::ITEM_SET items;
        items.Add( arc );

        m_router->SetFailureReason( wxEmptyString );
        BOOST_CHECK( dragger.Start( arc->Anchor( 0 ), items ) );
        BOOST_CHECK( m_router->FailureReason().IsEmpty() );

        world->KillChildren();
    }

    // Major arc (>= 180 deg): drag is refused with an explanatory failure reason.
    {
        std::unique_ptr<PNS::NODE> world( new PNS::NODE );
        world->SetMaxClearance( 10000000 );
        world->SetRuleResolver( &m_ruleResolver );

        PNS::ARC* arc = makeArc( EDA_ANGLE( 270, DEGREES_T ) );
        world->AddRaw( arc );

        PNS::DRAGGER dragger( m_router );
        dragger.SetWorld( world.get() );
        dragger.SetMode( PNS::DM_ARC );

        PNS::ITEM_SET items;
        items.Add( arc );

        m_router->SetFailureReason( wxEmptyString );
        BOOST_CHECK( !dragger.Start( arc->Anchor( 0 ), items ) );
        BOOST_CHECK( !m_router->FailureReason().IsEmpty() );

        world->KillChildren();
    }

    // Clockwise major arc (negative central angle): the magnitude is what matters, so
    // it must be refused just like its CCW counterpart.
    {
        std::unique_ptr<PNS::NODE> world( new PNS::NODE );
        world->SetMaxClearance( 10000000 );
        world->SetRuleResolver( &m_ruleResolver );

        PNS::ARC* arc = makeArc( EDA_ANGLE( -270, DEGREES_T ) );
        world->AddRaw( arc );

        PNS::DRAGGER dragger( m_router );
        dragger.SetWorld( world.get() );
        dragger.SetMode( PNS::DM_ARC );

        PNS::ITEM_SET items;
        items.Add( arc );

        m_router->SetFailureReason( wxEmptyString );
        BOOST_CHECK( !dragger.Start( arc->Anchor( 0 ), items ) );
        BOOST_CHECK( !m_router->FailureReason().IsEmpty() );

        world->KillChildren();
    }
}


// Base mock's NetCode() returns -1 for everything, which reads as unnetted; use a real net here
namespace
{
struct NETCODE_RULE_RESOLVER : public MOCK_RULE_RESOLVER
{
    int NetCode( PNS::NET_HANDLE aNet ) override { return aNet ? 1 : -1; }
};

PNS::ITEM* queryFinishAnchor( PNS::NODE& aWorld, const PNS::LINE& aTrack )
{
    PNS::TOPOLOGY   topo( &aWorld );
    VECTOR2I        anchorPoint;
    PNS_LAYER_RANGE anchorLayers;
    PNS::ITEM*      anchorItem = nullptr;

    BOOST_REQUIRE( topo.NearestUnconnectedAnchorPoint( &aTrack, anchorPoint, anchorLayers,
                                                       anchorItem ) );
    return anchorItem;
}
} // namespace


// F-key finish adds the track to a temporary branch node; a joint linking only to that
// track must still yield a persistent-world anchor, not a dangling branch pointer
//
// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24985
BOOST_FIXTURE_TEST_CASE( PNSFinishAnchorNoDanglingBranchItem, PNS_TEST_FIXTURE )
{
    NETCODE_RULE_RESOLVER resolver;

    PNS::NODE world;
    world.SetMaxClearance( 10000000 );
    world.SetRuleResolver( &resolver );

    PNS::NET_HANDLE net = (PNS::NET_HANDLE) 1;

    // Persistent unconnected target on the same net, away from the track
    PNS::SEGMENT* target = new PNS::SEGMENT( SEG( VECTOR2I( 10000000, 10000000 ),
                                                 VECTOR2I( 12000000, 10000000 ) ), net );
    target->SetWidth( 250000 );
    target->SetLayers( PNS_LAYER_RANGE( F_Cu ) );
    world.AddRaw( target );

    // Closed loop; end joint's two links are both owned by the temporary branch node
    PNS::LINE track;
    track.SetLayers( PNS_LAYER_RANGE( F_Cu ) );
    track.SetNet( net );
    track.SetWidth( 250000 );
    track.Line().Append( VECTOR2I( 0, 0 ) );
    track.Line().Append( VECTOR2I( 2000000, 0 ) );
    track.Line().Append( VECTOR2I( 2000000, 2000000 ) );
    track.Line().Append( VECTOR2I( 0, 2000000 ) );
    track.Line().Append( VECTOR2I( 0, 0 ) );

    // Anchor must be the persistent target, never a link owned by the destroyed temp branch
    BOOST_CHECK_EQUAL( queryFinishAnchor( world, track ), target );
}


// ConnectedJoints must cross arcs when subtracting the track; stopping at an arc left
// temporary primitives beyond it selectable as the anchor, reviving the dangling pointer
//
// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24985
BOOST_FIXTURE_TEST_CASE( PNSFinishAnchorCrossesArcInConnectivity, PNS_TEST_FIXTURE )
{
    NETCODE_RULE_RESOLVER resolver;

    PNS::NODE world;
    world.SetMaxClearance( 10000000 );
    world.SetRuleResolver( &resolver );

    PNS::NET_HANDLE net = (PNS::NET_HANDLE) 1;

    // Farther from the track end than its own far segment, wins only once that's subtracted
    PNS::SEGMENT* target = new PNS::SEGMENT( SEG( VECTOR2I( 7000000, 0 ),
                                                 VECTOR2I( 8000000, 0 ) ), net );
    target->SetWidth( 250000 );
    target->SetLayers( PNS_LAYER_RANGE( F_Cu ) );
    world.AddRaw( target );

    // Segment-arc-segment track; far segment lies across the arc, so connectivity must cross it
    PNS::LINE track;
    track.SetLayers( PNS_LAYER_RANGE( F_Cu ) );
    track.SetNet( net );
    track.SetWidth( 250000 );
    track.Line().Append( VECTOR2I( 0, 0 ) );
    track.Line().Append( VECTOR2I( 1000000, 0 ) );
    track.Line().Append( SHAPE_ARC( VECTOR2I( 1000000, 0 ), VECTOR2I( 1500000, 500000 ),
                                    VECTOR2I( 2000000, 0 ), 0 ) );
    track.Line().Append( VECTOR2I( 3000000, 0 ) );

    // Anchor must be the persistent target, not a branch-owned primitive across the arc
    BOOST_CHECK_EQUAL( queryFinishAnchor( world, track ), target );
}


// Regression tests for issues #18658 and #24132. Physical clearance rules must be
// enforced for same-net and free-pad pairs without disturbing the fast path on
// boards that do not define them.

namespace
{
PNS::VIA* makeVia( const VECTOR2I& aPos, PNS::NET_HANDLE aNet )
{
    PNS::VIA* v = new PNS::VIA( aPos, PNS_LAYER_RANGE( F_Cu, B_Cu ), 50000, 10000 );
    v->SetNet( aNet );
    return v;
}

PNS::SOLID* makePad( const VECTOR2I& aPos, PNS::NET_HANDLE aNet, bool aFreePad = false )
{
    PNS::SOLID* s = new PNS::SOLID;
    s->SetShape( new SHAPE_CIRCLE( aPos, 250000 ) );
    s->SetPos( aPos );
    s->SetLayers( PNS_LAYER_RANGE( F_Cu ) );
    s->SetNet( aNet );
    s->SetIsFreePad( aFreePad );
    return s;
}
} // namespace


// Cross-net via vs via with no physical rules: ordinary CT_CLEARANCE applies.
BOOST_FIXTURE_TEST_CASE( PNSCrossNetViaViaElectricalClearanceBaseline, PNS_TEST_FIXTURE )
{
    std::unique_ptr<PNS::NODE> world( new PNS::NODE );
    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    PNS::VIA* v1 = makeVia( VECTOR2I( 0, 0 ), (PNS::NET_HANDLE) 1 );
    PNS::VIA* v2 = makeVia( VECTOR2I( 0, 100000 ), (PNS::NET_HANDLE) 2 );
    world->AddRaw( v1 );
    world->AddRaw( v2 );

    m_ruleResolver.m_defaultClearance = 1000000;

    PNS::NODE::OBSTACLES obstacles;
    world->QueryColliding( v1, obstacles );

    BOOST_CHECK_GE( obstacles.size(), (size_t) 1 );
}


// Same-net via vs via with no physical rules: fast path keeps clearance = -1.
BOOST_FIXTURE_TEST_CASE( PNSSameNetNoPhysicalRulesFastPathNoCollision, PNS_TEST_FIXTURE )
{
    std::unique_ptr<PNS::NODE> world( new PNS::NODE );
    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    PNS::NET_HANDLE net = (PNS::NET_HANDLE) 1;
    world->AddRaw( makeVia( VECTOR2I( 0, 0 ), net ) );
    PNS::VIA* v1 = makeVia( VECTOR2I( 0, 100000 ), net );
    world->AddRaw( v1 );

    m_ruleResolver.m_hasUserPhysicalRules = false;
    m_ruleResolver.m_defaultClearance = 1000000;

    PNS::NODE::OBSTACLES obstacles;
    world->QueryColliding( v1, obstacles );

    BOOST_CHECK_EQUAL( obstacles.size(), (size_t) 0 );
}


// Same-net via vs via with a matching physical_clearance rule: collision reported.
// Regression scenario for issues #18658 and #24132.
BOOST_FIXTURE_TEST_CASE( PNSSameNetWithPhysicalRuleCollides, PNS_TEST_FIXTURE )
{
    std::unique_ptr<PNS::NODE> world( new PNS::NODE );
    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    PNS::NET_HANDLE net = (PNS::NET_HANDLE) 1;
    world->AddRaw( makeVia( VECTOR2I( 0, 0 ), net ) );
    PNS::VIA* v1 = makeVia( VECTOR2I( 0, 100000 ), net );
    world->AddRaw( v1 );

    m_ruleResolver.m_hasUserPhysicalRules = true;
    m_ruleResolver.m_defaultPhysicalClearance = 1000000;

    PNS::NODE::OBSTACLES obstacles;
    world->QueryColliding( v1, obstacles );

    BOOST_CHECK_GE( obstacles.size(), (size_t) 1 );
    if( !obstacles.empty() )
        BOOST_CHECK_EQUAL( obstacles.begin()->m_clearance, 1000000 );
}


// Free pad vs cross-net pair, no physical rules: fast path applies.
BOOST_FIXTURE_TEST_CASE( PNSFreePadNoPhysicalRulesFastPath, PNS_TEST_FIXTURE )
{
    std::unique_ptr<PNS::NODE> world( new PNS::NODE );
    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    PNS::SOLID* freePad = makePad( VECTOR2I( 0, 0 ), (PNS::NET_HANDLE) 1, /*aFreePad=*/true );
    PNS::VIA*   v = makeVia( VECTOR2I( 0, 100000 ), (PNS::NET_HANDLE) 2 );
    world->AddRaw( freePad );
    world->AddRaw( v );

    m_ruleResolver.m_hasUserPhysicalRules = false;
    m_ruleResolver.m_defaultClearance = 1000000;

    PNS::NODE::OBSTACLES obstacles;
    world->QueryColliding( v, obstacles );

    BOOST_CHECK_EQUAL( obstacles.size(), (size_t) 0 );
}


// Free pad vs cross-net pair, physical rules present but no match: safety net must
// keep free pads from reporting collisions just because the board has a physical
// rule somewhere.
BOOST_FIXTURE_TEST_CASE( PNSFreePadSafetyNet, PNS_TEST_FIXTURE )
{
    std::unique_ptr<PNS::NODE> world( new PNS::NODE );
    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    PNS::SOLID* freePad = makePad( VECTOR2I( 0, 0 ), (PNS::NET_HANDLE) 1, /*aFreePad=*/true );
    PNS::VIA*   v = makeVia( VECTOR2I( 0, 100000 ), (PNS::NET_HANDLE) 2 );
    world->AddRaw( freePad );
    world->AddRaw( v );

    m_ruleResolver.m_hasUserPhysicalRules = true;
    m_ruleResolver.m_defaultPhysicalClearance = 0; // rule does not match
    m_ruleResolver.m_defaultClearance = 1000000;   // would collide if !sameNet block runs

    PNS::NODE::OBSTACLES obstacles;
    world->QueryColliding( v, obstacles );

    BOOST_CHECK_EQUAL( obstacles.size(), (size_t) 0 );
}


// Free pad with a matching physical_clearance rule: collision reported.
BOOST_FIXTURE_TEST_CASE( PNSFreePadPhysicalRuleEnforced, PNS_TEST_FIXTURE )
{
    std::unique_ptr<PNS::NODE> world( new PNS::NODE );
    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    PNS::SOLID* freePad = makePad( VECTOR2I( 0, 0 ), (PNS::NET_HANDLE) 1, /*aFreePad=*/true );
    PNS::VIA*   v = makeVia( VECTOR2I( 0, 100000 ), (PNS::NET_HANDLE) 2 );
    world->AddRaw( freePad );
    world->AddRaw( v );

    m_ruleResolver.m_hasUserPhysicalRules = true;
    m_ruleResolver.m_defaultPhysicalClearance = 1000000;

    PNS::NODE::OBSTACLES obstacles;
    world->QueryColliding( v, obstacles );

    BOOST_CHECK_GE( obstacles.size(), (size_t) 1 );
}


// Same-net via vs pad with a matching physical_hole_clearance rule. Covers the
// drill-into-pad half of issue #24132 and exercises CT_PHYSICAL_HOLE_CLEARANCE.
BOOST_FIXTURE_TEST_CASE( PNSSameNetPhysicalHoleClearance, PNS_TEST_FIXTURE )
{
    std::unique_ptr<PNS::NODE> world( new PNS::NODE );
    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    PNS::NET_HANDLE net = (PNS::NET_HANDLE) 1;
    PNS::SOLID*     pad = makePad( VECTOR2I( 0, 0 ), net );
    PNS::VIA*       via = makeVia( VECTOR2I( 0, 100000 ), net );
    world->AddRaw( pad );
    world->AddRaw( via );

    m_ruleResolver.m_hasUserPhysicalRules = true;
    m_ruleResolver.m_defaultPhysicalHoleClearance = 1000000;

    PNS::NODE::OBSTACLES obstacles;
    world->QueryColliding( via, obstacles );

    BOOST_CHECK_GE( obstacles.size(), (size_t) 1 );
}


BOOST_FIXTURE_TEST_CASE( PNSSameNetSafetyNetOnOverlap, PNS_TEST_FIXTURE )
{
    std::unique_ptr<PNS::NODE> world( new PNS::NODE );
    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    PNS::NET_HANDLE net = (PNS::NET_HANDLE) 1;
    world->AddRaw( makeVia( VECTOR2I( 0, 0 ), net ) );
    PNS::VIA* v1 = makeVia( VECTOR2I( 0, 30000 ), net ); // overlaps the first via
    world->AddRaw( v1 );

    m_ruleResolver.m_hasUserPhysicalRules = true;
    m_ruleResolver.m_defaultPhysicalClearance = 0;

    PNS::NODE::OBSTACLES obstacles;
    world->QueryColliding( v1, obstacles );

    BOOST_CHECK_EQUAL( obstacles.size(), (size_t) 0 );
}


// Same-net pad+via with both physical_clearance and physical_hole_clearance
// matching: the resolver returns max across the two query points.
BOOST_FIXTURE_TEST_CASE( PNSBothPhysicalConstraintsMaxWins, PNS_TEST_FIXTURE )
{
    std::unique_ptr<PNS::NODE> world( new PNS::NODE );
    world->SetMaxClearance( 10000000 );
    world->SetRuleResolver( &m_ruleResolver );

    PNS::NET_HANDLE net = (PNS::NET_HANDLE) 1;
    PNS::SOLID*     pad = makePad( VECTOR2I( 0, 0 ), net );
    PNS::VIA*       via = makeVia( VECTOR2I( 0, 100000 ), net );
    world->AddRaw( pad );
    world->AddRaw( via );

    m_ruleResolver.m_hasUserPhysicalRules = true;
    m_ruleResolver.m_defaultPhysicalClearance = 100000;
    m_ruleResolver.m_defaultPhysicalHoleClearance = 2000000;

    PNS::NODE::OBSTACLES obstacles;
    world->QueryColliding( via, obstacles );

    BOOST_CHECK_GE( obstacles.size(), (size_t) 1 );

    // The recursive collideSimple call for the via's hole inserts a separate OBSTACLE
    // with the max-accumulated clearance, so look across all entries rather than
    // relying on std::set ordering (which is by pointer).
    int maxClearance = 0;
    for( const PNS::OBSTACLE& obs : obstacles )
        maxClearance = std::max( maxClearance, obs.m_clearance );
    BOOST_CHECK_EQUAL( maxClearance, 2000000 );
}


// Diff pair vias must respect copper-to-hole clearance, not just copper-to-copper and
// hole-to-hole. EffectiveDiffPairViaGap() is the copper-edge-to-copper-edge distance the
// placer fits vias to; it converts each clearance rule to that reference by subtracting the
// annular ring(s) of via copper that sit between a hole edge and the copper edge.
//
// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/21623 where the placer
// ignored copper-to-hole clearance and produced DRC violations on diff pair vias.
BOOST_AUTO_TEST_CASE( PNSDiffPairViaGapCopperToHoleClearance )
{
    PNS::SIZES_SETTINGS sizes;

    // 600um copper diameter over a 300um drill leaves a 150um annular ring.
    sizes.SetViaDiameter( 600000 );
    sizes.SetViaDrill( 300000 );
    sizes.SetDiffPairViaGapSameAsTraceGap( false );

    BOOST_CHECK_EQUAL( sizes.GetDiffPairCopperToHole(), 0 );

    // Copper-to-hole binds because 400um from a hole edge to the neighbour's copper edge is
    // 400000 - 150000 = 250000 copper-to-copper, exceeding both other rules.
    sizes.SetDiffPairViaGap( 200000 );
    sizes.SetDiffPairHoleToHole( 500000 ); // 500000 - 300000 = 200000 copper-to-copper
    sizes.SetDiffPairCopperToHole( 400000 );

    BOOST_CHECK_EQUAL( sizes.GetDiffPairCopperToHole(), 400000 );
    BOOST_CHECK_EQUAL( sizes.EffectiveDiffPairViaGap(), 250000 );

    // Hole-to-hole binds when it is the largest converted rule.
    sizes.SetDiffPairHoleToHole( 900000 ); // 900000 - 300000 = 600000 copper-to-copper
    BOOST_CHECK_EQUAL( sizes.EffectiveDiffPairViaGap(), 600000 );

    // Plain copper-to-copper gap binds when the hole-based rules are slack.
    sizes.SetDiffPairHoleToHole( 0 );
    sizes.SetDiffPairCopperToHole( 0 );
    BOOST_CHECK_EQUAL( sizes.EffectiveDiffPairViaGap(), 200000 );
}
