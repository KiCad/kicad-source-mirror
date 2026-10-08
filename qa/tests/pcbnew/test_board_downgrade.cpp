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

#include <boost/test/unit_test.hpp>

#include <cmath>
#include <memory>

#include <wx/ffile.h>
#include <wx/filename.h>

#include <base_units.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <padstack.h>
#include <pcb_barcode.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <board_stackup_manager/board_stackup.h>
#include <zone.h>
#include <zone_settings.h>
#include <nlohmann/json.hpp>

#include <downgrade/board_downgrade.h>
#include <downgrade_scan.h>
#include <downgrade_target.h>
#include <drc_rules_downgrade.h>
#include <drc/drc_rule.h>
#include <drc/drc_rule_parser.h>
#include <project/project_file_downgrade.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <settings/settings_manager.h>
#include <pcbnew_utils/board_test_utils.h>
#include <qa_utils/downgrade_oracle_utils.h>
#include <qa_utils/downgrade_golden_utils.h>
#include <qa_utils/temporary_directory.h>


BOOST_AUTO_TEST_SUITE( BoardDowngrade )

static const DOWNGRADE_TARGET& kicad9 = KI_TEST::RequireDowngradeTarget( wxT( "9.0" ) );
static const DOWNGRADE_TARGET& kicad10 = KI_TEST::RequireDowngradeTarget( wxT( "10.0" ) );


// A board with no post-10.0 features exports to 10.0 with nothing lost.
BOOST_AUTO_TEST_CASE( EmptyBoardIsClean )
{
    BOARD board;

    COMPATIBILITY_REPORT report = ClassifyBoardForDowngrade( &board, kicad10 );

    BOOST_CHECK( !report.IsLossy() );
    BOOST_CHECK( !report.IsBlocked() );
    BOOST_CHECK( report.Entries().empty() );
}


// The ellipse primitive only exists since 20260508, so 10.0 gets an approximation.
BOOST_AUTO_TEST_CASE( EllipseIsLoweredForKicad10 )
{
    BOARD board;
    board.Add( new PCB_SHAPE( &board, SHAPE_T::ELLIPSE ) );

    COMPATIBILITY_REPORT    report = ClassifyBoardForDowngrade( &board, kicad10 );

    BOOST_CHECK( report.IsLossy() );
    BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::LOWER ), 1 );
}


// A target that already has the ellipse primitive keeps it. This proves the version gate.
BOOST_AUTO_TEST_CASE( EllipseKeptForNewerTarget )
{
    BOARD board;
    board.Add( new PCB_SHAPE( &board, SHAPE_T::ELLIPSE ) );

    DOWNGRADE_TARGET     future = { wxT( "future" ), wxT( "future" ), 20260623, 20260629, 20260629 };
    COMPATIBILITY_REPORT report = ClassifyBoardForDowngrade( &board, future );

    BOOST_CHECK( !report.IsLossy() );
}


BOOST_AUTO_TEST_CASE( EllipseTransformedToPolygon )
{
    BOARD      board;
    PCB_SHAPE* ellipse = new PCB_SHAPE( &board, SHAPE_T::ELLIPSE );
    ellipse->SetStart( VECTOR2I( 0, 0 ) );
    ellipse->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 5 ), 0 ) );
    board.Add( ellipse );

    DowngradeBoardInPlace( &board, kicad10 );

    BOOST_CHECK( ellipse->GetShape() == SHAPE_T::POLY );
}


// An unfilled ellipse must be lowered to its outline polygon, not to stroke debris.
BOOST_AUTO_TEST_CASE( UnfilledEllipseKeepsOutline )
{
    BOARD      board;
    PCB_SHAPE* ellipse = new PCB_SHAPE( &board, SHAPE_T::ELLIPSE );
    ellipse->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    ellipse->SetEllipseMajorRadius( pcbIUScale.mmToIU( 5 ) );
    ellipse->SetEllipseMinorRadius( pcbIUScale.mmToIU( 3 ) );
    ellipse->SetFillMode( FILL_T::NO_FILL );
    ellipse->SetWidth( pcbIUScale.mmToIU( 0.2 ) );
    board.Add( ellipse );

    DowngradeBoardInPlace( &board, kicad10 );

    BOOST_REQUIRE( ellipse->GetShape() == SHAPE_T::POLY );
    BOOST_REQUIRE_EQUAL( ellipse->GetPolyShape().OutlineCount(), 1 );
    BOOST_CHECK( ellipse->GetFillMode() == FILL_T::NO_FILL );

    double areaMM =
            std::abs( ellipse->GetPolyShape().Outline( 0 ).Area() ) / ( pcbIUScale.IU_PER_MM * pcbIUScale.IU_PER_MM );

    BOOST_CHECK_CLOSE( areaMM, M_PI * 5.0 * 3.0, 2.0 );
}


// An elliptical arc is an open curve, so it is lowered to a filled polygon of its ink.
BOOST_AUTO_TEST_CASE( EllipseArcKeepsInk )
{
    BOARD      board;
    PCB_SHAPE* arc = new PCB_SHAPE( &board, SHAPE_T::ELLIPSE_ARC );
    arc->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    arc->SetEllipseMajorRadius( pcbIUScale.mmToIU( 5 ) );
    arc->SetEllipseMinorRadius( pcbIUScale.mmToIU( 3 ) );
    arc->SetEllipseStartAngle( ANGLE_0 );
    arc->SetEllipseEndAngle( ANGLE_90 );
    arc->SetWidth( pcbIUScale.mmToIU( 0.3 ) );
    board.Add( arc );

    DowngradeBoardInPlace( &board, kicad10 );

    BOOST_REQUIRE( arc->GetShape() == SHAPE_T::POLY );

    // A quarter arc of a 5 mm by 3 mm ellipse is about 6.4 mm of path at 0.3 mm width.
    double areaMM = std::abs( arc->GetPolyShape().Area() ) / ( pcbIUScale.IU_PER_MM * pcbIUScale.IU_PER_MM );

    BOOST_CHECK_GT( areaMM, 1.5 );
    BOOST_CHECK_LT( areaMM, 2.6 );
}


// Text box knockout (20250210) postdates 9.0, so the flag is dropped and reported for 9.0.
BOOST_AUTO_TEST_CASE( KnockoutTextBoxLoweredForKicad9 )
{
    BOARD        board;
    PCB_TEXTBOX* textbox = new PCB_TEXTBOX( &board );
    textbox->SetIsKnockout( true );
    board.Add( textbox );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad10 ).Count( DOWNGRADE_BUCKET::LOWER ), 0 );
    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::LOWER ), 1 );

    DowngradeBoardInPlace( &board, kicad9 );

    BOOST_CHECK( !textbox->IsKnockout() );
}


// Table cell knockout (20260603) postdates both targets, so the flag is dropped for 10.0.
BOOST_AUTO_TEST_CASE( TableCellKnockoutLoweredForKicad10 )
{
    BOARD      board;
    PCB_TABLE* table = new PCB_TABLE( &board, pcbIUScale.mmToIU( 0.1 ) );
    table->SetColCount( 1 );

    PCB_TABLECELL* cell = new PCB_TABLECELL( table );
    cell->SetIsKnockout( true );
    table->AddCell( cell );
    board.Add( table );

    // A table inside a footprint carries the same flag and must be counted and lowered too.
    FOOTPRINT* fp = new FOOTPRINT( &board );
    PCB_TABLE* fpTable = new PCB_TABLE( fp, pcbIUScale.mmToIU( 0.1 ) );
    fpTable->SetColCount( 1 );

    PCB_TABLECELL* fpCell = new PCB_TABLECELL( fpTable );
    fpCell->SetIsKnockout( true );
    fpTable->AddCell( fpCell );
    fp->Add( fpTable );
    board.Add( fp );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad10 ).Count( DOWNGRADE_BUCKET::LOWER ), 2 );

    DowngradeBoardInPlace( &board, kicad10 );

    BOOST_CHECK( !cell->IsKnockout() );
    BOOST_CHECK( !fpCell->IsKnockout() );
}


// A knockout cell in a library footprint goes through the footprint-scoped rule.
BOOST_AUTO_TEST_CASE( FootprintTableCellKnockoutLowered )
{
    FOOTPRINT* fp = new FOOTPRINT( nullptr );
    PCB_TABLE* table = new PCB_TABLE( fp, pcbIUScale.mmToIU( 0.1 ) );
    table->SetColCount( 1 );

    PCB_TABLECELL* cell = new PCB_TABLECELL( table );
    cell->SetIsKnockout( true );
    table->AddCell( cell );
    fp->Add( table );

    DowngradeFootprintInPlace( fp, kicad10 );

    BOOST_CHECK( !cell->IsKnockout() );
    delete fp;
}


