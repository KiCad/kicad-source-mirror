/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_edit_frame.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>
#include "teardrop_parameters.h"

#define MAGIC_TEARDROP_PADVIA_NAME "$teardrop_padvia$"
#define MAGIC_TEARDROP_TRACK_NAME "$teardrop_track$"

class TRACK_BUFFER;
class PROGRESS_REPORTER;

/**
 * Stores info of a pad, via or track end to build a teardrop
 */
struct VIAPAD
{
    VIAPAD( PCB_VIA* aVia );
    VIAPAD( PAD* aPad );
    VIAPAD( PCB_TRACK* aTrack, ENDPOINT_T aEndPoint );

    bool IsOnLayer( PCB_LAYER_ID aLayer ) const
    {
        return m_Parent->IsOnLayer( aLayer );
    }

    VECTOR2I m_Pos;
    int m_Width;        // The diameter of a round shape, or the min size for others
    int m_Drill;
    int m_NetCode;
    bool m_IsRound;     // true for round shapes, false for any other shape
    bool m_IsPad;       // true for pads, false for vias and tracks
    BOARD_CONNECTED_ITEM* m_Parent;
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

    /**
     * TEARDROP_MANAGER Ctor
     * @param aBoard is the board to manage
     * @param aFrame is the board editor frame
     * @param aReporter  optional: used to show a progrss bar when refilling zones
     */
    TEARDROP_MANAGER( BOARD* aBoard, PCB_EDIT_FRAME* aFrame, PROGRESS_REPORTER* aReporter = nullptr );

    /**
     * Set teardrops on a teardrop free board
     * @return the number of teardrop created
     * @param aCommitter is a BOARD_COMMIT reference (can be null)
     * @param aFollowTracks = true to use a track connected to the initial track connected
     * to a pad / via if this initial track is too short to build the teardrop
     * @param aFillAfter = true to performa zone fill at end
     */
    int SetTeardrops( BOARD_COMMIT* aCommitter, bool aFollowTracks, bool aFillAfter );

    /**
     * Remove all teardrops
     * @return the count of removed teardrops
     * @param aCommitter is a BOARD_COMMITto handle undo/Redo commands
     * @param aCommitAfterRemove = true to commit when removed,
     * false if some other changes must be made and the actual commit postponed
     * Can be nullptr
     */
    int RemoveTeardrops( BOARD_COMMIT* aCommitter, bool aCommitAfterRemove );


private:
    /**
     * Collect and build the list of all vias from the given board
     */
    void collectVias( std::vector< VIAPAD >& aList ) const;

    /**
     * Build a list of pads candidate for teardrops from the given board
     * Pads with no net are not candidate (and have no track connected
     * Custom pads are not candidate because one cannot define a teardrop shape
     * @param aList is the list of VIAPAD to fill
     * @param aDrilledViaPad = true to include drilled shapes
     * @param aRoundShapesOnly = true to ignore not round shapes
     * @param aIncludeNotDrilled = true to include not drilled pads like SMD
     */
    void collectPadsCandidate( std::vector< VIAPAD >& aList,
                               bool aDrilledViaPad,
                               bool aRoundShapesOnly,
                               bool aIncludeNotDrilled ) const;

    /**
     * Build a list of all teardrops on the current board
     * @param aList is the list to populate
     */
    void collectTeardrops( std::vector< ZONE* >& aList ) const;

    /**
     * Add teardrop on tracks of different sizes connected by their end
     */
    int addTeardropsOnTracks( BOARD_COMMIT* aCommitter );

    /**
     * @return true if the given aVia + aTrack is located inside a zone of the same netname
     */
    bool isViaAndTrackInSameZone( VIAPAD& aVia, PCB_TRACK* aTrack) const;

    /**
     * Compute the curve part points for teardrops connected to a round shape
     * The Bezier curve control points are optimized for a round pad/via shape,
     * and do not give a good curve shape for other pad shapes
     * use m_m_heightRatio
     */
    void computeCurvedForRoundShape( TEARDROP_PARAMETERS* aCurrParams,
                                     std::vector<VECTOR2I>& aPoly,
                                     int aTrackHalfWidth,
                                     VECTOR2D aTrackDir, VIAPAD& aViaPad,
                                     std::vector<VECTOR2I>& aPts ) const;


    /**
     * Compute the curve part points for teardrops connected to a rectangular/polygonal shape
     * The Bezier curve control points are not optimized for a special shape,
     * so use computeCurvedForRoundShape() for round shapes for better result
     */
    void computeCurvedForRectShape( TEARDROP_PARAMETERS* aCurrParams,
                                    std::vector<VECTOR2I>& aPoly, int aTdHeight,
                                    int aTrackHalfWidth, VIAPAD& aViaPad,
                                    std::vector<VECTOR2I>& aPts ) const;

