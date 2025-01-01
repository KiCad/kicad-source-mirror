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

#include <increment.h>


/**
 * Declares a struct as the Boost test fixture.
 */
BOOST_AUTO_TEST_SUITE( StringIncrement )


struct INCREMENT_TEST_CASE
{
    wxString input;
    int      delta;
    size_t   part;
    wxString expected;
};


/**
 * Check formatting the point
 */
BOOST_AUTO_TEST_CASE( BasicCase )
{
    const std::vector<INCREMENT_TEST_CASE> cases{
        // Null
        { "", 1, 0, "nullopt" },
        { "", 1, 1, "nullopt" },
        { "", -1, 1, "nullopt" },
        // Up
        { "1", 1, 0, "2" },
        { "1", 9, 0, "10" },
        // Down
        { "2", -1, 0, "1" },
        { "10", -1, 0, "9" },
        // Down from 0
        { "0", -1, 0, "nullopt" },
        // Ran out of a parts
        { "1", 1, 1, "nullopt" },
        // Leading zeros preserved
        { "01", 1, 0, "02" },
        // Alpha
        { "A", 1, 0, "B" },
        { "E", -1, 0, "D" },
        // Skip I
        { "H", 1, 0, "J" },
        { "J", -1, 0, "H" },
        // But I works if it's there
        { "I", 1, 0, "J" },
        { "I", -1, 0, "H" },
        // Alpha wrap
        { "Z", 1, 0, "AA" },
        // Reject huge alphabetic value
        { "ABB", 1, 0, "nullopt" },
        // Dashes skipped
        { "A-1", 1, 0, "A-2" },
        { "A-1", 1, 1, "B-1" },
    };

    STRING_INCREMENTER incrementer;
    incrementer.SetSkipIOSQXZ( true );

    for( const auto& c : cases )
    {
        BOOST_TEST_INFO( "Input: " << c.input << " Delta: " << c.delta << " Part: " << c.part );
        wxString result = incrementer.Increment( c.input, c.delta, c.part ).value_or( "nullopt" );
        BOOST_CHECK_EQUAL( result, c.expected );
    }
}


struct ALPHABETIC_TEST_CASE
{
    wxString        input;
    const wxString& alphabet;
    int             expected;
};


BOOST_AUTO_TEST_CASE( AlphabeticIndexes )
{
    const wxString alphabet = "ABCDEFGHJKLMNPRTUVWY";

    const std::vector<ALPHABETIC_TEST_CASE> cases{ {
            { "A", alphabet, 0 },
            { "B", alphabet, 1 },
            { "Y", alphabet, 19 },
            { "AA", alphabet, 20 },
            { "AY", alphabet, 39 },
    } };

    for( const auto& c : cases )
    {
        BOOST_TEST_INFO( "Input: " << c.input << " <-> " << c.expected );

        const int fromString = IndexFromAlphabetic( c.input, c.alphabet );
        BOOST_CHECK_EQUAL( fromString, c.expected );

        const wxString fromIndex = AlphabeticFromIndex( c.expected, c.alphabet, true );
        BOOST_CHECK_EQUAL( fromIndex, c.input );
    }
}


BOOST_AUTO_TEST_SUITE_END()
