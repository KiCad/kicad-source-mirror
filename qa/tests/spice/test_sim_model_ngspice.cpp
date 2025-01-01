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
#include <sim/sim_model_ngspice.h>
#include <boost/algorithm/string/predicate.hpp>


class TEST_SIM_MODEL_NGSPICE_FIXTURE : public SIM_MODEL_NGSPICE
{
public:
    TEST_SIM_MODEL_NGSPICE_FIXTURE() :
            SIM_MODEL_NGSPICE( TYPE::NONE )
    {}
};


BOOST_FIXTURE_TEST_SUITE( SimModelNgspice, TEST_SIM_MODEL_NGSPICE_FIXTURE )


BOOST_AUTO_TEST_CASE( ParamDuplicates )
{
    for( MODEL_TYPE type : MODEL_TYPE_ITERATOR() )
    {
        BOOST_TEST_CONTEXT( "Model name: " << ModelInfo( type ).name )
        {
            const std::vector<SIM_MODEL::PARAM::INFO> modelParams = ModelInfo( type ).modelParams;
            const std::vector<SIM_MODEL::PARAM::INFO> instanceParams = ModelInfo( type ).instanceParams;

            for( const SIM_MODEL::PARAM::INFO& modelParam : modelParams )
            {
                BOOST_TEST_CONTEXT( "Model param name: " << modelParam.name )
                {
                    // Ensure there's no model parameters that have the same name.
                    BOOST_CHECK( std::none_of( modelParams.begin(), modelParams.end(),
                                               [&modelParam]( const auto& aOtherModelParam )
                                               {
                                                   return modelParam.id != aOtherModelParam.id
                                                       && modelParam.name == aOtherModelParam.name;
                                               } ) );

                    // Ensure there's no model parameters that have the same name as an
                    // instance parameter.
                    BOOST_CHECK( std::none_of( instanceParams.begin(), instanceParams.end(),
                                               [&modelParam]( const auto& aInstanceParam )
                                               {
                                                   return modelParam.name == aInstanceParam.name;
                                               } ) );

                    if( boost::ends_with( modelParam.name, "_" ) )
                    {
                        // Ensure that for each model parameter ending with a "_" there exists an
                        // instance parameter with the same name but without this final character.
                        // We append an "_" to model parameters to disambiguate from these
                        // corresponding instance parameters.
                        BOOST_CHECK( std::any_of( instanceParams.begin(), instanceParams.end(),
                                [&modelParam]( const auto& aInstanceParam )
                                {
                                    return modelParam.name.substr( 0, modelParam.name.length() - 1 )
                                        == aInstanceParam.name;
                                } ) );
                    }
                }
            }

            // Ensure there's no instance parameters that have the same name.
            for( const SIM_MODEL::PARAM::INFO& instanceParam : instanceParams )
            {
                BOOST_TEST_CONTEXT( "Instance param name: " << instanceParam.name )
                {
                    BOOST_CHECK( std::none_of( instanceParams.begin(), instanceParams.end(),
                                               [&instanceParam]( const auto& aOtherInstanceParam )
                                               {
                                                   return instanceParam.id != aOtherInstanceParam.id
                                                       && instanceParam.dir != SIM_MODEL::PARAM::DIR_OUT
                                                       && aOtherInstanceParam.dir != SIM_MODEL::PARAM::DIR_OUT
                                                       && instanceParam.name == aOtherInstanceParam.name;
                                               } ) );
                }
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( ParamCount )
{
    // Count the total number of model and instance parameters for each model so that there will be
    // an error if someone accidentally removes a parameter.

    for( MODEL_TYPE type : MODEL_TYPE_ITERATOR() )
    {
        const std::vector<SIM_MODEL::PARAM::INFO> modelParams = ModelInfo( type ).modelParams;
        const std::vector<SIM_MODEL::PARAM::INFO> instanceParams = ModelInfo( type ).instanceParams;

        switch( type )
        {
        case MODEL_TYPE::NONE:
        case MODEL_TYPE::_ENUM_END:
            break;

        /*case MODEL_TYPE::RESISTOR:
            BOOST_CHECK_EQUAL( modelParams.size(), 22 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 25 );
            break;

        case MODEL_TYPE::CAPACITOR:
            BOOST_CHECK_EQUAL( modelParams.size(), 19 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 22 );
            break;

        case MODEL_TYPE::INDUCTOR:
            BOOST_CHECK_EQUAL( modelParams.size(), 9 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 20 );
            break;*/

        /*case MODEL_TYPE::LTRA:
            BOOST_CHECK_EQUAL( modelParams.size(), 18 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 9 );
            break;

        case MODEL_TYPE::TRANLINE:
            BOOST_CHECK_EQUAL( modelParams.size(), 0 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 17 );
            break;

        case MODEL_TYPE::URC:
            BOOST_CHECK_EQUAL( modelParams.size(), 7 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 5 );
            break;*/

        /*case MODEL_TYPE::TRANSLINE:
            BOOST_CHECK_EQUAL( modelParams.size(), 6 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 3 );
            break;*/

        /*case MODEL_TYPE::SWITCH:
            BOOST_CHECK_EQUAL( modelParams.size(), 7 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 8 );
            break;

        case MODEL_TYPE::CSWITCH:
            BOOST_CHECK_EQUAL( modelParams.size(), 7 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 7 );
            break;*/

        case MODEL_TYPE::DIODE:
            BOOST_CHECK_EQUAL( modelParams.size(), 76 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 30 );
            break;

        case MODEL_TYPE::BJT:
            BOOST_CHECK_EQUAL( modelParams.size(), 152 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 53 );
            break;

        case MODEL_TYPE::VBIC:
            BOOST_CHECK_EQUAL( modelParams.size(), 117 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 45 );
            break;

        case MODEL_TYPE::HICUM2:
            BOOST_CHECK_EQUAL( modelParams.size(), 149 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 61 );
            break;

        case MODEL_TYPE::JFET:
            BOOST_CHECK_EQUAL( modelParams.size(), 34 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 28 );
            break;

        case MODEL_TYPE::JFET2:
            BOOST_CHECK_EQUAL( modelParams.size(), 46 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 30 );
            break;

        case MODEL_TYPE::MES:
            BOOST_CHECK_EQUAL( modelParams.size(), 22 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 25 );
            break;

        case MODEL_TYPE::MESA:
            BOOST_CHECK_EQUAL( modelParams.size(), 71 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 30 );
            break;

        case MODEL_TYPE::HFET1:
            BOOST_CHECK_EQUAL( modelParams.size(), 68 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 28 );
            break;

        case MODEL_TYPE::HFET2:
            BOOST_CHECK_EQUAL( modelParams.size(), 40 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 28 );
            break;

        case MODEL_TYPE::VDMOS:
            BOOST_CHECK_EQUAL( modelParams.size(), 69 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 36 );
            break;

        case MODEL_TYPE::MOS1:
            BOOST_CHECK_EQUAL( modelParams.size(), 35 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 76 );
            break;

        case MODEL_TYPE::MOS2:
            BOOST_CHECK_EQUAL( modelParams.size(), 42 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 76 );
            break;

        case MODEL_TYPE::MOS3:
            BOOST_CHECK_EQUAL( modelParams.size(), 48 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 81 );
            break;

        case MODEL_TYPE::BSIM1:
            BOOST_CHECK_EQUAL( modelParams.size(), 81 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 14 );
            break;

        case MODEL_TYPE::BSIM2:
            BOOST_CHECK_EQUAL( modelParams.size(), 137 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 14 );
            break;

        case MODEL_TYPE::MOS6:
            BOOST_CHECK_EQUAL( modelParams.size(), 42 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 78 );
            break;

        case MODEL_TYPE::BSIM3:
            BOOST_CHECK_EQUAL( modelParams.size(), 429 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 46 );
            break;

        case MODEL_TYPE::MOS9:
            BOOST_CHECK_EQUAL( modelParams.size(), 48 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 81 );
            break;

        case MODEL_TYPE::B4SOI:
            BOOST_CHECK_EQUAL( modelParams.size(), 915 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 74 );
            break;

        case MODEL_TYPE::BSIM4:
            BOOST_CHECK_EQUAL( modelParams.size(), 892 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 84 );
            break;

        case MODEL_TYPE::B3SOIFD:
            BOOST_CHECK_EQUAL( modelParams.size(), 393 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 27 );
            break;

        case MODEL_TYPE::B3SOIDD:
            BOOST_CHECK_EQUAL( modelParams.size(), 393 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 27 );
            break;

        case MODEL_TYPE::B3SOIPD:
            BOOST_CHECK_EQUAL( modelParams.size(), 470 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 36 );
            break;

        case MODEL_TYPE::HISIM2:
            BOOST_CHECK_EQUAL( modelParams.size(), 486 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 59 );
            break;

        case MODEL_TYPE::HISIMHV1:
            BOOST_CHECK_EQUAL( modelParams.size(), 610 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 72 );
            break;

        case MODEL_TYPE::HISIMHV2:
            BOOST_CHECK_EQUAL( modelParams.size(), 730 );
            BOOST_CHECK_EQUAL( instanceParams.size(), 74 );
            break;

        default:
            BOOST_FAIL( wxString::Format(
                        "Unhandled type: %d "
                        "(if you created a new type you need to handle it in this switch "
                        "statement)",
                        type ) );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