// Rounded rectangles (20250829) postdate 9.0, so 9.0 gets a polygon that keeps the rounding.
BOOST_AUTO_TEST_CASE( RoundedRectangleLoweredForKicad9 )
{
    BOARD      board;
    PCB_SHAPE* rect = new PCB_SHAPE( &board, SHAPE_T::RECTANGLE );
    rect->SetStart( VECTOR2I( 0, 0 ) );
    rect->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 6 ) ) );
    rect->SetCornerRadius( pcbIUScale.mmToIU( 1 ) );
    rect->SetFillMode( FILL_T::NO_FILL );
    rect->SetWidth( pcbIUScale.mmToIU( 0.2 ) );
    board.Add( rect );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad10 ).Count( DOWNGRADE_BUCKET::LOWER ), 0 );
    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::LOWER ), 1 );

    DowngradeBoardInPlace( &board, kicad9 );

    BOOST_REQUIRE( rect->GetShape() == SHAPE_T::POLY );
    BOOST_REQUIRE_EQUAL( rect->GetPolyShape().OutlineCount(), 1 );

    // A 10 mm by 6 mm rectangle loses ( 4 - pi ) r^2 to the rounded corners.
    double areaMM =
            std::abs( rect->GetPolyShape().Outline( 0 ).Area() ) / ( pcbIUScale.IU_PER_MM * pcbIUScale.IU_PER_MM );

    BOOST_CHECK_CLOSE( areaMM, 60.0 - ( 4.0 - M_PI ), 2.0 );
}


// A custom footprint stackup (20250818) remaps inner layers. 9.0 would expand them and change
// the connectivity, so the export must block.
BOOST_AUTO_TEST_CASE( CustomFootprintStackupBlocksKicad9 )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetStackupMode( FOOTPRINT_STACKUP::CUSTOM_LAYERS );
    board.Add( fp );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad10 ).Count( DOWNGRADE_BUCKET::BLOCK ), 0 );
    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::BLOCK ), 1 );
}


// Per-zone layer properties (20250302) postdate 9.0, so they are dropped like the board-level
// zone defaults.
BOOST_AUTO_TEST_CASE( ZoneLayerPropertiesDroppedForKicad9 )
{
    BOARD board;
    ZONE* zone = new ZONE( &board );
    zone->LayerProperties()[F_Cu].hatching_offset = VECTOR2I( 100, 100 );
    board.Add( zone );

    // A zone inside a footprint carries the same properties and must be counted and dropped too.
    FOOTPRINT* fp = new FOOTPRINT( &board );
    ZONE*      fpZone = new ZONE( fp );
    fpZone->LayerProperties()[F_Cu].hatching_offset = VECTOR2I( 100, 100 );
    fp->Add( fpZone );
    board.Add( fp );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::DROP ), 2 );

    DowngradeBoardInPlace( &board, kicad9 );

    BOOST_CHECK( zone->LayerProperties().empty() );
    BOOST_CHECK( fpZone->LayerProperties().empty() );
}


BOOST_AUTO_TEST_CASE( BoardZoneDefaultsRoundTripAndDowngrade )
{
    SETTINGS_MANAGER settingsManager;
    BOARD            board;
    board.GetDesignSettings().m_ZoneLayerProperties[F_Cu].hatching_offset = VECTOR2I( 100000, 200000 );
    board.GetDesignSettings().m_ZoneLayerProperties[B_Cu].hatching_offset = VECTOR2I( 300000, 400000 );

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_board_zone_defaults" );
    const std::string            src = ( tmp.GetPath() / "source.kicad_pcb" ).string();
    PCB_IO_KICAD_SEXPR           io;
    io.SaveBoard( src, board );
    std::unique_ptr<BOARD> current( io.LoadBoard( src, nullptr ) );
    BOOST_REQUIRE( current );
    const auto& loadedDefaults = current->GetDesignSettings().m_ZoneLayerProperties;
    BOOST_REQUIRE_EQUAL( loadedDefaults.size(), 2 );
    BOOST_CHECK( loadedDefaults.at( F_Cu ).hatching_offset == VECTOR2I( 100000, 200000 ) );
    BOOST_CHECK( loadedDefaults.at( B_Cu ).hatching_offset == VECTOR2I( 300000, 400000 ) );
    BOOST_CHECK( current->GetDesignSettings().GetDefaultZoneSettings().m_LayerProperties.empty() );
}


// The old (at) header cannot represent a scaled footprint, so the export must block.
BOOST_AUTO_TEST_CASE( ScaledFootprintBlocksExport )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetTransformScale( 2.0, 1.0 );
    board.Add( fp );

    COMPATIBILITY_REPORT    report = ClassifyBoardForDowngrade( &board, kicad10 );

    BOOST_CHECK( report.IsBlocked() );
    BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::BLOCK ), 1 );
}


// IPC-4761 via protection postdates 9.0, so it is dropped and reported for 9.0.
BOOST_AUTO_TEST_CASE( ViaProtectionDroppedForKicad9 )
{
    BOARD    board;
    PCB_VIA* via = new PCB_VIA( &board );
    via->Padstack().Drill().is_capped = true;
    board.Add( via );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );

    DowngradeBoardInPlace( &board, kicad9 );

    BOOST_CHECK( !via->Padstack().Drill().is_capped.has_value() );
}


// Frequency-dependent dielectric models postdate both targets, so they are dropped for 10.0.
BOOST_AUTO_TEST_CASE( DielectricModelDroppedForKicad10 )
{
    BOARD               board;
    BOARD_STACKUP_ITEM* item = new BOARD_STACKUP_ITEM( BS_ITEM_TYPE_DIELECTRIC );
    item->SetDielectricModel( DIELECTRIC_MODEL::DJORDJEVIC_SARKAR );
    board.GetDesignSettings().GetStackupDescriptor().Add( item );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad10 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );

    DowngradeBoardInPlace( &board, kicad10 );

    BOOST_CHECK( item->GetDielectricModel() == DIELECTRIC_MODEL::CONSTANT );
}


// Group design block links postdate 9.0, so they are dropped and reported for 9.0.
BOOST_AUTO_TEST_CASE( GroupDesignBlockLinkDroppedForKicad9 )
{
    BOARD      board;
    PCB_GROUP* group = new PCB_GROUP( &board );
    group->SetDesignBlockLibId( LIB_ID( wxT( "Lib" ), wxT( "Block" ) ) );
    board.Add( group );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );

    DowngradeBoardInPlace( &board, kicad9 );

    BOOST_CHECK( !group->HasDesignBlockLink() );
}


// Extruded 3D bodies (20260410) have no equivalent in 10.0, so they are dropped.
BOOST_AUTO_TEST_CASE( ExtrudedBodyDroppedForKicad10 )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->EnsureExtrudedBody();
    board.Add( fp );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad10 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );

    DowngradeBoardInPlace( &board, kicad10 );

    BOOST_CHECK( !fp->HasExtrudedBody() );
}


// A refill in the old version would turn a thieving pattern into solid copper, quietly.
// That is silent corruption, so the export must block.
BOOST_AUTO_TEST_CASE( ThievingZoneBlocksKicad10 )
{
    BOARD board;
    ZONE* zone = new ZONE( &board );
    zone->SetFillMode( ZONE_FILL_MODE::COPPER_THIEVING );
    board.Add( zone );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad10 ).Count( DOWNGRADE_BUCKET::BLOCK ), 1 );

    DowngradeBoardInPlace( &board, kicad10 );

    BOOST_CHECK( zone->IsCopperThieving() );
}


// Backdrilling changes copper connectivity between layers, so the export must block for 9.0.
BOOST_AUTO_TEST_CASE( BackdrillBlocksKicad9 )
{
    BOARD    board;
    PCB_VIA* via = new PCB_VIA( &board );
    via->SetSecondaryDrillSize( VECTOR2I( pcbIUScale.mmToIU( 0.3 ), pcbIUScale.mmToIU( 0.3 ) ) );
    board.Add( via );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad10 ).Count( DOWNGRADE_BUCKET::BLOCK ), 0 );
    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::BLOCK ), 1 );
}


// Pad simulation electrical types (20260521) are dropped for 10.0.
BOOST_AUTO_TEST_CASE( PadSimTypeDroppedForKicad10 )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    PAD*       pad = new PAD( fp );
    pad->SetSimElectricalType( PAD_SIM_ELECTRICAL_TYPE::SOURCE );
    fp->Add( pad );
    board.Add( fp );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad10 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );

    DowngradeBoardInPlace( &board, kicad10 );

    BOOST_CHECK( pad->GetSimElectricalType() == PAD_SIM_ELECTRICAL_TYPE::NONE );
}


