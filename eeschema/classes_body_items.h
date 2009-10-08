/****************************************************************/
/*  Headers for library definition and lib component definitions */
/****************************************************************/

/* Definitions of graphic items used to create shapes in component libraries.
 */
#ifndef CLASSES_BODY_ITEMS_H
#define CLASSES_BODY_ITEMS_H


class LIB_COMPONENT;
class PLOTTER;
class LIB_DRAW_ITEM;


#define TARGET_PIN_DIAM     12  /* Circle diameter drawn at the active end of
                                 * pins */

#define DEFAULT_TEXT_SIZE   50  /* Default size for field texts */
#define PART_NAME_LEN       15  /* Maximum length of part name. */
#define PREFIX_NAME_LEN     5   /* Maximum length of prefix (IC, R, SW etc.). */
#define PIN_WIDTH           100 /* Width between 2 pins in internal units. */
#define PIN_LENGTH          300 /* Default Length of each pin to be drawn. */

#if defined(KICAD_GOST)
#define INVERT_PIN_RADIUS   20  /* Radius of inverted pin circle. */
#else
#define INVERT_PIN_RADIUS   35  /* Radius of inverted pin circle. */
#endif

#define CLOCK_PIN_DIM       40  /* Dim of clock pin symbol. */
#define IEEE_SYMBOL_PIN_DIM 40  /* Dim of special pin symbol. */



/**
 * Enum ElectricPinType
 * is the set of schematic pin types, used in ERC tests.
 */
enum ElectricPinType
{
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
extern const wxChar* MsgPinElectricType[];

/* Autres bits: bits du membre .Flag des Pins */
#define PINNOTDRAW 1        /* si 1: pin invisible */


/**
 * Enum DrawPinShape
 * is the set of shapes allowed for pins.
 */
enum DrawPinShape
{
    NONE         = 0,
    INVERT       = 1,
    CLOCK        = 2,
    LOWLEVEL_IN  = 4,
    LOWLEVEL_OUT = 8
};


/**
 * Enum DrawPinOrient
 * is the set of orientations allowed for pins.
 */
enum  DrawPinOrient
{
    PIN_RIGHT = 'R',
    PIN_LEFT  = 'L',
    PIN_UP    = 'U',
    PIN_DOWN  = 'D',
};


/****************************************************************************/
/* Classes for handle the body items of a component: pins add graphic items */
/****************************************************************************/


/* class LIB_DRAW_ITEM : Basic class for items used in a library component
 *  (graphic shapes, texts, fields, pins)
 */

class LIB_DRAW_ITEM : public EDA_BaseStruct
{
public:
    int      m_Unit;     /* Unit identification (for multi part per package)
                          * 0 if the item is common to all units */
    int      m_Convert;  /* Shape identification (for parts which have a convert
                          * shape) 0 if the item is common to all shapes */
    FILL_T   m_Fill;     /* NO_FILL, FILLED_SHAPE or FILLED_WITH_BG_BODYCOLOR.
                          * has meaning only for some items */
    wxString m_typeName; /* Name of object displayed in the message panel. */

public:
    LIB_DRAW_ITEM* Next()
    {
        return (LIB_DRAW_ITEM*) Pnext;
    }


    LIB_DRAW_ITEM( KICAD_T struct_type, LIB_COMPONENT * aParent );
    LIB_DRAW_ITEM( const LIB_DRAW_ITEM& item );
    virtual ~LIB_DRAW_ITEM() { }

    /**
     * Function Draw (virtual pure)
     *
     * Draw A body item
     * @param aPanel = DrawPanel to use (can be null) mainly used for clipping
     *                 purposes
     * @param aDC = Device Context (can be null)
     * @param aOffset = offset to draw
     * @param aColor = -1 to use the normal body item color, or use this color
     *                 if >= 0
     * @param aDrawMode = GR_OR, GR_XOR, ...
     * @param aData = value or pointer used to pass others parameters,
     *                depending on body items. used for some items to force
     *                to force no fill mode ( has meaning only for items what
     *                can be filled ). used in printing or moving objects mode
     *                or to pass reference to the lib component for pins
     * @param aTransformMatrix = Transform Matrix (rotation, mirror ..)
     */
    virtual void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC,
                       const wxPoint &aOffset, int aColor, int aDrawMode,
                       void* aData, const int aTransformMatrix[2][2] ) = 0;

