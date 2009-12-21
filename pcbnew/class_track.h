/*******************************************************************/
/*	class_track.h: definitions relatives to tracks, vias and zones */
/*******************************************************************/

#ifndef CLASS_TRACK_H
#define CLASS_TRACK_H

#include "base_struct.h"
#include "PolyLine.h"


// Via attributes (m_Shape parmeter)
#define VIA_THROUGH             3           /* Always a through hole via */
#define VIA_BLIND_BURIED        2           /* this via can be on internal layers */
#define VIA_MICROVIA            1           /* this via which connect from an external layer to the near neightbour internal layer */
#define VIA_NOT_DEFINED         0           /* not yet used */

/***/

class TRACK : public BOARD_CONNECTED_ITEM
{
    // make SetNext() and SetBack() private so that they may not be called from anywhere.
    // list management is done on TRACKs using DLIST<TRACK> only.
private:
    void SetNext( EDA_BaseStruct* aNext )       { Pnext = aNext; }
    void SetBack( EDA_BaseStruct* aBack )       { Pback = aBack; }


public:
    int         m_Width;            // Thickness of track, or via diameter
    wxPoint     m_Start;            // Line start point
    wxPoint     m_End;              // Line end point
    int         m_Shape;            // vias: shape and type, Track = shape..

protected:
    int         m_Drill;            // for vias: via drill (- 1 for default value)

public:
    BOARD_ITEM* start;              // pointers to a connected item (pad or track)
    BOARD_ITEM* end;

    // chain = 0 indique une connexion non encore traitee
    int         m_Param;            // Auxiliary variable ( used in some computations )

protected:
    TRACK( const TRACK& track );    // protected so Copy() is used instead.

public:
    TRACK( BOARD_ITEM* aParent, KICAD_T idtype = TYPE_TRACK );

    /**
     * Function Copy
     * will copy this object whether it is a TRACK or a SEGVIA returning
     * the corresponding type.
     * @return - TRACK*, SEGVIA*, or SEGZONE*, declared as the least common
     *  demoninator: TRACK
     */
    TRACK* Copy() const;

    TRACK* Next() const { return (TRACK*) Pnext; }
    TRACK* Back() const { return (TRACK*) Pback; }


    /**
     * Function Move
     * move this object.
     * @param const wxPoint& aMoveVector - the move vector for this object.
     */
    virtual void Move(const wxPoint& aMoveVector)
    {
        m_Start += aMoveVector;
        m_End += aMoveVector;
    }

    /**
     * Function Rotate
     * Rotate this object.
     * @param const wxPoint& aRotCentre - the rotation point.
     * @param aAngle - the rotation angle in 0.1 degree.
     */
    virtual void Rotate(const wxPoint& aRotCentre, int aAngle);

    /**
     * Function Flip
     * Flip this object, i.e. change the board side for this object
     * @param const wxPoint& aCentre - the rotation point.
     */
    virtual void Flip(const wxPoint& aCentre );

    /**
     * Function GetPosition
     * returns the position of this object.
     * @return const wxPoint& - The position of this object.
     */
    wxPoint& GetPosition()
    {
        return m_Start;  // it had to be start or end.
    }


    EDA_Rect GetBoundingBox();


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool    Save( FILE* aFile ) const;

    /**
     * Function GetBestInsertPoint
     * searches the "best" insertion point within the track linked list.
     * The best point is the begging of the corresponding net code section.
     * (The BOARD::m_Track and BOARD::m_Zone lists are sorted by netcode.)
     * @param aPcb The BOARD to search for the insertion point.
     * @return TRACK* - the item found in the linked list (or NULL if no track)
     */
    TRACK*  GetBestInsertPoint( BOARD* aPcb );

    /* Search (within the track linked list) the first segment matching the netcode
     * ( the linked list is always sorted by net codes )
     */
    TRACK*  GetStartNetCode( int NetCode );

    /* Search (within the track linked list) the last segment matching the netcode
     * ( the linked list is always sorted by net codes )
     */
    TRACK*  GetEndNetCode( int NetCode );

    /**
     * Function GetLength
     * returns the length of the track using the hypotenuse calculation.
     * @return double - the length of the track
     */
    double  GetLength() const
    {
        int dx = m_Start.x - m_End.x;
        int dy = m_Start.y - m_End.y;

        return hypot( dx, dy );
    }


