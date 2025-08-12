/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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

#ifndef PREVIEW_POLYGON_GEOM_MANAGER__H_
#define PREVIEW_POLYGON_GEOM_MANAGER__H_

#include <geometry/shape_line_chain.h>
#include <geometry/geometry_utils.h>

/**
 * Class that handles the drawing of a polygon, including management of last corner deletion
 * and drawing of leader lines with various constraints (eg 45 deg only).
 *
 * This class handles only the geometry of the process.
 */
class POLYGON_GEOM_MANAGER
{
public:

    /**
     * "Listener" interface for a class that wants to be updated about
     * polygon geometry changes
     */
    class CLIENT
    {
    public:
        /**
         * Called before the first point is added - clients can do
         * initialization here, and can veto the start of the process
         * (e.g. if user cancels a dialog)
         *
         * @return false to veto start of new polygon
         */
        virtual bool OnFirstPoint( POLYGON_GEOM_MANAGER& aMgr ) = 0;

        ///< Sent when the polygon geometry changes
        virtual void OnGeometryChange( const POLYGON_GEOM_MANAGER& aMgr ) = 0;

        ///< Called when the polygon is complete
        virtual void OnComplete( const POLYGON_GEOM_MANAGER& aMgr ) = 0;

        virtual ~CLIENT()
        {
        }
    };

    /**
     * @param aClient is the client to pass the results onto
     */
    POLYGON_GEOM_MANAGER( CLIENT& aClient );

    /**
     * Lock in a polygon point.
     */
    bool AddPoint( const VECTOR2I& aPt );

    /**
     * Mark the polygon finished and update the client.
     */
    void SetFinished();

    /**
     * Clear the manager state and start again.
     */
    void Reset();

    /**
     * Set the leader mode to use when calculating the leader/returner lines.
     */
    void SetLeaderMode( LEADER_MODE aMode );

    LEADER_MODE GetLeaderMode() const
    {
        return m_leaderMode;
    }

    /**
     * Enables/disables self-intersecting polygons.
     *
     * @param aEnabled true if self-intersecting polygons are enabled.
     */
    void AllowIntersections( bool aEnabled )
    {
        m_intersectionsAllowed = true;
    }

    /**
     * Check whether self-intersecting polygons are enabled.
     *
     * @return true if self-intersecting polygons are enabled.
     */
    bool IntersectionsAllowed() const
    {
        return m_intersectionsAllowed;
    }

    /**
     * Check whether the locked points constitute a self-intersecting outline.
     *
     * @param aIncludeLeaderPts when true, also the leading points (not placed ones) will be tested.
     * @return True when the outline is self-intersecting.
     */
    bool IsSelfIntersecting( bool aIncludeLeaderPts ) const;

    /**
     * Set the current cursor position
     */
    void SetCursorPosition( const VECTOR2I& aPos );

    /**
     * @return true if the polygon in "in progress", i.e. it has at least
     * one locked-in point
     */
    bool IsPolygonInProgress() const;

    int PolygonPointCount() const;

    /**
     * @return true if locking in the given point would close the current polygon.
     */
    bool NewPointClosesOutline( const VECTOR2I& aPt ) const;

    /**
     * Remove the last-added point from the polygon
     */
    std::optional<VECTOR2I> DeleteLastCorner();

    /* =================================================================
     * Interfaces for users of the geometry
     */

    /**
     * Get the "locked-in" points that describe the polygon itself
     */
    const SHAPE_LINE_CHAIN& GetLockedInPoints() const
    {
        return m_lockedPoints;
    }

    /**
     * Get the points comprising the leader line (the line from the
     * last locked-in point to the current cursor position
     *
     * How this is drawn will depend on the LEADER_MODE
     */
    const SHAPE_LINE_CHAIN& GetLeaderLinePoints() const
    {
        return m_leaderPts;
    }

    /**
     * Get the points from the current cursor position
     * to the polygon start point
     */
    const SHAPE_LINE_CHAIN& GetLoopLinePoints() const
    {
        return m_loopPts;
    }

private:

    /**
     * Update the leader and loop lines points based on a new endpoint (probably
     * a cursor position)
     */
    void updateTemporaryLines( const VECTOR2I& aEndPoint,
                               LEADER_MODE     aModifier = LEADER_MODE::DIRECT );

    ///< The "user" of the polygon data that is informed when the geometry changes
    CLIENT& m_client;

    ///< The current mode of the leader line
    LEADER_MODE m_leaderMode;

    ///< Flag enabling self-intersecting polygons
    bool m_intersectionsAllowed;

    ///< Point that have been "locked in"
    SHAPE_LINE_CHAIN m_lockedPoints;

    ///< Points in the temporary "leader" line(s)
    SHAPE_LINE_CHAIN m_leaderPts;

    ///< Points between the cursor and start point
    SHAPE_LINE_CHAIN m_loopPts;
};

#endif // PREVIEW_POLYGON_GEOM_MANAGER__H_
