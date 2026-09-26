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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <qa_utils/file_utils.h>

#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_dimension.h>
#include <pcb_drill_chart.h>
#include <pcb_drill_map.h>
#include <pcb_field.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_target.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <zone.h>
#include <pcbnew_utils/board_file_utils.h>

#include <filesystem>
#include <map>


namespace
{

VECTOR2I mm( double aX, double aY )
{
    return VECTOR2I( pcbIUScale.mmToIU( aX ), pcbIUScale.mmToIU( aY ) );
}


void buildBoard( BOARD& aBoard )
{
    PCB_TRACK* segment = new PCB_TRACK( &aBoard );
    segment->SetLayer( F_Cu );
    segment->SetStart( mm( 0, 0 ) );
    segment->SetEnd( mm( 5, 0 ) );
    segment->SetWidth( pcbIUScale.mmToIU( 0.25 ) );
    aBoard.Add( segment );

    PCB_ARC* arc = new PCB_ARC( &aBoard );
    arc->SetLayer( F_Cu );
    arc->SetStart( mm( 5, 0 ) );
    arc->SetMid( mm( 7, 2 ) );
    arc->SetEnd( mm( 9, 0 ) );
    arc->SetWidth( pcbIUScale.mmToIU( 0.25 ) );
    aBoard.Add( arc );

    PCB_VIA* via = new PCB_VIA( &aBoard );
    via->SetPosition( mm( 9, 0 ) );
    via->SetLayerPair( F_Cu, B_Cu );
    via->SetDrill( pcbIUScale.mmToIU( 0.3 ) );
    via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.6 ) );
    aBoard.Add( via );

    FOOTPRINT* fp = new FOOTPRINT( &aBoard );
    fp->SetPosition( mm( 20, 0 ) );
    fp->SetReference( wxS( "U1" ) );

    PCB_FIELD* userField = new PCB_FIELD( fp, FIELD_T::USER, wxS( "Supplier" ) );
    userField->SetText( wxS( "ACME" ) );
    userField->SetLayer( F_Fab );
    fp->Add( userField );

    PAD* pad = new PAD( fp );
    pad->SetPosition( mm( 20, 0 ) );
    pad->SetSize( PADSTACK::ALL_LAYERS, mm( 1, 1 ) );
    fp->Add( pad );

    PCB_TEXT* fpText = new PCB_TEXT( fp );
    fpText->SetText( wxS( "fp text" ) );
    fpText->SetLayer( F_SilkS );
    fp->Add( fpText );
    aBoard.Add( fp );

    PCB_SHAPE* shape = new PCB_SHAPE( &aBoard, SHAPE_T::SEGMENT );
    shape->SetLayer( Edge_Cuts );
    shape->SetStart( mm( 0, 10 ) );
    shape->SetEnd( mm( 10, 10 ) );
    aBoard.Add( shape );

    PCB_TEXT* text = new PCB_TEXT( &aBoard );
    text->SetText( wxS( "board text" ) );
    text->SetLayer( F_SilkS );
    aBoard.Add( text );

    PCB_TEXTBOX* textbox = new PCB_TEXTBOX( &aBoard );
    textbox->SetText( wxS( "box" ) );
    textbox->SetLayer( F_SilkS );
    textbox->SetStart( mm( 0, 20 ) );
    textbox->SetEnd( mm( 5, 25 ) );
    aBoard.Add( textbox );

    PCB_DIM_ALIGNED* dimension = new PCB_DIM_ALIGNED( &aBoard );
    dimension->SetLayer( Cmts_User );
    dimension->SetStart( mm( 0, 30 ) );
    dimension->SetEnd( mm( 10, 30 ) );
    aBoard.Add( dimension );

    PCB_TARGET* target = new PCB_TARGET( &aBoard );
    target->SetLayer( Edge_Cuts );
    target->SetPosition( mm( 30, 30 ) );
    aBoard.Add( target );

    ZONE* zone = new ZONE( &aBoard );
    zone->SetLayer( F_Cu );
    zone->AppendCorner( mm( 40, 0 ), -1 );
    zone->AppendCorner( mm( 45, 0 ), -1 );
    zone->AppendCorner( mm( 45, 5 ), -1 );
    aBoard.Add( zone );

    PCB_TABLE* table = new PCB_TABLE( &aBoard, 0 );
    table->SetLayer( F_SilkS );
    table->SetColCount( 1 );
    table->AddCell( new PCB_TABLECELL( table ) );
    aBoard.Add( table );

    PCB_DRILL_MAP* map = new PCB_DRILL_MAP( &aBoard );
    map->SetLayer( User_1 );
    aBoard.Add( map );

    PCB_DRILL_CHART* chart = new PCB_DRILL_CHART( &aBoard );
    chart->SetLayer( User_2 );
    aBoard.Add( chart );
    chart->RebuildCells( aBoard );

    PCB_GROUP* group = new PCB_GROUP( &aBoard );
    group->AddItem( text );
    aBoard.Add( group );
}

} // namespace


BOOST_AUTO_TEST_SUITE( CustomPropertyRoundTrip )


BOOST_AUTO_TEST_CASE( EveryBoardItemKeepsItsCustomProperty )
{
    BOARD board;
    buildBoard( board );

    // Keyed by uuid so a property that lands on the wrong item still fails
    std::map<KIID, wxString> expected;

    board.RunOnChildren(
            [&]( BOARD_ITEM* aItem )
            {
                // Chart cells are regenerated from the board, not loaded
                if( aItem->Type() == PCB_TABLECELL_T && aItem->GetParent()->Type() == PCB_DRILL_CHART_T )
                    return;

                aItem->SetCustomProperty( wxS( "qa" ), aItem->m_Uuid.AsString() );
                expected[aItem->m_Uuid] = aItem->GetClass();
            },
            RECURSE_MODE::RECURSE );

    KI_TEST::SCOPED_TEMP_DIR    tempDir( "kicad_qa_custom_property" );
    const std::filesystem::path path = tempDir.Path() / "custom_property.kicad_pcb";

    KI_TEST::DumpBoardToFile( board, path );
    std::unique_ptr<BOARD> restored = KI_TEST::ReadBoardFromFileOrStream( path.string() );
    BOOST_REQUIRE( restored );

    std::map<KIID, wxString> found;

    restored->RunOnChildren(
            [&]( BOARD_ITEM* aItem )
            {
                wxString value;

                if( aItem->GetCustomProperty( wxS( "qa" ), value ) && value == aItem->m_Uuid.AsString() )
                    found[aItem->m_Uuid] = aItem->GetClass();
            },
            RECURSE_MODE::RECURSE );

    for( const auto& [uuid, className] : expected )
    {
        BOOST_TEST_CONTEXT( className )
        {
            BOOST_CHECK( found.contains( uuid ) );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
