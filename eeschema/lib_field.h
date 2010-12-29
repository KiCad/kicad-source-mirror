/*************************************************************/
/* Lib component definitions (libentry) definition of fields */
/*************************************************************/

#ifndef CLASS_LIBENTRY_FIELDS_H
#define CLASS_LIBENTRY_FIELDS_H

#include "lib_draw_item.h"


/**
 * Class LIB_FIELD
 * is used in symbol libraries.  At least MANDATORY_FIELDS are always present
 * in a ram resident library symbol.  All constructors must ensure this because
 * the component property editor assumes it.
 * @see enum NumFieldType
 */
class LIB_FIELD : public LIB_DRAW_ITEM, public EDA_TextStruct
{
    int      m_id;           ///< @see enum NumFieldType
    wxString m_name;         ///< Name (not the field text value itself, that is .m_Text)

    wxString m_savedText;    ///< Temporary storage for the string when edition.
    bool     m_rotate;       ///< Flag to indicate a rotation occurred while editing.
    bool     m_updateText;   ///< Flag to indicate text change occurred while editing.

    /**
     * Draw the field.
     */
    void drawGraphic( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                      int aColor, int aDrawMode, void* aData, const TRANSFORM& aTransform );

    /**
     * Calculate the new circle at \a aPosition when editing.
     *
     * @param aPosition - The position to edit the circle in drawing coordinates.
     */
    void calcEdit( const wxPoint& aPosition );

public:

    LIB_FIELD( int idfield = 2 );
    LIB_FIELD( LIB_COMPONENT * aParent, int idfield = 2 );
    LIB_FIELD( const LIB_FIELD& field );
    ~LIB_FIELD();

    virtual wxString GetClass() const
    {
        return wxT( "LIB_FIELD" );
    }

    /**
     * Object constructor initialization helper.
     */
    void Init( int idfield );

    /**
     * Returns the field name.
     *
     * The first four field IDs are reserved and therefore always return their respective
     * names.  The user definable fields will return FieldN where N is the ID of the field
     * when the m_name member is empty.
     *
     * @return Name of the field.
     */
    wxString GetName() const;

    /**
     * Function SetName
     *
     * Sets a user definable field name to \a aName.
     *
     * Reserved fields such as value and reference are not renamed.  If the field name is
     * changed, the field modified flag is set.  If the field is the child of a component,
     * the parent component's modified flag is also set.
     *
     * @param aName - User defined field name.
     */
    void SetName( const wxString& aName );

    int GetId() { return m_id; }

    void SetId( int aId ) { m_id = aId; }

    /**
     * Function GetPenSize virtual pure
     * @return the size of the "pen" that be used to draw or plot this item
     */
    int GetPenSize( );

    /**
     * Writes field object out to a FILE in "*.lib" format.
     *
     * @param aFile The FILE to write to.
     * @return True if success writing else false.
     */
    virtual bool Save( FILE* aFile );
    virtual bool Load( char* line, wxString& errorMsg );

    /**
     * Copy parameters of this field to another field. Pointers are not copied.
     *
     * @param aTarget = Target field to copy values to.
     */
    void          Copy( LIB_FIELD* aTarget ) const;

    void          SetFields( const std::vector <LIB_FIELD> aFields );

    /**
     * Function IsVoid
     * @return true if the field value is void (no text in this field)
     */
    bool IsVoid()
    {
        return m_Text.IsEmpty();
    }

    /**
     * Function IsVisible
     * @return true is this field is visible, false if flagged invisible
     */
    bool IsVisible()
    {
        return (m_Attributs & TEXT_NO_VISIBLE) == 0 ? true : false;
    }

    /**
     * Return the bounding rectangle of the field text.
     * @return Bounding rectangle.
     */
    virtual EDA_Rect GetBoundingBox() const;

