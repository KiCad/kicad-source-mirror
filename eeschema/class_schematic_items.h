/************************************************************************/
/* classes to handle items used in schematic: wires, bus, junctions ... */
/************************************************************************/

#ifndef CLASS_SCHEMATIC_ITEMS_H
#define CLASS_SCHEMATIC_ITEMS_H

/* Flags for BUS ENTRY (bus to bus or wire to bus */
#define WIRE_TO_BUS 0
#define BUS_TO_BUS  1


/**
 * Class SCH_LINE
 * is a segment description base class to describe items which have 2 end
 * points (track, wire, draw line ...)
 */
class SCH_LINE : public SCH_ITEM
{
public:
    int     m_Width;            // 0 = line, > 0 = tracks, bus ...
    wxPoint m_Start;            // Line start point
    wxPoint m_End;              // Line end point

    bool    m_StartIsDangling;
    bool    m_EndIsDangling;    // TRUE if not connected  (wires, tracks...)

public:
    SCH_LINE( const wxPoint& pos, int layer );
    ~SCH_LINE() { }

    SCH_LINE* Next() const { return (SCH_LINE*) Pnext; }
    SCH_LINE* Back() const { return (SCH_LINE*) Pback; }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_LINE" );
    }


    bool      IsOneEndPointAt( const wxPoint& pos );
    SCH_LINE* GenCopy();

    bool IsNull()
    {
        return m_Start == m_End;
    }


    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    EDA_Rect     GetBoundingBox();

    virtual void Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                       int draw_mode, int Color = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool         Save( FILE* aFile ) const;

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int  GetPenSize();

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        if( (m_Flags & STARTPOINT) == 0 )
            m_Start += aMoveVector;
        if( (m_Flags & ENDPOINT) == 0 )
            m_End += aMoveVector;
    }


    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_X( int aXaxis_position );
    virtual void Mirror_Y( int aYaxis_position );
    virtual void Rotate( wxPoint rotationPoint );

#if defined(DEBUG)
    void         Show( int nestLevel, std::ostream& os );

#endif
};


class SCH_NO_CONNECT : public SCH_ITEM
{
public:
    wxPoint m_Pos;                      /* XY coordinates of NoConnect. */
    wxSize  m_Size;                     // size of this symbol

public:
    SCH_NO_CONNECT( const wxPoint& pos );
    ~SCH_NO_CONNECT() { }
    virtual wxString GetClass() const
    {
        return wxT( "SCH_NO_CONNECT" );
    }


    SCH_NO_CONNECT* GenCopy();

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int     GetPenSize();

    virtual void    Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                          const wxPoint& offset, int draw_mode,
                          int Color = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool            Save( FILE* aFile ) const;

    /** Function HitTest
     * @return true if the point aPosRef is within item area
     * @param aPosRef = a wxPoint to test
     */
    bool            HitTest( const wxPoint& aPosRef );

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display
     * purposes.  This box should be an enclosing perimeter for visible
     * components of this object, and the units should be in the pcb or
     * schematic coordinate system.  It is OK to overestimate the size
     * by a few counts.
     */
    EDA_Rect        GetBoundingBox();

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }


    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );
    virtual void Mirror_X( int aXaxis_position );
    virtual void Rotate( wxPoint rotationPoint );
};


/**
 * Class SCH_BUS_ENTRY
 *
 * Defines a bus or wire entry.
 */
class SCH_BUS_ENTRY : public SCH_ITEM
{
public:
    int     m_Width;
    wxPoint m_Pos;
    wxSize  m_Size;

public:
    SCH_BUS_ENTRY( const wxPoint& pos, int shape, int id );
    ~SCH_BUS_ENTRY() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_BUS_ENTRY" );
    }


    SCH_BUS_ENTRY* GenCopy();
    wxPoint        m_End() const;
    virtual void   Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                         const wxPoint& offset, int draw_mode,
                         int Color = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool           Save( FILE* aFile ) const;

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display
     * purposes.  This box should be an enclosing perimeter for visible
     * components of this object, and the units should be in the pcb or
     * schematic coordinate system.  It is OK to overestimate the size
     * by a few counts.
     */
    EDA_Rect       GetBoundingBox();

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int    GetPenSize();

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }


    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );
    virtual void Mirror_X( int aXaxis_position );
    virtual void Rotate( wxPoint rotationPoint );
};

class SCH_POLYLINE : public SCH_ITEM
{
public:
    int m_Width;                            /* Thickness */
    std::vector<wxPoint> m_PolyPoints;      // list of points (>= 2)

public:
    SCH_POLYLINE( int layer );
    ~SCH_POLYLINE();

    virtual wxString GetClass() const
    {
        return wxT( "SCH_POLYLINE" );
    }


    SCH_POLYLINE* GenCopy();
    virtual void  Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                        const wxPoint& offset, int draw_mode,
                        int Color = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool          Save( FILE* aFile ) const;

    /** Function AddPoint
     * add a corner to m_PolyPoints
     */
    void           AddPoint( const wxPoint& point )
    {
        m_PolyPoints.push_back( point );
    }


    /** Function GetCornerCount
     * @return the number of corners
     */

    unsigned GetCornerCount() const { return m_PolyPoints.size(); }

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize();

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
            m_PolyPoints[ii] += aMoveVector;
    }


    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );
    virtual void Mirror_X( int aXaxis_position );
    virtual void Rotate( wxPoint rotationPoint );
};


class SCH_JUNCTION : public SCH_ITEM
{
public:
    wxPoint m_Pos;                  /* XY coordinates of connection. */
    wxSize  m_Size;

public:
    SCH_JUNCTION( const wxPoint& pos );
    ~SCH_JUNCTION() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_JUNCTION" );
    }


    /** Function HitTest
     * @return true if the point aPosRef is within item area
     * @param aPosRef = a wxPoint to test
     */
    bool          HitTest( const wxPoint& aPosRef );

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display
     * purposes.  This box should be an enclosing perimeter for visible
     * components of this object, and the units should be in the pcb or
     * schematic coordinate system.  It is OK to overestimate the size
     * by a few counts.
     */
    EDA_Rect      GetBoundingBox();

    SCH_JUNCTION* GenCopy();

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int   GetPenSize();

    virtual void  Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                        const wxPoint& offset, int draw_mode, int Color = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool          Save( FILE* aFile ) const;

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }


    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );
    virtual void Mirror_X( int aXaxis_position );
    virtual void Rotate( wxPoint rotationPoint );

#if defined(DEBUG)
    void         Show( int nestLevel, std::ostream& os );

#endif
};


#endif /* CLASS_SCHEMATIC_ITEMS_H */
