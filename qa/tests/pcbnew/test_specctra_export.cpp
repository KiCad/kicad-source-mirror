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
#include <sstream>
#include <string>
#include <system_error>

#include <board.h>
#include <board_commit.h>
#include <footprint.h>
#include <layer_ids.h>
#include <locale_io.h>
#include <pcb_track.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <specctra_import_export/specctra.h>
#include <tool/tool_manager.h>

#include <wx/filename.h>
#include <wx/string.h>

using namespace DSN;


namespace
{
// Unique per-test scratch file that survives BOOST_REQUIRE early-outs and cleans up on scope exit.
class TEMP_FILE
{
public:
    explicit TEMP_FILE( const wxString& aPrefix ) :
            m_path( std::string( wxFileName::CreateTempFileName( aPrefix ).ToUTF8() ) )
    {
    }

    ~TEMP_FILE()
    {
        std::error_code ec;
        std::filesystem::remove( m_path, ec );
    }

    const std::filesystem::path& Path() const { return m_path; }
    std::string                  Str() const { return m_path.string(); }

private:
    std::filesystem::path m_path;
};


std::string slurp( const std::filesystem::path& aPath )
{
    std::ifstream     in( aPath );
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}


std::unique_ptr<BOARD> loadTestBoard()
{
    const std::filesystem::path boardPath =
            std::filesystem::path( KI_TEST::GetPcbnewTestDataDir() ) / "issue3812.kicad_pcb";

    BOOST_REQUIRE( std::filesystem::exists( boardPath ) );

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( boardPath.string() );
    BOOST_REQUIRE( board );

    return board;
}


int countTraces( BOARD* aBoard )
{
    int n = 0;

    for( PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_TRACE_T )
            ++n;
    }

    return n;
}


void importSession( BOARD* aBoard, const wxString& aSesPath )
{
    TOOL_MANAGER toolMgr;
    toolMgr.SetEnvironment( aBoard, nullptr, nullptr, nullptr, nullptr );
    toolMgr.RegisterTool( new KI_TEST::DUMMY_TOOL() );

    BOARD_COMMIT commit( &toolMgr, true, false );
    ImportSpecctraSession( aBoard, aSesPath, commit );
    commit.Push( wxT( "Import Specctra Session" ), SKIP_UNDO );
}
} // namespace


/*
 * A family of Specctra DSN quoting/naming defects that made otherwise-valid boards un-exportable
 * or produced structurally invalid interchange files (#24946/7/8).
 */


// #24946: a field payload holding the string delimiter (an inch mark in the Value/PN) must not
// terminate the quoted string early and desync a conforming reader.
BOOST_AUTO_TEST_CASE( SpecctraExportQuoteInField )
{
    LOCALE_IO              toggle;
    std::unique_ptr<BOARD> board = loadTestBoard();

    BOOST_REQUIRE( !board->Footprints().empty() );
    board->Footprints().front()->SetValue( wxT( "2x20 pin 0.1\" female header" ) );

    TEMP_FILE out( wxT( "kicad_specctra_24946_" ) );
    BOOST_REQUIRE_NO_THROW( ExportBoardToSpecctraFile( board.get(), out.Str() ) );

    const std::string dsn = slurp( out.Path() );

    // The delimiter must no longer appear inside the payload, and the transformed form is emitted.
    BOOST_CHECK( dsn.find( "0.1\" female" ) == std::string::npos );
    BOOST_CHECK( dsn.find( "0.1'' female" ) != std::string::npos );
}


