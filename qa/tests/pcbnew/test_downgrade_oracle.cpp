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

// A downgraded board must use only node heads the target release knows, taken from its real
// keyword files under qa/data/downgrade.

#include <boost/test/unit_test.hpp>

#include <cctype>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <base_units.h>
#include <board.h>
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
#include <footprint.h>
#include <constraints/pcb_constraint.h>
#include <generators/pcb_via_stack.h>
#include <generators/pcb_via_stitch.h>
#include <io/kicad/legacy_format.h>
#include <mmh3_hash.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_barcode.h>
#include <pcb_group.h>
#include <pcb_grid_item.h>
#include <pcb_drill_chart.h>
#include <pcb_drill_map.h>
#include <pcb_point.h>
#include <pcb_reference_image.h>
#include <pcb_shape.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <zone.h>
#include <downgrade/board_downgrade.h>
#include <downgrade_scan.h>
#include <downgrade_target.h>
#include <settings/settings_manager.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <qa_utils/downgrade_oracle_utils.h>
#include <qa_utils/downgrade_golden_utils.h>


BOOST_AUTO_TEST_SUITE( DowngradeOracle )

static const DOWNGRADE_TARGET& kicad9 = KI_TEST::RequireDowngradeTarget( wxT( "9.0" ) );
static const DOWNGRADE_TARGET& kicad10 = KI_TEST::RequireDowngradeTarget( wxT( "10.0" ) );


static void checkExportUsesOnlyKnownTokens( const DOWNGRADE_TARGET& aTarget, const std::string& aTokenFileName )
{
    SETTINGS_MANAGER settingsManager;

    const std::string src = KI_TEST::GetPcbnewTestDataDir() + "via_off_center.kicad_pcb";

    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_downgrade_oracle", "" );
    const std::string            dest = ( tmp.GetPath() / "out.kicad_pcb" ).string();

    COMPATIBILITY_REPORT report;
    BOOST_REQUIRE( ExportBoardToOlderVersion( src, dest, aTarget, report ) );

    std::ifstream file( dest );
    std::string   content( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );

    const std::set<std::string> known =
            KI_TEST::LoadTokenSet( KI_TEST::GetPcbnewTestDataDir() + "../downgrade/" + aTokenFileName );
    BOOST_REQUIRE( !known.empty() );

    std::vector<std::string> unsupported = KI_TEST::UnknownSexprHeads( content, known );

    if( !unsupported.empty() )
    {
        std::string msg = "Downgraded board uses tokens " + aTarget.m_name.ToStdString() + " cannot parse:";

        for( const std::string& token : unsupported )
            msg += " " + token;

        BOOST_ERROR( msg );
    }
}


BOOST_AUTO_TEST_CASE( BoardToKicad9UsesOnlyKnownTokens )
{
    checkExportUsesOnlyKnownTokens( kicad9, "kicad9_board_tokens.txt" );
}


BOOST_AUTO_TEST_CASE( BoardToKicad10UsesOnlyKnownTokens )
{
    checkExportUsesOnlyKnownTokens( kicad10, "kicad10_board_tokens.txt" );
}


