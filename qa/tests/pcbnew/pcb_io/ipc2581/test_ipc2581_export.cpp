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

/**
 * @file test_ipc2581_export.cpp
 * Test suite for IPC-2581 export validation
 *
 * Tests per GitLab issues:
 * - #22690: Surface finish data not exported
 * - #19912: OtherSideView may be incorrect for bottom components
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/ipc2581/pcb_io_ipc2581.h>
#include <pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>

#include <board.h>
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>

#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/process.h>
#include <wx/txtstrm.h>

#include <fstream>
#include <sstream>


namespace
{

/**
 * Check if xmllint is available on this system
 */
bool IsXmllintAvailable()
{
    wxArrayString output;
    wxArrayString errors;
    int result = wxExecute( "xmllint --version", output, errors, wxEXEC_SYNC );
    return result == 0;
}


/**
 * Validate an XML file against an XSD schema using xmllint
 * @return empty string on success, error message on failure
 */
wxString ValidateXmlWithXsd( const wxString& aXmlPath, const wxString& aXsdPath )
{
    wxString cmd = wxString::Format( "xmllint --noout --schema \"%s\" \"%s\"",
                                      aXsdPath, aXmlPath );

    wxArrayString output;
    wxArrayString errors;
    int result = wxExecute( cmd, output, errors, wxEXEC_SYNC );

    if( result != 0 )
    {
        wxString errorMsg;

        for( const wxString& line : errors )
            errorMsg += line + "\n";

        return errorMsg;
    }

    return wxEmptyString;
}


/**
 * Check if a file contains a specific XML element or attribute pattern
 */
bool FileContainsPattern( const wxString& aFilePath, const wxString& aPattern )
{
    std::ifstream file( aFilePath.ToStdString() );

    if( !file.is_open() )
        return false;

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    return content.find( aPattern.ToStdString() ) != std::string::npos;
}


/**
 * List of board files from qa/data/pcbnew to test for schema validation.
 * These are relatively simple boards that exercise various features.
 */
static const std::vector<std::string> VALIDATION_TEST_BOARDS = {
    "custom_pads.kicad_pcb",
    "notched_zones.kicad_pcb",
    "sliver.kicad_pcb",
    "tracks_arcs_vias.kicad_pcb",
    "issue7241.kicad_pcb",
    "issue10906.kicad_pcb",
};

} // anonymous namespace


struct IPC2581_EXPORT_FIXTURE
{
    IPC2581_EXPORT_FIXTURE() :
        m_xmllintAvailable( IsXmllintAvailable() )
    {
    }

    ~IPC2581_EXPORT_FIXTURE()
    {
        // Clean up temporary files
        for( const wxString& path : m_tempFiles )
        {
            if( wxFileExists( path ) )
                wxRemoveFile( path );
        }
    }

    wxString CreateTempFile( const wxString& aSuffix = wxT( "" ) )
    {
        wxString path = wxFileName::CreateTempFileName( wxT( "kicad_ipc2581_test" ) );

        if( !aSuffix.IsEmpty() )
            path += aSuffix;
        else
            path += wxT( ".xml" );

        m_tempFiles.push_back( path );
        return path;
    }

    wxString GetXsdPath( char aVersion )
    {
        wxString filename = ( aVersion == 'C' ) ? wxT( "IPC-2581C.xsd" ) : wxT( "IPC-2581B1.xsd" );
        return KI_TEST::GetPcbnewTestDataDir() + "ipc2581/" + filename;
    }

    std::unique_ptr<BOARD> LoadBoard( const std::string& aRelativePath )
    {
        std::string fullPath = KI_TEST::GetPcbnewTestDataDir() + aRelativePath;
        std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

        m_kicadPlugin.LoadBoard( fullPath, board.get(), nullptr, nullptr );

        return board;
    }

    bool ExportAndValidate( BOARD* aBoard, char aVersion, wxString& aErrorMsg )
    {
        wxString tempPath = CreateTempFile();

        std::map<std::string, UTF8> props;
        props["units"] = "mm";
        props["version"] = std::string( 1, aVersion );
        props["sigfig"] = "3";

        try
        {
            m_ipc2581Plugin.SaveBoard( tempPath, aBoard, &props );
        }
        catch( const std::exception& e )
        {
            aErrorMsg = wxString::Format( "Export failed: %s", e.what() );
            return false;
        }

        if( !wxFileExists( tempPath ) )
        {
            aErrorMsg = "Export file was not created";
            return false;
        }

        if( m_xmllintAvailable )
        {
            wxString xsdPath = GetXsdPath( aVersion );

            if( wxFileExists( xsdPath ) )
            {
                aErrorMsg = ValidateXmlWithXsd( tempPath, xsdPath );
                return aErrorMsg.IsEmpty();
            }
        }

        // If xmllint not available, just check that export succeeded
        return true;
    }

