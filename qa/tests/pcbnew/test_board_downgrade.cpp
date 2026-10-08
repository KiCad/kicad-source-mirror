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

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <memory>
#include <filesystem>
#include <set>

#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/mstream.h>

#include <base_units.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <font/font.h>
#include <gr_text.h>
#include <pcb_track.h>
#include <generators/pcb_tuning_pattern.h>
#include <generators/pcb_via_stack.h>
#include <generators/pcb_via_stitch.h>
#include <constraints/pcb_constraint.h>
#include <pad.h>
#include <padstack.h>
#include <pcb_barcode.h>
#include <pcb_group.h>
#include <pcb_dimension.h>
#include <pcb_drill_chart.h>
#include <pcb_drill_map.h>
#include <pcb_grid_item.h>
#include <pcb_reference_image.h>
#include <pcb_shape.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_textbox.h>
#include <board_stackup_manager/board_stackup.h>
#include <zone.h>
#include <zone_settings.h>
#include <nlohmann/json.hpp>
#include <netinfo.h>
#include <project.h>
#include <ki_exception.h>

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
    const std::string      original = KI_TEST::ReadGoldenText( src );
    std::unique_ptr<BOARD> current( io.LoadBoard( src, nullptr ) );
    BOOST_REQUIRE( current );
    const auto& loadedDefaults = current->GetDesignSettings().m_ZoneLayerProperties;
    BOOST_REQUIRE_EQUAL( loadedDefaults.size(), 2 );
    BOOST_CHECK( loadedDefaults.at( F_Cu ).hatching_offset == VECTOR2I( 100000, 200000 ) );
    BOOST_CHECK( loadedDefaults.at( B_Cu ).hatching_offset == VECTOR2I( 300000, 400000 ) );
    BOOST_CHECK( current->GetDesignSettings().GetDefaultZoneSettings().m_LayerProperties.empty() );

    for( const auto& target : { kicad9, kicad10 } )
    {
        COMPATIBILITY_REPORT report;
        const std::string    dest = ( tmp.GetPath() / ( target.m_id.ToStdString() + ".kicad_pcb" ) ).string();
        BOOST_REQUIRE( ExportBoardToOlderVersion( src, dest, target, report ) );
        std::unique_ptr<BOARD> reloaded( io.LoadBoard( dest, nullptr ) );
        BOOST_REQUIRE( reloaded );
        const auto& defaults = reloaded->GetDesignSettings().m_ZoneLayerProperties;

        if( target.m_id == wxT( "9.0" ) )
        {
            BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), 1 );
            BOOST_CHECK( defaults.empty() );
            BOOST_CHECK( KI_TEST::ReadGoldenText( dest ).find( "zone_defaults" ) == std::string::npos );
        }
        else
        {
            BOOST_CHECK( !report.IsLossy() );
            BOOST_REQUIRE_EQUAL( defaults.size(), 2 );
            BOOST_CHECK( defaults.at( F_Cu ).hatching_offset == loadedDefaults.at( F_Cu ).hatching_offset );
            BOOST_CHECK( defaults.at( B_Cu ).hatching_offset == loadedDefaults.at( B_Cu ).hatching_offset );
        }

        BOOST_CHECK( KI_TEST::ReadGoldenText( src ) == original );
    }
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


// The headline fail-closed promise: a blocked board export returns failure and writes nothing.
// The staging name must not be guessable, because the writer resolves symlinks on purpose.
BOOST_AUTO_TEST_CASE( BoardExportIgnoresAPredictableStagingSymlink )
{
    SETTINGS_MANAGER settingsManager;

    const std::string src = KI_TEST::GetPcbnewTestDataDir() + "via_off_center.kicad_pcb";

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_staging_symlink", "" );
    const wxString               dest = ( tmp.GetPath() / "out.kicad_pcb" ).wstring();
    const wxString               victim = ( tmp.GetPath() / "victim.txt" ).wstring();

    {
        wxFFile file( victim, wxT( "wb" ) );
        BOOST_REQUIRE( file.IsOpened() );
        file.Write( wxT( "untouched" ) );
    }

    const wxString decoy =
            dest + wxString::Format( wxT( ".downgrade_tmp.%lu" ), (unsigned long) wxGetProcessId() );
    std::error_code ec;
    std::filesystem::create_symlink( victim.ToStdString(), decoy.ToStdString(), ec );
    BOOST_REQUIRE( !ec );

    COMPATIBILITY_REPORT report;
    BOOST_CHECK( ExportBoardToOlderVersion( src, dest, kicad9, report ) );

    wxString after;
    {
        wxFFile file( victim, wxT( "rb" ) );
        BOOST_REQUIRE( file.IsOpened() && file.ReadAll( &after ) );
    }

    BOOST_CHECK( after == wxT( "untouched" ) );

    // A normal save in the same directory shares this umask, so the modes must agree.
    const wxString reference = ( tmp.GetPath() / "reference.kicad_pcb" ).wstring();
    PCB_IO_KICAD_SEXPR     io;
    std::unique_ptr<BOARD> board( io.LoadBoard( dest, nullptr ) );
    BOOST_REQUIRE( board );
    io.SaveBoard( reference, *board );

    BOOST_CHECK( std::filesystem::status( dest.ToStdString() ).permissions()
                 == std::filesystem::status( reference.ToStdString() ).permissions() );
}


BOOST_AUTO_TEST_CASE( BlockedBoardExportWritesNothing )
{
    SETTINGS_MANAGER settingsManager;

    BOARD board;
    ZONE* zone = new ZONE( &board );
    zone->SetLayer( F_Cu );
    zone->AppendCorner( VECTOR2I( 0, 0 ), -1 );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ), -1 );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ), -1 );
    zone->SetFillMode( ZONE_FILL_MODE::COPPER_THIEVING );
    board.Add( zone );

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_blocked_export", "" );
    const wxString               src = ( tmp.GetPath() / "src.kicad_pcb" ).wstring();
    const wxString               dest = ( tmp.GetPath() / "out.kicad_pcb" ).wstring();

    PCB_IO_KICAD_SEXPR io;
    io.SaveBoard( src, board );

    COMPATIBILITY_REPORT report;
    BOOST_CHECK( !ExportBoardToOlderVersion( src, dest, kicad10, report ) );
    BOOST_CHECK( report.IsBlocked() );
    BOOST_CHECK( !wxFileExists( dest ) );
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


// The v10 writer must not move anything. Every footprint and pad position in the export must match
// the source exactly. This guards the extracted writer against silent model drift.
BOOST_AUTO_TEST_CASE( ExportToKicad10PreservesGeometry )
{
    SETTINGS_MANAGER settingsManager;

    const std::string src = KI_TEST::GetPcbnewTestDataDir() + "via_off_center.kicad_pcb";

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_downgrade_geom", "" );
    const std::string            dest = ( tmp.GetPath() / "out.kicad_pcb" ).string();

    COMPATIBILITY_REPORT report;
    BOOST_REQUIRE( ExportBoardToOlderVersion( src, dest, kicad10, report ) );

    PCB_IO_KICAD_SEXPR     io;
    std::unique_ptr<BOARD> source( io.LoadBoard( src, nullptr ) );
    std::unique_ptr<BOARD> exported( io.LoadBoard( dest, nullptr ) );

    BOOST_REQUIRE( source && exported );
    BOOST_REQUIRE_EQUAL( source->Footprints().size(), exported->Footprints().size() );

    for( size_t i = 0; i < source->Footprints().size(); i++ )
    {
        FOOTPRINT* a = source->Footprints()[i];
        FOOTPRINT* b = exported->Footprints()[i];

        BOOST_CHECK_EQUAL( a->GetReference(), b->GetReference() );
        BOOST_CHECK( a->GetPosition() == b->GetPosition() );
        BOOST_CHECK( a->GetOrientation() == b->GetOrientation() );

        BOOST_REQUIRE_EQUAL( a->Pads().size(), b->Pads().size() );

        for( size_t p = 0; p < a->Pads().size(); p++ )
            BOOST_CHECK( a->Pads()[p]->GetPosition() == b->Pads()[p]->GetPosition() );
    }
}


