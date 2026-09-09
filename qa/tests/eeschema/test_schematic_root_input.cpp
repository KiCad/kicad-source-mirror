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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <qa_utils/file_utils.h>
#include <eeschema_helpers.h>
#include <locale_io.h>
#include <project.h>
#include <project/project_file.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <settings/settings_manager.h>
#include <wx/ffile.h>
#include <reporter.h>


namespace
{
size_t replaceFileText( const wxString& aPath, const wxString& aOld, const wxString& aNew )
{
    wxString contents;

    {
        wxFFile input( aPath, "rb" );
        BOOST_REQUIRE( input.IsOpened() && input.ReadAll( &contents ) );
    }

    const size_t count = contents.Replace( aOld, aNew );
    wxFFile output( aPath, "wb" );
    BOOST_REQUIRE( output.IsOpened() && output.Write( contents ) );
    return count;
}
}


BOOST_AUTO_TEST_CASE( HeadlessLoaderRequiresRootProvenanceOnlyWhenRequested )
{
    LOCALE_IO locale;
    WX_STRING_REPORTER reporter;
    KI_TEST::SCOPED_TEMP_DIR directory( "sch-root-input" );
    const wxString source = KI_TEST::GetEeschemaTestDataDir() + "issue23840/";

    for( const wxString& filename : { wxString( "BusAndVectors.kicad_pro" ),
                                     wxString( "BusAndVectors.kicad_sch" ), wxString( "LEDs.kicad_sch" ) } )
    {
        BOOST_REQUIRE( wxCopyFile( source + filename, directory.PathStr() + "/" + filename ) );
    }

    SETTINGS_MANAGER settings;
    BOOST_REQUIRE( settings.LoadProject( directory.PathStr() + "/BusAndVectors.kicad_pro" ) );
    const wxString rootFile = directory.PathStr() + "/BusAndVectors.kicad_sch";
    const wxString childFile = directory.PathStr() + "/LEDs.kicad_sch";
    std::unique_ptr<SCHEMATIC> root( EESCHEMA_HELPERS::LoadSchematic(
            rootFile, false, false, &settings.Prj(), false, &reporter ) );
    BOOST_REQUIRE( root );
    BOOST_CHECK_EQUAL( root->Hierarchy().size(), 3 );
    root.reset();

    {
        reporter.Clear();
        std::unique_ptr<SCHEMATIC> child( EESCHEMA_HELPERS::LoadSchematic(
                childFile, false, false, &settings.Prj(), false, &reporter ) );
        BOOST_CHECK( !child );
        BOOST_CHECK( reporter.GetMessages().Contains( "hierarchical subsheet" ) );
    }

    std::unique_ptr<SCHEMATIC> child( EESCHEMA_HELPERS::LoadSchematic(
            childFile, false, false, &settings.Prj(), false ) );
    BOOST_REQUIRE( child );
    BOOST_CHECK_EQUAL( child->Hierarchy().size(), 1 );
    const KIID childId = child->GetTopLevelSheet( 0 )->m_Uuid;
    child.reset();

    // Declaring a former child as a top-level sheet makes its new role explicit.
    settings.Prj().GetProjectFile().GetTopLevelSheets() = {
        TOP_LEVEL_SHEET_INFO( childId, wxS( "LEDs" ), wxS( "LEDs.kicad_sch" ) )
    };
    child.reset( EESCHEMA_HELPERS::LoadSchematic(
            childFile, false, false, &settings.Prj(), false, &reporter ) );
    BOOST_REQUIRE( child );
    BOOST_CHECK_EQUAL( child->Hierarchy().size(), 1 );
    child.reset();
    settings.Prj().GetProjectFile().GetTopLevelSheets().clear();

    // A shared file can retain parent placements while also being used as a root.
    BOOST_REQUIRE_GT( replaceFileText( childFile,
            wxS( "/4394464a-8613-4d36-8dd8-51c322850652/0ed71495-8674-4a6e-a1f9-3d74c1903959" ),
            wxS( "/42beb834-d08e-4ccf-b2ec-2a6c2b0cfb9c" ) ), 0 );

    child.reset( EESCHEMA_HELPERS::LoadSchematic(
            childFile, false, false, &settings.Prj(), false, &reporter ) );
    BOOST_REQUIRE( child );
    BOOST_CHECK_EQUAL( child->Hierarchy().size(), 1 );
}