static PCB_REFERENCE_IMAGE* addTransformFeatures( BOARD& aBoard )
{
    NETINFO_ITEM* net = new NETINFO_ITEM( &aBoard, wxT( "TRANSFORM_SIGNAL" ) );
    net->SetNetChain( wxT( "TRANSFORM_CHAIN" ) );
    aBoard.Add( net );

    FOOTPRINT* fp = new FOOTPRINT( &aBoard );
    fp->EnsureExtrudedBody().m_height = pcbIUScale.mmToIU( 5 );
    fp->SetUnitInfo( { { wxT( "A" ), { wxT( "1" ) } }, { wxT( "B" ), { wxT( "2" ) } } } );
    fp->SetDuplicatePadNumbersAreJumpers( true );
    fp->AddVariant( wxT( "alternate" ) )->SetDNP( true );
    fp->SetExcludedFromSim( true );
    fp->SetCustomProperty( wxT( "Manufacturing owner" ), wxT( "QA" ) );
    aBoard.SetVariantNames( { wxT( "alternate" ) } );
    aBoard.Add( fp );

    PAD* pad = new PAD( fp );
    pad->SetNumber( wxT( "1" ) );
    pad->SetNet( net );
    pad->SetProperty( PAD_PROP::PRESSFIT );
    pad->SetSimElectricalType( PAD_SIM_ELECTRICAL_TYPE::SOURCE );
    pad->SetPadToDieDelay( 1000 );
    pad->Padstack().FrontPostMachining().mode = PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE;
    pad->Padstack().FrontPostMachining().size = pcbIUScale.mmToIU( 1.2 );
    pad->Padstack().FrontPostMachining().depth = pcbIUScale.mmToIU( 0.2 );
    fp->Add( pad );

    PAD* customPad = new PAD( fp );
    customPad->SetNumber( wxT( "2" ) );
    customPad->SetShape( F_Cu, PAD_SHAPE::CUSTOM );
    PCB_SHAPE* primitive = new PCB_SHAPE( customPad, SHAPE_T::RECTANGLE );
    primitive->SetStart( VECTOR2I( -1000000, -1000000 ) );
    primitive->SetEnd( VECTOR2I( 1000000, 1000000 ) );
    primitive->SetCornerRadius( pcbIUScale.mmToIU( 0.3 ) );
    primitive->SetFillMode( FILL_T::FILLED_SHAPE );
    customPad->AddPrimitive( F_Cu, primitive );
    fp->Add( customPad );

    aBoard.Add( new PCB_GRID_ITEM( &aBoard ) );
    PCB_DRILL_CHART* chart = new PCB_DRILL_CHART( &aBoard );
    chart->SetLayer( Dwgs_User );
    aBoard.Add( chart );
    PCB_DRILL_MAP* map = new PCB_DRILL_MAP( &aBoard );
    map->SetLayer( Dwgs_User );
    aBoard.Add( map );
    aBoard.GetDesignSettings().GetDrillSymbolProfile().SetName( wxT( "QA symbols" ) );

    for( PCB_GENERATOR* generator : { static_cast<PCB_GENERATOR*>( new PCB_VIA_STITCH( &aBoard ) ),
                                      static_cast<PCB_GENERATOR*>( new PCB_VIA_STACK( &aBoard ) ) } )
    {
        aBoard.Add( generator );
        PCB_VIA* via = new PCB_VIA( &aBoard );
        via->SetWidth( F_Cu, pcbIUScale.mmToIU( 0.6 ) );
        via->SetDrill( pcbIUScale.mmToIU( 0.3 ) );
        aBoard.Add( via );
        generator->AddItem( via );
    }

    for( BOARD_ITEM_CONTAINER* parent :
         { static_cast<BOARD_ITEM_CONTAINER*>( &aBoard ), static_cast<BOARD_ITEM_CONTAINER*>( fp ) } )
    {
        PCB_SHAPE* ended = new PCB_SHAPE( parent, SHAPE_T::SEGMENT );
        ended->SetStart( VECTOR2I( 0, 0 ) );
        ended->SetEnd( VECTOR2I( 10000000, 0 ) );
        ended->SetWidth( 200000 );
        ended->SetLayer( F_SilkS );
        ended->SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
        parent->Add( ended );
        PCB_CONSTRAINT* constraint = new PCB_CONSTRAINT( parent, PCB_CONSTRAINT_TYPE::HORIZONTAL );
        constraint->AddMember( ended->m_Uuid );
        parent->Add( constraint );

        for( SHAPE_T type : { SHAPE_T::ELLIPSE, SHAPE_T::ELLIPSE_ARC } )
        {
            PCB_SHAPE* ellipse = new PCB_SHAPE( parent, type );
            ellipse->SetEllipseCenter( VECTOR2I( 0, 0 ) );
            ellipse->SetEllipseMajorRadius( pcbIUScale.mmToIU( 5 ) );
            ellipse->SetEllipseMinorRadius( pcbIUScale.mmToIU( 3 ) );
            ellipse->SetEllipseStartAngle( ANGLE_0 );
            ellipse->SetEllipseEndAngle( ANGLE_90 );
            ellipse->SetWidth( pcbIUScale.mmToIU( 0.3 ) );
            ellipse->SetLayer( F_SilkS );
            parent->Add( ellipse );
        }

        PCB_SHAPE* rounded = new PCB_SHAPE( parent, SHAPE_T::RECTANGLE );
        rounded->SetStart( VECTOR2I( 0, 0 ) );
        rounded->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 6 ) ) );
        rounded->SetCornerRadius( pcbIUScale.mmToIU( 1 ) );
        rounded->SetLayer( F_SilkS );
        parent->Add( rounded );

        PCB_SHAPE* hatch = new PCB_SHAPE( parent, SHAPE_T::RECTANGLE );
        hatch->SetStart( VECTOR2I( 0, 0 ) );
        hatch->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 6 ) ) );
        hatch->SetFillMode( FILL_T::HATCH );
        hatch->SetLayer( F_SilkS );
        parent->Add( hatch );

        PCB_SHAPE* copperHatch = new PCB_SHAPE( parent, SHAPE_T::RECTANGLE );
        copperHatch->SetStart( VECTOR2I( pcbIUScale.mmToIU( 20 ), 0 ) );
        copperHatch->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 6 ) ) );
        copperHatch->SetFillMode( FILL_T::HATCH );
        copperHatch->SetLayer( F_Cu );
        parent->Add( copperHatch );

        PCB_BARCODE* barcode = new PCB_BARCODE( parent );
        barcode->SetText( wxT( "TEST123" ) );
        barcode->SetWidth( pcbIUScale.mmToIU( 10 ) );
        barcode->SetHeight( pcbIUScale.mmToIU( 10 ) );
        barcode->SetLayer( F_SilkS );
        parent->Add( barcode );

        PCB_TEXTBOX* textbox = new PCB_TEXTBOX( parent );
        textbox->SetText( wxT( "Knockout" ) );
        textbox->SetIsKnockout( true );
        textbox->SetStart( VECTOR2I( 0, 0 ) );
        textbox->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 6 ) ) );
        textbox->SetLayer( F_SilkS );
        parent->Add( textbox );

        PCB_TABLE* table = new PCB_TABLE( parent, pcbIUScale.mmToIU( 0.1 ) );
        table->SetColCount( 1 );
        table->SetColWidth( 0, pcbIUScale.mmToIU( 10 ) );
        table->SetRowHeight( 0, pcbIUScale.mmToIU( 6 ) );
        table->SetLayer( F_SilkS );
        PCB_TABLECELL* cell = new PCB_TABLECELL( table );
        cell->SetText( wxT( "Knockout cell" ) );
        cell->SetIsKnockout( true );
        cell->SetStart( VECTOR2I( 0, 0 ) );
        cell->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 6 ) ) );
        table->AddCell( cell );
        parent->Add( table );

        ZONE* zone = new ZONE( parent );
        zone->SetLayer( F_Cu );
        zone->AppendCorner( VECTOR2I( 0, 0 ), -1 );
        zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 10 ), 0 ), -1 );
        zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ), -1 );
        zone->LayerProperties()[F_Cu].hatching_offset = VECTOR2I( 100, 100 );
        parent->Add( zone );
        parent->Add( new PCB_POINT( parent, VECTOR2I( 0, 0 ), pcbIUScale.mmToIU( 1 ) ) );

        PCB_GROUP* group = new PCB_GROUP( parent );
        group->SetName( wxT( "transform group" ) );
        group->SetDesignBlockLibId( LIB_ID( wxT( "Lib" ), wxT( "Block" ) ) );
        group->AddItem( hatch );
        group->AddItem( barcode );
        parent->Add( group );
    }

    PCB_VIA* skipVia = new PCB_VIA( &aBoard );
    skipVia->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.6 ) );
    skipVia->SetDrill( pcbIUScale.mmToIU( 0.3 ) );
    skipVia->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::START_END_ONLY );
    skipVia->Padstack().Drill().is_capped = true;
    aBoard.Add( skipVia );

    PCB_VIA* buriedVia = new PCB_VIA( &aBoard );
    buriedVia->SetViaType( VIATYPE::BURIED );
    buriedVia->SetLayerPair( In1_Cu, In2_Cu );
    buriedVia->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.6 ) );
    buriedVia->SetDrill( pcbIUScale.mmToIU( 0.3 ) );
    aBoard.Add( buriedVia );

    aBoard.GetDesignSettings().m_ZoneLayerProperties[F_Cu].hatching_offset = VECTOR2I( 100, 100 );

    for( BOARD_STACKUP_ITEM* item : aBoard.GetDesignSettings().GetStackupDescriptor().GetList() )
    {
        if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
            item->SetDielectricModel( DIELECTRIC_MODEL::DJORDJEVIC_SARKAR );
    }

    PCB_REFERENCE_IMAGE* image = new PCB_REFERENCE_IMAGE( &aBoard );
    wxMemoryBuffer       png = KI_TEST::MakePngWithFractionalPixelsPerCm();
    BOOST_REQUIRE( image->GetReferenceImage().ReadImageFile( png ) );
    image->GetReferenceImage().SetImageScale( 2.0 );
    aBoard.Add( image );

    for( BOARD_ITEM* item : aBoard.Drawings() )
    {
        if( item->Type() == PCB_DRILL_CHART_T )
            static_cast<PCB_DRILL_CHART*>( item )->RebuildCells( aBoard );
    }

    return image;
}