    /** Function GetPenSize virtual pure
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( ) = 0;

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const = 0;
    virtual bool Load( char* line, wxString& errorMsg ) = 0;

    LIB_COMPONENT * GetParent()
    {
        return (LIB_COMPONENT *)m_Parent;
    }

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& refPos )
    {
        return false;   // derived classes should override this function
    }

    /** Function HitTest (overlaid)
     * @return true if the point aPosRef is near this object
     * @param aPosRef = a wxPoint to test
     * @param aThreshold = max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] ) = 0;

   /** Function GetBoundingBox
     * @return the boundary box for this, in library coordinates
     */
    virtual EDA_Rect GetBoundingBox()
    {
        return EDA_BaseStruct::GetBoundingBox();
    }

    virtual void DisplayInfo( WinEDA_DrawFrame* frame );

    /**
     * Make a copy of this draw item.
     *
     * Classes derived from LIB_DRAW_ITEM must implement DoGenCopy().
     * This is just a placeholder for the derived class.
     *
     * @return Copy of this draw item.
     */
    LIB_DRAW_ITEM* GenCopy() { return DoGenCopy(); }

    /**
     * Test LIB_DRAW_ITEM objects for equivalence.
     *
     * @param other - Object to test against.
     *
     * @return bool - True if object is identical to this object.
     */
    bool operator==( const LIB_DRAW_ITEM& other ) const;
    bool operator==( const LIB_DRAW_ITEM* other ) const
    {
        return *this == *other;
    }

    /**
     * Test if another draw item is less than this draw object.
     *
     * @param other - Draw item to compare against.
     *
     * @return bool - True if object is less than this object.
     */
    bool operator<( const LIB_DRAW_ITEM& other) const;

    /**
     * Set drawing object offset from the current position.
     *
     * @param offset - Cooridinates to offset position.
     */
    void SetOffset( const wxPoint& offset ) { DoOffset( offset ); }

    /**
     * Test if any part of the draw object is inside rectangle bounds.
     *
     * This is used for block selection.  The real work is done by the
     * DoTestInside method for each derived object type.
     *
     * @param rect - Rectangle to check against.
     *
     * @return bool - True if object is inside rectangle.
     */
    bool Inside( EDA_Rect& rect ) { return DoTestInside( rect ); }

    /**
     * Move a draw object to a new position.
     *
     * The real work is done by the DoMove method for each derived object type.
     *
     * @param newPosition - Position to move draw item to.
     */
    void Move( const wxPoint& newPosition ) { DoMove( newPosition ); }

    /**
     * Return the current draw object start position.
     */
    wxPoint GetPosition( void ) { return DoGetPosition(); }

    /**
     * Mirror the draw object along the horizontal (X) axis about a point.
     *
     * @param center - Point to mirror around.
     */
    void MirrorHorizontal( const wxPoint& center )
    {
        DoMirrorHorizontal( center );
    }

    /**
     * Plot the draw item using the plot object.
     *
     * @param plotter - The plot object to plot to.
     */
    void Plot( PLOTTER* plotter, const wxPoint& offset, bool fill,
               const int transform[2][2] )
    {
        DoPlot( plotter, offset, fill, transform );
    }

    /**
     * Return the width of the draw item.
     *
     * @return int - Width of draw object.
     */
    int GetWidth( void ) { return DoGetWidth(); }


protected:
    virtual LIB_DRAW_ITEM* DoGenCopy() = 0;

