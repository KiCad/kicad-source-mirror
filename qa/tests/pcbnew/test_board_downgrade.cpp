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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>

#include <wx/ffile.h>
#include <wx/filename.h>

#include <downgrade_scan.h>
#include <pcbnew_utils/board_test_utils.h>


BOOST_AUTO_TEST_SUITE( BoardDowngrade )


// The staged library downgrade converts and verifies every file before any original is
// replaced, and a refusal leaves the folder untouched with no temp files behind.
BOOST_AUTO_TEST_CASE( LibraryFilesDowngradeFailsClosed )
{
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_lib_staged", "" );
    const wxString               dir = tmp.GetPath().wstring();

    auto writeFile = [&]( const wxString& aName, const wxString& aContent )
    {
        wxFFile file( dir + wxFileName::GetPathSeparator() + aName, wxT( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        file.Write( aContent );
    };

    writeFile( wxT( "a.fake_mod" ), wxT( "old a" ) );
    writeFile( wxT( "b.fake_mod" ), wxT( "old b" ) );

    auto noForbidden = []( const wxString& )
    {
        return wxString();
    };

    auto convertOk = [&]( const wxString& aFile, const wxString& aTmp ) -> DOWNGRADE_FILE_RESULT
    {
        wxFFile out( aTmp, wxT( "wb" ) );
        out.Write( wxT( "(version 123) converted" ) );
        return DOWNGRADE_FILE_RESULT::CONVERTED;
    };

    BOOST_CHECK( DowngradeLibraryFilesInPlace( dir, wxT( "*.fake_mod" ), 123, noForbidden, convertOk ).IsEmpty() );

    wxFFile  check( dir + wxFileName::GetPathSeparator() + wxT( "a.fake_mod" ), wxT( "rb" ) );
    wxString content;
    BOOST_REQUIRE( check.IsOpened() && check.ReadAll( &content ) );
    BOOST_CHECK( content.Contains( wxT( "converted" ) ) );

    // Second pass: the second file refuses, so the first must stay as it is now.
    auto convertRefuseB = [&]( const wxString& aFile, const wxString& aTmp ) -> DOWNGRADE_FILE_RESULT
    {
        if( aFile.Contains( wxT( "b.fake_mod" ) ) )
            return DOWNGRADE_FILE_RESULT::REFUSED;

        wxFFile out( aTmp, wxT( "wb" ) );
        out.Write( wxT( "(version 123) second pass" ) );
        return DOWNGRADE_FILE_RESULT::CONVERTED;
    };

    wxString bad = DowngradeLibraryFilesInPlace( dir, wxT( "*.fake_mod" ), 123, noForbidden, convertRefuseB );
    BOOST_CHECK( bad.Contains( wxT( "b.fake_mod" ) ) );

    wxArrayString leftovers;
    wxDir::GetAllFiles( dir, &leftovers );
    BOOST_CHECK_EQUAL( leftovers.size(), 2 );

    wxFFile  recheck( dir + wxFileName::GetPathSeparator() + wxT( "a.fake_mod" ), wxT( "rb" ) );
    wxString unchanged;
    BOOST_REQUIRE( recheck.IsOpened() && recheck.ReadAll( &unchanged ) );
    BOOST_CHECK( !unchanged.Contains( wxT( "second pass" ) ) );
}


// The project export carries design files only. UI state and non-KiCad files stay behind.
BOOST_AUTO_TEST_CASE( DesignFileFilter )
{
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/proj.kicad_pro" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/sub/sheet.kicad_sch" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/proj.kicad_pcb" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/proj.kicad_dru" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/frame.kicad_wks" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/lib.kicad_sym" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/lib.pretty/fp.kicad_mod" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/sym-lib-table" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/fp-lib-table" ) ) );

    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/proj.kicad_prl" ) ) );
    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/datasheet.pdf" ) ) );
    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/out/proj-F_Cu.gbr" ) ) );
    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/proj.kicad_jobset" ) ) );
    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/notes.txt" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