static void checkBoardTransformCoverage( const COMPATIBILITY_REPORT& aReport, const DOWNGRADE_TARGET& aTarget,
                                         bool aDropInsteadOfApproximate )
{
    // Dates are explicit expectations, independent of the production rule table.
    KI_TEST::CheckTransformCoverage(
            aReport, aTarget.m_boardVersion,
            {
                    { wxT( "Geometric constraints" ), 20260624, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Local grids" ), 20260728, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Via stitching and guarding" ), 20260816, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Microvia stack generators" ), 20260830, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Graphic line endings" ), 20260818, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "Footprint simulation exclusions" ), 20260828, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Custom properties" ), 20260831, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Drill charts and maps" ), 20260901, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Drill symbol configuration" ), 20260901, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Net chains" ), 20260512, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Ellipse graphics" ), 20260508, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "Extruded 3D bodies" ), 20260410, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Pad simulation types" ), 20260521, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Footprint unit metadata" ), 20250909, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Barcodes" ), 20250914, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "PCB variants" ), 20260101, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Jumper pad groups" ), 20250324, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Counterbore / countersink" ), 20251101, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Press-fit pads" ), 20250811, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Skip vias" ), 20250801, DOWNGRADE_BUCKET::LOWER, true },
                    { wxT( "Buried vias" ), 20250926, DOWNGRADE_BUCKET::LOWER, true },
                    { wxT( "Zone layer defaults" ), 20250302, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Pad-to-die delays" ), 20250401, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Hatched shape fills" ), 20250222, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "Hatched copper fills" ), 20250222, DOWNGRADE_BUCKET::LOWER, true },
                    { wxT( "PCB points" ), 20250901, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Via protection" ), 20250228, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Dielectric frequency models" ), 20260511, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Group design block links" ), 20250513, DOWNGRADE_BUCKET::DROP },
                    { wxT( "Knockout text boxes" ), 20250210, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "Rounded rectangles" ), 20250829, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "Knockout table cells" ), 20260603, DOWNGRADE_BUCKET::LOWER },
                    { wxT( "Per-zone layer properties" ), 20250302, DOWNGRADE_BUCKET::DROP },
            },
            aDropInsteadOfApproximate );
}


static void checkTransformedBoardUsesOnlyKnownTokens( const DOWNGRADE_TARGET& aTarget,
                                                      const std::string&      aTokenFileName )
{
    SETTINGS_MANAGER            settingsManager;
    const std::set<std::string> known =
            KI_TEST::LoadTokenSet( KI_TEST::GetPcbnewTestDataDir() + "../downgrade/" + aTokenFileName );
    BOOST_REQUIRE( !known.empty() );

    for( bool drop : { false, true } )
    {
        BOOST_TEST_CONTEXT( "Drop approximations: " << drop )
        {
            KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_board_transform_oracle" );
            const std::string            src = ( tmp.GetPath() / "source.kicad_pcb" ).string();
            const std::string            dest = ( tmp.GetPath() / "out.kicad_pcb" ).string();
            PCB_IO_KICAD_SEXPR           io;
            std::unique_ptr<BOARD>       board(
                    io.LoadBoard( KI_TEST::GetPcbnewTestDataDir() + "via_off_center.kicad_pcb", nullptr ) );
            BOOST_REQUIRE( board );
            PCB_REFERENCE_IMAGE*   image = addTransformFeatures( *board );
            const REFERENCE_IMAGE& ref = image->GetReferenceImage();
            BOOST_REQUIRE_NE( ref.GetImage().GetPPI(), ref.GetImage().GetLegacyPPI() );
            const double expectedScale = ref.GetImageScale() * ref.GetImage().GetLegacyPPI() / ref.GetImage().GetPPI();
            checkBoardTransformCoverage( ClassifyBoardForDowngrade( board.get(), aTarget, drop ), aTarget, drop );
            io.SaveBoard( src, *board );
            const std::string sourceContent = KI_TEST::ReadGoldenText( src );

            COMPATIBILITY_REPORT report;
            BOOST_REQUIRE( ExportBoardToOlderVersion( src, dest, aTarget, report, nullptr, wxEmptyString, drop ) );
            checkBoardTransformCoverage( report, aTarget, drop );

            const std::string content = KI_TEST::ReadGoldenText( dest );
            const auto        unsupported = KI_TEST::UnknownSexprHeads( content, known );

            for( const std::string& token : unsupported )
                BOOST_ERROR( "Transformed board uses unknown token: " + token );

            BOOST_CHECK( content.find( "(version " + std::to_string( aTarget.m_boardVersion ) + ")" )
                         != std::string::npos );
            BOOST_CHECK_CLOSE( KI_TEST::SerializedImageScale( content, image->m_Uuid.AsStdString() ), expectedScale,
                               0.1 );
            std::unique_ptr<BOARD> reloaded( io.LoadBoard( dest, nullptr ) );
            BOOST_REQUIRE( reloaded );
            const auto reloadReport = ClassifyBoardForDowngrade( reloaded.get(), aTarget, drop );
            BOOST_CHECK( !reloadReport.IsBlocked() );

            // Current readers reconstruct false IPC flags in legacy files. These in-memory
            // upgrades do not mean newer syntax was written.
            if( aTarget.m_id == wxT( "9.0" ) )
            {
                size_t viaCount = 0;
                size_t buriedCount = 0;

                for( PCB_TRACK* track : reloaded->Tracks() )
                {
                    if( track->Type() != PCB_VIA_T )
                        continue;

                    const PCB_VIA*  via = static_cast<const PCB_VIA*>( track );
                    const PADSTACK& padstack = via->Padstack();
                    ++viaCount;
                    BOOST_CHECK( padstack.FrontOuterLayers().has_covering == false );
                    BOOST_CHECK( padstack.BackOuterLayers().has_covering == false );
                    BOOST_CHECK( padstack.FrontOuterLayers().has_plugging == false );
                    BOOST_CHECK( padstack.BackOuterLayers().has_plugging == false );
                    BOOST_CHECK( padstack.Drill().is_filled == false );
                    BOOST_CHECK( padstack.Drill().is_capped == false );

                    if( via->IsBuriedVia() )
                    {
                        ++buriedCount;
                        BOOST_CHECK( via->GetViaType() == VIATYPE::BLIND );
                        BOOST_CHECK( via->TopLayer() == In1_Cu );
                        BOOST_CHECK( via->BottomLayer() == In2_Cu );
                    }
                }

                BOOST_CHECK_EQUAL( buriedCount, 1 );

                for( const COMPAT_ENTRY& entry : reloadReport.Entries() )
                {
                    if( entry.m_feature == wxT( "Via protection" ) )
                        BOOST_CHECK_EQUAL( entry.m_count, viaCount );
                    else
                        BOOST_ERROR( "Unexpected feature after reload: " + entry.m_feature.ToStdString() );
                }
            }
            else
            {
                BOOST_CHECK( !reloadReport.IsLossy() );
            }

            BOOST_CHECK( KI_TEST::ReadGoldenText( src ) == sourceContent );
        }
    }
}