    /**
     * Displays info (type, part  convert filed name and value)
     * in msg panel
     * @param aFrame = main frame where the message manel info is.
     */
    virtual void DisplayInfo( WinEDA_DrawFrame* aFrame );

    /**
     * Test if the given point is within the bounds of this object.
     *
     * @param aPosition A point to test in field coordinate system
     * @return True if a hit, else false
     */
    bool HitTest( const wxPoint& aPosition );

     /**
      * @param aPosition = a wxPoint to test
      * @param aThreshold = max distance to this object (usually the half
      *                     thickness of a line)
      * @param aTransform = the transform matrix
      * @return True if the point \a aPosition is near this object
      */
    virtual bool HitTest( wxPoint aPosition, int aThreshold, const TRANSFORM& aTransform );

    void operator=( const LIB_FIELD& field )
    {
        m_id = field.m_id;
        m_Text = field.m_Text;
        m_name = field.m_name;
        m_Pos = field.m_Pos;
        m_Size = field.m_Size;
        m_Thickness = field.m_Thickness;
        m_Orient = field.m_Orient;
        m_Mirror = field.m_Mirror;
        m_Attributs = field.m_Attributs;
        m_Italic = field.m_Italic;
        m_Bold = field.m_Bold;
        m_HJustify = field.m_HJustify;
        m_VJustify = field.m_VJustify;
        m_Parent = field.m_Parent;
    }

    /**
     * Return the text of a field.
     *
     * If the field is the reference field, the unit number is used to
     * create a pseudo reference text.  If the base reference field is U,
     * the string U?A will be returned for unit = 1.
     *
     * @param unit - The package unit number.  Only effects reference field.
     * @return Field text.
     */
    wxString GetFullText( int unit = 1 );

    int GetDefaultColor();

    /**
     * See LIB_DRAW_ITEM::BeginEdit().
     */
    void BeginEdit( int aEditMode, const wxPoint aStartPoint = wxPoint( 0, 0 ) );

    /**
     * See LIB_DRAW_ITEM::ContinueEdit().
     */
    bool ContinueEdit( const wxPoint aNextPoint );

    /**
     * See LIB_DRAW_ITEM::AbortEdit().
     */
    void EndEdit( const wxPoint& aPosition, bool aAbort = false );

    void Rotate();

    /**
     * Sets the field text to \a aText.
     *
     * This method does more than juat set the set the field text.  There are special
     * cases when changing the text string alone is not enough.  If the field is the
     * value field, the parent component's name is changed as well.  If the field is
     * being moved, the name change must be delayed until the next redraw to prevent
     * drawing artifacts.
     *
     * @param aText - New text value.
     */
    void SetText( const wxString& aText );

protected:
    virtual LIB_DRAW_ITEM* DoGenCopy();

    /**
     * Provide the field draw object specific comparison.
     *
     * The sort order for field is as follows:
     *
     *      - Field ID, REFERENCE, VALUE, etc.
     *      - Field string, case insensitive compare.
     *      - Field horizontal (X) position.
     *      - Field vertical (Y) position.
     *      - Field width.
     *      - Field height.
     */
    virtual int DoCompare( const LIB_DRAW_ITEM& other ) const;

    virtual void DoOffset( const wxPoint& offset );
    virtual bool DoTestInside( EDA_Rect& rect ) const;
    virtual void DoMove( const wxPoint& newPosition );
    virtual wxPoint DoGetPosition( void ) const { return m_Pos; }
    virtual void DoMirrorHorizontal( const wxPoint& center );
    virtual void DoPlot( PLOTTER* plotter, const wxPoint& offset, bool fill,
                         const TRANSFORM& aTransform );
    virtual int DoGetWidth( void ) const { return m_Thickness; }
    virtual void DoSetWidth( int width ) { m_Thickness = width; }
};

typedef std::vector< LIB_FIELD > LIB_FIELD_LIST;

#endif  //  CLASS_LIBENTRY_FIELDS_H
