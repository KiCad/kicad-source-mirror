/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
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

#ifndef __PNS_MEANDER_PLACER_BASE_H
#define __PNS_MEANDER_PLACER_BASE_H

#include <math/vector2d.h>

#include <geometry/shape_line_chain.h>

#include "pns_node.h"
#include "pns_line.h"
#include "pns_placement_algo.h"
#include "pns_meander.h"

namespace PNS {

class ROUTER;
class SHOVE;
class OPTIMIZER;

/**
 * Base class for Single trace & Differential pair meandering tools, as both of them share a
 * lot of code.
 */
class MEANDER_PLACER_BASE : public PLACEMENT_ALGO
{
public:
    ///< Result of the length tuning operation
    enum TUNING_STATUS {
        TOO_SHORT = 0,
        TOO_LONG,
        TUNED
    };

    MEANDER_PLACER_BASE( ROUTER* aRouter );
    virtual ~MEANDER_PLACER_BASE();

    /**
     * Return the resultant length or skew of the tuned traces.
     */
    virtual long long int TuningLengthResult() const = 0;

    /**
     * Return the resultant delay or skew of the tuned traces.
     */
    virtual int64_t TuningDelayResult() const { return 0; };

    bool HasBaseline() const { return m_baselineLength != 0 || m_baselineDelay != 0; }

    long long int TuningLengthDelta() const { return TuningLengthResult() - m_baselineLength; }
    int64_t TuningDelayDelta() const { return TuningDelayResult() - m_baselineDelay; }

    /**
     * Return the tuning status (too short, too long, etc.) of the trace(s) being tuned.
     */
    virtual TUNING_STATUS TuningStatus() const = 0;

    /**
     * Increase/decreases the current meandering amplitude by one step.
     *
     * @param aSign direction (negative = decrease, positive = increase).
     */
    virtual void AmplitudeStep( int aSign );

    /**
     * Increase/decrease the current meandering spacing by one step.
     *
     * @param aSign direction (negative = decrease, positive = increase).
     */
    virtual void SpacingStep( int aSign );

    /**
     * Return the clearance of the track(s) being length tuned
     *
     * @return clearance value in internal units
     */
    virtual int Clearance();

    /**
     * Return the current meandering configuration.
     *
     * @return the settings
     */
    virtual const MEANDER_SETTINGS& MeanderSettings() const;

    /*
     * Set the current meandering configuration.
     *
     * @param aSettings the settings.
     */
    virtual void UpdateSettings( const MEANDER_SETTINGS& aSettings);

    /**
     * Checks if it's OK to place the shape aShape (i.e. if it doesn't cause DRC violations
     * or collide with other meanders).
     *
     * @param aShape the shape to check.
     * @return true if the shape fits.
     */
    virtual bool CheckFit( MEANDER_SHAPE* aShape )
    {
        return false;
    }

    virtual const ITEM_SET TunedPath() = 0;

protected:
    /**
     * Take a set of meanders in \a aTuned and tunes their length to extend the original line
     * length by \a aElongation.
     */
    void tuneLineLength( MEANDERED_LINE& aTuned, long long int aElongation );

    /**
     * Calculate the total length of the line represented by an item set (tracks and vias)
     * @param aLine
     * @return
     */
    long long int lineLength( const ITEM_SET& aLine, const SOLID* aStartPad, const SOLID* aEndPad ) const;

    /**
     * Calculate the total delay of the line represented by an item set (tracks and vias)
     * @param aLine
     * @return
     */
    int64_t lineDelay( const ITEM_SET& aLine, const SOLID* aStartPad, const SOLID* aEndPad ) const;

    /**
     * Cache the per-session chain-extras length/delay (other nets in the same chain) so per-Move
     * use does not re-walk the live BOARD. Called from Start() of each derived placer.
     */
    void initChainExtras();

    /**
     * Return the length offset to subtract when converting a user-facing total signal length
     * target into a meander-only doMove target. The offset combines the chain-extras aggregate
     * (other nets in the same chain) with any unmeasured stub on the actively-tuned net that
     * lives outside the PNS-measured baseline.
     *
     * Single-ended and skew tuning both rely on this so the meander does not over-correct by
     * the chain budget already absorbed by sibling nets.
     */
    long long int chainNarrowingOffset() const;

    ///< Original path length/delay captured at Start().
    long long int m_baselineLength = 0;
    int64_t       m_baselineDelay = 0;

    ///< Aggregate length/delay of other nets in the same chain, cached at Start().
    ///< The non-edited members of a chain don't change during a tuning session, so we avoid
    ///< walking the live BOARD on every Move event.
    long long int m_chainExtrasLength = 0;
    long long int m_chainExtrasDelay = 0;
    bool          m_chainExtrasValid = false;

    ///< Pointer to world to search colliding items.
    NODE* m_world;

    ///< Width of the meandered trace(s).
    int m_currentWidth;

    ///< Meander settings.
    MEANDER_SETTINGS m_settings;

    ///< The current end point.
    VECTOR2I m_currentEnd;

    SOLID*   m_startPad_p;
    SOLID*   m_endPad_p;
    SOLID*   m_startPad_n;
    SOLID*   m_endPad_n;
};

}

#endif    // __PNS_MEANDER_PLACER_BASE_H
