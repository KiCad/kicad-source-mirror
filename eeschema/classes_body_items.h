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
class LIB_PIN;


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
 * The component library pin object electrical types used in ERC tests.
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

/* Electrical pin type names. */
extern const wxChar* MsgPinElectricType[];

/* Pin visibility flag bit. */
#define PINNOTDRAW 1    /* Set makes pin invisible */


/**
 * The component library pin object drawing shapes.
 */
enum DrawPinShape {
    NONE         = 0,
    INVERT       = 1,
    CLOCK        = 2,
    LOWLEVEL_IN  = 4,
    LOWLEVEL_OUT = 8
};


/**
 *  The component library pin object orientations.
 */
enum DrawPinOrient {
    PIN_RIGHT = 'R',
    PIN_LEFT  = 'L',
    PIN_UP    = 'U',
    PIN_DOWN  = 'D',
};


/**
 * Helper for defining a list of library draw object pointers.  The Boost
 * pointer containers are responsible for deleting object pointers placed
 * in them.  If you access a object pointer from the list, do not delete
 * it directly.
 */
typedef boost::ptr_vector< LIB_DRAW_ITEM > LIB_DRAW_ITEM_LIST;


/**
 * Helper for defining a list of pin object pointers.  The list does not
 * use a Boost pointer class so the ojbect pointers do not accidently get
 * deleted when the container is deleted.
 */
typedef std::vector< LIB_PIN* > LIB_PIN_LIST;


/****************************************************************************/
/* Classes for handle the body items of a component: pins add graphic items */
/****************************************************************************/


/**
 * Base class for drawable items used in library components.
 *  (graphic shapes, texts, fields, pins)
 */
class LIB_DRAW_ITEM : public EDA_BaseStruct
{
public:
    /**
     * Unit identification for multiple parts per package.  Set to 0 if the
     * item is common to all units.
     */
    int      m_Unit;

    /**
     * Shape identification for alternate body styles.  Set 0 if the item
     * is common to all body styles.  This is commonly referred to as
     * DeMorgan style and this is typically how it is used in Kicad.
     */
    int      m_Convert;
    FILL_T   m_Fill;     /* NO_FILL, FILLED_SHAPE or FILLED_WITH_BG_BODYCOLOR.
                          * has meaning only for some items */
    wxString m_typeName; /* Name of object displayed in the message panel. */

public:
    LIB_DRAW_ITEM* Next()
    {
        return (LIB_DRAW_ITEM*) Pnext;
    }


    LIB_DRAW_ITEM( KICAD_T aType, LIB_COMPONENT * aParent );
    LIB_DRAW_ITEM( const LIB_DRAW_ITEM& aItem );
    virtual ~LIB_DRAW_ITEM() { }

