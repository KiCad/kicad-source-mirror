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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file
 * Test suite for WX_FILNAME
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <wx_filename.h>

#include <wx/arrstr.h>

#include <string>
#include <utility>
#include <vector>

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


// Directory traversal guards for untrusted archive entry names
static const std::vector<wxString> hostile_entry_names = {
    wxT( "../evil" ),
    wxT( "a/../../evil" ),
    wxT( "plugins/../../../../tmp/PCM_PWNED" ), // issue 25227, PCM package install
    wxT( "../../../../tmp/PWNED" ),             // issue 25175, project unarchive
    wxT( "a/b/../../../evil" ),
    wxT( ".." ),
    wxT( "/etc/passwd" ),
    wxT( "//etc/passwd" ),
    wxT( "C:/evil" ),
    wxT( "C:evil" ),
    wxT( "..\\evil" ),
    wxT( "plugins\\..\\..\\..\\tmp\\PCM_PWNED" ),
    wxT( "\\\\server\\share\\evil" ),
    wxT( "" ),
};


// ".." and "." are only special as whole components, so "a..b" and "...leading" are legal.
static const std::vector<std::pair<wxString, wxString>> benign_entry_names = {
    { wxT( "board.kicad_pcb" ), wxT( "board.kicad_pcb" ) },
    { wxT( "./board.kicad_pcb" ), wxT( "board.kicad_pcb" ) },
    { wxT( "sub/board.kicad_pcb" ), wxT( "sub/board.kicad_pcb" ) },
    { wxT( "sub//board.kicad_pcb" ), wxT( "sub/board.kicad_pcb" ) },
    { wxT( "sub/./deep/board.kicad_pcb" ), wxT( "sub/deep/board.kicad_pcb" ) },
    { wxT( "a..b/c.kicad_sch" ), wxT( "a..b/c.kicad_sch" ) },
    { wxT( "...leading/x.txt" ), wxT( "...leading/x.txt" ) },
};


BOOST_AUTO_TEST_CASE( SplitArchiveEntryName_RejectsTraversal )
{
    for( const wxString& name : hostile_entry_names )
    {
        BOOST_TEST_CONTEXT( name )
        {
            wxArrayString parts;

            BOOST_CHECK( !WX_FILENAME::SplitArchiveEntryName( name, parts ) );
        }
    }
}


BOOST_AUTO_TEST_CASE( SplitArchiveEntryName_AcceptsRelativeNames )
{
    for( const auto& [name, expected] : benign_entry_names )
    {
        BOOST_TEST_CONTEXT( name )
        {
            wxArrayString parts;

            BOOST_REQUIRE( WX_FILENAME::SplitArchiveEntryName( name, parts ) );
            BOOST_CHECK_EQUAL( wxJoin( parts, '/', (wxChar) 0 ), expected );
        }
    }
}


BOOST_AUTO_TEST_CASE( ResolveArchiveEntryPath_RejectsTraversal )
{
    const wxString dest = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + wxT( "kicad-unarchive-dest" );

    for( const wxString& name : hostile_entry_names )
    {
        BOOST_TEST_CONTEXT( name )
        {
            wxFileName resolved;

            BOOST_CHECK( !WX_FILENAME::ResolveArchiveEntryPath( dest, name, resolved ) );
        }
    }
}


BOOST_AUTO_TEST_CASE( ResolveArchiveEntryPath_StaysBelowDestination )
{
    const wxString dest = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + wxT( "kicad-unarchive-dest" );
    const wxString destWithSep = wxFileName::DirName( dest ).GetPathWithSep();

    for( const auto& [name, expected] : benign_entry_names )
    {
        BOOST_TEST_CONTEXT( name )
        {
            wxFileName resolved;

            BOOST_REQUIRE( WX_FILENAME::ResolveArchiveEntryPath( dest, name, resolved ) );
            BOOST_CHECK( resolved.GetFullPath().StartsWith( destWithSep ) );

            wxFileName relative = resolved;
            relative.MakeRelativeTo( dest );
            BOOST_CHECK_EQUAL( relative.GetFullPath( wxPATH_UNIX ), expected );
        }
    }

    // A sibling directory that merely shares a prefix with the destination is still outside.
    wxFileName resolved;
    BOOST_REQUIRE( WX_FILENAME::ResolveArchiveEntryPath( dest, wxT( "x" ), resolved ) );
    BOOST_CHECK( !WX_FILENAME::ResolveArchiveEntryPath( dest, wxT( "../kicad-unarchive-dest-evil/x" ), resolved ) );
}


BOOST_AUTO_TEST_SUITE_END()