BOOST_AUTO_TEST_CASE( HeadlessLoaderReportsMalformedModernRootInstance )
{
    LOCALE_IO locale;
    WX_STRING_REPORTER reporter;
    KI_TEST::SCOPED_TEMP_DIR directory( "sch-malformed-root" );
    const wxString source = KI_TEST::GetEeschemaTestDataDir() + "issue23840/";

    for( const wxString& filename : { wxString( "BusAndVectors.kicad_pro" ),
                                     wxString( "BusAndVectors.kicad_sch" ), wxString( "LEDs.kicad_sch" ) } )
    {
        BOOST_REQUIRE( wxCopyFile( source + filename, directory.PathStr() + "/" + filename ) );
    }

    const wxString rootFile = directory.PathStr() + "/BusAndVectors.kicad_sch";
    BOOST_REQUIRE_EQUAL( replaceFileText( rootFile,
            wxS( "(path \"/\"" ), wxS( "(path \"/4394464a-8613-4d36-8dd8-51c322850652\"" ) ), 1 );

    SETTINGS_MANAGER settings;
    BOOST_REQUIRE( settings.LoadProject( directory.PathStr() + "/BusAndVectors.kicad_pro" ) );

    {
        reporter.Clear();
        std::unique_ptr<SCHEMATIC> schematic( EESCHEMA_HELPERS::LoadSchematic(
                rootFile, false, false, &settings.Prj(), false, &reporter ) );
        BOOST_CHECK( !schematic );
        BOOST_CHECK( reporter.GetMessages().Contains( "malformed root sheet instance path" ) );
    }

    std::unique_ptr<SCHEMATIC> schematic( EESCHEMA_HELPERS::LoadSchematic(
            rootFile, false, false, &settings.Prj(), false ) );
    BOOST_REQUIRE( schematic );
    BOOST_CHECK_EQUAL( schematic->Hierarchy().size(), 3 );
}


BOOST_AUTO_TEST_CASE( HeadlessRootValidationPreservesFlatAndOlderNativeInputs )
{
    LOCALE_IO locale;
    WX_STRING_REPORTER reporter;
    KI_TEST::SCOPED_TEMP_DIR directory( "sch-root-compatibility" );
    const wxString source = KI_TEST::GetEeschemaTestDataDir();

    for( const wxString& filename : { wxString( "SVG-Test.kicad_pro" ), wxString( "SVG-Test.kicad_sch" ),
                                     wxString( "toplevel2.kicad_sch" ), wxString( "toplevel3.kicad_sch" ) } )
    {
        BOOST_REQUIRE( wxCopyFile( source + "issue25198/" + filename,
                                  directory.PathStr() + "/" + filename ) );
    }

    SETTINGS_MANAGER settings;
    BOOST_REQUIRE( settings.LoadProject( directory.PathStr() + "/SVG-Test.kicad_pro" ) );
    std::unique_ptr<SCHEMATIC> flat( EESCHEMA_HELPERS::LoadSchematic(
            directory.PathStr() + "/SVG-Test.kicad_sch", false, false, &settings.Prj(), false, &reporter ) );
    BOOST_REQUIRE( flat );
    BOOST_CHECK_EQUAL( flat->GetTopLevelSheets().size(), 3 );
    BOOST_CHECK_EQUAL( flat->Hierarchy().size(), 3 );
    flat.reset();

    BOOST_REQUIRE_EQUAL( replaceFileText( directory.PathStr() + "/toplevel2.kicad_sch",
            wxS( "(path \"/\"" ), wxS( "(path \"/4394464a-8613-4d36-8dd8-51c322850652\"" ) ), 1 );

    {
        reporter.Clear();
        flat.reset( EESCHEMA_HELPERS::LoadSchematic(
                directory.PathStr() + "/SVG-Test.kicad_sch", false, false, &settings.Prj(), false, &reporter ) );
        BOOST_CHECK( !flat );
        BOOST_CHECK( reporter.GetMessages().Contains( "malformed root sheet instance path" ) );
    }

    const wxString oldFile = directory.PathStr() + "/NoConnectPinsConnectedByLine.kicad_sch";
    BOOST_REQUIRE( wxCopyFile( source + "NoConnectPinsConnectedByLine.kicad_sch", oldFile ) );
    std::unique_ptr<SCHEMATIC> old( EESCHEMA_HELPERS::LoadSchematic(
            oldFile, false, false, &settings.Prj(), false, &reporter ) );
    BOOST_REQUIRE( old );
    BOOST_CHECK_LT( old->RootScreen()->GetFileFormatVersionAtLoad(), 20221110 );
    BOOST_CHECK_EQUAL( old->Hierarchy().size(), 1 );
}