    /**
     * Draw a body item
     *
     * @param aPanel - DrawPanel to use (can be null) mainly used for clipping
     *                 purposes
     * @param aDC - Device Context (can be null)
     * @param aOffset - offset to draw
     * @param aColor - -1 to use the normal body item color, or use this color
     *                 if >= 0
     * @param aDrawMode - GR_OR, GR_XOR, ...
     * @param aData - value or pointer used to pass others parameters,
     *                depending on body items. used for some items to force
     *                to force no fill mode ( has meaning only for items what
     *                can be filled ). used in printing or moving objects mode
     *                or to pass reference to the lib component for pins
     * @param aTransformMatrix - Transform Matrix (rotation, mirror ..)
     */
    virtual void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC,
                       const wxPoint &aOffset, int aColor, int aDrawMode,
                       void* aData, const int aTransformMatrix[2][2] ) = 0;

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() = 0;

    /**
     * Write draw item object to /a aFile in "*.lib" format.
     *
     * @param aFile - The file to write to.
     * @param aErrorMsg - Error message if write fails.
     * @return - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) = 0;
    virtual bool Load( char* aLine, wxString& aErrorMsg ) = 0;

    LIB_COMPONENT* GetParent()
    {
        return (LIB_COMPONENT *)m_Parent;
    }

    /**
     * Tests if the given point is within the bounds of this object.
     *
     * Derived classes should override this function.
     *
     * @param aPosition - The coordinats to test.
     * @return - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aPosition )
    {
        return false;
    }

    /**
     * @param aPosRef - a wxPoint to test
     * @param aThreshold - max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat - the transform matrix
     * @return - true if the point aPosRef is near this object
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] ) = 0;

   /**
     * @return the boundary box for this, in library coordinates
     */
    virtual EDA_Rect GetBoundingBox()
    {
        return EDA_BaseStruct::GetBoundingBox();
    }

    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

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
     * @param aOther - Object to test against.
     * @return - True if object is identical to this object.
     */
    bool operator==( const LIB_DRAW_ITEM& aOther ) const;
    bool operator==( const LIB_DRAW_ITEM* aOther ) const
    {
        return *this == *aOther;
    }

    /**
     * Test if another draw item is less than this draw object.
     *
     * @param aOther - Draw item to compare against.
     * @return - True if object is less than this object.
     */
    bool operator<( const LIB_DRAW_ITEM& aOther) const;

    /**
     * Set drawing object offset from the current position.
     *
     * @param aOffset - Cooridinates to offset position.
     */
    void SetOffset( const wxPoint& aOffset ) { DoOffset( aOffset ); }

    /**
     * Test if any part of the draw object is inside rectangle bounds.
     *
     * This is used for block selection.  The real work is done by the
     * DoTestInside method for each derived object type.
     *
     * @param aRect - Rectangle to check against.
     * @return - True if object is inside rectangle.
     */
    bool Inside( EDA_Rect& aRect ) { return DoTestInside( aRect ); }

    /**
     * Move a draw object to a new /a aPosition.
     *
     * The real work is done by the DoMove method for each derived object type.
     *
     * @param aPosition - Position to move draw item to.
     */
    void Move( const wxPoint& aPosition ) { DoMove( aPosition ); }

    /**
     * Return the current draw object start position.
     */
    wxPoint GetPosition() { return DoGetPosition(); }

    /**
     * Mirror the draw object along the horizontal (X) axis about a point.
     *
     * @param aCenter - Point to mirror around.
     */
    void MirrorHorizontal( const wxPoint& aCenter )
    {
        DoMirrorHorizontal( aCenter );
    }

    /**
     * Plot the draw item using the plot object.
     *
     * @param aPlotter - The plot object to plot to.
     * @param aOffset - Plot offset position.
     * @param aFill - Flag to indicate whether or not the object is filled.
     * @param aTransform - The plot transform.
     */
    void Plot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
               const int aTransform[2][2] )
    {
        DoPlot( aPlotter, aOffset, aFill, aTransform );
    }

    /**
     * Return the width of the draw item.
     *
     * @return Width of draw object.
     */
    int GetWidth() { return DoGetWidth(); }
    void SetWidth( int aWidth ) { DoSetWidth( aWidth ); }

    /**
     * Check if draw object can be filled.
     *
     * The default setting is false.  If the derived object support filling,
     * set the m_isFillable member to true.
     *
     * @return - True if draw object can be fill.  Default is false.
     */
    bool IsFillable() { return m_isFillable; }

    /**
     * Return the modified status of the draw object.
     *
     * @return - True if the draw object has been modified.
     */
    bool IsModified() { return ( m_Flags & IS_CHANGED ) != 0; }

    /**
     * Return the new item status of the draw object.
     *
     * @return - True if the draw item has been added to the parent component.
     */
    bool IsNew() { return ( m_Flags & IS_NEW ) != 0; }

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
    virtual int DoCompare( const LIB_DRAW_ITEM& aOther ) const = 0;
    virtual void DoOffset( const wxPoint& aOffset ) = 0;
    virtual bool DoTestInside( EDA_Rect& aRect ) = 0;
    virtual void DoMove( const wxPoint& aPosition ) = 0;
    virtual wxPoint DoGetPosition() = 0;
    virtual void DoMirrorHorizontal( const wxPoint& aCenter ) = 0;
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const int aTransform[2][2] ) = 0;
    virtual int DoGetWidth() = 0;
    virtual void DoSetWidth( int aWidth ) = 0;

    /** Flag to indicate if draw item is fillable.  Default is false. */
    bool m_isFillable;
};


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
    LIB_PIN( const LIB_PIN& aPin );
    ~LIB_PIN() { }

    LIB_PIN* Next() const { return (LIB_PIN*) Pnext; }
    LIB_PIN* Back() const { return (LIB_PIN*) Pback; }

    virtual wxString GetClass() const
    {
        return wxT( "LIB_PIN" );
    }


    /**
     * Write pin object to a FILE in "*.lib" format.
     *
     * @param aFile The FILE to write to.
     * @return - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* aLine, wxString& aErrorMsg );


    /**
     * Test if the given point is within the bounds of this object.
     *
     * @param aRefPos A wxPoint to test
     * @return - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

    /**
     * @param aPosRef - a wxPoint to test
     * @param aThreshold - max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat - the transform matrix
     * @return - true if the point aPosRef is near this object
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const int aTransMat[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* frame );
    virtual EDA_Rect GetBoundingBox();
    wxPoint ReturnPinEndPoint();

    int ReturnPinDrawOrient( const int TransMat[2][2] );

    /**
     * Fill a string buffer with pin number.
     *
     * Pin numbers are coded as a long or 4 ASCII characters.  Used to print
     * or draw the pin number.
     *
     * @param aStringBuffer - the wxString to store the pin num as an unicode
     *                        string
     */
    void         ReturnPinStringNum( wxString& aStringBuffer ) const;

    wxString     GetNumber();

    /** Function ReturnPinStringNum (static function)
     * Pin num is coded as a long or 4 ascii chars
     * @param aPinNum = a long containing a pin num
     * @return aStringBuffer = the wxString to store the pin num as an
     *                         unicode string
     */
    static wxString ReturnPinStringNum( long aPinNum );

    void         SetPinNumFromString( wxString& aBuffer );

    /**
     * Set the pin name.
     *
     * This will also all of the pin names marked by EnableEditMode().
     *
     * @param name - New pin name.
     */
    void SetName( const wxString& aName );

    /**
     * Set the /a aSize of the pin name text.
     *
     * This will also update the text size of the name of the pins marked
     * by EnableEditMode().
     *
     * @param aSize - The text size of the pin name in schematic units ( mils ).
     */
    void SetNameTextSize( int aSize );

    /**
     * Set the pin number.
     *
     * This will also all of the pin numbers marked by EnableEditMode().
     *
     * @param aNumber - New pin number.
     */
    void SetNumber( const wxString& aNumber );

    /**
     * Set the size of the pin number text.
     *
     * This will also update the text size of the number of the pins marked
     * by EnableEditMode().
     *
     * @param aSize - The text size of the pin number in schematic
     *                units ( mils ).
     */
    void SetNumberTextSize( int aSize );

    /**
     * Set orientation on the pin.
     *
     * This will also update the orientation of the pins marked by
     * EnableEditMode().
     *
     * @param aOrientation - The orientation of the pin.
     */
    void SetOrientation( int aOrientation );

    /**
     * Set the draw style of the pin.
     *
     * This will also update the draw style of the pins marked by
     * EnableEditMode().
     *
     * @param aStyle - The draw style of the pin.
     */
    void SetDrawStyle( int aStyle );

    /**
     * Set the electrical type of the pin.
     *
     * This will also update the electrical type of the pins marked by
     * EnableEditMode().
     *
     * @param aType - The electrical type of the pin.
     */
    void SetElectricalType( int aType );

    /**
     * Set the pin length.
     *
     * This will also update the length of the pins marked by EnableEditMode().
     *
     * @param aLength - The length of the pin in mils.
     */
    void SetLength( int aLength );

    /**
     * Set the pin part number.
     *
     * If the pin is changed from not common to common to all parts, any
     * linked pins will be removed from the parent component.
     *
     * @param aPart - Number of the part the pin belongs to.  Set to zero to
     *                make pin common to all parts in a multi-part component.
     */
    void SetPartNumber( int aPart );

    /**
     * Set the body style (conversion) of the pin.
     *
     * If the pin is changed from not common to common to all body styles, any
     * linked pins will be removed from the parent component.
     *
     * @param conversion - Body style of the pin.  Set to zero to make pin
     *                     common to all body styles.
     */
    void SetConversion( int aConversion );

    /**
     * Set or clear the visibility flag for the pin.
     *
     * This will also update the visibility of the pins marked by
     * EnableEditMode().
     *
     * @param aVisible - True to make the pin visible or false to hide the pin.
     */
    void SetVisible( bool aVisible );

    /**
     * Enable or clear pin editing mode.
     *
     * The pin editing mode marks or unmarks all pins common to this
     * pin object for further editing.  If any of the pin modifcation
     * methods are called after enabling the editing mode, all pins
     * marked for editing will have the same attribute changed.  The
     * only case were this is not true making this pin common to all
     * parts or body styles in the component.  See SetCommonToAllParts()
     * and SetCommonToAllBodyStyles() for more information.
     *
     * @params aEnable - True marks all common pins for editing mode.  False
     *                   clears the editing mode.
     * @params aEditpinByPin - Enables the edit pin by pin mode.
     */
    void EnableEditMode( bool aEnable, bool aEditPinByPin = false );

    /**
     * Return the visibility status of the draw object.
     *
     * @return True if draw object is visible otherwise false.
     */
    bool IsVisible() { return ( m_Attributs & PINNOTDRAW ) == 0; }

    /**
     * @return the size of the "pen" that be used to draw or plot this item.
     */
    virtual int GetPenSize();

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData, const int aTransformMatrix[2][2] );

    void DrawPinSymbol( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aPosition,
                        int aOrientation, int aDrawMode, int aColor = -1 );

    void DrawPinTexts( WinEDA_DrawPanel* aPanel, wxDC* aDC, wxPoint& aPosition,
                       int aOrientation, int TextInside, bool DrawPinNum, bool DrawPinName,
                       int aColor, int aDrawMode );

    void PlotPinTexts( PLOTTER *aPlotter,
                       wxPoint& aPosition,
                       int      aOrientation,
                       int      aTextInside,
                       bool     aDrawPinNum,
                       bool     aDrawPinName,
                       int      aWidth );

    /**
     * Get a list of pin orientation names.
     *
     * @return List of valid pin orientation names.
     */
    static wxArrayString GetOrientationNames();

    /**
     * Get the orientation code by index used to set the pin orientation.
     *
     * @param aIndex - The index of the orientation code to look up.
     * @return Orientation code if index is valid.  Returns right
     *         orientation on index error.
     */
    static int GetOrientationCode( int aIndex );

    /**
     * Get the index of the orientation code.
     *
     * @param aCode - The orientation code to look up.
     * @return  The index of the orientation code if found.  Otherwise,
     *          return wxNOT_FOUND.
     */
    static int GetOrientationCodeIndex( int aCode );

    /**
     * Get a list of pin draw style names.
     *
     * @return  List of valid pin draw style names.
     */
    static wxArrayString GetStyleNames();

    /**
     * Get the pin draw style code by index used to set the pin draw style.
     *
     * @param aIndex - The index of the pin draw style code to look up.
     * @return  Pin draw style code if index is valid.  Returns NONE
     *          style on index error.
     */
    static int GetStyleCode( int aIndex );

    /**
     * Get the index of the pin draw style code.
     *
     * @param aCode - The pin draw style code to look up.
     * @return The index of the pin draw style code if found.  Otherwise,
     *         return wxNOT_FOUND.
     */
    static int GetStyleCodeIndex( int aCode );

    /**
     * Get a list of pin electrical type names.
     *
     * @return  List of valid pin electrical type names.
     */
    static wxArrayString GetElectricalTypeNames();

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
    virtual int DoCompare( const LIB_DRAW_ITEM& aOther ) const;
    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_Rect& aRect );
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const int aTransform[2][2] );
    virtual int DoGetWidth() { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
};


