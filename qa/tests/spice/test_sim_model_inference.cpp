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
#include <sim/sim_model.h>
#include <sch_pin.h>
#include <lib_symbol.h>

class TEST_SIM_MODEL_INFERENCE
{
public:
    TEST_SIM_MODEL_INFERENCE()
    {}
};


BOOST_FIXTURE_TEST_SUITE( SimModelInference, TEST_SIM_MODEL_INFERENCE )


BOOST_AUTO_TEST_CASE( InferPassiveValues )
{
    struct PASSIVE_VALUE_TEST_CASE
    {
        wxString reference;
        wxString value;
        wxString result;
    };

    const std::vector<PASSIVE_VALUE_TEST_CASE> testCases =
    {
        { wxString( "C1" ),  wxString( "33M" ),          wxString( "c=\"33Meg\"" )     },
        { wxString( "C2" ),  wxString( "33m" ),          wxString( "c=\"33m\"" )       },
        { wxString( "C3" ),  wxString( "33Meg" ),        wxString( "c=\"33Meg\"" )     },

        { wxString( "C4" ),  wxString( "33,000uF" ),     wxString( "c=\"33000u\"" )    },
        { wxString( "C5" ),  wxString( "3,300uF" ),      wxString( "c=\"3300u\"" )     },
        { wxString( "C6" ),  wxString( "33000uF" ),      wxString( "c=\"33000u\"" )    },
        { wxString( "C7" ),  wxString( "3300uF" ),       wxString( "c=\"3300u\"" )     },
        { wxString( "C8" ),  wxString( "3.3uF" ),        wxString( "c=\"3.3u\"" )      },
        { wxString( "C9" ),  wxString( "3,3uF" ),        wxString( "c=\"3.3u\"" )      },
        { wxString( "C10" ), wxString( "3,32uF" ),       wxString( "c=\"3.32u\"" )     },

        { wxString( "C11" ), wxString( "3u3" ),          wxString( "c=\"3.3u\"" )      },
        { wxString( "C12" ), wxString( "3p3" ),          wxString( "c=\"3.3p\"" )      },
        { wxString( "C13" ), wxString( "3u32" ),         wxString( "c=\"3.32u\"" )     },

        { wxString( "R1" ),  wxString( "3R3" ),          wxString( "r=\"3.3\"" )       },
        { wxString( "R2" ),  wxString( "3K3" ),          wxString( "r=\"3.3K\"" )      },
        { wxString( "R3" ),  wxString( "3K32" ),         wxString( "r=\"3.32K\"" )     },

        { wxString( "R4" ),  wxString( "3,000.5" ),      wxString( "r=\"3000.5\"" )    },
        { wxString( "R5" ),  wxString( "3.000,5" ),      wxString( "r=\"3000.5\"" )    },
        { wxString( "R6" ),  wxString( "3000.5" ),       wxString( "r=\"3000.5\"" )    },
        { wxString( "R7" ),  wxString( "3,000K" ),       wxString( "r=\"3000K\"" )     },
        { wxString( "R8" ),  wxString( "3.000K" ),       wxString( "r=\"3.000K\"" )    },
        { wxString( "R9" ),  wxString( "3.0,000,000" ),  wxString( "" )                },

        { wxString( "X1" ),  wxString( "3.3K" ),         wxString( "" )                },

        { wxString( "C14" ), wxString( "33,000,000uF" ), wxString( "c=\"33000000u\"" ) },
        { wxString( "C15" ), wxString( "33 000 000uF" ), wxString( "c=\"33000000u\"" ) },
        { wxString( "C16" ), wxString( "33.000,000uF" ), wxString( "c=\"33000.000u\"" )},
    };

    std::unique_ptr<LIB_SYMBOL> symbol = std::make_unique<LIB_SYMBOL>( "symbol", nullptr );
    symbol->AddDrawItem( new SCH_PIN( symbol.get() ) );
    symbol->AddDrawItem( new SCH_PIN( symbol.get() ) );

    wxString deviceType;
    wxString modelType;
    wxString modelParams;
    wxString pinMap;
    wxString msg;

    for( const auto& testCase : testCases )
    {
        symbol->GetReferenceField().SetText( testCase.reference );
        symbol->GetValueField().SetText( testCase.value );

        std::vector<SCH_FIELD> fields;
        fields.emplace_back( symbol->GetReferenceField() );
        fields.emplace_back( symbol->GetValueField() );

        SIM_MODEL::InferSimModel( *symbol, &fields, false, 0, SIM_VALUE_GRAMMAR::NOTATION::SPICE,
                                  &deviceType, &modelType, &modelParams, &pinMap );

        msg.Printf( "Passive model inference %s %s failed [%s != %s]",
                    testCase.reference,
                    testCase.value,
                    modelParams,
                    testCase.result );
        BOOST_CHECK_MESSAGE( modelParams == testCase.result,  msg.ToStdString() );
    }
}


BOOST_AUTO_TEST_SUITE_END()
