/****************************************************************/
/*	Headers fo library definition and lib component definitions */
/****************************************************************/

#ifndef CLASSES_BODY_ITEMS_H
#define CLASSES_BODY_ITEMS_H

#define TARGET_PIN_DIAM 12                                  /* Circle diameter drawn at the active end of pins */

#define DEFAULT_TEXT_SIZE 50                                /* Default size for field texts */
#define PART_NAME_LEN     15                                /* Maximum length of part name. */
#define PREFIX_NAME_LEN   5                                 /* Maximum length of prefix (IC, R, SW etc.). */
#define PIN_WIDTH         100                               /* Width between 2 pins in internal units. */
#define PIN_LENGTH        300                               /* Default Length of each pin to be drawn. */

#define INVERT_PIN_RADIUS   35                              /* Radius of inverted pin circle. */
#define CLOCK_PIN_DIM       40                              /* Dim of clock pin symbol. */
#define IEEE_SYMBOL_PIN_DIM 40                              /* Dim of special pin symbol. */


/**
 * Enum FILL_T
 * is the set of fill types used in plotting or drawing enclosed areas.
 */
enum FILL_T {
    NO_FILL,                        // Poly, Square, Circle, Arc = option No Fill
    FILLED_SHAPE,                   // Poly, Square, Circle, Arc = option Fill with current color ("Solid shape")
    FILLED_WITH_BG_BODYCOLOR,       /* Poly, Square, Circle, Arc = option Fill with background body color,
                                     * translucent (texts inside this shape can be seen)
                                     * not filled in B&W mode when plotting or printing
                                     */
};


/**
 * Enum ElectricPinType
 * is the set of schematic pin types, used in ERC tests.
 */
enum ElectricPinType {      /* Type des Pins. si modif: modifier tableau des mgs suivant */
    PIN_INPUT,
    PIN_OUTPUT,
    PIN_BIDI,
    PIN_TRISTATE,
    PIN_PASSIVE,
    PIN_UNSPECIFIED,
    PIN_POWER_IN,
    PIN_POWER_OUT,
    PIN_OPENCOLLECTOR,
    PIN_OPENEMITTER,
    PIN_NC,             /* No connect */
    PIN_NMAX            /* End of List (no used as pin type) */
};

/* Messages d'affichage du type electrique */
eda_global const wxChar* MsgPinElectricType[]
#ifdef MAIN
= {
    wxT( "input" ),
    wxT( "output" ),
    wxT( "BiDi" ),
    wxT( "3state" ),
    wxT( "passive" ),
    wxT( "unspc" ),
    wxT( "power_in" ),
    wxT( "power_out" ),
    wxT( "openCol" ),
    wxT( "openEm" ),
    wxT( "?????" )
}


#endif
;

/* Autres bits: bits du membre .Flag des Pins */
#define PINNOTDRAW 1        /* si 1: pin invisible */


/**
 * Enum DrawPinShape
 * is the set of shapes allowed for pins.
 */
enum  DrawPinShape {
    NONE   = 0,
    INVERT = 1,
    CLOCK  = 2,
    LOWLEVEL_IN  = 4,
    LOWLEVEL_OUT = 8
};


/**
 * Enum DrawPinOrient
 * is the set of orientations allowed for pins.
 */
enum  DrawPinOrient {
    PIN_RIGHT = 'R',
    PIN_LEFT  = 'L',
    PIN_UP = 'U',
    PIN_DOWN  = 'D',
};

// Struct to pass parameters for drawing pins, in function Draw
class DrawPinPrms
{
public:
    EDA_LibComponentStruct* m_Entry;    // Pointer to the component in lib
    bool m_DrawPinText;                 // Are pin texts drawn ?

    DrawPinPrms( EDA_LibComponentStruct* entry, bool drawpintext = true )
    {
        m_Entry = entry;
        m_DrawPinText = drawpintext;
    }
};

/****************************************************************************/
/* Classes for handle the body items of a compoment: pins add graphic items */
/****************************************************************************/


/* class LibEDA_BaseStruct : Basic class for items used in a library component
 *  (graphic shapes, texts, fields, pins)
 */

class LibEDA_BaseStruct : public EDA_BaseStruct
{
public:
    int     m_Unit;         /* Unit identification (for multi part per parkage)
                             *  0 if the item is common to all units */
    int     m_Convert;      /* Shape identification (for parts which have a convert shape)
                             *      0 if the item is common to all shapes */
    FILL_T  m_Fill;         /* NO_FILL, FILLED_SHAPE or FILLED_WITH_BG_BODYCOLOR. has meaning only for some items */

public:
    LibEDA_BaseStruct* Next()
    {
        return (LibEDA_BaseStruct*) Pnext;
    }