BOOST_AUTO_TEST_CASE( TransformedBoardToKicad9UsesOnlyKnownTokens )
{
    checkTransformedBoardUsesOnlyKnownTokens( kicad9, "kicad9_board_tokens.txt" );
}


BOOST_AUTO_TEST_CASE( TransformedBoardToKicad10UsesOnlyKnownTokens )
{
    checkTransformedBoardUsesOnlyKnownTokens( kicad10, "kicad10_board_tokens.txt" );
}


static void checkNativeBoardGolden( const DOWNGRADE_TARGET& aTarget, const std::string& aRelease,
                                    const std::string& aBoard, const std::string& aReference, size_t aExpectedDrops,
                                    size_t aExpectedLowerings, bool aDropInsteadOfApproximate = false )
{
    SETTINGS_MANAGER             settingsManager;
    const std::string            fixture = KI_TEST::GetPcbnewTestDataDir() + "../downgrade/golden/";
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_native_golden", "" );
    const std::string            dest = ( tmp.GetPath() / ( aBoard + ".kicad_pcb" ) ).string();
    COMPATIBILITY_REPORT         report;

    // Expected files were saved by native KiCad, never by the exporter under test.
    BOOST_REQUIRE( ExportBoardToOlderVersion( fixture + "current/" + aBoard + ".kicad_pcb", dest, aTarget, report,
                                              nullptr, wxEmptyString, aDropInsteadOfApproximate ) );
    BOOST_CHECK( !report.IsBlocked() );
    BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), aExpectedDrops );
    BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::LOWER ), aExpectedLowerings );
    const std::string difference =
            KI_TEST::GoldenFileDifference( fixture + aRelease + "/" + aReference + ".kicad_pcb", dest );
    BOOST_CHECK_MESSAGE( difference.empty(), difference );
}


BOOST_AUTO_TEST_CASE( BoardMatchesNativeKicad9Golden )
{
    // KiCad 9 drops the explicit disabled via-protection overrides without changing the via.
    checkNativeBoardGolden( kicad9, "v9", "geometry", "geometry", 1, 0 );
}


BOOST_AUTO_TEST_CASE( BoardMatchesNativeKicad10Golden )
{
    checkNativeBoardGolden( kicad10, "v10", "geometry", "geometry", 0, 0 );
}


BOOST_AUTO_TEST_CASE( LoweredGraphicsMatchNativeKicad9Golden )
{
    checkNativeBoardGolden( kicad9, "v9", "graphics", "graphics", 0, 2 );
}


BOOST_AUTO_TEST_CASE( GraphicsMatchNativeKicad10Golden )
{
    checkNativeBoardGolden( kicad10, "v10", "graphics", "graphics", 0, 0 );
}


BOOST_AUTO_TEST_CASE( DroppedGraphicsMatchNativeKicad9Golden )
{
    checkNativeBoardGolden( kicad9, "v9", "graphics", "graphics-drop", 2, 0, true );
}


BOOST_AUTO_TEST_CASE( DropApproximationsKeepsNativeKicad10GraphicsGolden )
{
    checkNativeBoardGolden( kicad10, "v10", "graphics", "graphics", 0, 0, true );
}


BOOST_AUTO_TEST_CASE( RequiredReleaseLookupFailsForMissingId )
{
    BOOST_CHECK( KI_TEST::RequireDowngradeTarget( wxT( "9.0" ) ).m_id == wxT( "9.0" ) );
    BOOST_CHECK( KI_TEST::RequireDowngradeTarget( wxT( "10.0" ) ).m_id == wxT( "10.0" ) );
    BOOST_CHECK_THROW( KI_TEST::RequireDowngradeTarget( wxT( "missing-release" ) ), std::runtime_error );
}