// Counterbore / countersink is a post-9.0 feature 9.0 cannot parse, so it is dropped for 9.0.
BOOST_AUTO_TEST_CASE( PostMachiningDroppedForKicad9 )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    PAD*       pad = new PAD( fp );
    pad->Padstack().FrontPostMachining().mode = PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE;
    fp->Add( pad );
    board.Add( fp );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );

    DowngradeBoardInPlace( &board, kicad9 );

    BOOST_CHECK( !pad->Padstack().FrontPostMachining().mode.has_value() );
}


// Press-fit is a post-9.0 pad property 9.0 cannot parse, so it is dropped for 9.0.
BOOST_AUTO_TEST_CASE( PressFitDroppedForKicad9 )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    PAD*       pad = new PAD( fp );
    pad->SetProperty( PAD_PROP::PRESSFIT );
    fp->Add( pad );
    board.Add( fp );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );

    DowngradeBoardInPlace( &board, kicad9 );

    BOOST_CHECK( pad->GetProperty() == PAD_PROP::NONE );
}


// Skip vias are post-9.0, so they are approximated by the nearest mode 9.0 understands.
BOOST_AUTO_TEST_CASE( SkipViaLoweredForKicad9 )
{
    BOARD    board;
    PCB_VIA* via = new PCB_VIA( &board );
    via->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::START_END_ONLY );
    board.Add( via );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::LOWER ), 1 );

    DowngradeBoardInPlace( &board, kicad9 );

    BOOST_CHECK( via->Padstack().UnconnectedLayerMode() == UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
}


// Buried vias postdate 9.0's blind/buried split, so they are written with the old combined type.
BOOST_AUTO_TEST_CASE( BuriedViaLoweredForKicad9 )
{
    BOARD    board;
    PCB_VIA* via = new PCB_VIA( &board );
    via->SetViaType( VIATYPE::BURIED );
    board.Add( via );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::LOWER ), 1 );

    DowngradeBoardInPlace( &board, kicad9 );

    BOOST_CHECK( via->GetViaType() == VIATYPE::BLIND );
}


// Per-layer zone defaults are a post-9.0 setup block, so they are dropped for 9.0.
BOOST_AUTO_TEST_CASE( ZoneDefaultsDroppedForKicad9 )
{
    BOARD board;
    board.GetDesignSettings().m_ZoneLayerProperties[F_Cu] = {};

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );

    DowngradeBoardInPlace( &board, kicad9 );

    BOOST_CHECK( board.GetDesignSettings().m_ZoneLayerProperties.empty() );
}


// A barcode is fabricated ink, so it becomes its rendered polygons instead of vanishing.
BOOST_AUTO_TEST_CASE( BarcodeLoweredToPolygonsForKicad9 )
{
    BOARD        board;
    PCB_BARCODE* barcode = new PCB_BARCODE( &board );
    barcode->SetLayer( F_SilkS );
    barcode->SetWidth( pcbIUScale.mmToIU( 10 ) );
    barcode->SetHeight( pcbIUScale.mmToIU( 10 ) );
    barcode->SetBarcodeText( wxT( "TEST123" ) );
    board.Add( barcode );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad10 ).Count( DOWNGRADE_BUCKET::LOWER ), 0 );
    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::LOWER ), 1 );

    // The net printed area, holes subtracted. A finder ring must not become a solid square.
    SHAPE_POLY_SET expected;
    barcode->TransformShapeToPolygon( expected, barcode->GetLayer(), 0, 0, ERROR_INSIDE );
    expected.Simplify();
    expected.Fracture();
    double expectedMM = expected.Area() / ( pcbIUScale.IU_PER_MM * pcbIUScale.IU_PER_MM );

    DowngradeBoardInPlace( &board, kicad9 );

    int    polyCount = 0;
    double areaMM = 0;

    for( BOARD_ITEM* item : board.Drawings() )
    {
        BOOST_REQUIRE( item->Type() != PCB_BARCODE_T );

        if( item->Type() == PCB_SHAPE_T )
        {
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );
            BOOST_REQUIRE( shape->GetShape() == SHAPE_T::POLY );
            BOOST_CHECK( shape->GetLayer() == F_SilkS );
            polyCount++;
            areaMM += std::abs( shape->GetPolyShape().Area() ) / ( pcbIUScale.IU_PER_MM * pcbIUScale.IU_PER_MM );
        }
    }

    // The printed ink survives: many module polygons with a real total area.
    BOOST_CHECK_GT( polyCount, 1 );
    BOOST_CHECK_GT( areaMM, 5.0 );
    BOOST_CHECK_CLOSE( areaMM, expectedMM, 0.1 );
}


// A barcode inside a footprint takes the same conversion through the footprint path.
BOOST_AUTO_TEST_CASE( FootprintBarcodeLoweredForKicad9 )
{
    BOARD        board;
    FOOTPRINT*   fp = new FOOTPRINT( &board );
    PCB_BARCODE* barcode = new PCB_BARCODE( fp );
    barcode->SetLayer( F_SilkS );
    barcode->SetWidth( pcbIUScale.mmToIU( 10 ) );
    barcode->SetHeight( pcbIUScale.mmToIU( 10 ) );
    barcode->SetBarcodeText( wxT( "TEST123" ) );
    fp->Add( barcode );
    board.Add( fp );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::LOWER ), 1 );

    DowngradeBoardInPlace( &board, kicad9 );

    int polyCount = 0;

    for( BOARD_ITEM* item : fp->GraphicalItems() )
    {
        BOOST_REQUIRE( item->Type() != PCB_BARCODE_T );

        if( item->Type() == PCB_SHAPE_T && static_cast<PCB_SHAPE*>( item )->GetShape() == SHAPE_T::POLY )
            polyCount++;
    }

    BOOST_CHECK_GT( polyCount, 1 );
}


// A library footprint carries the same features but is downgraded through its own path.
BOOST_AUTO_TEST_CASE( FootprintPressFitDroppedForKicad9 )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    PAD*       pad = new PAD( fp );
    pad->SetProperty( PAD_PROP::PRESSFIT );
    fp->Add( pad );

    DowngradeFootprintInPlace( fp, kicad9 );

    BOOST_CHECK( pad->GetProperty() == PAD_PROP::NONE );

    delete fp;
}


// The fail-closed gate: a post-10.0 token surviving in the output is caught, and lookalikes are not.
BOOST_AUTO_TEST_CASE( VerifyGateCatchesUnsupportedTokens )
{
    // An unstripped feature (e.g. an affine transform) must be caught.
    BOOST_CHECK( FindUnsupportedBoardToken( wxT( "(footprint (transform (translate 1 2)))" ), kicad10 )
                 == wxT( "transform" ) );
    BOOST_CHECK( FindUnsupportedBoardToken( wxT( "(net_chain \"n\")" ), kicad10 ) == wxT( "net_chain" ) );

    // A clean board, and lookalike prefixes, must pass.
    BOOST_CHECK( FindUnsupportedBoardToken( wxT( "(kicad_pcb (net 0 \"n\") (gr_line ...))" ), kicad10 ).IsEmpty() );

    // Value tokens the node scan cannot see must still be caught for 9.0, but not inside strings.

    BOOST_CHECK( FindUnsupportedBoardToken( wxT( "(via buried (at 1 1))" ), kicad9 ) == wxT( "buried" ) );
    BOOST_CHECK( FindUnsupportedBoardToken( wxT( "(gr_text \"buried treasure\")" ), kicad9 ).IsEmpty() );
}


// One unknown keyword makes an old KiCad drop the whole rules file, so the export must
// filter out rules the target cannot parse and keep the rest byte-identical.
BOOST_AUTO_TEST_CASE( DrcRulesFilterForKicad9 )
{
    const wxString rules = wxT( "(version 2)\n"
                                "# keep me\n"
                                "(rule \"clearance_ok\"\n"
                                "  # chamfer runs at 45deg here\n"
                                "  (constraint clearance (min 0.2mm))\n"
                                "  (condition \"A.NetClass == 'HV'\"))\n"
                                "(rule \"mask_check\"\n"
                                "  (constraint bridged_mask))\n"
                                "(rule \"chain_check\"\n"
                                "  (condition \"inNetChain('x')\")\n"
                                "  (constraint clearance (min 0.1mm)))\n"
                                "(rule \"delay_check\"\n"
                                "  (constraint skew (max 5ps)))\n" );

    DRC_RULES_FILTER_RESULT result = FilterDrcRulesForTarget( rules, kicad9 );

    BOOST_REQUIRE_EQUAL( result.m_dropped.size(), 3 );
    BOOST_CHECK( result.m_dropped[0] == wxT( "mask_check" ) );
    BOOST_CHECK( result.m_dropped[1] == wxT( "chain_check" ) );
    BOOST_CHECK( result.m_dropped[2] == wxT( "delay_check" ) );

    BOOST_CHECK( result.m_text.Contains( wxT( "(version 1)" ) ) );
    BOOST_CHECK( result.m_text.Contains( wxT( "# keep me" ) ) );
    BOOST_CHECK( result.m_text.Contains( wxT( "(rule \"clearance_ok\"" ) ) );
    BOOST_CHECK( result.m_text.Contains( wxT( "A.NetClass == 'HV'" ) ) );
    BOOST_CHECK( !result.m_text.Contains( wxT( "bridged_mask" ) ) );
    BOOST_CHECK( !result.m_text.Contains( wxT( "inNetChain" ) ) );
    BOOST_CHECK( !result.m_text.Contains( wxT( "5ps" ) ) );
}


