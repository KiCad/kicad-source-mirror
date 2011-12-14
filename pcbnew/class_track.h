/**
 * @file class_track.h
 * @brief Definitions for tracks, vias and zones.
 */

#ifndef CLASS_TRACK_H
#define CLASS_TRACK_H


#include "class_board_item.h"
#include "class_board_connected_item.h"
#include "PolyLine.h"


class TRACK;
class D_PAD;

// Via attributes (m_Shape parameter)
#define VIA_THROUGH             3           /* Always a through hole via */
#define VIA_BLIND_BURIED        2           /* this via can be on internal layers */
#define VIA_MICROVIA            1           /* this via which connect from an external layer
                                             * to the near neighbor internal layer */
#define VIA_NOT_DEFINED         0           /* not yet used */


/**
 * Function GetTrace
 * is a helper function to locate a trace segment having an end point at \a aPosition
 * on \a aLayerMask starting at \a aStartTrace and end at \a aEndTrace.
 * <p>
 * The segments of track that are flagged as deleted or busy are ignored.  Layer
 * visibility is also ignored.
 * </p>
 * @param aStartTrace A pointer to the TRACK object to begin searching.
 * @param aEndTrace A pointer to the TRACK object to stop the search.  A NULL value
 *                  searches to the end of the list.
 * @param aPosition A wxPoint object containing the position to test.
 * @param aLayerMask A layer or layers to mask the hit test.  Use -1 to ignore
 *                   layer mask.
 * @return A TRACK object pointer if found otherwise NULL.
 */
extern TRACK* GetTrace( TRACK* aStartTrace, TRACK* aEndTrace, const wxPoint& aPosition,
                        int aLayerMask );


class TRACK : public BOARD_CONNECTED_ITEM
{
    // make SetNext() and SetBack() private so that they may not be called from anywhere.
    // list management is done on TRACKs using DLIST<TRACK> only.
private:
    void SetNext( EDA_ITEM* aNext )       { Pnext = aNext; }
    void SetBack( EDA_ITEM* aBack )       { Pback = aBack; }


public:
    int         m_Width;            // Thickness of track, or via diameter
    wxPoint     m_Start;            // Line start point
    wxPoint     m_End;              // Line end point
    int         m_Shape;            // vias: shape and type, Track = shape..

protected:
    int         m_Drill;            // for vias: via drill (- 1 for default value)

public:
    BOARD_CONNECTED_ITEM* start;    // pointers to a connected item (pad or track)
    BOARD_CONNECTED_ITEM* end;
    std::vector<TRACK*> m_TracksConnected;  // list of other tracks connected to me
    std::vector<D_PAD*> m_PadsConnected;    // list of pads connected to me

    double      m_Param;            // Auxiliary variable ( used in some computations )

protected:
    TRACK( const TRACK& track );    // protected so Copy() is used instead.

public:
    TRACK( BOARD_ITEM* aParent, KICAD_T idtype = PCB_TRACE_T );

    /**
     * Function Copy
     * will copy this object whether it is a TRACK or a SEGVIA returning
     * the corresponding type.
     * Because of the way SEGVIA and SEGZONE are derived from TRACK and because there are
     * virtual functions being used, we can no longer simply copy a TRACK and
     * expect it to be a via or zone.  We must construct a true SEGVIA or SEGZONE so its
     * constructor can initialize the virtual function table properly.  This factory type
     * of function called Copy() can duplicate either a TRACK, SEGVIA, or SEGZONE.
     *
     * @return - TRACK*, SEGVIA*, or SEGZONE*, declared as the least common
     *           denominator: TRACK
     */
    TRACK* Copy() const;

    TRACK* Next() const { return (TRACK*) Pnext; }
    TRACK* Back() const { return (TRACK*) Pback; }

