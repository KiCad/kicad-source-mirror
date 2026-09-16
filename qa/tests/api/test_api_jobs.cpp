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

#include <fstream>
#include <set>
#include <sstream>
#include <string>

#include <wx/filename.h>

#include <boost/test/unit_test.hpp>
#include <qa_utils/file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include "api_e2e_utils.h"

#include <api/board/board_jobs.pb.h>
#include <api/board/board_types.pb.h>
#include <api/schematic/schematic_jobs.pb.h>


/**
 * Compare two text files line-by-line, optionally skipping the first @a aSkipLines lines of each
 * file (to ignore timestamps / version headers).  Returns true if files are identical after the
 * skipped prefix, false otherwise.  On mismatch the first differing line pair is logged via
 * BOOST_TEST_MESSAGE.
 */
bool textFilesMatch( const wxString& aGoldenPath, const wxString& aGeneratedPath, int aSkipLines )
{
    std::ifstream goldenStream( aGoldenPath.ToStdString() );
    std::ifstream generatedStream( aGeneratedPath.ToStdString() );

    if( !goldenStream.is_open() )
    {
        BOOST_TEST_MESSAGE( "Cannot open golden file: " + aGoldenPath );
        return false;
    }

    if( !generatedStream.is_open() )
    {
        BOOST_TEST_MESSAGE( "Cannot open generated file: " + aGeneratedPath );
        return false;
    }

    std::string goldenLine, generatedLine;

    for( int i = 0; i < aSkipLines; ++i )
    {
        std::getline( goldenStream, goldenLine );
        std::getline( generatedStream, generatedLine );
    }

    int lineNum = aSkipLines + 1;

    while( std::getline( goldenStream, goldenLine ) )
    {
        if( !std::getline( generatedStream, generatedLine ) )
        {
            BOOST_TEST_MESSAGE( "Generated file is shorter than golden at line " << lineNum );
            return false;
        }

        if( goldenLine != generatedLine )
        {
            BOOST_TEST_MESSAGE( "Mismatch at line " << lineNum << ":\n"
                                                    << "  golden:    " << goldenLine << "\n"
                                                    << "  generated: " << generatedLine );
            return false;
        }

        ++lineNum;
    }

    if( std::getline( generatedStream, generatedLine ) )
    {
        BOOST_TEST_MESSAGE( "Generated file is longer than golden after line " << lineNum );
        return false;
    }

    return true;
}


static size_t countOccurrences( const std::string& aHaystack, const std::string& aNeedle )
{
    size_t count = 0;

    for( size_t pos = aHaystack.find( aNeedle ); pos != std::string::npos;
         pos = aHaystack.find( aNeedle, pos + aNeedle.size() ) )
    {
        ++count;
    }

    return count;
}


/**
 * Return every distinct #RRGGBB literal appearing in @a aSvg.
 */
static std::set<std::string> collectColours( const std::string& aSvg )
{
    std::set<std::string> colours;

    for( size_t pos = aSvg.find( '#' ); pos != std::string::npos; pos = aSvg.find( '#', pos + 1 ) )
    {
        std::string colour = aSvg.substr( pos, 7 );

        if( colour.size() < 7 )
            continue;

        if( colour.find_first_not_of( "0123456789ABCDEFabcdef", 1 ) != std::string::npos )
            continue;

        colours.insert( colour );
    }

    return colours;
}


BOOST_AUTO_TEST_SUITE( ApiJobs )