// Same geometry guard for the v9 writer.
BOOST_AUTO_TEST_CASE( ExportToKicad9PreservesGeometry )
{
    SETTINGS_MANAGER settingsManager;

    const std::string src = KI_TEST::GetPcbnewTestDataDir() + "via_off_center.kicad_pcb";

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_downgrade_geom9", "" );
    const std::string            dest = ( tmp.GetPath() / "out.kicad_pcb" ).string();

    COMPATIBILITY_REPORT report;
    BOOST_REQUIRE( ExportBoardToOlderVersion( src, dest, kicad9, report ) );

    PCB_IO_KICAD_SEXPR     io;
    std::unique_ptr<BOARD> source( io.LoadBoard( src, nullptr ) );
    std::unique_ptr<BOARD> exported( io.LoadBoard( dest, nullptr ) );

    BOOST_REQUIRE( source && exported );
    BOOST_REQUIRE_EQUAL( source->Footprints().size(), exported->Footprints().size() );

    for( size_t i = 0; i < source->Footprints().size(); i++ )
    {
        FOOTPRINT* a = source->Footprints()[i];
        FOOTPRINT* b = exported->Footprints()[i];

        BOOST_CHECK( a->GetPosition() == b->GetPosition() );

        BOOST_REQUIRE_EQUAL( a->Pads().size(), b->Pads().size() );

        for( size_t pd = 0; pd < a->Pads().size(); pd++ )
            BOOST_CHECK( a->Pads()[pd]->GetPosition() == b->Pads()[pd]->GetPosition() );
    }
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


BOOST_AUTO_TEST_CASE( BarcodeProjectVariablesResolvedBeforeExport )
{
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_barcode_project" );
    wxString                     projectPath = ( tmp.GetPath() / "source.kicad_pro" ).string();
    wxString                     boardPath = ( tmp.GetPath() / "source.kicad_pcb" ).string();
    wxString                     outputPath = ( tmp.GetPath() / "output.kicad_pcb" ).string();

    {
        std::ofstream project( projectPath.ToStdString() );
        project << R"({"meta":{"version":3},"text_variables":{"SERIAL":"SERIAL123"}})";
    }

    SETTINGS_MANAGER settings;
    BOOST_REQUIRE( settings.LoadProject( projectPath, false ) );
    BOARD board;
    board.SetProject( settings.GetProject( projectPath ), true );
    PCB_BARCODE* barcode = new PCB_BARCODE( &board );
    barcode->SetLayer( F_SilkS );
    barcode->SetBarcodeKind( BARCODE_T::QR_CODE );
    barcode->SetWidth( pcbIUScale.mmToIU( 10 ) );
    barcode->SetHeight( pcbIUScale.mmToIU( 10 ) );
    barcode->SetBarcodeText( wxT( "${SERIAL}" ) );
    barcode->SetShowText( false );
    board.Add( barcode );
    BOOST_REQUIRE_EQUAL( barcode->GetShownText( FOR_CANVAS ), wxString( "SERIAL123" ) );

    SHAPE_POLY_SET expected;
    barcode->TransformShapeToPolygon( expected, F_SilkS, 0, 0, ERROR_INSIDE );
    expected.Simplify();
    expected.Fracture();
    double expectedArea = expected.Area();

    PCB_IO_KICAD_SEXPR io;
    io.SaveBoard( boardPath, board );
    COMPATIBILITY_REPORT report;
    BOOST_REQUIRE( ExportBoardToOlderVersion( boardPath, outputPath, kicad9, report ) );
    std::unique_ptr<BOARD> result( io.LoadBoard( outputPath, nullptr ) );
    BOOST_REQUIRE( result );
    int    polygonCount = 0;
    double resultArea = 0;

    for( BOARD_ITEM* item : result->Drawings() )
    {
        BOOST_REQUIRE( item->Type() != PCB_BARCODE_T );

        if( item->Type() == PCB_SHAPE_T )
        {
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );
            BOOST_REQUIRE( shape->GetShape() == SHAPE_T::POLY );
            polygonCount++;
            resultArea += std::abs( shape->GetPolyShape().Area() );
        }
    }

    BOOST_CHECK_EQUAL( polygonCount, expected.OutlineCount() );
    BOOST_CHECK_CLOSE( resultArea, expectedArea, 0.01 );
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


// Exercise the real .kicad_mod write path used to downgrade a footprint library, and confirm the
// result carries the target stamp and reloads cleanly.
BOOST_AUTO_TEST_CASE( FootprintFileDowngradeRoundTrips )
{
    const std::string src = KI_TEST::GetPcbnewTestDataDir() + "issue24045/QFN-24_L4.0-W4.0-P0.50-BL-EP2.6.kicad_mod";

    PCB_IO_KICAD_SEXPR         loader;
    wxString                   nameOut;
    std::unique_ptr<FOOTPRINT> fp( loader.ImportFootprint( src, nameOut ) );
    BOOST_REQUIRE( fp != nullptr );

    DowngradeFootprintInPlace( fp.get(), kicad9 );

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_fp_downgrade", "" );
    const wxString               dest = ( tmp.GetPath() / "out.kicad_mod" ).wstring();

    SaveFootprintForTarget( fp.get(), dest, kicad9 );

    // The stamp proves the dispatch picked the right writer, not just a parseable one.
    std::ifstream out( dest.ToStdString() );
    std::string   content( ( std::istreambuf_iterator<char>( out ) ), std::istreambuf_iterator<char>() );
    BOOST_CHECK( content.find( "(version " + std::to_string( kicad9.m_boardVersion ) + ")" ) != std::string::npos );

    PCB_IO_KICAD_SEXPR         reader;
    wxString                   reloadName;
    std::unique_ptr<FOOTPRINT> reloaded( reader.ImportFootprint( dest, reloadName ) );
    BOOST_CHECK( reloaded != nullptr );
}


// The v10 footprint file path gets the same round-trip check as v9.
BOOST_AUTO_TEST_CASE( FootprintFileV10RoundTrips )
{
    SETTINGS_MANAGER settingsManager;

    const std::string src = KI_TEST::GetPcbnewTestDataDir() + "issue24045/QFN-24_L4.0-W4.0-P0.50-BL-EP2.6.kicad_mod";

    PCB_IO_KICAD_SEXPR         loader;
    wxString                   nameOut;
    std::unique_ptr<FOOTPRINT> fp( loader.ImportFootprint( src, nameOut ) );
    BOOST_REQUIRE( fp != nullptr );

    DowngradeFootprintInPlace( fp.get(), kicad10 );

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_fp_downgrade10", "" );
    const wxString               dest = ( tmp.GetPath() / "out.kicad_mod" ).wstring();

    SaveFootprintForTarget( fp.get(), dest, kicad10 );

    std::ifstream out( dest.ToStdString() );
    std::string   content( ( std::istreambuf_iterator<char>( out ) ), std::istreambuf_iterator<char>() );
    BOOST_CHECK( content.find( "(version " + std::to_string( kicad10.m_boardVersion ) + ")" ) != std::string::npos );

    PCB_IO_KICAD_SEXPR         reader;
    wxString                   reloadName;
    std::unique_ptr<FOOTPRINT> reloaded( reader.ImportFootprint( dest, reloadName ) );
    BOOST_CHECK( reloaded != nullptr );
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


// Saving with a target version writes the older stamp, and reloading reads it back.
BOOST_AUTO_TEST_CASE( SaveWithTargetVersionStampsOlder )
{
    SETTINGS_MANAGER settingsManager;

    BOARD board;

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_downgrade", "" );
    std::string                  dest = ( tmp.GetPath() / "out.kicad_pcb" ).string();

    SaveBoardForTarget( &board, dest, kicad10 );

    PCB_IO_KICAD_SEXPR     reader;
    std::unique_ptr<BOARD> reloaded( reader.LoadBoard( dest, nullptr ) );

    BOOST_REQUIRE( reloaded != nullptr );
    BOOST_CHECK_EQUAL( reloaded->GetFileFormatVersionAtLoad(), kicad10.m_boardVersion );
}


// The fields deque can hold null slots. The writer must skip them like the current one does.
BOOST_AUTO_TEST_CASE( NullFieldSlotDoesNotCrashV9Writer )
{
    SETTINGS_MANAGER settingsManager;

    std::unique_ptr<FOOTPRINT> fp = std::make_unique<FOOTPRINT>( nullptr );
    fp->GetFields().push_back( nullptr );

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_v9_nullfield", "" );
    const wxString               dest = ( tmp.GetPath() / "out.kicad_mod" ).wstring();

    SaveFootprintForTarget( fp.get(), dest, kicad9 );

    BOOST_CHECK( std::filesystem::exists( dest.ToStdString() ) );
}


// The 10.0 release writer skips group members that are no longer on the board, so the v10
// snapshot must too. The 9.0 release writer had no such check and its snapshot stays faithful.
BOOST_AUTO_TEST_CASE( StaleGroupMemberNotWrittenForV10 )
{
    SETTINGS_MANAGER settingsManager;

    BOARD      board;
    PCB_SHAPE* onBoard = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    board.Add( onBoard );

    PCB_SHAPE* stale = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );

    PCB_GROUP* group = new PCB_GROUP( &board );
    group->AddItem( onBoard );
    group->AddItem( stale );
    board.Add( group );

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_v10_stalegroup", "" );
    const std::string            dest = ( tmp.GetPath() / "out.kicad_pcb" ).string();

    SaveBoardForTarget( &board, dest, kicad10 );

    std::ifstream out( dest );
    std::string   content( ( std::istreambuf_iterator<char>( out ) ), std::istreambuf_iterator<char>() );

    BOOST_CHECK( content.find( onBoard->m_Uuid.AsString().ToStdString() ) != std::string::npos );
    BOOST_CHECK( content.find( stale->m_Uuid.AsString().ToStdString() ) == std::string::npos );

    group->RemoveAll();
    delete stale;
}


// A via mode the era writer predates must be written as the nearest era mode, not silently
// fall through to keep-all.
BOOST_AUTO_TEST_CASE( SkipViaModeWrittenForV9 )
{
    SETTINGS_MANAGER settingsManager;

    BOARD    board;
    PCB_VIA* via = new PCB_VIA( &board );
    via->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::START_END_ONLY );
    board.Add( via );

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_v9_skipvia", "" );
    const std::string            dest = ( tmp.GetPath() / "out.kicad_pcb" ).string();

    SaveBoardForTarget( &board, dest, kicad9 );

    std::ifstream out( dest );
    std::string   content( ( std::istreambuf_iterator<char>( out ) ), std::istreambuf_iterator<char>() );

    BOOST_CHECK( content.find( "(remove_unused_layers yes)" ) != std::string::npos );
    BOOST_CHECK( content.find( "(keep_end_layers yes)" ) != std::string::npos );

    PCB_IO_KICAD_SEXPR     reader;
    std::unique_ptr<BOARD> reloaded( reader.LoadBoard( dest, nullptr ) );
    BOOST_REQUIRE( reloaded );
    BOOST_REQUIRE_EQUAL( reloaded->Tracks().size(), 1 );
    BOOST_REQUIRE( reloaded->Tracks().front()->Type() == PCB_VIA_T );
    const PCB_VIA* reloadedVia = static_cast<const PCB_VIA*>( reloaded->Tracks().front() );
    BOOST_CHECK( reloadedVia->Padstack().UnconnectedLayerMode()
                 == UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
}


// The target computes the image PPI the old truncating way, so the stored scale must be
// migrated back or the image renders at the wrong size there.
BOOST_AUTO_TEST_CASE( ImageScaleMigratedForV9 )
{
    SETTINGS_MANAGER settingsManager;

    BOARD                board;
    PCB_REFERENCE_IMAGE* otherImage = new PCB_REFERENCE_IMAGE( &board );
    wxMemoryBuffer       buf = KI_TEST::MakePngWithFractionalPixelsPerCm();
    BOOST_REQUIRE( otherImage->GetReferenceImage().ReadImageFile( buf ) );
    otherImage->GetReferenceImage().SetImageScale( 0.5 );
    board.Add( otherImage );

    PCB_REFERENCE_IMAGE* image = new PCB_REFERENCE_IMAGE( &board );
    REFERENCE_IMAGE&     ref = image->GetReferenceImage();

    BOOST_REQUIRE( ref.ReadImageFile( buf ) );
    ref.SetImageScale( 2.0 );
    board.Add( image );

    int ppi = ref.GetImage().GetPPI();
    int legacyPPI = ref.GetImage().GetLegacyPPI();
    BOOST_REQUIRE( legacyPPI > 0 && ppi != legacyPPI );

    double originalScale = ref.GetImageScale();

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_v9_imgscale", "" );
    const std::string            dest = ( tmp.GetPath() / "out.kicad_pcb" ).string();

    DowngradeBoardInPlace( &board, kicad9 );
    SaveBoardForTarget( &board, dest, kicad9 );

    std::ifstream out( dest );
    std::string   content( ( std::istreambuf_iterator<char>( out ) ), std::istreambuf_iterator<char>() );

    const double written = KI_TEST::SerializedImageScale( content, image->m_Uuid.AsStdString() );
    BOOST_CHECK_CLOSE( written, originalScale * legacyPPI / ppi, 0.1 );
    BOOST_CHECK_CLOSE( KI_TEST::SerializedImageScale( content, otherImage->m_Uuid.AsStdString() ),
                       0.5 * legacyPPI / ppi, 0.1 );
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


BOOST_AUTO_TEST_CASE( EmptyNativeProjectSectionsDoNotRequireConsent )
{
    const nlohmann::json              empty = nlohmann::json::array();
    const std::vector<nlohmann::json> tuningDefaults = {
        nullptr,
        nlohmann::json::object(),
        { { "meta", { { "version", 2 } } } },
        { { "meta", { { "version", 2 } } }, { "tuning_profiles_impedance_geometric", empty } },
    };
    const std::vector<nlohmann::json> componentDefaults = {
        nullptr,
        nlohmann::json::object(),
        { { "meta", { { "version", 0 } } } },
        { { "assignments", empty }, { "sheet_component_classes", nlohmann::json::object() } },
        { { "meta", { { "version", 0 } } },
          { "assignments", empty },
          { "sheet_component_classes", { { "enabled", false } } } },
    };

    for( const auto& target : { kicad9, kicad10 } )
    {
        for( const auto& tuning : tuningDefaults )
        {
            for( const auto& components : componentDefaults )
            {
                nlohmann::json doc = {
                    { "meta", { { "version", 4 } } },
                    { "board", { { "design_settings", { { "meta", { { "version", 3 } } } } } } },
                    { "erc", { { "meta", { { "version", 1 } } }, { "erc_exclusions", empty } } },
                    { "net_settings",
                      { { "meta", { { "version", 5 } } },
                        { "classes", { { { "name", "Default" }, { "tuning_profile", "" } } } } } },
                    { "tuning_profiles", tuning },
                    { "component_class_settings", components },
                };

                COMPATIBILITY_REPORT report;
                DowngradeProjectFileJson( doc, target, report );

                BOOST_CHECK( !report.IsBlocked() );
                BOOST_CHECK( !report.IsLossy() );
                BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), 0 );
                BOOST_CHECK_EQUAL( doc["meta"]["version"].get<int>(), 3 );
                BOOST_CHECK_EQUAL( doc["board"]["design_settings"]["meta"]["version"].get<int>(), 2 );
                BOOST_CHECK_EQUAL( doc["erc"]["meta"]["version"].get<int>(), 0 );
                BOOST_CHECK_EQUAL( doc.contains( "tuning_profiles" ), target.m_id == wxT( "10.0" ) );
                BOOST_CHECK_EQUAL( doc.contains( "component_class_settings" ), target.m_id == wxT( "10.0" ) );

                const nlohmann::json once = doc;
                COMPATIBILITY_REPORT repeated;
                DowngradeProjectFileJson( doc, target, repeated );
                BOOST_CHECK( doc == once );
                BOOST_CHECK( !repeated.IsLossy() );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( SavedProjectTuningAndClassChoicesStillRequireConsent )
{
    const nlohmann::json empty = nlohmann::json::array();
    const nlohmann::json profile = {
        { "profile_name", "Default" },
        { "frequency", 1e9 },
        { "model_solder_mask", false },
        { "net_chain_bridge_prop_delay", 0 },
    };
    const std::vector<nlohmann::json> meaningful = {
        { { "tuning_profiles", { { "tuning_profiles_impedance_geometric", { profile } } } } },
        { { "net_settings", { { "classes", { { { "tuning_profile", "Saved" } } } } } } },
        { { "tuning_profiles", { { "tuning_profiles_impedance_geometric", empty } } },
          { "net_settings", { { "classes", { { { "tuning_profile", "Saved" } } } } } } },
        { { "tuning_profiles", { { "tuning_profiles_impedance_geometric", { profile } } } },
          { "net_settings",
            { { "classes", { { { "tuning_profile", "Default" } }, { { "tuning_profile", "Default" } } } } } } },
        { { "component_class_settings",
            { { "assignments",
                { { { "component_class", "Fast" }, { "conditions_operator", "ALL" }, { "conditions", empty } } } },
              { "sheet_component_classes", { { "enabled", false } } } } } },
        { { "component_class_settings",
            { { "assignments", empty }, { "sheet_component_classes", { { "enabled", true } } } } } },
    };

    for( const auto& source : meaningful )
    {
        for( const auto& target : { kicad9, kicad10 } )
        {
            nlohmann::json       doc = source;
            COMPATIBILITY_REPORT report;
            DowngradeProjectFileJson( doc, target, report );
            BOOST_CHECK( !report.IsBlocked() );
            BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), target.m_id == wxT( "9.0" ) ? 1 : 0 );
            BOOST_CHECK_EQUAL( report.IsLossy(), target.m_id == wxT( "9.0" ) );

            const nlohmann::json once = doc;
            COMPATIBILITY_REPORT repeated;
            DowngradeProjectFileJson( doc, target, repeated );
            BOOST_CHECK( doc == once );
            BOOST_CHECK( !repeated.IsLossy() );
        }
    }
}


BOOST_AUTO_TEST_CASE( FutureComponentClassSchemaBlocksWithoutMutation )
{
    const nlohmann::json source = {
        { "meta", { { "version", 4 } } },
        { "component_class_settings", { { "meta", { { "version", 1 } } } } },
    };

    for( const auto& target : { kicad9, kicad10 } )
    {
        nlohmann::json       doc = source;
        COMPATIBILITY_REPORT report;
        DowngradeProjectFileJson( doc, target, report );
        BOOST_CHECK( report.IsBlocked() );
        BOOST_CHECK( doc == source );
    }
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


// The scale compensation must reach images inside footprints on the board.
BOOST_AUTO_TEST_CASE( FootprintImageScaleCompensatedOnBoard )
{
    BOARD                board;
    FOOTPRINT*           fp = new FOOTPRINT( &board );
    PCB_REFERENCE_IMAGE* image = new PCB_REFERENCE_IMAGE( fp );
    REFERENCE_IMAGE&     ref = image->GetReferenceImage();

    wxMemoryBuffer buf = KI_TEST::MakePngWithFractionalPixelsPerCm();
    BOOST_REQUIRE( ref.ReadImageFile( buf ) );
    ref.SetImageScale( 2.0 );
    fp->Add( image );
    board.Add( fp );

    int ppi = ref.GetImage().GetPPI();
    int legacyPPI = ref.GetImage().GetLegacyPPI();
    BOOST_REQUIRE( legacyPPI > 0 && ppi != legacyPPI );

    DowngradeBoardInPlace( &board, kicad9 );

    BOOST_CHECK_CLOSE( ref.GetImageScale(), 2.0 * legacyPPI / ppi, 0.1 );
}


// A library footprint goes through the footprint-scoped path and gets the same compensation.
BOOST_AUTO_TEST_CASE( FootprintImageScaleCompensatedInLibrary )
{
    std::unique_ptr<FOOTPRINT> fp = std::make_unique<FOOTPRINT>( nullptr );
    PCB_REFERENCE_IMAGE*       image = new PCB_REFERENCE_IMAGE( fp.get() );
    REFERENCE_IMAGE&           ref = image->GetReferenceImage();

    wxMemoryBuffer buf = KI_TEST::MakePngWithFractionalPixelsPerCm();
    BOOST_REQUIRE( ref.ReadImageFile( buf ) );
    ref.SetImageScale( 2.0 );
    fp->Add( image );

    int ppi = ref.GetImage().GetPPI();
    int legacyPPI = ref.GetImage().GetLegacyPPI();
    BOOST_REQUIRE( legacyPPI > 0 && ppi != legacyPPI );

    DowngradeFootprintInPlace( fp.get(), kicad9 );

    BOOST_CHECK_CLOSE( ref.GetImageScale(), 2.0 * legacyPPI / ppi, 0.1 );
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


// Downgrading a library file must not clip zone layers to a scratch board's layer set. The
// source file is its own oracle: the layer set must survive the round trip unchanged.
BOOST_AUTO_TEST_CASE( FootprintFileKeepsZoneLayers )
{
    SETTINGS_MANAGER settingsManager;

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_fp_zone_layers", "" );
    const wxString               src = ( tmp.GetPath() / "src.kicad_mod" ).wstring();
    const wxString               dest = ( tmp.GetPath() / "out.kicad_mod" ).wstring();

    {
        std::unique_ptr<FOOTPRINT> fp = std::make_unique<FOOTPRINT>( nullptr );
        ZONE*                      zone = new ZONE( fp.get() );

        zone->SetLayerSet( LSET::AllCuMask() );
        zone->Outline()->NewOutline();
        zone->Outline()->Append( 0, 0 );
        zone->Outline()->Append( pcbIUScale.mmToIU( 1 ), 0 );
        zone->Outline()->Append( pcbIUScale.mmToIU( 1 ), pcbIUScale.mmToIU( 1 ) );
        zone->Outline()->Append( 0, pcbIUScale.mmToIU( 1 ) );
        fp->Add( zone );

        SaveFootprintForTarget( fp.get(), src, kicad10 );
    }

    BOOST_REQUIRE( DowngradeFootprintFileToTemp( src, dest, kicad9 ) == DOWNGRADE_FILE_RESULT::CONVERTED );

    PCB_IO_KICAD_SEXPR         reader;
    wxString                   nameOut;
    std::unique_ptr<FOOTPRINT> srcAgain( reader.ImportFootprint( src, nameOut ) );
    std::unique_ptr<FOOTPRINT> converted( reader.ImportFootprint( dest, nameOut ) );

    BOOST_REQUIRE( srcAgain && converted );
    BOOST_REQUIRE_EQUAL( srcAgain->Zones().size(), 1 );
    BOOST_REQUIRE_EQUAL( converted->Zones().size(), 1 );

    BOOST_CHECK_EQUAL( converted->Zones()[0]->GetLayerSet().count(), srcAgain->Zones()[0]->GetLayerSet().count() );
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


static void checkFeature( const COMPATIBILITY_REPORT& aReport, const wxString& aName, DOWNGRADE_BUCKET aBucket,
                          int aCount )
{
    for( const COMPAT_ENTRY& entry : aReport.Entries() )
    {
        if( entry.m_feature == aName )
        {
            BOOST_CHECK( entry.m_bucket == aBucket );
            BOOST_CHECK_EQUAL( entry.m_count, aCount );
            BOOST_CHECK( !entry.m_detail.IsEmpty() );
            return;
        }
    }

    BOOST_ERROR( "Missing compatibility feature: " + aName.ToStdString() );
}


BOOST_AUTO_TEST_CASE( GeometricConstraintsDropWithoutMovingBoardOrFootprintGeometry )
{
    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        for( bool drop : { false, true } )
        {
            BOARD      board;
            FOOTPRINT* fp = new FOOTPRINT( &board );
            board.Add( fp );

            std::vector<PCB_SHAPE*> shapes;

            for( BOARD_ITEM_CONTAINER* parent :
                 { static_cast<BOARD_ITEM_CONTAINER*>( &board ), static_cast<BOARD_ITEM_CONTAINER*>( fp ) } )
            {
                PCB_SHAPE* shape = new PCB_SHAPE( parent, SHAPE_T::SEGMENT );
                shape->SetStart( VECTOR2I( 1000000, 2000000 ) );
                shape->SetEnd( VECTOR2I( 7000000, 2000000 ) );
                parent->Add( shape );
                shapes.push_back( shape );
                PCB_CONSTRAINT* constraint = new PCB_CONSTRAINT( parent, PCB_CONSTRAINT_TYPE::HORIZONTAL );
                constraint->AddMember( shape->m_Uuid );
                parent->Add( constraint );
            }

            const auto report = ClassifyBoardForDowngrade( &board, target, drop );
            BOOST_CHECK( !report.IsBlocked() );
            checkFeature( report, wxT( "Geometric constraints" ), DOWNGRADE_BUCKET::DROP, 2 );
            BOOST_CHECK_EQUAL( board.Constraints().size(), 1 );
            BOOST_CHECK_EQUAL( fp->Constraints().size(), 1 );
            DowngradeBoardInPlace( &board, target, drop );
            BOOST_CHECK( board.Constraints().empty() );
            BOOST_CHECK( fp->Constraints().empty() );

            for( PCB_SHAPE* shape : shapes )
            {
                BOOST_CHECK( shape->GetStart() == VECTOR2I( 1000000, 2000000 ) );
                BOOST_CHECK( shape->GetEnd() == VECTOR2I( 7000000, 2000000 ) );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( GridAndDrillDrawingLossesAreReportedBeforeRemoval )
{
    for( bool drop : { false, true } )
    {
        BOARD board;
        board.Add( new PCB_GRID_ITEM( &board ) );
        board.Add( new PCB_DRILL_CHART( &board ) );
        board.Add( new PCB_DRILL_MAP( &board ) );
        board.GetDesignSettings().GetDrillSymbolProfile().SetName( wxT( "Fabrication symbols" ) );
        PCB_VIA* via = new PCB_VIA( &board );
        via->SetPosition( VECTOR2I( 1000000, 2000000 ) );
        board.Add( via );

        const auto report = ClassifyBoardForDowngrade( &board, kicad10, drop );
        checkFeature( report, wxT( "Local grids" ), DOWNGRADE_BUCKET::DROP, 1 );
        checkFeature( report, wxT( "Drill charts and maps" ), DOWNGRADE_BUCKET::DROP, 2 );
        checkFeature( report, wxT( "Drill symbol configuration" ), DOWNGRADE_BUCKET::DROP, 1 );
        BOOST_CHECK_EQUAL( board.Drawings().size(), 3 );
        DowngradeBoardInPlace( &board, kicad10, drop );
        BOOST_CHECK( board.Drawings().empty() );
        BOOST_REQUIRE_EQUAL( board.Tracks().size(), 1 );
        BOOST_CHECK( via->GetPosition() == VECTOR2I( 1000000, 2000000 ) );
        BOOST_CHECK( board.GetDesignSettings().GetDrillSymbolProfile() == DRILL_SYMBOL_PROFILE() );
    }
}


BOOST_AUTO_TEST_CASE( NewGeneratorsLoseEditabilityAndKeepCopperAndOuterGroups )
{
    for( bool drop : { false, true } )
    {
        BOARD      board;
        PCB_GROUP* outer = new PCB_GROUP( &board );
        board.Add( outer );
        PCB_VIA_STITCH* stitch = new PCB_VIA_STITCH( &board );
        PCB_VIA_STACK*  stack = new PCB_VIA_STACK( &board );
        board.Add( stitch );
        board.Add( stack );
        outer->AddItem( stitch );
        outer->AddItem( stack );
        std::vector<PCB_VIA*> vias;

        for( PCB_GENERATOR* generator :
             { static_cast<PCB_GENERATOR*>( stitch ), static_cast<PCB_GENERATOR*>( stack ) } )
        {
            PCB_VIA* via = new PCB_VIA( &board );
            via->SetPosition( VECTOR2I( 1000000 * ( vias.size() + 1 ), 2000000 ) );
            via->SetWidth( F_Cu, 600000 );
            via->SetDrill( 300000 );
            board.Add( via );
            generator->AddItem( via );
            vias.push_back( via );
        }

        const auto report = ClassifyBoardForDowngrade( &board, kicad10, drop );
        checkFeature( report, wxT( "Via stitching and guarding" ), DOWNGRADE_BUCKET::DROP, 1 );
        checkFeature( report, wxT( "Microvia stack generators" ), DOWNGRADE_BUCKET::DROP, 1 );
        DowngradeBoardInPlace( &board, kicad10, drop );
        BOOST_CHECK( board.Generators().empty() );
        BOOST_REQUIRE_EQUAL( board.Tracks().size(), 2 );
        BOOST_REQUIRE_EQUAL( outer->GetItems().size(), 2 );

        for( size_t ii = 0; ii < vias.size(); ++ii )
        {
            BOOST_CHECK( vias[ii]->GetPosition() == VECTOR2I( 1000000 * ( ii + 1 ), 2000000 ) );
            BOOST_CHECK_EQUAL( vias[ii]->GetWidth( F_Cu ), 600000 );
            BOOST_CHECK_EQUAL( vias[ii]->GetDrill(), 300000 );
            BOOST_CHECK( vias[ii]->GetParentGroup() == outer );
        }
    }
}


BOOST_AUTO_TEST_CASE( NativeViaGeneratorFixtureExportsCopperAndOwnershipForBothTargetsAndPolicies )
{
    SETTINGS_MANAGER  settingsManager;
    const std::string src =
            KI_TEST::GetPcbnewTestDataDir() + "../downgrade/regressions/via_generators/via_generators.kicad_pcb";
    const std::string      originalBytes = KI_TEST::ReadGoldenText( src );
    PCB_IO_KICAD_SEXPR     io;
    std::unique_ptr<BOARD> source( io.LoadBoard( src, nullptr ) );
    BOOST_REQUIRE( source );
    BOOST_REQUIRE_EQUAL( source->GetCopperLayerCount(), 6 );
    BOOST_REQUIRE_EQUAL( source->Zones().size(), 3u );
    BOOST_REQUIRE_EQUAL( source->Generators().size(), 3u );
    BOOST_REQUIRE_EQUAL( source->Groups().size(), 1u );
    BOOST_REQUIRE( source->GroupsSanityCheck().IsEmpty() );

    const auto copperById = []( const BOARD& aBoard )
    {
        std::map<KIID, const PCB_TRACK*> items;

        for( const PCB_TRACK* item : aBoard.Tracks() )
            BOOST_REQUIRE( items.emplace( item->m_Uuid, item ).second );

        return items;
    };
    const auto memberIds = []( const PCB_GROUP& aGroup )
    {
        std::set<KIID> ids;

        for( const EDA_ITEM* member : aGroup.GetItems() )
            BOOST_REQUIRE( ids.insert( member->m_Uuid ).second );

        return ids;
    };
    const auto serializeNative = []( BOARD* aBoard )
    {
        STRING_FORMATTER   formatter;
        PCB_IO_KICAD_SEXPR serializer;
        serializer.FormatBoardToFormatter( &formatter, aBoard );
        return formatter.GetString();
    };

    const auto sourceCopper = copperById( *source );
    BOOST_REQUIRE_EQUAL( sourceCopper.size(), 83u );
    std::set<KIID> plainVias;
    std::set<KIID> generatedVias;
    size_t         sourceVias = 0;
    size_t         sourceSegments = 0;
    size_t         sourceMicrovias = 0;

    for( const auto& [id, item] : sourceCopper )
    {
        if( const PCB_VIA* via = dynamic_cast<const PCB_VIA*>( item ) )
        {
            ++sourceVias;

            if( via->GetViaType() == VIATYPE::MICROVIA )
            {
                ++sourceMicrovias;
                BOOST_REQUIRE( via->Padstack().Drill().is_filled.value_or( false ) );
            }

            if( !via->GetParentGroup() )
                plainVias.insert( id );
        }
        else
        {
            BOOST_REQUIRE( item->Type() == PCB_TRACE_T );
            ++sourceSegments;
        }
    }

    BOOST_REQUIRE_EQUAL( sourceVias, 80u );
    BOOST_REQUIRE_EQUAL( sourceSegments, 3u );
    BOOST_REQUIRE_EQUAL( sourceMicrovias, 2u );
    BOOST_REQUIRE_EQUAL( plainVias.size(), 3u );

    PCB_VIA_STITCH* stitch = nullptr;
    PCB_VIA_STITCH* guard = nullptr;
    PCB_VIA_STACK*  stack = nullptr;

    for( PCB_GENERATOR* generator : source->Generators() )
    {
        if( PCB_VIA_STITCH* viaStitch = dynamic_cast<PCB_VIA_STITCH*>( generator ) )
        {
            BOOST_REQUIRE( source->FindNet( viaStitch->GetNetCode() ) );
            BOOST_REQUIRE( source->FindNet( viaStitch->GetNetCode() )->GetNetname() == wxT( "GND" ) );

            if( viaStitch->GetMode() == PCB_VIA_STITCH_MODE::STITCH )
            {
                BOOST_REQUIRE( !stitch );
                stitch = viaStitch;
            }
            else
            {
                BOOST_REQUIRE( viaStitch->GetMode() == PCB_VIA_STITCH_MODE::GUARD );
                BOOST_REQUIRE( !guard );
                guard = viaStitch;
                BOOST_REQUIRE( source->FindNet( guard->GetGuardedNetCode() ) );
                BOOST_REQUIRE( source->FindNet( guard->GetGuardedNetCode() )->GetNetname() == wxT( "SIGNAL" ) );
            }
        }
        else
        {
            BOOST_REQUIRE( !stack );
            stack = dynamic_cast<PCB_VIA_STACK*>( generator );
            BOOST_REQUIRE( stack );
            BOOST_REQUIRE( stack->GetStyle() == VIA_STACK_STYLE::STACKED );
            BOOST_REQUIRE( stack->GetStartLayer() == F_Cu );
            BOOST_REQUIRE( stack->GetEndLayer() == In2_Cu );
        }

        for( BOARD_ITEM* member : generator->GetBoardItems() )
        {
            BOOST_REQUIRE( member->Type() == PCB_VIA_T );
            BOOST_REQUIRE( member->GetParentGroup() == static_cast<EDA_GROUP*>( generator ) );
            BOOST_REQUIRE( sourceCopper.count( member->m_Uuid ) == 1u );
            BOOST_REQUIRE( generatedVias.insert( member->m_Uuid ).second );
        }
    }

    BOOST_REQUIRE( stitch && guard && stack );
    BOOST_REQUIRE_EQUAL( stitch->GetItems().size(), 49u );
    BOOST_REQUIRE_EQUAL( guard->GetItems().size(), 26u );
    BOOST_REQUIRE_EQUAL( stack->GetItems().size(), 2u );
    BOOST_REQUIRE_EQUAL( generatedVias.size(), 77u );
    const auto     stitchMembers = memberIds( *stitch );
    PCB_GROUP*     outer = *source->Groups().begin();
    const KIID     outerId = outer->m_Uuid;
    const wxString outerName = outer->GetName();
    BOOST_REQUIRE( stitch->GetParentGroup() == static_cast<EDA_GROUP*>( outer ) );
    BOOST_REQUIRE( !guard->GetParentGroup() && !stack->GetParentGroup() );
    BOOST_REQUIRE( memberIds( *outer ) == std::set<KIID>{ stitch->m_Uuid } );

    const std::string            sourceModel = serializeNative( source.get() );
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_native_via_generators", "" );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        const bool preTen = target.m_boardVersion == kicad9.m_boardVersion;

        for( bool drop : { false, true } )
        {
            BOOST_TEST_CONTEXT( target.m_id.ToStdString() << " drop=" << drop )
            {
                const COMPATIBILITY_REPORT preview = ClassifyBoardForDowngrade( source.get(), target, drop );
                BOOST_CHECK( !preview.IsBlocked() );
                BOOST_CHECK_EQUAL( preview.Count( DOWNGRADE_BUCKET::LOWER ), 0 );
                BOOST_CHECK_EQUAL( preview.Count( DOWNGRADE_BUCKET::DROP ), preTen ? 5 : 3 );
                BOOST_CHECK_EQUAL( serializeNative( source.get() ), sourceModel );
                const std::string dest =
                        ( tmp.GetPath()
                          / ( target.m_id.ToStdString() + ( drop ? "-drop.kicad_pcb" : "-lower.kicad_pcb" ) ) )
                                .string();
                COMPATIBILITY_REPORT report;
                wxString             unsupportedToken;
                BOOST_REQUIRE( ExportBoardToOlderVersion( src, dest, target, report, &unsupportedToken, wxEmptyString,
                                                          drop ) );
                BOOST_CHECK( unsupportedToken.IsEmpty() );
                BOOST_CHECK( !report.IsBlocked() );
                BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::LOWER ), 0 );
                BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), preTen ? 5 : 3 );
                BOOST_REQUIRE_EQUAL( report.Entries().size(), preTen ? 3u : 2u );
                checkFeature( report, wxT( "Via stitching and guarding" ), DOWNGRADE_BUCKET::DROP, 2 );
                checkFeature( report, wxT( "Microvia stack generators" ), DOWNGRADE_BUCKET::DROP, 1 );

                if( preTen )
                    checkFeature( report, wxT( "Via protection" ), DOWNGRADE_BUCKET::DROP, 2 );

                BOOST_REQUIRE_EQUAL( report.Entries().size(), preview.Entries().size() );

                for( size_t ii = 0; ii < preview.Entries().size(); ++ii )
                {
                    BOOST_CHECK( report.Entries()[ii].m_bucket == preview.Entries()[ii].m_bucket );
                    BOOST_CHECK( report.Entries()[ii].m_feature == preview.Entries()[ii].m_feature );
                    BOOST_CHECK_EQUAL( report.Entries()[ii].m_count, preview.Entries()[ii].m_count );
                }

                const std::string outputBytes = KI_TEST::ReadGoldenText( dest );
                BOOST_CHECK( FindUnsupportedBoardToken( wxString::FromUTF8( outputBytes ), target ).IsEmpty() );
                BOOST_CHECK( outputBytes.find( "(generated" ) == std::string::npos );

                if( preTen )
                {
                    for( const char* token : { "(filling", "(capping", "(covering", "(plugging" } )
                        BOOST_CHECK( outputBytes.find( token ) == std::string::npos );
                }

                std::unique_ptr<BOARD> exported( io.LoadBoard( dest, nullptr ) );
                BOOST_REQUIRE( exported );
                BOOST_CHECK_EQUAL( exported->GetFileFormatVersionAtLoad(), target.m_boardVersion );
                BOOST_CHECK_EQUAL( exported->GetCopperLayerCount(), 6 );
                BOOST_CHECK_EQUAL( exported->Zones().size(), 3u );
                BOOST_CHECK( exported->Generators().empty() );
                BOOST_REQUIRE_EQUAL( exported->Groups().size(), 1u );
                BOOST_CHECK( exported->GroupsSanityCheck().IsEmpty() );

                const auto exportedCopper = copperById( *exported );
                BOOST_REQUIRE_EQUAL( exportedCopper.size(), sourceCopper.size() );
                PCB_GROUP* exportedOuter = *exported->Groups().begin();
                BOOST_CHECK( exportedOuter->m_Uuid == outerId );
                BOOST_CHECK( exportedOuter->GetName() == outerName );
                BOOST_CHECK( memberIds( *exportedOuter ) == stitchMembers );

                for( const auto& [id, original] : sourceCopper )
                {
                    const auto found = exportedCopper.find( id );
                    BOOST_REQUIRE( found != exportedCopper.end() );
                    const PCB_TRACK* item = found->second;
                    BOOST_CHECK( item->Type() == original->Type() );
                    BOOST_CHECK( item->GetStart() == original->GetStart() );
                    BOOST_CHECK( item->GetEnd() == original->GetEnd() );
                    BOOST_CHECK( item->GetLayerSet() == original->GetLayerSet() );
                    BOOST_CHECK_EQUAL( item->GetNetname().ToStdString(), original->GetNetname().ToStdString() );

                    if( const PCB_VIA* originalVia = dynamic_cast<const PCB_VIA*>( original ) )
                    {
                        const PCB_VIA* via = dynamic_cast<const PCB_VIA*>( item );
                        BOOST_REQUIRE( via );
                        BOOST_CHECK( via->GetPosition() == originalVia->GetPosition() );
                        BOOST_CHECK( via->GetViaType() == originalVia->GetViaType() );
                        BOOST_CHECK( via->TopLayer() == originalVia->TopLayer() );
                        BOOST_CHECK( via->BottomLayer() == originalVia->BottomLayer() );
                        BOOST_CHECK_EQUAL( via->GetDrillValue(), originalVia->GetDrillValue() );

                        for( PCB_LAYER_ID layer : LSET::AllCuMask( 6 ) )
                            BOOST_CHECK_EQUAL( via->GetWidth( layer ), originalVia->GetWidth( layer ) );

                        const PADSTACK& padstack = via->Padstack();

                        if( preTen )
                        {
                            // The current reader resolves omitted legacy fields to board defaults.
                            BOOST_CHECK( !padstack.Drill().is_filled.value_or( false ) );
                            BOOST_CHECK( !padstack.Drill().is_capped.value_or( false ) );
                            BOOST_CHECK( !padstack.FrontOuterLayers().has_covering.value_or( false ) );
                            BOOST_CHECK( !padstack.BackOuterLayers().has_covering.value_or( false ) );
                            BOOST_CHECK( !padstack.FrontOuterLayers().has_plugging.value_or( false ) );
                            BOOST_CHECK( !padstack.BackOuterLayers().has_plugging.value_or( false ) );
                        }
                        else
                        {
                            const PADSTACK& originalStack = originalVia->Padstack();
                            BOOST_CHECK( padstack.Drill().is_filled == originalStack.Drill().is_filled );
                            BOOST_CHECK( padstack.Drill().is_capped == originalStack.Drill().is_capped );
                            BOOST_CHECK( padstack.FrontOuterLayers().has_covering
                                         == originalStack.FrontOuterLayers().has_covering );
                            BOOST_CHECK( padstack.BackOuterLayers().has_covering
                                         == originalStack.BackOuterLayers().has_covering );
                            BOOST_CHECK( padstack.FrontOuterLayers().has_plugging
                                         == originalStack.FrontOuterLayers().has_plugging );
                            BOOST_CHECK( padstack.BackOuterLayers().has_plugging
                                         == originalStack.BackOuterLayers().has_plugging );
                        }

                        if( plainVias.count( id ) )
                            BOOST_CHECK( !via->GetParentGroup() );
                    }
                    else
                    {
                        BOOST_CHECK( item->GetLayer() == original->GetLayer() );
                        BOOST_CHECK_EQUAL( item->GetWidth(), original->GetWidth() );
                    }

                    if( stitchMembers.count( id ) )
                        BOOST_CHECK( item->GetParentGroup() == static_cast<EDA_GROUP*>( exportedOuter ) );
                    else
                        BOOST_CHECK( !item->GetParentGroup() );
                }

                for( BOARD_ITEM* drawing : source->Drawings() )
                {
                    if( const EDA_TEXT* text = dynamic_cast<const EDA_TEXT*>( drawing ) )
                    {
                        const auto found = std::find_if( exported->Drawings().begin(), exported->Drawings().end(),
                                                         [&]( const BOARD_ITEM* aItem )
                                                         {
                                                             return aItem->m_Uuid == drawing->m_Uuid;
                                                         } );
                        BOOST_REQUIRE( found != exported->Drawings().end() );
                        const EDA_TEXT* exportedText = dynamic_cast<const EDA_TEXT*>( *found );
                        BOOST_REQUIRE( exportedText );
                        BOOST_CHECK( exportedText->GetText() == text->GetText() );
                    }
                }

                BOOST_CHECK_EQUAL( serializeNative( source.get() ), sourceModel );
                BOOST_CHECK_EQUAL( KI_TEST::ReadGoldenText( src ), originalBytes );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( NativeTuningFixturesPreserveCopperAndEraEditabilityForBothPolicies )
{
    SETTINGS_MANAGER       settingsManager;
    PCB_IO_KICAD_SEXPR     io;
    const std::string      lengthPath = KI_TEST::GetPcbnewTestDataDir() + "tuning_generators_load_save.kicad_pcb";
    const std::string      timePath = KI_TEST::GetPcbnewTestDataDir() + "issue24772/fw16_MCIO8i.kicad_pcb";
    const std::string      lengthBytes = KI_TEST::ReadGoldenText( lengthPath );
    const std::string      timeBytes = KI_TEST::ReadGoldenText( timePath );
    std::unique_ptr<BOARD> lengthFixture( io.LoadBoard( lengthPath, nullptr ) );
    std::unique_ptr<BOARD> timeFixture( io.LoadBoard( timePath, nullptr ) );
    BOOST_REQUIRE( lengthFixture && timeFixture );
    BOOST_REQUIRE_EQUAL( lengthFixture->Generators().size(), 1u );
    BOOST_REQUIRE_EQUAL( timeFixture->Generators().size(), 16u );
    PCB_TUNING_PATTERN* originalTime = nullptr;

    for( PCB_GENERATOR* generator : timeFixture->Generators() )
    {
        if( generator->m_Uuid == KIID( "2acf841f-25ce-450c-b8df-dbbc34cc96ac" ) )
            originalTime = dynamic_cast<PCB_TUNING_PATTERN*>( generator );
    }

    BOOST_REQUIRE( originalTime );
    BOOST_REQUIRE( originalTime->GetSettings().m_isTimeDomain );
    BOOST_REQUIRE_EQUAL( originalTime->GetItems().size(), 1u );
    BOARD board;
    board.SetCopperLayerCount( timeFixture->GetCopperLayerCount() );
    board.SetEnabledLayers( timeFixture->GetEnabledLayers() );
    auto length = dynamic_cast<PCB_TUNING_PATTERN*>( ( *lengthFixture->Generators().begin() )->DeepClone() );
    auto time = dynamic_cast<PCB_TUNING_PATTERN*>( originalTime->DeepClone() );
    BOOST_REQUIRE( length && time );
    BOOST_REQUIRE( !length->GetSettings().m_isTimeDomain );
    BOOST_REQUIRE_GT( length->GetItems().size(), 1u );

    const auto addCopper = [&]( PCB_TRACK* aTrack )
    {
        NETINFO_ITEM* net = board.FindNet( aTrack->GetNetname() );

        if( !net )
        {
            net = new NETINFO_ITEM( &board, aTrack->GetNetname() );
            board.Add( net );
        }

        aTrack->SetParent( &board );
        aTrack->SetNetCode( net->GetNetCode() );
        board.Add( aTrack );
    };

    for( PCB_TUNING_PATTERN* pattern : { length, time } )
    {
        for( BOARD_ITEM* member : pattern->GetBoardItems() )
        {
            PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( member );
            BOOST_REQUIRE( track );
            addCopper( track );
        }

        board.Add( pattern );
    }

    PCB_TRACK* originalControl = nullptr;

    for( PCB_TRACK* track : timeFixture->Tracks() )
    {
        if( track->Type() == PCB_TRACE_T && !dynamic_cast<PCB_GENERATOR*>( track->GetParentGroup() ) )
        {
            originalControl = track;
            break;
        }
    }

    BOOST_REQUIRE( originalControl );
    auto control = static_cast<PCB_TRACK*>( originalControl->Clone() );
    control->SetParentGroup( nullptr );
    addCopper( control );
    PCB_GROUP* outer = new PCB_GROUP( &board );
    outer->SetName( wxT( "Native tuning ownership" ) );
    board.Add( outer );
    outer->AddItem( length );
    outer->AddItem( time );

    // Populate the numeric cache through native APIs without changing fixture copper.
    double patternLength = 0;

    for( BOARD_ITEM* member : length->GetBoardItems() )
        patternLength += static_cast<PCB_TRACK*>( member )->GetLength();

    STRING_ANY_MAP lengthProperties = length->GetProperties();

    // The native setter expects wxString values for these serialized string enums.
    for( const char* key : { "tuning_mode", "initial_side", "last_status" } )
    {
        std::string value;
        BOOST_REQUIRE( lengthProperties.get_to( key, value ) );
        lengthProperties[key] = wxAny( wxString::FromUTF8( value ) );
    }

    lengthProperties["last_tuning_length"] = wxAny( patternLength / pcbIUScale.IU_PER_MM );
    length->SetProperties( lengthProperties );
    const wxString tuningInfo = length->GetRowData().back().second.GetString();
    BOOST_REQUIRE( !tuningInfo.IsEmpty() );
    const auto memberIds = []( const PCB_GROUP& aGroup )
    {
        std::set<KIID> ids;

        for( const EDA_ITEM* member : aGroup.GetItems() )
            BOOST_REQUIRE( ids.insert( member->m_Uuid ).second );

        return ids;
    };
    const auto serializeNative = []( BOARD* aBoard )
    {
        STRING_FORMATTER   formatter;
        PCB_IO_KICAD_SEXPR serializer;
        serializer.FormatBoardToFormatter( &formatter, aBoard );
        return formatter.GetString();
    };
    std::map<KIID, const PCB_TRACK*> copper;

    for( const PCB_TRACK* track : board.Tracks() )
        BOOST_REQUIRE( copper.emplace( track->m_Uuid, track ).second );

    const auto lengthMembers = memberIds( *length );
    const auto timeMembers = memberIds( *time );
    BOOST_REQUIRE_EQUAL( copper.size(), lengthMembers.size() + timeMembers.size() + 1u );
    BOOST_REQUIRE( board.GroupsSanityCheck().IsEmpty() );
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_native_tuning_boundaries" );
    const std::string            src = ( tmp.GetPath() / "native-source.kicad_pcb" ).string();
    io.SaveBoard( src, board );
    const std::string sourceBytes = KI_TEST::ReadGoldenText( src );
    const std::string sourceModel = serializeNative( &board );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        const bool preTen = target.m_boardVersion == kicad9.m_boardVersion;

        for( bool drop : { false, true } )
        {
            BOOST_TEST_CONTEXT( target.m_id.ToStdString() << " drop=" << drop )
            {
                const auto preview = ClassifyBoardForDowngrade( &board, target, drop );
                BOOST_CHECK( !preview.IsBlocked() );
                BOOST_CHECK_EQUAL( preview.Count( DOWNGRADE_BUCKET::LOWER ), 0 );
                BOOST_CHECK_EQUAL( preview.Count( DOWNGRADE_BUCKET::DROP ), preTen ? 1 : 0 );
                BOOST_CHECK_EQUAL( serializeNative( &board ), sourceModel );
                const std::string dest =
                        ( tmp.GetPath()
                          / ( target.m_id.ToStdString() + ( drop ? "-drop.kicad_pcb" : "-lower.kicad_pcb" ) ) )
                                .string();
                COMPATIBILITY_REPORT report;
                wxString             unsupported;
                BOOST_REQUIRE(
                        ExportBoardToOlderVersion( src, dest, target, report, &unsupported, wxEmptyString, drop ) );
                BOOST_CHECK( unsupported.IsEmpty() );
                BOOST_CHECK( !report.IsBlocked() );
                BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::LOWER ), 0 );
                BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), preTen ? 1 : 0 );
                BOOST_REQUIRE_EQUAL( report.Entries().size(), preTen ? 1u : 0u );

                if( preTen )
                    checkFeature( report, wxT( "Time-domain tuning patterns" ), DOWNGRADE_BUCKET::DROP, 1 );

                const std::string outputBytes = KI_TEST::ReadGoldenText( dest );
                BOOST_CHECK( FindUnsupportedBoardToken( wxString::FromUTF8( outputBytes ), target ).IsEmpty() );

                if( preTen )
                {
                    for( const char* token : { "(is_time_domain", "(target_delay", "(last_tuning_length" } )
                        BOOST_CHECK( outputBytes.find( token ) == std::string::npos );

                    STRING_FORMATTER quoting;
                    BOOST_CHECK( outputBytes.find( "(last_tuning " + quoting.Quotew( tuningInfo ) + ")" )
                                 != std::string::npos );
                }
                else
                {
                    BOOST_CHECK( outputBytes.find( "(is_time_domain yes)" ) != std::string::npos );
                    BOOST_CHECK( outputBytes.find( "(last_tuning_length " ) != std::string::npos );
                    BOOST_CHECK( outputBytes.find( "(last_tuning " ) == std::string::npos );
                }

                std::unique_ptr<BOARD> exported( io.LoadBoard( dest, nullptr ) );
                BOOST_REQUIRE( exported );
                BOOST_CHECK_EQUAL( exported->GetFileFormatVersionAtLoad(), target.m_boardVersion );
                BOOST_REQUIRE_EQUAL( exported->Generators().size(), preTen ? 1u : 2u );
                BOOST_REQUIRE_EQUAL( exported->Groups().size(), 1u );
                BOOST_CHECK( exported->GroupsSanityCheck().IsEmpty() );
                PCB_GROUP* exportedOuter = *exported->Groups().begin();
                BOOST_CHECK( exportedOuter->m_Uuid == outer->m_Uuid );
                std::set<KIID> expectedOuter{ length->m_Uuid };

                if( preTen )
                    expectedOuter.insert( timeMembers.begin(), timeMembers.end() );
                else
                    expectedOuter.insert( time->m_Uuid );

                BOOST_CHECK( memberIds( *exportedOuter ) == expectedOuter );
                BOOST_REQUIRE_EQUAL( exported->Tracks().size(), copper.size() );

                for( const PCB_TRACK* track : exported->Tracks() )
                {
                    const auto found = copper.find( track->m_Uuid );
                    BOOST_REQUIRE( found != copper.end() );
                    const PCB_TRACK* original = found->second;
                    BOOST_CHECK( track->Type() == original->Type() );
                    BOOST_CHECK( track->GetStart() == original->GetStart() );
                    BOOST_CHECK( track->GetEnd() == original->GetEnd() );
                    BOOST_CHECK( track->GetLayerSet() == original->GetLayerSet() );
                    BOOST_CHECK_EQUAL( track->GetWidth(), original->GetWidth() );
                    BOOST_CHECK_EQUAL( track->GetNetname().ToStdString(), original->GetNetname().ToStdString() );

                    if( const PCB_ARC* arc = dynamic_cast<const PCB_ARC*>( original ) )
                    {
                        const PCB_ARC* exportedArc = dynamic_cast<const PCB_ARC*>( track );
                        BOOST_REQUIRE( exportedArc );
                        BOOST_CHECK( exportedArc->GetMid() == arc->GetMid() );
                    }

                    if( timeMembers.count( track->m_Uuid ) && preTen )
                        BOOST_CHECK( track->GetParentGroup() == static_cast<EDA_GROUP*>( exportedOuter ) );
                    else if( track->m_Uuid == control->m_Uuid )
                        BOOST_CHECK( !track->GetParentGroup() );
                }

                for( PCB_GENERATOR* generator : exported->Generators() )
                {
                    auto pattern = dynamic_cast<PCB_TUNING_PATTERN*>( generator );
                    BOOST_REQUIRE( pattern );
                    BOOST_CHECK( pattern->GetParentGroup() == static_cast<EDA_GROUP*>( exportedOuter ) );
                    BOOST_CHECK( pattern->m_Uuid == length->m_Uuid || ( !preTen && pattern->m_Uuid == time->m_Uuid ) );
                    const bool isTime = pattern->m_Uuid == time->m_Uuid;
                    BOOST_CHECK( pattern->GetSettings().m_isTimeDomain == isTime );
                    BOOST_CHECK( memberIds( *pattern ) == ( isTime ? timeMembers : lengthMembers ) );
                    const PCB_TUNING_PATTERN* original = isTime ? time : length;
                    const auto                properties = pattern->GetProperties();
                    const auto                originalProperties = original->GetProperties();

                    for( const char* key :
                         { "target_length", "target_length_min", "target_length_max", "target_skew", "target_skew_min",
                           "target_skew_max", "min_spacing", "max_amplitude", "min_amplitude" } )
                    {
                        double value = 0;
                        double originalValue = 0;
                        BOOST_REQUIRE( properties.get_to( key, value ) );
                        BOOST_REQUIRE( originalProperties.get_to( key, originalValue ) );
                        BOOST_CHECK_EQUAL( value, originalValue );
                    }

                    if( !preTen )
                    {
                        for( const char* key :
                             { "target_delay", "target_delay_min", "target_delay_max", "last_tuning_length" } )
                        {
                            double value = 0;
                            double originalValue = 0;
                            BOOST_REQUIRE( properties.get_to( key, value ) );
                            BOOST_REQUIRE( originalProperties.get_to( key, originalValue ) );
                            BOOST_CHECK_EQUAL( value, originalValue );
                        }
                    }
                }

                BOOST_CHECK_EQUAL( serializeNative( &board ), sourceModel );
                BOOST_CHECK_EQUAL( KI_TEST::ReadGoldenText( src ), sourceBytes );
                BOOST_CHECK_EQUAL( KI_TEST::ReadGoldenText( lengthPath ), lengthBytes );
                BOOST_CHECK_EQUAL( KI_TEST::ReadGoldenText( timePath ), timeBytes );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( UnreviewedTuningPropertiesBlockBothTargetsAndPolicies )
{
    class UNREVIEWED_TUNING_PATTERN : public PCB_TUNING_PATTERN
    {
    public:
        using PCB_TUNING_PATTERN::PCB_TUNING_PATTERN;

        const STRING_ANY_MAP GetProperties() const override
        {
            STRING_ANY_MAP properties = PCB_TUNING_PATTERN::GetProperties();
            properties.set( "future_tuning_setting", true );
            return properties;
        }
    };

    BOARD board;
    auto  pattern = new UNREVIEWED_TUNING_PATTERN( &board );
    auto  track = new PCB_TRACK( &board );
    track->SetStart( VECTOR2I( 1000000, 2000000 ) );
    track->SetEnd( VECTOR2I( 3000000, 2000000 ) );
    board.Add( track );
    board.Add( pattern );
    pattern->AddItem( track );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        for( bool drop : { false, true } )
        {
            const auto report = ClassifyBoardForDowngrade( &board, target, drop );
            checkFeature( report, wxT( "Unreviewed tuning properties" ), DOWNGRADE_BUCKET::BLOCK, 1 );
            BOOST_REQUIRE_EQUAL( board.Generators().size(), 1u );
            BOOST_CHECK( track->GetParentGroup() == static_cast<EDA_GROUP*>( pattern ) );
        }
    }

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_unreviewed_tuning" );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        const wxString path = ( tmp.GetPath() / ( target.m_id.ToStdString() + ".kicad_pcb" ) ).wstring();
        BOOST_CHECK_THROW( SaveBoardForTarget( &board, path, target ), IO_ERROR );
    }
}


BOOST_AUTO_TEST_CASE( HistoricalTuningWritersRejectUnsafeDirectSerialization )
{
    BOARD board;
    auto  pattern = new PCB_TUNING_PATTERN( &board );
    pattern->SetTargetDelay( 100000 );
    auto track = new PCB_TRACK( &board );
    track->SetStart( VECTOR2I( 1000000, 2000000 ) );
    track->SetEnd( VECTOR2I( 3000000, 2000000 ) );
    board.Add( track );
    board.Add( pattern );
    pattern->AddItem( track );
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_direct_time_tuning" );
    BOOST_CHECK_THROW( SaveBoardForTarget( &board, ( tmp.GetPath() / "unsafe.kicad_pcb" ).wstring(), kicad9 ),
                       IO_ERROR );

    for( const char* token :
         { "is_time_domain", "target_delay", "target_delay_min", "target_delay_max", "last_tuning_length" } )
    {
        const wxString value = wxString::FromUTF8( "(kicad_pcb (generated (" + std::string( token ) + " 0)))" );
        BOOST_CHECK_EQUAL( FindUnsupportedBoardToken( value, kicad9 ), wxString::FromUTF8( token ) );
        BOOST_CHECK( FindUnsupportedBoardToken( value, kicad10 ).IsEmpty() );
    }
}


BOOST_AUTO_TEST_CASE( HistoricalWritersRejectDirectViaGeneratorsAtomically )
{
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_direct_via_generators" );

    for( bool stack : { false, true } )
    {
        BOARD board;

        if( stack )
            board.Add( new PCB_VIA_STACK( &board ) );
        else
            board.Add( new PCB_VIA_STITCH( &board ) );

        const std::string kind = stack ? "via-stack" : "via-stitch";

        for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
        {
            BOOST_TEST_CONTEXT( kind << " target=" << target.m_id.ToStdString() )
            {
                const auto absent = tmp.GetPath() / ( kind + "-" + target.m_id.ToStdString() + "-absent.kicad_pcb" );
                BOOST_CHECK_THROW( SaveBoardForTarget( &board, absent.wstring(), target ), IO_ERROR );
                BOOST_CHECK( !wxFileExists( absent.wstring() ) );

                const auto existing =
                        tmp.GetPath() / ( kind + "-" + target.m_id.ToStdString() + "-existing.kicad_pcb" );

                {
                    std::ofstream sentinel( existing, std::ios::binary );
                    sentinel << "existing destination";
                    BOOST_REQUIRE( sentinel.good() );
                }

                BOOST_CHECK_THROW( SaveBoardForTarget( &board, existing.wstring(), target ), IO_ERROR );
                BOOST_CHECK_EQUAL( KI_TEST::ReadGoldenText( existing.string() ), "existing destination" );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( NativeGroupPlacementFixtureKeepsGeometryAndOwnershipForBothPolicies )
{
    SETTINGS_MANAGER       settingsManager;
    const std::string      src =
            KI_TEST::GetPcbnewTestDataDir() + "../downgrade/regressions/group_placement/group_placement.kicad_pcb";
    wxFileName projectFile( wxString::FromUTF8( src ) );
    projectFile.SetExt( wxT( "kicad_pro" ) );
    BOOST_REQUIRE( wxFileExists( projectFile.GetFullPath() ) );
    BOOST_REQUIRE( settingsManager.LoadProject( projectFile.GetFullPath(), false ) );
    const std::string      sourceBytes = KI_TEST::ReadGoldenText( src );
    PCB_IO_KICAD_SEXPR     io;
    std::unique_ptr<BOARD> source( io.LoadBoard( src, nullptr ) );
    BOOST_REQUIRE( source );
    BOOST_REQUIRE_EQUAL( source->Zones().size(), 7u );
    BOOST_REQUIRE_EQUAL( source->Groups().size(), 4u );
    BOOST_REQUIRE_EQUAL( source->Footprints().size(), 4u );
    BOOST_REQUIRE( source->GroupsSanityCheck().IsEmpty() );
    std::map<KIID, const PCB_TEXT*> sourceNotes;

    for( const BOARD_ITEM* drawing : source->Drawings() )
    {
        if( const PCB_TEXT* note = dynamic_cast<const PCB_TEXT*>( drawing ) )
            BOOST_REQUIRE( sourceNotes.emplace( note->m_Uuid, note ).second );
    }

    BOOST_REQUIRE_EQUAL( sourceNotes.size(), 1u );
    std::map<KIID, const ZONE*> sourceZones;
    int                         placementAreas = 0;
    int                         filledOutlines = 0;

    for( const ZONE* zone : source->Zones() )
    {
        BOOST_REQUIRE( sourceZones.emplace( zone->m_Uuid, zone ).second );

        if( zone->GetPlacementAreaSourceType() == PLACEMENT_SOURCE_T::GROUP_PLACEMENT )
        {
            ++placementAreas;
            BOOST_REQUIRE( zone->GetPlacementAreaEnabled() );
            BOOST_REQUIRE( !zone->GetPlacementAreaSource().IsEmpty() );
        }

        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
            filledOutlines += zone->GetFilledPolysList( layer )->OutlineCount();
    }

    BOOST_REQUIRE_EQUAL( placementAreas, 4 );
    BOOST_REQUIRE_GT( filledOutlines, 0 );
    const auto memberIds = []( const PCB_GROUP& aGroup )
    {
        std::set<KIID> ids;

        for( const EDA_ITEM* member : aGroup.GetItems() )
            BOOST_REQUIRE( ids.insert( member->m_Uuid ).second );

        return ids;
    };
    const auto checkPolygons = []( const SHAPE_POLY_SET& aOriginal, const SHAPE_POLY_SET& aExported )
    {
        BOOST_REQUIRE_EQUAL( aExported.OutlineCount(), aOriginal.OutlineCount() );

        for( int outline = 0; outline < aOriginal.OutlineCount(); ++outline )
        {
            BOOST_CHECK( aExported.COutline( outline ).CompareGeometry( aOriginal.COutline( outline ) ) );
            BOOST_REQUIRE_EQUAL( aExported.HoleCount( outline ), aOriginal.HoleCount( outline ) );

            for( int hole = 0; hole < aOriginal.HoleCount( outline ); ++hole )
                BOOST_CHECK( aExported.CHole( outline, hole ).CompareGeometry( aOriginal.CHole( outline, hole ) ) );
        }
    };
    const auto serializeNative = []( BOARD* aBoard )
    {
        STRING_FORMATTER   formatter;
        PCB_IO_KICAD_SEXPR serializer;
        serializer.FormatBoardToFormatter( &formatter, aBoard );
        return formatter.GetString();
    };
    const std::string            sourceModel = serializeNative( source.get() );
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_native_group_placement" );
    BOOST_CHECK_THROW( SaveBoardForTarget( source.get(), ( tmp.GetPath() / "unsafe.kicad_pcb" ).wstring(), kicad9 ),
                       IO_ERROR );
    BOOST_CHECK_EQUAL( serializeNative( source.get() ), sourceModel );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        const bool preTen = target.m_boardVersion == kicad9.m_boardVersion;

        for( bool drop : { false, true } )
        {
            BOOST_TEST_CONTEXT( target.m_id.ToStdString() << " drop=" << drop )
            {
                const auto preview = ClassifyBoardForDowngrade( source.get(), target, drop );
                BOOST_CHECK( !preview.IsBlocked() );
                BOOST_CHECK_EQUAL( preview.Count( DOWNGRADE_BUCKET::DROP ), preTen ? 4 : 0 );
                BOOST_CHECK_EQUAL( preview.Count( DOWNGRADE_BUCKET::LOWER ), 0 );
                BOOST_CHECK_EQUAL( serializeNative( source.get() ), sourceModel );
                const std::string dest =
                        ( tmp.GetPath()
                          / ( target.m_id.ToStdString() + ( drop ? "-drop.kicad_pcb" : "-lower.kicad_pcb" ) ) )
                                .string();
                COMPATIBILITY_REPORT report;
                wxString             unsupported;
                BOOST_REQUIRE(
                        ExportBoardToOlderVersion( src, dest, target, report, &unsupported, wxEmptyString, drop ) );
                BOOST_CHECK( unsupported.IsEmpty() );
                BOOST_CHECK( !report.IsBlocked() );
                BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), preTen ? 4 : 0 );
                BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::LOWER ), 0 );
                BOOST_REQUIRE_EQUAL( report.Entries().size(), preTen ? 1u : 0u );

                if( preTen )
                    checkFeature( report, wxT( "Group placement areas" ), DOWNGRADE_BUCKET::DROP, 4 );

                const std::string outputBytes = KI_TEST::ReadGoldenText( dest );
                BOOST_CHECK( FindUnsupportedBoardToken( wxString::FromUTF8( outputBytes ), target ).IsEmpty() );
                std::unique_ptr<BOARD> exported( io.LoadBoard( dest, nullptr ) );
                BOOST_REQUIRE( exported );
                BOOST_CHECK_EQUAL( exported->GetFileFormatVersionAtLoad(), target.m_boardVersion );
                BOOST_REQUIRE_EQUAL( exported->Zones().size(), source->Zones().size() );
                BOOST_REQUIRE_EQUAL( exported->Groups().size(), source->Groups().size() );
                BOOST_REQUIRE_EQUAL( exported->Footprints().size(), source->Footprints().size() );
                BOOST_CHECK( exported->GroupsSanityCheck().IsEmpty() );

                for( const auto& [id, original] : sourceNotes )
                {
                    const auto found = std::find_if( exported->Drawings().begin(), exported->Drawings().end(),
                                                     [&]( const BOARD_ITEM* aItem )
                                                     {
                                                         return aItem->m_Uuid == id;
                                                     } );
                    BOOST_REQUIRE( found != exported->Drawings().end() );
                    const PCB_TEXT* note = dynamic_cast<const PCB_TEXT*>( *found );
                    BOOST_REQUIRE( note );
                    BOOST_CHECK( note->GetText() == original->GetText() );
                    BOOST_CHECK( note->GetPosition() == original->GetPosition() );
                    BOOST_CHECK( note->GetLayer() == original->GetLayer() );
                }

                for( const ZONE* zone : exported->Zones() )
                {
                    const auto found = sourceZones.find( zone->m_Uuid );
                    BOOST_REQUIRE( found != sourceZones.end() );
                    const ZONE* original = found->second;
                    BOOST_CHECK( zone->GetZoneName() == original->GetZoneName() );
                    BOOST_CHECK( zone->GetLayerSet() == original->GetLayerSet() );
                    BOOST_CHECK( zone->GetIsRuleArea() == original->GetIsRuleArea() );
                    BOOST_CHECK( zone->GetDoNotAllowTracks() == original->GetDoNotAllowTracks() );
                    BOOST_CHECK( zone->GetDoNotAllowVias() == original->GetDoNotAllowVias() );
                    BOOST_CHECK( zone->GetDoNotAllowPads() == original->GetDoNotAllowPads() );
                    BOOST_CHECK( zone->GetDoNotAllowZoneFills() == original->GetDoNotAllowZoneFills() );
                    BOOST_CHECK( zone->GetDoNotAllowFootprints() == original->GetDoNotAllowFootprints() );
                    BOOST_CHECK( zone->GetNetname() == original->GetNetname() );
                    BOOST_CHECK( zone->IsFilled() == original->IsFilled() );
                    checkPolygons( original->GetBoardOutline(), zone->GetBoardOutline() );

                    for( PCB_LAYER_ID layer : original->GetLayerSet().Seq() )
                    {
                        const auto originalFill = original->GetFilledPolysList( layer );
                        const auto exportedFill = zone->GetFilledPolysList( layer );
                        checkPolygons( *originalFill, *exportedFill );

                        for( int outline = 0; outline < originalFill->OutlineCount(); ++outline )
                            BOOST_CHECK( zone->IsIsland( layer, outline ) == original->IsIsland( layer, outline ) );
                    }

                    if( preTen && original->GetPlacementAreaSourceType() == PLACEMENT_SOURCE_T::GROUP_PLACEMENT )
                    {
                        BOOST_CHECK( !zone->GetPlacementAreaEnabled() );
                        BOOST_CHECK( zone->GetPlacementAreaSource().IsEmpty() );
                        BOOST_CHECK( zone->GetPlacementAreaSourceType() == PLACEMENT_SOURCE_T::SHEETNAME );
                    }
                    else
                    {
                        BOOST_CHECK( zone->GetPlacementAreaEnabled() == original->GetPlacementAreaEnabled() );
                        BOOST_CHECK( zone->GetPlacementAreaSource() == original->GetPlacementAreaSource() );
                        BOOST_CHECK( zone->GetPlacementAreaSourceType() == original->GetPlacementAreaSourceType() );
                    }
                }

                for( const PCB_GROUP* group : exported->Groups() )
                {
                    const auto original = std::find_if( source->Groups().begin(), source->Groups().end(),
                                                        [&]( const PCB_GROUP* aGroup )
                                                        {
                                                            return aGroup->m_Uuid == group->m_Uuid;
                                                        } );
                    BOOST_REQUIRE( original != source->Groups().end() );
                    BOOST_CHECK( group->GetName() == ( *original )->GetName() );
                    BOOST_CHECK( memberIds( *group ) == memberIds( **original ) );
                }

                std::map<KIID, const PAD*> sourcePads;

                for( const PAD* pad : source->GetPads() )
                    BOOST_REQUIRE( sourcePads.emplace( pad->m_Uuid, pad ).second );

                BOOST_REQUIRE_EQUAL( sourcePads.size(), 8u );
                BOOST_REQUIRE_EQUAL( exported->GetPads().size(), sourcePads.size() );

                for( const PAD* pad : exported->GetPads() )
                {
                    const auto found = sourcePads.find( pad->m_Uuid );
                    BOOST_REQUIRE( found != sourcePads.end() );
                    const PAD* original = found->second;
                    BOOST_CHECK( pad->GetPosition() == original->GetPosition() );
                    BOOST_CHECK( pad->GetOrientation() == original->GetOrientation() );
                    BOOST_CHECK( pad->GetLayerSet() == original->GetLayerSet() );
                    BOOST_CHECK( pad->GetNetname() == original->GetNetname() );
                    BOOST_CHECK( pad->GetShape( F_Cu ) == original->GetShape( F_Cu ) );
                    BOOST_CHECK( pad->GetSize( F_Cu ) == original->GetSize( F_Cu ) );
                    BOOST_CHECK_EQUAL( pad->GetRoundRectRadiusRatio( F_Cu ),
                                       original->GetRoundRectRadiusRatio( F_Cu ) );
                }

                BOOST_CHECK_EQUAL( serializeNative( source.get() ), sourceModel );
                BOOST_CHECK_EQUAL( KI_TEST::ReadGoldenText( src ), sourceBytes );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( GroupPlacementLinkageUsesTheSameFootprintPolicy )
{
    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        for( bool drop : { false, true } )
        {
            BOARD board;
            auto  footprint = new FOOTPRINT( &board );
            auto  zone = new ZONE( footprint );
            zone->SetIsRuleArea( true );
            zone->SetPlacementAreaEnabled( true );
            zone->SetPlacementAreaSourceType( PLACEMENT_SOURCE_T::GROUP_PLACEMENT );
            zone->SetPlacementAreaSource( wxT( "Repeated group" ) );
            zone->SetDoNotAllowTracks( true );
            footprint->Add( zone );
            board.Add( footprint );
            const auto report = ClassifyBoardForDowngrade( &board, target, drop );
            BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), target.m_id == kicad9.m_id ? 1 : 0 );
            BOOST_CHECK( zone->GetPlacementAreaEnabled() );
            DowngradeFootprintInPlace( footprint, target, drop );
            BOOST_CHECK( zone->GetIsRuleArea() );
            BOOST_CHECK( zone->GetDoNotAllowTracks() );
            BOOST_CHECK( zone->GetPlacementAreaEnabled() == ( target.m_id == kicad10.m_id ) );
            BOOST_CHECK( zone->GetPlacementAreaSource()
                         == ( target.m_id == kicad10.m_id ? wxT( "Repeated group" ) : wxEmptyString ) );
        }
    }
}


BOOST_AUTO_TEST_CASE( GroupPlacementGuardIgnoresOrdinaryGroupsAndQuotedText )
{
    BOOST_CHECK( FindUnsupportedBoardToken( wxT( "(zone (placement (group \"SourceA\")))" ), kicad9 )
                 == wxT( "placement/group" ) );
    BOOST_CHECK( FindUnsupportedBoardToken( wxT( "(zone (placement (group \"SourceA\")))" ), kicad10 ).IsEmpty() );
    BOOST_CHECK( FindUnsupportedBoardToken( wxT( "(group \"Ordinary\" (members \"uuid\"))" ), kicad9 ).IsEmpty() );
    BOOST_CHECK( FindUnsupportedBoardToken( wxT( "(gr_text \"(zone (placement (group foo)))\")" ), kicad9 ).IsEmpty() );
}


BOOST_AUTO_TEST_CASE( InertGroupPlacementMetadataDoesNotRequireConsent )
{
    for( bool drop : { false, true } )
    {
        BOARD board;
        auto  emptyRuleArea = new ZONE( &board );
        emptyRuleArea->SetIsRuleArea( true );
        emptyRuleArea->SetPlacementAreaSourceType( PLACEMENT_SOURCE_T::GROUP_PLACEMENT );
        board.Add( emptyRuleArea );
        auto ordinaryZone = new ZONE( &board );
        ordinaryZone->SetPlacementAreaSourceType( PLACEMENT_SOURCE_T::GROUP_PLACEMENT );
        ordinaryZone->SetPlacementAreaEnabled( true );
        ordinaryZone->SetPlacementAreaSource( wxT( "Unused by ordinary copper zones" ) );
        board.Add( ordinaryZone );
        BOOST_CHECK( !ClassifyBoardForDowngrade( &board, kicad9, drop ).IsLossy() );
        DowngradeBoardInPlace( &board, kicad9, drop );
        BOOST_CHECK( emptyRuleArea->GetIsRuleArea() );
        BOOST_CHECK( !ordinaryZone->GetIsRuleArea() );
        BOOST_CHECK( emptyRuleArea->GetPlacementAreaSourceType() == PLACEMENT_SOURCE_T::SHEETNAME );
        BOOST_CHECK( ordinaryZone->GetPlacementAreaSourceType() == PLACEMENT_SOURCE_T::SHEETNAME );
    }
}


BOOST_AUTO_TEST_CASE( UnreviewedGeneratorKindBlocksBothPolicies )
{
    class FUTURE_GENERATOR : public PCB_VIA_STITCH
    {
    public:
        explicit FUTURE_GENERATOR( BOARD* aBoard ) :
                PCB_VIA_STITCH( aBoard )
        {
        }
        wxString GetGeneratorType() const override { return wxT( "future_generator" ); }
    };

    BOARD board;
    board.Add( new FUTURE_GENERATOR( &board ) );

    for( bool drop : { false, true } )
        BOOST_CHECK( ClassifyBoardForDowngrade( &board, kicad10, drop ).IsBlocked() );
}


BOOST_AUTO_TEST_CASE( LineEndingsLowerPrintedInkOrDropOnlyEmbellishments )
{
    for( bool drop : { false, true } )
    {
        BOARD      board;
        FOOTPRINT* fp = new FOOTPRINT( &board );
        board.Add( fp );
        std::vector<PCB_SHAPE*> shapes;
        double                  expectedArea = 0;

        for( BOARD_ITEM_CONTAINER* parent :
             { static_cast<BOARD_ITEM_CONTAINER*>( &board ), static_cast<BOARD_ITEM_CONTAINER*>( fp ) } )
        {
            PCB_SHAPE* shape = new PCB_SHAPE( parent, SHAPE_T::SEGMENT );
            shape->SetStart( VECTOR2I( 0, 0 ) );
            shape->SetEnd( VECTOR2I( 10000000, 0 ) );
            shape->SetWidth( 200000 );
            shape->SetLayer( F_SilkS );
            shape->SetEndEnding( LINE_ENDING( LINE_ENDING_STYLE::ARROW, 2000000, 1000000 ) );
            parent->Add( shape );
            shapes.push_back( shape );
            SHAPE_POLY_SET ink;
            shape->TransformWithLineEndingsToPolygon( ink, 0, 5000, ERROR_INSIDE );
            ink.Simplify();
            expectedArea += ink.Area();
        }

        const auto report = ClassifyBoardForDowngrade( &board, kicad10, drop );
        checkFeature( report, wxT( "Graphic line endings" ), drop ? DOWNGRADE_BUCKET::DROP : DOWNGRADE_BUCKET::LOWER,
                      2 );
        DowngradeBoardInPlace( &board, kicad10, drop );

        if( drop )
        {
            for( PCB_SHAPE* shape : shapes )
            {
                BOOST_CHECK( shape->GetShape() == SHAPE_T::SEGMENT );
                BOOST_CHECK( shape->GetEnd() == VECTOR2I( 10000000, 0 ) );
                BOOST_CHECK( shape->GetEndEndingStyle() == LINE_ENDING_STYLE::NONE );
            }
        }
        else
        {
            double area = 0;

            for( BOARD_ITEM* item : board.Drawings() )
                area += static_cast<PCB_SHAPE*>( item )->GetPolyShape().Area();

            for( BOARD_ITEM* item : fp->GraphicalItems() )
                area += static_cast<PCB_SHAPE*>( item )->GetPolyShape().Area();

            BOOST_CHECK_CLOSE( area, expectedArea, 0.05 );
        }
    }
}


BOOST_AUTO_TEST_CASE( DashedEndedLinesBlockApproximationButAllowDroppingEndings )
{
    BOARD      board;
    PCB_SHAPE* line = new PCB_SHAPE( &board, SHAPE_T::SEGMENT );
    line->SetStart( VECTOR2I( 0, 0 ) );
    line->SetEnd( VECTOR2I( 10000000, 0 ) );
    line->SetWidth( 200000 );
    line->SetLineStyle( LINE_STYLE::DASH );
    line->SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    board.Add( line );
    BOOST_CHECK( ClassifyBoardForDowngrade( &board, kicad10 ).IsBlocked() );
    BOOST_CHECK( !ClassifyBoardForDowngrade( &board, kicad10, true ).IsBlocked() );
    DowngradeBoardInPlace( &board, kicad10, true );
    BOOST_CHECK( line->GetLineStyle() == LINE_STYLE::DASH );
    BOOST_CHECK( line->GetEndEndingStyle() == LINE_ENDING_STYLE::NONE );
}


BOOST_AUTO_TEST_CASE( CustomPropertiesBakeOnlyAffectedVariablesBeforeRemoval )
{
    for( bool footprintOnly : { false, true } )
    {
        BOARD      board;
        FOOTPRINT* fp = new FOOTPRINT( &board );
        fp->SetReference( wxT( "42" ) );
        fp->SetCustomProperty( wxT( "Serial" ), wxT( "42" ) );
        fp->Value().SetText( wxT( "${PROPERTY.Reference}/${REFERENCE}" ) );
        fp->Reference().SetCustomProperty( wxT( "Owner" ), wxT( "Engineering" ) );
        PCB_TEXT* escaped = new PCB_TEXT( fp );
        escaped->SetText( wxT( "\\${PROPERTY.Serial}" ) );
        fp->Add( escaped );
        PCB_TEXT* expression = new PCB_TEXT( fp );
        expression->SetText( wxT( "@{${PROPERTY.Reference}+1}/${REFERENCE}" ) );
        fp->Add( expression );
        board.Add( fp );
        const auto report = ClassifyBoardForDowngrade( &board, kicad10 );
        checkFeature( report, wxT( "Custom properties" ), DOWNGRADE_BUCKET::DROP, 2 );
        BOOST_CHECK( !report.IsBlocked() );

        if( footprintOnly )
            DowngradeFootprintInPlace( fp, kicad10 );
        else
            DowngradeBoardInPlace( &board, kicad10 );

        BOOST_CHECK( !fp->HasCustomProperties() );
        BOOST_CHECK( !fp->Reference().HasCustomProperties() );
        BOOST_CHECK_EQUAL( fp->Value().GetText(), wxString( "42/${REFERENCE}" ) );
        BOOST_CHECK_EQUAL( expression->GetText(), wxString( "43/${REFERENCE}" ) );
        BOOST_CHECK_EQUAL( escaped->GetText(), wxString( "\\${PROPERTY.Serial}" ) );
    }
}


BOOST_AUTO_TEST_CASE( UnresolvedPropertyVariablesBlockBothPolicies )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->Value().SetText( wxT( "${PROPERTY.Missing}" ) );
    board.Add( fp );

    for( bool drop : { false, true } )
        BOOST_CHECK( ClassifyBoardForDowngrade( &board, kicad10, drop ).IsBlocked() );
}


BOOST_AUTO_TEST_CASE( UsedProjectVariablesBakeInFootprintContextAndUnusedDefinitionsDoNotBlock )
{
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_property_dependencies" );
    const wxString               projectPath = ( tmp.GetPath() / "source.kicad_pro" ).string();

    {
        std::ofstream project( projectPath.ToStdString() );
        project << R"({"meta":{"version":4},"text_variables":{"SERIAL":"${PROPERTY.Reference}","ALIAS":"${SERIAL}","UNUSED":"${PROPERTY.Missing}"}})";
    }

    SETTINGS_MANAGER settings;
    BOOST_REQUIRE( settings.LoadProject( projectPath, false ) );
    BOARD board;
    board.SetProject( settings.GetProject( projectPath ), true );

    std::vector<FOOTPRINT*> footprints;

    for( const wxString& serial : { wxT( "42" ), wxT( "73" ) } )
    {
        FOOTPRINT* fp = new FOOTPRINT( &board );
        fp->SetReference( serial );
        fp->SetCustomProperty( wxT( "Serial" ), serial );
        fp->Value().SetText( wxT( "${ALIAS}/${REFERENCE}" ) );
        board.Add( fp );
        footprints.push_back( fp );
    }

    BOOST_CHECK( !ClassifyBoardForDowngrade( &board, kicad10 ).IsBlocked() );
    DowngradeBoardInPlace( &board, kicad10 );
    BOOST_CHECK_EQUAL( footprints[0]->Value().GetText(), wxString( "42/${REFERENCE}" ) );
    BOOST_CHECK_EQUAL( footprints[1]->Value().GetText(), wxString( "73/${REFERENCE}" ) );

    footprints[0]->Value().SetText( wxT( "${UNUSED}" ) );
    BOOST_CHECK( ClassifyBoardForDowngrade( &board, kicad10 ).IsBlocked() );
    BOOST_CHECK( ClassifyBoardForDowngrade( &board, kicad10, true ).IsBlocked() );
}


BOOST_AUTO_TEST_CASE( CachedBoardVariableDependenciesBakeInFootprintContext )
{
    BOARD board;
    board.SetProperties( { { wxT( "SERIAL" ), wxT( "${PROPERTY.Reference}" ) },
                           { wxT( "UNUSED" ), wxT( "${PROPERTY.Missing}" ) } } );
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetReference( wxT( "42" ) );
    fp->Value().SetText( wxT( "${SERIAL}/${REFERENCE}" ) );
    board.Add( fp );
    BOOST_CHECK( !ClassifyBoardForDowngrade( &board, kicad10 ).IsBlocked() );
    DowngradeBoardInPlace( &board, kicad10 );
    BOOST_CHECK_EQUAL( fp->Value().GetText(), wxString( "42/${REFERENCE}" ) );
}


BOOST_AUTO_TEST_CASE( MixedPropertyTextKeepsEscapedLiteralsAndSupportedVariables )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetReference( wxT( "42" ) );
    fp->SetCustomProperty( wxT( "Serial" ), wxT( "42" ) );
    fp->SetKeywords( wxT( "\\${REFERENCE}/\\@{1+2}" ) );
    fp->Value().SetText( wxT( "${PROPERTY.Reference}/\\${U1:PROPERTY.Missing}/${REFERENCE}" ) );
    PCB_TEXT* text = new PCB_TEXT( fp );
    text->SetText( wxT( "${PROPERTY.Keywords}/${REFERENCE}" ) );
    fp->Add( text );
    board.Add( fp );
    const wxString shown = text->GetShownText( FOR_CANVAS );
    BOOST_CHECK( !ClassifyBoardForDowngrade( &board, kicad10 ).IsBlocked() );
    DowngradeBoardInPlace( &board, kicad10 );
    BOOST_CHECK_EQUAL( fp->Value().GetText(), wxString( "42/\\${U1:PROPERTY.Missing}/${REFERENCE}" ) );
    BOOST_CHECK( !text->GetText().Contains( wxT( "PROPERTY." ) ) );
    BOOST_CHECK_EQUAL( text->GetShownText( FOR_CANVAS ), shown );
}


BOOST_AUTO_TEST_CASE( NewExpressionUnitAliasIsBakedWithoutFreezingOtherVariables )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetReference( wxT( "U1" ) );
    fp->Value().SetText( wxT( "@{1000mils}/${REFERENCE}" ) );
    board.Add( fp );
    BOOST_CHECK( !ClassifyBoardForDowngrade( &board, kicad10 ).IsBlocked() );
    DowngradeBoardInPlace( &board, kicad10 );
    BOOST_CHECK_EQUAL( fp->Value().GetText(), wxString( "25.4/${REFERENCE}" ) );
}


BOOST_AUTO_TEST_CASE( UnresolvedAffectedExpressionsBlockBothPolicies )
{
    for( const wxString& raw :
         { wxT( "@{${PROPERTY.Reference}+${MISSING}}" ), wxT( "@{unknown_function(${PROPERTY.Reference})}" ) } )
    {
        BOARD      board;
        FOOTPRINT* fp = new FOOTPRINT( &board );
        fp->SetReference( wxT( "42" ) );
        fp->SetCustomProperty( wxT( "Serial" ), wxT( "42" ) );
        fp->Value().SetText( raw );
        board.Add( fp );

        for( bool drop : { false, true } )
            BOOST_CHECK( ClassifyBoardForDowngrade( &board, kicad10, drop ).IsBlocked() );
    }
}


BOOST_AUTO_TEST_CASE( EscapedTextControlsBlockKicad9AndRemainLiteralInKicad10 )
{
    for( bool inProperty : { false, true } )
    {
        BOARD      board;
        FOOTPRINT* fp = new FOOTPRINT( &board );
        fp->SetReference( wxT( "U1" ) );
        const wxString literal = wxT( "\\${REFERENCE}/\\@{1+2}" );
        fp->SetKeywords( literal );
        fp->Value().SetText( inProperty ? wxT( "${PROPERTY.Keywords}" ) : literal );
        board.Add( fp );
        const wxString shown = fp->Value().GetShownText( FOR_CANVAS );

        for( bool drop : { false, true } )
        {
            BOOST_CHECK( ClassifyBoardForDowngrade( &board, kicad9, drop ).IsBlocked() );
            BOOST_CHECK( !ClassifyBoardForDowngrade( &board, kicad10, drop ).IsBlocked() );
        }

        DowngradeBoardInPlace( &board, kicad10 );
        BOOST_CHECK_EQUAL( fp->Value().GetShownText( FOR_CANVAS ), shown );
    }
}


BOOST_AUTO_TEST_CASE( TitleBlockExpressionsBakeBeforeDependentFootprintText )
{
    for( bool drop : { false, true } )
    {
        BOARD board;
        board.GetTitleBlock().SetTitle( wxT( "@{1000mils}" ) );
        FOOTPRINT* fp = new FOOTPRINT( &board );
        fp->SetReference( wxT( "U1" ) );
        fp->Value().SetText( wxT( "${TITLE}/${REFERENCE}" ) );
        board.Add( fp );
        const auto report = ClassifyBoardForDowngrade( &board, kicad10, drop );
        BOOST_CHECK( !report.IsBlocked() );
        checkFeature( report, wxT( "Post-target text variables" ), DOWNGRADE_BUCKET::DROP, 2 );
        DowngradeBoardInPlace( &board, kicad10, drop );
        BOOST_CHECK_EQUAL( board.GetTitleBlock().GetTitle(), wxString( "25.4" ) );
        BOOST_CHECK_EQUAL( fp->Value().GetText(), wxString( "25.4/${REFERENCE}" ) );
    }
}


BOOST_AUTO_TEST_CASE( TitleBlockUnresolvablePropertyContextBlocksBothPolicies )
{
    BOARD board;
    board.GetTitleBlock().SetComment( 0, wxT( "${PROPERTY.Reference}" ) );

    for( bool drop : { false, true } )
        BOOST_CHECK( ClassifyBoardForDowngrade( &board, kicad10, drop ).IsBlocked() );
}


BOOST_AUTO_TEST_CASE( TitleBlockSelfReferenceDoesNotHideUsedProjectDependencies )
{
    KI_TEST::TEMPORARY_DIRECTORY temporary( "kicad_qa_title_dependencies" );
    const wxString               projectPath = ( temporary.GetPath() / "source.kicad_pro" ).string();

    {
        std::ofstream project( projectPath.ToStdString() );
        project << R"({"meta":{"version":4},"text_variables":{"TITLE":"${PROPERTY.Reference}"}})";
    }

    SETTINGS_MANAGER settings;
    BOOST_REQUIRE( settings.LoadProject( projectPath, false ) );
    BOARD board;
    board.SetProject( settings.GetProject( projectPath ), true );
    board.GetTitleBlock().SetTitle( wxT( "${TITLE}" ) );
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetReference( wxT( "42" ) );
    fp->Value().SetText( wxT( "${TITLE}" ) );
    board.Add( fp );
    BOOST_CHECK_EQUAL( fp->Value().GetShownText( FOR_CANVAS ), wxString( "42" ) );

    for( bool drop : { false, true } )
        BOOST_CHECK( ClassifyBoardForDowngrade( &board, kicad10, drop ).IsBlocked() );

    board.GetTitleBlock().SetTitle( wxT( "Ordinary title" ) );
    const auto report = ClassifyBoardForDowngrade( &board, kicad10 );
    BOOST_CHECK( !report.IsBlocked() );
    BOOST_CHECK( !report.IsLossy() );
}


BOOST_AUTO_TEST_CASE( AffectedTableExpressionsUseNativeCellCoordinates )
{
    BOARD      board;
    FOOTPRINT* fp = new FOOTPRINT( &board );
    fp->SetReference( wxT( "42" ) );
    board.Add( fp );
    PCB_TABLE* table = new PCB_TABLE( fp );
    table->SetColCount( 1 );
    table->SetColWidth( 0, 10000000 );
    table->SetRowHeight( 0, 6000000 );
    PCB_TABLECELL* cell = new PCB_TABLECELL( table );
    cell->SetText( wxT( "@{${PROPERTY.Reference}+${ROW}}/${COL}/${REFERENCE}" ) );
    table->AddCell( cell );
    fp->Add( table );
    BOOST_CHECK( !ClassifyBoardForDowngrade( &board, kicad10 ).IsBlocked() );
    DowngradeBoardInPlace( &board, kicad10 );
    BOOST_CHECK_EQUAL( cell->GetText(), wxString( "43/${COL}/${REFERENCE}" ) );
}


BOOST_AUTO_TEST_CASE( FootprintFileUsesTheSamePostMasterPoliciesAsBoardFootprints )
{
    SETTINGS_MANAGER             settingsManager;
    KI_TEST::TEMPORARY_DIRECTORY temporary( "kicad_qa_fp_post_master" );
    const wxString               source = ( temporary.GetPath() / "source.kicad_mod" ).string();
    FOOTPRINT                    footprint( nullptr );
    footprint.SetReference( wxT( "42" ) );
    footprint.Value().SetText( wxT( "${PROPERTY.Reference}/${REFERENCE}" ) );
    footprint.SetCustomProperty( wxT( "Owner" ), wxT( "QA" ) );
    footprint.SetExcludedFromSim( true );
    PCB_SHAPE* line = new PCB_SHAPE( &footprint, SHAPE_T::SEGMENT );
    line->SetStart( VECTOR2I( 0, 0 ) );
    line->SetEnd( VECTOR2I( 10000000, 0 ) );
    line->SetWidth( 200000 );
    line->SetLayer( F_SilkS );
    line->SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    footprint.Add( line );
    PCB_CONSTRAINT* constraint = new PCB_CONSTRAINT( &footprint, PCB_CONSTRAINT_TYPE::HORIZONTAL );
    constraint->AddMember( line->m_Uuid );
    footprint.Add( constraint );
    PCB_IO_KICAD_SEXPR io;
    io.FootprintSave( source, &footprint );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        for( bool drop : { false, true } )
        {
            const wxString destination = ( temporary.GetPath() / "out.kicad_mod" ).string();
            BOOST_REQUIRE( DowngradeFootprintFileToTemp( source, destination, target, drop )
                           == DOWNGRADE_FILE_RESULT::CONVERTED );
            wxString name;
            auto     converted = io.ImportFootprint( destination, name );
            BOOST_REQUIRE( converted );
            BOOST_CHECK( converted->Constraints().empty() );
            BOOST_CHECK( !converted->HasCustomProperties() );
            BOOST_CHECK( !converted->IsExcludedFromSim() );
            BOOST_CHECK_EQUAL( converted->Value().GetText(), wxString( "42/${REFERENCE}" ) );
            BOOST_REQUIRE( !converted->GraphicalItems().empty() );

            for( BOARD_ITEM* item : converted->GraphicalItems() )
            {
                BOOST_REQUIRE( item->Type() == PCB_SHAPE_T );
                const PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );
                BOOST_CHECK( shape->GetEndEndingStyle() == LINE_ENDING_STYLE::NONE );
                BOOST_CHECK( shape->GetShape() == ( drop ? SHAPE_T::SEGMENT : SHAPE_T::POLY ) );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( SimulationExclusionsFollowSelectedVariantAndReportTheirLoss )
{
    for( bool drop : { false, true } )
    {
        BOARD      board;
        FOOTPRINT* fp = new FOOTPRINT( &board );
        fp->SetExcludedFromSim( false );
        fp->AddVariant( wxT( "selected" ) )->SetExcludedFromSim( true );
        board.Add( fp );
        FlattenBoardVariant( &board, wxT( "selected" ) );
        BOOST_CHECK( fp->IsExcludedFromSim() );
        const auto report = ClassifyBoardForDowngrade( &board, kicad10, drop );
        checkFeature( report, wxT( "Footprint simulation exclusions" ), DOWNGRADE_BUCKET::DROP, 1 );
        DowngradeBoardInPlace( &board, kicad10, drop );
        BOOST_CHECK( !fp->IsExcludedFromSim() );
        for( const auto& [name, variant] : fp->GetVariants() )
            BOOST_CHECK( !variant.GetExcludedFromSim() );
    }
}


BOOST_AUTO_TEST_CASE( LegacyBoldWidthsPreserveNestedTextAndDoNotMutateSource )
{
    SETTINGS_MANAGER settingsManager;
    BOARD            board;
    FOOTPRINT*       fp = new FOOTPRINT( &board );
    board.Add( fp );
    std::vector<EDA_TEXT*> bold;
    PCB_TEXT*              text = new PCB_TEXT( &board );
    text->SetText( wxT( "board bold" ) );
    board.Add( text );
    bold.push_back( text );
    fp->Value().SetText( wxT( "footprint bold" ) );
    bold.push_back( &fp->Value() );
    PCB_TEXT* fpText = new PCB_TEXT( fp );
    fpText->SetText( wxT( "graphic bold" ) );
    fp->Add( fpText );
    bold.push_back( fpText );
    PCB_TABLE* table = new PCB_TABLE( fp, 100000 );
    table->SetColCount( 1 );
    table->SetColWidth( 0, 10000000 );
    table->SetRowHeight( 0, 5000000 );
    PCB_TABLECELL* cell = new PCB_TABLECELL( table );
    cell->SetText( wxT( "cell bold" ) );
    table->AddCell( cell );
    fp->Add( table );
    bold.push_back( cell );
    PCB_DIM_ALIGNED* dimension = new PCB_DIM_ALIGNED( &board );
    dimension->SetStart( VECTOR2I( 0, 0 ) );
    dimension->SetEnd( VECTOR2I( 10000000, 0 ) );
    dimension->SetHeight( 2000000 );
    dimension->ChangeOverrideText( wxT( "dimension bold" ) );
    board.Add( dimension );
    bold.push_back( dimension );

    for( EDA_TEXT* item : bold )
    {
        item->SetBold( true );
        item->SetTextThickness( 100000 );
    }

    PCB_TEXT* automatic = new PCB_TEXT( &board );
    automatic->SetText( wxT( "auto bold" ) );
    automatic->SetBold( true );
    automatic->SetTextThickness( 0 );
    board.Add( automatic );
    PCB_TEXT* outline = new PCB_TEXT( &board );
    outline->SetText( wxT( "outline bold" ) );
    outline->SetFont( KIFONT::FONT::GetFont( wxT( "DejaVu Sans" ), true ) );
    outline->SetBold( true );
    outline->SetTextThickness( 100000 );
    board.Add( outline );
    const bool                   outlineAvailable = outline->GetFont() && outline->GetFont()->IsOutline();
    KI_TEST::TEMPORARY_DIRECTORY temporary( "kicad_qa_legacy_bold_widths" );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        const std::string first = ( temporary.GetPath() / ( target.m_id.ToStdString() + "-first.kicad_pcb" ) ).string();
        const std::string second =
                ( temporary.GetPath() / ( target.m_id.ToStdString() + "-second.kicad_pcb" ) ).string();
        SaveBoardForTarget( &board, first, target );
        SaveBoardForTarget( &board, second, target );
        const std::string content = KI_TEST::ReadGoldenText( first );
        BOOST_CHECK( content.find( "(thickness 0.16)" ) != std::string::npos );
        BOOST_CHECK( content == KI_TEST::ReadGoldenText( second ) );

        for( EDA_TEXT* item : bold )
            BOOST_CHECK_EQUAL( item->GetTextThickness(), 100000 );

        BOOST_CHECK_EQUAL( automatic->GetTextThickness(), 0 );
        BOOST_CHECK_EQUAL( outline->GetTextThickness(), 100000 );
        PCB_IO_KICAD_SEXPR io;
        auto               reloaded = io.LoadBoard( first );
        BOOST_REQUIRE( reloaded );
        size_t checked = 0;
        reloaded->RunOnChildren(
                [&]( BOARD_ITEM* item )
                {
                    EDA_TEXT* reloadedText = dynamic_cast<EDA_TEXT*>( item );

                    if( !reloadedText || !reloadedText->IsBold() )
                        return;

                    if( reloadedText->GetText() == wxT( "auto bold" ) )
                        BOOST_CHECK_EQUAL( reloadedText->GetTextThickness(), 0 );
                    else if( reloadedText->GetText() != wxT( "outline bold" ) || outlineAvailable )
                        BOOST_CHECK_EQUAL( reloadedText->GetTextThickness(), 100000 );

                    ++checked;
                },
                RECURSE_MODE::RECURSE );
        BOOST_CHECK_EQUAL( checked, bold.size() + 2 );
    }
}


BOOST_AUTO_TEST_CASE( ComplexPadstackFrontAndInnerCustomOptionsRoundTripToKicad10 )
{
    SETTINGS_MANAGER settingsManager;
    BOARD            board;
    board.SetCopperLayerCount( 4 );
    FOOTPRINT* fp = new FOOTPRINT( &board );
    PAD*       pad = new PAD( fp );
    pad->SetAttribute( PAD_ATTRIB::PTH );
    pad->SetLayerSet( LSET::AllCuMask() );
    pad->Padstack().SetMode( PADSTACK::MODE::CUSTOM );
    pad->SetShape( F_Cu, PAD_SHAPE::OVAL );
    pad->SetSize( F_Cu, VECTOR2I( 2000000, 1000000 ) );
    pad->SetShape( In1_Cu, PAD_SHAPE::CUSTOM );
    pad->SetSize( In1_Cu, VECTOR2I( 500000, 500000 ) );
    pad->SetCustomShapeInZoneOpt( CUSTOM_SHAPE_ZONE_MODE::CONVEXHULL );
    PCB_SHAPE* primitive = new PCB_SHAPE( pad, SHAPE_T::SEGMENT );
    primitive->SetStart( VECTOR2I( -1000000, 0 ) );
    primitive->SetEnd( VECTOR2I( 1000000, 0 ) );
    primitive->SetWidth( 200000 );
    pad->AddPrimitive( In1_Cu, primitive );
    pad->SetDrillSize( VECTOR2I( 300000, 300000 ) );
    fp->Add( pad );
    board.Add( fp );
    KI_TEST::TEMPORARY_DIRECTORY temporary( "kicad_qa_complex_pad_front" );
    const std::string            path = ( temporary.GetPath() / "out.kicad_pcb" ).string();
    SaveBoardForTarget( &board, path, kicad10 );
    PCB_IO_KICAD_SEXPR io;
    auto               reloaded = io.LoadBoard( path );
    BOOST_REQUIRE( reloaded && reloaded->Footprints().size() == 1 );
    BOOST_REQUIRE_EQUAL( reloaded->Footprints()[0]->Pads().size(), 1 );
    const PAD* result = reloaded->Footprints()[0]->Pads()[0];
    BOOST_CHECK( result->GetShape( F_Cu ) == PAD_SHAPE::OVAL );
    BOOST_CHECK( result->GetSize( F_Cu ) == VECTOR2I( 2000000, 1000000 ) );
    BOOST_CHECK( result->GetShape( In1_Cu ) == PAD_SHAPE::CUSTOM );
    BOOST_REQUIRE_EQUAL( result->GetPrimitives( In1_Cu ).size(), 1 );
    BOOST_CHECK( result->GetCustomShapeInZoneOpt() == CUSTOM_SHAPE_ZONE_MODE::CONVEXHULL );
}


BOOST_AUTO_TEST_CASE( CurrentProjectSchemasAndSupportedDrcExclusionsMigrateForBothTargets )
{
    const nlohmann::json exclusion = { { "marker",
                                         { { "error_type", "DRCET_CLEARANCE" },
                                           { "position", { { "x_nm", "123" }, { "y_nm", "-456" } } },
                                           { "items",
                                             { { { "value", "11111111-1111-1111-1111-111111111111" } },
                                               { { "value", "22222222-2222-2222-2222-222222222222" } } } } } },
                                       { "comment", "accepted" } };

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        nlohmann::json doc = {
            { "meta", { { "version", 4 } } },
            { "board",
              { { "design_settings", { { "meta", { { "version", 3 } } }, { "drc_exclusions", { exclusion } } } } } }
        };
        COMPATIBILITY_REPORT report;
        DowngradeProjectFileJson( doc, target, report );
        BOOST_CHECK_EQUAL( doc["meta"]["version"].get<int>(), 3 );
        const auto& settings = doc["board"]["design_settings"];
        BOOST_CHECK_EQUAL( settings["meta"]["version"].get<int>(), 2 );
        BOOST_REQUIRE_EQUAL( settings["drc_exclusions"].size(), 1 );
        BOOST_CHECK_EQUAL(
                settings["drc_exclusions"][0][0].get<std::string>(),
                "clearance|123|-456|11111111-1111-1111-1111-111111111111|22222222-2222-2222-2222-222222222222" );
        BOOST_CHECK_EQUAL( settings["drc_exclusions"][0][1].get<std::string>(), "accepted" );
        BOOST_CHECK( !report.IsLossy() );
        const auto           migrated = doc;
        COMPATIBILITY_REPORT second;
        DowngradeProjectFileJson( doc, target, second );
        BOOST_CHECK( doc == migrated );
        BOOST_CHECK( !second.IsLossy() );
    }
}


BOOST_AUTO_TEST_CASE( UnrepresentableDrcExclusionsDropWithReports )
{
    nlohmann::json multiple = { { "marker",
                                  { { "error_type", "DRCET_CLEARANCE" },
                                    { "position", { { "x_nm", "123" }, { "y_nm", "-456" } } },
                                    { "items",
                                      { { { "value", "11111111-1111-1111-1111-111111111111" } },
                                        { { "value", "22222222-2222-2222-2222-222222222222" } },
                                        { { "value", "33333333-3333-3333-3333-333333333333" } } } } } },
                                { "comment", "three items" } };
    nlohmann::json unknown = multiple;
    unknown["marker"]["error_type"] = "DRCET_MICROVIA_STACK_DEPTH";
    unknown["marker"]["items"].erase( 2 );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        nlohmann::json doc = {
            { "board",
              { { "design_settings",
                  { { "meta", { { "version", 3 } } }, { "drc_exclusions", { multiple, unknown, "malformed" } } } } } }
        };
        COMPATIBILITY_REPORT report;
        DowngradeProjectFileJson( doc, target, report );
        BOOST_CHECK( doc["board"]["design_settings"]["drc_exclusions"].empty() );
        BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), 3 );
    }
}


