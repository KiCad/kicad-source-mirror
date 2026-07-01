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
#include <footprint.h>
#include <pad.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <base_units.h>

#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/process.h>
#include <wx/txtstrm.h>

#include <cmath>
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
    "issue22798.kicad_pcb",
    "padstacks_complex.kicad_pcb",
    "issue12609.kicad_pcb",
    "issue22794.kicad_pcb",
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

    // Verify SurfaceFinish element is present with correct type attribute
    // Schema requires: <SurfaceFinish type="ENIG-N"/>
    BOOST_CHECK_MESSAGE( FileContainsPattern( tempPath, wxT( "<SurfaceFinish" ) ),
                         "SurfaceFinish element should be present" );
    BOOST_CHECK_MESSAGE( FileContainsPattern( tempPath, wxT( "type=\"ENIG-N\"" ) ),
                         "SurfaceFinish type should be ENIG-N" );

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


/**
 * Test that SMD pad solder mask openings are exported (Issue #16658)
 *
 * This test verifies that SMD pads which have implicit solder mask openings
 * (pads on copper layers that don't explicitly include F_Mask/B_Mask in their
 * layer set) still get exported with solder mask features in the IPC-2581 output.
 */
BOOST_AUTO_TEST_CASE( SmdPadSolderMaskExport_Issue16658 )
{
    // Load a board with standard SMD components (capacitors using SMD footprints)
    std::unique_ptr<BOARD> board = LoadBoard( "issue16658/issue16658.kicad_pcb" );

    BOOST_REQUIRE( board );

    // Verify the board has SMD pads with implicit mask openings
    bool hasSmtPad = false;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::SMD )
            {
                hasSmtPad = true;

                // Verify pad is on copper but NOT explicitly on mask layer
                bool isOnCopperOnly = pad->IsOnLayer( F_Cu ) && !pad->IsOnLayer( F_Mask );

                if( isOnCopperOnly )
                {
                    // This is the condition we're testing
                    break;
                }
            }
        }

        if( hasSmtPad )
            break;
    }

    BOOST_REQUIRE_MESSAGE( hasSmtPad, "Test board should have SMD pads" );

    // Export to IPC-2581 version C
    wxString tempPath = CreateTempFile();

    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["version"] = "C";
    props["sigfig"] = "3";

    m_ipc2581Plugin.SaveBoard( tempPath, board.get(), &props );

    BOOST_REQUIRE( wxFileExists( tempPath ) );

    // Verify that F_Mask layer features are present in the export
    // (this was the bug - mask layers were empty for SMD pads)
    bool hasFMaskLayer = FileContainsPattern( tempPath, wxT( "layerRef=\"F.Mask\"" ) )
                         || FileContainsPattern( tempPath, wxT( "layerRef=\"TSM\"" ) );

    BOOST_CHECK_MESSAGE( hasFMaskLayer,
                         "IPC-2581 export should contain F.Mask layer features for SMD pads" );

    // Also check for LayerFeature element with mask layer reference
    bool hasLayerFeature = FileContainsPattern( tempPath, wxT( "<LayerFeature" ) );
    BOOST_CHECK_MESSAGE( hasLayerFeature, "IPC-2581 export should contain LayerFeature elements" );
}


/**
 * Test that footprints with empty references produce valid XML attributes.
 *
 * Footprints with blank GetReference() must not produce empty refDes,
 * RefDes/@name, or PinRef/@componentRef attributes, which are
 * schema-invalid qualified names.
 */
BOOST_AUTO_TEST_CASE( EmptyRefDesProducesValidXml )
{
    std::unique_ptr<BOARD> board = LoadBoard( "padstacks_complex.kicad_pcb" );
    BOOST_REQUIRE( board );

    for( char version : { 'B', 'C' } )
    {
        BOOST_TEST_CONTEXT( "Version " << version )
        {
            wxString tempPath = CreateTempFile();

            std::map<std::string, UTF8> props;
            props["units"] = "mm";
            props["version"] = std::string( 1, version );
            props["sigfig"] = "3";

            m_ipc2581Plugin.SaveBoard( tempPath, board.get(), &props );
            BOOST_REQUIRE( wxFileExists( tempPath ) );

            BOOST_CHECK_MESSAGE( !FileContainsPattern( tempPath, wxT( "refDes=\"\"" ) ),
                                 "Empty refDes attribute found" );
            BOOST_CHECK_MESSAGE( !FileContainsPattern( tempPath, wxT( "<RefDes name=\"\"" ) ),
                                 "Empty RefDes/@name attribute found" );
            BOOST_CHECK_MESSAGE( !FileContainsPattern( tempPath, wxT( "componentRef=\"\"" ) ),
                                 "Empty PinRef/@componentRef attribute found" );
        }

        m_ipc2581Plugin = PCB_IO_IPC2581();
    }
}


