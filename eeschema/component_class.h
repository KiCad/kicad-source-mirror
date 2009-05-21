/*****************************************************/
/* Definitions for the Component classes for EESchema */
/*****************************************************/

#ifndef COMPONENT_CLASS_H
#define COMPONENT_CLASS_H


#include "class_sch_screen.h"
#include <wx/arrstr.h>
#include <wx/dynarray.h>

#include "class_sch_cmp_field.h"


extern void DrawLibPartAux( WinEDA_DrawPanel* panel, wxDC* DC,
                            SCH_COMPONENT* Component,
                            EDA_LibComponentStruct* Entry,
                            const wxPoint& Pos, const int TransMat[2][2],
                            int Multi, int convert, int DrawMode,
                            int Color = -1, bool DrawPinText = TRUE );


WX_DECLARE_OBJARRAY( DrawSheetPath, ArrayOfSheetLists );


/**
 * Struct Error
 * is a holder of an error message and may be thrown from functions.
 */
struct Error
{
    wxString    errorText;

    Error( const wxChar* aMsg ) :
        errorText( aMsg )
    {
    }

    Error( const wxString& aMsg ) :
        errorText( aMsg )
    {
    }
};


/**
 * Enum NumFieldType
 * is the numbered set of all fields a SCH_COMPONENT can hold
 */
enum  NumFieldType {
    REFERENCE = 0,          ///< Field Reference of part, i.e. "IC21"
    VALUE,                  ///< Field Value of part, i.e. "3.3K"
    FOOTPRINT,              ///< Field Name Module PCB, i.e. "16DIP300"
    DATASHEET,              ///< name of datasheet

    FIELD1,
    FIELD2,
    FIELD3,
    FIELD4,
    FIELD5,
    FIELD6,
    FIELD7,
    FIELD8,

    NUMBER_OF_FIELDS
};


/// A container for several SCH_CMP_FIELD items
typedef std::vector<SCH_CMP_FIELD>  SCH_CMP_FIELDS;


/**
 * Class SCH_COMPONENT
 * describes a real schematic component
 */
class SCH_COMPONENT : public SCH_ITEM
{
    friend class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC;

public:
    int             m_Multi;     // In multi unit chip - which unit to draw.

    wxPoint         m_Pos;

    wxString        m_ChipName;  /* Key to look for in the library,
                                  * i.e. "74LS00". */

    wxString        m_PrefixString;   /* C, R, U, Q etc - the first character
                                       * which typically indicates what the
                                       * component is. Determined, upon
                                       * placement, from the library component.
                                       * determined, upon file load, by the
                                       * first non-digits in the reference
                                       * fields. */

    int             m_Convert;        /* Handle mutiple shape (for instance
                                       * De Morgan conversion) */
    int             m_Transform[2][2]; /* The rotation/mirror transformation
                                        * matrix. */

private:

    SCH_CMP_FIELDS  m_Fields;         ///< variable length list of fields


    /* Hierarchical references.
     * format is
     * path reference multi
     * with:
     * path = /<timestamp1>/<timestamp2> (subsheet path, = / for the root scheet)
     * reference = reference for this path (C23, R5, U78 ... )
     * multi = part selection in multi parts per package (0 or 1 for one part per package)
     */
    wxArrayString m_PathsAndReferences;

public:
    SCH_COMPONENT( const wxPoint& pos = wxPoint( 0, 0 ),
                   SCH_ITEM* aParent = NULL );

    /**
     * Copy Constructor
     * clones \a aTemplate into this object.  All fields are copied as is except
     * for the linked list management pointers which are set to NULL, and the
     * SCH_CMP_FIELD's m_Parent pointers which are set to the new parent,
     * i.e. this new object.
     */
    SCH_COMPONENT( const SCH_COMPONENT& aTemplate );

    ~SCH_COMPONENT() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_COMPONENT" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool    Save( FILE* aFile ) const;


    /**
     * Function Load
     * reads a component in from a file.  The file stream must be positioned at
     * the first field of the file, not at the component tag.
     * @param aFile The FILE to read from.
     * @throw Error containing the error message text if there is a file format
     *   error or if the disk read has failed.
    void            Load( FILE* aFile ) throw( Error );
     */