BOOST_AUTO_TEST_CASE( PostMasterProjectSettingsReportOnlyMeaningfulLosses )
{
    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        nlohmann::json doc;
        doc["net_settings"]["net_chain_netclasses"] = { { { "name", "fast" } } };
        doc["board"]["design_settings"]["via_stack_presets"] = { { { "name", "fanout" } } };
        doc["board"]["design_settings"]["teardrop_parameters"] = {
            { { "td_target_name", "td_track_end" }, { "td_enabled", true }, { "td_max_len", 1.0 } },
            { { "td_enabled", false } }
        };
        COMPATIBILITY_REPORT report;
        DowngradeProjectFileJson( doc, target, report );
        BOOST_CHECK( !doc["net_settings"].contains( "net_chain_netclasses" ) );
        const auto& settings = doc["board"]["design_settings"];
        BOOST_CHECK( !settings.contains( "via_stack_presets" ) );
        BOOST_CHECK( !settings["teardrop_parameters"][0].contains( "td_enabled" ) );
        BOOST_CHECK( !settings["teardrop_parameters"][1].contains( "td_enabled" ) );
        BOOST_CHECK_EQUAL( settings["teardrop_parameters"][0]["td_max_len"].get<double>(), 1.0 );
        BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), 3 );
        COMPATIBILITY_REPORT second;
        DowngradeProjectFileJson( doc, target, second );
        BOOST_CHECK( !second.IsLossy() );

        nlohmann::json defaults;
        defaults["net_settings"]["net_chain_netclasses"] = nlohmann::json::array();
        defaults["board"]["design_settings"]["via_stack_presets"] = nlohmann::json::array();
        defaults["board"]["design_settings"]["teardrop_parameters"] = { { { "td_enabled", false } } };
        COMPATIBILITY_REPORT silent;
        DowngradeProjectFileJson( defaults, target, silent );
        BOOST_CHECK( !silent.IsLossy() );
    }
}