/**
 * Test that flipped component rotation is not inverted (Finding A / Issue #18013)
 *
 * IPC-2581C 3.3.2 defines positive rotation as CCW from board top.
 * mirror="true" handles the X-axis negation for bottom components.
 * The component rotation must be the normalized board-top angle,
 * NOT its inverse.
 */
BOOST_AUTO_TEST_CASE( FlippedComponentRotation )
{
    std::unique_ptr<BOARD> board = LoadBoard( "issue12609.kicad_pcb" );
    BOOST_REQUIRE( board );

    wxString tempPath = CreateTempFile();

    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["version"] = "C";
    props["sigfig"] = "3";

    m_ipc2581Plugin.SaveBoard( tempPath, board.get(), &props );
    BOOST_REQUIRE( wxFileExists( tempPath ) );

    // C5 is on B.Cu at 90 degrees. Its Component Xform must be rotation="90.0",
    // not the inverted "270.0". Check by finding Component refDes="C5" and verifying
    // its Xform has rotation="90.0".
    std::ifstream xmlFile( tempPath.ToStdString() );
    BOOST_REQUIRE( xmlFile.is_open() );

    std::string xmlContent( ( std::istreambuf_iterator<char>( xmlFile ) ),
                            std::istreambuf_iterator<char>() );

    // Find the C5 component and check its rotation
    size_t c5Pos = xmlContent.find( "refDes=\"C5\"" );

    if( c5Pos == std::string::npos )
        c5Pos = xmlContent.find( "refDes=\"NOREF_" );

    BOOST_REQUIRE_MESSAGE( c5Pos != std::string::npos,
                           "C5 component should exist in export" );

    // Look for the Xform within the next 200 chars after refDes="C5"
    std::string c5Region = xmlContent.substr( c5Pos, 200 );
    BOOST_CHECK_MESSAGE( c5Region.find( "rotation=\"90.0\"" ) != std::string::npos
                         || c5Region.find( "rotation=\"90.00\"" ) != std::string::npos,
                         "C5 component rotation should be 90, not inverted. Region: "
                         + c5Region );
}


/**
 * Test that Content section includes BomRef when BOM is emitted
 */
BOOST_AUTO_TEST_CASE( ContentBomRef )
{
    std::unique_ptr<BOARD> board = LoadBoard( "issue12609.kicad_pcb" );
    BOOST_REQUIRE( board );

    wxString tempPath = CreateTempFile();

    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["version"] = "C";
    props["sigfig"] = "3";

    m_ipc2581Plugin.SaveBoard( tempPath, board.get(), &props );
    BOOST_REQUIRE( wxFileExists( tempPath ) );

    bool hasBom = FileContainsPattern( tempPath, wxT( "<Bom " ) );
    bool hasBomRef = FileContainsPattern( tempPath, wxT( "<BomRef " ) );

    if( hasBom )
    {
        BOOST_CHECK_MESSAGE( hasBomRef,
                             "Content should have BomRef when Bom section is present" );
    }
}


/**
 * Test that knockout text with holes emits schema-valid XML (Issue #23968)
 *
 * Knockout text renders as a filled polygon with each glyph contour becoming
 * a separate outline. The IPC-2581 FeaturesType allows only one Feature child,
 * so when a knockout text produces more than one Contour it must be wrapped
 * in a UserSpecial (which itself is a Feature that may contain any number
 * of child Features).
 *
 * test_copper_graphics.kicad_pcb contains a knockout text "Otherwise" on
 * B.Cu, whose 'O' glyph produces two contours (outer and inner), which
 * reproduces the multi-contour case.
 */
BOOST_AUTO_TEST_CASE( KnockoutTextMultiContour_Issue23968 )
{
    std::unique_ptr<BOARD> board = LoadBoard( "test_copper_graphics.kicad_pcb" );
    BOOST_REQUIRE( board );

    for( char version : { 'B', 'C' } )
    {
        BOOST_TEST_CONTEXT( "Version " << version )
        {
            wxString errorMsg;
            bool valid = ExportAndValidate( board.get(), version, errorMsg );

            BOOST_CHECK_MESSAGE( valid,
                    wxString::Format( "Knockout text export should be schema-valid "
                                      "(version %c): %s", version, errorMsg ) );
        }

        m_ipc2581Plugin = PCB_IO_IPC2581();
    }
}


