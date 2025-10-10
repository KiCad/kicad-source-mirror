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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PCBNEW_LENGTH_DELAY_CALCULATION_H
#define PCBNEW_LENGTH_DELAY_CALCULATION_H

#include "tuning_profile_parameters_iface.h"
#include "tuning_profile_parameters_user_defined.h"

#include <board_design_settings.h>
#include <connectivity/connectivity_data.h>
#include <length_delay_calculation/length_delay_calculation_item.h>
#include <unordered_set>

class BOARD;

/**
* Holds length measurement result details and statistics
*/
struct LENGTH_DELAY_STATS
{
    // Generic statistics
    int NumPads{ 0 };
    int NumVias{ 0 };

    // Space domain statistics
    int                                              ViaLength{ 0 };
    int64_t                                          TrackLength{ 0 };
    int                                              PadToDieLength{ 0 };
    std::unique_ptr<std::map<PCB_LAYER_ID, int64_t>> LayerLengths;

    /// Calculates the total electrical length for this set of statistics
    int64_t TotalLength() const { return ViaLength + TrackLength + PadToDieLength; }

    // Time domain statistics
    int64_t                                          ViaDelay{ 0 };
    int64_t                                          TrackDelay{ 0 };
    int64_t                                          PadToDieDelay{ 0 };
    std::unique_ptr<std::map<PCB_LAYER_ID, int64_t>> LayerDelays;

    /// Calculates the total electrical propagation delay for this set of statistics
    int64_t TotalDelay() const { return ViaDelay + TrackDelay + PadToDieDelay; }
};


/**
 * Struct to control which optimisations the length calculation code runs on
 * the given path objects. This is required as some call sites (e.g. PNS) run
 * their own path optimisation, whereas others (e.g. Net Inspector) do not.
 */
struct PATH_OPTIMISATIONS
{
    /// Optimise via layers for height calculations, ensuring only the distance
    /// between routed segments is considered
    bool OptimiseViaLayers = false;

    /// Merges all contiguous (end-to-end, same layer) tracks
    bool MergeTracks = false;

    /// Optimises the electrical length of tracks within pads. Note that the track
    /// must terminate at the trace anchor point to be considered for
    /// optimisation. Will require MergeTracks if used with a non-contiguous item
    /// set.
    bool OptimiseTracesInPads = false;

    /// Determines if there is a via-in-pad present on the board but not in the
    /// item set. This condition can arise from the PNS meander placer.
    /// TODO (JJ): This can be fixed in the router
    bool InferViaInPad = false;
};


/**
* Enum which controls the level of detail returned by the length / delay calculation methods.
*
* NO_LAYER_DETAIL: Only returns totals, without populating the layer length / delay detail maps
* WITH_LAYER_DETAIL: Populates the layer lengths / delay detail maps
*/
enum class LENGTH_DELAY_LAYER_OPT
{
    NO_LAYER_DETAIL,
    WITH_LAYER_DETAIL
};


/**
* Enum which controls the calculation domain of the length / delay calculation methods.
*
* NO_DELAY_DETAIL: Only calculates space domain (e.g. length) details
* WITH_DELAY_DETAIL: Calculates space and time domain (e.g. length and delay) details
*/
enum class LENGTH_DELAY_DOMAIN_OPT
{
    NO_DELAY_DETAIL,
    WITH_DELAY_DETAIL
};


/**
* Class which calculates lengths (and associated routing statistics) in a BOARD context
*/
class LENGTH_DELAY_CALCULATION
{
public:
    /**
     * Construct the calculator in the given BOARD context. Also constructs a default user-defined time domain
     * parameters provider
     */
    explicit LENGTH_DELAY_CALCULATION( BOARD* aBoard ) :
            m_board( aBoard ),
            m_tuningProfileParameters( std::make_unique<TUNING_PROFILE_PARAMETERS_USER_DEFINED>( aBoard, this ) )
    {
    }

    /**
     * @brief Calculates the electrical length of the given items
     *
     * @param aItems is the vector of items making up the route
     * @param aOptimisations details the electrical path optimisations that should be applied to the board items
     * @param aStartPad is the starting pad of the route
     * @param aEndPad is the ending pad of the route
     */
    int64_t CalculateLength( std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems, PATH_OPTIMISATIONS aOptimisations,
                             const PAD* aStartPad = nullptr, const PAD* aEndPad = nullptr ) const;

    /**
     * @brief Calculates the electrical propagation delay of the given items
     *
     * @param aItems is the vector of items making up the route
     * @param aOptimisations details the electrical path optimisations that should be applied to the board items
     * @param aStartPad is the starting pad of the route
     * @param aEndPad is the ending pad of the route
     */
    int64_t CalculateDelay( std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems, PATH_OPTIMISATIONS aOptimisations,
                            const PAD* aStartPad = nullptr, const PAD* aEndPad = nullptr ) const;

