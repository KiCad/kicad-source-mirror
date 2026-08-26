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
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <system_error>

#include <board.h>
#include <exporters/export_gencad_writer.h>
#include <exporters/export_hyperlynx.h>
#include <locale_io.h>
#include <pcbnew_utils/board_file_utils.h>
#include <specctra_import_export/specctra.h>

#include <wx/filename.h>
#include <wx/string.h>


/*
 * These formats all describe a padstack as a set of per-layer shapes.  Pad "3" of
 * padstacks_complex.kicad_pcb is MODE::CUSTOM with four distinct square sizes, one per copper
 * layer, so an exporter that flattens the stack to F_Cu drops sizes instead of skewing them.
 */

namespace
{
/// Scratch file that cleans up on scope exit, so a BOOST_REQUIRE early-out leaves nothing behind.
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
    wxString                     WxStr() const { return wxString::FromUTF8( m_path.string() ); }

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


std::unique_ptr<BOARD> loadBoard( const char* aName )
{
    const std::filesystem::path boardPath =
            std::filesystem::path( KI_TEST::GetPcbnewTestDataDir() ) / aName;

    BOOST_REQUIRE( std::filesystem::exists( boardPath ) );

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( boardPath.string() );
    BOOST_REQUIRE( board );

    return board;
}


std::unique_ptr<BOARD> loadComplexPadstackBoard()
{
    return loadBoard( "padstacks_complex.kicad_pcb" );
}


/// Collect every distinct capture of @p aPattern's first group across @p aText.
std::set<std::string> captureAll( const std::string& aText, const std::string& aPattern )
{
    std::set<std::string> found;
    std::regex            re( aPattern );

    for( auto it = std::sregex_iterator( aText.begin(), aText.end(), re );
         it != std::sregex_iterator(); ++it )
    {
        found.insert( ( *it )[1].str() );
    }

    return found;
}
} // namespace


BOOST_AUTO_TEST_SUITE( ComplexPadstackExport )


// GenCAD names one shape per layer in $PADS and references it per layer in $PADSTACKS
BOOST_AUTO_TEST_CASE( GencadEmitsShapePerLayer )
{
    LOCALE_IO              toggle;
    std::unique_ptr<BOARD> board = loadComplexPadstackBoard();

    TEMP_FILE out( wxT( "kicad_gencad_padstack_" ) );

    GENCAD_EXPORTER exporter( board.get() );
    BOOST_REQUIRE( exporter.WriteFile( out.WxStr() ) );

    const std::string cad = slurp( out.Path() );

    // Each unique padstack layer gets its own suffixed shape name
    std::set<std::string> shapeLayers = captureAll( cad, R"(\nPAD P[0-9]+_([A-Za-z0-9.]+) )" );

    BOOST_CHECK_MESSAGE( shapeLayers.count( "F.Cu" ), "expected an F.Cu-specific PAD shape" );
    BOOST_CHECK_MESSAGE( shapeLayers.count( "In1.Cu" ), "expected an In1.Cu-specific PAD shape" );
    BOOST_CHECK_MESSAGE( shapeLayers.count( "In2.Cu" ), "expected an In2.Cu-specific PAD shape" );
    BOOST_CHECK_MESSAGE( shapeLayers.count( "B.Cu" ), "expected a B.Cu-specific PAD shape" );

    // RECTANGLE is "x y width height", so the four square sizes give four distinct half-extents
    std::set<std::string> rectSizes = captureAll( cad, R"(\nRECTANGLE [-0-9.]+ [-0-9.]+ ([0-9.]+))" );

    BOOST_CHECK_MESSAGE( rectSizes.size() >= 4,
                         "flattened padstack: expected at least 4 distinct RECTANGLE sizes, got "
                                 << rectSizes.size() );

    // A padstack entry must be able to name a different shape on different layers
    std::regex  padstackBlock( R"(PADSTACK PAD[0-9]+ [^\n]*\n((?:PAD [^\n]*\n)+))" );
    bool        sawVaryingStack = false;

    for( auto it = std::sregex_iterator( cad.begin(), cad.end(), padstackBlock );
         it != std::sregex_iterator(); ++it )
    {
        if( captureAll( ( *it )[1].str(), R"(PAD (P[0-9_A-Za-z.]+) )" ).size() > 1 )
            sawVaryingStack = true;
    }

    BOOST_CHECK_MESSAGE( sawVaryingStack,
                         "no PADSTACK entry referenced more than one shape across its layers" );

    // Those references land on the right copper only if the names do.  GenCAD numbers inner
    // layers from the top at INNER1, and KiCad writes no $LAYERS section to remap them
    std::set<std::string> stackLayers = captureAll( cad, R"(\nPAD P[0-9_A-Za-z.]+ ([A-Za-z0-9-]+) )" );

    BOOST_CHECK_MESSAGE( stackLayers.count( "INNER1" ), "expected an INNER1 reference" );
    BOOST_CHECK_MESSAGE( stackLayers.count( "INNER2" ), "expected an INNER2 reference" );

    for( const std::string& layer : stackLayers )
    {
        BOOST_CHECK_MESSAGE( layer.find( '-' ) == std::string::npos,
                             "padstack references a layer GenCAD cannot name: " << layer );
    }
}