/**************************/
/* Graphic Body Item: Arc */
/**************************/

class LIB_ARC : public LIB_DRAW_ITEM
{
public:
    int     m_Radius;
    int     m_t1;       /* First radius angle of the arc in 0.1 degrees. */
    int     m_t2;       /* Second radius angle of the arc in 0.1 degrees. */
    wxPoint m_ArcStart;
    wxPoint m_ArcEnd;   /* Arc end position. */
    wxPoint m_Pos;      /* Radius center point. */
    int     m_Width;    /* Line width */

public:
    LIB_ARC(LIB_COMPONENT * aParent);
    LIB_ARC( const LIB_ARC& aArc );
    ~LIB_ARC() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_ARC" );
    }


    /**
     * Save arc object to a FILE in "*.lib" format.
     *
     * @param aFile The FILE to write to.
     * @return - True if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* aLine, wxString& aErrorMsg );

    /**
     * Tests if the given wxPoint is within the bounds of this object.
     *
     * @param aRefPos - Coordinates to test
     * @return - True if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

     /**
     * @param aPosRef - a wxPoint to test
     * @param aThreshold - max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat - the transform matrix
     * @return - True if the point aPosRef is near this object
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual EDA_Rect GetBoundingBox();
    virtual void DisplayInfo( WinEDA_DrawFrame* frame );
    /**
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
    virtual int DoCompare( const LIB_DRAW_ITEM& aOther ) const;
    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_Rect& aRect );
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const int aTransform[2][2] );
    virtual int DoGetWidth() { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
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
    LIB_CIRCLE( const LIB_CIRCLE& aCircle );
    ~LIB_CIRCLE() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_CIRCLE" );
    }


    /**
     * Write circle object to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* aLine, wxString& aErrorMsg );

    /**
     * Test if the given point is within the bounds of this object.
     *
     * @param aRefPos - A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

     /**
      * @param aPosRef - a wxPoint to test
      * @param aThreshold - max distance to this object (usually the half
      *                     thickness of a line)
      * @param aTransMat - the transform matrix
      * @return true if the point aPosRef is near this object
      */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual EDA_Rect GetBoundingBox();
    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

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
    virtual int DoCompare( const LIB_DRAW_ITEM& aOther ) const;

    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_Rect& aRect );
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const int aTransform[2][2] );
    virtual int DoGetWidth() { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
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
    LIB_TEXT( const LIB_TEXT& aText );
    ~LIB_TEXT() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_TEXT" );
    }


    /**
     * Write text object out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* aLine, wxString& aErrorMsg );

    /**
     * Test if the given point is within the bounds of this object.
     *
     * @param refPos - A wxPoint to test
     * @return - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& refPos );

     /**
      * @param aPosRef = a wxPoint to test, in eeschema coordinates
      * @param aThreshold = max distance to a segment
      * @param aTransMat = the transform matrix
      * @return true if the point aPosRef is near a segment
      */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    /**
     * Test if the given rectangle intersects this object.
     *
     * For now, an ending point must be inside this rect.
     *
     * @param aRect - the given EDA_Rect
     * @return - true if a hit, else false
     */
    virtual bool HitTest( EDA_Rect& aRect )
    {
        return TextHitTest( aRect );
    }

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

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
    virtual int DoCompare( const LIB_DRAW_ITEM& aOther ) const;

    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_Rect& aRect );
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const int aTransform[2][2] );
    virtual int DoGetWidth() { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
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
    LIB_RECTANGLE( const LIB_RECTANGLE& aRect );
    ~LIB_RECTANGLE() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_RECTANGLE" );
    }


    /**
     * Write rectangle object out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* aLine, wxString& aErrorMsg );

    /**
     * Test if the given point is within the bounds of this object.
     *
     * @param aRefPos - A wxPoint to test
     * @return - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

    /**
     * @param aPosRef - a wxPoint to test
     * @param aThreshold - max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat - the transform matrix
     * @return true if the point aPosRef is near this object
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual EDA_Rect GetBoundingBox();
    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

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
    virtual int DoCompare( const LIB_DRAW_ITEM& aOther ) const;

    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_Rect& aRect );
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const int aTransform[2][2] );
    virtual int DoGetWidth() { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
};


/**********************************/
/* Graphic Body Item: single line */
/**********************************/
class LIB_SEGMENT  : public LIB_DRAW_ITEM
{
public:
    wxPoint m_End;
    wxPoint m_Pos;      /* Segment start point */
    int     m_Width;    /* Line width */

public:
    LIB_SEGMENT(LIB_COMPONENT * aParent);
    LIB_SEGMENT( const LIB_SEGMENT& aSegment );
    ~LIB_SEGMENT() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_SEGMENT" );
    }


    /**
     * Writes segment object out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* aLine, wxString& aErrorMsg );

     /**
     * Test if the given point is within the bounds of this object.
     *
     * @param aRefPos - A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

    /**
     * @param aPosRef = a wxPoint to test
     * @param aThreshold = max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat = the transform matrix
     * @return true if the point aPosRef is near this object
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

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
    virtual int DoCompare( const LIB_DRAW_ITEM& aOther ) const;

    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_Rect& aRect );
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const int aTransform[2][2] );
    virtual int DoGetWidth() { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
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
    LIB_POLYLINE( const LIB_POLYLINE& aPolyline );
    ~LIB_POLYLINE() { }

    virtual wxString GetClass() const
    {
        return wxT( "LIB_POLYLINE" );
    }


    /**
     * Write polyline object out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* aLine, wxString& aErrorMsg );

    void AddPoint( const wxPoint& aPoint );

    /**
     * @return the number of corners
     */
    unsigned GetCornerCount() const { return m_PolyPoints.size(); }

     /**
     * Test if the given point is within the bounds of this object.
     *
     * @param aRefPos - A wxPoint to test
     * @return - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

    /**
     * @param aPosRef = a wxPoint to test
     * @param aThreshold = max distance to a segment
     * @param aTransMat = the transform matrix
     * @return true if the point aPosRef is near a segment
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    /**
     * @return the boundary box for this, in library coordinates
     */
    virtual EDA_Rect GetBoundingBox();

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the ployline segment draw object specific comparison.
     *
     * The sort order for each polyline segment point is as follows:
     *      - Line segment point horizontal (X) position.
     *      - Line segment point vertical (Y) position.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& aOther ) const;

    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_Rect& aRect );
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() { return m_PolyPoints[0]; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const int aTransform[2][2] );
    virtual int DoGetWidth() { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
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
    LIB_BEZIER( const LIB_BEZIER& aBezier );
    ~LIB_BEZIER() { }

    virtual wxString GetClass() const
    {
        return wxT( "LIB_BEZIER" );
    }


    /**
     * Write bezier curve object out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* aLine, wxString& aErrorMsg );

    void         AddPoint( const wxPoint& aPoint );

    /**
     * @return the number of corners
     */
    unsigned GetCornerCount() const { return m_PolyPoints.size(); }

    /**
     * Test if the given point is within the bounds of this object.
     *
     * @param aRefPos - A wxPoint to test
     * @return true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& aRefPos );

    /**
     * @param aPosRef = a wxPoint to test
     * @param aThreshold = max distance to a segment
     * @param aTransMat = the transform matrix
     * @return true if the point aPosRef is near a segment
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold,
                          const int aTransMat[2][2] );

    /**
     * @return the boundary box for this, in library coordinates
     */
    virtual EDA_Rect GetBoundingBox();

    /**
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize( );

    void Draw( WinEDA_DrawPanel * aPanel, wxDC * aDC, const wxPoint &aOffset,
               int aColor, int aDrawMode, void* aData,
               const int aTransformMatrix[2][2] );

    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the bezier curve draw object specific comparison.
     *
     * The sort order for each bezier curve segment point is as follows:
     *      - Bezier point horizontal (X) point position.
     *      - Bezier point vertical (Y) point position.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& aOther ) const;

    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_Rect& aRect );
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() { return m_PolyPoints[0]; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const int aTransform[2][2] );
    virtual int DoGetWidth() { return m_Width; }
    virtual void DoSetWidth( int aWidth ) { m_Width = aWidth; }
};

#endif  //  CLASSES_BODY_ITEMS_H