BOOST_FIXTURE_TEST_CASE( ExportBoardSvg, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    wxString testDataDir =
            wxString::FromUTF8( KI_TEST::GetTestDataRootDir() ) + wxS( "cli/artwork_generation_regressions/" );

    wxFileName boardPath( testDataDir, wxS( "ZoneFill-4.0.7.kicad_pcb" ) );

    kiapi::common::types::DocumentSpecifier document;

    BOOST_REQUIRE_MESSAGE( Client().OpenDocument( boardPath.GetFullPath(), &document ),
                           "OpenDocument failed: " + Client().LastError() );


    KI_TEST::SCOPED_TEMP_DIR tempDir( "api_job_svg" );
    wxFileName               outputPath( tempDir.ChildPathStr( "plot.svg" ) );

    kiapi::board::jobs::RunBoardJobExportSvg request;
    *request.mutable_job_settings()->mutable_document() = document;
    request.mutable_job_settings()->set_output_path( outputPath.GetFullPath().ToUTF8().data() );

    request.mutable_plot_settings()->add_layers( kiapi::board::types::BL_F_Cu );
    request.mutable_plot_settings()->set_black_and_white( true );
    request.mutable_plot_settings()->set_plot_drawing_sheet( false );
    request.mutable_plot_settings()->set_drill_marks( kiapi::board::jobs::PDM_FULL );
    request.set_page_mode( kiapi::board::jobs::BJPM_EACH_LAYER_OWN_FILE );
    request.set_precision( 4 );

    kiapi::common::types::RunJobResponse response;
    BOOST_REQUIRE_MESSAGE( Client().RunJob( request, &response ), "RunJob failed: " + Client().LastError() );

    BOOST_REQUIRE_MESSAGE( response.status() == kiapi::common::types::JS_SUCCESS,
                           "Job failed: " + wxString::FromUTF8( response.message() ) );

    BOOST_REQUIRE_MESSAGE( response.output_path_size() > 0, "Job returned no output paths" );

    wxString generatedPath = wxString::FromUTF8( response.output_path( 0 ) );
    BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( generatedPath ), "Generated SVG does not exist: " + generatedPath );

    // Plot fidelity is covered by the kicad-cli SVG regression tests, which rasterize and compare
    // against golden artwork.  All this test has to establish is that the job honoured the request.
    const std::string svg = KI_TEST::LoadStringData( generatedPath );

    BOOST_REQUIRE_MESSAGE( !svg.empty(), "Generated SVG is empty or unreadable: " + generatedPath );
    BOOST_CHECK( svg.find( "<svg" ) != std::string::npos );
    BOOST_CHECK( svg.find( "</svg>" ) != std::string::npos );

    // The board page is A4, and the four decimals are the requested precision
    BOOST_CHECK( svg.find( "width=\"297.0022mm\" height=\"210.0072mm\"" ) != std::string::npos );

    std::set<std::string> colours = collectColours( svg );
    BOOST_REQUIRE_MESSAGE( !colours.empty(), "Plot declares no colours at all" );

    for( const std::string& colour : colours )
    {
        BOOST_CHECK_MESSAGE( colour == "#000000" || colour == "#FFFFFF",
                             "black_and_white was requested but the plot contains " + colour );
    }

    // F.Cu of this board is dense; a near-empty plot means the layer selection was dropped
    BOOST_CHECK_GT( countOccurrences( svg, "<path" ), 100u );
}


// Reads the pixel dimensions out of the IHDR chunk of a PNG file.
// Returns true on success; on failure aWidth/aHeight are untouched.
static bool pngDimensions( const wxString& aPath, uint32_t& aWidth, uint32_t& aHeight )
{
    std::ifstream stream( aPath.ToStdString(), std::ios::binary );

    if( !stream.is_open() )
        return false;

    char header[8];

    if( !stream.read( header, 8 ) || memcmp( header, "\x89PNG\r\n\x1a\n", 8 ) != 0 )
        return false;

    char ihdr[16];

    if( !stream.read( ihdr, 16 ) )
        return false;

    auto readBE32 = []( const char* aBytes )
    {
        return ( static_cast<uint32_t>( static_cast<unsigned char>( aBytes[0] ) ) << 24 )
             | ( static_cast<uint32_t>( static_cast<unsigned char>( aBytes[1] ) ) << 16 )
             | ( static_cast<uint32_t>( static_cast<unsigned char>( aBytes[2] ) ) << 8 )
             | ( static_cast<uint32_t>( static_cast<unsigned char>( aBytes[3] ) ) );
    };

    aWidth = readBE32( ihdr + 8 );
    aHeight = readBE32( ihdr + 12 );
    return true;
}


