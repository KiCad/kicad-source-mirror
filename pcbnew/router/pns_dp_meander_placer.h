/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PNS_DP_MEANDER_PLACER_H
#define __PNS_DP_MEANDER_PLACER_H

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_node.h"
#include "pns_via.h"
#include "pns_line.h"
#include "pns_placement_algo.h"
#include "pns_meander.h"
#include "pns_meander_placer_base.h"
#include "pns_diff_pair.h"
#include "pns_debug_decorator.h"

namespace PNS {

class ROUTER;

/**
 * Differential Pair length-matching/meandering tool.
 */

class DP_MEANDER_PLACER : public MEANDER_PLACER_BASE
{
public:
    DP_MEANDER_PLACER( ROUTER* aRouter );
    ~DP_MEANDER_PLACER();

    /**
     * Start routing a single track at point \a aP, taking item \a aStartItem as anchor
     * (unless NULL).
     */
    bool Start( const VECTOR2I& aP, ITEM* aStartItem ) override;

    /**
     * Move the end of the currently routed trace to the point \a aP, taking \a aEndItem as
     * anchor (if not NULL).
     */
    bool Move( const VECTOR2I& aP, ITEM* aEndItem ) override;

    /**
     * Commit the currently routed track to the parent node, taking \a aP as the final end
     * point and \a aEndItem as the final anchor (if provided).
     *
     * @return true if route has been committed.  May return false if the routing result is
     *         violating design rules.  In such cases, the track is only committed if
     *         Settings.CanViolateDRC() is on.
     */
    bool FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish = false ) override;

    /// @copydoc PLACEMENT_ALGO::CommitPlacement()
    bool CommitPlacement() override;

    /// @copydoc PLACEMENT_ALGO::AbortPlacement()
    bool AbortPlacement() override;

    /// @copydoc PLACEMENT_ALGO::HasPlacedAnything()
    bool HasPlacedAnything() const override;

    const LINE Trace() const;

    const DIFF_PAIR& GetOriginPair();

    /**
     * Return the most recent world state.
     */
    NODE* CurrentNode( bool aLoopsRemoved = false ) const override;

    const ITEM_SET Traces() override;

    const ITEM_SET TunedPath() override;

    /// @copydoc PLACEMENT_ALGO::CurrentStart()
    const VECTOR2I& CurrentStart() const override;

    /// @copydoc PLACEMENT_ALGO::CurrentEnd()
    const VECTOR2I& CurrentEnd() const override;

    /// @copydoc PLACEMENT_ALGO::CurrentNets()
    const std::vector<NET_HANDLE> CurrentNets() const override;

    int CurrentLayer() const override;

    long long int totalLength();

    long long int TuningLengthResult() const override;

    /// @copydoc MEANDER_PLACER_BASE::TuningDelayResult()
    int64_t TuningDelayResult() const override;

    TUNING_STATUS TuningStatus() const override;

    bool CheckFit( MEANDER_SHAPE* aShape ) override;


private:
    friend class MEANDER_SHAPE;

    void meanderSegment( const SEG& aBase );

//    void addMeander ( PNS_MEANDER *aM );
//    void addCorner ( const VECTOR2I& aP );

    const SEG baselineSegment( const DIFF_PAIR::COUPLED_SEGMENTS& aCoupledSegs );
    bool pairOrientation( const DIFF_PAIR::COUPLED_SEGMENTS& aPair );

    void setWorld( NODE* aWorld );
    void release();

    long long int origPathLength() const;

    int64_t origPathDelay() const;

    void calculateTimeDomainTargets();

    ///< Current routing start point (end of tail, beginning of head).
    VECTOR2I m_currentStart;

    ///< Current world state.
    NODE* m_currentNode;

    DIFF_PAIR m_originPair;
    DIFF_PAIR::COUPLED_SEGMENTS_VEC m_coupledSegments;

    LINE m_currentTraceN, m_currentTraceP;
    ITEM_SET m_tunedPath, m_tunedPathP, m_tunedPathN;

    SHAPE_LINE_CHAIN m_finalShapeP, m_finalShapeN;
    MEANDERED_LINE m_result;
    LINKED_ITEM* m_initialSegment;

    long long int m_lastLength;
    int64_t       m_lastDelay;
    int           m_padToDieLengthP;
    int           m_padToDieLengthN;
    int64_t       m_padToDieDelayP;
    int64_t       m_padToDieDelayN;
    TUNING_STATUS m_lastStatus;

    NETCLASS* m_netClass;
};

}

#endif    // __PNS_DP_MEANDER_PLACER_H
