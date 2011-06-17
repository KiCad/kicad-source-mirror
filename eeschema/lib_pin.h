/****************************************************************/
/*  Headers for pins in lib component definitions */
/****************************************************************/

/* Definitions of class LIB_PIN used in component libraries.
 */
#ifndef CLASS_PIN_H
#define CLASS_PIN_H

#include "lib_draw_item.h"

class SCH_COMPONENT;


#define TARGET_PIN_RADIUS     12  /* Circle diameter drawn at the active end of pins */

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
#define NONLOGIC_PIN_DIM    30  /* Dim of nonlogic pin symbol (X). */

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
#define PIN_INVISIBLE 1    /* Set makes pin invisible */


/**
 * The component library pin object drawing shapes.
 */
enum DrawPinShape {
    NONE         = 0,
    INVERT       = 1,
    CLOCK        = 2,
    LOWLEVEL_IN  = 4,
    LOWLEVEL_OUT = 8,
    CLOCK_FALL   = 0x10, /* this is common form for inverted clock in Eastern Block */
    NONLOGIC     = 0x20
};


/**
 *  The component library pin object orientations.
 */
enum DrawPinOrient {
    PIN_RIGHT = 'R',
    PIN_LEFT  = 'L',
    PIN_UP    = 'U',
    PIN_DOWN  = 'D'
};


class LIB_PIN : public LIB_ITEM
{
    wxPoint  m_position;     ///< Position of the pin.
    int      m_length;       ///< Length of the pin.
    int      m_orientation;  ///< Pin orientation (Up, Down, Left, Right)
    int      m_shape;        ///< Bitwise ORed of pin shapes (see enum DrawPinShape)
    int      m_width;        ///< Line width of the pin.
    int      m_type;         ///< Electrical type of the pin.  See enum ElectricPinType.
    int      m_attributes;   ///< Set bit 0 to indicate pin is invisible.
    wxString m_name;
    long     m_number;       ///< Pin number defined as 4 ASCII characters like "12", "anod",
                             ///< "G6", or "12".  It is stored as "12\0\0" and does not
                             ///< depend on endian type.

    /**
     * Draw the pin.
     */
    void drawGraphic( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                      int aColor, int aDrawMode, void* aData, const TRANSFORM& aTransform );

public:
    int      m_PinNumSize;
    int      m_PinNameSize; /* Pin num and Pin name sizes */

    /* (Currently Unused) Pin num and Pin name text options: italic/normal
     * /bold, 0 = default */
    char     m_PinNumShapeOpt;
    char     m_PinNameShapeOpt;
    // (Currently Unused) Pin num and Pin name text opt position, 0 = default:
    char     m_PinNumPositionOpt;
    char     m_PinNamePositionOpt;

public:
    LIB_PIN( LIB_COMPONENT * aParent );
    LIB_PIN( const LIB_PIN& aPin );
    ~LIB_PIN() { }

    virtual wxString GetClass() const
    {
        return wxT( "LIB_PIN" );
    }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os );   // virtual override