BOOST_FIXTURE_TEST_CASE( ExportSchematicPng, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    wxString testDataDir = wxString::FromUTF8( KI_TEST::GetTestDataRootDir() ) + wxS( "cli/basic_test/" );

    wxFileName schPath( testDataDir, wxS( "basic_test.kicad_sch" ) );

    kiapi::common::types::DocumentSpecifier document;

    BOOST_REQUIRE_MESSAGE(
            Client().OpenDocument( schPath.GetFullPath(), kiapi::common::types::DOCTYPE_SCHEMATIC, &document ),
            "OpenDocument failed: " + Client().LastError() );

    // --- All-sheets mode: output path is a directory
    KI_TEST::SCOPED_TEMP_DIR tempDir( "api_job_sch_png" );
    wxString                 outputDir = tempDir.CreateChildDirStr( "all" ) + wxFileName::GetPathSeparator();

    {
        kiapi::schematic::jobs::RunSchematicJobExportPng request;
        *request.mutable_job_settings()->mutable_document() = document;
        request.mutable_job_settings()->set_output_path( outputDir.ToUTF8().data() );
        request.mutable_plot_settings()->set_sheet_mode( kiapi::schematic::jobs::SJSM_ALL_SHEETS );
        request.mutable_plot_settings()->set_black_and_white( true );
        request.set_dpi( 150 );

        kiapi::common::types::RunJobResponse response;
        BOOST_REQUIRE_MESSAGE( Client().RunJob( request, &response ),
                               "RunJob failed: " + Client().LastError() );

        BOOST_REQUIRE_MESSAGE( response.status() == kiapi::common::types::JS_SUCCESS,
                               "Job failed: " + wxString::FromUTF8( response.message() ) );
        BOOST_REQUIRE_MESSAGE( response.output_path_size() > 0, "Job returned no output paths" );

        wxString generatedPath = wxString::FromUTF8( response.output_path( 0 ) );
        BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( generatedPath ),
                               "Generated PNG does not exist: " + generatedPath );

        // basic_test.kicad_sch is A4 landscape; 297mm at 150dpi = 1754px (+/- rounding)
        uint32_t width = 0, height = 0;
        BOOST_REQUIRE_MESSAGE( pngDimensions( generatedPath, width, height ),
                               "Generated file is not a valid PNG: " + generatedPath );
        BOOST_CHECK_MESSAGE( width >= 1750 && width <= 1758,
                             "Unexpected PNG width " << width << " for 150dpi A4 landscape" );
        BOOST_CHECK_MESSAGE( height >= 1236 && height <= 1244,
                             "Unexpected PNG height " << height << " for 150dpi A4 landscape" );
    }

    // --- Single-sheet mode: output path is a file name
    wxFileName singlePath( tempDir.ChildPathStr( "single.png" ) );

    {
        kiapi::schematic::jobs::RunSchematicJobExportPng request;
        *request.mutable_job_settings()->mutable_document() = document;
        request.mutable_job_settings()->set_output_path( singlePath.GetFullPath().ToUTF8().data() );
        request.mutable_plot_settings()->set_sheet_mode( kiapi::schematic::jobs::SJSM_SINGLE_SHEET );
        request.mutable_plot_settings()->set_black_and_white( true );
        request.set_dpi( 300 );

        kiapi::common::types::RunJobResponse response;
        BOOST_REQUIRE_MESSAGE( Client().RunJob( request, &response ),
                               "Single-sheet RunJob failed: " + Client().LastError() );

        BOOST_REQUIRE_MESSAGE( response.status() == kiapi::common::types::JS_SUCCESS,
                               "Single-sheet job failed: " + wxString::FromUTF8( response.message() ) );
        BOOST_REQUIRE_MESSAGE( response.output_path_size() > 0,
                               "Single-sheet job returned no output paths" );

        wxString generatedPath = wxString::FromUTF8( response.output_path( 0 ) );
        BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( generatedPath ),
                               "Generated PNG does not exist: " + generatedPath );

        // 297mm at 300dpi = 3508px (+/- rounding)
        uint32_t width = 0, height = 0;
        BOOST_REQUIRE_MESSAGE( pngDimensions( generatedPath, width, height ),
                               "Generated file is not a valid PNG: " + generatedPath );
        BOOST_CHECK_MESSAGE( width >= 3500 && width <= 3516,
                             "Unexpected PNG width " << width << " for 300dpi A4 landscape" );

        // The single-file output path must be honored exactly
        BOOST_CHECK_MESSAGE( generatedPath == singlePath.GetFullPath(),
                             "Single-sheet output path not honored: " + generatedPath );
    }

    // --- Out-of-range dpi is rejected with a bad-request error
    {
        kiapi::schematic::jobs::RunSchematicJobExportPng request;
        *request.mutable_job_settings()->mutable_document() = document;
        request.mutable_job_settings()->set_output_path( outputDir.ToUTF8().data() );
        request.mutable_plot_settings()->set_sheet_mode( kiapi::schematic::jobs::SJSM_ALL_SHEETS );
        request.set_dpi( 10 );

        kiapi::common::types::RunJobResponse response;
        BOOST_CHECK_MESSAGE( !Client().RunJob( request, &response ),
                             "Out-of-range dpi was not rejected" );
    }

    // --- Omitted dpi falls back to the GUI default (300)
    {
        wxString defaultDir = tempDir.CreateChildDirStr( "default" ) + wxFileName::GetPathSeparator();

        kiapi::schematic::jobs::RunSchematicJobExportPng request;
        *request.mutable_job_settings()->mutable_document() = document;
        request.mutable_job_settings()->set_output_path( defaultDir.ToUTF8().data() );
        request.mutable_plot_settings()->set_sheet_mode( kiapi::schematic::jobs::SJSM_ALL_SHEETS );

        kiapi::common::types::RunJobResponse response;
        BOOST_REQUIRE_MESSAGE( Client().RunJob( request, &response ),
                               "Default-dpi RunJob failed: " + Client().LastError() );
        BOOST_REQUIRE_MESSAGE( response.status() == kiapi::common::types::JS_SUCCESS,
                               "Default-dpi job failed: " + wxString::FromUTF8( response.message() ) );
        BOOST_REQUIRE_MESSAGE( response.output_path_size() > 0,
                               "Default-dpi job returned no output paths" );

        uint32_t width = 0, height = 0;
        wxString generatedPath = wxString::FromUTF8( response.output_path( 0 ) );
        BOOST_REQUIRE_MESSAGE( pngDimensions( generatedPath, width, height ),
                               "Generated file is not a valid PNG: " + generatedPath );
        BOOST_CHECK_MESSAGE( width >= 3500 && width <= 3516,
                             "Default dpi should be 300; got width " << width );
    }
}


