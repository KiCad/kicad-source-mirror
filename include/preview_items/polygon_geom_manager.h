/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 Kicad Developers, see AUTHORS.txt for contributors.
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

#include <vector>
#include <math/vector2d.h>

/**
 * Class that handles the drawing of a polygon, including
 * management of last corner deletion and drawing of leader lines
 * with various constraints (eg 45 deg only).
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
         * initialisation here, and can veto the start of the process
         * (eg if user cancels a dialog)
         *
         * @return false to veto start of new polygon
         */
        virtual bool OnFirstPoint() = 0;

        ///> Sent when the polygon geometry changes
        virtual void OnGeometryChange( const POLYGON_GEOM_MANAGER& aMgr ) = 0;

        ///> Called when the polygon is complete
        virtual void OnComplete( const POLYGON_GEOM_MANAGER& aMgr ) = 0;
    };

    /**
     * The kind of the leader line
     */
    enum class LEADER_MODE
    {
        DIRECT,     ///> Unconstrained point-to-point
        DEG45,      ///> 45 Degree only
    };

    /**
     * @param the client to pass the results onto
     */
    POLYGON_GEOM_MANAGER( CLIENT& aClient );

    /**
     * Lock in a polygon point.
     */
    void AddPoint( const VECTOR2I& aPt );

    /**
     * Mark the polygon finished and update the client
     */
    void SetFinished();

    /**
     * Clear the manager state and start again
     */
    void Reset();

    /**
     * Set the leader mode to use when calculating the leader/returner
     * lines
     */
    void SetLeaderMode( LEADER_MODE aMode );

    /**
     * Set the current cursor position
     */
    void SetCursorPosition( const VECTOR2I& aPos );

    /**
     * @return true if the polygon in "in progress", i.e. it has at least
     * one locked-in point
     */
    bool IsPolygonInProgress() const;

    /**
     * @return true if locking in the given point would close the
     * current polygon
     */
    bool NewPointClosesOutline( const VECTOR2I& aPt ) const;

    /**
     * Remove the last-added point from the polygon
     */
    void DeleteLastCorner();

    /* =================================================================
     * Interfaces for users of the geometry
     */

    /**
     * Get the "locked-in" points that describe the polygon itself
     */
    const std::vector<VECTOR2I>& GetLockedInPoints() const;

    /**
     * Get the points comprising the leader line (the line from the
     * last locked-in point to the current cursor position
     *
     * How this is drawn will depend on the LEADER_MODE
     */
    const std::vector<VECTOR2I>& GetLeaderLinePoints() const;

private:

    /**
     * Update the leader line points based on a new endpoint (probably
     * a cursor position)
     */
    void updateLeaderPoints( const VECTOR2I& aEndPoint );

    ///> The "user" of the polygon data that is informed when the geometry changes
    CLIENT& m_client;

    ///> The current mode of the leader line
    LEADER_MODE m_leaderMode;

    ///> Point that have been "locked in"
    std::vector<VECTOR2I> m_lockedPoints;

    ///> Points in the temporary "leader" line(s)
    std::vector<VECTOR2I> m_leaderPts;
};

#endif // PREVIEW_POLYGON_GEOM_MANAGER__H_
