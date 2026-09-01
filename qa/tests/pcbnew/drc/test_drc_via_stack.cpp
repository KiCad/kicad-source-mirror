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

#include <fstream>
#include <memory>

#include <wx/filename.h>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <common.h>
#include <lset.h>
#include <padstack.h>
#include <pcb_track.h>
#include <generators/pcb_via_stack.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule_parser.h>
#include <pcbnew_utils/board_test_utils.h>
#include <reporter.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct VIA_STACK_DRC_FIXTURE
{
    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;

    int m_malformedSpans = 0;
    int m_notFilled = 0;
    int m_depth = 0;
    int m_aspect = 0;
    int m_crossesCore = 0;

    std::vector<int> m_ignored;

    wxFileName m_rulePath;

    // The temp dir has to outlive run(), so it is kept here rather than in each test.
    std::unique_ptr<KI_TEST::TEMPORARY_DIRECTORY> m_ruleDir;

    // The limits these checks read come from rules, so a test states them the way a user would.
    void setRule( const std::string& aConstraint, const std::string& aCondition = "" )
    {
        m_ruleDir = std::make_unique<KI_TEST::TEMPORARY_DIRECTORY>( "microvia_rule", "" );

        wxFileName rulePath( m_ruleDir->GetPath().string(), "test.kicad_dru" );

        {
            std::ofstream dru( rulePath.GetFullPath().ToStdString() );
            dru << "(version 1)\n"
                << "(rule \"via stack limits\"\n";

            if( !aCondition.empty() )
                dru << "    (condition \"" << aCondition << "\")\n";

            dru << "    (constraint " << aConstraint << ")\n"
                << ")\n";
        }

        m_rulePath = rulePath;
    }

    // Cases whose subject is the stackup still build their board here: a
    // synthesized stackup is not written to the board file, so a fixture would
    // come back with no thicknesses and quietly test something else.
    //
    // Each case's board is stored under drc_via_stack/ named after the test case,
    // so a renamed case fails loudly here rather than silently reading the wrong
    // board.  Via stacks do not exist in v10, so these fixtures are written by
    // the development build.
    void loadBoard()
    {
        m_board = KI_TEST::ReadBoardFromFileOrStream(
                KI_TEST::GetPcbnewTestDataDir() + "drc_via_stack/"
                + boost::unit_test::framework::current_test_case().p_name.get()
                + ".kicad_pcb" );

        BOOST_REQUIRE( m_board );
    }


    void run()
    {
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        if( !bds.m_DRCEngine )
            bds.m_DRCEngine = std::make_shared<DRC_ENGINE>( m_board.get(), &bds );

        bds.m_DRCEngine->InitEngine( m_rulePath );

        for( int code = DRCE_FIRST; code <= DRCE_LAST; ++code )
            bds.m_DRCSeverities[code] = SEVERITY::RPT_SEVERITY_IGNORE;

        bds.m_DRCSeverities[DRCE_MALFORMED_MICROVIA_STACK_SPAN] = SEVERITY::RPT_SEVERITY_ERROR;
        bds.m_DRCSeverities[DRCE_MICROVIA_STACK_NOT_FILLED] = SEVERITY::RPT_SEVERITY_ERROR;
        bds.m_DRCSeverities[DRCE_MICROVIA_STACK_DEPTH] = SEVERITY::RPT_SEVERITY_ERROR;
        bds.m_DRCSeverities[DRCE_MICROVIA_ASPECT_RATIO] = SEVERITY::RPT_SEVERITY_ERROR;
        bds.m_DRCSeverities[DRCE_MICROVIA_CROSSES_CORE] = SEVERITY::RPT_SEVERITY_WARNING;

        for( int code : m_ignored )
            bds.m_DRCSeverities[code] = SEVERITY::RPT_SEVERITY_IGNORE;

        bds.m_DRCEngine->SetViolationHandler(
                [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                     const std::function<void( PCB_MARKER* )>& )
                {
                    switch( aItem->GetErrorCode() )
                    {
                    case DRCE_MALFORMED_MICROVIA_STACK_SPAN: m_malformedSpans++; break;
                    case DRCE_MICROVIA_STACK_NOT_FILLED: m_notFilled++; break;
                    case DRCE_MICROVIA_STACK_DEPTH: m_depth++; break;
                    case DRCE_MICROVIA_ASPECT_RATIO: m_aspect++; break;
                    case DRCE_MICROVIA_CROSSES_CORE: m_crossesCore++; break;
                    default: break;
                    }
                } );

        bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );
        bds.m_DRCEngine->ClearViolationHandler();
    }
};