BOOST_FIXTURE_TEST_CASE( ExportBoardDrill, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    wxString testDataDir = wxString::FromUTF8( KI_TEST::GetTestDataRootDir() ) + wxS( "cli/basic_test/" );

    wxFileName boardPath( testDataDir, wxS( "basic_test.kicad_pcb" ) );

    kiapi::common::types::DocumentSpecifier document;

    BOOST_REQUIRE_MESSAGE( Client().OpenDocument( boardPath.GetFullPath(), &document ),
                           "OpenDocument failed: " + Client().LastError() );

    KI_TEST::SCOPED_TEMP_DIR tempDir( "api_job_drill" );
    wxString                 outputDir = tempDir.CreateChildDirStr( "drill" ) + wxFileName::GetPathSeparator();

    kiapi::board::jobs::RunBoardJobExportDrill request;
    *request.mutable_job_settings()->mutable_document() = document;
    request.mutable_job_settings()->set_output_path( outputDir.ToUTF8().data() );
    request.set_format( kiapi::board::jobs::DF_EXCELLON );

    kiapi::common::types::RunJobResponse response;
    BOOST_REQUIRE_MESSAGE( Client().RunJob( request, &response ), "RunJob failed: " + Client().LastError() );

    BOOST_REQUIRE_MESSAGE( response.status() == kiapi::common::types::JS_SUCCESS,
                           "Job failed: " + wxString::FromUTF8( response.message() ) );

    BOOST_REQUIRE_MESSAGE( response.output_path_size() > 0, "Job returned no output paths" );
    wxString generatedDrillPath;

    for( int i = 0; i < response.output_path_size(); ++i )
    {
        wxString outputPath = wxString::FromUTF8( response.output_path( i ) );

        if( outputPath.EndsWith( wxS( ".drl" ) ) )
        {
            generatedDrillPath = outputPath;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( !generatedDrillPath.IsEmpty(), "No .drl file found in job output" );
    BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( generatedDrillPath ),
                           "Generated drill file does not exist: " + generatedDrillPath );

    wxString goldenPath = testDataDir + wxS( "basic_test_excellon_inches.drl" );
    BOOST_CHECK_MESSAGE( textFilesMatch( goldenPath, generatedDrillPath, 5 ),
                         "Drill output does not match golden file" );
}


BOOST_FIXTURE_TEST_CASE( ExportBoardPng, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    wxString testDataDir =
            wxString::FromUTF8( KI_TEST::GetTestDataRootDir() ) + wxS( "cli/artwork_generation_regressions/" );

    wxFileName boardPath( testDataDir, wxS( "ZoneFill-4.0.7.kicad_pcb" ) );

    kiapi::common::types::DocumentSpecifier document;

    BOOST_REQUIRE_MESSAGE( Client().OpenDocument( boardPath.GetFullPath(), &document ),
                           "OpenDocument failed: " + Client().LastError() );

    KI_TEST::SCOPED_TEMP_DIR tempDir( "api_job_pcb_png" );

    // --- MULTI mode: one file per layer into a directory
    {
        wxString outputDir = tempDir.CreateChildDirStr( "multi" ) + wxFileName::GetPathSeparator();

        kiapi::board::jobs::RunBoardJobExportPng request;
        *request.mutable_job_settings()->mutable_document() = document;
        request.mutable_job_settings()->set_output_path( outputDir.ToUTF8().data() );
        request.mutable_plot_settings()->add_layers( kiapi::board::types::BL_F_Cu );
        request.mutable_plot_settings()->set_black_and_white( true );
        request.set_page_mode( kiapi::board::jobs::BJPM_EACH_LAYER_OWN_FILE );
        request.set_dpi( 150 );

        kiapi::common::types::RunJobResponse response;
        BOOST_REQUIRE_MESSAGE( Client().RunJob( request, &response ),
                               "RunJob failed: " + Client().LastError() );

        BOOST_REQUIRE_MESSAGE( response.status() == kiapi::common::types::JS_SUCCESS,
                               "Job failed: " + wxString::FromUTF8( response.message() ) );
        BOOST_REQUIRE_MESSAGE( response.output_path_size() > 0, "Job returned no output paths" );

        wxString generatedPath = wxString::FromUTF8( response.output_path( 0 ) );
        BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( generatedPath ),
                               "Generated PNG does not exist: " + generatedPath );

        uint32_t width = 0, height = 0;
        BOOST_REQUIRE_MESSAGE( pngDimensions( generatedPath, width, height ),
                               "Generated file is not a valid PNG: " + generatedPath );
        BOOST_CHECK_MESSAGE( width >= 1750 && width <= 1758,
                             "Unexpected PNG width " << width << " for 150dpi A4 landscape" );
    }

    // --- SINGLE mode: all layers composited into one file
    {
        wxFileName outputPath( tempDir.ChildPathStr( "single.png" ) );

        kiapi::board::jobs::RunBoardJobExportPng request;
        *request.mutable_job_settings()->mutable_document() = document;
        request.mutable_job_settings()->set_output_path( outputPath.GetFullPath().ToUTF8().data() );
        request.mutable_plot_settings()->add_layers( kiapi::board::types::BL_F_Cu );
        request.mutable_plot_settings()->add_layers( kiapi::board::types::BL_F_SilkS );
        request.mutable_plot_settings()->set_black_and_white( true );
        request.set_page_mode( kiapi::board::jobs::BJPM_ALL_LAYERS_ONE_PAGE );
        request.set_dpi( 300 );

        kiapi::common::types::RunJobResponse response;
        BOOST_REQUIRE_MESSAGE( Client().RunJob( request, &response ),
                               "Single-mode RunJob failed: " + Client().LastError() );

        BOOST_REQUIRE_MESSAGE( response.status() == kiapi::common::types::JS_SUCCESS,
                               "Single-mode job failed: " + wxString::FromUTF8( response.message() ) );
        BOOST_REQUIRE_MESSAGE( response.output_path_size() > 0,
                               "Single-mode job returned no output paths" );

        wxString generatedPath = wxString::FromUTF8( response.output_path( 0 ) );
        BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( generatedPath ),
                               "Generated PNG does not exist: " + generatedPath );
        BOOST_CHECK_MESSAGE( generatedPath == outputPath.GetFullPath(),
                             "Single-mode output path not honored: " + generatedPath );

        uint32_t width = 0, height = 0;
        BOOST_REQUIRE_MESSAGE( pngDimensions( generatedPath, width, height ),
                               "Generated file is not a valid PNG: " + generatedPath );
        BOOST_CHECK_MESSAGE( width >= 3500 && width <= 3516,
                             "Unexpected PNG width " << width << " for 300dpi A4 landscape" );
    }
}