// Guard the oracle itself: normalization must not hide meaningful design changes.
BOOST_AUTO_TEST_CASE( GoldenComparisonDetectsDesignAndSettingsChanges )
{
    SEXPR::PARSER     parser;
    const std::string golden = "(kicad_pcb (version 20241229) (generator_version \"9.0\") "
                               "(setup (pad_to_mask_clearance 0.05)) "
                               "(pad \"1\" (at 100 100 30) (size 2 1) (net 1 \"SIGNAL\")))";
    auto              expected = parser.Parse( golden );

    for( const auto& [from, to] : std::vector<std::pair<std::string, std::string>>{ { "100 100", "0 0" },
                                                                                    { "(size 2 1)", "(size 1 2)" },
                                                                                    { "SIGNAL", "GND" },
                                                                                    { "0.05", "0.1" },
                                                                                    { "100 100 30", "100 100 31" },
                                                                                    { "20241229", "20260206" } } )
    {
        std::string changed = golden;
        changed.replace( changed.find( from ), from.size(), to );
        auto actual = parser.Parse( changed );
        BOOST_CHECK( !KI_TEST::GoldenSexprDifference( *expected, *actual ).empty() );
    }

    auto metadataOnly = parser.Parse( "(kicad_pcb (version 20241229) (generator_version \"10.99\") "
                                      "(setup (pad_to_mask_clearance 0.05)) "
                                      "(pad \"1\" (at 100.000001 99.999999 30) (size 2 1) (net 1 \"SIGNAL\")))" );
    BOOST_CHECK( KI_TEST::GoldenSexprDifference( *expected, *metadataOnly ).empty() );

    const std::string plotGolden = "(kicad_pcb (setup (pcbplotparams (hpglpennumber 1) (hpglpenspeed 20) "
                                   "(hpglpendiameter 15) (plotinvisibletext no))))";
    auto              plotExpected = parser.Parse( plotGolden );
    auto modernPlot = parser.Parse( "(kicad_pcb (setup (pcbplotparams (pngdpi 300) (pngantialias yes))))" );
    BOOST_CHECK( KI_TEST::GoldenSexprDifference( *plotExpected, *modernPlot ).empty() );

    for( const auto& [from, to] :
         std::vector<std::pair<std::string, std::string>>{ { "hpglpennumber 1", "hpglpennumber 2" },
                                                           { "hpglpenspeed 20", "hpglpenspeed 30" },
                                                           { "hpglpendiameter 15", "hpglpendiameter 16" },
                                                           { "plotinvisibletext no", "plotinvisibletext yes" } } )
    {
        std::string changed = plotGolden;
        changed.replace( changed.find( from ), from.size(), to );
        auto nonDefault = parser.Parse( changed );
        BOOST_CHECK( !KI_TEST::GoldenSexprDifference( *nonDefault, *modernPlot ).empty() );
    }
}


BOOST_AUTO_TEST_CASE( HistoricalPlotSettingsDoNotUseCurrentFormatters )
{
    BOARD                        board;
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_historical_plot", "" );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        const wxString path = ( tmp.GetPath() / ( target.m_id.ToStdString() + ".kicad_pcb" ) ).string();
        SaveBoardForTarget( &board, path, target );
        std::ifstream file( path.ToStdString() );
        std::string   text( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );
        BOOST_CHECK( text.find( "(pngdpi " ) == std::string::npos );
        BOOST_CHECK( text.find( "(pngantialias " ) == std::string::npos );

        if( target.m_id == wxT( "9.0" ) )
        {
            BOOST_CHECK( text.find( "(hpglpennumber 1)" ) != std::string::npos );
            BOOST_CHECK( text.find( "(hpglpenspeed 20)" ) != std::string::npos );
            BOOST_CHECK( text.find( "(hpglpendiameter 15.000000)" ) != std::string::npos );
            BOOST_CHECK( text.find( "(plotinvisibletext no)" ) != std::string::npos );
        }
    }
}


BOOST_AUTO_TEST_CASE( HistoricalCustomPaperUsesReleasePrecision )
{
    BOARD     board;
    PAGE_INFO page( PAGE_SIZE_TYPE::User );
    page.SetWidthMM( 123.456789 );
    page.SetHeightMM( 100.123456 );
    board.SetPageSettings( page );
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_historical_paper", "" );

    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        const wxString path = ( tmp.GetPath() / ( target.m_id.ToStdString() + ".kicad_pcb" ) ).string();
        SaveBoardForTarget( &board, path, target );
        std::ifstream     file( path.ToStdString() );
        std::string       text( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() );
        const std::string expected = target.m_id == wxT( "9.0" ) ? "(paper \"User\" 123.457 100.123)"
                                                                 : "(paper \"User\" 123.456789 100.123456)";
        BOOST_CHECK( text.find( expected ) != std::string::npos );
    }
}


BOOST_AUTO_TEST_CASE( UnsupportedPlotSelectionIsReportedAndReset )
{
    for( const DOWNGRADE_TARGET& target : { kicad9, kicad10 } )
    {
        for( bool drop : { false, true } )
        {
            BOARD           board;
            PCB_PLOT_PARAMS options = board.GetPlotOptions();
            options.SetFormat( PLOT_FORMAT::PNG );
            board.SetPlotOptions( options );
            COMPATIBILITY_REPORT report = ClassifyBoardForDowngrade( &board, target, drop );
            BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), 1 );
            BOOST_CHECK( !report.IsBlocked() );
            BOOST_CHECK( board.GetPlotOptions().GetFormat() == PLOT_FORMAT::PNG );
            DowngradeBoardInPlace( &board, target, drop );
            BOOST_CHECK( board.GetPlotOptions().GetFormat() == PLOT_FORMAT::GERBER );
            BOOST_CHECK( !ClassifyBoardForDowngrade( &board, target, drop ).IsLossy() );
        }
    }
}


static std::string historicalTextEffects( const EDA_TEXT& aText, bool aV9, int aControlBits = 0 )
{
    STRING_FORMATTER formatter;

    if( aV9 )
        KICAD_FORMAT::LEGACY::FormatTextV9( &formatter, aText, pcbIUScale, aControlBits );
    else
        KICAD_FORMAT::LEGACY::FormatTextV10( &formatter, aText, pcbIUScale, aControlBits );

    return formatter.GetString();
}


