/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __PNS_MOUSE_TRAIL_TRACER_H
#define __PNS_MOUSE_TRAIL_TRACER_H

#include <math/vector2d.h>

#include <geometry/direction45.h>
#include <geometry/shape_line_chain.h>

namespace PNS {

class MOUSE_TRAIL_TRACER
{
public:
    MOUSE_TRAIL_TRACER();
    ~MOUSE_TRAIL_TRACER();

    void Clear();

    void AddTrailPoint( const VECTOR2I& aP );

    void SetTolerance( int toll ) { m_tolerance = toll; }

    void SetDefaultDirections( DIRECTION_45 aInitDirection, DIRECTION_45 aLastSegDir )
    {
        m_direction        = aInitDirection;
        m_lastSegDirection = aLastSegDir;
    }

    DIRECTION_45 GetPosture( const VECTOR2I& aP );

    void FlipPosture();

    /**
     * Disables the mouse-trail portion of the posture solver; leaving only the manual posture
     * switch and the previous-segment posture algorithm
     */
    void SetMouseDisabled( bool aDisabled = true ) { m_disableMouse = aDisabled; }

    bool IsManuallyForced() const { return m_manuallyForced; }

    VECTOR2I GetTrailLeadVector() const;
private:
     SHAPE_LINE_CHAIN m_trail;
    int              m_tolerance;
    DIRECTION_45     m_direction;
    DIRECTION_45     m_lastSegDirection;
    bool             m_forced;
    bool             m_disableMouse;
    bool             m_manuallyForced;
};

} // namespace PNS

#endif