    /**
     * Provide the draw object specific comparison.
     *
     * This is called by the == and < operators.
     *
     * The sort order is as follows:
     *      - Component alternate part (DeMorgan) number.
     *      - Component part number.
     *      - KICAD_T enum value.
     *      - Result of derived classes comparison.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& other ) const = 0;
    virtual void DoOffset( const wxPoint& offset ) = 0;
    virtual bool DoTestInside( EDA_Rect& rect ) = 0;
    virtual void DoMove( const wxPoint& newPosition ) = 0;
    virtual wxPoint DoGetPosition( void ) = 0;
    virtual void DoMirrorHorizontal( const wxPoint& center ) = 0;
    virtual void DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                         const int transform[2][2] ) = 0;
    virtual int DoGetWidth( void ) = 0;
};


/**
 * Helper for defining a list of library draw object pointers.
 */
typedef boost::ptr_vector< LIB_DRAW_ITEM > LIB_DRAW_ITEM_LIST;


/********/
/* Pins */
/********/
class LIB_PIN : public LIB_DRAW_ITEM
{
public:
    int      m_PinLen;      /* Pin length */
    int      m_Orient;      /* Pin orientation (Up, Down, Left, Right) */
    int      m_PinShape;    /* Bitwise ORed: Pin shape (see enum DrawPinShape) */
    int      m_PinType;     /* Electrical pin properties */
    int      m_Attributs;   /* bit 0 != 0: pin invisible */
    long     m_PinNum;      /* Pin number: 4 ASCII code like "12" or "anod"
                             * or "G6" "12" is stored as "12\0\0" ans does not
                             * depend on endian type*/
    wxString m_PinName;
    int      m_PinNumSize;
    int      m_PinNameSize; /* Pin num and Pin name sizes */

    /* (Currently Unused) Pin num and Pin name text options: italic/normal
     * /bold, 0 = default */
    char     m_PinNumShapeOpt;
    char     m_PinNameShapeOpt;
    // (Currently Unused) Pin num and Pin name text opt position, 0 = default:
    char     m_PinNumPositionOpt;
    char     m_PinNamePositionOpt;

    wxPoint  m_Pos;         /* Position or centre (Arc and Circle) or start
                             * point (segments) */
    int      m_Width;       /* Line width */

public:
    LIB_PIN(LIB_COMPONENT * aParent);
    LIB_PIN( const LIB_PIN& pin );
    ~LIB_PIN() { }

    LIB_PIN* Next() const { return (LIB_PIN*) Pnext; }
    LIB_PIN* Back() const { return (LIB_PIN*) Pback; }

    virtual wxString GetClass() const
    {
        return wxT( "LIB_PIN" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const;
    virtual bool Load( char* line, wxString& errorMsg );


    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param aRefPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

    /** Function HitTest
     * @return true if the point aPosRef is near this object
     * @param aPosRef = a wxPoint to test
     * @param aThreshold = max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* frame );
    virtual EDA_Rect GetBoundingBox();
    wxPoint ReturnPinEndPoint();

    int ReturnPinDrawOrient( const int TransMat[2][2] );

    /** Function ReturnPinStringNum
     * fill a buffer with pin num as a wxString
     *  Pin num is coded as a long or 4 ASCII chars
     *  Used to print/draw the pin num
     * @param aStringBuffer = the wxString to store the pin num as an unicode
     *                         string
     */
    void         ReturnPinStringNum( wxString& aStringBuffer ) const;

    /** Function ReturnPinStringNum (static function)
     * Pin num is coded as a long or 4 ascii chars
     * @param aPinNum = a long containing a pin num
     * @return aStringBuffer = the wxString to store the pin num as an
     *                         unicode string
     */
    static wxString ReturnPinStringNum( long aPinNum );

    void         SetPinNumFromString( wxString& buffer );

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    void DrawPinSymbol( WinEDA_DrawPanel* panel, wxDC* DC,
                        const wxPoint& pin_pos, int orient,
                        int DrawMode, int Color = -1 );

    void DrawPinTexts( WinEDA_DrawPanel* panel, wxDC* DC,
                       wxPoint& pin_pos, int orient,
                       int TextInside, bool DrawPinNum,
                       bool DrawPinName, int Color, int DrawMode );

    void PlotPinTexts( PLOTTER *plotter,
                       wxPoint& pin_pos,
                       int      orient,
                       int      TextInside,
                       bool     DrawPinNum,
                       bool     DrawPinNameint,
                       int      aWidth );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the pin draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Pin number.
     *      - Pin name, case insensitive compare.
     *      - Pin horizontal (X) position.
     *      - Pin vertical (Y) position.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& other ) const;
    virtual void DoOffset( const wxPoint& offset );
    virtual bool DoTestInside( EDA_Rect& rect );
    virtual void DoMove( const wxPoint& newPosition );
    virtual wxPoint DoGetPosition( void ) { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& center );
    virtual void DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                         const int transform[2][2] );
    virtual int DoGetWidth( void ) { return m_Width; }
};


/**************************/
/* Graphic Body Item: Arc */
/**************************/

class LIB_ARC : public LIB_DRAW_ITEM
{
public:
    int     m_Radius;
    int     m_t1;
    int     m_t2;       /* position des 2 extremites de l'arc en 0.1 degres */
    wxPoint m_ArcStart;
    wxPoint m_ArcEnd;   /* position des 2 extremites de l'arc en coord reelles*/
    wxPoint m_Pos;      /* Position or centre (Arc and Circle) or start point
                         * (segments) */
    int     m_Width;    /* Line width */

public:
    LIB_ARC(LIB_COMPONENT * aParent);
    LIB_ARC( const LIB_ARC& arc );
    ~LIB_ARC() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_ARC" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const;
    virtual bool Load( char* line, wxString& errorMsg );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param aRefPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

