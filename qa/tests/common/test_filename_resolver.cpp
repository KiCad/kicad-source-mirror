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
 * Tests for FILENAME_RESOLVER and SEARCH_STACK.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <filename_resolver.h>
#include <search_stack.h>

#include <wx/filename.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

BOOST_AUTO_TEST_SUITE( FilenameResolver )

BOOST_AUTO_TEST_CASE( ResolveAbsoluteAndWorkingPath )
{
    fs::path temp = fs::temp_directory_path() / "fnres_test";
    fs::create_directories( temp );

    fs::path work = temp / "work";
    fs::create_directories( work );

    fs::path file = work / "model.txt";
    std::ofstream( file.string() ) << "dummy";

    FILENAME_RESOLVER resolver;

    wxString   absFile = wxString::FromUTF8( file.string() );
    wxString   resolved = resolver.ResolvePath( absFile, wxEmptyString, {} );
    wxFileName fn( absFile );
    fn.Normalize( wxPATH_NORM_ABSOLUTE | wxPATH_NORM_DOTS );
    BOOST_CHECK_EQUAL( resolved, fn.GetFullPath() );

    wxString working = wxString::FromUTF8( work.string() );
    wxString relResolved = resolver.ResolvePath( wxS( "model.txt" ), working, {} );
    BOOST_CHECK_EQUAL( relResolved, fn.GetFullPath() );
}

BOOST_AUTO_TEST_CASE( ResolveAliasAndErrors )
{
    fs::path temp = fs::temp_directory_path() / "fnres_alias";
    fs::create_directories( temp );

    fs::path file = temp / "a.txt";
    std::ofstream( file.string() ) << "dummy";

    FILENAME_RESOLVER resolver;

    SEARCH_PATH sp;
    sp.m_Alias = wxS( "ALIAS" );
    sp.m_Pathvar = wxString::FromUTF8( temp.string() );
    std::vector<SEARCH_PATH> paths = { sp };
    resolver.UpdatePathList( paths );

    wxString resolved = resolver.ResolvePath( wxS( "ALIAS:a.txt" ), wxEmptyString, {} );
    BOOST_CHECK_EQUAL( resolved, wxString::FromUTF8( file.string() ) );

    wxString missing = resolver.ResolvePath( wxS( "MISSING:a.txt" ), wxEmptyString, {} );
    BOOST_CHECK( missing.IsEmpty() );
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( SearchStack )

BOOST_AUTO_TEST_CASE( RelativePathResolution )
{
    fs::path root = fs::temp_directory_path() / "ss_root";
    fs::path sub = root / "sub";
    fs::create_directories( sub );

    fs::path file = sub / "f.txt";
    std::ofstream( file.string() ) << "dummy";

    SEARCH_STACK stack;
    stack.AddPaths( wxString::FromUTF8( root.string() ) );

    wxString rel = stack.FilenameWithRelativePathInSearchList( wxString::FromUTF8( file.string() ),
                                                               wxString::FromUTF8( root.string() ) );
    BOOST_CHECK_EQUAL( rel, wxString::FromUTF8( ( fs::path( "sub" ) / "f.txt" ).string() ) );

    fs::path outside = fs::temp_directory_path() / "outside.txt";
    std::ofstream( outside.string() ) << "dummy";

    wxString full = stack.FilenameWithRelativePathInSearchList( wxString::FromUTF8( outside.string() ),
                                                                wxString::FromUTF8( root.string() ) );
    BOOST_CHECK_EQUAL( full, wxString::FromUTF8( outside.string() ) );
}

BOOST_AUTO_TEST_SUITE_END()
