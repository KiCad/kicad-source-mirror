/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <layer_ids.h>

BOOST_AUTO_TEST_SUITE( LSet )

struct LSETS_TO_TEST
{
    LSET lset;

    std::string expectedFmtHex;
    std::string expectedFmtBin;
};


const static std::vector<LSETS_TO_TEST> type_to_ext_cases = {
    { LSET( 2, F_Cu, F_Fab ), "0020000_00000001",
            "0000|0000_0010|0000_0000|0000_0000|0000_0000|0000_0000|0000_0000|0000_0001" },
    { LSET( 3, In14_Cu, B_Adhes, Rescue ), "8000001_00004000",
      "1000|0000_0000|0000_0000|0000_0001|0000_0000|0000_0000|0100_0000|0000_0000" }
};


BOOST_AUTO_TEST_CASE( FmtHex )
{
    for( const auto& c : type_to_ext_cases )
    {
        BOOST_CHECK_EQUAL( c.expectedFmtHex, c.lset.FmtHex() );
    }
}


BOOST_AUTO_TEST_CASE( FmtBin )
{
    for( const auto& c : type_to_ext_cases )
    {
        BOOST_CHECK_EQUAL( c.expectedFmtBin, c.lset.FmtBin() );
    }
}


BOOST_AUTO_TEST_SUITE_END()