     /** Function HitTest
     * @return true if the point aPosRef is near this object
     * @param aPosRef = a wxPoint to test
     * @param aThreshold = max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual EDA_Rect GetBoundingBox();
    virtual void DisplayInfo( WinEDA_DrawFrame* frame );
    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the arc draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Arc horizontal (X) position.
     *      - Arc vertical (Y) position.
     *      - Arc start angle.
     *      - Arc end angle.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& other ) const;
    virtual void DoOffset( const wxPoint& offset );
    virtual bool DoTestInside( EDA_Rect& rect );
    virtual void DoMove( const wxPoint& newPosition );
    virtual wxPoint DoGetPosition( void ) { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& center );
    virtual void DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                         const int transform[2][2] );
    virtual int DoGetWidth( void ) { return m_Width; }
};


/*****************************/
/* Graphic Body Item: Circle */
/*****************************/
class LIB_CIRCLE : public LIB_DRAW_ITEM
{
public:
    int     m_Radius;
    wxPoint m_Pos;    /* Position or centre (Arc and Circle) or start
                       * point (segments) */
    int     m_Width;  /* Line width */

public:
    LIB_CIRCLE(LIB_COMPONENT * aParent);
    LIB_CIRCLE( const LIB_CIRCLE& circle );
    ~LIB_CIRCLE() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_CIRCLE" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const;
    virtual bool Load( char* line, wxString& errorMsg );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param aRefPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

     /** Function HitTest
     * @return true if the point aPosRef is near this object
     * @param aPosRef = a wxPoint to test
     * @param aThreshold = max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual EDA_Rect GetBoundingBox();
    virtual void DisplayInfo( WinEDA_DrawFrame* frame );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the circle draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Circle horizontal (X) position.
     *      - Circle vertical (Y) position.
     *      - Circle radius.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& other ) const;

    virtual void DoOffset( const wxPoint& offset );
    virtual bool DoTestInside( EDA_Rect& rect );
    virtual void DoMove( const wxPoint& newPosition );
    virtual wxPoint DoGetPosition( void ) { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& center );
    virtual void DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                         const int transform[2][2] );
    virtual int DoGetWidth( void ) { return m_Width; }
};


/*********************************************/
/* Graphic Body Item: Text                   */
/* This is only a graphic text.              */
/* Fields like Ref , value... are not Text,  */
/* they are a separate class                 */
/*********************************************/
class LIB_TEXT : public LIB_DRAW_ITEM, public EDA_TextStruct
{
public:
    LIB_TEXT(LIB_COMPONENT * aParent);
    LIB_TEXT( const LIB_TEXT& text );
    ~LIB_TEXT() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_TEXT" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const;
    virtual bool Load( char* line, wxString& errorMsg );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& refPos );

