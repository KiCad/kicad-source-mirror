/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author James Jackson
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "diff_phase_skew_tool.h"

#include <router/pns_arc.h>
#include <router/pns_diff_pair.h>
#include <router/pns_helpers.h>
#include <router/pns_kicad_iface.h>
#include <router/pns_router.h>
#include <router/pns_topology.h>

#include <advanced_config.h>
#include <board.h>
#include <collectors.h>
#include <dialog_diff_phase_skew_properties.h>
#include <drc/drc_engine.h>
#include <gal/painter.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <pcb_edit_frame.h>
#include <pad.h>
#include <pcb_track.h>
#include <project/project_file.h>
#include <settings/settings_manager.h>
#include <tools/drc_tool.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tool/tool_manager.h>
#include <view/view.h>
#include <view/view_controls.h>


#define INITIAL_HOVER_HITTEST_THRESHOLD_PIXELS 5
#define DETAILS_HOVER_HITTEST_THRESHOLD_PIXELS 20


DIFF_PHASE_SKEW_TOOL::DIFF_PHASE_SKEW_TOOL() :
        PCB_TOOL_BASE( "pcbnew.InteractiveDiffPhaseSkew" )
{
    const ADVANCED_CFG& cfg = ADVANCED_CFG::GetCfg();
    m_overlayTrackInflation = cfg.m_DiffSkewOverlayTrackInflation;
    m_trackGapInflation = cfg.m_DiffSkewTrackGapInflation;
    m_cosThetaParallelTestValue = cfg.m_DiffSkewCosThetaParallelTestValue;
    m_colourInterpolationLogStrength = cfg.m_DiffSkewColourInterpolationLogStrength;
    m_targetDiffSegmentSize = cfg.m_DiffSkewTargetDiffSegmentSize;
}


DIFF_PHASE_SKEW_TOOL::~DIFF_PHASE_SKEW_TOOL()
{
}


bool DIFF_PHASE_SKEW_TOOL::Init()
{
    return true;
}


void DIFF_PHASE_SKEW_TOOL::Reset( const RESET_REASON aReason )
{
    delete m_router;
    delete m_iface; // Delete after m_router because PNS::NODE dtor needs m_ruleResolver

    if( aReason == RESET_REASON::SHUTDOWN )
    {
        m_router = nullptr;
        m_iface = nullptr;
        return;
    }

    // Get core objects
    m_view = getView();
    m_controls = getViewControls();
    m_board = getModel<BOARD>();
    m_frame = frame();
    DRC_TOOL* drcTool = m_toolMgr->GetTool<DRC_TOOL>();
    m_drcEngine = drcTool->GetDRCEngine();

    // Initialise a router instance
    m_iface = new PNS_KICAD_IFACE;
    m_iface->SetBoard( m_board );
    m_iface->SetView( m_view );
    m_iface->SetHostTool( this );

    m_router = new PNS::ROUTER;
    m_router->SetInterface( m_iface );
    m_router->ClearWorld();
    m_router->SyncWorld();
    m_router->UpdateSizes( m_savedSizes );

    PCBNEW_SETTINGS* settings = m_frame->GetPcbNewSettings();

    if( !settings->m_PnsSettings )
        settings->m_PnsSettings = std::make_unique<PNS::ROUTING_SETTINGS>( settings, "tools.pns" );

    m_router->LoadSettings( settings->m_PnsSettings.get() );

    resetStateVariables();
}


int DIFF_PHASE_SKEW_TOOL::ShowDiffPhaseSkew( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor || m_inDiffPhaseSkewTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDiffPhaseSkewTool );

    TOOL_EVENT pushedEvent = aEvent;
    m_frame->PushTool( aEvent );
    Activate();

    // Must be done after Activate() so that it gets set into the correct context
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    // controls->ShowCursor( true );
    // controls->ForceCursorPosition( false );

    // Set initial cursor
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::TUNE );

    // Get the required tools and helpers
    PCB_SELECTION_TOOL*         selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    GENERAL_COLLECTORS_GUIDE    guide = m_frame->GetCollectorsGuide();

    // Create the VIEW_OVERLAY
    getOverlay();

    m_pickerItemFirst = nullptr;

    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        m_cursorPos = controls->GetMousePosition();

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            // Roll back our mode, or exit if we are in the initial hover mode
            if( GetMode() == MODE::FIXED || GetMode() == MODE::SELECTED_FIRST )
            {
                m_pickerItemFirst = nullptr;
                m_pickerItemFirstIsDiffPair = false;
                m_pickerItemSecond = nullptr;
                clearOverlay();
                updateMessagePanel();
                m_maxSkew.reset();
                SetMode( MODE::HOVER );
            }
            else
            {
                updateMessagePanel();
                break;
            }
        }

        if( evt->IsMotion() )
        {
            if( GetMode() == MODE::HOVER )
            {
                m_pickerItemFirst = nullptr;
                m_pickerItemSecond = nullptr;
                m_pickerItemFirstIsDiffPair = false;
                doInitialHover( selectionTool, guide );
                updateMessagePanel();
            }
            else if( GetMode() == MODE::SELECTED_FIRST )
            {
                m_pickerItemSecond = nullptr;
                doInitialHover( selectionTool, guide );
                updateMessagePanel();
            }
            else if( GetMode() == MODE::FIXED )
            {
                doShowStatsAtCursor();
                updateMessagePanel();
            }
        }
        else if( evt->IsClick( BUT_LEFT ) && GetMode() == MODE::HOVER && m_pickerItemFirst )
        {
            m_originFirst = m_cursorPos;

            if( m_pickerItemFirstIsDiffPair )
            {
                SetMode( MODE::FIXED );
                doDisplayOverlay();
            }
            else
            {
                SetMode( MODE::SELECTED_FIRST );
            }

            updateMessagePanel();
        }
        else if( evt->IsClick( BUT_LEFT ) && GetMode() == MODE::SELECTED_FIRST && m_pickerItemSecond )
        {
            // First click to select the diff pair for inspection
            SetMode( MODE::FIXED );
            m_originSecond = m_cursorPos;
            doDisplayOverlay();
            updateMessagePanel();
        }
        else if( evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            PCBNEW_SETTINGS::DIFF_PHASE_SKEW_SETTINGS settings;
            PCBNEW_SETTINGS*                          cfg = m_frame->GetPcbNewSettings();
            settings = cfg->m_DiffPhaseSkewSettings;

            DIALOG_DIFF_PHASE_SKEW_PROPERTIES dlg( m_frame, &settings );

            if( dlg.ShowModal() == wxID_OK )
            {
                cfg->m_DiffPhaseSkewSettings = settings;

                if( GetMode() == MODE::FIXED )
                    doDisplayOverlay();
            }
        }
    }

    // Restore UI state
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    // Reset tool state
    resetStateVariables();
    SetMode( MODE::HOVER );

    updateNetHighlights( false );
    m_frame->GetCanvas()->Refresh();

    // Done
    m_frame->PopTool( aEvent );

    return 0;
}


void DIFF_PHASE_SKEW_TOOL::doInitialHover( const PCB_SELECTION_TOOL* aSelectionTool, GENERAL_COLLECTORS_GUIDE aGuide )
{
    GENERAL_COLLECTOR collector;
    collector.m_Threshold = KiROUND( m_view->ToWorld( INITIAL_HOVER_HITTEST_THRESHOLD_PIXELS ) );

    if( m_frame->GetDisplayOptions().m_ContrastModeDisplay != HIGH_CONTRAST_MODE::NORMAL )
        aGuide.SetIncludeSecondary( false );
    else
        aGuide.SetIncludeSecondary( true );

    aGuide.SetPreferredLayer( m_frame->GetActiveLayer() );
    collector.Collect( m_board, { PCB_TRACE_T, PCB_ARC_T }, m_cursorPos, aGuide );

    if( collector.GetCount() > 1 )
        aSelectionTool->GuessSelectionCandidates( collector, m_cursorPos );

    if( collector.GetCount() > 0 )
    {
        double min_dist_sq = std::numeric_limits<double>::max();

        for( EDA_ITEM* candidate : collector )
        {
            VECTOR2I candidatePos;

            if( candidate->Type() == PCB_TRACE_T )
                candidatePos = static_cast<PCB_TRACK*>( candidate )->GetCenter();
            else if( candidate->Type() == PCB_ARC_T )
                candidatePos = static_cast<PCB_ARC*>( candidate )->GetMid();

            const double dist_sq = ( m_cursorPos - candidatePos ).SquaredEuclideanNorm();

            if( dist_sq < min_dist_sq )
            {
                const auto          bci = static_cast<BOARD_CONNECTED_ITEM*>( candidate );
                const NETINFO_ITEM* candidateNet = bci->GetNet();

                if( GetMode() == MODE::HOVER )
                {
                    min_dist_sq = dist_sq;

                    // We only accept diff pairs in initial hover mode
                    const bool isDiffPairItem =
                            m_drcEngine->IsNetADiffPair( m_board, candidateNet, m_netcodeP, m_netcodeN );
                    m_pickerItemFirst = static_cast<BOARD_CONNECTED_ITEM*>( candidate );

                    if( isDiffPairItem )
                    {
                        m_pickerItemFirstIsDiffPair = true;
                    }
                    else
                    {
                        m_pickerItemFirstIsDiffPair = false;
                        m_netcodeP = candidateNet->GetNetCode();
                    }
                }
                else if( GetMode() == MODE::SELECTED_FIRST )
                {
                    int fakeNCP, fakeNCN;

                    // Reject diff pairs here as we have a not-diff-pair selected
                    const bool isDiffPairItem = m_drcEngine->IsNetADiffPair( m_board, candidateNet, fakeNCP, fakeNCN );

                    auto                existingBci = static_cast<BOARD_CONNECTED_ITEM*>( m_pickerItemFirst );
                    const NETINFO_ITEM* existingNet = existingBci->GetNet();

                    if( !isDiffPairItem && candidateNet != existingNet )
                    {
                        min_dist_sq = dist_sq;
                        m_pickerItemSecond = static_cast<BOARD_CONNECTED_ITEM*>( candidate );
                        m_netcodeN = candidateNet->GetNetCode();
                    }
                }
            }
        }
    }

    updateNetHighlights();
}