// A Specctra padstack holds one (shape ...) per layer; its id must stay unique for a given
// physical stack or two padstacks collide on one definition
BOOST_AUTO_TEST_CASE( SpecctraEmitsShapePerLayer )
{
    LOCALE_IO              toggle;
    std::unique_ptr<BOARD> board = loadComplexPadstackBoard();

    TEMP_FILE out( wxT( "kicad_specctra_padstack_" ) );
    BOOST_REQUIRE_NO_THROW( DSN::ExportBoardToSpecctraFile( board.get(), out.WxStr() ) );

    const std::string dsn = slurp( out.Path() );

    // A stack whose copper varies is named for every layer it defines
    BOOST_CHECK_MESSAGE( dsn.find( "Complex[" ) != std::string::npos,
                         "flattened padstack: no layer-by-layer padstack id was emitted" );

    // Rect corners come out per layer, so the four sizes give four distinct extents
    std::set<std::string> rectCorners = captureAll( dsn, R"(\(rect "[^"]+" ([-0-9]+) )" );

    BOOST_CHECK_MESSAGE( rectCorners.size() >= 4,
                         "flattened padstack: expected at least 4 distinct rect extents, got "
                                 << rectCorners.size() );
}


// A Hyperlynx PADSTACK lists ("<layer>", <shape>, <sx>, <sy>, ...) per layer.  A uniform stack
// collapses to the single "MDEF" default instead
BOOST_AUTO_TEST_CASE( HyperlynxEmitsShapePerLayer )
{
    LOCALE_IO              toggle;
    std::unique_ptr<BOARD> board = loadComplexPadstackBoard();

    TEMP_FILE out( wxT( "kicad_hyperlynx_padstack_" ) );
    BOOST_REQUIRE( ExportBoardToHyperlynxFile( board.get(), out.WxStr() ) );

    const std::string hyp = slurp( out.Path() );

    std::set<std::string> padSizes = captureAll( hyp, R"RX(\("[^"]+", [0-9]+, ([0-9.]+),)RX" );

    BOOST_CHECK_MESSAGE( padSizes.size() >= 4,
                         "flattened padstack: expected at least 4 distinct pad sizes, got "
                                 << padSizes.size() );

    // The four sizes must spread across layers, not repeat within one layer
    std::set<std::string> layersWithSizes = captureAll( hyp, R"RX(\("([^"]+)", [0-9]+, [0-9.]+,)RX" );

    BOOST_CHECK_MESSAGE( layersWithSizes.size() >= 4,
                         "expected per-layer entries rather than a single MDEF default" );
}


// The .HYP spec requires the PADSTACK drill parameter to be absent, not zero, on a hole-less
// padstack
BOOST_AUTO_TEST_CASE( HyperlynxOmitsDrillOnSmdPadstacks )
{
    LOCALE_IO toggle;

    std::unique_ptr<BOARD> board = loadBoard( "api_kitchen_sink.kicad_pcb" );

    TEMP_FILE out( wxT( "kicad_hyperlynx_smd_" ) );
    BOOST_REQUIRE( ExportBoardToHyperlynxFile( board.get(), out.WxStr() ) );

    const std::string hyp = slurp( out.Path() );

    BOOST_CHECK_MESSAGE( hyp.find( "{PADSTACK=" ) != std::string::npos,
                         "board produced no padstacks to check" );

    std::set<std::string> drills = captureAll( hyp, R"(\{PADSTACK=[0-9]+, ([0-9.]+))" );

    BOOST_CHECK_MESSAGE( drills.count( "0.000000000" ) == 0,
                         "a hole-less padstack declared a zero drill instead of omitting it" );
}


BOOST_AUTO_TEST_SUITE_END()
