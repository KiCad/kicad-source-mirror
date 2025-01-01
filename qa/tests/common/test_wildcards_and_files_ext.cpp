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

#include <wildcards_and_files_ext.h>


BOOST_AUTO_TEST_SUITE( WildcardFileExt )


/**
 * Data used to construct a simple test of one or more extensions
 * and get a filter string for WX dialogs out
 */
struct ExtWildcardFilterCase
{
    /// The list of exts handled
    std::vector<std::string> m_exts;

    /// Filter for case-insensitive environments
    std::string m_filter_case_insenstive;

    /// Filter for regex-capable environments (case insensitive filter in a
    /// case-sensitive environment)
    std::string m_re_filter;
};

const static std::vector<ExtWildcardFilterCase> ext_wildcard_cases = {
    {
            { "png" },
            " (*.png)|*.png",
            " (*.png)|*.[pP][nN][gG]",
    },
    {
            { "png", "gif" },
            " (*.png; *.gif)|*.png;*.gif",
            " (*.png; *.gif)|*.[pP][nN][gG];*.[gG][iI][fF]",
    },
};


static constexpr bool should_use_regex_filters()
{
#ifdef __WXGTK__
    return true;
#else
    return false;
#endif
}


// Structure used to store the extensions to test and the expected comparison result
struct testExtensions
{
    std::string ext;
    bool        insense_result;
    bool        sense_result;
};

const static std::vector<std::string>    extensionList = { "dxf", "svg", "SCH", "^g.*" };
const static std::vector<testExtensions> testExtensionList = {
    {
            "dxf",
            true, // Case insensitive comparison result
            true  // Case sensitive comparison result
    },
    {
            "sch",
            true, // Case insensitive comparison result
            false // Case sensitive comparison result
    },
    {
            "gbr",
            true, // Case insensitive comparison result
            true  // Case sensitive comparison result
    },
    {
            "pcb",
            false, // Case insensitive comparison result
            false  // Case sensitive comparison result
    }
};

/**
 * Check correct comparison of file names
 */
BOOST_AUTO_TEST_CASE( FileNameComparison )
{
    for( const auto& testExt : testExtensionList )
    {
        bool extPresent_insense = compareFileExtensions( testExt.ext, extensionList, false );
        bool extPresent_sense = compareFileExtensions( testExt.ext, extensionList, true );

        BOOST_TEST_INFO( "Case insensitive test for extension " + testExt.ext );
        BOOST_CHECK_EQUAL( extPresent_insense, testExt.insense_result );

        BOOST_TEST_INFO( "Case sensitive test for extension " + testExt.ext );
        BOOST_CHECK_EQUAL( extPresent_sense, testExt.sense_result );
    }
}


/**
 * Check correct handling of filter strings (as used by WX)
 */
BOOST_AUTO_TEST_CASE( BasicFilter )
{
    for( const auto& c : ext_wildcard_cases )
    {
        const std::string exp_filter =
                should_use_regex_filters() ? c.m_re_filter : c.m_filter_case_insenstive;

        const auto resp = AddFileExtListToFilter( c.m_exts );

        BOOST_CHECK_EQUAL( resp, exp_filter );
    }
}

static constexpr bool should_use_windows_filters()
{
#ifdef __WXMSW__
    return true;
#else
    return false;
#endif
}

BOOST_AUTO_TEST_CASE( AllFilesFilter )
{
    const auto resp = AddFileExtListToFilter( {} );

    const std::string exp_filter = should_use_windows_filters() ? " (*.*)|*.*" : " (*)|*";

    BOOST_CHECK_EQUAL( resp, exp_filter );
}

BOOST_AUTO_TEST_SUITE_END()
