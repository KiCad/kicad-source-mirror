/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef DIFF_PHASE_SKEW_TOOL_H
#define DIFF_PHASE_SKEW_TOOL_H

#include <length_delay_calculation/length_delay_calculation.h>
#include <length_delay_calculation/length_delay_calculation_item.h>

#include <router/pns_itemset.h>
#include <router/pns_sizes_settings.h>
#include <router/pns_solid.h>

#include <drc/drc_engine.h>
#include <tools/pcb_tool_base.h>

#include <optional>
#include <vector>

class TOOL_EVENT;
class BOARD;
class PCB_BASE_EDIT_FRAME;

namespace PNS
{
class ROUTER;
}

class PNS_KICAD_IFACE;


/**
 * Struct to represent one segment where tracks run parallel, including information about
 * absolute and relative length and phase
 */
struct PARALLEL_RUN
{
    /// The starting coordinate of the run on track A
    VECTOR2I startA;

    /// The ending coordinate of the run on track A
    VECTOR2I endA;

    /// The starting coordinate of the run on track B
    VECTOR2I startB;

    /// The ending coordinate of the run on track B
    VECTOR2I endB;

    /// The index of the parallel segment on track A
    size_t segA;

    /// The index of the parallel segment on track B
    size_t segB;

    /// Normalised values of the start (0) and end (1) coordinates on track A and B
    /// These are normalised to the range 0 = start, 1 = end
    double ta0, ta1, tb0, tb1;

    /// Cumulative length of track A at the start of the parallel run
    double startLenA;

    /// Cumulative length of track A at the end of the parallel run
    double endLenA;

    /// Cumulative length of track B at the start of the parallel run
    double startLenB;

    /// Cumulative length of track B at the end of the parallel run
    double endLenB;

    /// Cumulative delay of track A at the start of the parallel run
    double startDelayA;

    /// Cumulative delay of track A at the start of the parallel run
    double endDelayA;

    /// Cumulative delay of track A at the start of the parallel run
    double startDelayB;

    /// Cumulative delay of track A at the start of the parallel run
    double endDelayB;

    std::optional<double> startViaLengthA;
    std::optional<double> endViaLengthA;
    std::optional<double> startViaLengthB;
    std::optional<double> endViaLengthB;
    std::optional<double> startViaDelayA;
    std::optional<double> endViaDelayA;
    std::optional<double> startViaDelayB;
    std::optional<double> endViaDelayB;

    /// Calculate the fraction of track A that overlaps track B
    double overlapFractionA() const { return std::abs( ta1 - ta0 ); }

    /// Calculate the fraction of track B that overlaps track A
    double overlapFractionB() const { return std::abs( tb1 - tb0 ); }

    /// Calculates the {start, end} track length delta relative to the selected
    /// track. The delta relative to the couple track is the negation of these
    /// values.
    std::pair<double, double> pathLengthDelta() const { return { startLenB - startLenA, endLenB - endLenA }; }

    /// Calculates the {start, end} track delay delta relative to the selected
    /// track. The delta relative to the couple track is the negation of these
    /// values.
    std::pair<double, double> pathDelayDelta() const { return { startDelayB - startDelayA, endDelayB - endDelayA }; }
};


/**
 * Represents a point on a track where the length and delay relative to the coupled track can
 * be calculated
 */
struct KNOWN_RELATIVE_POINT
{
    /// The distance (in IU) along the track
    double LinearDistance;

    /// The length relative to the coupled track
    double RelativeLengthBefore;

    /// The propagation delay relative to the coupled track
    double RelativeDelayBefore;

    /// The length relative to the coupled track
    double RelativeLengthAfter;

    /// The propagation delay relative to the coupled track
    double RelativeDelayAfter;
};


/// Floating point comparison epsilon
constexpr double EPS = 1e-9;


/**
 * Interpolates known relative points along a track using linear distance. It is optimised to only support
 * calling ValueAt with monotonically increasing values of s.
 */
class KnownValueInterpolator
{
public:
    explicit KnownValueInterpolator( const std::vector<KNOWN_RELATIVE_POINT>& pts ) :
            m_pts( pts )
    {
    }