/**
 * Verify IPC-2581 backdrill encoding matches the schema and expresses the
 * correct must-not-cut layer.
 *
 * KiCad's PADSTACK::DRILL_PROPS::end is the must-cut layer (per the UI
 * "must-cut" labels). The IPC-2581 standard expects the must-not-cut layer,
 * which is the next signal layer past the must-cut layer. The exporter must
 * emit one Spec containing three sibling Backdrill children with type
 * START_LAYER, MUST_NOT_CUT_LAYER, and MAX_STUB_LENGTH, each carrying its
 * value in a Property child.
 */
BOOST_AUTO_TEST_CASE( BackdrillSpecEncoding )
{
    // Build a minimal 6-layer synthetic board so we can place a via whose
    // backdrill targets a specific must-cut layer.
    BOARD board;
    board.SetCopperLayerCount( 6 );

    BOARD_DESIGN_SETTINGS& dsn = board.GetDesignSettings();
    dsn.GetStackupDescriptor().BuildDefaultStackupList( &dsn, 6 );

    // Front-side backdrill: drill from F_Cu, must cut through In3_Cu. The
    // must-not-cut layer should therefore resolve to In4_Cu (the next signal
    // layer past must-cut going inward from the start surface).
    auto* via = new PCB_VIA( &board );
    via->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) ) );
    via->SetLayerPair( F_Cu, B_Cu );
    via->SetDrill( pcbIUScale.mmToIU( 0.30 ) );
    via->SetWidth( pcbIUScale.mmToIU( 0.60 ) );
    via->SetSecondaryDrillSize( pcbIUScale.mmToIU( 0.40 ) );
    via->SetSecondaryDrillStartLayer( F_Cu );
    via->SetSecondaryDrillEndLayer( In3_Cu );
    via->SetFrontPostMachiningMode( PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK );
    board.Add( via );

    wxString tempPath = CreateTempFile();
    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["version"] = "C";
    props["sigfig"] = "4";

    BOOST_REQUIRE_NO_THROW( m_ipc2581Plugin.SaveBoard( tempPath, &board, &props ) );
    BOOST_REQUIRE( wxFileExists( tempPath ) );

    BOOST_CHECK_MESSAGE(
            FileContainsPattern( tempPath, wxT( "<Backdrill type=\"START_LAYER\"" ) ),
            "Backdrill spec should declare a START_LAYER child" );
    BOOST_CHECK_MESSAGE(
            FileContainsPattern( tempPath, wxT( "<Backdrill type=\"MUST_NOT_CUT_LAYER\"" ) ),
            "Backdrill spec should declare a MUST_NOT_CUT_LAYER child" );
    BOOST_CHECK_MESSAGE(
            FileContainsPattern( tempPath, wxT( "<Backdrill type=\"MAX_STUB_LENGTH\"" ) ),
            "Backdrill spec should declare a MAX_STUB_LENGTH child" );

    // Schema requires layer references via Property layerOrGroupRef, not as
    // Backdrill attributes.
    BOOST_CHECK_MESSAGE( FileContainsPattern( tempPath, wxT( "layerOrGroupRef=" ) ),
                         "Backdrill must convey layers through Property layerOrGroupRef" );
    BOOST_CHECK_MESSAGE( !FileContainsPattern( tempPath, wxT( "startLayerRef=" ) ),
                         "Backdrill should not use schema-invalid startLayerRef attribute" );
    BOOST_CHECK_MESSAGE( !FileContainsPattern( tempPath, wxT( "mustNotCutLayerRef=" ) ),
                         "Backdrill should not use schema-invalid mustNotCutLayerRef attribute" );
    BOOST_CHECK_MESSAGE( !FileContainsPattern( tempPath, wxT( "maxStubLength=" ) ),
                         "Backdrill should not use schema-invalid maxStubLength attribute" );
    BOOST_CHECK_MESSAGE( !FileContainsPattern( tempPath, wxT( "postMachining=" ) ),
                         "Backdrill should not use the non-standard postMachining attribute" );

    // The must-not-cut layer for a backdrill from F_Cu through In3_Cu in a
    // 6-layer stack must resolve to In4_Cu, not the must-cut layer itself.
    BOOST_CHECK_MESSAGE(
            FileContainsPattern( tempPath, wxT( "layerOrGroupRef=\"In4.Cu\"" ) ),
            "Front backdrill must-not-cut layer should be In4.Cu" );

    // The third (primary) backdrill spec slot has been removed; through-drills
    // must not be exported as backdrill specs.
    BOOST_CHECK_MESSAGE( !FileContainsPattern( tempPath, wxT( "BD_1C" ) ),
                         "Exporter should not emit a primary backdrill spec slot" );

    // Counterbore/countersink encoded as a Backdrill type=OTHER child with
    // a comment, never as a non-standard postMachining attribute.
    BOOST_CHECK_MESSAGE(
            FileContainsPattern( tempPath, wxT( "<Backdrill type=\"OTHER\"" ) ),
            "Post-machining hint should produce a Backdrill type=OTHER child" );
    BOOST_CHECK_MESSAGE(
            FileContainsPattern( tempPath, wxT( "comment=\"post-machining=COUNTERSINK\"" ) ),
            "OTHER Backdrill should carry the post-machining comment" );
}