void DIFF_PHASE_SKEW_TOOL::buildLengthDelayItems( const PNS::ITEM_SET& aPath, const PNS::SOLID* aStartPad,
                                                  const PNS::SOLID* aEndPad, const NETINFO_ITEM* aNet,
                                                  std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems,
                                                  LENGTH_DELAY_ITEM_DETAILS&                  aItemDetails ) const
{
    if( aPath.Size() == 0 )
        return;

    // Convert path to length / delay interface types
    aItems = m_iface->GetLengthDelayCalculationItems( aPath, aNet->GetNetClass() );
    wxASSERT( aItems.size() == static_cast<size_t>( aPath.Size() ) );

    // The router returns compound lines - we need to split them in to their constituent segments / arcs
    splitLengthItems( aItems );

    // Get the per-item length / delay statistics
    constexpr PATH_OPTIMISATIONS opts = {
        .OptimiseVias = false, .MergeTracks = false, .OptimiseTracesInPads = false, .InferViaInPad = true
    };

    const PAD* startPad = dynamic_cast<PAD*>( aStartPad->BoardItem() );
    const PAD* endPad = dynamic_cast<PAD*>( aEndPad->BoardItem() );
    aItemDetails = LENGTH_DELAY_ITEM_DETAILS{};
    m_board->GetLengthCalculation()->CalculateLengthDetails(
            aItems, opts, startPad, endPad, LENGTH_DELAY_LAYER_OPT::NO_LAYER_DETAIL,
            LENGTH_DELAY_DOMAIN_OPT::WITH_DELAY_DETAIL, &aItemDetails );
    wxASSERT( aItemDetails.LengthsAndDelays.size() == aItems.size() );
}


void DIFF_PHASE_SKEW_TOOL::doDisplayOverlay()
{
    // Extract the diff pair net paths
    getNetPaths();

    // Determine what we are defining as the 'start' of the net
    const DIFF_PAIR_VALIDITY direction = determinePathDirections();

    if( reportValidityErrors( direction ) )
    {
        SetMode( MODE::HOVER );
        return;
    }

    m_timeDomain = false;

    // Check if both nets have time domain parameters
    if( m_selectedNetinfo->GetNetClass()->HasTuningProfile() && m_coupledNetinfo->GetNetClass()->HasTuningProfile() )
    {
        wxString selectedTuningProfileName = m_selectedNetinfo->GetNetClass()->GetTuningProfile();
        wxString coupledTuningProfileName = m_coupledNetinfo->GetNetClass()->GetTuningProfile();

        std::shared_ptr<TUNING_PROFILES> tuningParams = m_frame->Prj().GetProjectFile().TuningProfileParameters();
        const TUNING_PROFILE& selectedTuningProfile = tuningParams->GetTuningProfile( selectedTuningProfileName );
        const TUNING_PROFILE& coupledTuningProfile = tuningParams->GetTuningProfile( coupledTuningProfileName );

        if( selectedTuningProfile.m_EnableTimeDomainTuning && coupledTuningProfile.m_EnableTimeDomainTuning )
            m_timeDomain = true;
    }

    // Construct the length / delay calculation items
    buildLengthDelayItems( m_selectedPath, m_selectedStartPad, m_selectedEndPad, m_selectedNetinfo,
                           m_selectedLengthDelayItems, m_selectedLengthDelayDetails );

    buildLengthDelayItems( m_coupledPath, m_coupledStartPad, m_coupledEndPad, m_coupledNetinfo,
                           m_coupledLengthDelayItems, m_coupledLengthDelayDetails );

    // Build the cumulative length / delay structures
    m_selectedCumulative =
            buildCumulativeLengthsAndDelays( m_selectedLengthDelayItems, m_selectedLengthDelayDetails,
                                             m_selectedStartPad, m_selectedEndPad, m_selectedStartEndDetails );
    m_coupledCumulative =
            buildCumulativeLengthsAndDelays( m_coupledLengthDelayItems, m_coupledLengthDelayDetails, m_coupledStartPad,
                                             m_coupledEndPad, m_coupledStartEndDetails );

    // Walk the two tracks and construct the localised phase differences
    const std::vector<PARALLEL_RUN> parallelRuns = findParallelRuns();

    // Build the known delay reference points for each track
    buildKnownRelativePoints( parallelRuns );

    m_maxSkew.reset();
    buildDiffOverlaySegments( m_targetDiffSegmentSize );

    // Finally draw the overlay
    drawDiffOverlay();
}


