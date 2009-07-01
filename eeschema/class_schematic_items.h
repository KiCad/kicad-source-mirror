/************************************************************************/
/* classes to handle items used in schematic: wires, bus, junctions ... */
/************************************************************************/

#ifndef CLASS_SCHEMATIC_ITEMS_H
#define CLASS_SCHEMATIC_ITEMS_H

#define DRAWJUNCTION_DIAMETER  32       /* Size (diameter) of junctions between wires */
#define DRAWMARKER_SIZE    16       /* Rayon du symbole marqueur */
#define DRAWNOCONNECT_SIZE 48       /* Rayon du symbole No Connexion */

/* flags pour BUS ENTRY (bus to bus ou wire to bus */
#define WIRE_TO_BUS 0
#define BUS_TO_BUS  1


enum TypeMarker {      /* Type des Marqueurs */
    MARQ_UNSPEC,
    MARQ_ERC,
    MARQ_PCB,
    MARQ_SIMUL,
    MARQ_NMAX        /* Derniere valeur: fin de tableau */
};


/* Messages correspondants aux types des marqueurs */
extern const wxChar* NameMarqueurType[];


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

#if defined(DEBUG)
    void         Show( int nestLevel, std::ostream& os );
#endif
};


class DrawMarkerStruct : public SCH_ITEM   /* marqueurs */
{
public:
    wxPoint    m_Pos;               /* XY coordinates of marker. */
    TypeMarker m_Type;
    int        m_MarkFlags;         // complements d'information
    wxString   m_Comment;           /* Texte (commentaireassocie eventuel */

public:
    DrawMarkerStruct( const wxPoint& pos, const wxString& text );
    ~DrawMarkerStruct();
    virtual wxString GetClass() const
    {
        return wxT( "DrawMarker" );
    }


    DrawMarkerStruct* GenCopy();
    wxString          GetComment();
    virtual void      Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                            const wxPoint& offset, int draw_mode,
                            int Color = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool              Save( FILE* aFile ) const;

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     * for a marker, has no meaning, but it is necessary to satisfy the SCH_ITEM class requirements
     */
    virtual int GetPenSize( ) { return 0; };

#if defined(DEBUG)
    void              Show( int nestLevel, std::ostream& os );
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

#if defined(DEBUG)
    void                Show( int nestLevel, std::ostream& os );
#endif
};


#endif /* CLASS_SCHEMATIC_ITEMS_H */
