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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>

#include <cli/exit_codes.h>
#include <jobs/job_export_pcb_pdf.h>
#include <pcbnew_jobs_handler.h>
#include <pcbnew_utils/board_file_utils.h>

#include <wx/string.h>


BOOST_AUTO_TEST_CASE( ExportPdfSingleDocumentOutputPath )
{
    PCBNEW_JOBS_HANDLER handler( nullptr );

    const std::filesystem::path boardPath =
            std::filesystem::path( KI_TEST::GetPcbnewTestDataDir() ) / "api_kitchen_sink.kicad_pcb";

    BOOST_REQUIRE( std::filesystem::exists( boardPath ) );

    const std::filesystem::path outputRoot =
            std::filesystem::temp_directory_path() / "kicad_pdf_single_doc_test";
    const std::filesystem::path outputPath =
            outputRoot / "Assembly" / "Assembly_output.pdf";

    if( std::filesystem::exists( outputRoot ) )
        std::filesystem::remove_all( outputRoot );

    auto pdfJob = std::make_unique<JOB_EXPORT_PCB_PDF>();
    pdfJob->m_filename = wxString::FromUTF8( boardPath.string().c_str() );
    pdfJob->SetConfiguredOutputPath( wxString::FromUTF8( outputPath.string().c_str() ) );
    pdfJob->m_plotDrawingSheet = false;
    pdfJob->m_pdfSingle = true;
    pdfJob->m_pdfGenMode = JOB_EXPORT_PCB_PDF::GEN_MODE::ONE_PAGE_PER_LAYER_ONE_FILE;
    pdfJob->m_plotLayerSequence = LSEQ( { F_Cu } );

    int result = handler.JobExportPdf( pdfJob.get() );
    BOOST_CHECK_EQUAL( result, CLI::EXIT_CODES::OK );

    BOOST_CHECK( std::filesystem::exists( outputPath ) );
    BOOST_CHECK( std::filesystem::is_regular_file( outputPath ) );

    const std::filesystem::path nestedPdf =
            outputPath / ( boardPath.stem().string() + ".pdf" );
    BOOST_CHECK( !std::filesystem::exists( nestedPdf ) );

    std::filesystem::remove_all( outputRoot );
}


BOOST_AUTO_TEST_CASE( ExportPdfSingleDocumentTrailingDisabledLayers )
{
    // 24845: trailing disabled copper layers added a blank page in single-document PDF mode.
    PCBNEW_JOBS_HANDLER handler( nullptr );

    const std::filesystem::path boardPath =
            std::filesystem::path( KI_TEST::GetPcbnewTestDataDir() ) / "api_kitchen_sink.kicad_pcb";
    BOOST_REQUIRE( std::filesystem::exists( boardPath ) );

    const std::filesystem::path outputRoot =
            std::filesystem::temp_directory_path() / "kicad_pdf_trailing_disabled_test";
    const std::filesystem::path outputPath = outputRoot / "out.pdf";

    if( std::filesystem::exists( outputRoot ) )
        std::filesystem::remove_all( outputRoot );

    auto pdfJob = std::make_unique<JOB_EXPORT_PCB_PDF>();
    pdfJob->m_filename = wxString::FromUTF8( boardPath.string().c_str() );
    pdfJob->SetConfiguredOutputPath( wxString::FromUTF8( outputPath.string().c_str() ) );
    pdfJob->m_plotDrawingSheet = false;
    pdfJob->m_pdfSingle = true;
    pdfJob->m_pdfGenMode = JOB_EXPORT_PCB_PDF::GEN_MODE::ONE_PAGE_PER_LAYER_ONE_FILE;

    // api_kitchen_sink is 2-layer, so In1_Cu is disabled and skipped, leaving 2 pages.
    pdfJob->m_plotLayerSequence = LSEQ( { F_Cu, B_Cu, In1_Cu } );

    int result = handler.JobExportPdf( pdfJob.get() );
    BOOST_CHECK_EQUAL( result, CLI::EXIT_CODES::OK );
    BOOST_REQUIRE( std::filesystem::exists( outputPath ) );

    // Each page object is written as "/Type /Page\n" (the tree root uses "/Type /Pages\n").
    std::ifstream in( outputPath, std::ios::binary );
    std::string   pdf( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );

    size_t pageCount = 0;

    for( size_t pos = pdf.find( "/Type /Page\n" ); pos != std::string::npos;
         pos = pdf.find( "/Type /Page\n", pos + 1 ) )
        ++pageCount;

    BOOST_CHECK_EQUAL( pageCount, 2u ); // 3 before the fix

    std::filesystem::remove_all( outputRoot );
}