void DIFF_PHASE_SKEW_TOOL::buildKnownRelativePoints( const std::vector<PARALLEL_RUN>& aKnownRuns )
{
    m_selectedKnownPoints.clear();
    m_coupledKnownPoints.clear();

    const double padLenDiff =
            static_cast<double>( m_selectedStartEndDetails.StartPadLength - m_coupledStartEndDetails.StartPadLength );
    const double padDelayDiff =
            static_cast<double>( m_selectedStartEndDetails.StartPadDelay - m_coupledStartEndDetails.StartPadDelay );

    struct RELATIVE_PAIR
    {
        double len;
        double delay;
    };

    auto opposite = []( const RELATIVE_PAIR& a )
    {
        return RELATIVE_PAIR{ -a.len, -a.delay };
    };

    for( const auto& r : aKnownRuns )
    {
        const std::size_t segIdxA = r.segA;
        const std::size_t segIdxB = r.segB;

        const double segStartA = segIdxA == 0 ? 0.0 : static_cast<double>( m_selectedCumulative[segIdxA - 1].m_Length );
        const double segStartB = segIdxB == 0 ? 0.0 : static_cast<double>( m_coupledCumulative[segIdxB - 1].m_Length );
        const double segLenA = static_cast<double>( m_selectedLengthDelayDetails.LengthsAndDelays[segIdxA].first );
        const double segLenB = static_cast<double>( m_coupledLengthDelayDetails.LengthsAndDelays[segIdxB].first );

        const double s0A = segStartA + r.ta0 * segLenA;
        const double s1A = segStartA + r.ta1 * segLenA;
        const double s0B = segStartB + r.tb0 * segLenB;
        const double s1B = segStartB + r.tb1 * segLenB;

        const RELATIVE_PAIR startRel{ ( r.startLenA - r.startLenB ) + padLenDiff,
                                      ( r.startDelayA - r.startDelayB ) + padDelayDiff };
        const RELATIVE_PAIR endRel{ ( r.endLenA - r.endLenB ) + padLenDiff,
                                    ( r.endDelayA - r.endDelayB ) + padDelayDiff };
        const RELATIVE_PAIR startRelB = opposite( startRel );
        const RELATIVE_PAIR endRelB = opposite( endRel );

        const bool hasStartViaA = r.startViaLengthA.has_value();
        const bool hasEndViaA = r.endViaLengthA.has_value();
        const bool hasStartViaB = r.startViaLengthB.has_value();
        const bool hasEndViaB = r.endViaLengthB.has_value();

        const double startViaLenA = r.startViaLengthA.value_or( 0.0 );
        const double endViaLenA = r.endViaLengthA.value_or( 0.0 );
        const double startViaLenB = r.startViaLengthB.value_or( 0.0 );
        const double endViaLenB = r.endViaLengthB.value_or( 0.0 );

        const double startViaDelayA = r.startViaDelayA.value_or( 0.0 );
        const double endViaDelayA = r.endViaDelayA.value_or( 0.0 );
        const double startViaDelayB = r.startViaDelayB.value_or( 0.0 );
        const double endViaDelayB = r.endViaDelayB.value_or( 0.0 );

        // Injected start-via points
        if( hasStartViaA && hasStartViaB )
        {
            const RELATIVE_PAIR rel{ startRel.len - startViaLenA + startViaLenB,
                                     startRel.delay - startViaDelayA + startViaDelayB };
            const RELATIVE_PAIR relB = opposite( rel );
            m_selectedKnownPoints.push_back( { s0A - startViaLenA, rel.len, rel.delay, rel.len, rel.delay } );
            m_coupledKnownPoints.push_back( { s0B - startViaLenB, relB.len, relB.delay, relB.len, relB.delay } );
        }

        // Maybe-modified values used for interpolation around via discontinuities
        RELATIVE_PAIR startBeforeA = startRel;
        RELATIVE_PAIR startBeforeB = startRelB;

        if( hasStartViaA && !hasStartViaB )
        {
            startBeforeB.len += startViaLenA;
            startBeforeB.delay += startViaDelayA;
            m_selectedKnownPoints.push_back( { s0A - startViaLenA, startRel.len - startViaLenA,
                                               startRel.delay - startViaDelayA, startRel.len - startViaLenA,
                                               startRel.delay - startViaDelayA } );
        }
        else if( !hasStartViaA && hasStartViaB )
        {
            startBeforeA.len += startViaLenB;
            startBeforeA.delay += startViaDelayB;
            m_coupledKnownPoints.push_back( { s0B - startViaLenB, startRelB.len - startViaLenB,
                                              startRelB.delay - startViaDelayB, startRelB.len - startViaLenB,
                                              startRelB.delay - startViaDelayB } );
        }

        // Maybe-modified values used for interpolation around via discontinuities
        RELATIVE_PAIR endAfterA = endRel;
        RELATIVE_PAIR endAfterB = endRelB;

        if( hasEndViaA && !hasEndViaB )
        {
            endAfterB.len -= endViaLenA;
            endAfterB.delay -= endViaDelayA;
        }
        else if( !hasEndViaA && hasEndViaB )
        {
            endAfterA.len -= endViaLenB;
            endAfterA.delay -= endViaDelayB;
        }

        // Main known points
        m_selectedKnownPoints.push_back( { s0A, startBeforeA.len, startBeforeA.delay, startRel.len, startRel.delay } );
        m_selectedKnownPoints.push_back( { s1A, endRel.len, endRel.delay, endAfterA.len, endAfterA.delay } );
        m_coupledKnownPoints.push_back( { s0B, startBeforeB.len, startBeforeB.delay, startRelB.len, startRelB.delay } );
        m_coupledKnownPoints.push_back( { s1B, endRelB.len, endRelB.delay, endAfterB.len, endAfterB.delay } );

        // Injected end-via points
        if( hasEndViaA && hasEndViaB )
        {
            const RELATIVE_PAIR rel{ endRel.len + endViaLenA - endViaLenB, endRel.delay + endViaDelayA - endViaDelayB };
            const RELATIVE_PAIR relB = opposite( rel );
            m_selectedKnownPoints.push_back( { s1A + endViaLenA, rel.len, rel.delay, rel.len, rel.delay } );
            m_coupledKnownPoints.push_back( { s1B + endViaLenB, relB.len, relB.delay, relB.len, relB.delay } );
        }
        else if( hasEndViaA && !hasEndViaB )
        {
            m_selectedKnownPoints.push_back( { s1A + endViaLenA, endRel.len + endViaLenA, endRel.delay + endViaDelayA,
                                               endRel.len + endViaLenA, endRel.delay + endViaDelayA } );
        }
        else if( !hasEndViaA && hasEndViaB )
        {
            m_coupledKnownPoints.push_back( { s1B + endViaLenB, endRelB.len + endViaLenB, endRelB.delay + endViaDelayB,
                                              endRelB.len + endViaLenB, endRelB.delay + endViaDelayB } );
        }
    }
}


std::vector<double> DIFF_PHASE_SKEW_TOOL::buildSplitPositions( const std::vector<CUMULATIVE_ENTRY>& aSegments,
                                                               const double aTargetSubsegmentSize )
{
    if( aSegments.empty() )
        return {};

    std::vector<double> splits;

    // Start of line
    if( aSegments[0].m_SourceType == LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE )
        splits.push_back( 0.0 );

    double currentDistance = 0;

    for( const CUMULATIVE_ENTRY& seg : aSegments )
    {
        const double segStart = currentDistance;
        const double segEnd = seg.m_Length;
        currentDistance = segEnd;

        // Only emit segments for lines
        if( seg.m_SourceType != LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE )
            continue;

        // Fixed subdivision spacing
        for( double s = segStart; s < segEnd; s += aTargetSubsegmentSize )
        {
            splits.push_back( s );
        }

        splits.push_back( segEnd );
    }

    // Sort and make unique
    std::ranges::sort( splits );

    splits.erase( std::ranges::unique( splits,
                                       []( const double a, const double b )
                                       {
                                           return std::abs( a - b ) < EPS;
                                       } )
                          .begin(),
                  splits.end() );

    return splits;
}


std::pair<VECTOR2D, std::size_t>
DIFF_PHASE_SKEW_TOOL::pointAtDistance( const std::vector<CUMULATIVE_ENTRY>&              aSegments,
                                       const std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aSourceItemDetails,
                                       const double                                      aDist )
{
    for( std::size_t i = 0; i < aSegments.size(); ++i )
    {
        const double segStart = i == 0 ? 0.0 : static_cast<double>( aSegments[i - 1].m_Length );
        const double segEnd = static_cast<double>( aSegments[i].m_Length );

        if( aDist <= segEnd + EPS )
        {
            const double segLen = segEnd - segStart;

            double t = 0.0;

            if( segLen > EPS )
                t = ( aDist - segStart ) / segLen;

            t = std::clamp( t, 0.0, 1.0 );

            if( aSourceItemDetails[i].Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::VIA )
            {
                // We've hit a via - use the end point from the previous line
                wxASSERT( i > 0 );
                wxASSERT( aSourceItemDetails[i - 1].Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE );
                wxASSERT( aSourceItemDetails[i - 1].GetLine().CPoints().size() == 2 );
                return { aSourceItemDetails[i - 1].GetLine().CPoints()[1], i - 1 };
            }

            wxASSERT( aSourceItemDetails[i].Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE );
            wxASSERT( aSourceItemDetails[i].GetLine().CPoints().size() == 2 );

            return {
                lerp( aSourceItemDetails[i].GetLine().CPoints()[0], aSourceItemDetails[i].GetLine().CPoints()[1], t ), i
            };
        }
    }

    // We shouldn't reach this point...
    wxASSERT( false );
    return { { 0.0, 0.0 }, 0 };
}


COLOR4D DIFF_PHASE_SKEW_TOOL::interpolateColours( const COLOR4D& aColour1, const COLOR4D& aColour2, double aS,
                                                  const bool aUseLogScale ) const
{
    auto lerp = []( const double d1, const double d2, const double s )
    {
        return d1 + s * ( d2 - d1 );
    };

    if( aUseLogScale )
        aS = std::log( 1.0 + m_colourInterpolationLogStrength * aS )
             / std::log( 1.0 + m_colourInterpolationLogStrength );

    const double r = std::clamp( lerp( aColour1.r, aColour2.r, aS ), 0.0, 1.0 );
    const double g = std::clamp( lerp( aColour1.g, aColour2.g, aS ), 0.0, 1.0 );
    const double b = std::clamp( lerp( aColour1.b, aColour2.b, aS ), 0.0, 1.0 );
    const double a = std::clamp( lerp( aColour1.a, aColour2.a, aS ), 0.0, 1.0 );

    return COLOR4D( r, g, b, a );
}


void DIFF_PHASE_SKEW_TOOL::buildDiffOverlaySegments( const double aTargetSubsegmentSize )
{
    m_selectedDiffs = buildDiffOverlaySegmentsImpl( m_selectedCumulative, m_selectedLengthDelayItems,
                                                    m_selectedKnownPoints, aTargetSubsegmentSize );
    m_coupledDiffs = buildDiffOverlaySegmentsImpl( m_coupledCumulative, m_coupledLengthDelayItems, m_coupledKnownPoints,
                                                   aTargetSubsegmentSize );
}


