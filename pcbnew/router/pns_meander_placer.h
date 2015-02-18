/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_node.h"
#include "pns_via.h"
#include "pns_line.h"
#include "pns_placement_algo.h"
#include "pns_meander.h"
#include "pns_meander_placer_base.h"

class PNS_ROUTER;
class PNS_SHOVE;
class PNS_OPTIMIZER;
class PNS_ROUTER_BASE;

/**
 * Class PNS_MEANDER_PLACER
 *
 * Single track length matching/meandering tool.
 */
class PNS_MEANDER_PLACER : public PNS_MEANDER_PLACER_BASE
{
public:

    PNS_MEANDER_PLACER( PNS_ROUTER* aRouter );
    virtual ~PNS_MEANDER_PLACER();

    /// @copydoc PNS_PLACEMENT_ALGO::Start()
    virtual bool Start( const VECTOR2I& aP, PNS_ITEM* aStartItem );

    /// @copydoc PNS_PLACEMENT_ALGO::Move()
    virtual bool Move( const VECTOR2I& aP, PNS_ITEM* aEndItem );

    /// @copydoc PNS_PLACEMENT_ALGO::FixRoute()
    virtual bool FixRoute( const VECTOR2I& aP, PNS_ITEM* aEndItem );

    /// @copydoc PNS_PLACEMENT_ALGO::CurrentNode()
    PNS_NODE* CurrentNode( bool aLoopsRemoved = false ) const;

    /// @copydoc PNS_PLACEMENT_ALGO::Traces()
    const PNS_ITEMSET Traces();

    /// @copydoc PNS_PLACEMENT_ALGO::CurrentEnd()
    const VECTOR2I& CurrentEnd() const;

    /// @copydoc PNS_PLACEMENT_ALGO::CurrentNet()
    int CurrentNet() const;

    /// @copydoc PNS_PLACEMENT_ALGO::CurrentLayer()
    int CurrentLayer() const;

    /// @copydoc PNS_MEANDER_PLACER_BASE::TuningInfo()
    virtual const wxString TuningInfo() const;

    /// @copydoc PNS_MEANDER_PLACER_BASE::TuningStatus()
    virtual TUNING_STATUS TuningStatus() const;

    /// @copydoc PNS_MEANDER_PLACER_BASE::CheckFit()
    bool CheckFit ( PNS_MEANDER_SHAPE* aShape );

protected:

    bool doMove( const VECTOR2I& aP, PNS_ITEM* aEndItem, int aTargetLength );

    void setWorld( PNS_NODE* aWorld );

    virtual int origPathLength() const;

    ///> pointer to world to search colliding items
    PNS_NODE* m_world;

    ///> current routing start point (end of tail, beginning of head)
    VECTOR2I m_currentStart;

    ///> Current world state
    PNS_NODE* m_currentNode;

    PNS_LINE* m_originLine;
    PNS_LINE m_currentTrace;
    PNS_ITEMSET m_tunedPath;

    SHAPE_LINE_CHAIN m_finalShape;
    PNS_MEANDERED_LINE m_result;
    PNS_SEGMENT* m_initialSegment;

    int m_lastLength;
    TUNING_STATUS m_lastStatus;
};

#endif    // __PNS_MEANDER_PLACER_H