    /**
     * Function Move
     * move this object.
     * @param aMoveVector - the move vector for this object.
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Start += aMoveVector;
        m_End   += aMoveVector;
    }

    /**
     * Function Rotate
     * Rotate this object.
     * @param aRotCentre - the rotation point.
     * @param aAngle - the rotation angle in 0.1 degree.
     */
    virtual void Rotate( const wxPoint& aRotCentre, double aAngle );

    /**
     * Function Flip
     * Flip this object, i.e. change the board side for this object
     * @param aCentre - the rotation point.
     */
    virtual void Flip( const wxPoint& aCentre );

    void SetPosition( const wxPoint& aPos )     { m_Start = aPos; }     // overload
    const wxPoint GetPosition() const           { return m_Start; }     // overload

    void SetWidth( int aWidth )                 { m_Width = aWidth; }
    int GetWidth() const                        { return m_Width; }

    void SetEnd( const wxPoint& aEnd )          { m_End = aEnd; }
    const wxPoint& GetEnd() const               { return m_End; }

    void SetStart( const wxPoint& aStart )      { m_Start = aStart; }
    const wxPoint& GetStart() const             { return m_Start; }

    EDA_RECT GetBoundingBox() const;

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Function GetBestInsertPoint
     * searches the "best" insertion point within the track linked list.
     * The best point is the begging of the corresponding net code section.
     * (The BOARD::m_Track and BOARD::m_Zone lists are sorted by netcode.)
     * @param aPcb The BOARD to search for the insertion point.
     * @return TRACK* - the item found in the linked list (or NULL if no track)
     */
    TRACK* GetBestInsertPoint( BOARD* aPcb );

    /* Search (within the track linked list) the first segment matching the netcode
     * ( the linked list is always sorted by net codes )
     */
    TRACK* GetStartNetCode( int NetCode );

    /* Search (within the track linked list) the last segment matching the netcode
     * ( the linked list is always sorted by net codes )
     */
    TRACK* GetEndNetCode( int NetCode );

    /**
     * Function GetLength
     * returns the length of the track using the hypotenuse calculation.
     * @return double - the length of the track
     */
    double GetLength() const
    {
        double dx = m_Start.x - m_End.x;
        double dy = m_Start.y - m_End.y;

        return hypot( dx, dy );
    }