static PCB_VIA* makeMicrovia( BOARD* aBoard, const VECTOR2I& aPos, PCB_LAYER_ID aTop, PCB_LAYER_ID aBottom,
                              bool aFilled )
{
    PCB_VIA* via = new PCB_VIA( aBoard );
    via->SetViaType( VIATYPE::MICROVIA );
    via->SetLayerPair( aTop, aBottom );
    via->SetWidth( PADSTACK::ALL_LAYERS, pcbIUScale.mmToIU( 0.3 ) );
    via->SetDrill( pcbIUScale.mmToIU( 0.15 ) );
    via->SetPosition( aPos );
    via->Padstack().Drill().is_filled = aFilled;
    aBoard->Add( via );

    return via;
}


static PCB_VIA_STACK* makeStack( BOARD* aBoard, PCB_LAYER_ID aStart, PCB_LAYER_ID aEnd )
{
    PCB_VIA_STACK* stack = new PCB_VIA_STACK( aBoard, F_Cu );
    stack->SetStartLayer( aStart );
    stack->SetEndLayer( aEnd );
    stack->SetStyle( VIA_STACK_STYLE::STACKED );
    aBoard->Add( stack );

    return stack;
}


BOOST_AUTO_TEST_SUITE( DRCViaStack )


BOOST_FIXTURE_TEST_CASE( GoodStackNoViolations, VIA_STACK_DRC_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetCopperLayerCount( 4 );
    m_board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.SetCopperLayerCount( 4 );
    bds.GetStackupDescriptor().BuildDefaultStackupList( &bds, 4 );
    setRule( "microvia_aspect_ratio (max 1.0)" );

    for( BOARD_STACKUP_ITEM* item : bds.GetStackupDescriptor().GetList() )
    {
        if( item->GetType() != BS_ITEM_TYPE_DIELECTRIC )
            continue;

        item->SetThickness( pcbIUScale.mmToIU( 0.1 ) );
        item->SetTypeName( KEY_PREPREG );
    }

    PCB_VIA_STACK* stack = makeStack( m_board.get(), F_Cu, In2_Cu );
    stack->SetViaSize( pcbIUScale.mmToIU( 0.3 ) );
    stack->SetViaDrill( pcbIUScale.mmToIU( 0.15 ) );
    stack->SetPosition( VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ) );
    stack->Regenerate( m_board.get(), nullptr );

    run();

    BOOST_CHECK_EQUAL( m_malformedSpans, 0 );
    BOOST_CHECK_EQUAL( m_notFilled, 0 );
    BOOST_CHECK_EQUAL( m_depth, 0 );
    BOOST_CHECK_EQUAL( m_aspect, 0 );
    BOOST_CHECK_EQUAL( m_crossesCore, 0 );
}


BOOST_FIXTURE_TEST_CASE( StackNotTilingItsSpanReported, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    run();

    BOOST_CHECK_EQUAL( m_malformedSpans, 1 );
}


BOOST_FIXTURE_TEST_CASE( UnfilledInnerHopReported, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    run();

    BOOST_CHECK_EQUAL( m_malformedSpans, 0 );
    BOOST_CHECK_EQUAL( m_notFilled, 1 );
}


BOOST_FIXTURE_TEST_CASE( StackTooDeepReported, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    setRule( "microvia_stack_depth (max 1)" );

    run();

    BOOST_CHECK_EQUAL( m_malformedSpans, 0 );
    BOOST_CHECK_EQUAL( m_depth, 1 );
}


BOOST_FIXTURE_TEST_CASE( LooseStackedMicroviasReportNotFilled, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    run();

    BOOST_CHECK_EQUAL( m_notFilled, 1 );
}