BOOST_AUTO_TEST_CASE( HistoricalTextEffectsKeepReleaseHideAndAttributes )
{
    EDA_TEXT text( pcbIUScale );
    text.SetFont( nullptr );
    text.SetTextSize( VECTOR2I( 1000000, 2000000 ) );
    text.SetTextThickness( 100000 );
    text.SetLineSpacing( 1.25 );
    text.SetItalic( true );
    text.SetTextColor( KIGFX::COLOR4D( 0.2, 0.4, 0.6, 0.75 ) );
    text.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    text.SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    text.SetMirrored( true );
    text.SetVisible( false );
    text.SetHyperlink( wxT( "https://example.com/datasheet" ) );

    const std::string prefix = "(effects(font(size 2 1)(line_spacing 1.25)(thickness 0.1)"
                               "(italic yes)(color 51 102 153 0.75))(justify left bottom mirror)";
    const std::string href = "(href \"https://example.com/datasheet\"))";
    BOOST_CHECK_EQUAL( historicalTextEffects( text, true ), prefix + "(hide yes)" + href );
    BOOST_CHECK_EQUAL( historicalTextEffects( text, false ), prefix + href );
    BOOST_CHECK_EQUAL( historicalTextEffects( text, true, KICAD_FORMAT::LEGACY::OMIT_HIDE ), prefix + href );

    for( bool v9 : { false, true } )
    {
        const std::string omitted = historicalTextEffects( text, v9, ( 1 << 11 ) | ( 1 << 12 ) );
        BOOST_CHECK( omitted.find( "(color " ) == std::string::npos );
        BOOST_CHECK( omitted.find( "(href " ) == std::string::npos );
        BOOST_CHECK( omitted.find( "(justify left bottom mirror)" ) != std::string::npos );
    }

    BOOST_CHECK_EQUAL( text.GetTextThickness(), 100000 );
    BOOST_CHECK_EQUAL( text.GetLineSpacing(), 1.25 );
    BOOST_CHECK( !text.IsVisible() );
}


BOOST_AUTO_TEST_CASE( HistoricalTextThicknessKeepsAutomaticZeroAndExplicitWidths )
{
    EDA_TEXT text( pcbIUScale );
    text.SetFont( nullptr );
    text.SetTextSize( VECTOR2I( 1000000, 1000000 ) );

    for( bool v9 : { false, true } )
    {
        text.SetBold( false );
        text.SetTextThickness( 0 );
        BOOST_CHECK( text.GetAutoThickness() );
        BOOST_CHECK_EQUAL( historicalTextEffects( text, v9 ), "(effects(font(size 1 1)))" );

        text.SetTextThickness( 1 );
        BOOST_CHECK( !text.GetAutoThickness() );
        BOOST_CHECK_EQUAL( historicalTextEffects( text, v9 ), "(effects(font(size 1 1)(thickness 0.000001)))" );
        BOOST_CHECK_EQUAL( text.GetTextThickness(), 1 );

        text.SetTextThickness( 100000 );
        BOOST_CHECK_EQUAL( historicalTextEffects( text, v9 ), "(effects(font(size 1 1)(thickness 0.1)))" );

        text.SetBold( true );
        BOOST_CHECK_EQUAL( historicalTextEffects( text, v9 ), "(effects(font(size 1 1)(thickness 0.16)(bold yes)))" );
        BOOST_CHECK_EQUAL( text.GetTextThickness(), 100000 );

        text.SetTextThickness( 0 );
        BOOST_CHECK_EQUAL( historicalTextEffects( text, v9 ), "(effects(font(size 1 1)(bold yes)))" );
        BOOST_CHECK_EQUAL( text.GetTextThickness(), 0 );
    }
}


BOOST_AUTO_TEST_CASE( HistoricalDefaultTextFormattingKeepsNative10AutomaticWidthPredicate )
{
    EDA_TEXT text( pcbIUScale );
    text.SetFont( nullptr );
    text.SetMultilineAllowed( false );
    text.SetTextSize( VECTOR2I( 1000000, 1000000 ) );
    text.SetAutoThickness( true );
    text.SetVisible( false );
    BOOST_CHECK( text.GetAutoThickness() );
    BOOST_CHECK( KICAD_FORMAT::LEGACY::IsDefaultTextV10( text ) );

    text.SetTextThickness( 1 );
    BOOST_CHECK( !text.GetAutoThickness() );
    BOOST_CHECK( !KICAD_FORMAT::LEGACY::IsDefaultTextV10( text ) );

    text.SetTextThickness( 0 );
    BOOST_CHECK( text.GetAutoThickness() );
    BOOST_CHECK( KICAD_FORMAT::LEGACY::IsDefaultTextV10( text ) );

    text.SetBold( true );
    BOOST_CHECK( !KICAD_FORMAT::LEGACY::IsDefaultTextV10( text ) );
    text.SetBold( false );
    text.SetMirrored( true );
    BOOST_CHECK( !KICAD_FORMAT::LEGACY::IsDefaultTextV10( text ) );
}


BOOST_AUTO_TEST_CASE( HistoricalTitleAndStrokeKeepPinnedCommonGrammar )
{
    TITLE_BLOCK      title;
    STRING_FORMATTER empty;
    KICAD_FORMAT::LEGACY::FormatTitle( &empty, title );
    BOOST_CHECK( empty.GetString().empty() );

    title.SetTitle( wxT( "Production" ) );
    title.SetDate( wxT( "2026-10-08" ) );
    title.SetRevision( wxT( "A" ) );
    title.SetCompany( wxT( "KiCad" ) );
    title.SetComment( 0, wxT( "First" ) );
    title.SetComment( 8, wxT( "Last" ) );
    STRING_FORMATTER metadata;
    KICAD_FORMAT::LEGACY::FormatTitle( &metadata, title );
    BOOST_CHECK_EQUAL( metadata.GetString(), "(title_block(title \"Production\")(date \"2026-10-08\")(rev \"A\")"
                                             "(company \"KiCad\")(comment 1 \"First\")(comment 9 \"Last\"))" );
    BOOST_CHECK( title.GetComment( 8 ) == wxT( "Last" ) );

    for( const auto& [style, token] :
         std::vector<std::pair<LINE_STYLE, std::string>>{ { LINE_STYLE::DEFAULT, "default" },
                                                          { LINE_STYLE::SOLID, "solid" },
                                                          { LINE_STYLE::DASH, "dash" },
                                                          { LINE_STYLE::DOT, "dot" },
                                                          { LINE_STYLE::DASHDOT, "dash_dot" },
                                                          { LINE_STYLE::DASHDOTDOT, "dash_dot_dot" } } )
    {
        STROKE_PARAMS    stroke( 200000, style );
        STRING_FORMATTER plain;
        KICAD_FORMAT::LEGACY::FormatStroke( &plain, stroke, pcbIUScale );
        BOOST_CHECK_EQUAL( plain.GetString(), "(stroke (width 0.2) (type " + token + "))" );
        stroke.SetColor( KIGFX::COLOR4D( 0.2, 0.4, 0.6, 0.75 ) );
        STRING_FORMATTER colored;
        KICAD_FORMAT::LEGACY::FormatStroke( &colored, stroke, pcbIUScale );
        BOOST_CHECK_EQUAL( colored.GetString(), "(stroke (width 0.2) (type " + token + ") (color 51 102 153 0.75))" );
    }
}


