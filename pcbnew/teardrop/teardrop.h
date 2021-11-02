/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <pcb_edit_frame.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>

// TODO: remove MAGIC_TEARDROP_ZONE_ID use to identifay teardrops
// Use only MAGIC_TEARDROP_NAME, and later a specific property
#define MAGIC_TEARDROP_ZONE_ID 0x4242
#define MAGIC_TEARDROP_PADVIA_NAME "$teardrop_padvia$"
#define MAGIC_TEARDROP_TRACK_NAME "$teardrop_track$"
#define MAGIC_TEARDROP_BASE_NAME "$teardrop_"

class TRACK_BUFFER;

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

    wxPoint m_Pos;
    int m_Width;        // The diameter of a round shape, or the min size for others
    int m_Drill;
    int m_NetCode;
    bool m_IsRound;     // true for round shapes, false for any other shape
    bool m_IsPad;       // true for pads, false for vias and tracks
    BOARD_CONNECTED_ITEM* m_Parent;
};


enum class CURVED_OPTION
{
    OPTION_NONE,     // No curved teardrop shape
    OPTION_ROUND,    // Curved teardrop shape for vias and round pads
    OPTION_ALL_PADS,      // Curved teardrop shape for all pads but custom
};


/**
 * TEARDROP_PARAMETARS is a helper class to handle parameters needed to build teardrops
 * for a board
 * these parameters are sizes and filters
 */
class TEARDROP_PARAMETERS
{
    friend class TEARDROP_MANAGER;

public:
    TEARDROP_PARAMETERS():
        m_tdMaxLen( Millimeter2iu( 1.0 ) ),
        m_tdMaxHeight( Millimeter2iu( 2.0 ) ),
        m_lenghtRatio( 0.5),
        m_heightRatio( 1.0 ),
        m_curveSegCount( 0 ),
        m_tolerance( Millimeter2iu( 0.01 ) ),
        m_curveOpt( CURVED_OPTION::OPTION_NONE )
    {
    }

    /**
     * Set max allowed lenght and height for teardrops in IU.
     * a value <= 0 disable the constraint
     */
    void SetTeardropMaxSize( int aMaxLen, int aMaxHeight )
    {
        m_tdMaxLen = aMaxLen;
        m_tdMaxHeight = aMaxHeight;
    }

    /**
     * Set prefered lenght and height ratio for teardrops
     * the prefered lenght and height are VIAPAD width * aLenghtRatio and
     * VIAPAD width * aHeightRatio
     */
    void SetTeardropSizeRatio( double aLenghtRatio = 0.5, double aHeightRatio = 1.0 )
    {
        m_lenghtRatio = aLenghtRatio;
        m_heightRatio = aHeightRatio;
    }

    /**
     * Set the params for teardrop using curved shape
     * note: if aCurveSegCount is < 3, the shape uses a straight line
     */
    void SetTeardropCurvedPrms( CURVED_OPTION aCurveOpt = CURVED_OPTION::OPTION_NONE,
                                int aCurveSegCount = 5 )
    {
        m_curveOpt = aCurveOpt;
        m_curveSegCount = aCurveSegCount;
    }

protected:
    int    m_tdMaxLen;      /// max allowed lenght for teardrops in IU. <= 0 to disable
    int    m_tdMaxHeight;   /// max allowed height for teardrops in IU. <= 0 to disable
    double m_lenghtRatio;   /// The lenght of a teardrop as ratio between lenght and size of pad/via
    double m_heightRatio;   /// The height of a teardrop as ratio between height and size of pad/via
    int    m_curveSegCount; /// number of segments to build the curved sides of a teardrop area
                            /// must be > 2. for values <= 2 a straight line is used
    int    m_tolerance;     /// the max distance between a track end and a padvia position to see
                            /// them connected
    CURVED_OPTION m_curveOpt;
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
 * Size of area (height and lenght) are defined from the pad/via size or for pads having
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