BOOST_FIXTURE_TEST_CASE( ExportSchematicNetlist, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    wxString testDataDir = wxString::FromUTF8( KI_TEST::GetTestDataRootDir() ) + wxS( "cli/basic_test/" );

    wxFileName schPath( testDataDir, wxS( "basic_test.kicad_sch" ) );

    kiapi::common::types::DocumentSpecifier document;

    BOOST_REQUIRE_MESSAGE(
            Client().OpenDocument( schPath.GetFullPath(), kiapi::common::types::DOCTYPE_SCHEMATIC, &document ),
            "OpenDocument failed: " + Client().LastError() );

    KI_TEST::SCOPED_TEMP_DIR tempDir( "api_job_netlist" );
    wxFileName               outputPath( tempDir.ChildPathStr( "netlist.cadstar" ) );

    kiapi::schematic::jobs::RunSchematicJobExportNetlist request;
    *request.mutable_job_settings()->mutable_document() = document;
    request.mutable_job_settings()->set_output_path( outputPath.GetFullPath().ToUTF8().data() );
    request.set_format( kiapi::schematic::jobs::SNF_CADSTAR );

    kiapi::common::types::RunJobResponse response;
    BOOST_REQUIRE_MESSAGE( Client().RunJob( request, &response ), "RunJob failed: " + Client().LastError() );

    BOOST_REQUIRE_MESSAGE( response.status() == kiapi::common::types::JS_SUCCESS,
                           "Job failed: " + wxString::FromUTF8( response.message() ) );

    BOOST_REQUIRE_MESSAGE( response.output_path_size() > 0, "Job returned no output paths" );

    wxString generatedPath = wxString::FromUTF8( response.output_path( 0 ) );
    BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( generatedPath ),
                           "Generated netlist does not exist: " + generatedPath );

    // 3 header lines to skip (contain timestamp/version)
    wxString goldenPath = testDataDir + wxS( "basic_test.netlist.cadstar" );
    BOOST_CHECK_MESSAGE( textFilesMatch( goldenPath, generatedPath, 3 ), "Netlist output does not match golden file" );
}


