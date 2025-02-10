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

#ifndef __PNS_MEANDER_PLACER_H
#define __PNS_MEANDER_PLACER_H

#include <math/vector2d.h>

#include <geometry/shape_line_chain.h>

#include "pns_node.h"
#include "pns_line.h"
#include "pns_placement_algo.h"
#include "pns_meander.h"
#include "pns_meander_placer_base.h"

namespace PNS {

class ROUTER;
class SHOVE;
class OPTIMIZER;

/**
 * Single track length matching/meandering tool.
 */
class MEANDER_PLACER : public MEANDER_PLACER_BASE
{
public:

    MEANDER_PLACER( ROUTER* aRouter );
    virtual ~MEANDER_PLACER();

    /// @copydoc PLACEMENT_ALGO::Start()
    virtual bool Start( const VECTOR2I& aP, ITEM* aStartItem ) override;

    /// @copydoc PLACEMENT_ALGO::Move()
    virtual bool Move( const VECTOR2I& aP, ITEM* aEndItem ) override;

    /// @copydoc PLACEMENT_ALGO::FixRoute()
    virtual bool FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish = false ) override;

    /// @copydoc PLACEMENT_ALGO::CommitPlacement()
    bool CommitPlacement() override;

    /// @copydoc PLACEMENT_ALGO::AbortPlacement()
    bool AbortPlacement() override;

    /// @copydoc PLACEMENT_ALGO::HasPlacedAnything()
    bool HasPlacedAnything() const override;

    /// @copydoc PLACEMENT_ALGO::CurrentNode()
    NODE* CurrentNode( bool aLoopsRemoved = false ) const override;

    /// @copydoc PLACEMENT_ALGO::Traces()
    const ITEM_SET Traces() override;

    /// @copydoc PNS_MEANDER_PLACER_BASE::TunedPath()
    const ITEM_SET TunedPath() override;

    /// @copydoc PLACEMENT_ALGO::CurrentStart()
    const VECTOR2I& CurrentStart() const override;

    /// @copydoc PLACEMENT_ALGO::CurrentEnd()
    const VECTOR2I& CurrentEnd() const override;

    /// @copydoc PLACEMENT_ALGO::CurrentNets()
    const std::vector<NET_HANDLE> CurrentNets() const override
    {
        return std::vector<NET_HANDLE> (1, m_originLine.Net() );
    }

    /// @copydoc PLACEMENT_ALGO::CurrentLayer()
    int CurrentLayer() const override;

    /// @copydoc MEANDER_PLACER_BASE::TuningLengthResult()
    long long int TuningLengthResult() const override;

    /// @copydoc MEANDER_PLACER_BASE::TuningDelayResult()
    int64_t TuningDelayResult() const override;

    /// @copydoc MEANDER_PLACER_BASE::TuningStatus()
    TUNING_STATUS TuningStatus() const override;

    /// @copydoc MEANDER_PLACER_BASE::CheckFit()
    bool CheckFit ( MEANDER_SHAPE* aShape ) override;

protected:
    bool doMove( const VECTOR2I& aP, ITEM* aEndItem, long long int aTargetLength,
                 long long int aTargetMin, long long int aTargetMax );

    virtual long long int origPathLength() const;

    virtual int64_t origPathDelay() const;

    virtual void calculateTimeDomainTargets();

    ///< current routing start point (end of tail, beginning of head)
    VECTOR2I m_currentStart;

    ///< Current world state
    NODE* m_currentNode;

    LINE     m_originLine;
    LINE     m_currentTrace;
    ITEM_SET m_tunedPath;

    SHAPE_LINE_CHAIN m_finalShape;
    MEANDERED_LINE   m_result;
    LINKED_ITEM*     m_initialSegment;

    ///< Total length added by pad to die size.
    int m_padToDieLength;

    ///< Total length added by pad to die size.
    int m_padToDieDelay;

    ///< The netclass for the placed segments
    NETCLASS* m_netClass;

    long long int m_lastLength;
    int64_t       m_lastDelay;
    TUNING_STATUS m_lastStatus;
};

}

#endif    // __PNS_MEANDER_PLACER_H