static std::string plotOneLayerToPdf( const std::string& aBoardFile, PCB_LAYER_ID aLayer, const std::string& aTag,
                                      const std::function<void( JOB_EXPORT_PCB_PDF* )>& aConfigure = {} )
{
    PCBNEW_JOBS_HANDLER handler( nullptr );

    const std::filesystem::path boardPath = std::filesystem::path( KI_TEST::GetPcbnewTestDataDir() ) / aBoardFile;
    BOOST_REQUIRE( std::filesystem::exists( boardPath ) );

    const std::filesystem::path outputRoot = std::filesystem::temp_directory_path() / aTag;
    const std::filesystem::path outputPath = outputRoot / "out.pdf";

    if( std::filesystem::exists( outputRoot ) )
        std::filesystem::remove_all( outputRoot );

    auto pdfJob = std::make_unique<JOB_EXPORT_PCB_PDF>();
    pdfJob->m_filename = wxString::FromUTF8( boardPath.string().c_str() );
    pdfJob->SetConfiguredOutputPath( wxString::FromUTF8( outputPath.string().c_str() ) );
    pdfJob->m_plotDrawingSheet = false;
    pdfJob->m_pdfSingle = true;
    pdfJob->m_pdfGenMode = JOB_EXPORT_PCB_PDF::GEN_MODE::ONE_PAGE_PER_LAYER_ONE_FILE;
    pdfJob->m_plotLayerSequence = LSEQ( { aLayer } );

    if( aConfigure )
        aConfigure( pdfJob.get() );

    BOOST_REQUIRE_EQUAL( handler.JobExportPdf( pdfJob.get() ), CLI::EXIT_CODES::OK );
    BOOST_REQUIRE( std::filesystem::exists( outputPath ) );

    std::ifstream in( outputPath, std::ios::binary );
    std::string   pdf( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );

    std::filesystem::remove_all( outputRoot );

    return pdf;
}


// component_classes.kicad_pcb carries R1 and C2 on the back and U1 on the front.
// The bookmark titles live in the uncompressed outline dictionary.
BOOST_AUTO_TEST_CASE( ExportPdfFrontPageBookmarksOnlyFrontFootprints )
{
    std::string pdf = plotOneLayerToPdf( "component_classes.kicad_pcb", F_Fab, "kicad_pdf_popup_front_test" );

    BOOST_CHECK( pdf.find( "/Title (U1)" ) != std::string::npos );
    BOOST_CHECK_MESSAGE( pdf.find( "/Title (R1)" ) == std::string::npos,
                         "back footprint R1 was bookmarked on a front layer page" );
    BOOST_CHECK_MESSAGE( pdf.find( "/Title (C2)" ) == std::string::npos,
                         "back footprint C2 was bookmarked on a front layer page" );
}


BOOST_AUTO_TEST_CASE( ExportPdfBackPageBookmarksOnlyBackFootprints )
{
    std::string pdf = plotOneLayerToPdf( "component_classes.kicad_pcb", B_Fab, "kicad_pdf_popup_back_test" );

    BOOST_CHECK( pdf.find( "/Title (R1)" ) != std::string::npos );
    BOOST_CHECK( pdf.find( "/Title (C2)" ) != std::string::npos );
    BOOST_CHECK_MESSAGE( pdf.find( "/Title (U1)" ) == std::string::npos,
                         "front footprint U1 was bookmarked on a back layer page" );
}


// No footprint in component_classes.kicad_pcb has board outline geometry of its own.
BOOST_AUTO_TEST_CASE( ExportPdfBoardOutlinePageBookmarksNoFootprints )
{
    std::string pdf = plotOneLayerToPdf( "component_classes.kicad_pcb", Edge_Cuts, "kicad_pdf_popup_edge_test" );

    BOOST_CHECK_MESSAGE( pdf.find( "/Title (U1)" ) == std::string::npos,
                         "U1 was bookmarked on the board outline page" );
    BOOST_CHECK_MESSAGE( pdf.find( "/Title (R1)" ) == std::string::npos,
                         "R1 was bookmarked on the board outline page" );
}


// variant_test.kicad_pcb marks R3 do not populate in "Variant A" only, and R1 is always
// populated. Hiding do not populate footprints leaves nothing of R3 on the fabrication layer.
BOOST_AUTO_TEST_CASE( ExportPdfFabPageSkipsHiddenDoNotPopulateFootprints )
{
    auto hideDNP = []( JOB_EXPORT_PCB_PDF* aJob )
    {
        aJob->m_variant = wxT( "Variant A" );
        aJob->m_hideDNPFPsOnFabLayers = true;
    };

    std::string pdf =
            plotOneLayerToPdf( "variant_test/variant_test.kicad_pcb", F_Fab, "kicad_pdf_popup_dnp_test", hideDNP );

    BOOST_CHECK( pdf.find( "/Title (R1)" ) != std::string::npos );
    BOOST_CHECK_MESSAGE( pdf.find( "/Title (R3)" ) == std::string::npos,
                         "a hidden do not populate footprint was bookmarked on the fab page" );
}


// 3Rs_bv.kicad_pcb is four layers. R3 has through-hole pads, R1 is surface mount, and
// REF** is an unplated mounting hole, which plots no copper at all.
BOOST_AUTO_TEST_CASE( ExportPdfInnerLayerPageSkipsSurfaceMountFootprints )
{
    std::string pdf = plotOneLayerToPdf( "issue23451/3Rs_bv.kicad_pcb", In1_Cu, "kicad_pdf_popup_inner_test" );

    BOOST_CHECK( pdf.find( "/Title (R3)" ) != std::string::npos );
    BOOST_CHECK_MESSAGE( pdf.find( "/Title (R1)" ) == std::string::npos,
                         "surface mount R1 was bookmarked on an inner copper page" );
    BOOST_CHECK_MESSAGE( pdf.find( "/Title (REF**)" ) == std::string::npos,
                         "an unplated hole was bookmarked on an inner copper page" );
}
