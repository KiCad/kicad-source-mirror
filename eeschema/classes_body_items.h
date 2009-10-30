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

/* Electrical pin type names. */
extern const wxChar* MsgPinElectricType[];

/* Pin visibility flag bit. */
#define PINNOTDRAW 1    /* Set makes pin invisible */


/**
 * The component library pin object drawing shapes.
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
 *  The component library pin object orientations.
 */
enum DrawPinOrient
{
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


    LIB_DRAW_ITEM( KICAD_T struct_type, LIB_COMPONENT * aParent );
    LIB_DRAW_ITEM( const LIB_DRAW_ITEM& item );
    virtual ~LIB_DRAW_ITEM() { }

    /**
     * Draw A body item
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
    virtual int GetPenSize( ) = 0;

    /**
     * Write draw item object to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile ) = 0;
    virtual bool Load( char* line, wxString& errorMsg ) = 0;

    LIB_COMPONENT * GetParent()
    {
        return (LIB_COMPONENT *)m_Parent;
    }

    /**
     * Tests if the given point is within the bounds of this object.
     *
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( const wxPoint& refPos )
    {
        return false;   // derived classes should override this function
    }

    /**
     * @param aPosRef - a wxPoint to test
     * @param aThreshold - max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransMat - the transform matrix
     * @return true if the point aPosRef is near this object
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
    void SetWidth( int width ) { DoSetWidth( width ); }

    /**
     * Check if draw object can be filled.
     *
     * The default setting is false.  If the derived object support filling,
     * set the m_isFillable member to true.
     *
     * @return bool - True if draw object can be fill.  Default is false.
     */
    bool IsFillable( void ) { return m_isFillable; }

    /**
     * Return the modified status of the draw object.
     *
     * @return bool - True if the draw object has been modified.
     */
    bool IsModified( void ) { return ( m_Flags & IS_CHANGED ) != 0; }