BOOST_AUTO_TEST_CASE( DrcRuleNamesAndExpressionLiteralsDoNotTriggerDrops )
{
    const wxString rules = wxT( "(version 1)\n"
                                "(rule \"return_path\" (constraint clearance (min 0.2mm)))\n"
                                "(rule inNetChain (constraint clearance (min 0.2mm)))\n"
                                "(rule \"literal\" (condition \"A.NetName == 'hasNetChain() 100 ps .Parent.'\")\n"
                                "  (constraint clearance (min 0.2mm)))\n" );

    for( const DOWNGRADE_TARGET* target : { &kicad9, &kicad10 } )
    {
        DRC_RULES_FILTER_RESULT result = FilterDrcRulesForTarget( rules, *target );
        BOOST_CHECK( result.m_error.IsEmpty() );
        BOOST_CHECK( result.m_dropped.empty() );
        BOOST_CHECK_EQUAL( result.m_text.Mid( result.m_text.Find( "(rule" ) ), rules.Mid( rules.Find( "(rule" ) ) );
    }
}


BOOST_AUTO_TEST_CASE( SpacedTimeAndAngleUnitsAreDroppedForKicad9 )
{
    const wxString rules = wxT( "(version 1)\n"
                                "(rule \"time\" (constraint length (min \"100 ps\")))\n"
                                "(rule \"femtoseconds\" (constraint length (max \"2 * 100 fs\")))\n"
                                "(rule \"angle\" (constraint track_angle (max \"90 deg\")))\n"
                                "(rule \"ordinary\" (constraint clearance (min \"0.2 mm\")))\n" );

    DRC_RULES_FILTER_RESULT result9 = FilterDrcRulesForTarget( rules, kicad9 );
    BOOST_CHECK( result9.m_error.IsEmpty() );
    BOOST_CHECK_EQUAL( result9.m_dropped.size(), 3 );
    BOOST_CHECK( result9.m_text.Contains( "ordinary" ) );

    DRC_RULES_FILTER_RESULT result10 = FilterDrcRulesForTarget( rules, kicad10 );
    BOOST_CHECK( result10.m_error.IsEmpty() );
    BOOST_CHECK( result10.m_dropped.empty() );
    BOOST_CHECK_EQUAL( result10.m_text.Mid( result10.m_text.Find( "(rule" ) ), rules.Mid( rules.Find( "(rule" ) ) );
}


// 10.0 knows the mask constraints and time units. Only post-10.0 syntax drops for it.
BOOST_AUTO_TEST_CASE( DrcRulesFilterForKicad10 )
{
    const wxString rules = wxT( "(version 2)\n"
                                "(rule \"mask_check\" (constraint bridged_mask))\n"
                                "(rule \"delay_check\" (constraint skew (max 5ps)))\n"
                                "(rule \"chain_check\" (constraint net_chain_length (max 10mm)))\n" );

    DRC_RULES_FILTER_RESULT result = FilterDrcRulesForTarget( rules, kicad10 );

    BOOST_REQUIRE_EQUAL( result.m_dropped.size(), 1 );
    BOOST_CHECK( result.m_dropped[0] == wxT( "chain_check" ) );
    BOOST_CHECK( result.m_text.Contains( wxT( "bridged_mask" ) ) );
    BOOST_CHECK( result.m_text.Contains( wxT( "5ps" ) ) );
}


// Comment syntax must never participate in balancing a rule. Parentheses or quotes
// in a full-line comment used to truncate or swallow otherwise valid rules.
BOOST_AUTO_TEST_CASE( DrcRulesWithCommentsRemainIntact )
{
    const wxString kept = wxT( "(rule \"clearance # literal\"\n"
                               "  # unmatched: ) (( \" \\\n"
                               "  (condition \"A.NetName == 'SIGNAL#1'\")\n"
                               "  # inNetChain('x')\n"
                               "  (constraint clearance (min 0.2mm))\n"
                               "  # ) \" (\n"
                               ")\n"
                               "(rule \"second\" (constraint track_width (min 0.25mm)))" );
    const wxString input = wxT( "(version 2)\n" ) + kept
                           + wxT( "\n(rule \"unsupported\"\n # ) \" (\n"
                                  " (constraint net_chain_length (max 10mm)))\n" );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        DRC_RULES_FILTER_RESULT result = FilterDrcRulesForTarget( input, target );
        BOOST_REQUIRE( result.m_error.IsEmpty() );
        BOOST_REQUIRE_EQUAL( result.m_dropped.size(), 1 );
        BOOST_CHECK( result.m_dropped.front() == wxT( "unsupported" ) );
        BOOST_CHECK( result.m_text.Contains( kept ) );

        DRC_RULES_PARSER                       parser( result.m_text, wxT( "downgrade_inline_comments" ) );
        WX_STRING_REPORTER                     reporter;
        std::vector<std::shared_ptr<DRC_RULE>> rules;
        parser.Parse( rules, &reporter );
        BOOST_CHECK_MESSAGE( !reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR ), reporter.GetMessages() );
        BOOST_REQUIRE_EQUAL( rules.size(), 2 );
        BOOST_CHECK( rules[0]->FindConstraint( CLEARANCE_CONSTRAINT ).has_value() );
        BOOST_CHECK( rules[1]->FindConstraint( TRACK_WIDTH_CONSTRAINT ).has_value() );
    }
}


BOOST_AUTO_TEST_CASE( InvalidDrcCommentAndUnbalancedSyntaxFailClosed )
{
    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        for( const wxString& input : { wxT( "(version 1)\n(rule \"inline\" # )\n (constraint clearance (min 0.2mm)))" ),
                                       wxT( "(version 1)\n(rule \"missing_close\" (constraint clearance (min 0.2mm))" ),
                                       wxT( "(version 1)\n(rule \"missing_quote" ) } )
        {
            BOOST_CHECK( !FilterDrcRulesForTarget( input, target ).m_error.IsEmpty() );
        }
    }
}


BOOST_AUTO_TEST_CASE( DrcRulesMatchIndependentOlderReferences )
{
    const std::string fixture = KI_TEST::GetPcbnewTestDataDir() + "../downgrade/golden/";
    const wxString    input = wxString::FromUTF8( KI_TEST::ReadGoldenText( fixture + "current/rules.kicad_dru" ) );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        DRC_RULES_FILTER_RESULT result = FilterDrcRulesForTarget( input, target );
        BOOST_REQUIRE( result.m_error.IsEmpty() );
        const std::string release = target.m_id == wxT( "9.0" ) ? "v9" : "v10";
        BOOST_CHECK( result.m_text.ToStdString() == KI_TEST::ReadGoldenText( fixture + release + "/rules.kicad_dru" ) );
        BOOST_CHECK_EQUAL( result.m_dropped.size(), release == "v9" ? 2 : 1 );
        DRC_RULES_PARSER                       parser( result.m_text, wxT( "downgrade_drc_golden" ) );
        WX_STRING_REPORTER                     reporter;
        std::vector<std::shared_ptr<DRC_RULE>> rules;
        parser.Parse( rules, &reporter );
        BOOST_CHECK_MESSAGE( !reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR ), reporter.GetMessages() );
        BOOST_CHECK_EQUAL( rules.size(), release == "v9" ? 1 : 2 );
    }
}


// Flattening bakes the chosen variant's overrides into the base attributes, so the exported
// board matches that variant after the registry is dropped.
BOOST_AUTO_TEST_CASE( VariantFlattenedIntoBoard )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetDNP( false );

    FOOTPRINT_VARIANT* variant = fp->AddVariant( wxT( "V1" ) );
    variant->SetDNP( true );
    variant->SetFieldValue( wxT( "Value" ), wxT( "10k" ) );
    board.Add( fp );

    FlattenBoardVariant( &board, wxT( "V1" ) );

    BOOST_CHECK( fp->IsDNP() );
    BOOST_REQUIRE( fp->GetField( wxT( "Value" ) ) != nullptr );
    BOOST_CHECK( fp->GetField( wxT( "Value" ) )->GetText() == wxT( "10k" ) );
}


