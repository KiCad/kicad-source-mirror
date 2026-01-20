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

#ifndef ARC_CHORD_PARAMS_H
#define ARC_CHORD_PARAMS_H

#include <geometry/eda_angle.h>
#include <math/vector2d.h>

/**
 * Arc geometry computed from a chord-based coordinate system.
 *
 * This class computes arc parameters using a coordinate system where:
 * - u: unit vector along the chord from start to end
 * - n: unit vector perpendicular to the chord, pointing toward the arc center
 *
 * This representation allows arc points to be generated without computing the
 * center point explicitly, which provides better numerical stability for
 * shallow arcs where the center point may be very far from the arc itself.
 *
 * The class can compute the center point when needed via GetCenterPoint().
 */
class ARC_CHORD_PARAMS
{
public:
    /**
     * Compute arc geometry from three points defining the arc.
     *
     * @param aStart Arc start point
     * @param aMid Arc midpoint (a point on the arc, not the chord midpoint)
     * @param aEnd Arc end point
     * @return true if valid arc geometry was computed, false if degenerate (collinear points)
     */
    bool Compute( const VECTOR2I& aStart, const VECTOR2I& aMid, const VECTOR2I& aEnd );

    /**
     * Check if the parameters represent a valid arc.
     */
    bool IsValid() const { return m_valid; }

    /**
     * Get the chord length (distance from start to end).
     */
    double GetChordLength() const { return m_chordLen; }

    /**
     * Get half the chord length.
     */
    double GetHalfChord() const { return m_halfChord; }

    /**
     * Get the unit vector along the chord (from start to end).
     */
    VECTOR2D GetChordUnitVec() const { return VECTOR2D( m_ux, m_uy ); }

    /**
     * Get the unit vector perpendicular to the chord, pointing toward the arc center.
     */
    VECTOR2D GetNormalUnitVec() const { return VECTOR2D( m_nx, m_ny ); }

    /**
     * Get the sagitta (perpendicular distance from chord midpoint to arc).
     */
    double GetSagitta() const { return m_sagitta; }

    /**
     * Get the arc radius.
     */
    double GetRadius() const { return m_radius; }

    /**
     * Get the distance from chord midpoint to arc center along the normal.
     */
    double GetCenterOffset() const { return m_centerOffset; }

    /**
     * Get the chord midpoint coordinates.
     */
    VECTOR2D GetChordMidpoint() const { return VECTOR2D( m_midx, m_midy ); }

    /**
     * Get the arc center point.
     *
     * Note: For shallow arcs, the center may be very far from the arc itself.
     * Consider using the chord-based methods when possible for better numerical stability.
     */
    VECTOR2D GetCenterPoint() const;

    /**
     * Get the angle from arc center to the start point.
     *
     * In the chord coordinate system, start is at u = -half_chord, so the direction
     * from center to start is: -sin(half_angle) * u - cos(half_angle) * n
     */
    EDA_ANGLE GetStartAngle() const;

    /**
     * Get the angle from arc center to the end point.
     *
     * In the chord coordinate system, end is at u = +half_chord, so the direction
     * from center to end is: sin(half_angle) * u - cos(half_angle) * n
     */
    EDA_ANGLE GetEndAngle() const;

    /**
     * Get the arc angle (total angle swept by the arc) in radians.
     */
    double GetArcAngle() const;

    // Direct access to internal parameters for performance-critical code
    double GetUx() const { return m_ux; }
    double GetUy() const { return m_uy; }
    double GetNx() const { return m_nx; }
    double GetNy() const { return m_ny; }
    double GetMidX() const { return m_midx; }
    double GetMidY() const { return m_midy; }

private:
    double m_chordLen = 0.0;
    double m_halfChord = 0.0;
    double m_ux = 0.0, m_uy = 0.0;      ///< Unit vector along chord (from start to end)
    double m_nx = 0.0, m_ny = 0.0;      ///< Unit vector perpendicular to chord, toward arc center
    double m_sagitta = 0.0;
    double m_radius = 0.0;
    double m_centerOffset = 0.0;        ///< Distance from chord midpoint to arc center along n
    double m_midx = 0.0, m_midy = 0.0;  ///< Chord midpoint coordinates
    bool   m_valid = false;
};

#endif // ARC_CHORD_PARAMS_H
