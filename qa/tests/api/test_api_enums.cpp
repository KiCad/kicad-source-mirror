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

#include <boost/test/unit_test.hpp>
#include <boost/bimap.hpp>
#include <magic_enum.hpp>
#include <import_export.h>
#include <qa_utils/wx_utils/wx_assert.h>

// Common
#include <api/api_enums.h>
#include <api/common/types/enums.pb.h>
#include <core/typeinfo.h>
#include <font/text_attributes.h>
#include <layer_ids.h>
#include <pin_type.h>
#include <stroke_params.h>
#include <widgets/report_severity.h>

// Board-specific
#include <api/board/board_types.pb.h>
#include <api/board/board_commands.pb.h>
#include <board_stackup_manager/board_stackup.h>
#include <padstack.h>
#include <pcb_dimension.h>
#include <pcb_track.h>
#include <project/board_project_settings.h>
#include <zones.h>
#include <zone_settings.h>

using namespace kiapi::common;

BOOST_AUTO_TEST_SUITE( ApiEnums )

/**
 * Checks if a KiCad enum has been properly mapped to a Protobuf enum
 * @tparam KiCadEnum is an enum type
 * @tparam ProtoEnum is a Protobuf enum type
 * @param aPartiallyMapped is true if only some of the KiCad enum values are exposed to the API
 */
template<typename KiCadEnum, typename ProtoEnum>
void testEnums( bool aPartiallyMapped = false )
{
    boost::bimap<ProtoEnum, KiCadEnum> protoToKiCadSeen;
    std::set<ProtoEnum> seenProtos;

    for( ProtoEnum value : magic_enum::enum_values<ProtoEnum>() )
    {
        BOOST_TEST_CONTEXT( magic_enum::enum_type_name<ProtoEnum>() << "::"
                            << magic_enum::enum_name( value ) )
        {
            std::string name( magic_enum::enum_name( value ) );
            auto splitPos = name.find_first_of( '_' );

            // Protobuf enum names should be formatted as PREFIX_KEY
            BOOST_REQUIRE_MESSAGE( splitPos != std::string::npos,
                                   "Proto enum name doesn't have a prefix" );

            std::string suffix = name.substr( splitPos );

            // Protobuf enum with the value 0 should not map to anything
            if( static_cast<int>( value ) == 0 )
            {
                BOOST_REQUIRE_MESSAGE( suffix.compare( "_UNKNOWN" ) == 0,
                                       "Proto enum with value 0 must be named <PREFIX>_UNKNOWN" );
                continue;
            }

            KiCadEnum result;
            // Every non-unknown Proto value should map to a valid KiCad value
            BOOST_REQUIRE_NO_THROW( result = ( FromProtoEnum<KiCadEnum, ProtoEnum>( value ) ) );

            // There should be a 1:1 mapping
            BOOST_REQUIRE( !protoToKiCadSeen.left.count( value ) );
            protoToKiCadSeen.left.insert( { value, result } );
        }
    }

    for( KiCadEnum value : magic_enum::enum_values<KiCadEnum>() )
    {
        BOOST_TEST_CONTEXT( magic_enum::enum_type_name<KiCadEnum>() << "::"
                            << magic_enum::enum_name( value ) )
        {
            ProtoEnum result;

            if( aPartiallyMapped )
            {
                try
                {
                     result = ToProtoEnum<KiCadEnum, ProtoEnum>( value );
                }
                catch( KI_TEST::WX_ASSERT_ERROR )
                {
                    // If it wasn't mapped from KiCad to Proto, it shouldn't be mapped the other way
                    BOOST_REQUIRE_MESSAGE( !protoToKiCadSeen.right.count( value ),
                            "Proto enum is mapped to this KiCad enum, but not vice versa" );
                    continue;
                }
            }
            else
            {
                // Every KiCad enum value should map to a non-unknown Protobuf value
                BOOST_REQUIRE_NO_THROW( result = ( ToProtoEnum<KiCadEnum, ProtoEnum>( value ) ) );
            }

            // Protobuf "unknown" should always be zero value by convention
            BOOST_REQUIRE( result != static_cast<ProtoEnum>( 0 ) );

            // There should be a 1:1 mapping
            BOOST_REQUIRE( !seenProtos.count( result ) );
            seenProtos.insert( result );

            // Round-tripping should work
            KiCadEnum roundTrip = FromProtoEnum<KiCadEnum, ProtoEnum>( result );
            BOOST_REQUIRE( roundTrip == value );
        }
    }
}

BOOST_AUTO_TEST_CASE( HorizontalAlignment )
{
    testEnums<GR_TEXT_H_ALIGN_T, types::HorizontalAlignment>();
}

BOOST_AUTO_TEST_CASE( VerticalAlignment )
{
    testEnums<GR_TEXT_V_ALIGN_T, types::VerticalAlignment>();
}