#endif


    /**
     * Write pin object to a FILE in "*.lib" format.
     *
     * @param aFile The FILE to write to.
     * @return - true if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* aLine, wxString& aErrorMsg );

    /**
     * Function HitTest
     * verifies that \a aRefPos within the bounds of this pin attached to \a aComponent.
     * <p>
     * The coordinates of the pin are calculated relative to \a aComponent if not NULL.
     * Otherwise, the pin coordinates are relative to the library anchor position.
     * </p>
     * @param aRefPos A wxPoint to test
     * @return True \a aRefPos lies within the pin bounding box else false.
     */
    virtual bool HitTest( const wxPoint& aRefPos );

    /**
     * @param aPosRef - a wxPoint to test
     * @param aThreshold - max distance to this object (usually the half
     *                     thickness of a line)
     * @param aTransform - the transform matrix
     * @return - true if the point aPosRef is near this object
     */
    virtual bool HitTest( wxPoint aPosRef, int aThreshold, const TRANSFORM& aTransform );

    virtual void DisplayInfo( EDA_DRAW_FRAME* frame );

    virtual EDA_RECT GetBoundingBox() const;

    wxPoint ReturnPinEndPoint() const;

    /**
     * Function ReturnPinDrawOrient
     * returns the pin real orientation (PIN_UP, PIN_DOWN, PIN_RIGHT, PIN_LEFT),
     * according to its orientation and the matrix transform (rot, mirror) \a aTransform
     * @param aTransform = transform matrix
     */
    int ReturnPinDrawOrient( const TRANSFORM& aTransform ) const;

    /**
     * Fill a string buffer with pin number.
     *
     * Pin numbers are coded as a long or 4 ASCII characters.  Used to print
     * or draw the pin number.
     *
     * @param aStringBuffer - the wxString to store the pin num as an unicode string
     */
    void ReturnPinStringNum( wxString& aStringBuffer ) const;

    long GetNumber() const { return m_number; }

    wxString GetNumberString() const { return ReturnPinStringNum( m_number ); }

    /**
     * Function ReturnPinStringNum (static function)
     * Pin num is coded as a long or 4 ascii chars
     * @param aPinNum = a long containing a pin num
     * @return aStringBuffer = the wxString to store the pin num as an
     *                         unicode string
     */
    static wxString ReturnPinStringNum( long aPinNum );

    void SetPinNumFromString( wxString& aBuffer );

    wxString GetName() const { return m_name; }

    /**
     * Set the pin name.
     *
     * This will also all of the pin names marked by EnableEditMode().
     *
     * @param aName - New pin name.
     */
    void SetName( const wxString& aName );

    /**
     * Set the \a aSize of the pin name text.
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
     * Others pin numbers marked by EnableEditMode() are not modified
     * because each pin has its own number
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

    int GetOrientation() const { return m_orientation; }

    /**
     * Set orientation on the pin.
     *
     * This will also update the orientation of the pins marked by EnableEditMode().
     *
     * @param aOrientation - The orientation of the pin.
     */
    void SetOrientation( int aOrientation );

    void Rotate();

    int GetShape() const { return m_shape; }

    /**
     * Set the shape of the pin to \a aShape.
     *
     * This will also update the draw style of the pins marked by EnableEditMode().
     *
     * @param aShape - The draw shape of the pin.  See enum DrawPinShape.
     */
    void SetShape( int aShape );

    /**
     * Get the electrical type of the pin.
     *
     * @return The electrical type of the pin (see enun ElectricPinType for values).
     */
    int GetType() const { return m_type; }

    /**
     * return a string giving the electrical type of the pin.
     *
     * @return The electrical name of the pin (see enun MsgPinElectricType for names).
     */
    wxString GetTypeString() const { return MsgPinElectricType[m_type]; }

    /**
     * Set the electrical type of the pin.
     *
     * This will also update the electrical type of the pins marked by
     * EnableEditMode().
     *
     * @param aType - The electrical type of the pin(see enun ElectricPinType for values).
     */
    void SetType( int aType );

    /**
     * Set the pin length.
     *
     * This will also update the length of the pins marked by EnableEditMode().
     *
     * @param aLength - The length of the pin in mils.
     */
    void SetLength( int aLength );

    int GetLength() { return m_length; }

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
     * @param aConversion - Body style of the pin.  Set to zero to make pin
     *                      common to all body styles.
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
     * @param aEnable - True marks all common pins for editing mode.  False
     *                  clears the editing mode.
     * @param aEditPinByPin - Enables the edit pin by pin mode.
     */
    void EnableEditMode( bool aEnable, bool aEditPinByPin = false );

    /**
     * Return the visibility status of the draw object.
     *
     * @return True if draw object is visible otherwise false.
     */
    bool IsVisible() { return ( m_attributes & PIN_INVISIBLE ) == 0; }

    /**
     * @return the size of the "pen" that be used to draw or plot this item.
     */
    virtual int GetPenSize() const;

    void DrawPinSymbol( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                        int aOrientation, int aDrawMode, int aColor = -1 );

    void DrawPinTexts( EDA_DRAW_PANEL* aPanel, wxDC* aDC, wxPoint& aPosition,
                       int aOrientation, int TextInside, bool DrawPinNum, bool DrawPinName,
                       int aColor, int aDrawMode );

    void PlotPinTexts( PLOTTER *aPlotter,
                       wxPoint& aPosition,
                       int      aOrientation,
                       int      aTextInside,
                       bool     aDrawPinNum,
                       bool     aDrawPinName,
                       int      aWidth );

    void PlotSymbol( PLOTTER* aPlotter, const wxPoint& aPosition, int aOrientation );

    /**
     * Get a list of pin orientation names.
     *
     * @return List of valid pin orientation names.
     */
    static wxArrayString GetOrientationNames();

    /**
     * Get a list of pin orientation bitmaps for menus and dialogs.
     *
     * @return  List of valid pin orientation bitmaps symbols in .xpm format
     */
    static const char*** GetOrientationSymbols();

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
     * Get a list of pin styles bitmaps for menus and dialogs.
     *
     * @return  List of valid pin electrical type bitmaps symbols in .xpm format.
     */
    static const char*** GetStyleSymbols();

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

    /**
     * Get a list of pin electrical bitmaps for menus and dialogs.
     *
     * @return  List of valid pin electrical type bitmaps symbols in .xpm format
     */
    static const char*** GetElectricalTypeSymbols();

    virtual const char** GetMenuImage() const;

    virtual wxString GetSelectMenuText() const;

protected:
    virtual EDA_ITEM* doClone() const;

    /**
     * Provide the pin draw object specific comparison.
     *
     * The sort order is as follows:
     *      - Pin number.
     *      - Pin name, case insensitive compare.
     *      - Pin horizontal (X) position.
     *      - Pin vertical (Y) position.
     */
    virtual int DoCompare( const LIB_ITEM& aOther ) const;
    virtual void DoOffset( const wxPoint& aOffset );
    virtual bool DoTestInside( EDA_RECT& aRect ) const;
    virtual void DoMove( const wxPoint& aPosition );
    virtual wxPoint DoGetPosition() const { return m_position; }
    virtual void DoMirrorHorizontal( const wxPoint& aCenter );
    virtual void DoMirrorVertical( const wxPoint& aCenter );
    virtual void DoRotate( const wxPoint& aCenter, bool aRotateCCW = true );
    virtual void DoPlot( PLOTTER* aPlotter, const wxPoint& aOffset, bool aFill,
                         const TRANSFORM& aTransform );
    virtual int DoGetWidth() const { return m_width; }
    virtual void DoSetWidth( int aWidth );
};


#endif  //  CLASS_PIN_H