    /**
     * Return the new item status of the draw object.
     *
     * @return bool - True if the draw item has been added to the
     *                parent component.
     */
    bool IsNew( void ) { return ( m_Flags & IS_NEW ) != 0; }

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
    virtual void DoSetWidth( int width ) = 0;

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
    LIB_PIN( const LIB_PIN& pin );
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
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* line, wxString& errorMsg );


    /**
     * Test if the given point is within the bounds of this object.
     *
     * @param aRefPos A wxPoint to test
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

    wxString     GetNumber( void );

    /** Function ReturnPinStringNum (static function)
     * Pin num is coded as a long or 4 ascii chars
     * @param aPinNum = a long containing a pin num
     * @return aStringBuffer = the wxString to store the pin num as an
     *                         unicode string
     */
    static wxString ReturnPinStringNum( long aPinNum );

    void         SetPinNumFromString( wxString& buffer );

    /**
     * Set the pin name.
     *
     * This will also all of the pin names marked by EnableEditMode().
     *
     * @param name - New pin name.
     */
    void SetName( const wxString& name );

    /**
     * Set the size of the pin name text.
     *
     * This will also update the text size of the name of the pins marked
     * by EnableEditMode().
     *
     * @param size - The text size of the pin name in schematic units ( mils ).
     */
    void SetNameTextSize( int size );

    /**
     * Set the pin number.
     *
     * This will also all of the pin numbers marked by EnableEditMode().
     *
     * @param number - New pin number.
     */
    void SetNumber( const wxString& number );

    /**
     * Set the size of the pin number text.
     *
     * This will also update the text size of the number of the pins marked
     * by EnableEditMode().
     *
     * @param size - The text size of the pin number in schematic
     *               units ( mils ).
     */
    void SetNumberTextSize( int size );

    /**
     * Set orientation on the pin.
     *
     * This will also update the orientation of the pins marked by
     * EnableEditMode().
     *
     * @param orientation - The orientation of the pin.
     */
    void SetOrientation( int orientation );

    /**
     * Set the draw style of the pin.
     *
     * This will also update the draw style of the pins marked by
     * EnableEditMode().
     *
     * @param style - The draw style of the pin.
     */
    void SetDrawStyle( int style );

    /**
     * Set the electrical type of the pin.
     *
     * This will also update the electrical type of the pins marked by
     * EnableEditMode().
     *
     * @param type - The electrical type of the pin.
     */
    void SetElectricalType( int style );

    /**
     * Set the pin length.
     *
     * This will also update the length of the pins marked by EnableEditMode().
     *
     * @param size - The length of the pin in mils.
     */
    void SetLength( int length );

    /**
     * Set the pin part number.
     *
     * If the pin is changed from not common to common to all parts, any
     * linked pins will be removed from the parent component.
     *
     * @param part - Number of the part the pin belongs to.  Set to zero to
     *               make pin common to all parts in a multi-part component.
     */
    void SetPartNumber( int part );

    /**
     * Set the body style (conversion) of the pin.
     *
     * If the pin is changed from not common to common to all body styles, any
     * linked pins will be removed from the parent component.
     *
     * @param conversion - Body style of the pin.  Set to zero to make pin
     *                     common to all body styles.
     */
    void SetConversion( int conversion );

    /**
     * Set or clear the visibility flag for the pin.
     *
     * This will also update the visibility of the pins marked by
     * EnableEditMode().
     *
     * @param visible - True to make the pin visible or false to hide the pin.
     */
    void SetVisible( bool visible );

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
     * @params enable - True marks all common pins for editing mode.  False
     *                  clears the editing mode.
     * @params editpinByPin - Enables the edit pin by pin mode.
     */
    void EnableEditMode( bool enable, bool pinByPin = false );

    /**
     * Return the visibility status of the draw object.
     *
     * @return bool - True if draw object is visible otherwise false.
     */
    bool IsVisible( void ) { return ( m_Attributs & PINNOTDRAW ) == 0; }

    /**
     * @return the size of the "pen" that be used to draw or plot this item.
     */
    virtual int GetPenSize();

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

    /**
     * Get a list of pin orientation names.
     *
     * @return wxArrayString - List of valid pin orientation names.
     */
    static wxArrayString GetOrientationNames( void );

    /**
     * Get the orientation code by index used to set the pin orientation.
     *
     * @param index - The index of the orientation code to look up.
     * @return int - Orientation code if index is valid.  Returns right
     *               orientation on index error.
     */
    static int GetOrientationCode( int index );

    /**
     * Get the index of the orientation code.
     *
     * @param code - The orientation code to look up.
     * @return int - The index of the orientation code if found.  Otherwise,
     *               return wxNOT_FOUND.
     */
    static int GetOrientationCodeIndex( int code );

    /**
     * Get a list of pin draw style names.
     *
     * @return wxArrayString - List of valid pin draw style names.
     */
    static wxArrayString GetStyleNames( void );

    /**
     * Get the pin draw style code by index used to set the pin draw style.
     *
     * @param index - The index of the pin draw style code to look up.
     * @return int - Pin draw style code if index is valid.  Returns NONE
     *               style on index error.
     */
    static int GetStyleCode( int index );

    /**
     * Get the index of the pin draw style code.
     *
     * @param code - The pin draw style code to look up.
     * @return int - The index of the pin draw style code if found.  Otherwise,
     *               return wxNOT_FOUND.
     */
    static int GetStyleCodeIndex( int code );

    /**
     * Get a list of pin electrical type names.
     * @return wxArrayString - List of valid pin electrical type names.
     */
    static wxArrayString GetElectricalTypeNames( void );

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
    virtual void DoSetWidth( int width ) { m_Width = width; }
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
    LIB_ARC( const LIB_ARC& arc );
    ~LIB_ARC() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_ARC" );
    }


    /**
     * Save arc object to a FILE in "*.lib" format.
     *
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* line, wxString& errorMsg );

    /**
     * Tests if the given wxPoint is within the bounds of this object.
     *
     * @param aRefPos A wxPoint to test
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
    virtual int DoCompare( const LIB_DRAW_ITEM& other ) const;
    virtual void DoOffset( const wxPoint& offset );
    virtual bool DoTestInside( EDA_Rect& rect );
    virtual void DoMove( const wxPoint& newPosition );
    virtual wxPoint DoGetPosition( void ) { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& center );
    virtual void DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                         const int transform[2][2] );
    virtual int DoGetWidth( void ) { return m_Width; }
    virtual void DoSetWidth( int width ) { m_Width = width; }
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
     * Write circle object to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* line, wxString& errorMsg );

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
    virtual void DoSetWidth( int width ) { m_Width = width; }
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
     * Write text object out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* line, wxString& errorMsg );

    /**
     * Test if the given point is within the bounds of this object.
     *
     * @param refPos - A wxPoint to test
     * @return bool - true if a hit, else false
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
     * @param refArea - the given EDA_Rect
     * @return bool - true if a hit, else false
     */
    virtual bool HitTest( EDA_Rect& refArea )
    {
        return TextHitTest( refArea );
    }

    /**
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
    virtual void DoSetWidth( int width ) { m_Width = width; }
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
     * Write rectangle object out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* line, wxString& errorMsg );

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
    virtual void DoSetWidth( int width ) { m_Width = width; }
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
    LIB_SEGMENT( const LIB_SEGMENT& segment );
    ~LIB_SEGMENT() { }
    virtual wxString GetClass() const
    {
        return wxT( "LIB_SEGMENT" );
    }


    /**
     * Writes segment object out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* line, wxString& errorMsg );

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
    virtual void DoSetWidth( int width ) { m_Width = width; }
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
     * Write polyline object out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* line, wxString& errorMsg );

    void AddPoint( const wxPoint& point );

    /**
     * @return the number of corners
     */
    unsigned GetCornerCount() const { return m_PolyPoints.size(); }

     /**
     * Test if the given point is within the bounds of this object.
     *
     * @param aRefPos - A wxPoint to test
     * @return bool - true if a hit, else false
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
    virtual void DoSetWidth( int width ) { m_Width = width; }
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
     * Write bezier curve object out to a FILE in "*.lib" format.
     *
     * @param aFile - The FILE to write to.
     * @return bool - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* line, wxString& errorMsg );

    void         AddPoint( const wxPoint& point );

    /**
     * @return the number of corners
     */
    unsigned GetCornerCount() const { return m_PolyPoints.size(); }

    /**
     * Test if the given point is within the bounds of this object.
     *
     * @param aRefPos - A wxPoint to test
     * @return bool - true if a hit, else false
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
    virtual void DoSetWidth( int width ) { m_Width = width; }
};

#endif  //  CLASSES_BODY_ITEMS_H
