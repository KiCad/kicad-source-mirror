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
#include <sstream>

#include <boost/test/unit_test.hpp>
#include <wx/dir.h>
#include <wx/filename.h>

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

    wxFileName outputPath = wxFileName::CreateTempFileName( wxS( "api_job_svg_" ) );
    outputPath.SetExt( wxS( "svg" ) );

    kiapi::board::jobs::RunBoardJobExportSvg request;
    *request.mutable_job_settings()->mutable_document() = document;
    request.mutable_job_settings()->set_output_path( outputPath.GetFullPath().ToUTF8().data() );

    request.mutable_plot_settings()->add_layers( kiapi::board::types::BL_F_Cu );
    request.mutable_plot_settings()->set_black_and_white( true );
    request.mutable_plot_settings()->set_plot_drawing_sheet( false );
    request.mutable_plot_settings()->set_drill_marks( kiapi::board::jobs::PDM_FULL );
    request.set_page_mode( kiapi::board::jobs::BJPM_EACH_LAYER_OWN_FILE );

    kiapi::common::types::RunJobResponse response;
    BOOST_REQUIRE_MESSAGE( Client().RunJob( request, &response ), "RunJob failed: " + Client().LastError() );

    BOOST_REQUIRE_MESSAGE( response.status() == kiapi::common::types::JS_SUCCESS,
                           "Job failed: " + wxString::FromUTF8( response.message() ) );

    BOOST_REQUIRE_MESSAGE( response.output_path_size() > 0, "Job returned no output paths" );

    wxString generatedPath = wxString::FromUTF8( response.output_path( 0 ) );
    BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( generatedPath ), "Generated SVG does not exist: " + generatedPath );

    // Image comparison in c++ would be hard; so for now just check size is close
    constexpr long target_size = 41839;
    wxFileName generatedFn( generatedPath );
    BOOST_CHECK_LT( std::abs( target_size - static_cast<long>( generatedFn.GetSize().GetValue() ) ), 10 );

    if( wxFileName::FileExists( generatedPath ) )
        wxRemoveFile( generatedPath );

    if( wxFileName::FileExists( outputPath.GetFullPath() ) )
        wxRemoveFile( outputPath.GetFullPath() );
}


BOOST_FIXTURE_TEST_CASE( ExportBoardDrill, API_SERVER_E2E_FIXTURE )
{
    BOOST_REQUIRE_MESSAGE( Start(), LastError() );

    wxString testDataDir = wxString::FromUTF8( KI_TEST::GetTestDataRootDir() ) + wxS( "cli/basic_test/" );

    wxFileName boardPath( testDataDir, wxS( "basic_test.kicad_pcb" ) );

    kiapi::common::types::DocumentSpecifier document;

    BOOST_REQUIRE_MESSAGE( Client().OpenDocument( boardPath.GetFullPath(), &document ),
                           "OpenDocument failed: " + Client().LastError() );

    wxString outputDir = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + wxS( "api_job_drill_" )
                         + wxString::Format( "%ld", wxGetProcessId() ) + wxFileName::GetPathSeparator();
    wxFileName::Mkdir( outputDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    kiapi::board::jobs::RunBoardJobExportDrill request;
    *request.mutable_job_settings()->mutable_document() = document;
    request.mutable_job_settings()->set_output_path( outputDir.ToUTF8().data() );
    request.set_format( kiapi::board::jobs::DF_EXCELLON );

    kiapi::common::types::RunJobResponse response;
    BOOST_REQUIRE_MESSAGE( Client().RunJob( request, &response ), "RunJob failed: " + Client().LastError() );

    BOOST_REQUIRE_MESSAGE( response.status() == kiapi::common::types::JS_SUCCESS,
                           "Job failed: " + wxString::FromUTF8( response.message() ) );

    BOOST_REQUIRE_MESSAGE( response.output_path_size() > 0, "Job returned no output paths" );

    // Find the generated .drl file.  The API may return either explicit file paths or an output
    // directory (for jobs that emit multiple files).
    wxString generatedOutputDir = wxString::FromUTF8( response.output_path( 0 ) );
    wxString generatedDrillPath;

    if( !generatedOutputDir.IsEmpty() )
    {
        wxDir    dir( generatedOutputDir );
        wxString filename;

        if( dir.IsOpened() && dir.GetFirst( &filename, wxS( "*.drl" ), wxDIR_FILES ) )
        {
            wxFileName fn( generatedOutputDir, filename );
            generatedDrillPath = fn.GetFullPath();
        }
    }

    BOOST_REQUIRE_MESSAGE( !generatedDrillPath.IsEmpty(), "No .drl file found in job output" );
    BOOST_REQUIRE_MESSAGE( wxFileName::FileExists( generatedDrillPath ),
                           "Generated drill file does not exist: " + generatedDrillPath );

    wxString goldenPath = testDataDir + wxS( "basic_test_excellon_inches.drl" );
    BOOST_CHECK_MESSAGE( textFilesMatch( goldenPath, generatedDrillPath, 5 ),
                         "Drill output does not match golden file" );

    wxFileName::Rmdir( outputDir, wxPATH_RMDIR_RECURSIVE );
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

    wxFileName outputPath = wxFileName::CreateTempFileName( wxS( "api_job_netlist_" ) );
    outputPath.SetExt( wxS( "cadstar" ) );

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

    // Clean up
    if( wxFileName::FileExists( generatedPath ) )
        wxRemoveFile( generatedPath );

    if( wxFileName::FileExists( outputPath.GetFullPath() ) )
        wxRemoveFile( outputPath.GetFullPath() );
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

    wxFileName outputPath = wxFileName::CreateTempFileName( wxS( "api_job_bom_" ) );
    outputPath.SetExt( wxS( "csv" ) );

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

    if( wxFileName::FileExists( generatedPath ) )
        wxRemoveFile( generatedPath );

    if( wxFileName::FileExists( outputPath.GetFullPath() ) )
        wxRemoveFile( outputPath.GetFullPath() );
}


BOOST_AUTO_TEST_SUITE_END()