    std::optional<std::pair<double, double>> ValueAt( const double s )
    {
        if( m_pts.empty() )
            return std::nullopt;

        // Unknown before first known point
        if( s < m_pts.front().LinearDistance - EPS )
            return std::nullopt;

        // Unknown after last known point
        if( s > m_pts.back().LinearDistance + EPS )
            return std::nullopt;

        // Find known value point window
        while( ( m_idx + 1 ) < m_pts.size() && s > m_pts[m_idx + 1].LinearDistance + EPS )
        {
            ++m_idx;
        }

        // Exact last point
        if( m_idx + 1 >= m_pts.size() )
        {
            const auto& p = m_pts.back();
            return std::pair{ p.RelativeLengthBefore, p.RelativeDelayBefore };
        }

        // Interpolate between known points
        const auto& a = m_pts[m_idx];
        const auto& b = m_pts[m_idx + 1];

        const double len = b.LinearDistance - a.LinearDistance;

        // Degenerate interval
        if( len <= EPS )
        {
            return std::pair{ a.RelativeLengthBefore, a.RelativeDelayBefore };
        }

        const double t = ( s - a.LinearDistance ) / len;
        const double relativeLength = a.RelativeLengthAfter + t * ( b.RelativeLengthBefore - a.RelativeLengthAfter );
        const double relativeDelay = a.RelativeDelayAfter + t * ( b.RelativeDelayBefore - a.RelativeDelayAfter );

        return std::pair{ relativeLength, relativeDelay };
    }

private:
    const std::vector<KNOWN_RELATIVE_POINT>& m_pts;
    std::size_t                              m_idx{ 0 };
};


/**
 * Tool responsible for displaying an overlay of incremental phase difference between differential pair tracks
 */
class DIFF_PHASE_SKEW_TOOL : public PCB_TOOL_BASE
{
public:
    DIFF_PHASE_SKEW_TOOL();
    ~DIFF_PHASE_SKEW_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    ///< The possible states of DRAWING_TOOL
    enum class MODE
    {
        // Initial hover state to identify diff pair / first track
        HOVER,

        // First track was selected and wasn't a diff pair
        SELECTED_FIRST,

        // Both tracks selected
        FIXED
    };

    ///< Return the current mode of the tool
    MODE GetMode() const { return m_mode; }

    ///< Set the current mode of the tool
    void SetMode( const MODE aMode ) { m_mode = aMode; }

    ///< The tool entry point
    int ShowDiffPhaseSkew( const TOOL_EVENT& aEvent );

    ///< Flags for the analysis state of the selected tracks
    enum class DIFF_PAIR_VALIDITY : int
    {
        VALID = 0,
        NO_START_OR_END_PADS,
        NO_SOURCE_PADS_SELECTED_TRACK,
        NO_SOURCE_PADS_COUPLED_TRACK,
        MULTIPLE_SOURCES_SELECTED_TRACK,
        MULTIPLE_SOURCES_COUPLED_TRACK
    };

protected:
    ///< Set up handlers for tool events
    void setTransitions() override;

    ///< Highlight nets when in MODE::HOVER and we have active nets in scope
    void updateNetHighlights( bool aRefresh = true ) const;

    ///< Handle hover events before a DP pair is selected
    void doInitialHover( const PCB_SELECTION_TOOL* aSelectionTool, GENERAL_COLLECTORS_GUIDE aGuide );

    ///< Display the phase overlay for the current hover item
    void doDisplayOverlay();

    ///< Use the router to get the +ve and -ve paths from the selected item
    void getNetPaths();

    ///< Normalises the path to ensure SHAPE_LINE_CHAIN points are in overall path walk order
    static void normalisePathItems( const PNS::ITEM_SET& aPath, const PNS::SOLID* aStartPad );

    ///< Determine which end of the extracted paths we are defining as the signal start point
    DIFF_PAIR_VALIDITY determinePathDirections();

    ///< Reverses the direction of the selected and coupled paths (including swapping
    ///< start / end pads
    static void reversePath( PNS::ITEM_SET& aPath, PNS::SOLID** aStartPad, PNS::SOLID** aEndPad );

    ///< Report to the user any errors after determining the signal direction
    ///< @returns true if any errors reported, otherwise false
    bool reportValidityErrors( DIFF_PAIR_VALIDITY aDirection ) const;

    ///< Struct to represent one cumulative length and delay point
    struct CUMULATIVE_ENTRY
    {
        int64_t                             m_Length{ 0 };
        int64_t                             m_Delay{ 0 };
        LENGTH_DELAY_CALCULATION_ITEM::TYPE m_SourceType{ LENGTH_DELAY_CALCULATION_ITEM::TYPE::UNKNOWN };
        VECTOR2I                            m_Start{ 0, 0 };
        VECTOR2I                            m_End{ 0, 0 };
    };

    ///< Builds the length / delay calculation items from a given path
    void buildLengthDelayItems( const PNS::ITEM_SET& aPath, const PNS::SOLID* aStartPad, const PNS::SOLID* aEndPad,
                                const NETINFO_ITEM* aNet, std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems,
                                LENGTH_DELAY_ITEM_DETAILS& aItemDetails ) const;

