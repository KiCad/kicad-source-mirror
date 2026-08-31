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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TEARDROP_H
#define TEARDROP_H

#include <unordered_map>

#include <tool/tool_manager.h>
#include <board.h>
#include <drc/drc_rtree.h>
#include "teardrop_parameters.h"

class BOARD;
class PCB_TRACK;
class ZONE;


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
     * @return a reference to the internal buffer
     */
    std::map<int, std::vector<PCB_TRACK*>>& GetBuffer() { return m_map_tracks; }

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
    std::map<int, std::vector<PCB_TRACK*>> m_map_tracks;
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
 * Size of area (height and length) are defined from the pad/via size or for pads having
 * a size X and a size Y, the smallest of X,Y size.
 * For a custom pad that size and position describe only the anchor; the copper may lie far away
 * and need not be convex or connected, so anchor-derived geometry must be clamped to that copper
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
     * Remove teardrops on dirty pads, vias or tracks, and any whose neighbouring copper moved,
     * dirtying the anchors of the latter.  Call this BEFORE connectivity is updated.
     */
    void RemoveTeardrops( BOARD_COMMIT& aCommit, std::vector<BOARD_ITEM*>* dirtyPadsAndVias,
                          std::set<PCB_TRACK*>* dirtyTracks,
                          const std::vector<BOARD_ITEM*>* dirtyCopper = nullptr );
    /**
     * Update teardrops on a list of items.
     */
    void UpdateTeardrops( BOARD_COMMIT& aCommit, const std::vector<BOARD_ITEM*>* dirtyPadsAndVias,
                          const std::set<PCB_TRACK*>* dirtyTracks, bool aForceFullUpdate = false );

    /**
     * Add teardrop on tracks of different sizes connected by their end
     * @param aSetPriorities is false when the caller numbers them itself once the rest are built
     */
    void AddTeardropsOnTracks( BOARD_COMMIT& aCommit, const std::set<PCB_TRACK*>* aTracks,
                               bool aForceFullUpdate = false, bool aSetPriorities = true );

    void DeleteTrackToTrackTeardrops( BOARD_COMMIT& aCommit );

    static int GetWidth( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer );
    static bool IsRound( BOARD_ITEM* aItem, PCB_LAYER_ID aLayer );
    static bool IsUniformlyRound( BOARD_ITEM* aItem );

    void BuildTrackCaches();

