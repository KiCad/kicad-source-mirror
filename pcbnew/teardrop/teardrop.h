/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef TEARDROP_H
#define TEARDROP_H

#include <tool/tool_manager.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>
#include <drc/drc_rtree.h>
#include "teardrop_parameters.h"


// A class to store tracks grouped by layer and netcode
class TRACK_BUFFER
{
public:
    TRACK_BUFFER() {}

    /**
     * Add a track in buffer, in space grouping tracks having the same netcode and the same layer
     */
    void AddTrack( PCB_TRACK* aTrack, int aLayer, int aNetcode );

    /**
     * @return the buffer that stores tracks having the same layer and the same netcode
     */
    std::vector<PCB_TRACK*>* GetTrackList( int aLayer, int aNetcode ) const
    {
        return m_map_tracks.at( idxFromLayNet( aLayer, aNetcode ) );
    }

    /**
     * @return a reference to the internal buffer
     */
    const std::map< int, std::vector<PCB_TRACK*>* >& GetBuffer() const { return m_map_tracks; }

    static void GetNetcodeAndLayerFromIndex( int aIdx, int* aLayer, int* aNetcode )
    {
        *aLayer = aIdx & 0xFF;
        *aNetcode = aIdx >> 8;
    }

private:
    // Build an index from the layer id and the netcode, to store a track in buffer
    int idxFromLayNet( int aLayer, int aNetcode ) const
    {
        return ( aNetcode << 8 ) + ( aLayer & 0xFF );
    }

    // Track buffer, tracks are grouped by layer+netcode
    std::map< int, std::vector<PCB_TRACK*>* > m_map_tracks;
};


/**
 * TEARDROP_MANAGER manage and build teardrop areas
 * A teardrop area is a polygonal area (a copper ZONE) having:
 * 2 points on the track connected to a pad or via
 * 2 points on the outline of this pad or via
 * 1 point near the pad/via position (calculated in order to have this pad/via position
 * inside the area
 * The 2 sides joining a point on the track to the corresponding point on the pad/via
 * outline can be a straight line or a curved shape (defined from a Bezier curve)
 * This curved shape is built by segments (3 to 10) from this Bezier curve
 * Because one cannot build a suitable shape for a custom pad, custom pads are ignored.
 * Size of area (height and length) are defined from the pad/via size or for pads having
 * a size X and a size Y, the smallest of X,Y size.
 */
class TEARDROP_MANAGER
{
    friend class TEARDROP_PARAMETERS;

public:
    enum TEARDROP_VARIANT
    {
        TD_TYPE_PADVIA,     // Specify a teardrop on a pad via
        TD_TYPE_TRACKEND    // specify a teardrop on a rond end of a wide track
    };

    TEARDROP_MANAGER( BOARD* aBoard, TOOL_MANAGER* aToolManager );

    /**
     * Remove teardrops connected to any dirty pads, vias or tracks.  They need to be removed
     * before being rebuilt.
     *
     * NB: this must be called BEFORE the connectivity is updated for the change in question.
     */
    void RemoveTeardrops( BOARD_COMMIT& aCommit, const std::vector<BOARD_ITEM*>* dirtyPadsAndVias,
                          const std::set<PCB_TRACK*>* dirtyTracks );
    /**
     * Update teardrops on a list of items.
     */
    void UpdateTeardrops( BOARD_COMMIT& aCommit, const std::vector<BOARD_ITEM*>* dirtyPadsAndVias,
                          const std::set<PCB_TRACK*>* dirtyTracks, bool aForceFullUpdate = false );

    /**
     * Add teardrop on tracks of different sizes connected by their end
     */
    void AddTeardropsOnTracks( BOARD_COMMIT& aCommit, const std::set<PCB_TRACK*>* aTracks,
                               bool aForceFullUpdate = false );

    void DeleteTrackToTrackTeardrops( BOARD_COMMIT& aCommit );

    static int GetWidth( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer );
    static bool IsRound( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer );

    void BuildTrackCaches();

private:
    /**
     * @return true if the given aViaPad + aTrack is located inside a zone of the same netname
     */
    bool areItemsInSameZone( BOARD_ITEM* aPadOrVia, PCB_TRACK* aTrack) const;

    /**
     * Compute the curve part points for teardrops connected to a round shape
     * The Bezier curve control points are optimized for a round pad/via shape,
     * and do not give a good curve shape for other pad shapes
     * use m_m_heightRatio
     */
    void computeCurvedForRoundShape( const TEARDROP_PARAMETERS& aParams,
                                     std::vector<VECTOR2I>& aPoly, PCB_LAYER_ID aLayer,
                                     int aTrackHalfWidth,
                                     const VECTOR2D& aTrackDir, BOARD_ITEM* aOther,
                                     const VECTOR2I& aOtherPos, std::vector<VECTOR2I>& aPts ) const;


    /**
     * Compute the curve part points for teardrops connected to a rectangular/polygonal shape
     * The Bezier curve control points are not optimized for a special shape,
     * so use computeCurvedForRoundShape() for round shapes for better result
     */
    void computeCurvedForRectShape( const TEARDROP_PARAMETERS& aParams,
                                    std::vector<VECTOR2I>& aPoly, int aTdWidth,
                                    int aTrackHalfWidth, std::vector<VECTOR2I>& aPts,
                                    const VECTOR2I& aIntersection ) const;

