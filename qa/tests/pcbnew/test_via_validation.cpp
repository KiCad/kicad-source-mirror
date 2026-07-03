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
#include <base_units.h>
#include <pcb_track.h>

using FIELD = PCB_VIA::VIA_PARAMETER_ERROR::FIELD;


BOOST_AUTO_TEST_SUITE( ViaValidation )


// A well-formed via with drill smaller than diameter must validate clean.
BOOST_AUTO_TEST_CASE( WellFormedViaPasses )
{
    auto error = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.4 ), F_Cu, B_Cu );

    BOOST_CHECK( !error.has_value() );

    // Still clean when the endpoints are checked against a real two-layer stack.
    auto inStack = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.4 ), F_Cu, B_Cu,
                                                   std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                   std::nullopt, std::nullopt, 2 );

    BOOST_CHECK( !inStack.has_value() );
}


// Drill equal to or larger than the diameter leaves no annular ring and must be
// flagged against the drill field.
BOOST_AUTO_TEST_CASE( DrillNotSmallerThanDiameterFails )
{
    auto error = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.4 ), pcbIUScale.mmToIU( 0.4 ), F_Cu, B_Cu );

    BOOST_REQUIRE( error.has_value() );
    BOOST_CHECK( error->m_Field == FIELD::DRILL );

    auto oversized = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.4 ), pcbIUScale.mmToIU( 0.6 ), F_Cu, B_Cu );

    BOOST_REQUIRE( oversized.has_value() );
    BOOST_CHECK( oversized->m_Field == FIELD::DRILL );
}


// Sub-minimum geometry is rejected on the offending field.
BOOST_AUTO_TEST_CASE( TooSmallGeometryFails )
{
    auto tinyDiameter = PCB_VIA::ValidateViaParameters( GEOMETRY_MIN_SIZE - 1, std::nullopt, F_Cu, B_Cu );

    BOOST_REQUIRE( tinyDiameter.has_value() );
    BOOST_CHECK( tinyDiameter->m_Field == FIELD::DIAMETER );

    auto tinyDrill = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), GEOMETRY_MIN_SIZE - 1, F_Cu, B_Cu );

    BOOST_REQUIRE( tinyDrill.has_value() );
    BOOST_CHECK( tinyDrill->m_Field == FIELD::DRILL );
}


// Diameter and drill must be defined together.
BOOST_AUTO_TEST_CASE( PartialSizeSpecificationFails )
{
    auto noDrill = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), std::nullopt, F_Cu, B_Cu );

    BOOST_REQUIRE( noDrill.has_value() );
    BOOST_CHECK( noDrill->m_Field == FIELD::DRILL );

    auto noDiameter = PCB_VIA::ValidateViaParameters( std::nullopt, pcbIUScale.mmToIU( 0.4 ), F_Cu, B_Cu );

    BOOST_REQUIRE( noDiameter.has_value() );
    BOOST_CHECK( noDiameter->m_Field == FIELD::DIAMETER );
}


// A via cannot start and end on the same layer.
BOOST_AUTO_TEST_CASE( CoincidentLayersFail )
{
    auto error = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.4 ), F_Cu, F_Cu );

    BOOST_REQUIRE( error.has_value() );
    BOOST_CHECK( error->m_Field == FIELD::START_LAYER );
}


// Non-copper endpoints and endpoints outside the board stack are rejected.
BOOST_AUTO_TEST_CASE( InvalidLayersFail )
{
    auto nonCopper = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.4 ), F_Cu,
                                                     F_SilkS );

    BOOST_REQUIRE( nonCopper.has_value() );
    BOOST_CHECK( nonCopper->m_Field == FIELD::END_LAYER );

    // In2_Cu exists in the enum but a two-layer board does not contain it.
    auto offStack = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.4 ), F_Cu, In2_Cu,
                                                    std::nullopt, std::nullopt, std::nullopt, std::nullopt,
                                                    std::nullopt, std::nullopt, 2 );

    BOOST_REQUIRE( offStack.has_value() );
    BOOST_CHECK( offStack->m_Field == FIELD::END_LAYER );
}


// A backdrill must be at least as large as the primary drill; a valid one passes.
BOOST_AUTO_TEST_CASE( BackdrillSizeIsValidated )
{
    auto tooSmall = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.4 ), F_Cu, B_Cu,
                                                    pcbIUScale.mmToIU( 0.3 ) );

    BOOST_REQUIRE( tooSmall.has_value() );
    BOOST_CHECK( tooSmall->m_Field == FIELD::SECONDARY_DRILL );

    auto tertiaryTooSmall = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.4 ),
                                                            F_Cu, B_Cu, std::nullopt, std::nullopt, std::nullopt,
                                                            pcbIUScale.mmToIU( 0.3 ) );

    BOOST_REQUIRE( tertiaryTooSmall.has_value() );
    BOOST_CHECK( tertiaryTooSmall->m_Field == FIELD::TERTIARY_DRILL );

    auto valid = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.4 ), F_Cu, B_Cu,
                                                 pcbIUScale.mmToIU( 0.5 ) );

    BOOST_CHECK( !valid.has_value() );
}


// Backdrill start and end layers get the same copper-layer checks as the primary span.
BOOST_AUTO_TEST_CASE( BackdrillLayersAreValidated )
{
    auto secondaryStart = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.4 ),
                                                          F_Cu, B_Cu, pcbIUScale.mmToIU( 0.5 ), F_SilkS );

    BOOST_REQUIRE( secondaryStart.has_value() );
    BOOST_CHECK( secondaryStart->m_Field == FIELD::SECONDARY_START_LAYER );

    auto secondaryEnd = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.4 ),
                                                        F_Cu, B_Cu, pcbIUScale.mmToIU( 0.5 ), F_Cu, F_SilkS );

    BOOST_REQUIRE( secondaryEnd.has_value() );
    BOOST_CHECK( secondaryEnd->m_Field == FIELD::SECONDARY_END_LAYER );

    auto tertiaryStart = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.4 ),
                                                         F_Cu, B_Cu, std::nullopt, std::nullopt, std::nullopt,
                                                         pcbIUScale.mmToIU( 0.5 ), F_SilkS );

    BOOST_REQUIRE( tertiaryStart.has_value() );
    BOOST_CHECK( tertiaryStart->m_Field == FIELD::TERTIARY_START_LAYER );

    auto tertiaryEnd = PCB_VIA::ValidateViaParameters( pcbIUScale.mmToIU( 0.8 ), pcbIUScale.mmToIU( 0.4 ),
                                                       F_Cu, B_Cu, std::nullopt, std::nullopt, std::nullopt,
                                                       pcbIUScale.mmToIU( 0.5 ), F_Cu, F_SilkS );

    BOOST_REQUIRE( tertiaryEnd.has_value() );
    BOOST_CHECK( tertiaryEnd->m_Field == FIELD::TERTIARY_END_LAYER );
}


BOOST_AUTO_TEST_SUITE_END()