std::vector<DIFF_PHASE_SKEW_TOOL::OUTPUT_SEGMENT> DIFF_PHASE_SKEW_TOOL::buildDiffOverlaySegmentsImpl(
        const std::vector<CUMULATIVE_ENTRY>&              aSegments,
        const std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aSourceItemDetails,
        const std::vector<KNOWN_RELATIVE_POINT>& aKnownPoints, double aTargetSubsegmentSize )
{
    std::vector<OUTPUT_SEGMENT> result;

    if( aSegments.empty() )
        return result;

    PCBNEW_SETTINGS*                           cfg = m_frame->GetPcbNewSettings();
    PCBNEW_SETTINGS::DIFF_PHASE_SKEW_SETTINGS& settings = cfg->m_DiffPhaseSkewSettings;

    // Get min and max values
    double minLen = 0.0;
    double maxLen = 0.0;
    double minDelay = 0.0;
    double maxDelay = 0.0;

    for( const auto& [_1, relLenBefore, relDelayBefore, relLenAfter, relDelayAfter] : aKnownPoints )
    {
        minLen = std::min( minLen, std::min( relLenBefore, relLenAfter ) );
        maxLen = std::max( maxLen, std::max( relLenBefore, relLenAfter ) );
        minDelay = std::min( minDelay, std::min( relDelayBefore, relDelayAfter ) );
        maxDelay = std::max( maxDelay, std::max( relDelayBefore, relDelayAfter ) );
    }

    int maxSkew = m_maxSkew.value_or( 0 );

    if( m_timeDomain )
        m_maxSkew = std::max( static_cast<int>( std::round( maxDelay / 10 ) * 10 ), maxSkew );
    else
        m_maxSkew = std::max( static_cast<int>( std::round( maxLen / 10 ) * 10 ), maxSkew );

    // Build all subdivision boundaries
    const auto splits = buildSplitPositions( aSegments, aTargetSubsegmentSize );

    KnownValueInterpolator interp( aKnownPoints );

    // Emit subdivided segments
    for( std::size_t i = 0; i + 1 < splits.size(); ++i )
    {
        const double s0 = splits[i];
        const double s1 = splits[i + 1];

        // Skip degenerate intervals
        if( s1 - s0 <= EPS )
            continue;

        OUTPUT_SEGMENT out;

        auto [startPoint, segIdx] = pointAtDistance( aSegments, aSourceItemDetails, s0 );
        out.Width = static_cast<int>( aSourceItemDetails[segIdx].GetWidth() * m_overlayTrackInflation );
        out.Start = startPoint;
        auto [endPoint, _] = pointAtDistance( aSegments, aSourceItemDetails, s1 );
        out.End = endPoint;

        const double sMid = ( s0 + s1 ) / 2.0;

        const std::optional<std::pair<double, double>> knownInterp = interp.ValueAt( sMid );
        out.RelativeValueKnown = knownInterp.has_value();

        double min = 0.0;
        double max = 0.0;

        if( m_timeDomain )
        {
            out.RelativeValueAtMid = knownInterp.value_or( std::pair<double, double>{ 0.0, 0.0 } ).second;
            min = minDelay;
            max = maxDelay;
        }
        else
        {
            out.RelativeValueAtMid = knownInterp.value_or( std::pair<double, double>{ 0.0, 0.0 } ).first;
            min = minLen;
            max = maxLen;
        }

        // Round value to nearest 10 IU to reduce low-level colour jitter on equal tracks
        out.RelativeValueAtMid = std::round( out.RelativeValueAtMid / 10 ) * 10;

        // Calculate colour value
        if( !out.RelativeValueKnown )
        {
            out.Colour = settings.m_UnknownSkewColor;
        }
        else if( out.RelativeValueAtMid < 0.0 )
        {
            const double frac = fabs( out.RelativeValueAtMid / min );
            out.Colour = interpolateColours( settings.m_ZeroSkewColor, settings.m_NegativeSkewColor, frac,
                                             settings.m_UseLogScale );
        }
        else if( out.RelativeValueAtMid > 0.0 )
        {
            const double frac = fabs( out.RelativeValueAtMid / max );
            out.Colour = interpolateColours( settings.m_ZeroSkewColor, settings.m_PositiveSkewColor, frac,
                                             settings.m_UseLogScale );
        }
        else
        {
            out.Colour = settings.m_ZeroSkewColor;
        }

        result.push_back( out );
    }

    return result;
}


void DIFF_PHASE_SKEW_TOOL::drawDiffOverlay() const
{
    clearOverlay();

    const std::size_t selIdx = m_segmentForStatisticsDisplay.first;
    const bool        isSelected = m_segmentForStatisticsDisplay.second;
    const bool        drawHighlight = selIdx < std::numeric_limits<std::size_t>::max();

    m_viewOverlay->SetIsStroke( true );
    m_viewOverlay->SetIsFill( false );

    for( std::size_t i = 0; i < m_selectedDiffs.size(); ++i )
    {
        const OUTPUT_SEGMENT& segment = m_selectedDiffs[i];

        if( drawHighlight && isSelected && i == selIdx )
            m_viewOverlay->SetStrokeColor( COLOR4D( 1.0, 0.0, 0.937, 1.0 ) );
        else
            m_viewOverlay->SetStrokeColor( segment.Colour );

        m_viewOverlay->Segment( segment.Start, segment.End, segment.Width );
    }

    for( std::size_t i = 0; i < m_coupledDiffs.size(); ++i )
    {
        const OUTPUT_SEGMENT& segment = m_coupledDiffs[i];

        if( drawHighlight && !isSelected && i == selIdx )
            m_viewOverlay->SetStrokeColor( COLOR4D( 1.0, 0.0, 0.937, 1.0 ) );
        else
            m_viewOverlay->SetStrokeColor( segment.Colour );

        m_viewOverlay->Segment( segment.Start, segment.End, segment.Width );
    }

    std::vector<MSG_PANEL_ITEM> items;
    wxString                    description, value;

    updateOverlay();
}


void DIFF_PHASE_SKEW_TOOL::updateMessagePanel() const
{
    std::vector<MSG_PANEL_ITEM> items;

    if( !m_pickerItemFirst )
    {
        frame()->SetMsgPanel( m_board );
        return;
    }

    if( !m_pickerItemFirstIsDiffPair )
    {
        wxString description = wxString::Format( _( "Net A Name" ) );
        wxString netName = m_pickerItemFirst->GetNet()->GetDisplayNetname();
        items.emplace_back( description, netName );

        if( m_pickerItemSecond )
        {
            description = wxString::Format( _( "Net B Name" ) );
            netName = m_pickerItemSecond->GetNet()->GetDisplayNetname();
            items.emplace_back( description, netName );
        }
    }
    else
    {
        wxString description = wxString::Format( _( "Net P Name" ) );
        wxString netName = m_board->GetNetInfo().GetNetItem( m_netcodeP )->GetDisplayNetname();
        items.emplace_back( description, netName );

        description = wxString::Format( _( "Net N Name" ) );
        netName = m_board->GetNetInfo().GetNetItem( m_netcodeN )->GetDisplayNetname();
        items.emplace_back( description, netName );
    }

    if( m_maxSkew.has_value() )
    {
        wxString description = wxString::Format( _( "Max Skew" ) );
        wxString value;

        if( m_timeDomain )
            value = m_frame->MessageTextFromValue( m_maxSkew.value(), true, EDA_DATA_TYPE::TIME );
        else
            value = m_frame->MessageTextFromValue( m_maxSkew.value(), true, EDA_DATA_TYPE::DISTANCE );

        items.emplace_back( description, value );
    }

    const std::size_t selIdx = m_segmentForStatisticsDisplay.first;
    const bool        isSelected = m_segmentForStatisticsDisplay.second;
    const bool        drawHighlight = selIdx < std::numeric_limits<std::size_t>::max();

    if( drawHighlight )
    {
        const OUTPUT_SEGMENT& segment = isSelected ? m_selectedDiffs[selIdx] : m_coupledDiffs[selIdx];
        double                normalisedValue = std::round( segment.RelativeValueAtMid );
        normalisedValue = ( normalisedValue == 0.0 ) ? 0.0 : normalisedValue;

        wxString description = _( "Local Skew" );
        wxString value = _( "Unknown" );

        if( segment.RelativeValueKnown )
        {
            value = m_frame->MessageTextFromValue( normalisedValue, true,
                                                   m_timeDomain ? EDA_DATA_TYPE::TIME : EDA_DATA_TYPE::DISTANCE );
        }


        items.emplace_back( description, value );
    }

    frame()->SetMsgPanel( items );
}