BOOST_AUTO_TEST_CASE( NewTuningBridgeDelayIsRemovedAndReportedForKicad10 )
{
    nlohmann::json doc = {
        { "tuning_profiles",
          { { "meta", { { "version", 2 } } },
            { "tuning_profiles_impedance_geometric",
              { { { "net_chain_bridge_prop_delay", 25 } }, { { "net_chain_bridge_prop_delay", 0 } } } } } }
    };
    COMPATIBILITY_REPORT report;
    DowngradeProjectFileJson( doc, kicad10, report );
    BOOST_CHECK_EQUAL( doc["tuning_profiles"]["meta"]["version"].get<int>(), 0 );
    BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), 1 );

    for( const auto& profile : doc["tuning_profiles"]["tuning_profiles_impedance_geometric"] )
        BOOST_CHECK( !profile.contains( "net_chain_bridge_prop_delay" ) );
}


BOOST_AUTO_TEST_CASE( NativeNetChainClassMapReportsOnlyNonemptyMappings )
{
    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        nlohmann::json doc;
        doc["net_settings"]["net_chain_classes"] = { { "clock", "Fast" } };
        COMPATIBILITY_REPORT report;
        DowngradeProjectFileJson( doc, target, report );
        BOOST_CHECK( !doc["net_settings"].contains( "net_chain_classes" ) );
        BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), 1 );
        COMPATIBILITY_REPORT second;
        DowngradeProjectFileJson( doc, target, second );
        BOOST_CHECK( !second.IsLossy() );
        doc["net_settings"]["net_chain_classes"] = nlohmann::json::object();
        COMPATIBILITY_REPORT empty;
        DowngradeProjectFileJson( doc, target, empty );
        BOOST_CHECK( !empty.IsLossy() );
    }
}


