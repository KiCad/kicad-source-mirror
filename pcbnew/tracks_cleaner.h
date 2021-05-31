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

#include <track.h>
#include <board.h>

class BOARD_COMMIT;
class CLEANUP_ITEM;


// Helper class used to clean tracks and vias
class TRACKS_CLEANER
{
public:

    enum CLEANUP_FLAGS
    {
        CF_STACKED_VIAS         = ( 1 << 0 ),  ///< Remove superimposed vias
        CF_DANGLING_VIAS        = ( 1 << 1 ),  ///< Remove vias that only connect on one layer
        CF_SHORTING_SEGMENTS    = ( 1 << 2 ),  ///< Remove segments that short two nets
        CF_COLLINEAR_SEGMENTS   = ( 1 << 3 ),  ///< Merge co-linear segments
        CF_DANGLING_SEGMENTS    = ( 1 << 4 ),  ///< Remove unconnected segments
        CF_SEGMENTS_INSIDE_PADS = ( 1 << 5 )   ///< Remove segments fully inside a pad
    };

    TRACKS_CLEANER( BOARD* aPcb, BOARD_COMMIT& aCommit );

    /**
     * the cleanup function.
     * @param aFlags controls which cleaning operations to run: @see CLEANUP_FLAGS
     * @param aIncludeNewTracksInCommit will include to-be-committed tracks when merging segments
     */
    void CleanupBoard( bool aDryRun, std::vector<std::shared_ptr<CLEANUP_ITEM> >* aItemsList,
                       int aFlags, bool aIncludeNewTracksInCommit = false );

private:
    /*
     * Removes track segments which are connected to more than one net (short circuits).
     */
    void removeShortingTrackSegments();

    /**
     * Removes tracks or vias only connected on one end
     * @param aTrack if true, clean dangling tracks
     * @param aVia if true, clean dangling vias
     * @return true if any items were deleted
     */
    bool deleteDanglingTracks( bool aTrack, bool aVia );

    void deleteTracksInPads();

    /**
     * Geometry-based cleanup: duplicate items, null items, colinear items.
     */
    void cleanup( bool aDeleteDuplicateVias, bool aDeleteNullSegments,
                  bool aDeleteDuplicateSegments, bool aMergeSegments );

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
     * @return true if a track end position is a node, i.e. a end connected
     * to more than one item.
     * @param aTrack is the track to test.
     * @param aTstStart = true ot test the start point of the track or false for end point
     */
    bool testTrackEndpointIsNode( TRACK* aTrack, bool aTstStart );

    void removeItems( std::set<BOARD_ITEM*>& aItems );

private:
    BOARD*                      m_brd;
    BOARD_COMMIT&               m_commit;
    bool                        m_dryRun;
    bool                        m_includeNewTracks;
    std::vector<std::shared_ptr<CLEANUP_ITEM> >* m_itemsList;
};


#endif //KICAD_TRACKS_CLEANER_H
