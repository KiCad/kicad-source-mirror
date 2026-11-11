/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>

#include <footprint.h>
#include <pad.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcb_shape.h>
#include <pcb_textbox.h>
#include <zone.h>


namespace
{

std::unique_ptr<FOOTPRINT> loadFootprint( const std::string& aPath )
{
    PCB_IO_KICAD_SEXPR io;
    wxString           name;

    return io.ImportFootprint( wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() + aPath ), name );
}

} // namespace


BOOST_AUTO_TEST_SUITE( FootprintItemEquality )


BOOST_AUTO_TEST_CASE( CustomPadPrimitiveGeometryCounts )
{
    const std::string path = "plugins/cadstar/lib/footprint-with-thermal-pad.pretty/CP_32_7.kicad_mod";

    std::unique_ptr<FOOTPRINT> before = loadFootprint( path );
    std::unique_ptr<FOOTPRINT> after = loadFootprint( path );

    BOOST_REQUIRE( before && after );

    auto customPad = []( FOOTPRINT& aFootprint ) -> PAD*
    {
        for( PAD* pad : aFootprint.Pads() )
        {
            if( !pad->GetPrimitives( F_Cu ).empty() )
                return pad;
        }

        return nullptr;
    };

    PAD* padBefore = customPad( *before );
    PAD* padAfter = customPad( *after );

    BOOST_REQUIRE( padBefore && padAfter );
    BOOST_REQUIRE( *padBefore == *padAfter );

    padAfter->GetPrimitives( F_Cu ).front()->Move( VECTOR2I( 50000, 0 ) );

    BOOST_CHECK( !( *padBefore == *padAfter ) );
}


BOOST_AUTO_TEST_CASE( TextBoxSizeCounts )
{
    const std::string path = "../downgrade/golden/current/metadata.pretty/metadata.kicad_mod";

    std::unique_ptr<FOOTPRINT> before = loadFootprint( path );
    std::unique_ptr<FOOTPRINT> after = loadFootprint( path );

    BOOST_REQUIRE( before && after );

    auto textBox = []( FOOTPRINT& aFootprint ) -> PCB_TEXTBOX*
    {
        for( BOARD_ITEM* item : aFootprint.GraphicalItems() )
        {
            if( item->Type() == PCB_TEXTBOX_T )
                return static_cast<PCB_TEXTBOX*>( item );
        }

        return nullptr;
    };

    PCB_TEXTBOX* boxBefore = textBox( *before );
    PCB_TEXTBOX* boxAfter = textBox( *after );

    BOOST_REQUIRE( boxBefore && boxAfter );
    BOOST_REQUIRE( *boxBefore == *boxAfter );

    boxAfter->SetEnd( boxAfter->GetEnd() + VECTOR2I( 500000, 0 ) );

    BOOST_CHECK( !( *boxBefore == *boxAfter ) );
}


BOOST_AUTO_TEST_CASE( ZoneLayersCount )
{
    const std::string path = "plugins/altium/pcblib/Tracks.pretty/KEEPOUT.kicad_mod";

    std::unique_ptr<FOOTPRINT> before = loadFootprint( path );
    std::unique_ptr<FOOTPRINT> after = loadFootprint( path );

    BOOST_REQUIRE( before && after && !before->Zones().empty() && !after->Zones().empty() );

    ZONE* zoneBefore = before->Zones().front();
    ZONE* zoneAfter = after->Zones().front();

    BOOST_REQUIRE( *zoneBefore == *zoneAfter );

    zoneAfter->SetLayerSet( zoneAfter->GetLayerSet().Contains( B_Cu ) ? LSET{ F_Cu } : LSET{ B_Cu } );

    BOOST_CHECK( !( *zoneBefore == *zoneAfter ) );
}


BOOST_AUTO_TEST_CASE( PadNumberCounts )
{
    const std::string path = "plugins/cadstar/lib/footprint-with-thermal-pad.pretty/CP_32_7.kicad_mod";

    std::unique_ptr<FOOTPRINT> before = loadFootprint( path );
    std::unique_ptr<FOOTPRINT> after = loadFootprint( path );

    BOOST_REQUIRE( before && after && !after->Pads().empty() );
    BOOST_REQUIRE( *before == *after );

    after->Pads().front()->SetNumber( after->Pads().front()->GetNumber() + wxS( "0" ) );

    BOOST_CHECK( !( *before == *after ) );
}


BOOST_AUTO_TEST_SUITE_END()