int DIFF_PHASE_SKEW_TOOL::getMaxDiffPairGap( const BOARD_CONNECTED_ITEM* aItem ) const
{
    if( m_pickerItemFirstIsDiffPair )
    {
        // TODO: This assumes the gap is the same across all traces, but actually this can vary
        // TODO: by layer. We should get a representative segment for each layer from the
        // TODO: two tracks and find the max constraint across all of them.
        const DRC_CONSTRAINT constraint =
                m_drcEngine->EvalRules( DIFF_PAIR_GAP_CONSTRAINT, aItem, nullptr, aItem->GetLayer() );

        if( constraint.IsNull() || constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
            return std::numeric_limits<int>::max();

        const MINOPTMAX<int>& val = constraint.GetValue();

        if( val.HasOpt() && val.HasMax() )
            return std::max( val.Max(), val.Opt() );
        else if( val.HasMax() )
            return val.Max();
        else if( val.HasOpt() )
            return val.Opt();

        return std::numeric_limits<int>::max();
    }
    else
    {
        const VECTOR2I dist = m_originFirst - m_originSecond;
        return dist.EuclideanNorm();
    }
}


void DIFF_PHASE_SKEW_TOOL::findParallelRunsImpl( std::pair<std::size_t, std::size_t> aRangeA,
                                                 std::pair<std::size_t, std::size_t> aRangeB, double aMaxSpacing,
                                                 std::vector<PARALLEL_RUN>& aRuns ) const
{
    for( size_t ia = aRangeA.first; ia < aRangeA.second; ++ia )
    {
        const LENGTH_DELAY_CALCULATION_ITEM& selectedItem = m_selectedLengthDelayItems[ia];

        if( selectedItem.Type() != LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE )
            continue;

        const SHAPE_LINE_CHAIN& lineA = selectedItem.GetLine();
        wxASSERT( lineA.SegmentCount() == 1 );
        const SEG segA = lineA.Segment( 0 );

        VECTOR2D A0 = segA.A;
        VECTOR2D A1 = segA.B;

        VECTOR2D     dA = A1 - A0;
        const double lenA = dA.EuclideanNorm();
        VECTOR2D     nA{ dA.x / lenA, dA.y / lenA };

        for( size_t ib = aRangeB.first; ib < aRangeB.second; ++ib )
        {
            const LENGTH_DELAY_CALCULATION_ITEM& coupledItem = m_coupledLengthDelayItems[ib];

            if( coupledItem.Type() != LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE )
                continue;

            if( selectedItem.GetStartLayer() != coupledItem.GetStartLayer() )
                continue;

            const SHAPE_LINE_CHAIN& lineB = coupledItem.GetLine();
            wxASSERT( lineB.SegmentCount() == 1 );
            const SEG segB = lineB.Segment( 0 );

            VECTOR2D B0 = segB.A;
            VECTOR2D B1 = segB.B;

            VECTOR2D     dB = B1 - B0;
            const double lenB = dB.EuclideanNorm();
            VECTOR2D     nB{ dB.x / lenB, dB.y / lenB };

            // Test for parallel line segments
            const double dp = nA.Dot( nB );

            // Note that this test is explicitly signed (not fabs(dp)) to ensure anti-parallel tracks are rejected
            if( dp < m_cosThetaParallelTestValue )
                continue;

            // Test for perpendicular distance
            VECTOR2D midB = ( B0 + B1 ) * 0.5;

            // Perpendicular distance from midpoint of B to infinite line through A
            const double perpDistance = std::fabs( nA.Cross( midB - A0 ) );

            const double maxItemGap =
                    ( aMaxSpacing + ( selectedItem.GetWidth() + coupledItem.GetWidth() ) / 2.0 ) * m_trackGapInflation;

            if( perpDistance > maxItemGap )
                continue;

            // Project on to common axis
            double a0 = A0.Dot( nA );
            double a1 = A1.Dot( nA );
            double b0 = B0.Dot( nA );
            double b1 = B1.Dot( nA );

            bool aReversed = false;
            bool bReversed = false;

            if( a0 > a1 )
            {
                std::swap( a0, a1 );
                aReversed = true;
            }

            if( b0 > b1 )
            {
                std::swap( b0, b1 );
                bReversed = true;
            }

            // Compute overlap interval
            double overlap0 = std::max( a0, b0 );
            double overlap1 = std::min( a1, b1 );

            // Test for overlap
            if( overlap1 <= overlap0 )
                continue;

            // Convert overlap to segment parameters
            double tA0 = ( overlap0 - a0 ) / ( a1 - a0 );
            double tA1 = ( overlap1 - a0 ) / ( a1 - a0 );
            double tB0 = ( overlap0 - b0 ) / ( b1 - b0 );
            double tB1 = ( overlap1 - b0 ) / ( b1 - b0 );

            // Handle reversed parameterization
            if( aReversed )
            {
                tA0 = 1.0 - tA0;
                tA1 = 1.0 - tA1;
            }

            if( bReversed )
            {
                tB0 = 1.0 - tB0;
                tB1 = 1.0 - tB1;
            }

            // Normalize parameter ordering
            if( tA0 > tA1 )
                std::swap( tA0, tA1 );

            if( tB0 > tB1 )
                std::swap( tB0, tB1 );

            // Clamp to physical range
            tA0 = std::clamp( tA0, 0.0, 1.0 );
            tA1 = std::clamp( tA1, 0.0, 1.0 );

            tB0 = std::clamp( tB0, 0.0, 1.0 );
            tB1 = std::clamp( tB1, 0.0, 1.0 );

            const CUMULATIVE_ENTRY& thisSelCumItem = m_selectedCumulative[ia];
            const CUMULATIVE_ENTRY& thisCoupledCumItem = m_coupledCumulative[ib];

            auto HasStartingVia = []( const std::vector<CUMULATIVE_ENTRY>& cumulative, const std::size_t index,
                                      const CUMULATIVE_ENTRY& current )
            {
                if( index == 0 )
                    return false;

                const auto& prev = cumulative[index - 1];

                return prev.m_SourceType == LENGTH_DELAY_CALCULATION_ITEM::TYPE::VIA && current.m_Start == prev.m_End;
            };

            auto HasEndingVia = []( const std::vector<CUMULATIVE_ENTRY>& cumulative, const std::size_t index,
                                    const CUMULATIVE_ENTRY& current )
            {
                if( index + 1 >= cumulative.size() )
                    return false;

                const auto& next = cumulative[index + 1];

                return next.m_SourceType == LENGTH_DELAY_CALCULATION_ITEM::TYPE::VIA && current.m_End == next.m_Start;
            };

            // Emit the parallel run
            PARALLEL_RUN run;

            run.ta0 = tA0;
            run.ta1 = tA1;
            run.tb0 = tB0;
            run.tb1 = tB1;

            run.segA = ia;
            run.segB = ib;

            // Calculate start and end points as linear interpolations along segments
            run.startA = lerp( A0, A1, tA0 );
            run.endA = lerp( A0, A1, tA1 );
            run.startB = lerp( B0, B1, tB0 );
            run.endB = lerp( B0, B1, tB1 );

            // Calculate cumulative values for deltas
            auto [length1, delay1] = getCumulativeLengthAndDelayAt(
                    m_selectedLengthDelayDetails, m_selectedStartEndDetails, m_selectedCumulative, ia, run.ta0 );
            run.startLenA = length1;
            run.startDelayA = delay1;

            auto [length2, delay2] = getCumulativeLengthAndDelayAt(
                    m_selectedLengthDelayDetails, m_selectedStartEndDetails, m_selectedCumulative, ia, run.ta1 );
            run.endLenA = length2;
            run.endDelayA = delay2;

            auto [length3, delay3] = getCumulativeLengthAndDelayAt(
                    m_coupledLengthDelayDetails, m_coupledStartEndDetails, m_coupledCumulative, ib, run.tb0 );
            run.startLenB = length3;
            run.startDelayB = delay3;

            auto [length4, delay4] = getCumulativeLengthAndDelayAt(
                    m_coupledLengthDelayDetails, m_coupledStartEndDetails, m_coupledCumulative, ib, run.tb1 );
            run.endLenB = length4;
            run.endDelayB = delay4;

            // Add start / end via length and delay information
            if( std::abs( tA0 ) < EPS && HasStartingVia( m_selectedCumulative, ia, thisSelCumItem ) )
            {
                run.startViaLengthA = m_selectedLengthDelayDetails.LengthsAndDelays[ia - 1].first;
                run.startViaDelayA = m_selectedLengthDelayDetails.LengthsAndDelays[ia - 1].second;
            }

            if( std::abs( tB0 ) < EPS && HasStartingVia( m_coupledCumulative, ib, thisCoupledCumItem ) )
            {
                run.startViaLengthB = m_coupledLengthDelayDetails.LengthsAndDelays[ib - 1].first;
                run.startViaDelayB = m_coupledLengthDelayDetails.LengthsAndDelays[ib - 1].second;
            }

            if( std::abs( tA1 - 1.0 ) < EPS && HasEndingVia( m_selectedCumulative, ia, thisSelCumItem ) )
            {
                run.endViaLengthA = m_selectedLengthDelayDetails.LengthsAndDelays[ia + 1].first;
                run.endViaDelayA = m_selectedLengthDelayDetails.LengthsAndDelays[ia + 1].second;
            }

            if( std::abs( tB1 - 1.0 ) < EPS && HasEndingVia( m_coupledCumulative, ib, thisCoupledCumItem ) )
            {
                run.endViaLengthB = m_coupledLengthDelayDetails.LengthsAndDelays[ib + 1].first;
                run.endViaDelayB = m_coupledLengthDelayDetails.LengthsAndDelays[ib + 1].second;
            }

            aRuns.push_back( run );
        }
    }
}


std::vector<PARALLEL_RUN> DIFF_PHASE_SKEW_TOOL::findParallelRuns() const
{
    std::vector<PARALLEL_RUN> runs;
    const double              maxGap = getMaxDiffPairGap( m_pickerItemFirst );

    // First find runs with regular spacing
    findParallelRunsImpl( { 0, m_selectedCumulative.size() }, { 0, m_coupledCumulative.size() }, maxGap, runs );

    if( runs.empty() )
        return {};

    // Check what the min and max segment IDs of each track are
    const auto [minSelected, maxSelected] = std::ranges::minmax( runs, {},
                                                                 []( const PARALLEL_RUN& a )
                                                                 {
                                                                     return a.segA;
                                                                 } );

    const auto [minCoupled, maxCoupled] = std::ranges::minmax( runs, {},
                                                               []( const PARALLEL_RUN& a )
                                                               {
                                                                   return a.segB;
                                                               } );

    // Find parallel segments with start separation if needed
    const std::size_t firstSegA = minSelected.segA;
    const std::size_t lastSegA = maxSelected.segA;
    const std::size_t firstSegB = minCoupled.segA;
    const std::size_t lastSegB = maxCoupled.segA;

    // Assume that tracks start in parallel from pads that are wider than the diff pair spacing. Use the pad separation
    // to search for start tracks up to the start of the identified existing parallel segments
    if( firstSegA > 0 || firstSegB > 0 )
    {
        const VECTOR2I selPadLocn = m_selectedStartPad->Pos();
        const VECTOR2I coupledPadLocn = m_coupledStartPad->Pos();
        const VECTOR2D distVec = selPadLocn - coupledPadLocn;
        const double   padSeparation = distVec.EuclideanNorm();

        findParallelRunsImpl( { 0, firstSegA }, { 0, firstSegB }, padSeparation, runs );
    }

    // Assume that tracks end in parallel from pads that are wider than the diff pair spacing. Use the pad separation
    // to search for end tracks from the end of the identified existing parallel segments
    if( lastSegA < m_selectedCumulative.size() || lastSegB > m_coupledCumulative.size() )
    {
        const VECTOR2I selPadLocn = m_selectedEndPad->Pos();
        const VECTOR2I coupledPadLocn = m_coupledEndPad->Pos();
        const VECTOR2D distVec = selPadLocn - coupledPadLocn;
        const double   padSeparation = distVec.EuclideanNorm();

        findParallelRunsImpl( { lastSegA, m_selectedCumulative.size() }, { lastSegB, m_coupledCumulative.size() },
                              padSeparation, runs );
    }

    std::ranges::sort( runs,
                       []( const PARALLEL_RUN& a, const PARALLEL_RUN& b )
                       {
                           if( a.startLenA != b.startLenA )
                           {
                               return a.startLenA < b.startLenA;
                           }

                           return a.startLenB < b.startLenB;
                       } );

    return runs;
}


std::pair<int64_t, int64_t> DIFF_PHASE_SKEW_TOOL::getCumulativeLengthAndDelayAt(
        const LENGTH_DELAY_ITEM_DETAILS& aLengthDelayDetails, const START_END_DETAILS& aPadDetails,
        const std::vector<CUMULATIVE_ENTRY>& aCumulative, const std::size_t aSegIdx, const double aT )
{
    const double segLength = static_cast<double>( aLengthDelayDetails.LengthsAndDelays[aSegIdx].first );
    const double segDelay = static_cast<double>( aLengthDelayDetails.LengthsAndDelays[aSegIdx].second );

    // cumulative[i] is the cumulative length / delay at the end of the given segment index. Therefore, subtract
    // the not-included fraction of the track from the cumulative value at the segment end to get the length or
    // delay at the required fractional distance on the source segment
    const double partLen = segLength * ( 1.0 - aT );
    const double partDelay = segDelay * ( 1.0 - aT );
    return { aCumulative[aSegIdx].m_Length - static_cast<int64_t>( partLen ) + aPadDetails.StartPadLength,
             aCumulative[aSegIdx].m_Delay - static_cast<int64_t>( partDelay ) + aPadDetails.StartPadDelay };
}


std::vector<DIFF_PHASE_SKEW_TOOL::CUMULATIVE_ENTRY> DIFF_PHASE_SKEW_TOOL::buildCumulativeLengthsAndDelays(
        const std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems, const LENGTH_DELAY_ITEM_DETAILS& aLengthDelayDetails,
        const PNS::SOLID* aStartPad, const PNS::SOLID* aEndPad, START_END_DETAILS& aStartEndDetails )
{
    wxASSERT( aItems.size() == aLengthDelayDetails.LengthsAndDelays.size() );

    if( aLengthDelayDetails.LengthsAndDelays.empty() )
        return {};

    std::vector<CUMULATIVE_ENTRY> cumulative;
    cumulative.reserve( aLengthDelayDetails.LengthsAndDelays.size() );

    // Calculate start and end pad details
    aStartEndDetails.StartPadLength = aStartPad->GetPadToDie() + aLengthDelayDetails.InferredStartViaLength;
    aStartEndDetails.StartPadDelay = aStartPad->GetPadToDieDelay() + aLengthDelayDetails.InferredStartViaDelay;
    aStartEndDetails.EndPadLength = aEndPad->GetPadToDie() + aLengthDelayDetails.InferredEndViaLength;
    aStartEndDetails.EndPadDelay = aEndPad->GetPadToDieDelay() + aLengthDelayDetails.InferredEndViaDelay;

    int64_t totalLength = 0;
    int64_t totalDelay = 0;

    // Add track element details. Note that this adds the cumulative length at the *end*
    // of each track element to the cumulative vector.
    for( std::size_t i = 0; i < aItems.size(); ++i )
    {
        const auto [itemLen, itemDly] = aLengthDelayDetails.LengthsAndDelays[i];
        const LENGTH_DELAY_CALCULATION_ITEM& item = aItems[i];

        VECTOR2I start, end;

        if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE )
        {
            start = item.GetLine().CPoint( 0 );
            end = item.GetLine().CLastPoint();
        }
        else if( item.Type() == LENGTH_DELAY_CALCULATION_ITEM::TYPE::VIA )
        {
            start = item.GetVia()->GetPosition();
            end = start;
        }

        totalLength += itemLen;
        totalDelay += itemDly;
        cumulative.emplace_back( totalLength, totalDelay, item.Type(), start, end );
    }

    return cumulative;
}