/**
 * Test that SMD pads without F.Paste in their layer set are not added to the
 * solder paste layer (Issue #24318).
 *
 * Exposed-pad QFN/QFP footprints commonly use a copper-only thermal pad
 * (F.Cu + F.Mask, no F.Paste) plus several separate paste-only apertures.
 * The earlier #16658 fix implicitly added every copper SMD pad to the paste
 * layer, which contaminated the stencil for these footprints. Mask is still
 * implicit for SMD pads (every copper pad needs a mask opening), but paste
 * must be respected exactly as authored.
 */
BOOST_AUTO_TEST_CASE( ExposedPadPasteRespected_Issue24318 )
{
    BOARD board;

    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetReference( wxT( "U1" ) );
    fp->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 50 ), pcbIUScale.mmToIU( 50 ) ) );
    board.Add( fp );

    // Copper-only thermal pad: F.Cu + F.Mask, deliberately NOT on F.Paste.
    PAD* thermalPad = new PAD( fp );
    thermalPad->SetNumber( wxT( "33" ) );
    thermalPad->SetAttribute( PAD_ATTRIB::SMD );
    thermalPad->SetProperty( PAD_PROP::HEATSINK );
    thermalPad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    thermalPad->SetSize( PADSTACK::ALL_LAYERS,
                         VECTOR2I( pcbIUScale.mmToIU( 3.45 ), pcbIUScale.mmToIU( 3.45 ) ) );
    thermalPad->SetLayerSet( LSET( { F_Cu, F_Mask } ) );
    fp->Add( thermalPad );

    // Paste-only aperture pad, models a stencil opening for the thermal pad.
    PAD* pasteAperture = new PAD( fp );
    pasteAperture->SetNumber( wxEmptyString );
    pasteAperture->SetAttribute( PAD_ATTRIB::SMD );
    pasteAperture->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    pasteAperture->SetSize( PADSTACK::ALL_LAYERS,
                            VECTOR2I( pcbIUScale.mmToIU( 0.93 ),
                                      pcbIUScale.mmToIU( 0.93 ) ) );
    pasteAperture->SetPosition( fp->GetPosition()
                                + VECTOR2I( pcbIUScale.mmToIU( 1.15 ),
                                            pcbIUScale.mmToIU( 1.15 ) ) );
    pasteAperture->SetLayerSet( LSET( { F_Paste } ) );
    fp->Add( pasteAperture );

    // Add a control pad whose paste IS authored (F.Cu + F.Mask + F.Paste). It must still
    // appear on the paste layer, confirming the fix doesn't suppress legitimate paste pads.
    FOOTPRINT* fp2 = new FOOTPRINT( &board );
    fp2->SetReference( wxT( "R1" ) );
    fp2->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 60 ), pcbIUScale.mmToIU( 60 ) ) );
    board.Add( fp2 );

    PAD* normalSmd = new PAD( fp2 );
    normalSmd->SetNumber( wxT( "1" ) );
    normalSmd->SetAttribute( PAD_ATTRIB::SMD );
    normalSmd->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    normalSmd->SetSize( PADSTACK::ALL_LAYERS,
                        VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );
    normalSmd->SetLayerSet( LSET( { F_Cu, F_Mask, F_Paste } ) );
    fp2->Add( normalSmd );

    // Implicit-mask control pad: F.Cu only. Mask must be added implicitly by the exporter,
    // matching the #16658 fix. This guards against accidental removal of the mask code path.
    PAD* implicitMaskSmd = new PAD( fp2 );
    implicitMaskSmd->SetNumber( wxT( "2" ) );
    implicitMaskSmd->SetAttribute( PAD_ATTRIB::SMD );
    implicitMaskSmd->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
    implicitMaskSmd->SetSize( PADSTACK::ALL_LAYERS,
                              VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );
    implicitMaskSmd->SetPosition( fp2->GetPosition()
                                  + VECTOR2I( pcbIUScale.mmToIU( 2.0 ), 0 ) );
    implicitMaskSmd->SetLayerSet( LSET( { F_Cu } ) );
    fp2->Add( implicitMaskSmd );

    wxString tempPath = CreateTempFile();
    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["version"] = "C";
    props["sigfig"] = "4";

    BOOST_REQUIRE_NO_THROW( m_ipc2581Plugin.SaveBoard( tempPath, &board, &props ) );
    BOOST_REQUIRE( wxFileExists( tempPath ) );

    std::ifstream xmlFile( tempPath.ToStdString() );
    BOOST_REQUIRE( xmlFile.is_open() );

    std::string xml( ( std::istreambuf_iterator<char>( xmlFile ) ),
                     std::istreambuf_iterator<char>() );

    // Locate the F.Paste LayerFeature block, if any.
    const std::string pasteOpen = "<LayerFeature layerRef=\"F.Paste\"";
    size_t pasteStart = xml.find( pasteOpen );

    if( pasteStart != std::string::npos )
    {
        size_t pasteEnd = xml.find( "</LayerFeature>", pasteStart );
        BOOST_REQUIRE( pasteEnd != std::string::npos );

        std::string pasteRegion = xml.substr( pasteStart, pasteEnd - pasteStart );

        // The exposed thermal pad (U1 pin 33) must NOT appear on F.Paste.
        BOOST_CHECK_MESSAGE(
                pasteRegion.find( "<PinRef componentRef=\"U1\" pin=\"33\"" ) == std::string::npos,
                "Copper-only thermal pad U1.33 must not appear on F.Paste layer feature" );

        // The normal SMD pad (R1 pin 1) SHOULD still appear on F.Paste.
        BOOST_CHECK_MESSAGE(
                pasteRegion.find( "<PinRef componentRef=\"R1\" pin=\"1\"" ) != std::string::npos,
                "Normal SMD pad with explicit F.Paste in layer set should still emit a paste "
                "feature" );

        // The implicit-mask control pad (R1 pin 2) had F.Cu only and must NOT have paste.
        BOOST_CHECK_MESSAGE(
                pasteRegion.find( "<PinRef componentRef=\"R1\" pin=\"2\"" ) == std::string::npos,
                "Copper-only SMD pad R1.2 must not appear on F.Paste layer feature" );
    }
    else
    {
        // If no F.Paste layer feature was emitted at all the regression would be hidden, so
        // require its presence (R1 must drive its creation).
        BOOST_FAIL( "Expected an F.Paste LayerFeature for the explicitly-pasted control pad" );
    }

    // Mask must still be added implicitly for the thermal pad. Confirm an F.Mask
    // LayerFeature exists with U1 pin 33 so the #16658 behavior is preserved for mask.
    const std::string maskOpen = "<LayerFeature layerRef=\"F.Mask\"";
    size_t maskStart = xml.find( maskOpen );
    BOOST_REQUIRE_MESSAGE( maskStart != std::string::npos,
                           "F.Mask LayerFeature should still be emitted for SMD copper pads" );

    size_t maskEnd = xml.find( "</LayerFeature>", maskStart );
    BOOST_REQUIRE( maskEnd != std::string::npos );

    std::string maskRegion = xml.substr( maskStart, maskEnd - maskStart );

    BOOST_CHECK_MESSAGE(
            maskRegion.find( "<PinRef componentRef=\"U1\" pin=\"33\"" ) != std::string::npos,
            "Thermal pad with explicit F.Mask should appear on F.Mask layer feature" );

    // The truly implicit case: R1 pin 2 had F.Cu only and must still acquire an F.Mask
    // entry. This is the actual regression guard for the #16658 implicit-mask behavior.
    BOOST_CHECK_MESSAGE(
            maskRegion.find( "<PinRef componentRef=\"R1\" pin=\"2\"" ) != std::string::npos,
            "SMD copper pad without explicit F.Mask must get an implicit F.Mask opening" );
}


