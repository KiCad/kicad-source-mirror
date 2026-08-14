/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

// Regression tests for
// https://gitlab.com/kicad/code/kicad/-/issues/25198
// https://gitlab.com/kicad/code/kicad/-/issues/25203
// https://gitlab.com/kicad/code/kicad/-/issues/25231

#include <qa_utils/pdf_test_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <eeschema_helpers.h>
#include <locale_io.h>
#include <sch_plotter.h>
#include <sch_render_settings.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>

#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>


namespace
{

// The issue 25231 reproduction project has three flat top-level sheets in schematic.top_level_sheets
struct MULTI_TOP_LEVEL_PLOT_FIXTURE
{
    MULTI_TOP_LEVEL_PLOT_FIXTURE()
    {
        wxString schPath = wxString::FromUTF8( KI_TEST::GetEeschemaTestDataDir() )
                           + wxS( "issue25198/SVG-Test.kicad_sch" );

        m_schematic.reset( EESCHEMA_HELPERS::LoadSchematic( schPath, true, false ) );

        wxFileName outDir( wxFileName::CreateTempFileName(
                wxStandardPaths::Get().GetTempDir() + wxFileName::GetPathSeparator()
                + wxS( "issue25198" ) ) );

        wxRemoveFile( outDir.GetFullPath() );
        m_outputDir = outDir.GetFullPath();
        wxFileName::Mkdir( m_outputDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
    }

    ~MULTI_TOP_LEVEL_PLOT_FIXTURE()
    {
        if( !m_outputDir.IsEmpty() )
            wxFileName::Rmdir( m_outputDir, wxPATH_RMDIR_RECURSIVE );
    }

    // Mimics the hierarchy navigator, which sets only the screen filename and leaves SHEET_FILENAME empty
    void AddTopLevelSheet( const wxString& aName, const wxString& aFileName )
    {
        SCH_SHEET*  sheet = new SCH_SHEET( m_schematic.get() );
        SCH_SCREEN* screen = new SCH_SCREEN( m_schematic.get() );

        sheet->SetScreen( screen );
        sheet->GetField( FIELD_T::SHEET_NAME )->SetText( aName );
        screen->SetFileName( aFileName );

        m_schematic->AddTopLevelSheet( sheet );
    }

    std::vector<wxString> PlotAll( PLOT_FORMAT aFormat )
    {
        SCH_RENDER_SETTINGS renderSettings;

        SCH_PLOT_OPTS opts;
        opts.m_plotAll = true;
        opts.m_plotDrawingSheet = true;
        opts.m_outputDirectory = m_outputDir;

        SCH_PLOTTER plotter( m_schematic.get() );
        plotter.Plot( aFormat, opts, &renderSettings, nullptr );

        return plotter.GetOutputFilePaths();
    }

    std::unique_ptr<SCHEMATIC> m_schematic;
    wxString                   m_outputDir;
};


std::string ReadFile( const wxString& aPath )
{
    std::vector<uint8_t> bytes = KI_TEST::LoadBinaryData( aPath.ToStdString() );
    return std::string( bytes.begin(), bytes.end() );
}


int CountPdfPages( const wxString& aPath )
{
    std::string contents;

    if( !ReadPdfWithDecompressedStreams( aPath, contents ) )
        return 0;

    // The plotter writes the page tree node as "/Type /Pages", so the newline keeps them apart
    return CountOccurrences( contents, "/Type /Page\n" );
}

} // namespace


BOOST_FIXTURE_TEST_SUITE( Issue25198MultiTopLevelPlot, MULTI_TOP_LEVEL_PLOT_FIXTURE )


// kicad-cli used to load only the named file, dropping every sibling top-level sheet
BOOST_AUTO_TEST_CASE( LoadsEveryTopLevelSheet )
{
    LOCALE_IO dummy;

    BOOST_REQUIRE( m_schematic != nullptr );
    BOOST_REQUIRE_EQUAL( m_schematic->GetTopLevelSheets().size(), 3u );
    BOOST_CHECK_EQUAL( m_schematic->Hierarchy().size(), 3u );

    std::set<wxString> names;

    for( const SCH_SHEET* sheet : m_schematic->GetTopLevelSheets() )
        names.insert( sheet->GetName() );

    BOOST_CHECK_EQUAL( names.count( wxS( "Toplevel1" ) ), 1u );
    BOOST_CHECK_EQUAL( names.count( wxS( "Toplevel2" ) ), 1u );
    BOOST_CHECK_EQUAL( names.count( wxS( "Toplevel3" ) ), 1u );
}


BOOST_AUTO_TEST_CASE( PlotsEveryTopLevelSheetToPdf )
{
    LOCALE_IO dummy;

    BOOST_REQUIRE( m_schematic != nullptr );

    std::vector<wxString> outputs = PlotAll( PLOT_FORMAT::PDF );

    BOOST_REQUIRE_EQUAL( outputs.size(), 1u );
    BOOST_REQUIRE( wxFileName::FileExists( outputs[0] ) );
    BOOST_CHECK_EQUAL( CountPdfPages( outputs[0] ), 3 );
}


// SVG, DXF and Postscript write one file per page, so each top-level sheet needs a unique name
BOOST_AUTO_TEST_CASE( PlotsEveryTopLevelSheetToItsOwnSvg )
{
    LOCALE_IO dummy;

    BOOST_REQUIRE( m_schematic != nullptr );

    std::vector<wxString> outputs = PlotAll( PLOT_FORMAT::SVG );

    BOOST_REQUIRE_EQUAL( outputs.size(), 3u );

    std::set<wxString> unique( outputs.begin(), outputs.end() );
    BOOST_REQUIRE_EQUAL( unique.size(), 3u );

    std::set<wxString> baseNames;

    for( const wxString& path : outputs )
    {
        BOOST_REQUIRE( wxFileName::FileExists( path ) );

        // An empty file is not a plot
        std::string contents = ReadFile( path );
        BOOST_CHECK_MESSAGE( contents.find( "<svg" ) != std::string::npos,
                             path.ToStdString() + " is not an SVG" );
        BOOST_CHECK_MESSAGE( contents.find( "</svg>" ) != std::string::npos,
                             path.ToStdString() + " is truncated" );

        baseNames.insert( wxFileName( path ).GetName() );
    }

    BOOST_CHECK_EQUAL( baseNames.count( wxS( "SVG-Test" ) ), 1u );
    BOOST_CHECK_EQUAL( baseNames.count( wxS( "toplevel2" ) ), 1u );
    BOOST_CHECK_EQUAL( baseNames.count( wxS( "toplevel3" ) ), 1u );
}


// Screen-only sheets used to fall back to the same default plot name and overwrite each other
BOOST_AUTO_TEST_CASE( PlotsNewTopLevelSheetsToDistinctFiles )
{
    LOCALE_IO dummy;

    BOOST_REQUIRE( m_schematic != nullptr );

    AddTopLevelSheet( wxS( "Toplevel4" ), wxS( "toplevel4.kicad_sch" ) );
    AddTopLevelSheet( wxS( "Toplevel5" ), wxS( "toplevel5.kicad_sch" ) );

    BOOST_REQUIRE_EQUAL( m_schematic->GetTopLevelSheets().size(), 5u );

    std::vector<wxString> outputs = PlotAll( PLOT_FORMAT::SVG );

    BOOST_REQUIRE_EQUAL( outputs.size(), 5u );

    std::set<wxString> unique( outputs.begin(), outputs.end() );
    BOOST_CHECK_EQUAL( unique.size(), 5u );

    std::set<wxString> baseNames;

    for( const wxString& path : outputs )
    {
        BOOST_CHECK_MESSAGE( wxFileName::FileExists( path ),
                             path.ToStdString() + " was not written" );
        baseNames.insert( wxFileName( path ).GetName() );
    }

    BOOST_CHECK_EQUAL( baseNames.count( wxS( "toplevel4" ) ), 1u );
    BOOST_CHECK_EQUAL( baseNames.count( wxS( "toplevel5" ) ), 1u );
}


BOOST_AUTO_TEST_SUITE_END()