    bool                     m_xmllintAvailable;
    std::vector<wxString>    m_tempFiles;
    PCB_IO_IPC2581           m_ipc2581Plugin;
    PCB_IO_KICAD_SEXPR       m_kicadPlugin;
};


BOOST_FIXTURE_TEST_SUITE( Ipc2581Export, IPC2581_EXPORT_FIXTURE )


/**
 * Test that surface finish is exported correctly (Issue #22690)
 *
 * This test verifies that when a board has a surface finish defined:
 * 1. A Spec element with SurfaceFinish is created
 * 2. COATING_TOP and COATING_BOTTOM Layer elements are created with layerFunction="COATINGCOND"
 * 3. StackupLayer entries reference the surface finish Spec
 */
BOOST_AUTO_TEST_CASE( SurfaceFinishExport )
{
    // Load a board with ENIG surface finish (issue3812.kicad_pcb has ENIG)
    std::unique_ptr<BOARD> board = LoadBoard( "issue3812.kicad_pcb" );

    BOOST_REQUIRE( board );

    // Verify the board has ENIG finish
    const BOARD_STACKUP& stackup = board->GetDesignSettings().GetStackupDescriptor();
    BOOST_CHECK_EQUAL( stackup.m_FinishType, wxT( "ENIG" ) );

    // Export to IPC-2581 version C
    wxString tempPath = CreateTempFile();

    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["version"] = "C";
    props["sigfig"] = "3";

    m_ipc2581Plugin.SaveBoard( tempPath, board.get(), &props );

    BOOST_REQUIRE( wxFileExists( tempPath ) );

    // Verify SurfaceFinish element is present with correct type
    // Upstream uses nested structure: <SurfaceFinish><Finish type="ENIG-N"/></SurfaceFinish>
    BOOST_CHECK_MESSAGE( FileContainsPattern( tempPath, wxT( "<SurfaceFinish" ) ),
                         "SurfaceFinish element should be present" );
    BOOST_CHECK_MESSAGE( FileContainsPattern( tempPath, wxT( "<Finish" ) ),
                         "Finish element should be present" );
    BOOST_CHECK_MESSAGE( FileContainsPattern( tempPath, wxT( "type=\"ENIG-N\"" ) ),
                         "Finish type should be ENIG-N" );

    // Verify coating layers are present
    BOOST_CHECK_MESSAGE( FileContainsPattern( tempPath, wxT( "COATING_TOP" ) ),
                         "COATING_TOP layer should be present" );
    BOOST_CHECK_MESSAGE( FileContainsPattern( tempPath, wxT( "COATING_BOTTOM" ) ),
                         "COATING_BOTTOM layer should be present" );
    BOOST_CHECK_MESSAGE( FileContainsPattern( tempPath, wxT( "layerFunction=\"COATINGCOND\"" ) ),
                         "Coating layers should have layerFunction=COATINGCOND" );

    // Note: XSD validation is done separately in SchemaValidation tests.
    // This test focuses on verifying the surface finish elements are present.
}


/**
 * Test that boards without surface finish don't generate coating layers
 */
BOOST_AUTO_TEST_CASE( NoSurfaceFinishExport )
{
    // Load a board without surface finish (vme-wren.kicad_pcb has "None")
    std::unique_ptr<BOARD> board = LoadBoard( "vme-wren.kicad_pcb" );

    BOOST_REQUIRE( board );

    // Verify the board has no finish
    const BOARD_STACKUP& stackup = board->GetDesignSettings().GetStackupDescriptor();
    BOOST_CHECK( stackup.m_FinishType == wxT( "None" ) || stackup.m_FinishType.IsEmpty() );

    // Export to IPC-2581 version C
    wxString tempPath = CreateTempFile();

    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["version"] = "C";
    props["sigfig"] = "3";

    m_ipc2581Plugin.SaveBoard( tempPath, board.get(), &props );

    BOOST_REQUIRE( wxFileExists( tempPath ) );

    // Verify SurfaceFinish element is NOT present
    BOOST_CHECK_MESSAGE( !FileContainsPattern( tempPath, wxT( "<SurfaceFinish" ) ),
                         "SurfaceFinish element should not be present for 'None' finish" );

    // Verify coating layers are NOT present
    BOOST_CHECK_MESSAGE( !FileContainsPattern( tempPath, wxT( "COATING_TOP" ) ),
                         "COATING_TOP layer should not be present" );
    BOOST_CHECK_MESSAGE( !FileContainsPattern( tempPath, wxT( "COATING_BOTTOM" ) ),
                         "COATING_BOTTOM layer should not be present" );

    // Note: XSD validation is done separately in SchemaValidation tests.
    // This test focuses on verifying coating layers are NOT present for "None" finish.
}