/**
 * Read an exported file's full contents into a std::string.
 */
static std::string ReadFile( const wxString& aPath )
{
    std::ifstream file( aPath.ToStdString() );
    return std::string( ( std::istreambuf_iterator<char>( file ) ),
                        std::istreambuf_iterator<char>() );
}


/**
 * Extract the text of the first LayerFeature block for a given layer reference.
 * Returns an empty string when no such LayerFeature exists.
 */
static std::string LayerFeatureRegion( const std::string& aXml, const std::string& aLayerRef )
{
    const std::string open = "<LayerFeature layerRef=\"" + aLayerRef + "\"";
    size_t start = aXml.find( open );

    if( start == std::string::npos )
        return std::string();

    size_t end = aXml.find( "</LayerFeature>", start );

    if( end == std::string::npos )
        return std::string();

    return aXml.substr( start, end - start );
}


/**
 * Test that a rounded gr_rect keeps its corner radius on export (Issue #24754).
 *
 * The exporter used to emit a RectRound with all corner flags off and a radius of
 * stroke_width/2, discarding the shape's real corner radius. A gr_rect with a 0.75 mm corner
 * radius must round its corners and carry that radius in the RectRound primitive.
 */
BOOST_AUTO_TEST_CASE( GrRectCornerRadius_Issue24754 )
{
    BOARD board;
    board.SetCopperLayerCount( 2 );

    PCB_SHAPE* rect = new PCB_SHAPE( &board, SHAPE_T::RECTANGLE );
    rect->SetLayer( Edge_Cuts );
    rect->SetStart( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    rect->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 11.5 ), pcbIUScale.mmToIU( 17 ) ) );
    rect->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.05 ), LINE_STYLE::SOLID ) );
    rect->SetFilled( false );
    rect->SetCornerRadius( pcbIUScale.mmToIU( 0.75 ) );
    board.Add( rect );

    wxString tempPath = CreateTempFile();
    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["version"] = "C";
    props["sigfig"] = "4";

    BOOST_REQUIRE_NO_THROW( m_ipc2581Plugin.SaveBoard( tempPath, &board, &props ) );
    BOOST_REQUIRE( wxFileExists( tempPath ) );

    std::string xml = ReadFile( tempPath );

    // The RectRound must round its corners and carry the 0.75 mm radius, not stroke_width/2.
    size_t rectPos = xml.find( "<RectRound" );
    BOOST_REQUIRE_MESSAGE( rectPos != std::string::npos, "Export should contain a RectRound" );

    std::string rectNode = xml.substr( rectPos, xml.find( '>', rectPos ) - rectPos );

    BOOST_CHECK_MESSAGE( rectNode.find( "radius=\"0.750\"" ) != std::string::npos,
                         "Rounded gr_rect must export its 0.75 mm corner radius. Node: "
                         + rectNode );
    BOOST_CHECK_MESSAGE( rectNode.find( "upperRight=\"true\"" ) != std::string::npos
                         && rectNode.find( "lowerLeft=\"true\"" ) != std::string::npos,
                         "Rounded gr_rect must set its corner flags. Node: " + rectNode );
    BOOST_CHECK_MESSAGE( rectNode.find( "radius=\"0.025\"" ) == std::string::npos,
                         "Corner radius must not collapse to stroke_width/2. Node: " + rectNode );
}