    LibEDA_BaseStruct( KICAD_T struct_type );
    virtual ~LibEDA_BaseStruct() { }

    /** Function Draw (virtual pure)
     * Draw A body item
     * @param aPanel = DrawPanel to use (can be null) mainly used for clipping purposes
     * @param aDC = Device Context (can be null)
     * @param aOffset = offset to draw
     * @param aColor = -1 to use the normal body item color, or use this color if >= 0
     * @param aDrawMode = GR_OR, GR_XOR, ...
     * @param aData = value or pointer used to pass others parametres, depending on body items.
     *          used for some items to force to force no fill mode
     *         ( has meaning only for items what can be filled ). used in printing or moving objects mode
     *         or to pass refernce to the lib component for pins
     * @param aTransformMatrix = Transform Matrix (rotaion, mirror ..)
     */
    virtual void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset, int aColor,
                       int aDrawMode, void* aData, int aTransformMatrix[2][2] ) = 0;

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool    Save( FILE* aFile ) const = 0;

    void            Display_Infos_DrawEntry( WinEDA_DrawFrame* frame );
};


/********/
/* Pins */
/********/
class LibDrawPin : public LibEDA_BaseStruct
{
public:
    int      m_PinLen;      /* Pin lenght */
    int      m_Orient;      /* Pin orientation (Up, Down, Left, Right) */
    int      m_PinShape;    /* Bitwise ORed: Pin shape (see enum DrawPinShape) */
    int      m_PinType;     /* Electrical pin properties */
    int      m_Attributs;   /* bit 0 != 0: pin invisible */
    long     m_PinNum;      /* Pin number: 4 Ascii code like "12" or "anod" or "G6"
                             * "12" is stored as "12\0\0" ans does not depend on endian type*/
    wxString m_PinName;
    int      m_PinNumSize, m_PinNameSize; /* Pin num and Pin name sizes */

//	int m_PinNumWidth, m_PinNameWidth;	/* (Currently Unused) Pin num and Pin name text width */
    wxPoint m_Pos;          /* Position or centre (Arc and Circle) or start point (segments) */
    int     m_Width;        /* Tickness */

public:
    LibDrawPin();
    ~LibDrawPin() { }

    LibDrawPin* Next() const { return (LibDrawPin*) Pnext; }
    LibDrawPin* Back() const { return (LibDrawPin*) Pback; }

