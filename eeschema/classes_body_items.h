/****************************************************************/
/*	Headers fo library definition and lib component definitions */
/****************************************************************/

/* Definitions of graphic items used to create shapes in component libraries.
 */
#ifndef CLASSES_BODY_ITEMS_H
#define CLASSES_BODY_ITEMS_H

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
enum ElectricPinType {
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
    int    m_Unit;       /* Unit identification (for multi part per parkage)
                          * 0 if the item is common to all units */
    int    m_Convert;    /* Shape identification (for parts which have a convert
                          * shape) 0 if the item is common to all shapes */
    FILL_T m_Fill;       /* NO_FILL, FILLED_SHAPE or FILLED_WITH_BG_BODYCOLOR.
                          * has meaning only for some items */
    wxString m_typeName; /* Name of object displayed in the message panel. */

public:
    LibEDA_BaseStruct* Next()
    {
        return (LibEDA_BaseStruct*) Pnext;
    }


    LibEDA_BaseStruct( KICAD_T struct_type, EDA_LibComponentStruct * aParent );
    virtual ~LibEDA_BaseStruct() { }

    /** Function Draw (virtual pure)
     * Draw A body item
     * @param aPanel = DrawPanel to use (can be null) mainly used for clipping
     *                 purposes
     * @param aDC = Device Context (can be null)
     * @param aOffset = offset to draw
     * @param aColor = -1 to use the normal body item color, or use this color
     *                 if >= 0
     * @param aDrawMode = GR_OR, GR_XOR, ...
     * @param aData = value or pointer used to pass others parametres,
     *                depending on body items. used for some items to force
     *                to force no fill mode ( has meaning only for items what
     *                can be filled ). used in printing or moving objects mode
     *                or to pass refernce to the lib component for pins
     * @param aTransformMatrix = Transform Matrix (rotaion, mirror ..)
     */
    virtual void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC,
                       const wxPoint &aOffset, int aColor, int aDrawMode,
                       void* aData, const int aTransformMatrix[2][2] ) = 0;

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) const = 0;
    virtual bool Load( char* line, wxString& errorMsg ) = 0;

    EDA_LibComponentStruct * GetParent()
    {
        return (EDA_LibComponentStruct *)m_Parent;
    }

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool    HitTest( const wxPoint& refPos )
    {
        return false;   // derived classes should override this function
    }

    /** Function HitTest (overlayed)
     * @return true if the point aPosRef is near this object
     * @param aPosRef = a wxPoint to test
     * @param aThreshold = max distance to this object (usually the half thickness of a line)
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] ) = 0;

   /** Function GetBoundingBox
     * @return the boundary box for this, in library coordinates
     */
    virtual EDA_Rect GetBoundingBox()
    {
        return EDA_BaseStruct::GetBoundingBox();
    }

    virtual void DisplayInfo( WinEDA_DrawFrame* frame );
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
    long     m_PinNum;      /* Pin number: 4 Ascii code like "12" or "anod"
                             * or "G6" "12" is stored as "12\0\0" ans does not
                             * depend on endian type*/
    wxString m_PinName;
    int      m_PinNumSize;
    int      m_PinNameSize; /* Pin num and Pin name sizes */

	// (Currently Unused) Pin num and Pin name text opt: italic/normal/bold, 0 = default:
    char m_PinNumShapeOpt, m_PinNameShapeOpt;
	// (Currently Unused) Pin num and Pin name text opt position, 0 = default:
    char m_PinNumPositionOpt, m_PinNamePositionOpt;

wxPoint  m_Pos;         /* Position or centre (Arc and Circle) or start
                             * point (segments) */
    int      m_Width;       /* Line width */

public:
    LibDrawPin(EDA_LibComponentStruct * aParent);
    ~LibDrawPin() { }

    LibDrawPin* Next() const { return (LibDrawPin*) Pnext; }
    LibDrawPin* Back() const { return (LibDrawPin*) Pback; }

    virtual wxString GetClass() const
    {
        return wxT( "LibDrawPin" );
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
     * @param aThreshold = max distance to this object (usually the half thickness of a line)
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] );

    LibDrawPin*  GenCopy();
    virtual void DisplayInfo( WinEDA_DrawFrame* frame );
    virtual EDA_Rect GetBoundingBox();
    wxPoint      ReturnPinEndPoint();

    int ReturnPinDrawOrient( const int TransMat[2][2] );
    void         ReturnPinStringNum( wxString& buffer ) const;
    void         SetPinNumFromString( wxString& buffer );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    void         DrawPinSymbol( WinEDA_DrawPanel* panel, wxDC* DC,
                                const wxPoint& pin_pos, int orient,
                                int DrawMode, int Color = -1 );

    void         DrawPinTexts( WinEDA_DrawPanel* panel, wxDC* DC,
                               wxPoint& pin_pos, int orient,
                               int TextInside, bool DrawPinNum,
                               bool DrawPinName, int Color, int DrawMode );
    void         PlotPinTexts( Plotter *plotter,
			       wxPoint& pin_pos,
                               int      orient,
                               int      TextInside,
                               bool     DrawPinNum,
                               bool     DrawPinNameint,
                               int      aWidth);
};


/**************************/
/* Graphic Body Item: Arc */
/**************************/

class LibDrawArc : public LibEDA_BaseStruct
{
public:
    int     m_Rayon;
    int     t1, t2;     /* position des 2 extremites de l'arc en 0.1 degres */
    wxPoint m_ArcStart;
    wxPoint m_ArcEnd;   /* position des 2 extremites de l'arc en coord reelles*/
    wxPoint m_Pos;      /* Position or centre (Arc and Circle) or start point
                         * (segments) */
    int     m_Width;    /* Line width */

public:
    LibDrawArc(EDA_LibComponentStruct * aParent);
    ~LibDrawArc() { }
    virtual wxString GetClass() const
    {
        return wxT( "LibDrawArc" );
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
     * @param aThreshold = max distance to this object (usually the half thickness of a line)
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] );

    LibDrawArc*  GenCopy();

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual EDA_Rect GetBoundingBox();
    virtual void DisplayInfo( WinEDA_DrawFrame* frame );
};