BOOST_FIXTURE_TEST_CASE( LooseStackedMicroviasReportDepth, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    setRule( "microvia_stack_depth (max 2)" );

    run();

    BOOST_CHECK_EQUAL( m_depth, 1 );
}


// Two microvias sharing an x,y but not touching are two structures, not one three deep.
BOOST_FIXTURE_TEST_CASE( CoaxialButSeparatedMicroviasAreNotOneStack, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    setRule( "microvia_stack_depth (max 1)" );

    run();

    BOOST_CHECK_EQUAL( m_depth, 0 );
    BOOST_CHECK_EQUAL( m_notFilled, 0 );
}


// A generator stack must be judged once, not once as a generator and again as loose geometry.
BOOST_FIXTURE_TEST_CASE( GeneratorStackIsNotReportedTwice, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    run();

    BOOST_CHECK_EQUAL( m_notFilled, 2 );
}


// A hair of offset must not split one structure into two. The upper via still lands on the one
// below, so both rules still apply.
BOOST_FIXTURE_TEST_CASE( OverlappingButOffsetMicroviasAreOneStructure, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    run();

    BOOST_CHECK_EQUAL( m_notFilled, 1 );
}


// A real stagger clears the via below, so the fill rule does not apply to it.
BOOST_FIXTURE_TEST_CASE( ClearOfTheViaBelowIsAStaggerNotAStack, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    run();

    BOOST_CHECK_EQUAL( m_notFilled, 0 );
    BOOST_CHECK_EQUAL( m_depth, 0 );
}


// IPC-6012 requires the hops of a stacked microvia to be filled. That applies to the whole
// structure and not only to the ones carrying another hop.
BOOST_FIXTURE_TEST_CASE( EveryHopOfAStackMustBeFilled, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    run();

    BOOST_CHECK_EQUAL( m_notFilled, 2 );
}


// A lone microvia carries nothing, so it may be unfilled.
BOOST_FIXTURE_TEST_CASE( ALoneMicroviaMayBeUnfilled, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    run();

    BOOST_CHECK_EQUAL( m_notFilled, 0 );
}


// IPC-2226 caps a microvia at 1:1. A default 6 layer stackup has dielectrics far thicker than a
// 0.1 mm hole, so a microvia across one is well outside that.
BOOST_FIXTURE_TEST_CASE( MicroviaTooDeepForItsDiameterIsReported, VIA_STACK_DRC_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetCopperLayerCount( 6 );
    m_board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.SetCopperLayerCount( 6 );
    bds.GetStackupDescriptor().BuildDefaultStackupList( &bds, 6 );
    setRule( "microvia_aspect_ratio (max 1.0)" );

    makeMicrovia( m_board.get(), VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ), F_Cu, In1_Cu, true );

    run();

    BOOST_CHECK_EQUAL( m_aspect, 1 );
}


// With no limit set the check says nothing, however thick the dielectric.
BOOST_FIXTURE_TEST_CASE( AspectRatioIsSilentWithoutALimit, VIA_STACK_DRC_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetCopperLayerCount( 6 );
    m_board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.SetCopperLayerCount( 6 );
    bds.GetStackupDescriptor().BuildDefaultStackupList( &bds, 6 );


    makeMicrovia( m_board.get(), VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ), F_Cu, In1_Cu, true );

    run();

    BOOST_CHECK_EQUAL( m_aspect, 0 );

    // The same via violates once a limit exists, so the silence came from the missing limit.
    m_aspect = 0;
    setRule( "microvia_aspect_ratio (max 1.0)" );

    run();

    BOOST_CHECK_EQUAL( m_aspect, 1 );
}