// The project file is migrated down: newer settings are removed, nested schema versions are
// rewound, and meaningful losses are reported.
BOOST_AUTO_TEST_CASE( ProjectFileDowngradeForKicad9 )
{
    nlohmann::json doc = {
        { "meta", { { "filename", "p.kicad_pro" }, { "version", 3 } } },
        { "board", { { "layer_presets", { { { "name", "P" }, { "renderLayers", { "tracks", "vias" } } } } } } },
        { "net_settings",
          { { "meta", { { "version", 5 } } },
            { "classes", { { { "name", "Default" }, { "priority", 0 }, { "tuning_profile", "tp" } } } },
            { "net_chain_classes", { { { "name", "chainA" } } } } } },
        { "tuning_profiles",
          { { "meta", { { "version", 1 } } },
            { "tuning_profiles_impedance_geometric",
              { { { "name", "tp" }, { "frequency", 1e9 }, { "model_solder_mask", false } } } } } },
        { "component_class_settings", { { "meta", { { "version", 0 } } } } },
        { "schematic",
          { { "top_level_sheets", { "aaaa" } },
            { "bus_aliases", nlohmann::json::array() },
            { "variants", nlohmann::json::array() } } },
    };

    COMPATIBILITY_REPORT report;
    DowngradeProjectFileJson( doc, kicad9, report );

    // 9.0 shipped project schema 3 and reads render layers by name, skipping names it does
    // not know. So the version and the preset strings must pass through untouched.
    BOOST_CHECK_EQUAL( doc["meta"]["version"].get<int>(), 3 );

    const nlohmann::json& preset = doc["board"]["layer_presets"][0];
    BOOST_REQUIRE( preset["renderLayers"].is_array() );
    BOOST_REQUIRE_EQUAL( preset["renderLayers"].size(), 2 );
    BOOST_CHECK_EQUAL( preset["renderLayers"][0].get<std::string>(), "tracks" );
    BOOST_CHECK_EQUAL( preset["renderLayers"][1].get<std::string>(), "vias" );

    BOOST_CHECK_EQUAL( doc["net_settings"]["meta"]["version"].get<int>(), 4 );
    BOOST_CHECK( !doc["net_settings"]["classes"][0].contains( "tuning_profile" ) );
    BOOST_CHECK( doc["net_settings"]["classes"][0].contains( "name" ) );
    BOOST_CHECK( !doc["net_settings"].contains( "net_chain_classes" ) );
    BOOST_CHECK( !doc.contains( "tuning_profiles" ) );
    BOOST_CHECK( !doc.contains( "component_class_settings" ) );
    BOOST_CHECK( !doc["schematic"].contains( "top_level_sheets" ) );
    BOOST_CHECK( !doc["schematic"].contains( "bus_aliases" ) );
    BOOST_CHECK( !doc["schematic"].contains( "variants" ) );

    // The meaningful losses show in the report. Hygiene removals stay silent.
    BOOST_CHECK( report.IsLossy() );
    BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), 2 );
}


// A structurally odd but valid-JSON project file must not throw. Classification swallows
// exceptions, so a throw here would surface only mid-export and break fail-closed.
BOOST_AUTO_TEST_CASE( WrongShapeProjectFileDoesNotThrow )
{
    nlohmann::json metaIsString = {
        { "meta", "not an object" },
        { "net_settings", { { "meta", "also a string" }, { "classes", "not an array" } } },
        { "schematic", "a string" },
    };

    nlohmann::json profilesWrong = {
        { "tuning_profiles",
          { { "meta", 7 }, { "tuning_profiles_impedance_geometric", { "just", "strings" } } } },
    };

    COMPATIBILITY_REPORT report;
    BOOST_CHECK_NO_THROW( DowngradeProjectFileJson( metaIsString, kicad9, report ) );
    BOOST_CHECK_NO_THROW( DowngradeProjectFileJson( profilesWrong, kicad10, report ) );
}


// An empty net chain class array is removed as hygiene, not reported as a loss.
BOOST_AUTO_TEST_CASE( EmptyNetChainClassesSilent )
{
    nlohmann::json doc = {
        { "net_settings", { { "net_chain_classes", nlohmann::json::array() } } },
    };

    COMPATIBILITY_REPORT report;
    DowngradeProjectFileJson( doc, kicad10, report );

    BOOST_CHECK( !doc["net_settings"].contains( "net_chain_classes" ) );
    BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), 0 );
}


BOOST_AUTO_TEST_CASE( NonDefaultTuningModelSettingsAreReportedForKicad10 )
{
    nlohmann::json doc = {
        { "tuning_profiles",
          { { "meta", { { "version", 1 } } },
            { "tuning_profiles_impedance_geometric",
              { { { "profile_name", "5 GHz" }, { "frequency", 5e9 }, { "model_solder_mask", true } },
                { { "profile_name", "Default" }, { "frequency", 1e9 }, { "model_solder_mask", false } } } } } }
    };

    COMPATIBILITY_REPORT report;
    DowngradeProjectFileJson( doc, kicad10, report );
    BOOST_CHECK( report.IsLossy() );
    BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), 2 );

    for( const auto& profile : doc["tuning_profiles"]["tuning_profiles_impedance_geometric"] )
    {
        BOOST_CHECK( !profile.contains( "frequency" ) );
        BOOST_CHECK( !profile.contains( "model_solder_mask" ) );
    }

    COMPATIBILITY_REPORT secondReport;
    DowngradeProjectFileJson( doc, kicad10, secondReport );
    BOOST_CHECK( !secondReport.IsLossy() );
}


BOOST_AUTO_TEST_CASE( ProjectFileDowngradeForKicad10 )
{
    nlohmann::json doc = {
        { "meta", { { "version", 3 } } },
        { "net_settings",
          { { "meta", { { "version", 5 } } },
            { "classes", { { { "name", "Default" }, { "priority", 0 }, { "tuning_profile", "tp" } } } },
            { "net_chain_classes", { { { "name", "chainA" } } } } } },
        { "tuning_profiles",
          { { "meta", { { "version", 1 } } },
            { "tuning_profiles_impedance_geometric",
              { { { "name", "tp" }, { "frequency", 1e9 }, { "model_solder_mask", false } } } } } },
    };

    COMPATIBILITY_REPORT report;
    DowngradeProjectFileJson( doc, kicad10, report );

    // 10.0 knows tuning profiles and reads the per-class reference unguarded, so both stay.
    BOOST_CHECK_EQUAL( doc["meta"]["version"].get<int>(), 3 );
    BOOST_CHECK_EQUAL( doc["net_settings"]["meta"]["version"].get<int>(), 5 );
    BOOST_CHECK( doc["net_settings"]["classes"][0].contains( "tuning_profile" ) );
    BOOST_CHECK( !doc["net_settings"].contains( "net_chain_classes" ) );

    BOOST_CHECK_EQUAL( doc["tuning_profiles"]["meta"]["version"].get<int>(), 0 );
    BOOST_CHECK( !doc["tuning_profiles"]["tuning_profiles_impedance_geometric"][0].contains( "frequency" ) );
    BOOST_CHECK( !doc["tuning_profiles"]["tuning_profiles_impedance_geometric"][0].contains( "model_solder_mask" ) );

    BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), 1 );
}


// A near-closed thick elliptical arc strokes to a ring. The writers save one outline per
// shape, so the lowered polygon must carry no holes.
BOOST_AUTO_TEST_CASE( EllipseArcRingHasNoHoles )
{
    BOARD      board;
    PCB_SHAPE* arc = new PCB_SHAPE( &board, SHAPE_T::ELLIPSE_ARC );
    arc->SetEllipseCenter( VECTOR2I( 0, 0 ) );
    arc->SetEllipseMajorRadius( pcbIUScale.mmToIU( 5 ) );
    arc->SetEllipseMinorRadius( pcbIUScale.mmToIU( 3 ) );
    arc->SetEllipseStartAngle( ANGLE_0 );
    arc->SetEllipseEndAngle( ANGLE_360 );
    arc->SetWidth( pcbIUScale.mmToIU( 0.3 ) );
    board.Add( arc );

    DowngradeBoardInPlace( &board, kicad10 );

    BOOST_REQUIRE( arc->GetShape() == SHAPE_T::POLY );

    SHAPE_POLY_SET& poly = arc->GetPolyShape();

    for( int ii = 0; ii < poly.OutlineCount(); ++ii )
        BOOST_CHECK_EQUAL( poly.HoleCount( ii ), 0 );

    // The ring's ink, nowhere near the filled disk's 47 square mm.
    double areaMM = std::abs( poly.Area() ) / ( pcbIUScale.IU_PER_MM * pcbIUScale.IU_PER_MM );

    BOOST_CHECK_GT( areaMM, 4.0 );
    BOOST_CHECK_LT( areaMM, 15.0 );
}