/*****************************/
/* Graphic Body Item: Circle */
/*****************************/
class LibDrawCircle : public LibEDA_BaseStruct
{
public:
    int     m_Rayon;
    wxPoint m_Pos;    /* Position or centre (Arc and Circle) or start
                       * point (segments) */
    int     m_Width;  /* Line width */

public:
    LibDrawCircle(EDA_LibComponentStruct * aParent);
    ~LibDrawCircle() { }
    virtual wxString GetClass() const
    {
        return wxT( "LibDrawCircle" );
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
     * @param aThreshold = max distance to this object (usually the half thickness of a line)
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] );

    LibDrawCircle* GenCopy();

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual EDA_Rect GetBoundingBox();
    virtual void DisplayInfo( WinEDA_DrawFrame* frame );
};


/*********************************************/
/* Graphic Body Item: Text                   */
/* This is only a graphic text.              */
/* Fields like Ref , value... are not Text,  */
/* they are a separate class                 */
/*********************************************/
class LibDrawText : public LibEDA_BaseStruct, public EDA_TextStruct
{
public:
    LibDrawText(EDA_LibComponentStruct * aParent);
    ~LibDrawText() { }
    virtual wxString GetClass() const
    {
        return wxT( "LibDrawText" );
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
    virtual bool    HitTest( const wxPoint& refPos );

     /** Function HitTest
     * @return true if the point aPosRef is near a segment
     * @param aPosRef = a wxPoint to test, in eeschema coordinates
     * @param aThreshold = max distance to a segment
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] );

    /**
     * Function HitTest (overlayed)
     * tests if the given EDA_Rect intersect this object.
     * For now, an ending point must be inside this rect.
     * @param refArea : the given EDA_Rect
     * @return bool - true if a hit, else false
     */
    virtual bool    HitTest( EDA_Rect& refArea )
    {
        return TextHitTest( refArea );
    }

    LibDrawText* GenCopy();

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* frame );
};


/********************************/
/* Graphic Body Item: Rectangle */
/********************************/
class LibDrawSquare  : public LibEDA_BaseStruct
{
public:
    wxPoint m_End;     /* Rectangle end point. */
    wxPoint m_Pos;     /* Rectangle start point. */
    int     m_Width;   /* Line width */

public:
    LibDrawSquare(EDA_LibComponentStruct * aParent);
    ~LibDrawSquare() { }
    virtual wxString GetClass() const
    {
        return wxT( "LibDrawSquare" );
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
     * @param aThreshold = max distance to this object (usually the half thickness of a line)
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] );

    LibDrawSquare* GenCopy();

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual EDA_Rect GetBoundingBox();
    virtual void DisplayInfo( WinEDA_DrawFrame* frame );
};

/**********************************/
/* Graphic Body Item: single line */
/**********************************/
class LibDrawSegment  : public LibEDA_BaseStruct
{
public:
    wxPoint m_End;
    wxPoint m_Pos;      /* Position or centre (Arc and Circle) or start point
                         * (segments) */
    int     m_Width;    /* Line width */

public:
    LibDrawSegment(EDA_LibComponentStruct * aParent);
    ~LibDrawSegment() { }
    virtual wxString GetClass() const
    {
        return wxT( "LibDrawSegment" );
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
     * @param aThreshold = max distance to this object (usually the half thickness of a line)
     * @param aTransMat = the transform matrix
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] );

    LibDrawSegment* GenCopy();

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* frame );
};


/**********************************************************/
/* Graphic Body Item: Polygon and polyline (set of lines) */
/**********************************************************/
class LibDrawPolyline : public LibEDA_BaseStruct
{
public:
    int m_Width;                            /* Line width */
    std::vector<wxPoint> m_PolyPoints;      // list of points (>= 2)

public:
    LibDrawPolyline(EDA_LibComponentStruct * aParent);
    ~LibDrawPolyline() { }

    virtual wxString GetClass() const
    {
        return wxT( "LibDrawPolyline" );
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

    LibDrawPolyline* GenCopy();
    void             AddPoint( const wxPoint& point );

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
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] );

    /** Function GetBoundingBox
     * @return the boundary box for this, in library coordinates
     */
    virtual EDA_Rect GetBoundingBox();

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* frame );
};

/**********************************************************/
/* Graphic Body Item: Bezier Curve (set of lines) */
/**********************************************************/
class LibDrawBezier : public LibEDA_BaseStruct
	{
	public:
		int m_Width;                            /* Line width */
		std::vector<wxPoint> m_BezierPoints;      // list of parameter (3|4)
		std::vector<wxPoint> m_PolyPoints;      // list of points (>= 2)
		
	public:
		LibDrawBezier(EDA_LibComponentStruct * aParent);
		~LibDrawBezier() { }
		
		virtual wxString GetClass() const
		{
			return wxT( "LibDrawBezier" );
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
		
		LibDrawBezier* GenCopy();
		void             AddPoint( const wxPoint& point );
		
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
		virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] );
		
		/** Function GetBoundingBox
		 * @return the boundary box for this, in library coordinates
		 */
		virtual EDA_Rect GetBoundingBox();
		
		void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
				  int aColor, int aDrawMode, void* aData,
				  const int aTransformMatrix[2][2] );
		
		virtual void DisplayInfo( WinEDA_DrawFrame* frame );
	};

#endif  //  CLASSES_BODY_ITEMS_H
