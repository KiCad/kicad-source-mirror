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

#include <array_pad_number_provider.h> // UUT

#include <common.h> // make_unique

#include <footprint.h>
#include <pad.h>


/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( ArrayPadNumberProv )


struct APNP_CASE
{
    std::string                    m_case_name;
    std::set<wxString>             m_existing_pad_numbers;
    std::unique_ptr<ARRAY_OPTIONS> m_arr_opts;
    std::vector<wxString>          m_expected_numbers;
};


/**
 * Get Array Pad Name Provider cases when a footprint is looked at to determine what names
 * are available.
 */
std::vector<APNP_CASE> GetFootprintAPNPCases()
{
    std::vector<APNP_CASE> cases;

    auto opts = std::make_unique<ARRAY_GRID_OPTIONS>();

    // simple linear numbering
    opts->m_2dArrayNumbering = false;
    opts->m_pri_axis.SetOffset( 1 );
    opts->m_pri_axis.SetAxisType( ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC );

    cases.push_back( {
            "Simple linear, skip some",
            { "1", "3" },
            std::move( opts ),
            { "2", "4", "5", "6", "7" },
    } );

    // one without a footprint
    opts = std::make_unique<ARRAY_GRID_OPTIONS>();

    // simple linear numbering (again)
    opts->m_2dArrayNumbering = false;
    opts->m_pri_axis.SetOffset( 1 );
    opts->m_pri_axis.SetAxisType( ARRAY_AXIS::NUMBERING_TYPE::NUMBERING_NUMERIC );

    cases.push_back( {
            "Simple linear, no footprint",
            {}, // not used
            std::move( opts ),
            { "1", "2", "3", "4", "5" },
    } );

    // Grid numberings with skips don't make a lot of sense, there is
    // no particular contract made for them

    return cases;
}


/**
 * Check that an #ARRAY_PAD_NUMBER_PROVIDER provides the right names
 * @param aProvider the provider
 * @param aExpNames ordered list of expected names
 */
void CheckPadNumberProvider( ARRAY_PAD_NUMBER_PROVIDER& aProvider,
                             std::vector<wxString> aExpectedNumbers )
{
    std::vector<wxString> got_numbers;

    for( unsigned i = 0; i < aExpectedNumbers.size(); ++i )
        got_numbers.push_back( aProvider.GetNextPadNumber() );

    BOOST_CHECK_EQUAL_COLLECTIONS( aExpectedNumbers.begin(), aExpectedNumbers.end(),
                                   got_numbers.begin(), got_numbers.end() );
}


BOOST_AUTO_TEST_CASE( FootprintCases )
{
    for( const auto& c : GetFootprintAPNPCases() )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            std::unique_ptr<FOOTPRINT> footprint;

            ARRAY_PAD_NUMBER_PROVIDER apnp( c.m_existing_pad_numbers, *c.m_arr_opts );

            CheckPadNumberProvider( apnp, c.m_expected_numbers );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