BOOST_AUTO_TEST_CASE( HistoricalEmbeddedFilesKeepEmptyReleasePolicy )
{
    EMBEDDED_FILES files;
    auto           file = std::make_shared<EMBEDDED_FILES::EMBEDDED_FILE>();
    file->name = wxT( "metadata.bin" );
    file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::OTHER;
    file->data_hash = "metadata-only";
    files.AddFile( file );

    STRING_FORMATTER invalidData;
    BOOST_CHECK_THROW( KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV9( invalidData, files, true ), IO_ERROR );

    STRING_FORMATTER reference;
    KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV9( reference, files, false );
    BOOST_CHECK_EQUAL( reference.GetString(),
                       "(embedded_files (file (name \"metadata.bin\")(type other)(checksum \"metadata-only\")))" );

    for( bool writeData : { false, true } )
    {
        STRING_FORMATTER skipped;
        KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV10( skipped, files, writeData );
        BOOST_CHECK_EQUAL( skipped.GetString(), "(embedded_files )" );
    }

    BOOST_REQUIRE( files.GetEmbeddedFile( wxT( "metadata.bin" ) ) );
    BOOST_CHECK( file->compressedEncodedData.empty() );
    BOOST_CHECK_EQUAL( file->data_hash, "metadata-only" );
}


BOOST_AUTO_TEST_CASE( HistoricalEmbeddedFilesKeepValidDataAndNativeChunkDelimiters )
{
    EMBEDDED_FILES files;
    auto           file = std::make_shared<EMBEDDED_FILES::EMBEDDED_FILE>();
    file->name = wxT( "payload.bin" );
    file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::OTHER;

    for( int ii = 0; ii < 4096; ++ii )
        file->decompressedData.push_back( static_cast<char>( ( ii * 131 + ii / 251 ) % 256 ) );

    BOOST_REQUIRE( EMBEDDED_FILES::CompressAndEncode( *file ) == EMBEDDED_FILES::RETURN_CODE::OK );
    BOOST_REQUIRE_GT( file->compressedEncodedData.size(), 76 );
    files.AddFile( file );
    const std::string encoded = file->compressedEncodedData;
    const std::string hash = file->data_hash;

    std::string expected = "(embedded_files (file (name \"payload.bin\")(type other)(data";

    for( size_t first = 0; first < encoded.size(); first += 76 )
    {
        const std::string chunk = encoded.substr( first, 76 );
        expected += "\n";
        expected += first ? " " : "|";
        expected += chunk;

        if( first + chunk.size() == encoded.size() )
            expected += "|";

        expected += "\n";
    }

    expected += ")(checksum \"" + hash + "\")))";

    for( bool v9 : { false, true } )
    {
        STRING_FORMATTER serialized;

        if( v9 )
            KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV9( serialized, files, true );
        else
            KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV10( serialized, files, true );

        BOOST_CHECK_EQUAL( serialized.GetString(), expected );
        STRING_FORMATTER reference;

        if( v9 )
            KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV9( reference, files, false );
        else
            KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV10( reference, files, false );

        BOOST_CHECK_EQUAL( reference.GetString(),
                           "(embedded_files (file (name \"payload.bin\")(type other)(checksum \"" + hash + "\")))" );
    }

    BOOST_CHECK_EQUAL( file->compressedEncodedData, encoded );
    BOOST_CHECK_EQUAL( file->data_hash, hash );
}


static std::string historicalEmbeddedFiles( const EMBEDDED_FILES& aFiles, bool aV9, bool aWriteData )
{
    STRING_FORMATTER formatter;

    if( aV9 )
        KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV9( formatter, aFiles, aWriteData );
    else
        KICAD_FORMAT::LEGACY::FormatEmbeddedFilesV10( formatter, aFiles, aWriteData );

    return formatter.GetString();
}


static std::string releaseEmbeddedChecksum( const std::vector<char>& aPayload )
{
    MMH3_HASH hash( EMBEDDED_FILES::Seed() );
    hash.addDataV1( reinterpret_cast<const uint8_t*>( aPayload.data() ), aPayload.size() );
    return hash.digest().ToString();
}