     /** Function HitTest
     * @return true if the point aPosRef is near a segment
     * @param aPosRef = a wxPoint to test, in eeschema coordinates
     * @param aThreshold = max distance to a segment
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    /**
     * Function HitTest (overlayed)
     * tests if the given EDA_Rect intersect this object.
     * For now, an ending point must be inside this rect.
     * @param refArea : the given EDA_Rect
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( EDA_Rect& refArea )
    {
        return TextHitTest( refArea );
    }

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* frame );

    virtual EDA_Rect GetBoundingBox();

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the text draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Text string, case insensitive compare.
     *      - Text horizontal (X) position.
     *      - Text vertical (Y) position.
     *      - Text width.
     *      - Text height.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& other ) const;

    virtual void DoOffset( const wxPoint& offset );
    virtual bool DoTestInside( EDA_Rect& rect );
    virtual void DoMove( const wxPoint& newPosition );
    virtual wxPoint DoGetPosition( void ) { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& center );
    virtual void DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                         const int transform[2][2] );
    virtual int DoGetWidth( void ) { return m_Width; }
};


/********************************/
/* Graphic Body Item: Rectangle */
/********************************/
class LIB_RECTANGLE  : public LIB_DRAW_ITEM
{
public:
    wxPoint m_End;     /* Rectangle end point. */
    wxPoint m_Pos;     /* Rectangle start point. */
    int     m_Width;   /* Line width */

public:
    LIB_RECTANGLE(LIB_COMPONENT * aParent);
    LIB_RECTANGLE( const LIB_RECTANGLE& rect );
    ~LIB_RECTANGLE() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_RECTANGLE" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     *  format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const;
    virtual bool Load( char* line, wxString& errorMsg );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param aRefPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

    /** Function HitTest
     * @return true if the point aPosRef is near this object
     * @param aPosRef = a wxPoint to test
     * @param aThreshold = max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual EDA_Rect GetBoundingBox();
    virtual void DisplayInfo( WinEDA_DrawFrame* frame );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the rectangle draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Rectangle horizontal (X) start position.
     *      - Rectangle vertical (Y) start position.
     *      - Rectangle horizontal (X) end position.
     *      - Rectangle vertical (Y) end position.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& other ) const;

    virtual void DoOffset( const wxPoint& offset );
    virtual bool DoTestInside( EDA_Rect& rect );
    virtual void DoMove( const wxPoint& newPosition );
    virtual wxPoint DoGetPosition( void ) { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& center );
    virtual void DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                         const int transform[2][2] );
    virtual int DoGetWidth( void ) { return m_Width; }
};

/**********************************/
/* Graphic Body Item: single line */
/**********************************/
class LIB_SEGMENT  : public LIB_DRAW_ITEM
{
public:
    wxPoint m_End;
    wxPoint m_Pos;      /* Position or centre (Arc and Circle) or start point
                         * (segments) */
    int     m_Width;    /* Line width */

public:
    LIB_SEGMENT(LIB_COMPONENT * aParent);
    LIB_SEGMENT( const LIB_SEGMENT& segment );
    ~LIB_SEGMENT() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_SEGMENT" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const;
    virtual bool Load( char* line, wxString& errorMsg );

     /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param aRefPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