    TEARDROP_MANAGER( BOARD* aBoard, PCB_EDIT_FRAME* aFrame ) :
        m_applyToViaPads( true ),
        m_applyToRoundShapesOnly( false ),
        m_applyToSurfacePads( true ),
        m_board( aBoard )
    {}

    /**
     * Set teardrops on a teardrop free board
     * @return the number of teardrop created
     * @param aCommitter is a BOARD_COMMIT reference (can be null)
     * @param aDiscardInSameZone = to do not create teardrops for pads in a zone of the same net
     * @param aFollowTracks = true to use a track connected to the initial track connected
     * to a pad / via if this initial track is too short to build the teardrop
     */
    int SetTeardrops( BOARD_COMMIT* aCommitter,
                      bool aDiscardInSameZone = true,
                      bool aFollowTracks = true );


    /**
     * Remove all teardrops
     * @return the count of removed teardrops
     * @param aCommitter is a BOARD_COMMITto handle undo/Redo commands
     * @param aCommitAfterRemove = true to commit when removed,
     * false if some other changes must be made and the actual commit postponed
     * Can be nullptr
     */
    int  RemoveTeardrops( BOARD_COMMIT* aCommitter, bool aCommitAfterRemove );

    /**
     * Set max allowed lenght and height for teardrops in IU.
     * a value <= 0 disable the constraint
     */
    void SetTeardropMaxSize( int aMaxLen, int aMaxHeight )
    {
        m_Parameters.SetTeardropMaxSize( aMaxLen, aMaxHeight );
    }


    /**
     * Set prefered lenght and height ratio for teardrops
     * the prefered lenght and height are VIAPAD width * aLenghtRatio and
     * VIAPAD width * aHeightRatio
     */
    void SetTeardropSizeRatio( double aLenghtRatio = 0.5, double aHeightRatio = 1.0 )
    {
        m_Parameters.SetTeardropSizeRatio( aLenghtRatio, aHeightRatio );
    }


    /**
     * Set the params for teardrop using curved shape
     * note: if aSegCount is < 3, the shape uses a straight line
     */
    void SetTeardropCurvedPrms( CURVED_OPTION aCurveOpt = CURVED_OPTION::OPTION_NONE,
                                int aCurveSegCount = 5 )
    {
        m_Parameters.SetTeardropCurvedPrms( aCurveOpt, aCurveSegCount );
    }

    /**
     * Define the items used to add a teardrop
     * @param aApplyToPadVias = true to add Td to vias and PTH
     * @param aApplyToRoundShapesOnly = true to restrict Td on round PTH
     * @param aApplyToSurfacePads = true to add Td to not drilled pads (like SMD)
     * @param aApplyToTracks = true to add Td to tracks connected point when having
     * different sizes
     */
    void SetTargets( bool aApplyToPadVias, bool aApplyToRoundShapesOnly,
                     bool aApplyToSurfacePads, bool aApplyToTracks );

    TEARDROP_PARAMETERS m_Parameters;

private:
    /**
     * Collect and build the list of all vias from the given board
     */
    void collectVias( std::vector< VIAPAD >& aList );

    /**
     * Build a list of pads candidate for teardrops from the given board
     * Pads with no net are not candidate (and have no track connected
     * Custom pads are not candidate because one cannot define a teardrop shape
     * @param aList is the list of VIAPAD to fill
     * @param aRoundShapesOnly = true to ignore not round shapes
     * @param aIncludeNotDrilled = true to include not drilled pads like SMD
     */
    void collectPadsCandidate( std::vector< VIAPAD >& aList,
                               bool aRoundShapesOnly,
                               bool aIncludeNotDrilled );

    /**
     * Build a list of all teardrops on the current board
     * @param aList is the list to populate
     */
    void collectTeardrops( std::vector< ZONE* >& aList );

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
    void computeCurvedForRoundShape( std::vector<wxPoint>& aPoly,
                                     int aTrackHalfWidth,
                                     VECTOR2D aTrackDir, VIAPAD& aViaPad,
                                     std::vector<wxPoint>& aPts ) const;


