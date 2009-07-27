/************************************************************************/
/* classes to handle items used in schematic: wires, bus, junctions ... */
/************************************************************************/

#ifndef CLASS_SCHEMATIC_ITEMS_H
#define CLASS_SCHEMATIC_ITEMS_H

#define DRAWJUNCTION_DIAMETER  32       /* Size (diameter) of junctions between wires */
#define DRAWNOCONNECT_SIZE 48       /* Rayon du symbole No Connexion */

/* flags pour BUS ENTRY (bus to bus ou wire to bus */
#define WIRE_TO_BUS 0
#define BUS_TO_BUS  1


/**
 * Class EDA_DrawLineStruct
 * is a segment decription base class to describe items which have 2 end
 * points (track, wire, draw line ...)
 */
class EDA_DrawLineStruct : public SCH_ITEM
{
public:
    int     m_Width;            // 0 = line, > 0 = tracks, bus ...
    wxPoint m_Start;            // Line start point
    wxPoint m_End;              // Line end point

    bool    m_StartIsDangling;
    bool    m_EndIsDangling;    // TRUE si Start ou End not connected  (wires, tracks...)

public:
    EDA_DrawLineStruct( const wxPoint& pos, int layer );
    ~EDA_DrawLineStruct() { }

    EDA_DrawLineStruct* Next() const { return (EDA_DrawLineStruct*) Pnext; }
    EDA_DrawLineStruct* Back() const { return (EDA_DrawLineStruct*) Pback; }

    virtual wxString GetClass() const
    {
        return wxT( "EDA_DrawLine" );
    }


    bool                IsOneEndPointAt( const wxPoint& pos );
    EDA_DrawLineStruct* GenCopy();

    bool IsNull()
    {
        return m_Start == m_End;
    }


    EDA_Rect     GetBoundingBox();

    virtual void Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset,
                       int draw_mode, int Color = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool         Save( FILE* aFile ) const;

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    // Geometric transforms (used in block operations):
    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the deplacement vector
     */
    virtual void Move(const wxPoint& aMoveVector)
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
    virtual void Mirror_Y(int aYaxis_position)
    {
        m_Start.x -= aYaxis_position;
        NEGATE(  m_Start.x );
        m_Start.x += aYaxis_position;
        m_End.x -= aYaxis_position;
        NEGATE(  m_End.x );
        m_End.x += aYaxis_position;
    }

#if defined(DEBUG)
    void         Show( int nestLevel, std::ostream& os );
#endif
};


class DrawNoConnectStruct : public SCH_ITEM    /* Symboles de non connexion */
{
public:
    wxPoint m_Pos;                      /* XY coordinates of NoConnect. */

public:
    DrawNoConnectStruct( const wxPoint& pos );
    ~DrawNoConnectStruct() { }
    virtual wxString GetClass() const
    {
        return wxT( "DrawNoConnect" );
    }


    DrawNoConnectStruct* GenCopy();

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    virtual void         Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                               const wxPoint& offset, int draw_mode,
                               int Color = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool                 Save( FILE* aFile ) const;

    /** Function HitTest
     * @return true if the point aPosRef is within item area
     * @param aPosRef = a wxPoint to test
     */
    bool HitTest( const wxPoint& aPosRef );

    EDA_Rect             GetBoundingBox();
    // Geometric transforms (used in block operations):
    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the deplacement vector
     */
    virtual void Move(const wxPoint& aMoveVector)
    {
        m_Pos += aMoveVector;
    }

    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y(int aYaxis_position)
    {
        m_Pos.x -= aYaxis_position;
        NEGATE(  m_Pos.x );
        m_Pos.x += aYaxis_position;
    }
};


/**
 * Class DrawBusEntryStruct
 * Struct de descr 1 raccord a 45 degres de BUS ou WIRE
 */
class DrawBusEntryStruct  : public SCH_ITEM
{
public:
    int     m_Width;
    wxPoint m_Pos;
    wxSize  m_Size;

public:
    DrawBusEntryStruct( const wxPoint& pos, int shape, int id );
    ~DrawBusEntryStruct() { }

    virtual wxString GetClass() const
    {
        return wxT( "DrawBusEntry" );
    }


    DrawBusEntryStruct* GenCopy();
    wxPoint             m_End() const;  // retourne la coord de fin du raccord
    virtual void        Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                              const wxPoint& offset, int draw_mode,
                              int Color = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool                Save( FILE* aFile ) const;

    EDA_Rect            GetBoundingBox();

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    // Geometric transforms (used in block operations):
    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the deplacement vector
     */
    virtual void Move(const wxPoint& aMoveVector)
    {
        m_Pos += aMoveVector;
    }

    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y(int aYaxis_position)
    {
        m_Pos.x -= aYaxis_position;
        NEGATE(  m_Pos.x );
        m_Pos.x += aYaxis_position;
        NEGATE(  m_Size.x );
    }
};

class DrawPolylineStruct  : public SCH_ITEM /* Polyligne (serie de segments) */
{
public:
    int m_Width;                            /* Tickness */
    std::vector<wxPoint> m_PolyPoints;      // list of points (>= 2)

public:
    DrawPolylineStruct( int layer );
    ~DrawPolylineStruct();

    virtual wxString GetClass() const
    {
        return wxT( "DrawPolyline" );
    }


    DrawPolylineStruct* GenCopy();
    virtual void        Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                              const wxPoint& offset, int draw_mode,
                              int Color = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool                Save( FILE* aFile ) const;

    /** Function AddPoint
     * add a corner to m_PolyPoints
     */
    void             AddPoint( const wxPoint& point )
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
    virtual int GetPenSize( );

    // Geometric transforms (used in block operations):
    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the deplacement vector
     */
    virtual void Move(const wxPoint& aMoveVector)
    {
        for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
            m_PolyPoints[ii] += aMoveVector;
    }
    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y(int aYaxis_position)
    {
        for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
        {
            m_PolyPoints[ii].x -= aYaxis_position;
            NEGATE(  m_PolyPoints[ii].x );
            m_PolyPoints[ii].x = aYaxis_position;
        }
    }
};


class DrawJunctionStruct : public SCH_ITEM
{
public:
    wxPoint m_Pos;                  /* XY coordinates of connection. */

public:
    DrawJunctionStruct( const wxPoint& pos );
    ~DrawJunctionStruct() { }

    virtual wxString GetClass() const
    {
        return wxT( "DrawJunction" );
    }


    /** Function HitTest
     * @return true if the point aPosRef is within item area
     * @param aPosRef = a wxPoint to test
     */
    bool HitTest( const wxPoint& aPosRef );

    EDA_Rect            GetBoundingBox();

    DrawJunctionStruct* GenCopy();

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    virtual void        Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                              const wxPoint& offset, int draw_mode,
                              int Color = -1 );
    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool                Save( FILE* aFile ) const;

    // Geometric transforms (used in block operations):
    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the deplacement vector
     */
    virtual void Move(const wxPoint& aMoveVector)
    {
        m_Pos += aMoveVector;
    }

    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y(int aYaxis_position)
    {
        m_Pos.x -= aYaxis_position;
        NEGATE(  m_Pos.x );
        m_Pos.x += aYaxis_position;
    }

#if defined(DEBUG)
    void                Show( int nestLevel, std::ostream& os );
#endif
};


#endif /* CLASS_SCHEMATIC_ITEMS_H */