    /** Function HitTest
     * @return true if the point aPosRef is near this object
     * @param aPosRef = a wxPoint to test
     * @param aThreshold = max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* frame );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the line segment draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Line segment horizontal (X) start position.
     *      - Line segment vertical (Y) start position.
     *      - Line segment horizontal (X) end position.
     *      - Line segment vertical (Y) end position.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& other ) const;

    virtual void DoOffset( const wxPoint& offset );
    virtual bool DoTestInside( EDA_Rect& rect );
    virtual void DoMove( const wxPoint& newPosition );
    virtual wxPoint DoGetPosition( void ) { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& center );
    virtual void DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                         const int transform[2][2] );
    virtual int DoGetWidth( void ) { return m_Width; }
};


/**********************************************************/
/* Graphic Body Item: Polygon and polyline (set of lines) */
/**********************************************************/
class LIB_POLYLINE : public LIB_DRAW_ITEM
{
public:
    int m_Width;                            /* Line width */
    std::vector<wxPoint> m_PolyPoints;      // list of points (>= 2)

public:
    LIB_POLYLINE(LIB_COMPONENT * aParent);
    LIB_POLYLINE( const LIB_POLYLINE& polyline );
    ~LIB_POLYLINE() { }

    virtual wxString GetClass() const
    {
        return wxT( "LIB_POLYLINE" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const;
    virtual bool Load( char* line, wxString& errorMsg );

    void AddPoint( const wxPoint& point );

    /** Function GetCornerCount
     * @return the number of corners
     */
    unsigned GetCornerCount() const { return m_PolyPoints.size(); }

     /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param aRefPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

    /** Function HitTest
     * @return true if the point aPosRef is near a segment
     * @param aPosRef = a wxPoint to test
     * @param aThreshold = max distance to a segment
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    /** Function GetBoundingBox
     * @return the boundary box for this, in library coordinates
     */
    virtual EDA_Rect GetBoundingBox();

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* frame );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the ployline segment draw object specific comparison.
     *
     * The sort order for each polyline segment point is as follows:
     *      - Line segment point horizontal (X) position.
     *      - Line segment point vertical (Y) position.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& other ) const;

    virtual void DoOffset( const wxPoint& offset );
    virtual bool DoTestInside( EDA_Rect& rect );
    virtual void DoMove( const wxPoint& newPosition );
    virtual wxPoint DoGetPosition( void ) { return m_PolyPoints[0]; }
    virtual void DoMirrorHorizontal( const wxPoint& center );
    virtual void DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                         const int transform[2][2] );
    virtual int DoGetWidth( void ) { return m_Width; }
};

/**********************************************************/
/* Graphic Body Item: Bezier Curve (set of lines) */
/**********************************************************/
class LIB_BEZIER : public LIB_DRAW_ITEM
{
public:
    int m_Width;                            /* Line width */
    std::vector<wxPoint> m_BezierPoints;      // list of parameter (3|4)
    std::vector<wxPoint> m_PolyPoints;      // list of points (>= 2)

public:
    LIB_BEZIER( LIB_COMPONENT * aParent );
    LIB_BEZIER( const LIB_BEZIER& bezier );
    ~LIB_BEZIER() { }

    virtual wxString GetClass() const
    {
        return wxT( "LIB_BEZIER" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const;
    virtual bool Load( char* line, wxString& errorMsg );

    void         AddPoint( const wxPoint& point );

    /** Function GetCornerCount
     * @return the number of corners
     */
    unsigned GetCornerCount() const { return m_PolyPoints.size(); }

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param aRefPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

    /** Function HitTest
     * @return true if the point aPosRef is near a segment
     * @param aPosRef = a wxPoint to test
     * @param aThreshold = max distance to a segment
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    /** Function GetBoundingBox
     * @return the boundary box for this, in library coordinates
     */
    virtual EDA_Rect GetBoundingBox();

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* frame );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the bezier curve draw object specific comparison.
     *
     * The sort order for each bezier curve segment point is as follows:
     *      - Bezier point horizontal (X) point position.
     *      - Bezier point vertical (Y) point position.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& other ) const;

    virtual void DoOffset( const wxPoint& offset );
    virtual bool DoTestInside( EDA_Rect& rect );
    virtual void DoMove( const wxPoint& newPosition );
    virtual wxPoint DoGetPosition( void ) { return m_PolyPoints[0]; }
    virtual void DoMirrorHorizontal( const wxPoint& center );
    virtual void DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                         const int transform[2][2] );
    virtual int DoGetWidth( void ) { return m_Width; }
};

#endif  //  CLASSES_BODY_ITEMS_H