    virtual wxString GetClass() const
    {
        return wxT( "LibDrawPin" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool    Save( FILE* aFile ) const;


    LibDrawPin*     GenCopy();
    void            Display_Infos( WinEDA_DrawFrame* frame );
    wxPoint         ReturnPinEndPoint();

    int ReturnPinDrawOrient( int TransMat[2][2] );
    void            ReturnPinStringNum( wxString& buffer ) const;
    void            SetPinNumFromString( wxString& buffer );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset, int aColor,
               int aDrawMode, void* aData, int aTransformMatrix[2][2] );

    void            DrawPinSymbol( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& pin_pos,
                                   int orient,
                                   int DrawMode, int Color = -1 );

    void            DrawPinTexts( WinEDA_DrawPanel* panel, wxDC* DC,
                                  wxPoint& pin_pos, int orient,
                                  int TextInside, bool DrawPinNum, bool DrawPinName,
                                  int Color, int DrawMode );
    void            PlotPinTexts( wxPoint& pin_pos, int orient,
                                  int TextInside, bool DrawPinNum, bool DrawPinNameint, int aWidth, bool aItalic );
};


/**************************/
/* Graphic Body Item: Arc */
/**************************/

class LibDrawArc      : public LibEDA_BaseStruct
{
public:
    int     m_Rayon;
    int     t1, t2;                 /* position des 2 extremites de l'arc en 0.1 degres */
    wxPoint m_ArcStart, m_ArcEnd;   /* position des 2 extremites de l'arc en coord reelles*/
    wxPoint m_Pos;          /* Position or centre (Arc and Circle) or start point (segments) */
    int     m_Width;        /* Tickness */

public:
    LibDrawArc();
    ~LibDrawArc() { }
    virtual wxString GetClass() const
    {
        return wxT( "LibDrawArc" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool    Save( FILE* aFile ) const;


    LibDrawArc*     GenCopy();

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset, int aColor,
               int aDrawMode, void* aData, int aTransformMatrix[2][2] );
};

/*****************************/
/* Graphic Body Item: Circle */
/*****************************/
class LibDrawCircle   : public LibEDA_BaseStruct
{
public:
    int m_Rayon;
    wxPoint m_Pos;          /* Position or centre (Arc and Circle) or start point (segments) */
    int     m_Width;        /* Tickness */

public:
    LibDrawCircle();
    ~LibDrawCircle() { }
    virtual wxString GetClass() const
    {
        return wxT( "LibDrawCircle" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool    Save( FILE* aFile ) const;


    LibDrawCircle*  GenCopy();

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset, int aColor,
               int aDrawMode, void* aData, int aTransformMatrix[2][2] );
};


/*********************************************/
/* Graphic Body Item: Text                   */
/* This is only a graphic text.              */
/* Fields like Ref , value... are not Text,  */
/* they are a separate class                 */
/*********************************************/
class LibDrawText  : public LibEDA_BaseStruct
			, public EDA_TextStruct
{

public:
    LibDrawText();
    ~LibDrawText() { }
    virtual wxString GetClass() const
    {
        return wxT( "LibDrawText" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool    Save( FILE* aFile ) const;


    LibDrawText*    GenCopy();

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset, int aColor,
               int aDrawMode, void* aData, int aTransformMatrix[2][2] );
};


/********************************/
/* Graphic Body Item: Rectangle */
/********************************/
class LibDrawSquare  : public LibEDA_BaseStruct
{
public:
    wxPoint m_End;
    wxPoint m_Pos;          /* Position or centre (Arc and Circle) or start point (segments) */
    int     m_Width;        /* Tickness */

public:
    LibDrawSquare();
    ~LibDrawSquare() { }
    virtual wxString GetClass() const
    {
        return wxT( "LibDrawSquare" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool    Save( FILE* aFile ) const;


    LibDrawSquare*  GenCopy();

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset, int aColor,
               int aDrawMode, void* aData, int aTransformMatrix[2][2] );
};

/**********************************/
/* Graphic Body Item: single line */
/**********************************/
class LibDrawSegment  : public LibEDA_BaseStruct
{
public:
    wxPoint m_End;
    wxPoint m_Pos;          /* Position or centre (Arc and Circle) or start point (segments) */
    int     m_Width;        /* Tickness */

public:
    LibDrawSegment();
    ~LibDrawSegment() { }
    virtual wxString GetClass() const
    {
        return wxT( "LibDrawSegment" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool    Save( FILE* aFile ) const;


    LibDrawSegment* GenCopy();

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset, int aColor,
               int aDrawMode, void* aData, int aTransformMatrix[2][2] );
};

/*********************************************/
/* Graphic Body Item: Polygon (set of lines) */
/*********************************************/
class LibDrawPolyline : public LibEDA_BaseStruct
{
public:
    int  m_CornersCount;
    int* m_PolyList;
    int     m_Width;        /* Tickness */

public:
    LibDrawPolyline();
    ~LibDrawPolyline()
    {
        if( m_PolyList )
            free( m_PolyList );
    }


    virtual wxString GetClass() const
    {
        return wxT( "LibDrawPolyline" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool        Save( FILE* aFile ) const;

    LibDrawPolyline*    GenCopy();
    void                AddPoint( const wxPoint& point );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset, int aColor,
               int aDrawMode, void* aData, int aTransformMatrix[2][2] );
};


/**********/
/* Fields */
/**********/

/* Fields , same as component fields.
 * can be defined in libraries (mandatory for ref and value, ca be useful for footprints)
 * 2 Fields are always defined :
 *     Prefix (U, IC..) with gives the reference in schematic)
 *     Name (74LS00..) used to find the component in libraries, and give the default value in schematic
 */
class LibDrawField :  public LibEDA_BaseStruct
		, public EDA_TextStruct
{
public:
    int      m_FieldId;         /* 0 a 11
                                 * 0 = Reference; 1 = Value
                                 * 2 = Default footprint, 3 = subsheet (not used, reserved)
                                 * .. 11 other fields
                                 */
    wxString m_Name;                            /* Field Name (not the fielsd text itself, that is .m_Text) */

public:

    LibDrawField* Next() const { return (LibDrawField*) Pnext; }
    LibDrawField* Back() const { return (LibDrawField*) Pback; }


    LibDrawField( int idfield = 2 );
    ~LibDrawField();
    virtual wxString GetClass() const
    {
        return wxT( "LibDrawField" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool    Save( FILE* aFile ) const;


    LibDrawField*   GenCopy();
    void            Copy( LibDrawField* Target );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset, int aColor,
               int aDrawMode, void* aData, int aTransformMatrix[2][2] );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test, in Field coordinate system
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& refPos );
};

#endif  //  CLASSES_BODY_ITEMS_H
