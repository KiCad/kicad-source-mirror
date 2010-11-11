/******************************************************/
/* Definitions for the Component classes for EESchema */
/******************************************************/

#ifndef COMPONENT_CLASS_H
#define COMPONENT_CLASS_H


#include "sch_field.h"
#include "transform.h"
#include "general.h"


class SCH_SHEET_PATH;
class LIB_PIN;
class LIB_COMPONENT;


/**
 * Holder of an error message and may be thrown from functions.
 */
struct Error
{
    wxString errorText;

    Error( const wxChar* aMsg ) :
        errorText( aMsg )
    {
    }


    Error( const wxString& aMsg ) :
        errorText( aMsg )
    {
    }
};

/// A container for several SCH_FIELD items
typedef std::vector<SCH_FIELD> SCH_FIELDS;


/**
 * Class SCH_COMPONENT
 * describes a real schematic component
 */
class SCH_COMPONENT : public SCH_ITEM
{
    friend class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC;

public:
    int      m_Multi;            // In multi unit chip - which unit to draw.

    wxPoint  m_Pos;

    wxString m_ChipName;         /* Key to look for in the library,
                                  * i.e. "74LS00". */

    wxString m_PrefixString;          /* C, R, U, Q etc - the first character
                                       * which typically indicates what the
                                       * component is. Determined, upon
                                       * placement, from the library component.
                                       * determined, upon file load, by the
                                       * first non-digits in the reference
                                       * fields. */

    int m_Convert;                    /* Handle multiple shape (for instance
                                       * De Morgan conversion) */
    TRANSFORM m_Transform;            /* The rotation/mirror transformation
                                       * matrix. */

private:

    SCH_FIELDS m_Fields;              ///< variable length list of fields


    /* Hierarchical references.
     * format is
     * path reference multi
     * with:
     * path = /<timestamp1>/<timestamp2> (subsheet path, = / for the root sheet)
     * reference = reference for this path (C23, R5, U78 ... )
     * multi = part selection in multi parts per package (0 or 1 for one part
     *         per package)
     */
    wxArrayString m_PathsAndReferences;

    void Init( const wxPoint& pos = wxPoint( 0, 0 ) );

public:
    SCH_COMPONENT( const wxPoint& pos = wxPoint( 0, 0 ), SCH_ITEM* aParent = NULL );

    /**
     * Create schematic component from library component object.
     *
     * @param libComponent - Component library object to create schematic
     *                       component from.
     * @param sheet - Schematic sheet the component is place into.
     * @param unit - Part for components that have multiple parts per
     *               package.
     * @param convert - Use the alternate body style for the schematic
     *                  component.
     * @param pos - Position to place new component.
     * @param setNewItemFlag - Set the component IS_NEW and IS_MOVED flags.
     */
    SCH_COMPONENT( LIB_COMPONENT& libComponent, SCH_SHEET_PATH* sheet,
                   int unit = 0, int convert = 0,
                   const wxPoint& pos = wxPoint( 0, 0 ),
                   bool setNewItemFlag = false );

    /**
     * Copy Constructor
     * clones \a aTemplate into this object.  All fields are copied as is except
     * for the linked list management pointers which are set to NULL, and the
     * SCH_FIELD's m_Parent pointers which are set to the new parent,
     * i.e. this new object.
     */
    SCH_COMPONENT( const SCH_COMPONENT& aTemplate );