// The aspect ratio check is independent of the other three, so silencing them must not
// silence it.
BOOST_FIXTURE_TEST_CASE( AspectRatioSurvivesTheOtherChecksBeingIgnored, VIA_STACK_DRC_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetCopperLayerCount( 6 );
    m_board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.SetCopperLayerCount( 6 );
    bds.GetStackupDescriptor().BuildDefaultStackupList( &bds, 6 );
    setRule( "microvia_aspect_ratio (max 1.0)" );

    m_ignored = { DRCE_MALFORMED_MICROVIA_STACK_SPAN, DRCE_MICROVIA_STACK_NOT_FILLED, DRCE_MICROVIA_STACK_DEPTH };

    makeMicrovia( m_board.get(), VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ), F_Cu, In1_Cu, true );

    run();

    BOOST_CHECK_EQUAL( m_aspect, 1 );
}


// Without a stackup there are no thicknesses to divide, so the check stands down.
// With stackup thicknesses to divide, the limit applies; with none, the check stands down.
BOOST_FIXTURE_TEST_CASE( AspectRatioNeedsStackupThicknesses, VIA_STACK_DRC_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetCopperLayerCount( 4 );
    m_board->SetEnabledLayers( LSET::AllCuMask( 4 ) | LSET::AllTechMask() );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    setRule( "microvia_aspect_ratio (max 0.5)" );

    VECTOR2I at( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );
    makeMicrovia( m_board.get(), at, F_Cu, In1_Cu, true );

    // No stackup list at all, so there is nothing to measure.
    run();
    BOOST_CHECK_EQUAL( m_aspect, 0 );

    // The same via against a real stackup exceeds the same limit.
    m_aspect = 0;
    bds.SetCopperLayerCount( 4 );
    bds.GetStackupDescriptor().BuildDefaultStackupList( &bds, 4 );
    run();
    BOOST_CHECK_MESSAGE( m_aspect == 1, "with thicknesses present the limit must apply" );
}


BOOST_FIXTURE_TEST_CASE( MicroviaStackDepthRuleIsHonoured, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    setRule( "microvia_stack_depth (max 2)" );

    run();
    BOOST_CHECK_EQUAL( m_depth, 1 );

    m_depth = 0;
    setRule( "microvia_stack_depth (max 4)" );

    // A rule that allows four leaves the same three hops alone.
    run();
    BOOST_CHECK_EQUAL( m_depth, 0 );

    // With a condition the rule still has to match, so a limit of two bites again.
    m_depth = 0;
    setRule( "microvia_stack_depth (max 2)", "A.isStackedVia()" );

    run();
    BOOST_CHECK_MESSAGE( m_depth == 1, "the rule condition did not select the stack" );
}


// A microvia whose two layers are the same spans no dielectric. Such vias point at each other
// in the chain walk, so a handful of them must not multiply into a pile of violations.
BOOST_FIXTURE_TEST_CASE( ZeroSpanMicroviasDoNotMultiplyViolations, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    run();

    // A via spanning one layer forms no column, so there is nothing to report at all.
    BOOST_CHECK_MESSAGE( m_notFilled == 0, "3 vias produced " << m_notFilled << " violations" );
}


// A dielectric built from several sublayers is as deep as all of them together.
BOOST_FIXTURE_TEST_CASE( AspectRatioCountsEveryDielectricSublayer, VIA_STACK_DRC_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetCopperLayerCount( 6 );
    m_board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.SetCopperLayerCount( 6 );
    bds.GetStackupDescriptor().BuildDefaultStackupList( &bds, 6 );

    // Split the dielectric under F.Cu into two 0.1 mm sublayers.
    for( BOARD_STACKUP_ITEM* item : bds.GetStackupDescriptor().GetList() )
    {
        if( item->GetType() != BS_ITEM_TYPE_DIELECTRIC )
            continue;

        item->AddDielectricPrms( 1 );
        item->SetThickness( pcbIUScale.mmToIU( 0.1 ), 0 );
        item->SetThickness( pcbIUScale.mmToIU( 0.1 ), 1 );
        break;
    }

    // 0.2 mm of dielectric over a 0.15 mm drill is a ratio above 1, one sublayer alone is not.
    setRule( "microvia_aspect_ratio (max 1.2)" );

    makeMicrovia( m_board.get(), VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ), F_Cu, In1_Cu, true );

    run();

    BOOST_CHECK_MESSAGE( m_aspect == 1, "both sublayers must count toward the via depth" );
}


