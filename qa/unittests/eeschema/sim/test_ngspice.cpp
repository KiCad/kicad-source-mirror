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
#include <sim/ngspice.h>

BOOST_AUTO_TEST_SUITE( Ngspice )


BOOST_AUTO_TEST_CASE( Models )
{
    // Count the total number of model and instance parameters for each model so that there will be
    // an error if someone accidentally removes a parameter.

    for( NGSPICE::MODEL_TYPE type : NGSPICE::MODEL_TYPE_ITERATOR() )
    {
        unsigned int modelParamCount = NGSPICE::ModelInfo( type ).modelParams.size();
        unsigned int instanceParamCount = NGSPICE::ModelInfo( type ).instanceParams.size();
        
        switch( type )
        {
        case NGSPICE::MODEL_TYPE::NONE:
        case NGSPICE::MODEL_TYPE::_ENUM_END:
            break;

        case NGSPICE::MODEL_TYPE::RESISTOR:
            BOOST_CHECK_EQUAL( modelParamCount, 22 );
            BOOST_CHECK_EQUAL( instanceParamCount, 25 );
            break;
        
        case NGSPICE::MODEL_TYPE::CAPACITOR:
            BOOST_CHECK_EQUAL( modelParamCount, 19 );
            BOOST_CHECK_EQUAL( instanceParamCount, 22 );
            break;
        
        case NGSPICE::MODEL_TYPE::INDUCTOR:
            BOOST_CHECK_EQUAL( modelParamCount, 9 );
            BOOST_CHECK_EQUAL( instanceParamCount, 20 );
            break;
        
        case NGSPICE::MODEL_TYPE::LTRA:
            BOOST_CHECK_EQUAL( modelParamCount, 18 );
            BOOST_CHECK_EQUAL( instanceParamCount, 9 );
            break;
        
        case NGSPICE::MODEL_TYPE::TRANLINE:
            BOOST_CHECK_EQUAL( modelParamCount, 0 );
            BOOST_CHECK_EQUAL( instanceParamCount, 17 );
            break;
        
        case NGSPICE::MODEL_TYPE::URC:
            BOOST_CHECK_EQUAL( modelParamCount, 7 );
            BOOST_CHECK_EQUAL( instanceParamCount, 5 );
            break;
        
        case NGSPICE::MODEL_TYPE::TRANSLINE:
            BOOST_CHECK_EQUAL( modelParamCount, 6 );
            BOOST_CHECK_EQUAL( instanceParamCount, 3 );
            break;
        
        case NGSPICE::MODEL_TYPE::DIODE:
            BOOST_CHECK_EQUAL( modelParamCount, 76 );
            BOOST_CHECK_EQUAL( instanceParamCount, 30 );
            break;
        
        case NGSPICE::MODEL_TYPE::BJT:
            BOOST_CHECK_EQUAL( modelParamCount, 152 );
            BOOST_CHECK_EQUAL( instanceParamCount, 52 );
            break;
        
        case NGSPICE::MODEL_TYPE::VBIC:
            BOOST_CHECK_EQUAL( modelParamCount, 117 );
            BOOST_CHECK_EQUAL( instanceParamCount, 44 );
            break;
        
        case NGSPICE::MODEL_TYPE::HICUM2:
            BOOST_CHECK_EQUAL( modelParamCount, 149 );
            BOOST_CHECK_EQUAL( instanceParamCount, 60 );
            break;
        
        case NGSPICE::MODEL_TYPE::JFET:
            BOOST_CHECK_EQUAL( modelParamCount, 28 );
            BOOST_CHECK_EQUAL( instanceParamCount, 28 );
            break;
        
        case NGSPICE::MODEL_TYPE::JFET2:
            BOOST_CHECK_EQUAL( modelParamCount, 39 );
            BOOST_CHECK_EQUAL( instanceParamCount, 30 );
            break;
        
        case NGSPICE::MODEL_TYPE::MES:
            BOOST_CHECK_EQUAL( modelParamCount, 22 );
            BOOST_CHECK_EQUAL( instanceParamCount, 25 );
            break;
        
        case NGSPICE::MODEL_TYPE::MESA:
            BOOST_CHECK_EQUAL( modelParamCount, 51 );
            BOOST_CHECK_EQUAL( instanceParamCount, 30 );
            break;
        
        case NGSPICE::MODEL_TYPE::HFET1:
            BOOST_CHECK_EQUAL( modelParamCount, 22 );
            BOOST_CHECK_EQUAL( instanceParamCount, 28 );
            break;
        
        case NGSPICE::MODEL_TYPE::HFET2:
            BOOST_CHECK_EQUAL( modelParamCount, 19 );
            BOOST_CHECK_EQUAL( instanceParamCount, 28 );
            break;
        
        case NGSPICE::MODEL_TYPE::MOS1:
            BOOST_CHECK_EQUAL( modelParamCount, 35 );
            BOOST_CHECK_EQUAL( instanceParamCount, 76 );
            break;
        
        case NGSPICE::MODEL_TYPE::MOS2:
            BOOST_CHECK_EQUAL( modelParamCount, 42 );
            BOOST_CHECK_EQUAL( instanceParamCount, 76 );
            break;
        
        case NGSPICE::MODEL_TYPE::MOS3:
            BOOST_CHECK_EQUAL( modelParamCount, 48 );
            BOOST_CHECK_EQUAL( instanceParamCount, 81 );
            break;
        
        case NGSPICE::MODEL_TYPE::BSIM1:
            BOOST_CHECK_EQUAL( modelParamCount, 81 );
            BOOST_CHECK_EQUAL( instanceParamCount, 14 );
            break;
        
        case NGSPICE::MODEL_TYPE::BSIM2:
            BOOST_CHECK_EQUAL( modelParamCount, 137 );
            BOOST_CHECK_EQUAL( instanceParamCount, 14 );
            break;
        
        case NGSPICE::MODEL_TYPE::MOS6:
            BOOST_CHECK_EQUAL( modelParamCount, 42 );
            BOOST_CHECK_EQUAL( instanceParamCount, 78 );
            break;
        
        case NGSPICE::MODEL_TYPE::BSIM3:
            BOOST_CHECK_EQUAL( modelParamCount, 429 );
            BOOST_CHECK_EQUAL( instanceParamCount, 46 );
            break;
        
        case NGSPICE::MODEL_TYPE::MOS9:
            BOOST_CHECK_EQUAL( modelParamCount, 48 );
            BOOST_CHECK_EQUAL( instanceParamCount, 81 );
            break;
        
        case NGSPICE::MODEL_TYPE::B4SOI:
            BOOST_CHECK_EQUAL( modelParamCount, 915 );
            BOOST_CHECK_EQUAL( instanceParamCount, 74 );
            break;
        
        case NGSPICE::MODEL_TYPE::BSIM4:
            BOOST_CHECK_EQUAL( modelParamCount, 892 );
            BOOST_CHECK_EQUAL( instanceParamCount, 84 );
            break;
        
        case NGSPICE::MODEL_TYPE::B3SOIFD:
            BOOST_CHECK_EQUAL( modelParamCount, 393 );
            BOOST_CHECK_EQUAL( instanceParamCount, 27 );
            break;
        
        case NGSPICE::MODEL_TYPE::B3SOIDD:
            BOOST_CHECK_EQUAL( modelParamCount, 393 );
            BOOST_CHECK_EQUAL( instanceParamCount, 27 );
            break;
        
        case NGSPICE::MODEL_TYPE::B3SOIPD:
            BOOST_CHECK_EQUAL( modelParamCount, 470 );
            BOOST_CHECK_EQUAL( instanceParamCount, 36 );
            break;
        
        case NGSPICE::MODEL_TYPE::HISIM2:
            BOOST_CHECK_EQUAL( modelParamCount, 486 );
            BOOST_CHECK_EQUAL( instanceParamCount, 59 );
            break;
        
        case NGSPICE::MODEL_TYPE::HISIMHV1:
            BOOST_CHECK_EQUAL( modelParamCount, 536 );
            BOOST_CHECK_EQUAL( instanceParamCount, 66 );
            break;
        
        case NGSPICE::MODEL_TYPE::HISIMHV2:
            BOOST_CHECK_EQUAL( modelParamCount, 630 );
            BOOST_CHECK_EQUAL( instanceParamCount, 68 );
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