void DIFF_PHASE_SKEW_TOOL::splitLengthItems( std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems )
{
    std::vector<LENGTH_DELAY_CALCULATION_ITEM> splitItems;

    auto makeLengthDelayItem = [&splitItems]( const SEG& aSeg, const LENGTH_DELAY_CALCULATION_ITEM& aSourceItem )
    {
        SHAPE_LINE_CHAIN newLine;
        newLine.Append( aSeg.A );
        newLine.Append( aSeg.B );

        LENGTH_DELAY_CALCULATION_ITEM newItem;
        newItem.SetLine( newLine );
        newItem.SetWidth( aSourceItem.GetWidth() );
        newItem.SetLayers( aSourceItem.GetStartLayer() );
        newItem.SetEffectiveNetClass( aSourceItem.GetEffectiveNetClass() );
        splitItems.emplace_back( std::move( newItem ) );
    };

    for( const auto& sourceItem : aItems )
    {
        // Only process lines
        if( sourceItem.Type() != LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE )
        {
            splitItems.emplace_back( sourceItem );
            continue;
        }

        SHAPE_LINE_CHAIN& line = sourceItem.GetLine();

        for( int segIdx = 0; segIdx < line.SegmentCount(); ++segIdx )
        {
            SEG seg = line.GetSegment( segIdx );
            makeLengthDelayItem( seg, sourceItem );
        }
    }

    aItems = std::move( splitItems );
}