BOOST_AUTO_TEST_CASE( NewDrcConstraintsAndFunctionsAreFilteredWithoutLiteralFalsePositives )
{
    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        for( const wxString& keyword : { wxT( "microvia_aspect_ratio" ), wxT( "microvia_stack_depth" ) } )
        {
            const wxString rule = "(version 1)(rule \"new\" (constraint " + keyword + " (max 2)))";
            const auto     result = FilterDrcRulesForTarget( rule, target );
            BOOST_CHECK( result.m_error.IsEmpty() );
            BOOST_CHECK_EQUAL( result.m_dropped.size(), 1 );
        }

        for( const wxString& call :
             { wxT( "A.intersectsKeepout('x')" ), wxT( "A.intersectsArea('x')" ), wxT( "A.isStackedVia()" ),
               wxT( "A.ISSTACKEDVIA()" ), wxT( "A.customProperty('Owner') == 'Team'" ),
               wxT( "A.hasCustomProperty('Owner')" ) } )
        {
            for( const wxString& expression : { wxT( "condition" ), wxT( "assertion" ) } )
            {
                const wxString rule =
                        expression == wxT( "condition" )
                                ? "(rule \"new\" (condition \"" + call + "\") (constraint clearance (min 0.2mm)))"
                                : "(rule \"new\" (constraint assertion \"" + call + "\"))";
                const auto result = FilterDrcRulesForTarget( wxT( "(version 1)" ) + rule, target );
                BOOST_CHECK( result.m_error.IsEmpty() );
                BOOST_CHECK_EQUAL( result.m_dropped.size(), 1 );
            }
        }

        const wxString kept = wxT( "(rule \"customProperty\" (condition \"A.NetName == 'isStackedVia()'\") "
                                   "(constraint clearance (min 0.2mm)))\n"
                                   "(rule \"literal\" (condition \"A.NetName == 'intersectsArea()'\") "
                                   "(constraint clearance (min 0.2mm)))\n"
                                   "(rule \"uppercase literal\" (condition \"A.NetName == 'ISSTACKEDVIA()'\") "
                                   "(constraint clearance (min 0.2mm)))\n"
                                   "(rule \"legacy\" (condition \"A.insideArea('x')\") "
                                   "(constraint clearance (min 0.2mm)))\n"
                                   "(rule \"position\" (condition \"A.Start_X > 1mm && A.Start_Y > 1mm\") "
                                   "(constraint clearance (min 0.2mm)))" );
        const auto     result = FilterDrcRulesForTarget( wxT( "(version 1)\n" ) + kept, target );
        BOOST_CHECK( result.m_error.IsEmpty() );
        BOOST_CHECK( result.m_dropped.empty() );
        BOOST_CHECK( result.m_text.Contains( kept ) );
    }
}


