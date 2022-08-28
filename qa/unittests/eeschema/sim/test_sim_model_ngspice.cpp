/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.TXT for contributors.
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

class TEST_SIM_MODEL_NGSPICE_FIXTURE : public SIM_MODEL_NGSPICE
{
public:
    TEST_SIM_MODEL_NGSPICE_FIXTURE() : SIM_MODEL_NGSPICE( TYPE::NONE ) {}
};


BOOST_FIXTURE_TEST_SUITE( SimModelNgspice, TEST_SIM_MODEL_NGSPICE_FIXTURE )


BOOST_AUTO_TEST_CASE( Models )
{
    // Count the total number of model and instance parameters for each model so that there will be
    // an error if someone accidentally removes a parameter.

    for( MODEL_TYPE type : MODEL_TYPE_ITERATOR() )
    {
        unsigned modelParamCount = ModelInfo( type ).modelParams.size();
        unsigned instanceParamCount = ModelInfo( type ).instanceParams.size();

        switch( type )
        {
        case MODEL_TYPE::NONE:
        case MODEL_TYPE::_ENUM_END:
            break;

        /*case MODEL_TYPE::RESISTOR:
            BOOST_CHECK_EQUAL( modelParamCount, 22 );
            BOOST_CHECK_EQUAL( instanceParamCount, 25 );
            break;
        
        case MODEL_TYPE::CAPACITOR:
            BOOST_CHECK_EQUAL( modelParamCount, 19 );
            BOOST_CHECK_EQUAL( instanceParamCount, 22 );
            break;
        
        case MODEL_TYPE::INDUCTOR:
            BOOST_CHECK_EQUAL( modelParamCount, 9 );
            BOOST_CHECK_EQUAL( instanceParamCount, 20 );
            break;*/
        
        /*case MODEL_TYPE::LTRA:
            BOOST_CHECK_EQUAL( modelParamCount, 18 );
            BOOST_CHECK_EQUAL( instanceParamCount, 9 );
            break;
        
        case MODEL_TYPE::TRANLINE:
            BOOST_CHECK_EQUAL( modelParamCount, 0 );
            BOOST_CHECK_EQUAL( instanceParamCount, 17 );
            break;
        
        case MODEL_TYPE::URC:
            BOOST_CHECK_EQUAL( modelParamCount, 7 );
            BOOST_CHECK_EQUAL( instanceParamCount, 5 );
            break;*/
        
        /*case MODEL_TYPE::TRANSLINE:
            BOOST_CHECK_EQUAL( modelParamCount, 6 );
            BOOST_CHECK_EQUAL( instanceParamCount, 3 );
            break;*/

        /*case MODEL_TYPE::SWITCH:
            BOOST_CHECK_EQUAL( modelParamCount, 7 );
            BOOST_CHECK_EQUAL( instanceParamCount, 8 );
            break;
        
        case MODEL_TYPE::CSWITCH:
            BOOST_CHECK_EQUAL( modelParamCount, 7 );
            BOOST_CHECK_EQUAL( instanceParamCount, 7 );
            break;*/
        
        case MODEL_TYPE::DIODE:
            BOOST_CHECK_EQUAL( modelParamCount, 76 );
            BOOST_CHECK_EQUAL( instanceParamCount, 30 );
            break;
        
        case MODEL_TYPE::BJT:
            BOOST_CHECK_EQUAL( modelParamCount, 152 );
            BOOST_CHECK_EQUAL( instanceParamCount, 53 );
            break;
        
        case MODEL_TYPE::VBIC:
            BOOST_CHECK_EQUAL( modelParamCount, 117 );
            BOOST_CHECK_EQUAL( instanceParamCount, 45 );
            break;
        
        case MODEL_TYPE::HICUM2:
            BOOST_CHECK_EQUAL( modelParamCount, 149 );
            BOOST_CHECK_EQUAL( instanceParamCount, 61 );
            break;
        
        case MODEL_TYPE::JFET:
            BOOST_CHECK_EQUAL( modelParamCount, 28 );
            BOOST_CHECK_EQUAL( instanceParamCount, 28 );
            break;
        
        case MODEL_TYPE::JFET2:
            BOOST_CHECK_EQUAL( modelParamCount, 46 );
            BOOST_CHECK_EQUAL( instanceParamCount, 30 );
            break;
        
        case MODEL_TYPE::MES:
            BOOST_CHECK_EQUAL( modelParamCount, 22 );
            BOOST_CHECK_EQUAL( instanceParamCount, 25 );
            break;
        
        case MODEL_TYPE::MESA:
            BOOST_CHECK_EQUAL( modelParamCount, 71 );
            BOOST_CHECK_EQUAL( instanceParamCount, 30 );
            break;
        
        case MODEL_TYPE::HFET1:
            BOOST_CHECK_EQUAL( modelParamCount, 68 );
            BOOST_CHECK_EQUAL( instanceParamCount, 28 );
            break;
        
        case MODEL_TYPE::HFET2:
            BOOST_CHECK_EQUAL( modelParamCount, 40 );
            BOOST_CHECK_EQUAL( instanceParamCount, 28 );
            break;
        
        case MODEL_TYPE::MOS1:
            BOOST_CHECK_EQUAL( modelParamCount, 35 );
            BOOST_CHECK_EQUAL( instanceParamCount, 76 );
            break;
        
        case MODEL_TYPE::MOS2:
            BOOST_CHECK_EQUAL( modelParamCount, 42 );
            BOOST_CHECK_EQUAL( instanceParamCount, 76 );
            break;
        
        case MODEL_TYPE::MOS3:
            BOOST_CHECK_EQUAL( modelParamCount, 48 );
            BOOST_CHECK_EQUAL( instanceParamCount, 81 );
            break;
        
        case MODEL_TYPE::BSIM1:
            BOOST_CHECK_EQUAL( modelParamCount, 81 );
            BOOST_CHECK_EQUAL( instanceParamCount, 14 );
            break;
        
        case MODEL_TYPE::BSIM2:
            BOOST_CHECK_EQUAL( modelParamCount, 137 );
            BOOST_CHECK_EQUAL( instanceParamCount, 14 );
            break;
        
        case MODEL_TYPE::MOS6:
            BOOST_CHECK_EQUAL( modelParamCount, 42 );
            BOOST_CHECK_EQUAL( instanceParamCount, 78 );
            break;
        
        case MODEL_TYPE::BSIM3:
            BOOST_CHECK_EQUAL( modelParamCount, 429 );
            BOOST_CHECK_EQUAL( instanceParamCount, 46 );
            break;
        
        case MODEL_TYPE::MOS9:
            BOOST_CHECK_EQUAL( modelParamCount, 48 );
            BOOST_CHECK_EQUAL( instanceParamCount, 81 );
            break;
        
        case MODEL_TYPE::B4SOI:
            BOOST_CHECK_EQUAL( modelParamCount, 915 );
            BOOST_CHECK_EQUAL( instanceParamCount, 74 );
            break;
        
        case MODEL_TYPE::BSIM4:
            BOOST_CHECK_EQUAL( modelParamCount, 892 );
            BOOST_CHECK_EQUAL( instanceParamCount, 84 );
            break;
        
        case MODEL_TYPE::B3SOIFD:
            BOOST_CHECK_EQUAL( modelParamCount, 393 );
            BOOST_CHECK_EQUAL( instanceParamCount, 27 );
            break;
        
        case MODEL_TYPE::B3SOIDD:
            BOOST_CHECK_EQUAL( modelParamCount, 393 );
            BOOST_CHECK_EQUAL( instanceParamCount, 27 );
            break;
        
        case MODEL_TYPE::B3SOIPD:
            BOOST_CHECK_EQUAL( modelParamCount, 470 );
            BOOST_CHECK_EQUAL( instanceParamCount, 36 );
            break;
        
        case MODEL_TYPE::HISIM2:
            BOOST_CHECK_EQUAL( modelParamCount, 486 );
            BOOST_CHECK_EQUAL( instanceParamCount, 59 );
            break;
        
        case MODEL_TYPE::HISIMHV1:
            BOOST_CHECK_EQUAL( modelParamCount, 610 );
            BOOST_CHECK_EQUAL( instanceParamCount, 72 );
            break;
        
        case MODEL_TYPE::HISIMHV2:
            BOOST_CHECK_EQUAL( modelParamCount, 730 );
            BOOST_CHECK_EQUAL( instanceParamCount, 74 );
            break;
        
        default:
            BOOST_FAIL( wxString::Format(
                        "Unhandled type: %d "
                        "(if you created a new type you need to handle it in this switch statement)",
                        type ) );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