    /**
     * Compute all teardrop points of the polygon shape
     * @return true if the polygonal shape was calculated, false if not buildable
     * use m_lengthRatio and m_heightRatio
     */
    bool computeTeardropPolygon( const TEARDROP_PARAMETERS& aParams,
                                 std::vector<VECTOR2I>& aCorners, PCB_TRACK* aTrack,
                                 BOARD_ITEM* aOther, const VECTOR2I& aOtherPos ) const;
    /**
     * Compute the 2 points on pad/via of the teardrop shape
     * @return false if these 2 points are not found
     * @param aLayer is the layer for the teardrop
     * @param aItem is the via/pad/track used to build the teardrop
     * @param aPos is the via/pad position, or track start or end
     * teardrop height = aViaPad size * aHeightRatio
     * @param aPts is the buffer that contains initial and final teardrop polygonal shape
     * in aPts:
     * A and B ( aPts[0] and  aPts[1] ) are points on the track
     * C and E ( aPts[2] and  aPts[4] ) are points on the aViaPad
     * D ( aPts[3] ) is midpoint behind the aViaPad centre
     * m_heightRatio is the factor to calculate the aViaPad teardrop size
    */
    bool computeAnchorPoints( const TEARDROP_PARAMETERS& aParams, PCB_LAYER_ID aLayer,
                              BOARD_ITEM* aItem, const VECTOR2I& aPos,
                              std::vector<VECTOR2I>& aPts ) const;

    /**
     * Find a track connected to the end of another track
     * @return a reference to the touching track (or nullptr)
     * @param aMatchType returns the end point id 0, STARTPOINT, ENDPOINT
     * @param aTrackRef is the reference track
     * @param aEndpoint is the coordinate to test
     * @param tracksRTree is an RTree containing the available tracks
     */
    PCB_TRACK* findTouchingTrack( EDA_ITEM_FLAGS& aMatchType, PCB_TRACK* aTrackRef,
                                  const VECTOR2I& aEndPoint ) const;

    /**
     * Creates a teardrop (a ZONE item) from its polygonal shape, track netcode and layer
     * @param aTeardropVariant = variant of the teardrop( attached to a pad, or a track end )
     * @param aPoints is the polygonal shape
     * @param aTrack is the track connected to the starting points of the teardrop
     * (mainly for net info)
     */
    ZONE* createTeardrop( TEARDROP_VARIANT aTeardropVariant,
                          std::vector<VECTOR2I>& aPoints, PCB_TRACK* aTrack ) const;

    ZONE* createTeardropMask( TEARDROP_VARIANT aTeardropVariant,
                              std::vector<VECTOR2I>& aPoints, PCB_TRACK* aTrack ) const;

    /**
     * Creates and adds a teardrop with optional mask to the board
     * @param aCommit the board commit to add the teardrop to
     * @param aTeardropVariant = variant of the teardrop( attached to a pad, or a track end )
     * @param aPoints is the polygonal shape
     * @param aTrack is the track connected to the starting points of the teardrop
     */
    void createAndAddTeardropWithMask( BOARD_COMMIT& aCommit, TEARDROP_VARIANT aTeardropVariant,
                                       std::vector<VECTOR2I>& aPoints, PCB_TRACK* aTrack );

    /**
     * Attempts to create a track-to-track teardrop
     * @param aCommit the board commit to add the teardrop to
     * @param aParams the teardrop parameters
     * @param aTeardropVariant = variant of the teardrop( attached to a pad, or a track end )
     * @param aTrack the source track
     * @param aCandidate the target item
     * @param aPos the connection position
     * @return true if teardrop was created successfully
     */
    bool tryCreateTrackTeardrop( BOARD_COMMIT& aCommit, const TEARDROP_PARAMETERS& aParams,
                                 TEARDROP_VARIANT aTeardropVariant, PCB_TRACK* aTrack,
                                 BOARD_ITEM* aCandidate, const VECTOR2I& aPos );

    /**
     * Set priority of created teardrops. smaller have bigger priority
     */
    void setTeardropPriorities();

    /**
     * @return true if a point on a track can be found as anchor point of a teardrop
     * @param aStartPoint is the start point of the track found (always inside the teardrop)
     * @param aEndPoint is the start point of the track found (always outside the teardrop)
     * @param aIntersection is the point where the track's centerline meets the pad/via edge
     * @param aTrack is the track connected to the pad/via used to search a anchor point
     *  this reference can be modified if a connected track to the initial track is selected
     * @param aOther is the via/pad/track used to build the teardrop
     * @param aOtherPos is the via/pad position, or track start or end
     * @param aEffectiveTeardropLen is the actual teardrop length, that can be smaller than expected
     *  if the connected track length is too small
     * @param aFollowTracks = true to use a connected track to aTrack if aTrack is too small
     * @param aTrackLookupList is the list of tracks to explore if aFollowTracks = true
     * m_lengthRatio is the length of teardrop (ratio pad/via size/teardrop len)
    */
    bool findAnchorPointsOnTrack( const TEARDROP_PARAMETERS& aParams, VECTOR2I& aStartPoint,
                                  VECTOR2I& aEndPoint, VECTOR2I& aIntersection,
                                  PCB_TRACK*& aTrack, BOARD_ITEM* aOther, const VECTOR2I& aOtherPos,
                                  int* aEffectiveTeardropLen ) const;

private:
    int                       m_tolerance;      // max dist between track end point and pad/via
                                                //   center to see them connected to ut a teardrop
    BOARD*                    m_board;
    TOOL_MANAGER*             m_toolManager;
    TEARDROP_PARAMETERS_LIST* m_prmsList;       // the teardrop parameters list, from the board design settings

    DRC_RTREE                 m_tracksRTree;
    TRACK_BUFFER              m_trackLookupList;
    std::vector<ZONE*>        m_createdTdList;  // list of new created teardrops
};

#endif  // ifndef TEARDROP_H