bool DIFF_PHASE_SKEW_TOOL::reportValidityErrors( const DIFF_PAIR_VALIDITY aDirection ) const
{
    wxString message;
    bool     error = true;

    switch( aDirection )
    {
    case DIFF_PAIR_VALIDITY::MULTIPLE_SOURCES_SELECTED_TRACK:
        message = wxString::Format( _( "Net %s has multiple simulation electrical source pads" ),
                                    m_selectedNetinfo->GetShortNetname() );
        break;
    case DIFF_PAIR_VALIDITY::MULTIPLE_SOURCES_COUPLED_TRACK:
        message = wxString::Format( _( "Net %s has multiple simulation electrical source pads" ),
                                    m_coupledNetinfo->GetShortNetname() );
        break;
    case DIFF_PAIR_VALIDITY::NO_START_OR_END_PADS:
        message = wxString::Format( _( "Differential pair %s / %s is missing start and / or end pads" ),
                                    m_selectedNetinfo->GetShortNetname(), m_coupledNetinfo->GetShortNetname() );
        break;
    case DIFF_PAIR_VALIDITY::NO_SOURCE_PADS_SELECTED_TRACK:
        message = wxString::Format( _( "Net %s is missing electrical simulation source pad" ),
                                    m_selectedNetinfo->GetShortNetname() );
        break;
    case DIFF_PAIR_VALIDITY::NO_SOURCE_PADS_COUPLED_TRACK:
        message = wxString::Format( _( "Net %s is missing electrical simulation source pad" ),
                                    m_coupledNetinfo->GetShortNetname() );
        break;
    default: error = false; break;
    }

    if( error )
        m_frame->ShowInfoBarError( message, true );

    return error;
}


void DIFF_PHASE_SKEW_TOOL::getNetPaths()
{
    if( m_pickerItemFirstIsDiffPair )
    {
        const int pnsLayer = m_iface->GetPNSLayerFromBoardLayer( m_pickerItemFirst->GetLayer() );

        PCB_TRACK* track = nullptr;
        m_originFirst = PNS::HELPERS::SnapToNearestTrack( m_originFirst, m_board, nullptr, &track );
        wxCHECK( track, /* void */ );

        // Determine primary and secondary net codes
        m_selectedNetcode = track->GetNetCode();
        m_coupledNetcode = m_selectedNetcode == m_netcodeP ? m_netcodeN : m_netcodeP;

        // Get the netcodes
        m_selectedNetinfo = m_board->GetNetInfo().GetNetItem( m_selectedNetcode );
        m_coupledNetinfo = m_board->GetNetInfo().GetNetItem( m_coupledNetcode );

        VECTOR2I          startSnapPoint;
        PNS::LINKED_ITEM* startItem =
                PNS::HELPERS::PickSegment( m_router, m_originFirst, pnsLayer, startSnapPoint, SHAPE_LINE_CHAIN() );

        if( !startItem || !startItem->OfKind( PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T ) )
        {
            m_frame->ShowInfoBarError( _( "Phase skew initial selection failed" ), true );
            return;
        }

        PNS::NODE*     world = m_router->GetWorld()->Branch();
        PNS::TOPOLOGY  topo( world );
        PNS::DIFF_PAIR originPair;

        m_selectedStartPad = nullptr;
        m_selectedEndPad = nullptr;
        m_coupledStartPad = nullptr;
        m_coupledEndPad = nullptr;

        if( !topo.AssembleDiffPair( startItem, originPair ) )
        {
            m_frame->ShowInfoBarError( _( "Differential pair identification failed" ), true );
            return;
        }

        if( !originPair.PLine().SegmentCount() || !originPair.NLine().SegmentCount() )
            return;

        if( m_selectedNetcode == m_netcodeP )
        {
            m_selectedPath = topo.AssembleTuningPath( m_iface, originPair.PLine().GetLink( 0 ), &m_selectedStartPad,
                                                      &m_selectedEndPad );
            m_coupledPath = topo.AssembleTuningPath( m_iface, originPair.NLine().GetLink( 0 ), &m_coupledStartPad,
                                                     &m_coupledEndPad );
        }
        else
        {
            m_coupledPath = topo.AssembleTuningPath( m_iface, originPair.PLine().GetLink( 0 ), &m_coupledStartPad,
                                                     &m_coupledEndPad );
            m_selectedPath = topo.AssembleTuningPath( m_iface, originPair.NLine().GetLink( 0 ), &m_selectedStartPad,
                                                      &m_selectedEndPad );
        }
    }
    else
    {
        const int pnsLayerFirst = m_iface->GetPNSLayerFromBoardLayer( m_pickerItemFirst->GetLayer() );
        const int pnsLayerSecond = m_iface->GetPNSLayerFromBoardLayer( m_pickerItemSecond->GetLayer() );

        PCB_TRACK* trackFirst = nullptr;
        m_originFirst = PNS::HELPERS::SnapToNearestTrack( m_originFirst, m_board, nullptr, &trackFirst );
        wxCHECK( trackFirst, /* void */ );

        PCB_TRACK* trackSecond = nullptr;
        m_originSecond = PNS::HELPERS::SnapToNearestTrack( m_originSecond, m_board, nullptr, &trackSecond );
        wxCHECK( trackSecond, /* void */ );

        // Determine primary and secondary net codes
        m_selectedNetcode = trackFirst->GetNetCode();
        m_coupledNetcode = trackSecond->GetNetCode();

        // Get the netcodes
        m_selectedNetinfo = m_board->GetNetInfo().GetNetItem( m_selectedNetcode );
        m_coupledNetinfo = m_board->GetNetInfo().GetNetItem( m_coupledNetcode );

        VECTOR2I          startSnapPointFirst, startSnapPointSecond;
        PNS::LINKED_ITEM* startItemFirst = PNS::HELPERS::PickSegment( m_router, m_originFirst, pnsLayerFirst,
                                                                      startSnapPointFirst, SHAPE_LINE_CHAIN() );
        PNS::LINKED_ITEM* startItemSecond = PNS::HELPERS::PickSegment( m_router, m_originSecond, pnsLayerSecond,
                                                                       startSnapPointSecond, SHAPE_LINE_CHAIN() );

        if( !startItemFirst || !startItemFirst->OfKind( PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T ) || !startItemSecond
            || !startItemSecond->OfKind( PNS::ITEM::SEGMENT_T | PNS::ITEM::ARC_T ) )
        {
            m_frame->ShowInfoBarError( _( "Phase skew initial selection failed" ), true );
            return;
        }

        PNS::NODE*    world = m_router->GetWorld()->Branch();
        PNS::TOPOLOGY topo( world );

        m_selectedStartPad = nullptr;
        m_selectedEndPad = nullptr;
        m_coupledStartPad = nullptr;
        m_coupledEndPad = nullptr;

        m_selectedPath = topo.AssembleTuningPath( m_iface, startItemFirst, &m_selectedStartPad, &m_selectedEndPad );
        m_coupledPath = topo.AssembleTuningPath( m_iface, startItemSecond, &m_coupledStartPad, &m_coupledEndPad );
    }

    // The router can return SHAPE_LINE_CHAINS that are in a reversed order. This doesn't play nicely
    // with our parallel / antiparallel rejection tests, therfore we need to ensure the SHAPE_LINE_CHAIN
    // points are in line order here
    normalisePathItems( m_selectedPath, m_selectedStartPad );
    normalisePathItems( m_coupledPath, m_coupledStartPad );
}

void DIFF_PHASE_SKEW_TOOL::normalisePathItems( const PNS::ITEM_SET& aPath, const PNS::SOLID* aStartPad )
{
    if( !aStartPad )
        return;

    VECTOR2I pathSearchLoc = aStartPad->Pos();

    for( int i = 0; i < aPath.Size(); ++i )
    {
        PNS::ITEM* curItem = aPath[i];

        if( curItem->Kind() == PNS::ITEM::LINE_T )
        {
            PNS::LINE* lineItem = dynamic_cast<PNS::LINE*>( curItem );

            if( !lineItem )
                continue;

            SHAPE_LINE_CHAIN& line = lineItem->Line();
            const std::size_t numPoints = line.GetPointCount();
            wxASSERT( numPoints >= 2 );

            if( line.GetPoint( numPoints - 1 ) == pathSearchLoc )
                line = line.Reverse();

            pathSearchLoc = line.GetPoint( numPoints - 1 );
        }
        else if( curItem->Kind() == PNS::ITEM::VIA_T )
        {
            const PNS::VIA* viaItem = dynamic_cast<PNS::VIA*>( curItem );
            wxASSERT( viaItem->Pos() == pathSearchLoc );
        }
    }
}


