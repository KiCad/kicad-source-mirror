/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
#include <reporter.h>
#include <sim/sim_model.h>
#include <sim/spice_generator.h>


// Regression coverage for issue #24050: PWL sources must emit scale prefixes (u, m, k, Meg)
// and pass simulator parameter references through to ngspice.

class TEST_SIM_MODEL_SOURCE_PWL_FIXTURE
{
public:
    std::string GeneratePwlItemLine( SIM_MODEL::TYPE aType, const std::string& aPwlValue ) const
    {
        WX_STRING_REPORTER          reporter;
        std::vector<SCH_PIN*>       pins;
        std::unique_ptr<SIM_MODEL>  model = SIM_MODEL::Create( aType, pins, reporter );

        BOOST_REQUIRE( model );
        model->SetParamValue( "pwl", aPwlValue );

        SPICE_ITEM item;
        item.refName = "1";
        item.pinNetNames = { "a", "b" };
        item.model = model.get();

        return model->SpiceGenerator().ItemLine( item );
    }
};


BOOST_FIXTURE_TEST_SUITE( SimModelSourcePwl, TEST_SIM_MODEL_SOURCE_PWL_FIXTURE )


BOOST_AUTO_TEST_CASE( PwlPlainNumbers )
{
    const std::string out = GeneratePwlItemLine( SIM_MODEL::TYPE::V_PWL, "0 0 1 1 2 0" );
    BOOST_TEST_INFO( out );
    BOOST_CHECK( out.find( "PWL(" ) != std::string::npos );
    BOOST_CHECK( out.find( "0 0 1 1 2 0" ) != std::string::npos );
}


BOOST_AUTO_TEST_CASE( PwlMicroPrefix )
{
    // Issue #24050: "1u 0 2u 1" must survive netlist generation.
    const std::string out = GeneratePwlItemLine( SIM_MODEL::TYPE::V_PWL, "0 0 1u 0 2u 1" );
    BOOST_TEST_INFO( out );
    BOOST_CHECK( out.find( "1u" ) != std::string::npos );
    BOOST_CHECK( out.find( "2u" ) != std::string::npos );
    BOOST_CHECK( out.find( "PWL( )" ) == std::string::npos );
}


BOOST_AUTO_TEST_CASE( PwlMilliPrefix )
{
    const std::string out = GeneratePwlItemLine( SIM_MODEL::TYPE::V_PWL, "0 0 1m 1 2m 0" );
    BOOST_TEST_INFO( out );
    BOOST_CHECK( out.find( "1m" ) != std::string::npos );
    BOOST_CHECK( out.find( "PWL( )" ) == std::string::npos );
}


BOOST_AUTO_TEST_CASE( PwlMegaPrefix )
{
    // KiCad SI notation uses "M" for mega; ngspice requires "Meg".
    const std::string out = GeneratePwlItemLine( SIM_MODEL::TYPE::V_PWL, "0 0 1M 1" );
    BOOST_TEST_INFO( out );
    BOOST_CHECK( out.find( "1Meg" ) != std::string::npos );
}


BOOST_AUTO_TEST_CASE( PwlParameterReference )
{
    // Issue #24050: "{t_start}" parameter references must pass through to ngspice.
    const std::string out = GeneratePwlItemLine( SIM_MODEL::TYPE::V_PWL, "0 0 {t_start} 1" );
    BOOST_TEST_INFO( out );
    BOOST_CHECK( out.find( "{t_start}" ) != std::string::npos );
    BOOST_CHECK( out.find( "PWL( )" ) == std::string::npos );
}


BOOST_AUTO_TEST_CASE( PwlMixedParamsAndPrefixes )
{
    const std::string out = GeneratePwlItemLine( SIM_MODEL::TYPE::V_PWL,
                                                 "0 0 {t_rise} 1u {t_fall} 0" );
    BOOST_TEST_INFO( out );
    BOOST_CHECK( out.find( "{t_rise}" ) != std::string::npos );
    BOOST_CHECK( out.find( "{t_fall}" ) != std::string::npos );
    BOOST_CHECK( out.find( "1u" ) != std::string::npos );
}


BOOST_AUTO_TEST_CASE( IpwlPrefixesAndParams )
{
    const std::string out = GeneratePwlItemLine( SIM_MODEL::TYPE::I_PWL,
                                                 "0 0 1u 500m {t_end} 1" );
    BOOST_TEST_INFO( out );
    BOOST_CHECK( out.find( "1u" ) != std::string::npos );
    BOOST_CHECK( out.find( "500m" ) != std::string::npos );
    BOOST_CHECK( out.find( "{t_end}" ) != std::string::npos );
}


BOOST_AUTO_TEST_CASE( PwlBraceExpressionWithSpaces )
{
    // Brace expressions may contain whitespace; they must not be split by the tokenizer.
    const std::string out = GeneratePwlItemLine( SIM_MODEL::TYPE::V_PWL,
                                                 "0 0 {t_start + 1u} 1" );
    BOOST_TEST_INFO( out );
    BOOST_CHECK( out.find( "{t_start + 1u}" ) != std::string::npos );
    BOOST_CHECK( out.find( "PWL( )" ) == std::string::npos );
}


BOOST_AUTO_TEST_CASE( PwlNestedBraces )
{
    const std::string out = GeneratePwlItemLine( SIM_MODEL::TYPE::V_PWL,
                                                 "0 0 {max( {t_a}, {t_b} )} 1" );
    BOOST_TEST_INFO( out );
    BOOST_CHECK( out.find( "{max( {t_a}, {t_b} )}" ) != std::string::npos );
}


BOOST_AUTO_TEST_CASE( PwlBraceMegaPrefix )
{
    // KiCad's 'M' is mega; ngspice reads 'M' as milli.  A brace expression must rewrite
    // numeric literals so the value matches the same number written outside braces.
    const std::string out = GeneratePwlItemLine( SIM_MODEL::TYPE::V_PWL,
                                                 "0 0 {t_start + 1M} 1" );
    BOOST_TEST_INFO( out );
    BOOST_CHECK( out.find( "1Meg" ) != std::string::npos );
    BOOST_CHECK( out.find( "1M}" ) == std::string::npos );
}


BOOST_AUTO_TEST_CASE( PwlBracePreservesIdentifiersAndOperators )
{
    // Identifiers, operators, and whitespace inside braces must pass through untouched
    // even when the expression contains numeric literals to rewrite.
    const std::string out = GeneratePwlItemLine( SIM_MODEL::TYPE::V_PWL,
                                                 "0 0 {t_start + 1u * scale} 1" );
    BOOST_TEST_INFO( out );
    BOOST_CHECK( out.find( "{t_start + 1u * scale}" ) != std::string::npos );
}


BOOST_AUTO_TEST_CASE( PwlBraceLeavesExistingMegSpelling )
{
    // A user who already typed ngspice's 'Meg' inside a brace must not see it mangled.
    const std::string out = GeneratePwlItemLine( SIM_MODEL::TYPE::V_PWL,
                                                 "0 0 {1Meg + t_start} 1" );
    BOOST_TEST_INFO( out );
    BOOST_CHECK( out.find( "{1Meg + t_start}" ) != std::string::npos );
}


BOOST_AUTO_TEST_SUITE_END()
