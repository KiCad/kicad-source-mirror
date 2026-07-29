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

#include <filesystem>
#include <fstream>
#include <memory>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <base_units.h>
#include <board.h>
#include <kiid.h>
#include <pcb_shape.h>
#include <pcb_textbox.h>
#include <pcbnew/pcb_io/odbpp/pcb_io_odbpp.h>
#include <settings/settings_manager.h>
#include <core/utf8.h>

namespace fs = std::filesystem;


// A knockout reaches the feature file as filled polygons, so surface records prove it was emitted.
static int countSurfaceRecords( const fs::path& aFeaturesFile )
{
    int           count = 0;
    std::ifstream stream( aFeaturesFile );
    std::string   line;

    while( std::getline( stream, line ) )
    {
        if( line.rfind( "S ", 0 ) == 0 )
            count++;
    }

    return count;
}


// A knockout text box was cast to PCB_TEXT before being turned into polygons.  The two share no
// base below EDA_TEXT, so the export read the wrong subobject and died.
BOOST_AUTO_TEST_CASE( OdbKnockoutTextboxExport )
{
    SETTINGS_MANAGER       settingsManager;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    const int side = pcbIUScale.mmToIU( 20 );

    auto addEdge = [&]( const VECTOR2I& aStart, const VECTOR2I& aEnd )
    {
        PCB_SHAPE* edge = new PCB_SHAPE( board.get(), SHAPE_T::SEGMENT );
        edge->SetLayer( Edge_Cuts );
        edge->SetStart( aStart );
        edge->SetEnd( aEnd );
        edge->SetWidth( pcbIUScale.mmToIU( 0.1 ) );
        board->Add( edge );
    };

    addEdge( { 0, 0 }, { side, 0 } );
    addEdge( { side, 0 }, { side, side } );
    addEdge( { side, side }, { 0, side } );
    addEdge( { 0, side }, { 0, 0 } );

    PCB_TEXTBOX* textbox = new PCB_TEXTBOX( board.get() );
    textbox->SetLayer( B_SilkS );
    textbox->SetStart( { pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 8 ) } );
    textbox->SetEnd( { pcbIUScale.mmToIU( 15 ), pcbIUScale.mmToIU( 12 ) } );
    textbox->SetText( wxT( "MCU" ) );
    textbox->SetTextSize( { pcbIUScale.mmToIU( 1.26 ), pcbIUScale.mmToIU( 1.26 ) } );
    textbox->SetTextThickness( pcbIUScale.mmToIU( 0.254 ) );
    textbox->SetBold( true );
    textbox->SetMirrored( true );
    textbox->SetBorderEnabled( false );
    textbox->SetIsKnockout( true );
    board->Add( textbox );

    const fs::path outDir =
            fs::temp_directory_path() / ( "kicad_qa_odb_knockout_25074_" + KIID().AsString().ToStdString() );

    BOOST_REQUIRE( fs::create_directory( outDir ) );

    PCB_IO_ODBPP                odbExporter;
    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["sigfig"] = "6";

    BOOST_REQUIRE_NO_THROW( odbExporter.SaveBoard( outDir.string(), board.get(), &props ) );

    const fs::path features = outDir / "steps" / "pcb" / "layers" / "b.silkscreen" / "features";

    BOOST_REQUIRE_MESSAGE( fs::exists( features ), "ODB++ export produced no back silkscreen features file" );

    BOOST_CHECK_MESSAGE( countSurfaceRecords( features ) > 0,
                         "Knockout text box produced no surface features on the back silkscreen" );

    fs::remove_all( outDir );
}