    ~SCH_COMPONENT() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_COMPONENT" );
    }

    TRANSFORM& GetTransform() const { return const_cast< TRANSFORM& >( m_Transform ); }

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic component from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read the component from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the component.
     * @return True if the component loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function Load
     * reads a component in from a file.  The file stream must be positioned at
     * the first field of the file, not at the component tag.
     * @param aFile The FILE to read from.
     * @throw Error containing the error message text if there is a file format
     *   error or if the disk read has failed.
     *  void            Load( FILE* aFile ) throw( Error );
     */

    /**
     * Function GenCopy
     * returns a copy of this object but with the linked list pointers
     * set to NULL.
     * @return SCH_COMPONENT* - a copy of me.
     */
    SCH_COMPONENT* GenCopy()
    {
        return new SCH_COMPONENT( *this );
    }


    void       SetOrientation( int aOrientation );

    /** function GetOrientation()
     * Used to display component orientation (in dialog editor or info)
     * @return the orientation and mirror
     * Note: Because there are different ways to have a given orientation/mirror,
     * the orientation/mirror is not necessary what the used does
     * (example : a mirrorX then a mirrorY give no mirror but rotate the
     * component).
     * So this function find a rotation and a mirror value
     * ( CMP_MIRROR_X because this is the first mirror option tested)
     *  but can differs from the orientation made by an user
     * ( a CMP_MIRROR_Y is find as a CMP_MIRROR_X + orientation 180, because
     * they are equivalent)
     */
    int        GetOrientation();

    wxPoint    GetScreenCoord( const wxPoint& coord );
    void       DisplayInfo( WinEDA_DrawFrame* frame );

    /**
     * Suppress annotation ( i.i IC23 changed to IC? and part reset to 1)
     * @param aSheet: SCH_SHEET_PATH value: if NULL remove all annotations,
     *             else remove annotation relative to this sheetpath
     */
    void       ClearAnnotation( SCH_SHEET_PATH* aSheet );

    /** function SetTimeStamp
     * Change the old time stamp to the new time stamp.
     * the time stamp is also modified in paths
     * @param aNewTimeStamp = new time stamp
     */
    void       SetTimeStamp( long aNewTimeStamp );

    /**
     * Function GetBoundaryBox
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for graphic items and pins.
     * this include only fields defined in library
     * use GetBoundingBox() to include fields in schematic
     */
    EDA_Rect   GetBoundaryBox() const;

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    EDA_Rect   GetBoundingBox();

    //-----<Fields>-----------------------------------------------------------

    /**
     * Function ReturnFieldName
     * returns the Field name given a field index like (REFERENCE, VALUE ..)
     * @reeturn wxString - the field name or wxEmptyString if invalid field
     *                     index.
     */
    wxString   ReturnFieldName( int aFieldNdx ) const;

    /**
     * Function GetField
     * returns a field.
     * @param aFieldNdx An index into the array of fields, not a field id.
     * @return SCH_FIELD* - the field value or NULL if does not exist
     */
    SCH_FIELD* GetField( int aFieldNdx ) const;

    /**
     * Function AddField
     * adds a field to the component.  The field is copied as it is put into
     * the component.
     * @param aField A const reference to the SCH_FIELD to add.
     * @return SCH_FIELD* - the newly inserted field.
     */
    SCH_FIELD* AddField( const SCH_FIELD& aField );

    /**
     * Function FindField
     * searches for SCH_FIELD with \a aFieldName and returns it if found, else NULL.
     */
    SCH_FIELD* FindField( const wxString& aFieldName );

    void SetFields( const SCH_FIELDS& aFields )
    {
        m_Fields = aFields;     // vector copying, length is changed possibly
    }


    //-----</Fields>----------------------------------------------------------

    /**
     * Function GetFieldCount
     * returns the number of fields in this component.
     */
    int GetFieldCount() const { return (int) m_Fields.size(); }

    /**
     * Find a component pin by number.
     *
     * @param number - The number of the pin to find.
     * @return Pin object if found, otherwise NULL.
     */
    LIB_PIN* GetPin( const wxString& number );

    virtual void            Draw( WinEDA_DrawPanel* panel,
                                  wxDC*             DC,
                                  const wxPoint&    offset,
                                  int               draw_mode,
                                  int               Color = -1 )
    {
        Draw( panel, DC, offset, draw_mode, Color, true );
    }


    void Draw( WinEDA_DrawPanel* panel,
               wxDC*             DC,
               const wxPoint&    offset,
               int               draw_mode,
               int               Color,
               bool              DrawPinText );

    void           SwapData( SCH_COMPONENT* copyitem );

    void           Place( WinEDA_SchematicFrame* frame, wxDC* DC );

    // returns a unique ID, in the form of a path.
    wxString       GetPath( SCH_SHEET_PATH* sheet );

    /**
     * Function GetRef
     * returns the reference, for the given sheet path.
     */
    const wxString GetRef( SCH_SHEET_PATH* sheet );

    // Set the reference, for the given sheet path.
    void           SetRef( SCH_SHEET_PATH* sheet, const wxString& ref );

    /**
     * Function AddHierarchicalReference
     * adds a full hierarchical reference (path + local reference)
     * @param aPath = hierarchical path (/<sheet timestamp>/component
     *                timestamp> like /05678E50/A23EF560)
     * @param aRef = local reference like C45, R56
     * @param aMulti = part selection, used in multi part per package (0 or 1
     *                 for non multi)
     */
    void           AddHierarchicalReference( const wxString& aPath,
                                             const wxString& aRef,
                                             int             aMulti );

    // returns the unit selection, for the given sheet path.
    int            GetUnitSelection( SCH_SHEET_PATH* aSheet );

    // Set the unit selection, for the given sheet path.
    void           SetUnitSelection( SCH_SHEET_PATH* aSheet, int aUnitSelection );

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     * for a component, has no meaning, but it is necessary to satisfy the
     * SCH_ITEM class requirements.
     */
    virtual int GetPenSize() { return 0; }

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
        for( int ii = 0; ii < GetFieldCount(); ii++ )
            GetField( ii )->Move( aMoveVector );
    }


    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );
    virtual void Mirror_X( int aXaxis_position );
    virtual void Rotate( wxPoint rotationPoint );


    /**
     * Compare schematic component reference and value fields against search string.
     *
     * @param aSearchData - Criteria to search against.
     * @param aAuxData - a pointer on auxiliary data, if needed.
     *        When searching string in REFERENCE field we must know the sheet path
     *          This param is used in this case
     * @param aFindLocation - a wxPoint where to put the location of matched item. can be NULL.
     * @return True if this component reference or value field matches the search criteria.
     */
    virtual bool Matches( wxFindReplaceData& aSearchData, void* aAuxData, wxPoint* aFindLocation );

    virtual void GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList );

    wxPoint GetPinPhysicalPosition( LIB_PIN* Pin );

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const;

    #if defined(DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void         Show( int nestLevel, std::ostream& os );

#endif
};


#endif /* COMPONENT_CLASS_H */
