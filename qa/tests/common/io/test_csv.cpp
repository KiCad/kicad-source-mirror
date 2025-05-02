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

#include <io/csv.h>

#include <wx/sstream.h>


/**
 * Define a stream function for logging this type.
 */
template <typename T>
std::ostream& boost_test_print_type( std::ostream& os, const std::vector<std::vector<T>>& aTable )
{
    os << "TABLE[ " << std::endl;
    for( size_t i = 0; i < aTable.size(); ++i )
    {
        const auto& row = aTable[i];
        os << "  Row " << i << " [ ";
        for( size_t j = 0; j < row.size(); ++j )
        {
            os << row[j];
            if( j < row.size() - 1 )
                os << ", ";
        }
        os << "] " << std::endl;
    }
    os << " ]" << std::endl;
    return os;
}


static bool TableDataEqual( const std::vector<std::vector<wxString>>& aExpected,
                            const std::vector<std::vector<wxString>>& aActual )
{
    if( aExpected.size() != aActual.size() )
    {
        BOOST_TEST_MESSAGE( "Row count mismatch: " << aExpected.size() << " != " << aActual.size() );
        return false;
    }

    for( size_t i = 0; i < aExpected.size(); ++i )
    {
        BOOST_TEST_INFO_SCOPE( "Row " << i );
        if( aExpected[i].size() != aActual[i].size() )
            return false;

        for( size_t j = 0; j < aExpected[i].size(); ++j )
        {
            if( aExpected[i][j] != aActual[i][j] )
                return false;
        }
    }

    return true;
}


BOOST_AUTO_TEST_SUITE( CsvTests )


struct CsvRoundTripCase
{
    wxString                           m_name;
    std::vector<std::vector<wxString>> m_rows;
    wxString                           m_expected;
};


BOOST_AUTO_TEST_CASE( BasicRoundTrips )
{
    // clang-format off
    static const std::vector<CsvRoundTripCase> testCases = {
        {
            "Basic CSV, Double Quoted, Backslash escaped",
            {
                { "Head 1", "Head 2", "Head, \"3\"" },
                { "Row 1 Col 1", "Row 1 Col 2", "Row 1 Col 3" }
            },
            "\"Head 1\",\"Head 2\",\"Head, \"\"3\"\"\"\n"
            "\"Row 1 Col 1\",\"Row 1 Col 2\",\"Row 1 Col 3\"\n",
        },
    };
    // clang-format on

    for( const auto& testCase : testCases )
    {
        BOOST_TEST_INFO_SCOPE( testCase.m_name );

        wxStringOutputStream os;
        CSV_WRITER           writer( os );
        writer.WriteLines( testCase.m_rows );

        BOOST_CHECK_EQUAL( os.GetString(), testCase.m_expected );

        std::vector<std::vector<wxString>> readRows;

        bool result = AutoDecodeCSV( os.GetString(), readRows );
        BOOST_CHECK( result );
        BOOST_CHECK_PREDICATE( TableDataEqual, ( testCase.m_rows )( readRows ) );
    }
}


struct CsvDecodeCase
{
    wxString                           m_name;
    wxString                           m_input;
    std::vector<std::vector<wxString>> m_expectedRows;
};


BOOST_AUTO_TEST_CASE( BasicDecode )
{
    // clang-format off
    static const std::vector<CsvDecodeCase> testCases = {
        {
            "Basic TSV, Double Quoted",
            "\"Head 1\"\t\"Head 2\"\t\"Head, 3\"\n"
            "\"Row 1 Col 1\"\t\"Row 1 Col 2\"\t\"Row 1 Col 3\"\n",
            {
                { "Head 1", "Head 2", "Head, 3" },
                { "Row 1 Col 1", "Row 1 Col 2", "Row 1 Col 3" }
            },
        }
    };
    // clang-format on

    for( const auto& testCase : testCases )
    {
        BOOST_TEST_INFO_SCOPE( testCase.m_name );

        std::vector<std::vector<wxString>> readRows;

        bool result = AutoDecodeCSV( testCase.m_input, readRows );
        BOOST_CHECK( result );
        BOOST_CHECK_PREDICATE( TableDataEqual, ( testCase.m_expectedRows )( readRows ) );
    }
}


BOOST_AUTO_TEST_SUITE_END()