BOOST_FIXTURE_TEST_CASE( ExportSchematicBom, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    wxString testDataDir = wxString::FromUTF8( KI_TEST::GetTestDataRootDir() ) + wxS( "cli/variants/" );

    wxFileName schPath( testDataDir, wxS( "variants.kicad_sch" ) );

    kiapi::common::types::DocumentSpecifier document;

    BOOST_REQUIRE_MESSAGE(
            Client().OpenDocument( schPath.GetFullPath(), kiapi::common::types::DOCTYPE_SCHEMATIC, &document ),
            "OpenDocument failed: " + Client().LastError() );

    KI_TEST::SCOPED_TEMP_DIR tempDir( "api_job_bom" );
    wxFileName               outputPath( tempDir.ChildPathStr( "bom.csv" ) );

    kiapi::schematic::jobs::RunSchematicJobExportBOM request;
    *request.mutable_job_settings()->mutable_document() = document;
    request.mutable_job_settings()->set_output_path( outputPath.GetFullPath().ToUTF8().data() );
    request.set_exclude_dnp( true );

    request.mutable_format()->set_preset_name( "CSV" );

    auto* refField = request.mutable_fields()->add_fields();
    refField->set_name( "Reference" );
    refField->set_label( "Refs" );
    refField->set_group_by( false );

    auto* valField = request.mutable_fields()->add_fields();
    valField->set_name( "Value" );
    valField->set_label( "Value" );
    valField->set_group_by( false );

    kiapi::common::types::RunJobResponse response;
    BOOST_REQUIRE_MESSAGE( Client().RunJob( request, &response ), "RunJob failed: " + Client().LastError() );

    BOOST_REQUIRE_MESSAGE( response.status() == kiapi::common::types::JS_SUCCESS,
                           "Job failed: " + wxString::FromUTF8( response.message() ) );

    BOOST_REQUIRE_MESSAGE( response.output_path_size() > 0, "Job returned no output paths" );

    wxString generatedPath = wxString::FromUTF8( response.output_path( 0 ) );
    BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( generatedPath ), "Generated BOM does not exist: " + generatedPath );

    wxString goldenPath = testDataDir + wxS( "variants_default.bom.csv" );
    BOOST_CHECK_MESSAGE( textFilesMatch( goldenPath, generatedPath, 0 ), "BOM output does not match golden file" );

    request.mutable_format()->clear_preset_name();
    request.mutable_format()->set_field_delimiter( "," );
    request.mutable_format()->set_string_delimiter( "\"" );
    request.mutable_format()->set_ref_delimiter( "," );
    request.mutable_format()->set_ref_range_delimiter( "" );
    request.mutable_format()->set_include_byte_order_mark( true );
    response.Clear();

    BOOST_REQUIRE_MESSAGE( Client().RunJob( request, &response ),
                           "BOM job with byte order mark failed: " + Client().LastError() );

    BOOST_REQUIRE_MESSAGE( response.status() == kiapi::common::types::JS_SUCCESS,
                           "BOM job with byte order mark failed: " + wxString::FromUTF8( response.message() ) );
    BOOST_REQUIRE_MESSAGE( response.output_path_size() > 0, "BOM job with byte order mark returned no output paths" );

    generatedPath = wxString::FromUTF8( response.output_path( 0 ) );
    std::ifstream generatedStream( generatedPath.ToStdString(), std::ios::binary );
    std::ifstream goldenStream( goldenPath.ToStdString(), std::ios::binary );
    BOOST_REQUIRE( generatedStream.is_open() );
    BOOST_REQUIRE( goldenStream.is_open() );

    std::ostringstream generatedContents;
    std::ostringstream goldenContents;
    generatedContents << generatedStream.rdbuf();
    goldenContents << goldenStream.rdbuf();

    const std::string utf8ByteOrderMark( "\xEF\xBB\xBF", 3 );
    const std::string bom = generatedContents.str();
    BOOST_REQUIRE_GE( bom.size(), utf8ByteOrderMark.size() );
    BOOST_CHECK_EQUAL( bom.substr( 0, utf8ByteOrderMark.size() ), utf8ByteOrderMark );
    BOOST_CHECK_EQUAL( bom.substr( utf8ByteOrderMark.size() ), goldenContents.str() );

    generatedStream.close();
    goldenStream.close();
}


BOOST_AUTO_TEST_SUITE_END()
