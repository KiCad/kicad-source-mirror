/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
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

#ifndef __PNS_MEANDER_SKEW_PLACER_H
#define __PNS_MEANDER_SKEW_PLACER_H

#include "pns_meander_placer.h"
#include "pns_diff_pair.h"

class PNS_ROUTER;
class PNS_SHOVE;
class PNS_OPTIMIZER;
class PNS_ROUTER_BASE;

/**
 * Class PNS_MEANDER_SKEW_PLACER
 *
 * Differential pair skew adjustment algorithm.
 */
class PNS_MEANDER_SKEW_PLACER : public PNS_MEANDER_PLACER
{
public:
    PNS_MEANDER_SKEW_PLACER( PNS_ROUTER* aRouter );
    ~PNS_MEANDER_SKEW_PLACER();

    /// @copydoc PNS_PLACEMENT_ALGO::Start()
    bool Start( const VECTOR2I& aP, PNS_ITEM* aStartItem );

    /// @copydoc PNS_PLACEMENT_ALGO::Move()
    bool Move( const VECTOR2I& aP, PNS_ITEM* aEndItem );

    /// @copydoc PNS_MEANDER_PLACER_BASE::TuningInfo()
    const wxString TuningInfo() const;

private:

    int currentSkew( ) const;
    int itemsetLength( const PNS_ITEMSET& aSet ) const;

    int origPathLength () const;

    PNS_DIFF_PAIR m_originPair;
    PNS_ITEMSET m_tunedPath, m_tunedPathP, m_tunedPathN;

    int m_coupledLength;
};

#endif    // __PNS_MEANDER_SKEW_PLACER_H