BOOST_AUTO_TEST_CASE( HistoricalEmbeddedFilesRestoreReleaseChecksumsForAllTailLengths )
{
    std::vector<size_t> sizes = { 4095, 4096, 4097 };

    for( size_t size = 0; size <= 32; ++size )
        sizes.push_back( size );

    for( size_t size : sizes )
    {
        EMBEDDED_FILES files;
        auto           file = std::make_shared<EMBEDDED_FILES::EMBEDDED_FILE>();
        file->name = wxT( "payload.bin" );

        for( size_t ii = 0; ii < size; ++ii )
            file->decompressedData.push_back( static_cast<char>( ( ii * 131 + ii / 251 ) % 256 ) );

        BOOST_REQUIRE( EMBEDDED_FILES::CompressAndEncode( *file ) == EMBEDDED_FILES::RETURN_CODE::OK );
        files.AddFile( file );
        const auto        original = *file;
        const std::string checksum = releaseEmbeddedChecksum( original.decompressedData );

        if( size % 16 != 0 )
            BOOST_CHECK_NE( checksum, original.data_hash );

        for( bool v9 : { false, true } )
        {
            for( bool writeData : { false, true } )
            {
                BOOST_TEST_CONTEXT( "bytes=" << size << " v9=" << v9 << " data=" << writeData )
                {
                    const std::string output = historicalEmbeddedFiles( files, v9, writeData );
                    BOOST_CHECK( output.find( "(checksum \"" + checksum + "\")" ) != std::string::npos );
                    BOOST_CHECK_EQUAL( output.find( "(data" ) != std::string::npos, writeData );

                    if( !writeData )
                    {
                        BOOST_CHECK_EQUAL( output, "(embedded_files (file (name \"payload.bin\")(type other)"
                                                   "(checksum \""
                                                           + checksum + "\")))" );
                    }

                    BOOST_CHECK( file->decompressedData == original.decompressedData );
                    BOOST_CHECK_EQUAL( file->compressedEncodedData, original.compressedEncodedData );
                    BOOST_CHECK_EQUAL( file->data_hash, original.data_hash );
                    BOOST_CHECK_EQUAL( file->is_valid, original.is_valid );
                }
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( HistoricalEmbeddedFilesDecodeEncodedOnlyAttachmentsWithoutMutatingSource )
{
    for( size_t size : { 0u, 1u, 13u, 17u, 31u, 4097u } )
    {
        EMBEDDED_FILES files;
        auto           file = std::make_shared<EMBEDDED_FILES::EMBEDDED_FILE>();
        file->name = wxT( "encoded.bin" );
        file->decompressedData.assign( size, 'x' );
        const std::string checksum = releaseEmbeddedChecksum( file->decompressedData );
        BOOST_REQUIRE( EMBEDDED_FILES::CompressAndEncode( *file ) == EMBEDDED_FILES::RETURN_CODE::OK );
        file->decompressedData.clear();
        files.AddFile( file );
        const auto original = *file;

        for( bool v9 : { false, true } )
        {
            for( bool writeData : { false, true } )
            {
                BOOST_TEST_CONTEXT( "bytes=" << size << " v9=" << v9 << " data=" << writeData )
                {
                    const std::string output = historicalEmbeddedFiles( files, v9, writeData );
                    BOOST_CHECK( output.find( "(checksum \"" + checksum + "\")" ) != std::string::npos );
                    BOOST_CHECK_EQUAL( output.find( "(data" ) != std::string::npos, writeData );
                    BOOST_CHECK( file->decompressedData.empty() );
                    BOOST_CHECK_EQUAL( file->compressedEncodedData, original.compressedEncodedData );
                    BOOST_CHECK_EQUAL( file->data_hash, original.data_hash );
                    BOOST_CHECK_EQUAL( file->is_valid, original.is_valid );
                }
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( HistoricalEmbeddedFilesRejectMalformedEncodedOnlyAttachments )
{
    for( bool badChecksum : { false, true } )
    {
        EMBEDDED_FILES files;
        auto           file = std::make_shared<EMBEDDED_FILES::EMBEDDED_FILE>();
        file->name = wxT( "invalid.bin" );
        file->decompressedData.assign( 13, 'x' );
        BOOST_REQUIRE( EMBEDDED_FILES::CompressAndEncode( *file ) == EMBEDDED_FILES::RETURN_CODE::OK );
        file->decompressedData.clear();

        if( badChecksum )
            file->data_hash = "invalid-checksum";
        else
            file->compressedEncodedData = "invalid-base64";

        files.AddFile( file );
        const auto original = *file;

        for( bool v9 : { false, true } )
        {
            for( bool writeData : { false, true } )
            {
                BOOST_CHECK_THROW( historicalEmbeddedFiles( files, v9, writeData ), IO_ERROR );
                BOOST_CHECK( file->decompressedData.empty() );
                BOOST_CHECK_EQUAL( file->compressedEncodedData, original.compressedEncodedData );
                BOOST_CHECK_EQUAL( file->data_hash, original.data_hash );
                BOOST_CHECK_EQUAL( file->is_valid, original.is_valid );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( HistoricalEmbeddedFilesRestoreCommittedWorksheetChecksums )
{
    SETTINGS_MANAGER       settingsManager;
    const std::string      source = KI_TEST::GetPcbnewTestDataDir() + "issue22102.kicad_pcb";
    const std::string      sourceBytes = KI_TEST::ReadGoldenText( source );
    PCB_IO_KICAD_SEXPR     io;
    std::unique_ptr<BOARD> board( io.LoadBoard( source ) );
    BOOST_REQUIRE( board );

    // The fixture was committed in 2025 before the MMH3 tail-byte correction.
    const std::vector<std::pair<wxString, std::string>> checksums = {
        { wxT( "SchematicTemplate.kicad_wks" ), "6003EC72C5978E83C182073E67278FCF" },
        { wxT( "SchematicTemplateEmpty.kicad_wks" ), "0D054F65E3E0563620A9E16BBBB777DD" }
    };

    for( const auto& [name, checksum] : checksums )
    {
        const auto* file = board->GetEmbeddedFile( name );
        BOOST_REQUIRE( file );
        BOOST_REQUIRE( !file->decompressedData.empty() );
        BOOST_CHECK_NE( file->data_hash, checksum );
        BOOST_CHECK_EQUAL( releaseEmbeddedChecksum( file->decompressedData ), checksum );
        const auto original = *file;

        for( bool v9 : { false, true } )
        {
            for( bool writeData : { false, true } )
            {
                const std::string output = historicalEmbeddedFiles( *board, v9, writeData );
                BOOST_CHECK( output.find( "(name \"" + name.ToStdString() + "\")" ) != std::string::npos );
                BOOST_CHECK( output.find( "(checksum \"" + checksum + "\")" ) != std::string::npos );
                BOOST_CHECK( output.find( "(checksum \"" + original.data_hash + "\")" ) == std::string::npos );
                BOOST_CHECK_EQUAL( output.find( "(data" ) != std::string::npos, writeData );
                BOOST_CHECK( file->decompressedData == original.decompressedData );
                BOOST_CHECK_EQUAL( file->compressedEncodedData, original.compressedEncodedData );
                BOOST_CHECK_EQUAL( file->data_hash, original.data_hash );
                BOOST_CHECK_EQUAL( file->is_valid, original.is_valid );
            }
        }
    }

    BOOST_CHECK_EQUAL( KI_TEST::ReadGoldenText( source ), sourceBytes );
}


BOOST_AUTO_TEST_SUITE_END()