/**
 * Test that a back-only masked through-hole pad gets no F.Mask opening (Issue #24753).
 *
 * A pad on *.Cu with an explicit B.Mask (and no F.Mask) must not be added to the F.Mask
 * LayerFeature. The implicit-mask heuristic only applies when the pad authors no mask side.
 */
BOOST_AUTO_TEST_CASE( BackOnlyMaskNoFrontOpening_Issue24753 )
{
    BOARD board;
    board.SetCopperLayerCount( 2 );

    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetReference( wxT( "J29" ) );
    fp->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 50 ), pcbIUScale.mmToIU( 50 ) ) );
    board.Add( fp );

    // Through-hole pad masked on the back only: *.Cu + B.Mask, deliberately no F.Mask.
    PAD* pad = new PAD( fp );
    pad->SetNumber( wxT( "1" ) );
    pad->SetAttribute( PAD_ATTRIB::PTH );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
    pad->SetSize( PADSTACK::ALL_LAYERS,
                  VECTOR2I( pcbIUScale.mmToIU( 2.5 ), pcbIUScale.mmToIU( 4.0 ) ) );
    pad->SetDrillSize( VECTOR2I( pcbIUScale.mmToIU( 1.65 ), pcbIUScale.mmToIU( 1.65 ) ) );
    pad->SetLayerSet( LSET( { F_Cu, B_Cu, B_Mask } ) );
    fp->Add( pad );

    wxString tempPath = CreateTempFile();
    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["version"] = "C";
    props["sigfig"] = "4";

    BOOST_REQUIRE_NO_THROW( m_ipc2581Plugin.SaveBoard( tempPath, &board, &props ) );
    BOOST_REQUIRE( wxFileExists( tempPath ) );

    std::string xml = ReadFile( tempPath );

    // The pad must appear on B.Mask (authored) but never on F.Mask.
    std::string fMask = LayerFeatureRegion( xml, "F.Mask" );
    BOOST_CHECK_MESSAGE(
            fMask.find( "<PinRef componentRef=\"J29\" pin=\"1\"" ) == std::string::npos,
            "Back-only masked pad must not appear on F.Mask layer feature" );

    std::string bMask = LayerFeatureRegion( xml, "B.Mask" );
    BOOST_CHECK_MESSAGE(
            bMask.find( "<PinRef componentRef=\"J29\" pin=\"1\"" ) != std::string::npos,
            "Back-only masked pad should appear on B.Mask layer feature" );
}