// A microvia reaching B.Cu is drilled from the back, so it passes through the B.Cu foil.
BOOST_FIXTURE_TEST_CASE( AspectRatioUsesTheFoilTheLaserEnters, VIA_STACK_DRC_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetCopperLayerCount( 6 );
    m_board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.SetCopperLayerCount( 6 );
    bds.GetStackupDescriptor().BuildDefaultStackupList( &bds, 6 );

    const std::vector<BOARD_STACKUP_ITEM*>& stack = bds.GetStackupDescriptor().GetList();

    // 1 oz outer foil, half that inside, and a known dielectric between In4.Cu and B.Cu.
    for( BOARD_STACKUP_ITEM* item : stack )
    {
        if( item->GetType() != BS_ITEM_TYPE_COPPER )
            continue;

        bool outer = item->GetBrdLayerId() == F_Cu || item->GetBrdLayerId() == B_Cu;
        item->SetThickness( pcbIUScale.mmToIU( outer ? 0.035 : 0.0175 ) );
    }

    for( int i = (int) stack.size() - 1; i >= 0; --i )
    {
        if( stack[i]->GetType() == BS_ITEM_TYPE_DIELECTRIC )
        {
            stack[i]->SetThickness( pcbIUScale.mmToIU( 0.1 ) );
            break;
        }
    }

    // Through the B.Cu foil the depth is 0.135 mm over a 0.1 mm drill, a ratio of 1.35.
    // Through the In4.Cu foil it would read 1.175 and slip under the limit.
    setRule( "microvia_aspect_ratio (max 1.25)" );

    PCB_VIA* via = makeMicrovia( m_board.get(), VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ), In4_Cu,
                                 B_Cu, true );
    via->SetDrill( pcbIUScale.mmToIU( 0.1 ) );

    run();

    BOOST_CHECK_MESSAGE( m_aspect == 1, "a back-side microvia must be measured through the B.Cu foil" );
}


// An unset fill means "use the board default", not "unfilled".
BOOST_FIXTURE_TEST_CASE( FillFromBoardHonoursTheBoardDefault, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    run();

    BOOST_CHECK_MESSAGE( m_notFilled == 0, "board default is filled, but got " << m_notFilled << " not-filled errors" );
}


// Two overlapping hops can both land on the one below them, and it must not be reported once
// for each.
BOOST_FIXTURE_TEST_CASE( AViaCarriedTwiceIsReportedOnce, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    run();

    BOOST_CHECK_EQUAL( m_notFilled, 1 );
}


// Holes a drill diameter apart touch, leaving no wall between them.
BOOST_FIXTURE_TEST_CASE( TouchingHolesFormAColumn, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    run();

    BOOST_CHECK_EQUAL( m_notFilled, 1 );
}


// Laser drilling needs the resin to ablate, so a microvia belongs in the build-up layers
// rather than through the glass reinforced core.
BOOST_FIXTURE_TEST_CASE( MicroviaThroughCoreIsReported, VIA_STACK_DRC_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetCopperLayerCount( 6 );
    m_board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.SetCopperLayerCount( 6 );
    bds.GetStackupDescriptor().BuildDefaultStackupList( &bds, 6 );

    const std::vector<BOARD_STACKUP_ITEM*>& stack = bds.GetStackupDescriptor().GetList();

    // Name the copper layers either side of the first core, and of the first prepreg.
    PCB_LAYER_ID coreTop = UNDEFINED_LAYER, coreBot = UNDEFINED_LAYER;
    PCB_LAYER_ID pregTop = UNDEFINED_LAYER, pregBot = UNDEFINED_LAYER;
    PCB_LAYER_ID lastCopper = UNDEFINED_LAYER;

    for( size_t i = 0; i < stack.size(); ++i )
    {
        if( stack[i]->GetType() == BS_ITEM_TYPE_COPPER )
        {
            lastCopper = stack[i]->GetBrdLayerId();
            continue;
        }

        if( stack[i]->GetType() != BS_ITEM_TYPE_DIELECTRIC )
            continue;

        PCB_LAYER_ID below = UNDEFINED_LAYER;

        for( size_t j = i + 1; j < stack.size(); ++j )
        {
            if( stack[j]->GetType() == BS_ITEM_TYPE_COPPER )
            {
                below = stack[j]->GetBrdLayerId();
                break;
            }
        }

        if( stack[i]->GetTypeName() == KEY_CORE && coreTop == UNDEFINED_LAYER )
        {
            coreTop = lastCopper;
            coreBot = below;
        }
        else if( stack[i]->GetTypeName() == KEY_PREPREG && pregTop == UNDEFINED_LAYER )
        {
            pregTop = lastCopper;
            pregBot = below;
        }
    }

    BOOST_REQUIRE( coreTop != UNDEFINED_LAYER && coreBot != UNDEFINED_LAYER );
    BOOST_REQUIRE( pregTop != UNDEFINED_LAYER && pregBot != UNDEFINED_LAYER );

    VECTOR2I overCore( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) );
    VECTOR2I overPrepreg( pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 10 ) );

    makeMicrovia( m_board.get(), overCore, coreTop, coreBot, true );
    makeMicrovia( m_board.get(), overPrepreg, pregTop, pregBot, true );

    run();

    // Only the one crossing core is reported, and it does not need an aspect ratio limit set.
    BOOST_CHECK_EQUAL( m_crossesCore, 1 );
}


