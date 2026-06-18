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

#include <string>

#include <pin_map.h>
#include <api/schematic/schematic_types.pb.h>
#include <api/api_sch_utils.h>


BOOST_AUTO_TEST_SUITE( PinMapApi )


BOOST_AUTO_TEST_CASE( LibSymbolPinMapsProtoRoundTrip )
{
    kiapi::schematic::types::LibSymbolPinMaps maps;

    kiapi::schematic::types::AssociatedFootprint* assoc = maps.add_associated_footprints();
    assoc->mutable_footprint()->set_library_nickname( "Package_SO" );
    assoc->mutable_footprint()->set_entry_name( "SOIC-8" );
    assoc->set_map_name( "STD-8" );

    kiapi::schematic::types::PinMap* map = maps.add_pin_maps();
    map->set_name( "STD-8" );
    kiapi::schematic::types::PinMapEntry* entry = map->add_entries();
    entry->set_pin_number( "4" );
    entry->set_pad_number( "[4,9]" );

    std::string bytes;
    BOOST_REQUIRE( maps.SerializeToString( &bytes ) );

    kiapi::schematic::types::LibSymbolPinMaps parsed;
    BOOST_REQUIRE( parsed.ParseFromString( bytes ) );

    BOOST_REQUIRE_EQUAL( parsed.pin_maps_size(), 1 );
    BOOST_CHECK_EQUAL( parsed.pin_maps( 0 ).name(), "STD-8" );
    BOOST_REQUIRE_EQUAL( parsed.pin_maps( 0 ).entries_size(), 1 );
    BOOST_CHECK_EQUAL( parsed.pin_maps( 0 ).entries( 0 ).pad_number(), "[4,9]" );
    BOOST_REQUIRE_EQUAL( parsed.associated_footprints_size(), 1 );
    BOOST_CHECK_EQUAL( parsed.associated_footprints( 0 ).map_name(), "STD-8" );
    BOOST_CHECK_EQUAL( parsed.associated_footprints( 0 ).footprint().entry_name(), "SOIC-8" );
}


BOOST_AUTO_TEST_CASE( InstanceOverrideRoundTrip )
{
    PIN_MAP_INSTANCE_OVERRIDE override;
    override.m_Mode = PIN_MAP_OVERRIDE_MODE::USE_NAMED_MAP;
    override.m_ActiveMapName = wxS( "DFN-8-EP" );
    override.m_Edits.push_back( { wxS( "1" ), wxS( "8" ) } );
    override.m_Edits.push_back( { wxS( "8" ), wxS( "1" ) } );

    kiapi::schematic::types::PinMapInstanceOverride proto;
    PackPinMapOverride( &proto, override );

    std::string bytes;
    BOOST_REQUIRE( proto.SerializeToString( &bytes ) );

    kiapi::schematic::types::PinMapInstanceOverride parsedProto;
    BOOST_REQUIRE( parsedProto.ParseFromString( bytes ) );

    PIN_MAP_INSTANCE_OVERRIDE result = UnpackPinMapOverride( parsedProto );

    BOOST_CHECK( result == override );
}


BOOST_AUTO_TEST_SUITE_END()
