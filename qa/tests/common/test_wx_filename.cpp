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

/**
 * @file
 * Test suite for WX_FILNAME
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <wx_filename.h>

/**
 * Declare the test suite
 */
BOOST_AUTO_TEST_SUITE( WxFilename )


struct WX_FILENAME_SPLIT_CASE
{
    // Ctor params
    std::string m_path;
    std::string m_name;

    // Split results
    std::string m_exp_name;
    std::string m_exp_full_name;
    std::string m_exp_path;
    std::string m_exp_full_path;
};


// clang-format off
static const std::vector<WX_FILENAME_SPLIT_CASE> split_cases = {
    {
        "",
        "",
        "",
        "",
        "",
        "/", // This doesn't look right...
    },
    {
        "",
        "name.ext",
        "name",
        "name.ext",
        "",
        "/name.ext", // This doesn't look right...
    },
    {
        "/tmp/example",
        "",
        "",
        "",
        "/tmp/example",
        "/tmp/example/",
    },
    {
        "/tmp/example",
        "name.ext",
        "name",
        "name.ext",
        "/tmp/example",
        "/tmp/example/name.ext",
    },
    {
        "/tmp/example",
        "name", // no extension
        "name",
        "name",
        "/tmp/example",
        "/tmp/example/name",
    },
    {
        "/tmp/example",
        "name.ext1.ext2", // two extensions
        "name.ext1", // remove the first one
        "name.ext1.ext2",
        "/tmp/example",
        "/tmp/example/name.ext1.ext2",
    },
};
// clang-format on

/**
 * Check the various split cases work correctly
 */
BOOST_AUTO_TEST_CASE( Split )
{
    for( const auto& c : split_cases )
    {
        std::stringstream ss;
        ss << c.m_path << ", " << c.m_name;
        BOOST_TEST_CONTEXT( ss.str() )
        {
            // Const: all methods called must be const
            const WX_FILENAME wx_fn( c.m_path, c.m_name );

            BOOST_CHECK_EQUAL( c.m_exp_name, wx_fn.GetName() );
            BOOST_CHECK_EQUAL( c.m_exp_full_name, wx_fn.GetFullName() );
            BOOST_CHECK_EQUAL( c.m_exp_path, wx_fn.GetPath() );
            BOOST_CHECK_EQUAL( c.m_exp_full_path, wx_fn.GetFullPath() );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
