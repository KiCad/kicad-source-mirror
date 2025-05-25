/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <eeschema_test_utils.h>


class TEST_NETLIST_EXPORTER_KICAD_FIXTURE : public TEST_NETLIST_EXPORTER_FIXTURE<NETLIST_EXPORTER_KICAD>
{
public:
    void CompareNetlists() override
    {
        NETLIST golden;
        NETLIST test;

        {
            std::unique_ptr<NETLIST_READER> netlistReader( NETLIST_READER::GetNetlistReader(
                                                &golden, GetNetlistPath(), wxEmptyString ) );

            BOOST_ASSERT( netlistReader );
            BOOST_REQUIRE_NO_THROW( netlistReader->LoadNetlist() );
        }

        {
            std::unique_ptr<NETLIST_READER> netlistReader( NETLIST_READER::GetNetlistReader(
                                                &test, GetNetlistPath( true ), wxEmptyString ) );

            BOOST_ASSERT( netlistReader );
            BOOST_REQUIRE_NO_THROW( netlistReader->LoadNetlist() );
        }

        // Number of components should match
        BOOST_REQUIRE_EQUAL( golden.GetCount(), test.GetCount() );

        for( unsigned i = 0; i < golden.GetCount(); i++ )
        {
            COMPONENT* goldenComp = golden.GetComponent( i );
            COMPONENT* refComp    = test.GetComponentByReference( goldenComp->GetReference() );

            // Retrieval by reference
            BOOST_REQUIRE_NE( refComp, nullptr );

            // Retrieval by KIID
            KIID_PATH path = goldenComp->GetPath();

            BOOST_REQUIRE( !goldenComp->GetKIIDs().empty() );

            path.push_back( goldenComp->GetKIIDs().front() );

            COMPONENT* pathComp = test.GetComponentByPath( path );
            BOOST_REQUIRE_NE( pathComp, nullptr );

            // We should have found the same component
            BOOST_REQUIRE_EQUAL( refComp->GetReference(), pathComp->GetReference() );

            // And that component should have the same number of attached nets
            BOOST_REQUIRE_EQUAL( goldenComp->GetNetCount(), refComp->GetNetCount() );

            for( unsigned net = 0; net < goldenComp->GetNetCount(); net++ )
            {
                const COMPONENT_NET& goldenNet = goldenComp->GetNet( net );
                const COMPONENT_NET& testNet   = refComp->GetNet( net );

                // The video test has a bunch of unconnected RESERVED pins which cause duplicate
                // auto-generated netnames.  The connectivity algo disambiguates these with "_n"
                // suffixes, but since the algorithm is multi-threaded, which ones get which suffix
                // is not deterministic.  So skip these.
                if( testNet.GetPinFunction().Contains( "RESERVED" ) )
                    continue;

                // The two nets at the same index should be identical
                BOOST_REQUIRE_EQUAL( goldenNet.GetNetName(), testNet.GetNetName() );
                BOOST_REQUIRE_EQUAL( goldenNet.GetPinName(), testNet.GetPinName() );
                BOOST_REQUIRE_EQUAL( goldenNet.GetPinType(), testNet.GetPinType() );
            }

            // And the the resolved component class is the same
            BOOST_REQUIRE( goldenComp->GetComponentClassNames()
                           == refComp->GetComponentClassNames() );
        }
    }
};


BOOST_FIXTURE_TEST_SUITE( Netlists, TEST_NETLIST_EXPORTER_KICAD_FIXTURE )


BOOST_AUTO_TEST_CASE( FindPlugin )
{
    BOOST_CHECK_NE( m_pi.get(), nullptr );
}


BOOST_AUTO_TEST_CASE( GlobalPromotion )
{
    TestNetlist( "test_global_promotion" );
}


BOOST_AUTO_TEST_CASE( GlobalPromotion2 )
{
    TestNetlist( "test_global_promotion_2" );
}


BOOST_AUTO_TEST_CASE( Video )
{
    TestNetlist( "video" );
}


BOOST_AUTO_TEST_CASE( ComplexHierarchy )
{
    TestNetlist( "complex_hierarchy" );
}


BOOST_AUTO_TEST_CASE( WeakVectorBusDisambiguation )
{
    TestNetlist( "weak_vector_bus_disambiguation" );
}


BOOST_AUTO_TEST_CASE( BusJunctions )
{
    TestNetlist( "bus_junctions" );
}


BOOST_AUTO_TEST_CASE( HierRenaming )
{
    TestNetlist( "test_hier_renaming" );
}


BOOST_AUTO_TEST_CASE( NoConnects )
{
    TestNetlist( "noconnects" );
}


BOOST_AUTO_TEST_CASE( PrefixBusAlias )
{
    TestNetlist( "prefix_bus_alias" );
}


BOOST_AUTO_TEST_CASE( GroupBusMatching )
{
    TestNetlist( "group_bus_matching" );
}


BOOST_AUTO_TEST_CASE( TopLevelHierPins )
{
    TestNetlist( "top_level_hier_pins" );
}


BOOST_AUTO_TEST_CASE( BusEntries )
{
    TestNetlist( "bus_entries" );
}


BOOST_AUTO_TEST_CASE( HierNoConnect )
{
    TestNetlist( "test_hier_no_connect" );
}


BOOST_AUTO_TEST_CASE( BusConnection )
{
    TestNetlist( "bus_connection" );
}

BOOST_AUTO_TEST_CASE( Issue14657 )
{
    TestNetlist( "issue14657" );
}

BOOST_AUTO_TEST_CASE( HierarchyAliases )
{
    TestNetlist( "hierarchy_aliases" );
}

BOOST_AUTO_TEST_CASE( Issue14818 )
{
    TestNetlist( "issue14818" );
}

BOOST_AUTO_TEST_CASE( Issue16003 )
{
    TestNetlist( "issue16003" );
}

BOOST_AUTO_TEST_CASE( Issue16439 )
{
    TestNetlist( "issue16439" );
}

BOOST_AUTO_TEST_CASE( ComponentClasses )
{
    TestNetlist( "component_classes" );
}


BOOST_AUTO_TEST_CASE( Jumpers )
{
    TestNetlist( "jumpers" );
}


BOOST_AUTO_TEST_SUITE_END()
