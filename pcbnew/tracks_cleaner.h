/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_TRACKS_CLEANER_H
#define KICAD_TRACKS_CLEANER_H

#include <class_track.h>

class BOARD;
class BOARD_COMMIT;


// Helper class used to clean tracks and vias
class TRACKS_CLEANER
{
public:
    TRACKS_CLEANER( EDA_UNITS_T aUnits, BOARD* aPcb, BOARD_COMMIT& aCommit );

    /**
     * the cleanup function.
     * return true if some item was modified
     * @param aCleanVias = true to remove superimposed vias
     * @param aRemoveMisConnected = true to remove segments connecting 2 different nets
     * @param aMergeSegments = true to merge collinear segmenst and remove 0 len segm
     * @param aDeleteUnconnected = true to remove dangling tracks
     * @param aDeleteTracksinPad = true to remove tracks fully inside pads
     * (short circuits)
     */
    bool CleanupBoard( bool aDryRun, DRC_LIST* aItemsList, bool aCleanVias,
            bool aRemoveMisConnected, bool aMergeSegments, bool aDeleteUnconnected,
            bool aDeleteTracksinPad );

private:
    /* finds and remove all track segments which are connected to more than one net.
     * (short circuits)
     */
    bool removeBadTrackSegments();

    /**
     * Removes redundant vias like vias at same location
     * or on pad through
     */
    bool cleanupVias();

    /**
     * Removes dangling tracks
     */
    bool deleteDanglingTracks();

    /**
     * Removes tracks that are fully inside pads
     */
    bool deleteTracksInPads();

    /// Delete null length track segments
    bool deleteNullSegments( TRACKS& aTracks );

    /**
     * Merge collinear segments and remove duplicated and null len segments
     */
    bool cleanupSegments();

    /**
     * helper function
     * Rebuild list of tracks, and connected tracks
     * this info must be rebuilt when tracks are erased
     */
    void buildTrackConnectionInfo();

    /**
     * helper function
     * merge aTrackRef and aCandidate, when possible,
     * i.e. when they are colinear, same width, and obviously same layer
     * @return true if the segments are merged, false if not
     * @param aSeg1 is the reference
     * @param aSeg2 is the candidate, and after merging, the removed segment
     */
    bool mergeCollinearSegments( TRACK* aSeg1, TRACK* aSeg2 );

    /**
     * @return true if aTrack has at least one end dangling, i.e. connected
     * to nothing.
     * if aTrack is a via, it is dangling if the via is connected to nothing
     * or only one item.
     * @param aTrack is the track (or the via) to test.
     */
    bool testTrackEndpointDangling( TRACK* aTrack );

    /**
     * @return true if a track end position is a node, i.e. a end connected
     * to more than one item.
     * @param aTrack is the track to test.
     * @param aTstStart = true ot test the start point of the track, and false to
     * test the end point
     */
    bool testTrackEndpointIsNode( TRACK* aTrack, bool aTstStart );

    EDA_UNITS_T   m_units;
    BOARD*        m_brd;
    BOARD_COMMIT& m_commit;
    bool          m_dryRun;
    DRC_LIST*     m_itemsList;

    bool removeItems( std::set<BOARD_ITEM*>& aItems );
};


#endif //KICAD_TRACKS_CLEANER_H