/**
 * Validate IPC-2581B export against schema for multiple boards
 *
 * This test exports each board in VALIDATION_TEST_BOARDS to IPC-2581B format
 * and validates against the IPC-2581B1.xsd schema using xmllint (if available).
 */
BOOST_AUTO_TEST_CASE( SchemaValidationVersionB )
{
    if( !m_xmllintAvailable )
    {
        BOOST_WARN_MESSAGE( false, "xmllint not available, skipping schema validation tests" );
        return;
    }

    wxString xsdPath = GetXsdPath( 'B' );

    if( !wxFileExists( xsdPath ) )
    {
        BOOST_WARN_MESSAGE( false, "IPC-2581B1.xsd not found, skipping schema validation" );
        return;
    }

    for( const std::string& boardFile : VALIDATION_TEST_BOARDS )
    {
        BOOST_TEST_CONTEXT( "Board: " << boardFile << " (Version B)" )
        {
            std::unique_ptr<BOARD> board = LoadBoard( boardFile );

            if( !board )
            {
                BOOST_WARN_MESSAGE( false, "Could not load board: " + boardFile );
                continue;
            }

            wxString errorMsg;
            bool valid = ExportAndValidate( board.get(), 'B', errorMsg );

            BOOST_CHECK_MESSAGE( valid, "IPC-2581B validation failed for " + boardFile + ": " + errorMsg );
        }
    }
}


/**
 * Validate IPC-2581C export against schema for multiple boards
 *
 * This test exports each board in VALIDATION_TEST_BOARDS to IPC-2581C format
 * and validates against the IPC-2581C.xsd schema using xmllint (if available).
 */
BOOST_AUTO_TEST_CASE( SchemaValidationVersionC )
{
    if( !m_xmllintAvailable )
    {
        BOOST_WARN_MESSAGE( false, "xmllint not available, skipping schema validation tests" );
        return;
    }

    wxString xsdPath = GetXsdPath( 'C' );

    if( !wxFileExists( xsdPath ) )
    {
        BOOST_WARN_MESSAGE( false, "IPC-2581C.xsd not found, skipping schema validation" );
        return;
    }

    for( const std::string& boardFile : VALIDATION_TEST_BOARDS )
    {
        BOOST_TEST_CONTEXT( "Board: " << boardFile << " (Version C)" )
        {
            std::unique_ptr<BOARD> board = LoadBoard( boardFile );

            if( !board )
            {
                BOOST_WARN_MESSAGE( false, "Could not load board: " + boardFile );
                continue;
            }

            wxString errorMsg;
            bool valid = ExportAndValidate( board.get(), 'C', errorMsg );

            BOOST_CHECK_MESSAGE( valid, "IPC-2581C validation failed for " + boardFile + ": " + errorMsg );
        }
    }
}


/**
 * Test export of boards with complex features
 *
 * Tests boards with zones, custom pads, and other features that may
 * exercise edge cases in the IPC-2581 exporter.
 */
BOOST_AUTO_TEST_CASE( ComplexBoardExport )
{
    // Test boards with specific complex features
    static const std::vector<std::string> complexBoards = {
        "intersectingzones.kicad_pcb",
        "custom_pads.kicad_pcb",
    };

    for( const std::string& boardFile : complexBoards )
    {
        BOOST_TEST_CONTEXT( "Complex board: " << boardFile )
        {
            std::unique_ptr<BOARD> board = LoadBoard( boardFile );

            if( !board )
            {
                BOOST_WARN_MESSAGE( false, "Could not load board: " + boardFile );
                continue;
            }

            // Test both versions
            for( char version : { 'B', 'C' } )
            {
                BOOST_TEST_CONTEXT( "Version " << version )
                {
                    wxString errorMsg;
                    bool valid = ExportAndValidate( board.get(), version, errorMsg );

                    BOOST_CHECK_MESSAGE( valid,
                        wxString::Format( "Export/validation failed for %s version %c: %s",
                                          boardFile, version, errorMsg ) );
                }
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