/**
 * Test that a roundrect pad's mask aperture shrinks its radius with its size (Issue #24751).
 *
 * A roundrect pad with a negative mask margin has a smaller mask aperture, and its corner
 * radius must shrink by the per-side margin too. The exporter used to reuse the copper radius,
 * and the shape-dict key ignored the layer/expansion so the mask referenced the copper shape.
 */
BOOST_AUTO_TEST_CASE( RoundRectMaskRadius_Issue24751 )
{
    BOARD board;
    board.SetCopperLayerCount( 2 );

    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetReference( wxT( "R1" ) );
    fp->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 40 ), pcbIUScale.mmToIU( 40 ) ) );
    board.Add( fp );

    PAD* pad = new PAD( fp );
    pad->SetNumber( wxT( "1" ) );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::ROUNDRECT );
    pad->SetSize( PADSTACK::ALL_LAYERS,
                  VECTOR2I( pcbIUScale.mmToIU( 0.45 ), pcbIUScale.mmToIU( 0.30 ) ) );
    pad->SetRoundRectRadiusRatio( PADSTACK::ALL_LAYERS, 0.3333333333 );
    pad->SetLayerSet( LSET( { F_Cu, F_Mask } ) );
    pad->SetLocalSolderMaskMargin( pcbIUScale.mmToIU( -0.05 ) );
    fp->Add( pad );

    wxString tempPath = CreateTempFile();
    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["version"] = "C";
    props["sigfig"] = "4";

    BOOST_REQUIRE_NO_THROW( m_ipc2581Plugin.SaveBoard( tempPath, &board, &props ) );
    BOOST_REQUIRE( wxFileExists( tempPath ) );

    std::string xml = ReadFile( tempPath );

    // Copper roundrect radius is 0.10 mm; with a -0.05 mm per-side margin the mask aperture is
    // 0.35 x 0.20 with radius 0.05 mm. Both distinct primitives must be present.
    BOOST_CHECK_MESSAGE( xml.find( "radius=\"0.10\"" ) != std::string::npos,
                         "Copper roundrect should keep its 0.10 mm radius" );
    BOOST_CHECK_MESSAGE( xml.find( "radius=\"0.050\"" ) != std::string::npos,
                         "Mask roundrect aperture must shrink its radius to 0.05 mm" );
    BOOST_CHECK_MESSAGE( xml.find( "width=\"0.350\"" ) != std::string::npos,
                         "Mask roundrect aperture should be 0.35 mm wide" );
}


/**
 * Test that a multi-layer footprint graphic is emitted on every layer (Issue #24752).
 *
 * An fp_poly on F.Cu + F.Mask used to export only on its primary layer (F.Cu). KiCad plots it
 * on both, so the F.Mask copy must be present too.
 */
BOOST_AUTO_TEST_CASE( MultiLayerFootprintGraphic_Issue24752 )
{
    BOARD board;
    board.SetCopperLayerCount( 2 );

    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetReference( wxT( "G1" ) );
    fp->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 30 ) ) );
    board.Add( fp );

    PCB_SHAPE* poly = new PCB_SHAPE( fp, SHAPE_T::POLY );
    poly->SetLayerSet( LSET( { F_Cu, F_Mask } ) );
    poly->SetFilled( true );
    poly->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );

    SHAPE_POLY_SET polySet;
    polySet.NewOutline();
    polySet.Append( pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 30 ) );
    polySet.Append( pcbIUScale.mmToIU( 31 ), pcbIUScale.mmToIU( 30 ) );
    polySet.Append( pcbIUScale.mmToIU( 31 ), pcbIUScale.mmToIU( 31 ) );
    polySet.Append( pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 31 ) );
    poly->SetPolyShape( polySet );
    fp->Add( poly );

    wxString tempPath = CreateTempFile();
    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["version"] = "C";
    props["sigfig"] = "4";

    BOOST_REQUIRE_NO_THROW( m_ipc2581Plugin.SaveBoard( tempPath, &board, &props ) );
    BOOST_REQUIRE( wxFileExists( tempPath ) );

    std::string xml = ReadFile( tempPath );

    // The polygon graphic must show up under both F.Cu and F.Mask.
    std::string fCu = LayerFeatureRegion( xml, "F.Cu" );
    std::string fMask = LayerFeatureRegion( xml, "F.Mask" );

    BOOST_CHECK_MESSAGE( fCu.find( "UserPrimitiveRef" ) != std::string::npos,
                         "Footprint graphic should be present on F.Cu" );
    BOOST_CHECK_MESSAGE( fMask.find( "UserPrimitiveRef" ) != std::string::npos,
                         "Multi-layer footprint graphic must also appear on F.Mask" );
}