BOOST_AUTO_TEST_CASE( DrcPostTargetPropertiesAreGuardedInConditionsAndAssertions )
{
    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        for( const wxString& kind : { wxT( "condition" ), wxT( "assertion" ) } )
        {
            const wxString expression = wxT( "A.Parent == A.Parent && A.NetName != 'A.parent.Reference'" );
            const wxString body =
                    kind == wxT( "condition" )
                            ? wxT( "(condition \"" ) + expression + wxT( "\") (constraint clearance (min 0.2mm))" )
                            : wxT( "(constraint assertion \"" ) + expression + wxT( "\")" );
            const auto parent =
                    FilterDrcRulesForTarget( wxT( "(version 1)(rule \"parent property\" " ) + body + ")", target );
            BOOST_CHECK( parent.m_error.IsEmpty() );
            BOOST_CHECK( parent.m_dropped.empty() );
        }

        for( const wxString& property :
             { wxT( "A.Start_Shape" ), wxT( "A.start_shape" ), wxT( "B.End_Width" ), wxT( "AB.Scale_X" ),
               wxT( "A.parent.Reference" ), wxT( "A . PARENT . Reference" ), wxT( "A.Major_Radius" ),
               wxT( "A.Exclude_From_Simulation" ), wxT( "A.Automatically_Update_Net" ), wxT( "A.Grid_Type" ) } )
        {
            for( const wxString& kind : { wxT( "condition" ), wxT( "assertion" ) } )
            {
                const wxString expression =
                        property + ( property.Contains( wxT( "Reference" ) ) ? wxT( " == 'U1'" ) : wxT( " == 1" ) );
                const wxString body =
                        kind == wxT( "condition" )
                                ? wxT( "(condition \"" ) + expression + wxT( "\") (constraint clearance (min 0.2mm))" )
                                : wxT( "(constraint assertion \"" ) + expression + wxT( "\")" );
                const auto result =
                        FilterDrcRulesForTarget( wxT( "(version 1)(rule \"props\" " ) + body + ")", target );
                BOOST_CHECK( result.m_error.IsEmpty() );
                BOOST_CHECK_EQUAL( result.m_dropped.size(), 1 );
            }
        }

        for( const wxString& property :
             { wxT( "A.Corner_Radius" ), wxT( "A.Auto_Thickness" ), wxT( "A.Target_Delay" ) } )
        {
            for( const wxString& kind : { wxT( "condition" ), wxT( "assertion" ) } )
            {
                const wxString expression = property + wxT( " == 1" );
                const wxString body =
                        kind == wxT( "condition" )
                                ? wxT( "(condition \"" ) + expression + wxT( "\") (constraint clearance (min 0.2mm))" )
                                : wxT( "(constraint assertion \"" ) + expression + wxT( "\")" );
                const auto result =
                        FilterDrcRulesForTarget( wxT( "(version 1)(rule \"older\" " ) + body + ")", target );
                BOOST_CHECK( result.m_error.IsEmpty() );
                BOOST_CHECK_EQUAL( result.m_dropped.size(), target.m_boardVersion == kicad9.m_boardVersion ? 1 : 0 );
            }
        }

        const wxString kept = wxT( "(rule \"literal\" (condition \"A.NetName == 'A.Start_Shape' && A.Start_X > 1mm "
                                   "&& A.End_Y > 1mm && A.Radius > 1mm\") (constraint clearance (min 0.2mm)))" );
        const auto     result = FilterDrcRulesForTarget( wxT( "(version 1)" ) + kept, target );
        BOOST_CHECK( result.m_error.IsEmpty() );
        BOOST_CHECK( result.m_dropped.empty() );
        BOOST_CHECK( result.m_text.Contains( kept ) );
    }
}


BOOST_AUTO_TEST_SUITE_END()