DIFF_PHASE_SKEW_TOOL::DIFF_PAIR_VALIDITY DIFF_PHASE_SKEW_TOOL::determinePathDirections()
{
    // This condition can occur if the route contains items that are not tracks (e.g. unconverted arcs)
    if( !m_selectedStartPad || !m_selectedEndPad || !m_coupledStartPad || !m_coupledEndPad )
        return DIFF_PAIR_VALIDITY::NO_START_OR_END_PADS;

    PAD* selectedStartPad = static_cast<PAD*>( m_selectedStartPad->BoardItem() );
    PAD* selectedEndPad = static_cast<PAD*>( m_selectedEndPad->BoardItem() );
    PAD* coupledStartPad = static_cast<PAD*>( m_coupledStartPad->BoardItem() );
    PAD* coupledEndPad = static_cast<PAD*>( m_coupledEndPad->BoardItem() );

    if( !selectedStartPad || !selectedEndPad || !coupledStartPad || !coupledEndPad )
        return DIFF_PAIR_VALIDITY::NO_START_OR_END_PADS;

    // Normalise directions if possible
    if( selectedStartPad->GetSimElectricalType() != PAD_SIM_ELECTRICAL_TYPE::SOURCE
        && selectedEndPad->GetSimElectricalType() == PAD_SIM_ELECTRICAL_TYPE::SOURCE )
    {
        reversePath( m_selectedPath, &m_selectedStartPad, &m_selectedEndPad );
        std::swap( selectedStartPad, selectedEndPad );
    }

    if( coupledStartPad->GetSimElectricalType() != PAD_SIM_ELECTRICAL_TYPE::SOURCE
        && coupledEndPad->GetSimElectricalType() == PAD_SIM_ELECTRICAL_TYPE::SOURCE )
    {
        reversePath( m_coupledPath, &m_coupledStartPad, &m_coupledEndPad );
        std::swap( coupledStartPad, coupledEndPad );
    }

    // Both start pads must be sources
    if( selectedStartPad->GetSimElectricalType() != PAD_SIM_ELECTRICAL_TYPE::SOURCE )
    {
        return DIFF_PAIR_VALIDITY::NO_SOURCE_PADS_SELECTED_TRACK;
    }

    if( coupledStartPad->GetSimElectricalType() != PAD_SIM_ELECTRICAL_TYPE::SOURCE )
    {
        return DIFF_PAIR_VALIDITY::NO_SOURCE_PADS_COUPLED_TRACK;
    }

    // End pads can't be a source
    if( selectedEndPad->GetSimElectricalType() == PAD_SIM_ELECTRICAL_TYPE::SOURCE )
        return DIFF_PAIR_VALIDITY::MULTIPLE_SOURCES_SELECTED_TRACK;

    if( coupledEndPad->GetSimElectricalType() == PAD_SIM_ELECTRICAL_TYPE::SOURCE )
        return DIFF_PAIR_VALIDITY::MULTIPLE_SOURCES_COUPLED_TRACK;

    return DIFF_PAIR_VALIDITY::VALID;
}


void DIFF_PHASE_SKEW_TOOL::reversePath( PNS::ITEM_SET& aPath, PNS::SOLID** aStartPad, PNS::SOLID** aEndPad )
{
    PNS::ITEM_SET reversed;

    for( auto itr = aPath.rbegin(); itr != aPath.rend(); ++itr )
    {
        if( ( *itr )->Kind() == PNS::ITEM::LINE_T )
        {
            PNS::LINE* l = dyn_cast<PNS::LINE*>( *itr );
            wxASSERT( l != nullptr );

            if( l != nullptr )
                l->Reverse();
        }

        reversed.Add( *itr );
    }

    aPath = std::move( reversed );

    // Finally reverse the start / end pads
    std::swap( *aStartPad, *aEndPad );
}


void DIFF_PHASE_SKEW_TOOL::doShowStatsAtCursor()
{
    constexpr std::size_t maxIdx = std::numeric_limits<std::size_t>::max();
    const double          hitTestDistance = m_view->ToWorld( DETAILS_HOVER_HITTEST_THRESHOLD_PIXELS );
    const auto [selectedIdx, isSelectedTrack] = getNearestDiffSegments( m_cursorPos, hitTestDistance );

    if( selectedIdx < maxIdx )
    {
        if( isSelectedTrack )
            m_segmentForStatisticsDisplay = { selectedIdx, true };
        else
            m_segmentForStatisticsDisplay = { selectedIdx, false };
    }
    else
    {
        m_segmentForStatisticsDisplay = { maxIdx, false };
    }

    drawDiffOverlay();
}


std::pair<std::size_t, bool> DIFF_PHASE_SKEW_TOOL::getNearestDiffSegments( const VECTOR2D& aCursorPos,
                                                                           const double    aHitTestDistance ) const
{
    auto getNearestSegment = [&aCursorPos, aHitTestDistance]( const std::vector<OUTPUT_SEGMENT>& segments )
    {
        const double maxDistSq = aHitTestDistance * aHitTestDistance;

        std::size_t bestIndex = std::numeric_limits<std::size_t>::max();
        double      bestDistSq = maxDistSq;

        for( size_t i = 0; i < segments.size(); ++i )
        {
            const auto& seg = segments[i];

            VECTOR2D     mid = ( seg.Start + seg.End ) / 2.0;
            const double distSq = ( aCursorPos - mid ).SquaredEuclideanNorm();

            if( distSq <= bestDistSq )
            {
                bestDistSq = distSq;
                bestIndex = i;
            }
        }

        return std::pair<std::size_t, double>( bestIndex, bestDistSq );
    };

    const auto [selectedIdx, selectedDist] = getNearestSegment( m_selectedDiffs );
    const auto [coupledIdx, coupledDist] = getNearestSegment( m_coupledDiffs );

    constexpr std::size_t maxIdx = std::numeric_limits<std::size_t>::max();

    if( selectedIdx != maxIdx && coupledIdx != maxIdx )
    {
        if( selectedDist <= coupledDist )
            return { selectedIdx, true };

        return { coupledIdx, false };
    }

    if( selectedIdx != maxIdx )
        return { selectedIdx, true };

    if( coupledIdx != maxIdx )
        return { coupledIdx, false };

    return { maxIdx, true };
}


void DIFF_PHASE_SKEW_TOOL::updateNetHighlights( bool aRefresh ) const
{
    RENDER_SETTINGS* renderSettings = m_frame->GetCanvas()->GetView()->GetPainter()->GetSettings();
    renderSettings->SetHighlight( false );

    if( m_pickerItemFirst )
    {
        renderSettings->SetHighlight( true, m_netcodeP, true );

        if( m_pickerItemFirstIsDiffPair )
            renderSettings->SetHighlight( true, m_netcodeN, true );
    }

    if( m_pickerItemSecond )
        renderSettings->SetHighlight( true, m_netcodeN, true );

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();

    if( aRefresh )
        m_frame->GetCanvas()->Refresh();
}


void DIFF_PHASE_SKEW_TOOL::setTransitions()
{
    // clang-format off
    Go( &DIFF_PHASE_SKEW_TOOL::ShowDiffPhaseSkew,PCB_ACTIONS::showDiffPhaseSkew.MakeEvent() );
    // clang-format on
}


void DIFF_PHASE_SKEW_TOOL::getOverlay()
{
    if( !m_viewOverlay )
    {
        m_viewOverlay = m_view->MakeOverlay();
        m_view->Add( m_viewOverlay.get() );
    }
}


void DIFF_PHASE_SKEW_TOOL::clearOverlay() const
{
    if( m_viewOverlay )
    {
        m_viewOverlay->Clear();
        updateOverlay();
    }
}


void DIFF_PHASE_SKEW_TOOL::updateOverlay() const
{
    if( m_viewOverlay )
    {
        m_view->Update( m_viewOverlay.get() );
    }
}


void DIFF_PHASE_SKEW_TOOL::resetStateVariables()
{
    m_pickerItemFirst = nullptr;
    m_pickerItemSecond = nullptr;
    m_pickerItemFirstIsDiffPair = false;
    m_netcodeP = 0;
    m_netcodeN = 0;
    m_originFirst = { 0, 0 };
    m_originSecond = { 0, 0 };
    m_timeDomain = false;
    m_selectedNetcode = 0;
    m_coupledNetcode = 0;
    m_selectedNetinfo = nullptr;
    m_coupledNetinfo = nullptr;
    m_selectedPath.Clear();
    m_coupledPath.Clear();
    m_selectedStartPad = nullptr;
    m_selectedEndPad = nullptr;
    m_coupledStartPad = nullptr;
    m_coupledEndPad = nullptr;
    m_selectedLengthDelayItems.clear();
    m_coupledLengthDelayItems.clear();
    m_selectedLengthDelayDetails.LengthsAndDelays.clear();
    m_coupledLengthDelayDetails.LengthsAndDelays.clear();
    m_selectedDiffs.clear();
    m_coupledDiffs.clear();
    m_segmentForStatisticsDisplay = { std::numeric_limits<std::size_t>::max(), false };
    m_maxSkew.reset();
}