    /* Display on screen: */
    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int aDrawMode,
               const wxPoint& aOffset = ZeroOffset );

    /* divers */
    int GetShape() const { return m_Shape & 0xFF; }
    void SetShape( int aShape ) { m_Shape = aShape; }

    /**
     * Function TransformShapeWithClearanceToPolygon
     * Convert the track shape to a closed polygon
     * Used in filling zones calculations
     * Circles (vias) and arcs (ends of tracks) are approximated by segments
     * @param aCornerBuffer = a buffer to store the polygon
     * @param aClearanceValue = the clearance around the pad
     * @param aCircleToSegmentsCount = the number of segments to approximate a circle
     * @param aCorrectionFactor = the correction to apply to circles radius to keep
     * clearance when the circle is approximated by segment bigger or equal
     * to the real clearance value (usually near from 1.0)
     */
    void TransformShapeWithClearanceToPolygon( std::vector <CPolyPt>& aCornerBuffer,
                                               int                    aClearanceValue,
                                               int                    aCircleToSegmentsCount,
                                               double                 aCorrectionFactor );
    /**
     * Function SetDrillValue
     * Set the drill value for vias
     * @param drill_value = new drill value
    */
    void SetDrill( int aDrill )             { m_Drill = aDrill; }

    /**
     * Function GetDrill
     * returns the local drill setting for this VIA.  If you want the calculated value,
     * use GetDrillValue() instead.
     */
    int GetDrill() const                    { return m_Drill; }

    /**
     * Function GetDrillValue
     * "calculates" the drill value for vias (m-Drill if > 0, or default
     * drill value for the board.
     * @return real drill_value
    */
    int GetDrillValue() const;

    /**
     * Function SetDrillDefault
     * Set the drill value for vias at default value (-1)
    */
    void SetDrillDefault()      { m_Drill = -1; }

    /**
     * Function IsDrillDefault
     * @return true if the drill value is default value (-1)
    */
    bool IsDrillDefault()       { return m_Drill <= 0; }

    /**
     * Function ReturnMaskLayer
     * returns a "layer mask", which is a bitmap of all layers on which the
     * TRACK segment or SEGVIA physically resides.
     * @return int - a layer mask, see pcbstruct.h's LAYER_BACK, etc.
     */
    int ReturnMaskLayer() const;

    /**
     * Function IsPointOnEnds
     * returns STARTPOINT if point if near (dist = min_dist) start point, ENDPOINT if
     * point if near (dist = min_dist) end point,STARTPOINT|ENDPOINT if point if near
     * (dist = min_dist) both ends, or 0 if none of the above.
     * if min_dist < 0: min_dist = track_width/2
     */
    int IsPointOnEnds( const wxPoint& point, int min_dist = 0 );

    /**
     * Function IsNull
     * returns true if segment length is zero.
     */
    bool IsNull();

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_ITEM.
     * Display info about the track segment and the full track length
     * @param frame A EDA_DRAW_FRAME in which to print status information.
     */
    void DisplayInfo( EDA_DRAW_FRAME* frame );

    /**
     * Function DisplayInfoBase
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Display info about the track segment only, and does not calculate the full track length
     * @param frame A EDA_DRAW_FRAME in which to print status information.
     */
    void DisplayInfoBase( EDA_DRAW_FRAME* frame );

    /**
     * Function ShowWidth
     * returns the width of the track in displayable user units.
     */
    wxString ShowWidth() const;

    /**
     * Function Visit
     * is re-implemented here because TRACKs and SEGVIAs are in the same list
     * within BOARD.  If that were not true, then we could inherit the
     * version from EDA_ITEM.  This one does not iterate through scanTypes
     * but only looks at the first item in the list.
     * @param inspector An INSPECTOR instance to use in the inspection.
     * @param testData Arbitrary data used by the inspector.
     * @param scanTypes Which KICAD_T types are of interest and the order
     *  is significant too, terminated by EOT.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *  else SCAN_CONTINUE, and determined by the inspector.
     */
    SEARCH_RESULT Visit( INSPECTOR* inspector, const void* testData,
                         const KICAD_T scanTypes[] );


    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool HitTest( const wxPoint& refPos );

    /**
     * Function HitTest (overlaid)
     * tests if the given wxRect intersect this object.
     * For now, an ending point must be inside this rect.
     * @param refArea an EDA_RECT to test
     * @return bool - true if a hit, else false
     */
    bool HitTest( EDA_RECT& refArea );

    /**
     * Function GetVia
     * finds the first SEGVIA object at \a aPosition on \a aLayer starting at the trace.
     *
     * @param aPosition The wxPoint to HitTest() against.
     * @param aLayerMask The layer to match, pass -1 for a don't care.
     * @return A pointer to a SEGVIA object if found, else NULL.
     */
    TRACK* GetVia( const wxPoint& aPosition, int aLayerMask = -1 );

    /**
     * Function GetVia
     * finds the first SEGVIA object at \a aPosition on \a aLayer starting at the trace
     * and ending at \a aEndTrace.
     *
     * @param aEndTrace Pointer to the last TRACK object to end search.
     * @param aPosition The wxPoint to HitTest() against.
     * @param aLayerMask The layers to match, pass -1 for a don't care.
     * @return A pointer to a SEGVIA object if found, else NULL.
     */
    TRACK* GetVia( TRACK* aEndTrace, const wxPoint& aPosition, int aLayerMask );

    /**
     * Function GetTrace
     * return the trace segment connected to the segment at \a aEndPoint from \a
     * aStartTrace to \a aEndTrace.
     *
     * @param aStartTrace A pointer to the TRACK object to begin searching.
     * @param aEndTrace A pointer to the TRACK object to stop the search.  A NULL value
     *                  searches to the end of the list.
     * @param aEndPoint The start or end point of the segment to test against.
     * @return A TRACK object pointer if found otherwise NULL.
     */
    TRACK* GetTrace( TRACK* aStartTrace, TRACK* aEndTrace, int aEndPoint );

    /**
     * Function GetEndSegments
     * get the segments connected to the end point of the track.
     *  return 1 if OK, 0 when a track is a closed loop
     *  and the beginning and the end of the track in *StartTrack and *EndTrack
     *  Modify *StartTrack en *EndTrack  :
     *  (*StartTrack)->m_Start coordinate is the beginning of the track
     *  (*EndTrack)->m_End coordinate is the end of the track
     *  Segments connected must be consecutive in list
     */
    int GetEndSegments( int NbSegm, TRACK** StartTrack, TRACK** EndTrack );

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "TRACK" );
    }

     /**
     * Function GetClearance
     * returns the clearance in internal units.  If \a aItem is not NULL then the
     * returned clearance is the greater of this object's clearance and
     * aItem's clearance.  If \a aItem is NULL, then this objects clearance
     * is returned.
     * @param aItem is another BOARD_CONNECTED_ITEM or NULL
     * @return int - the clearance in internal units.
     */
    virtual int GetClearance( BOARD_CONNECTED_ITEM* aItem = NULL ) const;

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  showtrack_xpm; }