    /**
     * Compute the curve part points for teardrops connected to a rectangular/polygonal shape
     * The Bezier curve control points are not optimized for a special shape,
     * so use computeCurvedForRoundShape() for round shapes for better result
     */
    void computeCurvedForRectShape( std::vector<wxPoint>& aPoly, int aTdHeight,
                                            int aTrackHalfWidth, VIAPAD& aViaPad,
                                            std::vector<wxPoint>& aPts ) const;

    /**
     * Compute all teardrop points of the polygon shape
     * @return true if the polygonal shape was calculated, false if not buildable
     * use m_lenghtRatio and m_heightRatio
     */
    bool computeTeardropPolygonPoints( std::vector<wxPoint>& aCorners,
                                       PCB_TRACK* aTrack, VIAPAD& aVia,
                                       bool aFollowTracks,
                                       TRACK_BUFFER& aTrackLookupList) const;
    /**
     * Compute the 2 points on pad/via of the teardrop shape
     * @return false if these 2 points are not found
     * @param aTrack is the reference track included in teardrop
     * @param aViaPad is the teardrop anchor
     * teardrop height = aViaPad size * aHeightRatio
     * @param aPts is the buffer that contains initial and final teardrop polygonal shape
     * in aPts:
     * A and B ( aPts[0] and  aPts[1] ) are points on the track
     * C and E ( aPts[2] and  aPts[4] ) are points on the aViaPad
     * D ( aPts[3] ) is midpoint behind the aViaPad centre
     * m_heightRatio is the factor to calculate the aViaPad teardrop size
    */
    bool ComputePointsOnPadVia( PCB_TRACK* aTrack, VIAPAD& aViaPad,
                                std::vector<wxPoint>& aPts ) const;

    /**
     * Find a track connected to the end of another track
     * @return a reference to the touching track (or nullptr)
     * @param aMatchType returns the end point id 0, STARTPOINT, ENDPOINT
     * @param aTrackRef is the reference track
     * @param aEndpoint is the coordinate to test
     * @param aTrackLookupList is the buffer of available tracks
     */
    PCB_TRACK* findTouchingTrack( EDA_ITEM_FLAGS& aMatchType, PCB_TRACK* aTrackRef,
                                  const wxPoint& aEndPoint,
                                  TRACK_BUFFER& aTrackLookupList ) const;

    /**
     * Creates a teardrop (a ZONE item) from its polygonal shape, track netcode and layer
     * @param aTeardropVariant = variant of the teardrop( attached to a pad, or a track end )
     * @param aPoints is the polygonal shape
     * @param aTrack is the track connected to the starting points of the teardrop
     * (mainly for net info)
     */
    ZONE* createTeardrop( TEARDROP_VARIANT aTeardropVariant,
                          std::vector<wxPoint>& aPoints, PCB_TRACK* aTrack);

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
     * m_lenghtRatio is the lenght of teardrop (ratio pad/via size/teardrop len)
    */
    bool findAnchorPointsOnTrack( wxPoint& aStartPoint, wxPoint& aEndPoint,
                                  PCB_TRACK*& aTrack, VIAPAD& aViaPad,
                                  int* aEffectiveTeardropLen,
                                  bool aFollowTracks, TRACK_BUFFER& aTrackLookupList ) const;

private:
    bool    m_applyToViaPads;           // true to add a teardrop to vias and PTH pads
    bool    m_applyToRoundShapesOnly;   // true to add a teardrop to round pads only
    bool    m_applyToSurfacePads;       // true to add a teardrop not drilled pads (like SMD)
    bool    m_applyToTracks;            // true to add a teardrop to connected point of 2 tracks
                                        // having different width
    BOARD*  m_board;
    std::vector<ZONE*> m_createdTdList; // list of new created teardrops
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
    int idxFromLayNet( int aLayer, int aNetcode )
    {
        return ( aNetcode << 8 ) + ( aLayer & 0xFF );
    }

    // Track buffer, tracks are grouped by layer+netcode
    std::map< int, std::vector<PCB_TRACK*>* > m_map_tracks;
};
