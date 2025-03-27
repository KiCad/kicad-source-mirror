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

#ifndef PCBNEW_LENGTH_CALCULATION_H
#define PCBNEW_LENGTH_CALCULATION_H

#include <board_design_settings.h>
#include <geometry/shape_line_chain.h>
#include <pad.h>
#include <pcb_track.h>
#include <layer_ids.h>
#include <unordered_set>
#include <connectivity/connectivity_data.h>

class BOARD;

/**
 * Lightweight class which holds a pad, via, or a routed trace outline.  Proxied objects passed by pointer are not
 * owned by this container.
 */
class LENGTH_CALCULATION_ITEM
{
public:
    /// The type of routing object this item proxies
    enum class TYPE
    {
        UNKNOWN,
        PAD,
        LINE,
        VIA
    };

    /// Whether this item is UNMERGED, it has been merged and should be used (MERGED_IN_USE), or it has been merged
    /// and has been retired from use (MERGED_RETIRED). MERGED_RETIRED essentially means the object has been merged
    /// in to a MERGED_IN_USE item.
    enum class MERGE_STATUS
    {
        UNMERGED,
        MERGED_IN_USE,
        MERGED_RETIRED
    };

    /// Gets the routing item type
    TYPE Type() const { return m_type; };

    /// Sets the parent PAD associated with this item
    void SetPad( PAD* aPad )
    {
        m_type = TYPE::PAD;
        m_pad = aPad;
    }

    /// Gets the parent PAD associated with this item
    PAD* GetPad() const { return m_pad; }

    /// Sets the source SHAPE_LINE_CHAIN of this item
    void SetLine( const SHAPE_LINE_CHAIN& aLine )
    {
        m_type = TYPE::LINE;
        m_line = aLine;
    }

    /// Gets the SHAPE_LINE_CHAIN associated with this item
    SHAPE_LINE_CHAIN& GetLine() const { return m_line; }

    /// Sets the VIA associated with this item
    void              SetVia( PCB_VIA* aVia )
    {
        m_type = TYPE::VIA;
        m_via = aVia;
    }

    /// Gets the VIA associated with this item
    PCB_VIA* GetVia() const { return m_via; }

    /// Sets the first and last layers associated with this item
    void     SetLayers( const PCB_LAYER_ID aStart, const PCB_LAYER_ID aEnd = PCB_LAYER_ID::UNDEFINED_LAYER )
    {
        m_layerStart = aStart;
        m_layerEnd = aEnd;
    }

    /// Sets the MERGE_STATUS of this item. MERGED_RETIRED essentially means the object has been merged
    /// in to a MERGED_IN_USE item.
    void SetMergeStatus( const MERGE_STATUS aStatus ) { m_mergeStatus = aStatus; }

    /// Gets the MERGE_STATUS of this item
    MERGE_STATUS GetMergeStatus() const { return m_mergeStatus; }

    /// Gets the upper and lower layers for the proxied item
    std::tuple<PCB_LAYER_ID, PCB_LAYER_ID> GetLayers() const { return { m_layerStart, m_layerEnd }; }

    /// Gets the start board layer for the proxied item
    PCB_LAYER_ID GetStartLayer() const { return m_layerStart; }

    /// Gets the end board layer for the proxied item.
    PCB_LAYER_ID GetEndLayer() const { return m_layerEnd; }

    /// Calculates active via payers for a proxied VIA object
    void CalculateViaLayers( const BOARD* aBoard );

protected:
    /// A proxied PAD object. Set to nullptr if not proxying a PAD.
    PAD*                     m_pad{ nullptr };

    /// A proxied SHAPE_LINE_CHAIN object. Line is empty if not proxying a SHAPE_LINE_CHAIN.
    mutable SHAPE_LINE_CHAIN m_line;

    /// A proxied PVIAAD object. Set to nullptr if not proxying a VIA.
    PCB_VIA*                 m_via{ nullptr };

    /// The start board layer for the proxied object
    PCB_LAYER_ID             m_layerStart{ PCB_LAYER_ID::UNDEFINED_LAYER };

    /// The end board layer for the proxied object
    PCB_LAYER_ID             m_layerEnd{ PCB_LAYER_ID::UNDEFINED_LAYER };

    /// Flags whether this item has already been merged with another
    MERGE_STATUS m_mergeStatus{ MERGE_STATUS::UNMERGED };

    /// The routing object type of the proxied parent
    TYPE m_type{ TYPE::UNKNOWN };
};


/**
* Holds length measurement result details and statistics
*/
struct LENGTH_DETAILS
{
    int                                              NumPads{ 0 };
    int                                              NumVias{ 0 };
    int                                              ViaLength{ 0 };
    int64_t                                          TrackLength{ 0 };
    int                                              PadToDieLength{ 0 };
    std::unique_ptr<std::map<PCB_LAYER_ID, int64_t>> LayerLengths;