    ///< Start and end pad lengths and delays (pad-to-die + inferred via-in-pad)
    struct START_END_DETAILS
    {
        int64_t StartPadLength{ 0 };
        int64_t EndPadLength{ 0 };
        int64_t StartPadDelay{ 0 };
        int64_t EndPadDelay{ 0 };
    };

    ///< Builds a vector in which each entry represents the cumulative length and delay at the start
    ///< of a given segment. The first and last segments are virtual to allow the total length to be stored.
    ///< Total size of the returned vectors is (numSegments + 2).
    static std::vector<CUMULATIVE_ENTRY>
    buildCumulativeLengthsAndDelays( const std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems,
                                     const LENGTH_DELAY_ITEM_DETAILS& aLengthDelayDetails, const PNS::SOLID* aStartPad,
                                     const PNS::SOLID* aEndPad, START_END_DETAILS& aStartEndDetails );

    ///< Splits the calculation items from compound segments in to individual items
    static void splitLengthItems( std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems );

    ///< Finds all parallel segment runs in the selected and coupled tracks
    std::vector<PARALLEL_RUN> findParallelRuns() const;

    ///< Finds all parallel segment runs in the selected and coupled tracks within the given segment ranges and
    ///< track spacing
    void findParallelRunsImpl( std::pair<std::size_t, std::size_t> aRangeA, std::pair<std::size_t, std::size_t> aRangeB,
                               double aMaxSpacing, std::vector<PARALLEL_RUN>& aRuns ) const;

    ///< Linear interpolate from point A to B at line fraction T
    static VECTOR2D lerp( const VECTOR2D aA, const VECTOR2D aB, const double aT )
    {
        return { aA.x + ( aB.x - aA.x ) * aT, aA.y + ( aB.y - aA.y ) * aT };
    }

    ///< Gets the cumulative length and delay at the given fractional coordinate in the given segment
    static std::pair<int64_t, int64_t>
    getCumulativeLengthAndDelayAt( const LENGTH_DELAY_ITEM_DETAILS&     aLengthDelayDetails,
                                   const START_END_DETAILS&             aPadDetails,
                                   const std::vector<CUMULATIVE_ENTRY>& aCumulative, std::size_t aSegIdx, double aT );

    ///< Gets the maximum diff pair gap for the given item, taken from DRC rules
    int getMaxDiffPairGap( const BOARD_CONNECTED_ITEM* aItem ) const;

    ///< Struct containing a final computed output diff segment
    struct OUTPUT_SEGMENT
    {
        /// The start point of the segment
        VECTOR2D Start;

        /// The end point of the segment
        VECTOR2D End;

        /// The segment colour
        COLOR4D Colour;

        /// The segment width
        int Width;

        /// Flag whether the diff value is valid at this segment
        bool RelativeValueKnown;

        /// The value of the diff at the beginning of this segment
        double RelativeValueAtMid;
    };

    ///< Builds the final overlay output segments for plotting
    void buildDiffOverlaySegments( double aTargetSubsegmentSize );

    std::vector<OUTPUT_SEGMENT>
    buildDiffOverlaySegmentsImpl( const std::vector<CUMULATIVE_ENTRY>&              aSegments,
                                  const std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aSourceItemDetails,
                                  const std::vector<KNOWN_RELATIVE_POINT>& aKnownPoints, double aTargetSubsegmentSize );

    ///< Builds a vector of known relative skew points on each track
    void buildKnownRelativePoints( const std::vector<PARALLEL_RUN>& aKnownRuns );

    ///< Determines where to apply overlay segment subsections on the source segments
    static std::vector<double> buildSplitPositions( const std::vector<CUMULATIVE_ENTRY>& aSegments,
                                                    double                               aTargetSubsegmentSize );

    ///< Returns the coordinate at the given linear distance along the line, along with the segment
    ///< index the point belongs to
    static std::pair<VECTOR2D, std::size_t>
    pointAtDistance( const std::vector<CUMULATIVE_ENTRY>&              aSegments,
                     const std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aSourceItemDetails, double aDist );

    ///< Linearly interpolates between colour1 and colour2, with interpolation point given by
    ///< aS [0-1]
    COLOR4D interpolateColours( const COLOR4D& aColour1, const COLOR4D& aColour2, double aS, bool aUseLogScale ) const;

    ///< Draws the visual skew overlay
    void drawDiffOverlay() const;

    ///< Shows the diff stats nearest the cursor
    void doShowStatsAtCursor();