// Two hops landing on one shared via below are two structures. The second must still be
// fill checked rather than dropped for sharing.
BOOST_FIXTURE_TEST_CASE( ASecondHopSharingALandingViaIsStillChecked, VIA_STACK_DRC_FIXTURE )
{
    loadBoard();

    run();

    BOOST_CHECK_EQUAL( m_notFilled, 2 );
}


// The core check shares the aspect ratio walk, so an exhausted aspect limit must not end it.
BOOST_FIXTURE_TEST_CASE( AnIgnoredAspectRatioDoesNotStopTheCoreCheck, VIA_STACK_DRC_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetCopperLayerCount( 6 );
    m_board->SetEnabledLayers( LSET::AllCuMask( 6 ) | LSET::AllTechMask() );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.SetCopperLayerCount( 6 );
    bds.GetStackupDescriptor().BuildDefaultStackupList( &bds, 6 );

    // A limit is set, but its severity is off. The first via would otherwise end the walk.
    setRule( "microvia_aspect_ratio (max 0.1)" );
    m_ignored = { DRCE_MICROVIA_ASPECT_RATIO };

    const std::vector<BOARD_STACKUP_ITEM*>& stack = bds.GetStackupDescriptor().GetList();
    PCB_LAYER_ID coreTop = UNDEFINED_LAYER, coreBot = UNDEFINED_LAYER, lastCopper = UNDEFINED_LAYER;

    for( size_t i = 0; i < stack.size(); ++i )
    {
        if( stack[i]->GetType() == BS_ITEM_TYPE_COPPER )
        {
            lastCopper = stack[i]->GetBrdLayerId();
            continue;
        }

        if( stack[i]->GetType() == BS_ITEM_TYPE_DIELECTRIC && stack[i]->GetTypeName() == KEY_CORE
            && coreTop == UNDEFINED_LAYER )
        {
            coreTop = lastCopper;

            for( size_t j = i + 1; j < stack.size(); ++j )
            {
                if( stack[j]->GetType() == BS_ITEM_TYPE_COPPER )
                {
                    coreBot = stack[j]->GetBrdLayerId();
                    break;
                }
            }
        }
    }

    BOOST_REQUIRE( coreTop != UNDEFINED_LAYER && coreBot != UNDEFINED_LAYER );

    makeMicrovia( m_board.get(), VECTOR2I( pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) ), coreTop, coreBot, true );
    makeMicrovia( m_board.get(), VECTOR2I( pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 10 ) ), coreTop, coreBot, true );

    run();

    BOOST_CHECK_EQUAL( m_aspect, 0 );
    BOOST_CHECK_MESSAGE( m_crossesCore == 2, "the walk stopped early, only " << m_crossesCore << " reported" );
}


BOOST_AUTO_TEST_SUITE_END()