BOOST_AUTO_TEST_CASE( StrokeLineStyle )
{
    testEnums<LINE_STYLE, types::StrokeLineStyle>();
}

BOOST_AUTO_TEST_CASE( KiCadObjectType )
{
    testEnums<KICAD_T, types::KiCadObjectType>( true );
}

BOOST_AUTO_TEST_CASE( ElectricalPinType )
{
    testEnums<ELECTRICAL_PINTYPE, types::ElectricalPinType>( true );
}

BOOST_AUTO_TEST_CASE( BoardLayer )
{
    testEnums<PCB_LAYER_ID, kiapi::board::types::BoardLayer>( true );
}

BOOST_AUTO_TEST_CASE( PadStackShape )
{
    testEnums<PAD_SHAPE, kiapi::board::types::PadStackShape>();
}

BOOST_AUTO_TEST_CASE( ZoneConnectionStyle )
{
    testEnums<ZONE_CONNECTION, kiapi::board::types::ZoneConnectionStyle>();
}

BOOST_AUTO_TEST_CASE( PadType )
{
    testEnums<PAD_ATTRIB, kiapi::board::types::PadType>();
}

BOOST_AUTO_TEST_CASE( PadStackType )
{
    testEnums<PADSTACK::MODE, kiapi::board::types::PadStackType>();
}

BOOST_AUTO_TEST_CASE( DrillShape )
{
    testEnums<PAD_DRILL_SHAPE, kiapi::board::types::DrillShape>();
}

BOOST_AUTO_TEST_CASE( UnconnectedLayerRemoval )
{
    testEnums<UNCONNECTED_LAYER_MODE, kiapi::board::types::UnconnectedLayerRemoval>();
}

BOOST_AUTO_TEST_CASE( ViaType )
{
    // VIATYPE::NOT_DEFINED is not mapped
    testEnums<VIATYPE, kiapi::board::types::ViaType>( true );
}

BOOST_AUTO_TEST_CASE( IslandRemovalMode )
{
    testEnums<ISLAND_REMOVAL_MODE, kiapi::board::types::IslandRemovalMode>();
}

BOOST_AUTO_TEST_CASE( ZoneFillMode )
{
    testEnums<ZONE_FILL_MODE, kiapi::board::types::ZoneFillMode>();
}

BOOST_AUTO_TEST_CASE( ZoneBorderStyle )
{
    testEnums<ZONE_BORDER_DISPLAY_STYLE, kiapi::board::types::ZoneBorderStyle>();
}

BOOST_AUTO_TEST_CASE( PlacementRuleSourceType )
{
    testEnums<PLACEMENT_SOURCE_T, kiapi::board::types::PlacementRuleSourceType>();
}

BOOST_AUTO_TEST_CASE( TeardropType )
{
    testEnums<TEARDROP_TYPE, kiapi::board::types::TeardropType>();
}

BOOST_AUTO_TEST_CASE( DimensionTextBorderStyle )
{
    testEnums<DIM_TEXT_BORDER, kiapi::board::types::DimensionTextBorderStyle>();
}

BOOST_AUTO_TEST_CASE( DimensionUnitFormat )
{
    testEnums<DIM_UNITS_FORMAT, kiapi::board::types::DimensionUnitFormat>();
}

BOOST_AUTO_TEST_CASE( DimensionArrowDirection )
{
    testEnums<DIM_ARROW_DIRECTION, kiapi::board::types::DimensionArrowDirection>();
}

BOOST_AUTO_TEST_CASE( DimensionPrecision )
{
    testEnums<DIM_PRECISION, kiapi::board::types::DimensionPrecision>();
}

BOOST_AUTO_TEST_CASE( DimensionTextPosition )
{
    testEnums<DIM_TEXT_POSITION, kiapi::board::types::DimensionTextPosition>();
}

BOOST_AUTO_TEST_CASE( DimensionUnit )
{
    testEnums<DIM_UNITS_MODE, kiapi::board::types::DimensionUnit>();
}

BOOST_AUTO_TEST_CASE( InactiveLayerDisplayMode )
{
    testEnums<HIGH_CONTRAST_MODE, kiapi::board::commands::InactiveLayerDisplayMode>();
}

BOOST_AUTO_TEST_CASE( NetColorDisplayMode )
{
    testEnums<NET_COLOR_MODE, kiapi::board::commands::NetColorDisplayMode>();
}

BOOST_AUTO_TEST_CASE( RatsnestDisplayMode )
{
    testEnums<RATSNEST_MODE, kiapi::board::commands::RatsnestDisplayMode>();
}

BOOST_AUTO_TEST_CASE( BoardStackupLayerType )
{
    testEnums<BOARD_STACKUP_ITEM_TYPE, kiapi::board::BoardStackupLayerType>();
}

BOOST_AUTO_TEST_CASE( DrcSeverity )
{
    testEnums<SEVERITY, kiapi::board::commands::DrcSeverity>();
}

BOOST_AUTO_TEST_SUITE_END()
