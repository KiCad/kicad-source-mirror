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

#include <boost/test/unit_test.hpp>

#include <filesystem>
#include <fstream>
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