// The duplicate-pad-numbers jumper flag is jumper data too, so 9.0 must drop and report it.
BOOST_AUTO_TEST_CASE( DuplicatePadJumperFlagDroppedForKicad9 )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetDuplicatePadNumbersAreJumpers( true );
    board.Add( fp );

    BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, kicad9 ).Count( DOWNGRADE_BUCKET::DROP ), 1 );

    DowngradeBoardInPlace( &board, kicad9 );

    BOOST_CHECK( !fp->GetDuplicatePadNumbersAreJumpers() );
}


// The staged library downgrade converts and verifies every file before any original is
// replaced, and a refusal leaves the folder untouched with no temp files behind.
BOOST_AUTO_TEST_CASE( LibraryFilesDowngradeFailsClosed )
{
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_lib_staged", "" );
    const wxString               dir = tmp.GetPath().wstring();

    auto writeFile = [&]( const wxString& aName, const wxString& aContent )
    {
        wxFFile file( dir + wxFileName::GetPathSeparator() + aName, wxT( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        file.Write( aContent );
    };

    writeFile( wxT( "a.fake_mod" ), wxT( "old a" ) );
    writeFile( wxT( "b.fake_mod" ), wxT( "old b" ) );

    auto noForbidden = []( const wxString& )
    {
        return wxString();
    };

    auto convertOk = [&]( const wxString& aFile, const wxString& aTmp ) -> DOWNGRADE_FILE_RESULT
    {
        wxFFile out( aTmp, wxT( "wb" ) );
        out.Write( wxT( "(version 123) converted" ) );
        return DOWNGRADE_FILE_RESULT::CONVERTED;
    };

    BOOST_CHECK( DowngradeLibraryFilesInPlace( dir, wxT( "*.fake_mod" ), 123, noForbidden, convertOk ).IsEmpty() );

    wxFFile  check( dir + wxFileName::GetPathSeparator() + wxT( "a.fake_mod" ), wxT( "rb" ) );
    wxString content;
    BOOST_REQUIRE( check.IsOpened() && check.ReadAll( &content ) );
    BOOST_CHECK( content.Contains( wxT( "converted" ) ) );

    // Second pass: the second file refuses, so the first must stay as it is now.
    auto convertRefuseB = [&]( const wxString& aFile, const wxString& aTmp ) -> DOWNGRADE_FILE_RESULT
    {
        if( aFile.Contains( wxT( "b.fake_mod" ) ) )
            return DOWNGRADE_FILE_RESULT::REFUSED;

        wxFFile out( aTmp, wxT( "wb" ) );
        out.Write( wxT( "(version 123) second pass" ) );
        return DOWNGRADE_FILE_RESULT::CONVERTED;
    };

    wxString bad = DowngradeLibraryFilesInPlace( dir, wxT( "*.fake_mod" ), 123, noForbidden, convertRefuseB );
    BOOST_CHECK( bad.Contains( wxT( "b.fake_mod" ) ) );

    wxArrayString leftovers;
    wxDir::GetAllFiles( dir, &leftovers );
    BOOST_CHECK_EQUAL( leftovers.size(), 2 );

    wxFFile  recheck( dir + wxFileName::GetPathSeparator() + wxT( "a.fake_mod" ), wxT( "rb" ) );
    wxString unchanged;
    BOOST_REQUIRE( recheck.IsOpened() && recheck.ReadAll( &unchanged ) );
    BOOST_CHECK( !unchanged.Contains( wxT( "second pass" ) ) );
}


// The project export carries design files only. UI state and non-KiCad files stay behind.
BOOST_AUTO_TEST_CASE( DesignFileFilter )
{
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/proj.kicad_pro" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/sub/sheet.kicad_sch" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/proj.kicad_pcb" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/proj.kicad_dru" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/frame.kicad_wks" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/lib.kicad_sym" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/lib.pretty/fp.kicad_mod" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/sym-lib-table" ) ) );
    BOOST_CHECK( IsDowngradeDesignFile( wxT( "proj/fp-lib-table" ) ) );

    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/proj.kicad_prl" ) ) );
    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/datasheet.pdf" ) ) );
    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/out/proj-F_Cu.gbr" ) ) );
    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/proj.kicad_jobset" ) ) );
    BOOST_CHECK( !IsDowngradeDesignFile( wxT( "proj/notes.txt" ) ) );
}


// When this fails a format bump happened. Review the new keywords, update the denylist, then bump
// BoardDowngradeCoveredVersion.
BOOST_AUTO_TEST_CASE( DenylistCoversCurrentFormat )
{
    BOOST_CHECK_EQUAL( SEXPR_BOARD_FILE_VERSION, BoardDowngradeCoveredVersion() );
}


// Every graphical LOWER rule has an omission path, both on boards and in footprints.
BOOST_AUTO_TEST_CASE( DropApproximationsOmitsGraphicsWithoutReplacements )
{
    for( bool footprintOnly : { false, true } )
    {
        BOARD      board;
        FOOTPRINT* fp = new FOOTPRINT( &board );
        board.Add( fp );
        BOARD_ITEM_CONTAINER* parent = footprintOnly ? static_cast<BOARD_ITEM_CONTAINER*>( fp ) : &board;
        PCB_GROUP*            group = new PCB_GROUP( parent );
        parent->Add( group );

        for( SHAPE_T type : { SHAPE_T::ELLIPSE, SHAPE_T::ELLIPSE_ARC, SHAPE_T::RECTANGLE } )
        {
            PCB_SHAPE* shape = new PCB_SHAPE( parent, type );
            if( type == SHAPE_T::RECTANGLE )
            {
                shape->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 6 ) ) );
                shape->SetCornerRadius( pcbIUScale.mmToIU( 1 ) );
            }
            parent->Add( shape );
            group->AddItem( shape );
        }

        PCB_BARCODE* barcode = new PCB_BARCODE( parent );
        parent->Add( barcode );
        group->AddItem( barcode );
        PCB_TEXTBOX* textbox = new PCB_TEXTBOX( parent );
        textbox->SetIsKnockout( true );
        textbox->SetText( wxT( "Do not retain as plain text" ) );
        parent->Add( textbox );
        group->AddItem( textbox );

        PCB_SHAPE* supported = new PCB_SHAPE( parent, SHAPE_T::CIRCLE );
        parent->Add( supported );
        group->AddItem( supported );
        const KIID              supportedUuid = supported->m_Uuid;
        const DOWNGRADE_TARGET& target = kicad9;
        auto                    normal = ClassifyBoardForDowngrade( &board, target );
        auto                    omit = ClassifyBoardForDowngrade( &board, target, true );
        BOOST_CHECK_EQUAL( normal.Count( DOWNGRADE_BUCKET::LOWER ), 5 );
        BOOST_CHECK_EQUAL( omit.Count( DOWNGRADE_BUCKET::LOWER ), 0 );
        BOOST_CHECK_EQUAL( omit.Count( DOWNGRADE_BUCKET::DROP ), 5 );
        BOOST_REQUIRE_EQUAL( normal.Entries().size(), omit.Entries().size() );
        for( size_t i = 0; i < normal.Entries().size(); ++i )
        {
            BOOST_CHECK( normal.Entries()[i].m_feature == omit.Entries()[i].m_feature );
            BOOST_CHECK_EQUAL( normal.Entries()[i].m_count, omit.Entries()[i].m_count );
            BOOST_CHECK( normal.Entries()[i].m_detail != omit.Entries()[i].m_detail );
            BOOST_CHECK( !omit.Entries()[i].m_detail.IsEmpty() );
        }

        // Classification is read-only.
        BOOST_CHECK_EQUAL( group->GetItems().size(), 6 );
        if( footprintOnly )
            DowngradeFootprintInPlace( fp, target, true );
        else
            DowngradeBoardInPlace( &board, target, true );
        BOOST_REQUIRE_EQUAL( group->GetItems().size(), 1 );
        BOOST_CHECK( *group->GetItems().begin() == supported );
        BOOST_CHECK( supported->m_Uuid == supportedUuid );
        BOOST_CHECK( !ClassifyBoardForDowngrade( &board, target, true ).IsLossy() );
    }
}