    /**
     * Compute all teardrop points of the polygon shape
     * @return true if the polygonal shape was calculated, false if not buildable
     * use m_lengthRatio and m_heightRatio
     */
    bool computeTeardropPolygonPoints( TEARDROP_PARAMETERS* aCurrParams,
                                       std::vector<VECTOR2I>& aCorners,
                                       PCB_TRACK* aTrack, VIAPAD& aVia,
                                       bool aFollowTracks,
                                       TRACK_BUFFER& aTrackLookupList) const;
    /**
     * Compute the 2 points on pad/via of the teardrop shape
     * @return false if these 2 points are not found
     * @param aLayer is the layer for the teardrop
     * @param aViaPad is the teardrop anchor
     * teardrop height = aViaPad size * aHeightRatio
     * @param aPts is the buffer that contains initial and final teardrop polygonal shape
     * in aPts:
     * A and B ( aPts[0] and  aPts[1] ) are points on the track
     * C and E ( aPts[2] and  aPts[4] ) are points on the aViaPad
     * D ( aPts[3] ) is midpoint behind the aViaPad centre
     * m_heightRatio is the factor to calculate the aViaPad teardrop size
    */
    bool ComputePointsOnPadVia( TEARDROP_PARAMETERS* aCurrParams,
                                PCB_LAYER_ID aLayer, VIAPAD& aViaPad,
                                std::vector<VECTOR2I>& aPts ) const;

    /**
     * Find a track connected to the end of another track
     * @return a reference to the touching track (or nullptr)
     * @param aMatchType returns the end point id 0, STARTPOINT, ENDPOINT
     * @param aTrackRef is the reference track
     * @param aEndpoint is the coordinate to test
     * @param aTrackLookupList is the buffer of available tracks
     */
    PCB_TRACK* findTouchingTrack( EDA_ITEM_FLAGS& aMatchType, PCB_TRACK* aTrackRef,
                                  const VECTOR2I& aEndPoint,
                                  TRACK_BUFFER& aTrackLookupList ) const;

    /**
     * Creates a teardrop (a ZONE item) from its polygonal shape, track netcode and layer
     * @param aTeardropVariant = variant of the teardrop( attached to a pad, or a track end )
     * @param aPoints is the polygonal shape
     * @param aTrack is the track connected to the starting points of the teardrop
     * (mainly for net info)
     */
    ZONE* createTeardrop( TEARDROP_VARIANT aTeardropVariant,
                          std::vector<VECTOR2I>& aPoints, PCB_TRACK* aTrack) const;

    /**
     * Set priority of created teardrops. smaller have bigger priority
     */
    void setTeardropPriorities();

    /**
     * @return true if a point on a track can be found as anchor point of a teardrop
     * @param aStartPoint is the start point of the track found (always inside the teardrop)
     * @param aEndPoint is the start point of the track found (always outside the teardrop)
     * @param aTrack is the track connected to the pad/via used to search a anchor point
     *  this reference can be modified if a connected track to the initial track is selected
     * @param aViaPad is the via/pad used to build the teardrop
     * @param aEffectiveTeardropLen is the actual teardrop length, that can be smaller than expected
     *  if the connected track length is too small
     * @param aFollowTracks = true to use a connected track to aTrack if aTrack is too small
     * @param aTrackLookupList is the list of tracks to explore if aFollowTracks = true
     * m_lengthRatio is the length of teardrop (ratio pad/via size/teardrop len)
    */
    bool findAnchorPointsOnTrack( TEARDROP_PARAMETERS* aCurrParams,
                                  VECTOR2I& aStartPoint, VECTOR2I& aEndPoint,
                                  PCB_TRACK*& aTrack, VIAPAD& aViaPad,
                                  int* aEffectiveTeardropLen,
                                  bool aFollowTracks, TRACK_BUFFER& aTrackLookupList ) const;

private:
    int                       m_tolerance;      // max dist between track end point and pad/via
                                                //   center to see them connected to ut a teardrop
    BOARD*                    m_board;
    TOOL_MANAGER*             m_toolManager;
    TEARDROP_PARAMETERS_LIST* m_prmsList;       // the teardrop parameters list, from the board design settings
    std::vector<ZONE*>        m_createdTdList;  // list of new created teardrops
    PCB_EDIT_FRAME*           m_frame;
    PROGRESS_REPORTER*        m_reporter;       // A reporter to show a progress bar if refilling zones
};


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
    std::vector<PCB_TRACK*>* GetTrackList( int aLayer, int aNetcode )
    {
        return m_map_tracks.at( idxFromLayNet( aLayer, aNetcode ) );
    }

    /**
     * @return a reference to the internal buffer
     */
    std::map< int, std::vector<PCB_TRACK*>* >& GetBuffer() { return m_map_tracks ; }

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

#endif  // ifndef TEARDROP_H