    ///< Determines the nearest points to the cursor from the diff segments
    std::pair<std::size_t, bool> getNearestDiffSegments( const VECTOR2D& aCursorPos, double aHitTestDistance ) const;

    ///< Ensures we have an active VIEW_OVERLAY to display the diff graphics
    void getOverlay();

    ///< Refreshes the VIEW_OVERLAY in the active VIEW
    void updateOverlay() const;

    ///< Clears the VIEW_OVERLAY
    void clearOverlay() const;

    ///< Resets all select-specific variables
    void resetStateVariables();

    ///< Updates the message panel
    void updateMessagePanel() const;

private:
    KIGFX::VIEW*                         m_view{ nullptr };
    std::shared_ptr<KIGFX::VIEW_OVERLAY> m_viewOverlay{ nullptr };
    KIGFX::VIEW_CONTROLS*                m_controls{ nullptr };
    BOARD*                               m_board{ nullptr };
    PCB_BASE_EDIT_FRAME*                 m_frame{ nullptr };
    std::shared_ptr<DRC_ENGINE>          m_drcEngine{ nullptr };
    MODE                                 m_mode{ MODE::HOVER };
    bool                                 m_inDiffPhaseSkewTool{ false }; // Re-entrancy guard
    VECTOR2D                             m_cursorPos;

    // Data for the candidate track selection
    BOARD_CONNECTED_ITEM* m_pickerItemFirst{ nullptr };
    BOARD_CONNECTED_ITEM* m_pickerItemSecond{ nullptr };
    bool                  m_pickerItemFirstIsDiffPair{ false };
    int                   m_netcodeP{ 0 };
    int                   m_netcodeN{ 0 };
    VECTOR2I              m_originFirst;
    VECTOR2I              m_originSecond;
    bool                  m_timeDomain{ false };

    // Router for path extraction
    PNS::ROUTER*        m_router{ nullptr };
    PNS_KICAD_IFACE*    m_iface{ nullptr };
    PNS::SIZES_SETTINGS m_savedSizes; // Stores sizes settings between router invocations

    // Extracted diff net paths
    int           m_selectedNetcode{ 0 };
    int           m_coupledNetcode{ 0 };
    NETINFO_ITEM* m_selectedNetinfo{ nullptr };
    NETINFO_ITEM* m_coupledNetinfo{ nullptr };
    PNS::ITEM_SET m_selectedPath;
    PNS::ITEM_SET m_coupledPath;

    // Extracted diff net start / end pads
    PNS::SOLID* m_selectedStartPad{ nullptr };
    PNS::SOLID* m_selectedEndPad{ nullptr };
    PNS::SOLID* m_coupledStartPad{ nullptr };
    PNS::SOLID* m_coupledEndPad{ nullptr };

    // Length and delay items from extracted paths
    std::vector<LENGTH_DELAY_CALCULATION_ITEM> m_selectedLengthDelayItems;
    std::vector<LENGTH_DELAY_CALCULATION_ITEM> m_coupledLengthDelayItems;
    LENGTH_DELAY_ITEM_DETAILS                  m_selectedLengthDelayDetails;
    LENGTH_DELAY_ITEM_DETAILS                  m_coupledLengthDelayDetails;

    // Length and delay details for start and end pads
    START_END_DETAILS m_selectedStartEndDetails;
    START_END_DETAILS m_coupledStartEndDetails;

    // Cumulative length and delay details for each track
    std::vector<CUMULATIVE_ENTRY> m_selectedCumulative;
    std::vector<CUMULATIVE_ENTRY> m_coupledCumulative;

    // The known relative delay points for each track
    std::vector<KNOWN_RELATIVE_POINT> m_selectedKnownPoints;
    std::vector<KNOWN_RELATIVE_POINT> m_coupledKnownPoints;

    // The final calculated path diffs
    std::vector<OUTPUT_SEGMENT> m_selectedDiffs;
    std::vector<OUTPUT_SEGMENT> m_coupledDiffs;

    // Flags for interactive display of diff values
    std::optional<int>           m_maxSkew;
    std::pair<std::size_t, bool> m_segmentForStatisticsDisplay{ std::numeric_limits<std::size_t>::max(), false };

    // Configuration parameters. These are taken from the advanced config to ensure we can
    // tweak them during initial user usage.
    double m_overlayTrackInflation{ 1.1 };
    double m_trackGapInflation{ 1.2 };
    double m_cosThetaParallelTestValue{ 0.9999 };
    double m_colourInterpolationLogStrength{ 9.0 };
    double m_targetDiffSegmentSize{ 50000.0 };
};

#endif /* DIFF_PHASE_SKEW_TOOL_H */