    /**
     * Function GenCopy
     * returns a copy of this object but with the linked list pointers
     * set to NULL.
     * @return SCH_COMPONENT* - a copy of me.
     */
    SCH_COMPONENT*  GenCopy()
    {
        return new SCH_COMPONENT( *this );
    }

    void            SetRotationMiroir( int type );
    /** function GetRotationMiroir()
     * Used to display component orientation (in dialog editor or info)
     * @return the orientation and mirror
     * Note: Because there are different ways to have a given orientation/mirror,
     * the orientation/mirror is not necessary wht the used does
     * (example : a mirrorX then a mirrorY give no mirror but rotate the component).
     * So this function find a rotation and a mirror value
     * ( CMP_MIROIR_X because this is the first mirror option tested)
     *  but can differs from the orientation made by an user
     * ( a CMP_MIROIR_Y is find as a CMP_MIROIR_X + orientation 180, because they are equivalent)
     */
    int             GetRotationMiroir();

    wxPoint         GetScreenCoord( const wxPoint& coord );
    void            DisplayInfo( WinEDA_DrawFrame* frame );

    /**
     * Suppress annotation ( i.i IC23 changed to IC? and part reset to 1)
     * @param aSheet: DrawSheetPath value: if NULL remove all annotations,
     *             else remove annotation relative to this sheetpath
     */
    void            ClearAnnotation( DrawSheetPath* aSheet );

    /** function SetTimeStamp
     * Change the old time stamp to the new time stamp.
     * the time stamp is also modified in paths
     * @param aNewTimeStamp = new time stamp
     */
    void            SetTimeStamp( long aNewTimeStamp);

    EDA_Rect        GetBoundaryBox() const;
    EDA_Rect        GetBoundingBox();

    /**
     * Function ReturnFieldName
     * returns the Field name given a field index like (REFERENCE, VALUE ..)
     * @reeturn wxString - the field name or wxEmptyString if invalid field
     *                     index.
     */
    wxString ReturnFieldName( int aFieldNdx ) const;

    /**
     * Function GetField
     * returns a field.
     * @param aFieldNdx An index into the array of fields
     * @return SCH_CMP_FIELD* - the field value or NULL if does not exist
     */
    SCH_CMP_FIELD* GetField( int aFieldNdx ) const;

    /**
     * Function AddField
     * adds a field to the component.  The field is copied as it is put into
     * the component.
     * @param aField A const reference to the SCH_CMP_FIELD to add.
     */
    void AddField( const SCH_CMP_FIELD& aField );

    void SetFields( const SCH_CMP_FIELDS& aFields )
    {
        m_Fields = aFields;     // vector copying, length is changed possibly
    }


    /**
     * Function GetFieldCount
     * returns the number of fields in this component.
     */
    int GetFieldCount() const { return (int) m_Fields.size(); }

    virtual void            Draw( WinEDA_DrawPanel* panel,
                                  wxDC*             DC,
                                  const wxPoint&    offset,
                                  int               draw_mode,
                                  int               Color = -1 );

    void                    SwapData( SCH_COMPONENT* copyitem );

    void                    Place( WinEDA_SchematicFrame* frame, wxDC* DC );

    // returns a unique ID, in the form of a path.
    wxString                GetPath( DrawSheetPath* sheet );

    /**
     * Function GetRef
     * returns the reference, for the given sheet path.
     */
    const wxString          GetRef( DrawSheetPath* sheet );

    // Set the reference, for the given sheet path.
    void                    SetRef( DrawSheetPath* sheet, const wxString& ref );

    /**
     * Function AddHierarchicalReference
     * adds a full hierachical reference (path + local reference)
     * @param aPath = hierarchical path (/<sheet timestamp>/component
     *                timestamp> like /05678E50/A23EF560)
     * @param aRef = local reference like C45, R56
     * @param aMulti = part selection, used in multi part per package (0 or 1
     *                 for non multi)
     */
    void                    AddHierarchicalReference( const wxString& aPath,
                                                      const wxString& aRef,
                                                      int aMulti );

    //returns the unit selection, for the given sheet path.
    int                     GetUnitSelection( DrawSheetPath* aSheet );

    //Set the unit selection, for the given sheet path.
    void                    SetUnitSelection( DrawSheetPath* aSheet,
                                              int aUnitSelection );

#if defined (DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void                    Show( int nestLevel, std::ostream& os );

#endif
};


#endif /* COMPONENT_CLASS_H */