BOOST_AUTO_TEST_CASE( DropApproximationsOmitsFillAndCellContentsOnly )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    board.Add( fp );
    for( BOARD_ITEM_CONTAINER* parent :
         { static_cast<BOARD_ITEM_CONTAINER*>( &board ), static_cast<BOARD_ITEM_CONTAINER*>( fp ) } )
    {
        PCB_SHAPE* hatch = new PCB_SHAPE( parent, SHAPE_T::RECTANGLE );
        hatch->SetLayer( F_SilkS );
        hatch->SetFillMode( FILL_T::HATCH );
        parent->Add( hatch );
        PCB_TABLE* table = new PCB_TABLE( parent, pcbIUScale.mmToIU( 0.1 ) );
        table->SetColCount( 2 );
        PCB_TABLECELL* knockout = new PCB_TABLECELL( table );
        knockout->SetIsKnockout( true );
        knockout->SetText( wxT( "knockout" ) );
        table->AddCell( knockout );
        PCB_TABLECELL* plain = new PCB_TABLECELL( table );
        plain->SetText( wxT( "supported" ) );
        table->AddCell( plain );
        parent->Add( table );

        if( parent == fp )
            DowngradeFootprintInPlace( fp, kicad9, true );
        else
            DowngradeBoardInPlace( &board, kicad9, true );
        BOOST_CHECK( hatch->GetFillMode() == FILL_T::NO_FILL );
        BOOST_CHECK( hatch->GetShape() == SHAPE_T::RECTANGLE );
        BOOST_REQUIRE_EQUAL( table->GetCells().size(), 2 );
        BOOST_CHECK( knockout->GetText().IsEmpty() );
        BOOST_CHECK( !knockout->IsKnockout() );
        BOOST_CHECK( plain->GetText() == wxT( "supported" ) );
    }
}


// Removing a hatch fill on a copper layer would delete copper, and the omission preference is
// not an electrical override. Copper keeps a solid fill under both policies. Other layers carry
// ink, so there the fill may go.
// The report must name the copper case separately. One combined row would label the copper
// approximation a drop as soon as the omission preference is set, and hide how many of the
// counted shapes are copper.
BOOST_AUTO_TEST_CASE( HatchedCopperAndInkFillsAreReportedSeparately )
{
    BOARD board;

    for( int ii = 0; ii < 3; ++ii )
    {
        PCB_SHAPE* ink = new PCB_SHAPE( &board, SHAPE_T::RECTANGLE );
        ink->SetLayer( F_SilkS );
        ink->SetFillMode( FILL_T::HATCH );
        board.Add( ink );
    }

    PCB_SHAPE* copper = new PCB_SHAPE( &board, SHAPE_T::RECTANGLE );
    copper->SetLayer( F_Cu );
    copper->SetFillMode( FILL_T::HATCH );
    board.Add( copper );

    for( bool drop : { false, true } )
    {
        BOOST_TEST_CONTEXT( "Drop approximations: " << drop )
        {
            COMPATIBILITY_REPORT report = ClassifyBoardForDowngrade( &board, kicad9, drop );

            BOOST_CHECK_EQUAL( report.Entries().size(), 2 );
            BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::LOWER ), drop ? 1 : 4 );
            BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), drop ? 3 : 0 );
        }
    }
}


BOOST_AUTO_TEST_CASE( DropApproximationsKeepsHatchedCopperAsSolidFill )
{
    for( bool drop : { false, true } )
    {
        BOOST_TEST_CONTEXT( "Drop approximations: " << drop )
        {
            BOARD      board;
            FOOTPRINT* fp = new FOOTPRINT( &board );
            board.Add( fp );

            std::vector<PCB_SHAPE*> copper;
            std::vector<PCB_SHAPE*> silk;

            for( BOARD_ITEM_CONTAINER* parent : { static_cast<BOARD_ITEM_CONTAINER*>( &board ),
                                                  static_cast<BOARD_ITEM_CONTAINER*>( fp ) } )
            {
                PCB_SHAPE* onCopper = new PCB_SHAPE( parent, SHAPE_T::RECTANGLE );
                onCopper->SetLayer( F_Cu );
                onCopper->SetFillMode( FILL_T::HATCH );
                parent->Add( onCopper );
                copper.push_back( onCopper );

                PCB_SHAPE* onSilk = new PCB_SHAPE( parent, SHAPE_T::RECTANGLE );
                onSilk->SetLayer( F_SilkS );
                onSilk->SetFillMode( FILL_T::HATCH );
                parent->Add( onSilk );
                silk.push_back( onSilk );
            }

            DowngradeBoardInPlace( &board, kicad9, drop );

            for( PCB_SHAPE* shape : copper )
                BOOST_CHECK( shape->GetFillMode() == FILL_T::FILLED_SHAPE );

            for( PCB_SHAPE* shape : silk )
                BOOST_CHECK( shape->GetFillMode() == ( drop ? FILL_T::NO_FILL : FILL_T::FILLED_SHAPE ) );
        }
    }
}


BOOST_AUTO_TEST_CASE( DropApproximationsOmitsRoundedPadPrimitives )
{
    for( bool footprintOnly : { false, true } )
    {
        BOARD      board;
        FOOTPRINT* fp = new FOOTPRINT( &board );
        board.Add( fp );
        PAD* pad = new PAD( fp );
        fp->Add( pad );
        pad->SetShape( F_Cu, PAD_SHAPE::CUSTOM );
        PCB_SHAPE* rounded = new PCB_SHAPE( fp, SHAPE_T::RECTANGLE );
        rounded->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 6 ) ) );
        rounded->SetCornerRadius( pcbIUScale.mmToIU( 1 ) );
        pad->AddPrimitive( F_Cu, rounded );
        PCB_SHAPE* supported = new PCB_SHAPE( fp, SHAPE_T::CIRCLE );
        pad->AddPrimitive( F_Cu, supported );
        const DOWNGRADE_TARGET& target = kicad9;
        BOOST_CHECK_EQUAL( ClassifyBoardForDowngrade( &board, target, true ).Count( DOWNGRADE_BUCKET::DROP ), 1 );
        if( footprintOnly )
            DowngradeFootprintInPlace( fp, target, true );
        else
            DowngradeBoardInPlace( &board, target, true );
        BOOST_REQUIRE_EQUAL( pad->GetPrimitives( F_Cu ).size(), 1 );
        BOOST_CHECK( pad->GetPrimitives( F_Cu ).front()->GetShape() == SHAPE_T::CIRCLE );
        BOOST_CHECK_EQUAL( fp->Pads().size(), 1 );
    }
}


// Removing a via deletes copper and splits a net, so the omission preference does not reach it.
BOOST_AUTO_TEST_CASE( ViaApproximationsSurviveDropPolicy )
{
    for( bool drop : { false, true } )
    {
        BOOST_TEST_CONTEXT( "Drop approximations: " << drop )
        {
            BOARD    board;
            PCB_VIA* skip = new PCB_VIA( &board );
            skip->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::START_END_ONLY );
            board.Add( skip );
            PCB_VIA* buried = new PCB_VIA( &board );
            buried->SetViaType( VIATYPE::BURIED );
            board.Add( buried );
            PCB_VIA* supported = new PCB_VIA( &board );
            board.Add( supported );

            const auto report = ClassifyBoardForDowngrade( &board, kicad9, drop );

            BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::LOWER ), 2 );
            BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), 0 );

            DowngradeBoardInPlace( &board, kicad9, drop );

            BOOST_REQUIRE_EQUAL( board.Tracks().size(), 3 );
            BOOST_CHECK( skip->Padstack().UnconnectedLayerMode()
                         == UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
            BOOST_CHECK( buried->GetViaType() == VIATYPE::BLIND );
            BOOST_CHECK( supported->GetViaType() == VIATYPE::THROUGH );
        }
    }
}


BOOST_AUTO_TEST_CASE( DropApproximationsKeepsTargetSupportedFeatures )
{
    BOARD      board;
    PCB_SHAPE* rect = new PCB_SHAPE( &board, SHAPE_T::RECTANGLE );
    rect->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 6 ) ) );
    rect->SetCornerRadius( pcbIUScale.mmToIU( 1 ) );
    rect->SetFillMode( FILL_T::HATCH );
    board.Add( rect );
    PCB_TEXTBOX* textbox = new PCB_TEXTBOX( &board );
    textbox->SetIsKnockout( true );
    board.Add( textbox );
    board.Add( new PCB_BARCODE( &board ) );
    PCB_VIA* buried = new PCB_VIA( &board );
    buried->SetViaType( VIATYPE::BURIED );
    board.Add( buried );
    PCB_VIA* skip = new PCB_VIA( &board );
    skip->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::START_END_ONLY );
    board.Add( skip );
    const auto& target = kicad10;
    BOOST_CHECK( !ClassifyBoardForDowngrade( &board, target, true ).IsLossy() );
    DowngradeBoardInPlace( &board, target, true );
    BOOST_CHECK_EQUAL( board.Drawings().size(), 3 );
    BOOST_CHECK_EQUAL( board.Tracks().size(), 2 );
    BOOST_CHECK_EQUAL( rect->GetCornerRadius(), pcbIUScale.mmToIU( 1 ) );
    BOOST_CHECK( rect->GetFillMode() == FILL_T::HATCH );
    BOOST_CHECK( textbox->IsKnockout() );
    BOOST_CHECK( buried->GetViaType() == VIATYPE::BURIED );
    BOOST_CHECK( skip->Padstack().UnconnectedLayerMode() == UNCONNECTED_LAYER_MODE::START_END_ONLY );
}