#if defined (DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void Show( int nestLevel, std::ostream& os );


    /**
     * Function ShowState
     * converts a set of state bits to a wxString
     * @param stateBits Is an OR-ed together set of bits like BUSY, EDIT, etc.
     */
    static wxString ShowState( int stateBits );

#endif
};


class SEGZONE : public TRACK
{
public:
    SEGZONE( BOARD_ITEM* aParent );

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "ZONE" );
    }


    SEGZONE* Next() const { return (SEGZONE*) Pnext; }

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  add_zone_xpm; }
};


class SEGVIA : public TRACK
{
public:
    SEGVIA( BOARD_ITEM* aParent );

    SEGVIA( const SEGVIA& source ) :
        TRACK( source )
    {
    }


    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int aDrawMode,
               const wxPoint& aOffset = ZeroOffset );

    /**
     * Function IsOnLayer
     * tests to see if this object is on the given layer.  Is virtual
     * from BOARD_ITEM.  Tests the starting and ending range of layers for the via.
     * @param aLayer the layer to test for.
     * @return bool - true if on given layer, else false.
     */
    bool IsOnLayer( int aLayer ) const;

    /**
     * Function SetLayerPair
     * set the .m_Layer member param:
     *  For a via m_Layer contains the 2 layers :
     * top layer and bottom layer used by the via.
     * The via connect all layers from top layer to bottom layer
     * 4 bits for the first layer and 4 next bits for the secaon layer
     * @param top_layer = first layer connected by the via
     * @param bottom_layer = last layer connected by the via
     */
    void SetLayerPair( int top_layer, int bottom_layer );

    /**
     * Function ReturnLayerPair
     * Return the 2 layers used by  the via (the via actually uses
     * all layers between these 2 layers)
     *  @param top_layer = pointer to the first layer (can be null)
     *  @param bottom_layer = pointer to the last layer (can be null)
     */
    void ReturnLayerPair( int* top_layer, int* bottom_layer ) const;

    const wxPoint GetPosition() const   // overload
    {
        return m_Start;
    }

    void SetPosition( const wxPoint& aPoint ) { m_Start = aPoint;  m_End = aPoint; }    // overload

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "VIA" );
    }

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  via_sketch_xpm; }

#if defined (DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void Show( int nestLevel, std::ostream& os );

#endif
};


#endif /* CLASS_TRACK_H */