// #24947: duplicate and empty reference designators (unannotated REF** fiducials, intentional
// duplicates, case-only differences) must not abort the export; they are uniquified on the fly,
// case-insensitively to match the exported case_sensitive default.
BOOST_AUTO_TEST_CASE( SpecctraExportDuplicateEmptyRefs )
{
    LOCALE_IO              toggle;
    std::unique_ptr<BOARD> board = loadTestBoard();

    BOOST_REQUIRE_GE( board->Footprints().size(), 5u );

    board->Footprints()[0]->SetReference( wxEmptyString );
    board->Footprints()[1]->SetReference( wxEmptyString );
    board->Footprints()[2]->SetReference( wxT( "U1" ) );
    board->Footprints()[3]->SetReference( wxT( "U1" ) );
    board->Footprints()[4]->SetReference( wxT( "u1" ) );

    TEMP_FILE out( wxT( "kicad_specctra_24947_" ) );
    BOOST_REQUIRE_NO_THROW( ExportBoardToSpecctraFile( board.get(), out.Str() ) );

    const std::string dsn = slurp( out.Path() );

    // Empty ids take a REF** base; case-insensitive collisions are all suffixed distinctly.
    BOOST_CHECK( dsn.find( "(place REF**_1 " ) != std::string::npos );
    BOOST_CHECK( dsn.find( "(place U1 " ) != std::string::npos );
    BOOST_CHECK( dsn.find( "(place U1_1 " ) != std::string::npos );
    BOOST_CHECK( dsn.find( "(place u1_2 " ) != std::string::npos );
}


// Allegro Specctra writes zone fills as (wire (poly ...)) — an abbreviation of (polygon).
// Parsing must accept that token and skip the pour geometry (zones stay on the board).
BOOST_AUTO_TEST_CASE( SpecctraImportAllegroPolyWire )
{
    LOCALE_IO              toggle;
    std::unique_ptr<BOARD> board = loadTestBoard();

    TEMP_FILE ses( wxT( "kicad_specctra_allegro_poly_" ) );

    const std::string validLayer = std::string( board->GetLayerName( F_Cu ).ToUTF8() );

    std::ofstream out( ses.Path() );
    out << "(session test.ses\n"
        << "  (base_design test.dsn)\n"
        << "  (routes\n"
        << "    (resolution um 10)\n"
        << "    (library_out )\n"
        << "    (network_out\n"
        << "      (net \"/ALLPST\"\n"
        << "        (wire (path \"" << validLayer << "\" 1600  0 0  1000 0)\n"
        << "          (type route))\n"
        << "        (wire (poly \"" << validLayer << "\" 0  0 0  1000 0  1000 1000  0 1000  0 0)\n"
        << "          (type normal))\n"
        << "      )\n"
        << "    )\n"
        << "  )\n"
        << ")\n";
    out.close();

    BOOST_REQUIRE_NO_THROW( importSession( board.get(), ses.Str() ) );

    // Path wire imported; poly wire ignored (zone fill), so exactly one trace.
    BOOST_CHECK_EQUAL( countTraces( board.get() ), 1 );
}


// #24948: single unresolved session items (an unknown wire layer or a placement referencing a
// uniquified id the board never had) must be skipped rather than sinking the whole import.
BOOST_AUTO_TEST_CASE( SpecctraImportSkipsInvalidWireLayer )
{
    LOCALE_IO              toggle;
    std::unique_ptr<BOARD> board = loadTestBoard();

    TEMP_FILE ses( wxT( "kicad_specctra_24948_" ) );

    // The valid wire uses a layer name taken from the loaded board so this is a faithful
    // router-output fragment; the placement id and the second wire's layer are deliberately absent.
    const std::string validLayer = std::string( board->GetLayerName( F_Cu ).ToUTF8() );

    std::ofstream out( ses.Path() );
    out << "(session test.ses\n"
        << "  (base_design test.dsn)\n"
        << "  (placement\n"
        << "    (resolution um 10)\n"
        << "    (component test\n"
        << "      (place Nonexistent_Ref 0 0 front 0)\n"
        << "    )\n"
        << "  )\n"
        << "  (routes\n"
        << "    (resolution um 10)\n"
        << "    (library_out )\n"
        << "    (network_out\n"
        << "      (net \"/ALLPST\"\n"
        << "        (wire (path \"" << validLayer << "\" 1600  0 0  1000 0))\n"
        << "        (wire (path Bogus_Missing_Layer 1600  0 0  1000 1000))\n"
        << "      )\n"
        << "    )\n"
        << "  )\n"
        << ")\n";
    out.close();

    BOOST_REQUIRE_NO_THROW( importSession( board.get(), ses.Str() ) );

    // The session replaces all unlocked traces; only the valid wire survives, the bogus wire and
    // the unresolved placement are dropped rather than throwing.
    BOOST_CHECK_EQUAL( countTraces( board.get() ), 1 );
}