    /**
     * @brief Calculates the electrical length of the given items
     *
     * @param aItems is the vector of items making up the route
     * @param aOptimisations details the electrical path optimisations that should be applied to the board items
     * @param aStartPad is the starting pad of the route
     * @param aEndPad is the ending pad of the route
     * @param aLayerOpt determines whether the layer details map is populated
     * @param aDomain determines whether calculations include time domain (delay) details
     */
    LENGTH_DELAY_STATS
    CalculateLengthDetails( std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems, PATH_OPTIMISATIONS aOptimisations,
                            const PAD* aStartPad = nullptr, const PAD* aEndPad = nullptr,
                            LENGTH_DELAY_LAYER_OPT  aLayerOpt = LENGTH_DELAY_LAYER_OPT::NO_LAYER_DETAIL,
                            LENGTH_DELAY_DOMAIN_OPT aDomain = LENGTH_DELAY_DOMAIN_OPT::NO_DELAY_DETAIL ) const;

    /**
  * Gets the propagation delay for the given shape line chain
  *
  * @param aShape is the shape to calculate delay for
  * @param aCtx is the geometry context for which to query to propagation delay
  */
    int64_t CalculatePropagationDelayForShapeLineChain( const SHAPE_LINE_CHAIN&                aShape,
                                                        const TUNING_PROFILE_GEOMETRY_CONTEXT& aCtx ) const;

    /**
     * @brief Calculates the length of track required for the given delay in a specific geometry context
     *
     * @param aDesiredDelay is the desired track delay (in IU)
     * @param aCtx is the track geometry context to calculate propagation velocitiy against
     */
    int64_t CalculateLengthForDelay( int64_t aDesiredDelay, const TUNING_PROFILE_GEOMETRY_CONTEXT& aCtx ) const;

    /// Optimises the given trace / line to minimise the electrical path length within the given pad
    static void OptimiseTraceInPad( SHAPE_LINE_CHAIN& aLine, const PAD* aPad, PCB_LAYER_ID aPcbLayer );

    /// Return a LENGTH_CALCULATION_ITEM constructed from the given BOARD_CONNECTED_ITEM
    LENGTH_DELAY_CALCULATION_ITEM GetLengthCalculationItem( const BOARD_CONNECTED_ITEM* aBoardItem ) const;

    /// Sets the provider for tuning profile parameter resolution
    void SetTuningProfileParametersProvider( std::unique_ptr<TUNING_PROFILE_PARAMETERS_IFACE>&& aProvider );

    /// Ensure time domain properties provider is synced with board / project settings if required
    void SynchronizeTuningProfileProperties() const;

    /**
     * Returns the stackup distance between the two given layers.
     *
     * Note: Can return 0 if the board design settings disallow stackup height calculations
     */
    int StackupHeight( PCB_LAYER_ID aFirstLayer, PCB_LAYER_ID aSecondLayer ) const;

protected:
    /// The parent board for all items
    BOARD* m_board;

    /// The active provider of tuning profile parameters
    std::unique_ptr<TUNING_PROFILE_PARAMETERS_IFACE> m_tuningProfileParameters;

    /// Enum to describe whether track merging is attempted from the start or end of a track segment
    enum class MERGE_POINT
    {
        START,
        END
    };

    /**
     * Optimises the given set of items to minimise the electrical path length. At the moment
     * only optimises lines attached to pads, future work could optimise paths through pads
     *
     * Assumes that any polylines are only connected at either end, and not at midpoints
     */
    static void optimiseTracesInPads( const std::vector<LENGTH_DELAY_CALCULATION_ITEM*>& aPads,
                                      const std::vector<LENGTH_DELAY_CALCULATION_ITEM*>& aLines );

    /// Clips the given line to the minimal direct electrical length within the pad
    static void clipLineToPad( SHAPE_LINE_CHAIN& aLine, const PAD* aPad, PCB_LAYER_ID aLayer, bool aForward = true );

    /**
     * Optimises the via layers. Ensures that vias that are routed through only on one layer do not count towards total
     * length calculations.
     */
    static void
    optimiseViaLayers( const std::vector<LENGTH_DELAY_CALCULATION_ITEM*>&                            aVias,
                       std::vector<LENGTH_DELAY_CALCULATION_ITEM*>&                                  aLines,
                       std::map<VECTOR2I, std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*>>&       aLinesPositionMap,
                       const std::map<VECTOR2I, std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*>>& aPadsPositionMap );

    /**
     * Merges any lines (traces) that are contiguous, on one layer, and with no junctions
     */
    static void mergeLines( std::vector<LENGTH_DELAY_CALCULATION_ITEM*>&                            aLines,
                            std::map<VECTOR2I, std::unordered_set<LENGTH_DELAY_CALCULATION_ITEM*>>& aLinesPositionMap );

    /**
     * Merges two SHAPE_LINE_CHAINs where there is a shared endpoing.
     *
     * aSecondary is merged in to aPrimary
     */
    static void mergeShapeLineChains( SHAPE_LINE_CHAIN& aPrimary, const SHAPE_LINE_CHAIN& aSecondary,
                                      MERGE_POINT aMergePoint );

    /**
     * Infers if there is a via in the given pad. Adds via details to the length details data structure if found.
     */
    void inferViaInPad( const PAD* aPad, const LENGTH_DELAY_CALCULATION_ITEM& aItem,
                        LENGTH_DELAY_STATS& aDetails ) const;
};

#endif //PCBNEW_LENGTH_DELAY_CALCULATION_H