/**
 * Test that the per-layer solder mask margin expands the exported mask primitive
 * (Issue #24749).
 *
 * The standard-primitive cache was keyed on pad geometry alone, so the copper
 * primitive (no expansion) processed first was reused verbatim for the mask
 * layer, dropping the solder mask margin. A 1.0 mm circular pad with a 0.5 mm
 * mask margin must therefore export a 2.0 mm mask circle, not a 1.0 mm one.
 */
BOOST_AUTO_TEST_CASE( SolderMaskMarginExpandsMaskPrimitive_Issue24749 )
{
    BOARD board;

    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetReference( wxT( "FID4" ) );
    fp->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 50 ), pcbIUScale.mmToIU( 50 ) ) );
    board.Add( fp );

    PAD* pad = new PAD( fp );
    pad->SetNumber( wxT( "1" ) );
    pad->SetAttribute( PAD_ATTRIB::SMD );
    pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
    pad->SetSize( PADSTACK::ALL_LAYERS,
                  VECTOR2I( pcbIUScale.mmToIU( 1.0 ), pcbIUScale.mmToIU( 1.0 ) ) );
    pad->SetLayerSet( LSET( { F_Cu, F_Mask } ) );
    pad->SetLocalSolderMaskMargin( pcbIUScale.mmToIU( 0.5 ) );
    fp->Add( pad );

    wxString tempPath = CreateTempFile();
    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["version"] = "C";
    props["sigfig"] = "4";

    BOOST_REQUIRE_NO_THROW( m_ipc2581Plugin.SaveBoard( tempPath, &board, &props ) );
    BOOST_REQUIRE( wxFileExists( tempPath ) );

    std::ifstream xmlFile( tempPath.ToStdString() );
    BOOST_REQUIRE( xmlFile.is_open() );

    std::string xml( ( std::istreambuf_iterator<char>( xmlFile ) ),
                     std::istreambuf_iterator<char>() );

    // Resolve the primitive id referenced by the F.Mask padstack entry, then confirm
    // that primitive's circle diameter is the mask-expanded 2.0 mm, not the 1.0 mm copper size.
    const std::string maskRef = "<PadstackPadDef layerRef=\"F.Mask\"";
    size_t maskDefPos = xml.find( maskRef );
    BOOST_REQUIRE_MESSAGE( maskDefPos != std::string::npos,
                           "F.Mask padstack pad definition should be present" );

    size_t refPos = xml.find( "StandardPrimitiveRef id=\"", maskDefPos );
    BOOST_REQUIRE( refPos != std::string::npos );
    refPos += std::string( "StandardPrimitiveRef id=\"" ).size();
    size_t refEnd = xml.find( '"', refPos );
    std::string maskPrimId = xml.substr( refPos, refEnd - refPos );

    std::string entry = "<EntryStandard id=\"" + maskPrimId + "\"";
    size_t entryPos = xml.find( entry );
    BOOST_REQUIRE_MESSAGE( entryPos != std::string::npos,
                           "Mask primitive EntryStandard should be defined" );

    size_t diaPos = xml.find( "diameter=\"", entryPos );
    BOOST_REQUIRE( diaPos != std::string::npos );
    diaPos += std::string( "diameter=\"" ).size();
    size_t diaEnd = xml.find( '"', diaPos );
    double diameter = std::stod( xml.substr( diaPos, diaEnd - diaPos ) );

    BOOST_CHECK_MESSAGE( std::abs( diameter - 2.0 ) < 1e-4,
                         "F.Mask primitive diameter should be 2.0 mm (1.0 mm pad + 0.5 mm "
                         "margin per side), got " << diameter );
}


BOOST_AUTO_TEST_SUITE_END()
