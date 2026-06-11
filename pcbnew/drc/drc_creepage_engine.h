/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include <math/box2.h>
#include <layer_ids.h>
#include <pcb_shape.h>

#include <drc/drc_creepage_utils.h>

class BOARD;

/**
 * Result of a single creepage query between two nets on one layer.
 *
 * m_path holds the rendered creepage polyline, identical to the geometry the batch DRC marker
 * uses, so the realtime overlay and the eventual DRC marker draw the same shape.
 */
struct CREEPAGE_RESULT
{
    int                    m_netA = -1;
    int                    m_netB = -1;
    double                 m_distance = 0.0;
    double                 m_constraint = 0.0;
    bool                   m_violation = false;
    VECTOR2I               m_start;
    VECTOR2I               m_end;
    const BOARD_ITEM*      m_itemA = nullptr;
    const BOARD_ITEM*      m_itemB = nullptr;
    std::vector<PCB_SHAPE> m_path;
};


/**
 * Reusable creepage solver shared by the batch DRC provider and the realtime drag overlay.
 *
 * In whole-board mode (SolveNetPairWholeBoard) the engine rebuilds a graph for one net pair and
 * is the reference used to prove exact parity with the legacy inline path.
 *
 * In interactive mode the engine builds the static board-edge plus other-net sub-graph once
 * inside a constraint-bounded ROI (BeginInteractive), then per frame truncates back to that
 * static prefix, re-adds only the dragged item's geometry at its current board position, links
 * just the moving geometry, and solves the dragged nets against their neighbours (Update).
 */
class CREEPAGE_ENGINE
{
public:
    explicit CREEPAGE_ENGINE( BOARD& aBoard );

    void SetMinGrooveWidth( int aWidth ) { m_minGrooveWidth = aWidth; }

    /**
     * Solve a single net pair against the whole board on one layer, building a fresh graph.
     *
     * Returns the creepage result when a path shorter than the constraint exists (a violation),
     * otherwise std::nullopt. This is the parity reference for the batch provider.
     */
    std::optional<CREEPAGE_RESULT> SolveNetPairWholeBoard( int aNetA, int aNetB, PCB_LAYER_ID aLayer,
                                                           double aConstraint );

    /**
     * Begin an interactive drag session.
     *
     * Builds the static board-edge sub-graph once and records it as the immutable graph prefix.
     * aAffectedNets are the nets carried by the dragged items; aMargin bounds the search radius
     * (the largest creepage constraint plus a near-violation band). Other nets' geometry and the
     * copper paths are rebuilt each frame within that margin of the dragged items.
     */
    void BeginInteractive( PCB_LAYER_ID aLayer, const std::set<int>& aAffectedNets,
                           const std::set<const BOARD_ITEM*>& aMovingItems, int aMargin,
                           std::function<double( int, int )> aConstraintFn );

    /**
     * Recompute creepage for the dragged nets at the items' current board positions.
     *
     * Returns the nearest creepage path per affected net pair whose distance is below
     * constraint + aNearMargin. Reuses the static board-edge prefix; rebuilds the copper paths.
     */
    std::vector<CREEPAGE_RESULT> Update( int aNearMargin = 0 );

    void EndInteractive();

    bool IsInteractive() const { return m_interactive; }

private:
    /// Build the board-edge geometry into aGraph: groove width, outline (NPTH-subtracted), edge
    /// shapes, and the node decomposition. aOutline must outlive aGraph as its boardOutline. Shared
    /// by the whole-board solve and the interactive prefix so they stay in lockstep.
    void populateBoardEdgeGraph( CREEPAGE_GRAPH& aGraph, SHAPE_POLY_SET& aOutline );

    /// Run the same-parent ConnectChildren pass the legacy solver runs before Dijkstra.
    void connectChildren( CREEPAGE_GRAPH& aGraph );

    /// Extract a CREEPAGE_RESULT from a solved shortest path, or nullopt if not a violation.
    std::optional<CREEPAGE_RESULT>
    extractResult( const std::vector<std::shared_ptr<GRAPH_CONNECTION>>& aPath, double aDistance,
                   int aNetA, int aNetB, double aConstraint, int aNearMargin );

    /// Add net elements for the affected nets plus every other net whose bbox intersects aRegion.
    /// Returns the per-net virtual node keyed by net code.
    std::map<int, std::shared_ptr<GRAPH_NODE>> addNetsInRegion( const BOX2I& aRegion );

    /// (Re)build the board-edge visibility sub-graph from the board at its current state and
    /// record it as the prefix. Called once at drag start, or every frame when a dragged item
    /// carries NPTH holes (which are moving board-edge obstacles).
    void buildBoardEdgePrefix();

    /// True when any dragged item carries an NPTH hole, making the board-edge graph position
    /// dependent and therefore not cacheable across frames.
    bool movingItemsHaveNPTH() const;

private:
    BOARD&       m_board;
    int          m_minGrooveWidth = 0;

    bool         m_interactive = false;
    bool         m_dynamicEdges = false;
    PCB_LAYER_ID m_layer = UNDEFINED_LAYER;
    int          m_margin = 0;

    std::set<int>                     m_affectedNets;
    std::set<const BOARD_ITEM*>       m_movingItems;
    std::function<double( int, int )> m_constraintFn;

    // Bounding box of every non-affected net, cached once at drag start. Non-affected geometry is
    // static during a drag, so this avoids recomputing NETINFO_ITEM::GetBoundingBox (a full
    // connectivity sweep) for every net on every frame.
    std::map<int, BOX2I> m_staticNetBBoxes;

    std::unique_ptr<CREEPAGE_GRAPH> m_graph;
    std::unique_ptr<SHAPE_POLY_SET> m_outline;

    // The static board-edge sub-graph prefix: nodes [0, m_staticNodeCount) and connections
    // [0, m_staticConnCount) are pure board geometry, reused every frame.
    size_t m_staticNodeCount = 0;
    size_t m_staticConnCount = 0;
};