    /* Display on screen: */
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, int aDrawMode, const wxPoint& offset = ZeroOffset );

    /* divers */
    int Shape() const { return m_Shape & 0xFF; }

    /** Function TransformShapeWithClearanceToPolygon
     * Convert the track shape to a closed polygon
     * Used in filling zones calculations
     * Circles (vias) and arcs (ends of tracks) are approximated by segments
     * @param aCornerBuffer = a buffer to store the polygon
     * @param aClearanceValue = the clearance around the pad
     * @param aCircleToSegmentsCount = the number of segments to approximate a circle
     * @param aCorrectionFactor = the correction to apply to circles radius to keep
     * clearance when the circle is approxiamted by segment bigger or equal
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
    void SetDrillValue(int drill_value) { m_Drill = drill_value; }

    /**
     * Function SetDrillDefault
     * Set the drill value for vias at default value (-1)
    */
    void SetDrillDefault(void) { m_Drill = -1; }

    /**
     * Function IsDrillDefault
     * @return true if the drill value is default value (-1)
    */
    bool IsDrillDefault(void) { return m_Drill <= 0; }

    /**
     * Function GetDrillValue
     * calculate the drill value for vias (m-Drill if > 0, or default drill value for the board
     * @return real drill_value
    */
    int GetDrillValue() const;

    /**
     * Function ReturnMaskLayer
     * returns a "layer mask", which is a bitmap of all layers on which the
     * TRACK segment or SEGVIA physically resides.
     * @return int - a layer mask, see pcbstruct.h's LAYER_BACK, etc.
     */
    int             ReturnMaskLayer();

    int             IsPointOnEnds( const wxPoint& point, int min_dist = 0 );

    /**
     * Function IsNull
     * returns true if segment length is zero.
     */
    bool            IsNull();

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * Display info about the track segment and the full track length
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */
    void            DisplayInfo( WinEDA_DrawFrame* frame );

    /**
     * Function DisplayInfoBase
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Display info about the track segment only, and does not calculate the full track length
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */
    void            DisplayInfoBase( WinEDA_DrawFrame* frame );

    /**
     * Function ShowWidth
     * returns the width of the track in displayable user units.
     */
    wxString        ShowWidth();

    /**
     * Function Visit
     * is re-implemented here because TRACKs and SEGVIAs are in the same list
     * within BOARD.  If that were not true, then we could inherit the
     * version from EDA_BaseStruct.  This one does not iterate through scanTypes
     * but only looks at the first item in the list.
     * @param inspector An INSPECTOR instance to use in the inspection.
     * @param testData Arbitrary data used by the inspector.
     * @param scanTypes Which KICAD_T types are of interest and the order
     *  is significant too, terminated by EOT.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *  else SCAN_CONTINUE, and determined by the inspector.
     */
    SEARCH_RESULT   Visit( INSPECTOR* inspector, const void* testData,
                           const KICAD_T scanTypes[] );


    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool            HitTest( const wxPoint& refPos );

    /**
     * Function HitTest (overlayed)
     * tests if the given wxRect intersect this object.
     * For now, an ending point must be inside this rect.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool            HitTest( EDA_Rect& refArea );

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "TRACK" );
    }


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
};


class SEGVIA : public TRACK
{
public:
    SEGVIA( BOARD_ITEM* aParent );

    SEGVIA( const SEGVIA& source ) :
        TRACK( source )
    {
    }


    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, int aDrawMode, const wxPoint& offset = ZeroOffset );


    /**
     * Function IsOnLayer
     * tests to see if this object is on the given layer.  Is virtual
     * from BOARD_ITEM.  Tests the starting and ending range of layers for the
     * via.
     * @param aLayer The layer to test for.
     * @return bool - true if on given layer, else false.
     */
    bool    IsOnLayer( int aLayer ) const;

    void    SetLayerPair( int top_layer, int bottom_layer );
    void    ReturnLayerPair( int* top_layer, int* bottom_layer ) const;

    /**
     * Function GetPosition
     * returns the position of this object.
     * @return const wxPoint& - The position of this object.
     */
    wxPoint& GetPosition()
    {
        return m_Start;
    }


    void  SetPosition( const wxPoint& aPoint ) { m_Start = aPoint;  m_End = aPoint; }

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "VIA" );
    }


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