private:
    /**
     * @return true if the given aViaPad + aTrack is located inside a zone of the same netname
     */
    bool areItemsInSameZone( BOARD_ITEM* aPadOrVia, PCB_TRACK* aTrack) const;

    /**
     * @return one set per filled island of aZone on aLayer, holding what that island reaches.
     * Islands stay apart because two items are only joined by the pour if one island has both.
     */
    const std::vector<std::set<const BOARD_ITEM*>>& zoneConnections( ZONE* aZone,
                                                                    PCB_LAYER_ID aLayer ) const;

    /// @return aItem's netcode, or NETINFO_LIST::UNCONNECTED.  IsConnected() is not a proxy;
    /// a copper graphic in a footprint answers false to it and still carries a net.
    static int copperNetcode( const BOARD_ITEM* aItem );

    /// @return the clearance aSourceTrack owes aItem on aLayer, less the DRC epsilon, so that
    /// geometry sitting exactly at the limit is not rejected where DRC would pass it.
    int pairClearance( PCB_TRACK* aSourceTrack, BOARD_ITEM* aItem,
                       PCB_LAYER_ID aLayer ) const;

    /// Build the copper collision index, deferred so a commit with no teardrop candidate
    /// never pays for it.
    void ensureCopperIndex() const;

    /**
     * @return true if a teardrop with the given outline would touch copper of another net.
     * @param aExempt are the items it overlaps by construction, its anchor and its track(s);
     * without them a no-net teardrop collides with its own anchor, no net being shared with none
     */
    bool collidesWithOtherNets( const std::vector<VECTOR2I>& aPoints, PCB_TRACK* aSourceTrack,
                                const std::vector<const BOARD_ITEM*>& aExempt ) const;

    /**
     * Widen a teardrop as far as the surrounding copper allows.  The requested width knows
     * nothing of what is nearby, so bisect down from it; the result is within a few percent.
     * @return false if no width down to the track width clears the neighbouring copper
     */
    bool computeFittedTeardropPolygon( const TEARDROP_PARAMETERS& aParams,
                                       std::vector<VECTOR2I>& aPoints, PCB_TRACK* aTrack,
                                       PCB_TRACK* aSourceTrack, BOARD_ITEM* aOther,
                                       const VECTOR2I& aOtherPos ) const;

    /// Return the centerline chord length through aOther's copper span at aInsidePoint.
    /// Degenerate, arc, or non-crossing cases return INT_MAX to avoid rejection.
    int computeChordThroughShape( PCB_TRACK* aTrack, BOARD_ITEM* aOther, PCB_LAYER_ID aLayer,
                                  const VECTOR2I& aInsidePoint ) const;

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
     * so use computeCurvedForRoundShape() for round shapes for better result.
     * For rounded rectangles, special handling ensures curves are tangent to corner radii.
     */
    void computeCurvedForRectShape( const TEARDROP_PARAMETERS& aParams,
                                    std::vector<VECTOR2I>& aPoly, int aTdWidth,
                                    int aTrackHalfWidth, std::vector<VECTOR2I>& aPts,
                                    const VECTOR2I& aIntersection, BOARD_ITEM* aOther,
                                    const VECTOR2I& aOtherPos, PCB_LAYER_ID aLayer ) const;

    /**
     * Compute all teardrop points of the polygon shape
     * @return true if the polygonal shape was calculated, false if not buildable
     * use m_lengthRatio and m_heightRatio
     */
    bool computeTeardropPolygon( const TEARDROP_PARAMETERS& aParams,
                                 std::vector<VECTOR2I>& aCorners, PCB_TRACK* aTrack,
                                 PCB_TRACK* aSourceTrack, BOARD_ITEM* aOther,
                                 const VECTOR2I& aOtherPos ) const;
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
     * @param aSourceTrack is the board track aTrackRef stands in for, excluded from the search
     * @param aEndpoint is the coordinate to test
     * @param tracksRTree is an RTree containing the available tracks
     */
    PCB_TRACK* findTouchingTrack( EDA_ITEM_FLAGS& aMatchType, PCB_TRACK* aTrackRef,
                                  PCB_TRACK* aSourceTrack, const VECTOR2I& aEndPoint ) const;

    /**
     * Build the UUID a teardrop is created with.
     * @param aSlot separates the two teardrops of a crossing track; each slot spans two UUIDs
     */
    static KIID teardropUuid( const PCB_TRACK* aTrack, const BOARD_ITEM* aCandidate, int aSlot );

    /// Build the UUID of the mask sibling of aCopperUuid.  Producer and consumer of that
    /// pairing are far apart, so it has one home.
    static KIID maskUuidFor( const KIID& aCopperUuid );

    /// Set aStub up as the segment from one end of aTrack to aEnd, for a track that crosses
    /// the pad or via it connects to.
    static void buildCrossingStub( PCB_TRACK& aStub, const PCB_TRACK* aTrack,
                                   const VECTOR2I& aEnd );

    /**
     * Creates a teardrop (a ZONE item) from its polygonal shape, track netcode and layer
     * @param aTeardropVariant = variant of the teardrop( attached to a pad, or a track end )
     * @param aPoints is the polygonal shape
     * @param aSourceTrack is the board track it belongs to, not the stub geometry came from
     * @param aUuid is the UUID to give the teardrop
     */
    ZONE* createTeardrop( TEARDROP_VARIANT aTeardropVariant, std::vector<VECTOR2I>& aPoints,
                          PCB_TRACK* aSourceTrack, const KIID& aUuid ) const;

    ZONE* createTeardropMask( TEARDROP_VARIANT aTeardropVariant, std::vector<VECTOR2I>& aPoints,
                              PCB_TRACK* aSourceTrack, const KIID& aUuid ) const;

    /**
     * Creates and adds a teardrop with optional mask to the board
     * @param aCommit the board commit to add the teardrop to
     * @param aTeardropVariant = variant of the teardrop( attached to a pad, or a track end )
     * @param aPoints is the polygonal shape
     * @param aSourceTrack is the board track the teardrop belongs to
     * @param aUuid is the UUID to give the teardrop
     */
    void createAndAddTeardropWithMask( BOARD_COMMIT& aCommit, TEARDROP_VARIANT aTeardropVariant,
                                       std::vector<VECTOR2I>& aPoints, PCB_TRACK* aSourceTrack,
                                       const KIID& aUuid );

    /**
     * Attempts to create a track-to-track teardrop
     * @param aCommit the board commit to add the teardrop to
     * @param aParams the teardrop parameters
     * @param aTeardropVariant = variant of the teardrop( attached to a pad, or a track end )
     * @param aTrack the track the teardrop geometry is built from
     * @param aSourceTrack the board track it belongs to, the same as aTrack unless that is a stub
     * @param aCandidate the target item
     * @param aPos the connection position
     * @param aUuid is the UUID to give the teardrop
     * @return true if teardrop was created successfully
     */
    bool tryCreateTrackTeardrop( BOARD_COMMIT& aCommit, const TEARDROP_PARAMETERS& aParams,
                                 TEARDROP_VARIANT aTeardropVariant, PCB_TRACK* aTrack,
                                 PCB_TRACK* aSourceTrack, BOARD_ITEM* aCandidate,
                                 const VECTOR2I& aPos, const KIID& aUuid );

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
                                  PCB_TRACK*& aTrack, PCB_TRACK* aSourceTrack, BOARD_ITEM* aOther,
                                  const VECTOR2I& aOtherPos, int* aEffectiveTeardropLen ) const;

private:
    int                       m_tolerance;      // max dist between track end point and pad/via
                                                //   center to see them connected to ut a teardrop
    BOARD*                    m_board;
    TOOL_MANAGER*             m_toolManager;
    TEARDROP_PARAMETERS_LIST* m_prmsList;       // the teardrop parameters list, from the board design settings

    DRC_RTREE                 m_tracksRTree;
    TRACK_BUFFER              m_trackLookupList;
    std::vector<ZONE*>        m_createdTdList;  // list of new created teardrops

    /// Every copper item plus the teardrops built so far, to keep teardrops off other nets.
    mutable DRC_RTREE         m_copperRTree;
    mutable bool              m_copperIndexed;

    mutable std::unordered_map<PTR_LAYER_CACHE_KEY, std::vector<std::set<const BOARD_ITEM*>>>
            m_zoneConnectionCache;

    mutable std::unordered_map<PTR_PTR_LAYER_CACHE_KEY, int> m_pairClearanceCache;
};

#endif  // ifndef TEARDROP_H