BOOST_AUTO_TEST_CASE( DropApproximationsCannotBypassBlockRules )
{
    BOARD board;
    ZONE* zone = new ZONE( &board );
    zone->SetFillMode( ZONE_FILL_MODE::COPPER_THIEVING );
    board.Add( zone );
    BOOST_CHECK( ClassifyBoardForDowngrade( &board, kicad10, true ).IsBlocked() );
}


BOOST_AUTO_TEST_CASE( LibraryReplacementsWaitForTheDesignTransaction )
{
    KI_TEST::TEMPORARY_DIRECTORY temporary( "kicad_qa_shared_downgrade_transaction" );
    const wxString               directory = temporary.GetPath().wstring();
    const wxString               library = directory + wxT( "/library.fake_mod" );
    const wxString               board = directory + wxT( "/board.kicad_pcb" );

    auto write = []( const wxString& aPath, const wxString& aContent )
    {
        wxFFile file( aPath, wxT( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() && file.Write( aContent ) && file.Close() );
    };

    auto read = []( const wxString& aPath )
    {
        wxFFile  file( aPath, wxT( "rb" ) );
        wxString content;
        BOOST_REQUIRE( file.IsOpened() && file.ReadAll( &content ) );
        return content;
    };

    auto noForbidden = []( const wxString& )
    {
        return wxString();
    };
    auto convert = [&]( const wxString&, const wxString& aTemporary )
    {
        write( aTemporary, wxT( "(version 123) converted" ) );
        return DOWNGRADE_FILE_RESULT::CONVERTED;
    };

    write( library, wxT( "original library" ) );
    write( board, wxT( "original board" ) );

    {
        DOWNGRADE_FILE_TRANSACTION transaction;
        BOOST_REQUIRE(
                DowngradeLibraryFilesInPlace( directory, wxT( "*.fake_mod" ), 123, noForbidden, convert, &transaction )
                        .IsEmpty() );
        BOOST_CHECK_EQUAL( read( library ), wxString( wxT( "original library" ) ) );
        BOOST_CHECK( transaction.Stage( directory + wxT( "/missing/board.kicad_pcb" ) ).IsEmpty() );
    }

    BOOST_CHECK_EQUAL( read( library ), wxString( wxT( "original library" ) ) );
    BOOST_CHECK_EQUAL( read( board ), wxString( wxT( "original board" ) ) );

    {
        DOWNGRADE_FILE_TRANSACTION transaction;
        BOOST_REQUIRE(
                DowngradeLibraryFilesInPlace( directory, wxT( "*.fake_mod" ), 123, noForbidden, convert, &transaction )
                        .IsEmpty() );
        wxString stagedBoard = transaction.Stage( board );
        BOOST_REQUIRE( !stagedBoard.IsEmpty() );
        write( stagedBoard, wxT( "converted board" ) );
        BOOST_REQUIRE( transaction.Commit().IsEmpty() );
    }

    BOOST_CHECK_EQUAL( read( library ), wxString( wxT( "(version 123) converted" ) ) );
    BOOST_CHECK_EQUAL( read( board ), wxString( wxT( "converted board" ) ) );
    wxArrayString files;
    wxDir::GetAllFiles( directory, &files );
    BOOST_CHECK_EQUAL( files.size(), 2 );
}


BOOST_AUTO_TEST_CASE( DirectoryDestinationStopsAllTransactionReplacements )
{
    KI_TEST::TEMPORARY_DIRECTORY temporary( "kicad_qa_downgrade_transaction_directory" );
    const wxString               directory = temporary.GetPath().wstring();
    const wxString               first = directory + wxT( "/first.kicad_pcb" );
    const wxString               second = directory + wxT( "/second.kicad_pcb" );

    {
        wxFFile file( first, wxT( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() && file.Write( wxT( "original" ) ) && file.Close() );
    }

    {
        DOWNGRADE_FILE_TRANSACTION transaction;
        wxString                   stagedFirst = transaction.Stage( first );
        wxString                   stagedSecond = transaction.Stage( second );
        BOOST_REQUIRE( !stagedFirst.IsEmpty() && !stagedSecond.IsEmpty() );

        for( const wxString& file : { stagedFirst, stagedSecond } )
        {
            wxFFile output( file, wxT( "wb" ) );
            BOOST_REQUIRE( output.IsOpened() && output.Write( wxT( "converted" ) ) && output.Close() );
        }

        BOOST_REQUIRE( wxMkdir( second ) );
        BOOST_CHECK_EQUAL( transaction.Commit(), second );
        BOOST_CHECK( transaction.GetRecoveryError().IsEmpty() );
    }

    wxFFile  input( first, wxT( "rb" ) );
    wxString content;
    BOOST_REQUIRE( input.IsOpened() && input.ReadAll( &content ) );
    BOOST_CHECK_EQUAL( content, wxString( wxT( "original" ) ) );
    BOOST_CHECK( wxDirExists( second ) );
    wxArrayString files;
    wxDir::GetAllFiles( directory, &files );
    BOOST_CHECK_EQUAL( files.size(), 1 );
}


BOOST_AUTO_TEST_CASE( ConcurrentTransactionsUseDifferentOwnedStagingFiles )
{
    KI_TEST::TEMPORARY_DIRECTORY temporary( "kicad_qa_downgrade_transaction_unique" );
    const wxString               destination = temporary.GetPath().wstring() + wxT( "/board.kicad_pcb" );

    {
        DOWNGRADE_FILE_TRANSACTION first;
        DOWNGRADE_FILE_TRANSACTION second;
        wxString                   firstTemporary = first.Stage( destination );
        wxString                   secondTemporary = second.Stage( destination );
        BOOST_REQUIRE( !firstTemporary.IsEmpty() && !secondTemporary.IsEmpty() );
        BOOST_CHECK( firstTemporary != secondTemporary );
        BOOST_CHECK( wxFileExists( firstTemporary ) );
        BOOST_CHECK( wxFileExists( secondTemporary ) );
        first.Discard( firstTemporary );
        BOOST_CHECK( !wxFileExists( firstTemporary ) );
        BOOST_CHECK( wxFileExists( secondTemporary ) );
        BOOST_CHECK( !wxFileExists( destination ) );
    }

    BOOST_CHECK( std::filesystem::is_empty( temporary.GetPath() ) );
}


BOOST_AUTO_TEST_CASE( TransactionSymlinkReplacementLeavesLinkedSourcesUnchanged )
{
    KI_TEST::TEMPORARY_DIRECTORY temporary( "kicad_qa_downgrade_transaction_symlink" );
    const auto                   original = temporary.GetPath() / u8"original-μ.kicad_mod";
    const auto                   alias = temporary.GetPath() / "alias.kicad_mod";
    const auto                   danglingAlias = temporary.GetPath() / "dangling.kicad_mod";

    {
        wxFFile file( original.wstring(), wxT( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() && file.Write( wxT( "original source" ) ) && file.Close() );
    }

    std::error_code error;
    std::filesystem::create_symlink( original.filename(), alias, error );

    if( error )
    {
        BOOST_TEST_MESSAGE( "Symlink creation is unavailable on this system" );
        return;
    }

    std::filesystem::create_symlink( "missing.kicad_mod", danglingAlias, error );
    BOOST_REQUIRE( !error );

    {
        DOWNGRADE_FILE_TRANSACTION transaction;

        for( const auto& destination : { alias, danglingAlias } )
        {
            wxString staged = transaction.Stage( destination.wstring() );
            BOOST_REQUIRE( !staged.IsEmpty() );
            wxFFile file( staged, wxT( "wb" ) );
            BOOST_REQUIRE( file.IsOpened() && file.Write( wxT( "converted output" ) ) && file.Close() );
        }

        BOOST_REQUIRE( transaction.Commit().IsEmpty() );
    }

    wxFFile  file( original.wstring(), wxT( "rb" ) );
    wxString content;
    BOOST_REQUIRE( file.IsOpened() && file.ReadAll( &content ) );
    BOOST_CHECK_EQUAL( content, wxString( wxT( "original source" ) ) );
    BOOST_CHECK( !std::filesystem::is_symlink( alias ) );
    BOOST_CHECK( !std::filesystem::is_symlink( danglingAlias ) );
    BOOST_CHECK_EQUAL( std::distance( std::filesystem::directory_iterator( temporary.GetPath() ),
                                      std::filesystem::directory_iterator() ),
                       3 );
}


BOOST_AUTO_TEST_SUITE_END()