    int64_t TotalLength() const { return ViaLength + TrackLength + PadToDieLength; }
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
* Class which calculates lengths (and associated routing statistics) in a BOARD context
*/
class LENGTH_CALCULATION
{
public:
    /// Construct the calculator in the given BOARD context
    explicit LENGTH_CALCULATION( BOARD* aBoard ) : m_board( aBoard ) {}

    /**
     * @brief Calculates the electrical length of the given items
     * @param aItems is the vector of items making up the route
     * @param aPathType indicates whether this is an ordered route, or an unordered collection
     * @param aOptimised indicates whether this has been optimised for electrical length (e.g. clipping within pads)
     * @param aStartPad is the starting pad of the route
     * @param aEndPad is the ending pad of the route
     */
    int64_t CalculateLength( std::vector<LENGTH_CALCULATION_ITEM>& aItems, PATH_OPTIMISATIONS aOptimisations,
                             const PAD* aStartPad = nullptr, const PAD* aEndPad = nullptr ) const;

    /**
     * @brief Calculates the electrical length of the given items
     * @param aItems is the vector of items making up the route
     * @param aPathType indicates whether this is an ordered route, or an unordered collection
     * @param aOptimised indicates whether this has been optimised for electrical length (e.g. clipping within pads)
     * @param aStartPad is the starting pad of the route
     * @param aEndPad is the ending pad of the route
     * @param aWithLayerLengths indicates whether the layer length structure should be populated
     */
    LENGTH_DETAILS CalculateLengthDetails( std::vector<LENGTH_CALCULATION_ITEM>& aItems,
                                           PATH_OPTIMISATIONS aOptimisations, const PAD* aStartPad = nullptr,
                                           const PAD* aEndPad = nullptr, bool aWithLayerLengths = false ) const;

    /// Optimises the given trace / line to minimise the electrical path length within the given pad
    static void OptimiseTraceInPad( SHAPE_LINE_CHAIN& aLine, const PAD* aPad, PCB_LAYER_ID aPcbLayer );

    /// Return a LENGTH_CALCULATION_ITEM constructed from the given BOARD_CONNECTED_ITEM
    LENGTH_CALCULATION_ITEM GetLengthCalculationItem( BOARD_CONNECTED_ITEM* aBoardItem ) const;

protected:
    /// The parent board for all items
    BOARD* m_board;

    /// Enum to describe whether track merging is attempted from the start or end of a track segment
    enum class MERGE_POINT
    {
        START,
        END
    };

    /**
     * Returns the stackup distance between the two given layers.
     *
     * Note: Can return 0 if the board design settings disallow stackup height calculations
     */
    int stackupHeight( PCB_LAYER_ID aFirstLayer, PCB_LAYER_ID aSecondLayer ) const;

    /**
     * Optimises the given set of items to minimise the electrical path length. At the moment
     * only optimises lines attached to pads, future work could optimise paths through pads
     *
     * Assumes that any polylines are only connected at either end, and not at midpoints
     */
    static void optimiseTracesInPads( const std::vector<LENGTH_CALCULATION_ITEM*>& aPads,
                                      const std::vector<LENGTH_CALCULATION_ITEM*>& aLines );

    /// Clips the given line to the minimal direct electrical length within the pad
    static void clipLineToPad( SHAPE_LINE_CHAIN& aLine, const PAD* aPad, PCB_LAYER_ID aLayer, bool aForward = true );

    /**
     * Optimises the via layers. Ensures that vias that are routed through only on one layer do not count towards total
     * length calculations.
     */
    static void
    optimiseViaLayers( const std::vector<LENGTH_CALCULATION_ITEM*>&                            aVias,
                       std::vector<LENGTH_CALCULATION_ITEM*>&                                  aLines,
                       std::map<VECTOR2I, std::unordered_set<LENGTH_CALCULATION_ITEM*>>&       aLinesPositionMap,
                       const std::map<VECTOR2I, std::unordered_set<LENGTH_CALCULATION_ITEM*>>& aPadsPositionMap );

    /**
     * Merges any lines (traces) that are contiguous, on one layer, and with no junctions
     */
    static void mergeLines( std::vector<LENGTH_CALCULATION_ITEM*>&                            aLines,
                            std::map<VECTOR2I, std::unordered_set<LENGTH_CALCULATION_ITEM*>>& aLinesPositionMap );

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
    void inferViaInPad( const PAD* aPad, const LENGTH_CALCULATION_ITEM& aItem, LENGTH_DETAILS& aDetails ) const;
};

#endif //PCBNEW_LENGTH_CALCULATION_H
