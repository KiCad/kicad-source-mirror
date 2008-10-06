/*****************************************************/
/* Definitions for the Component classes for EESchema */
/*****************************************************/

#ifndef COMPONENT_CLASS_H
#define COMPONENT_CLASS_H

#ifndef eda_global
#define eda_global extern
#endif

#include "base_struct.h"
#include "class_screen.h"
#include <wx/arrstr.h>
#include <wx/dynarray.h>

#include "class_sch_cmp_field.h"


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

/* Fields identifiers */
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


/**
 * Class SCH_COMPONENT
 * describes a real schematic component
 */
class SCH_COMPONENT : public SCH_ITEM
{
public:
    int               m_Multi; /* In multi unit chip - which unit to draw. */

    wxPoint           m_Pos;

    wxString          m_ChipName;  /* Key to look for in the library, i.e. "74LS00". */
    wxString          m_PrefixString;   /* C, R, U, Q etc - the first character which typically indicates what the component is.
                                         * determined, upon placement, from the library component.
                                         * determined, upon file load, by the first non-digits in the reference fields. */

    int               m_Convert;                    /* Gestion (management) des mutiples representations (ex: conversion De Morgan) */
    int               m_Transform[2][2];            /* The rotation/mirror transformation matrix. */

private:

/** how many fields are fixed, or automatic and pre-made in the SCH_COMPONENT class */

    typedef std::vector<SCH_CMP_FIELD>  SCH_CMP_FIELDS;
    SCH_CMP_FIELDS    m_Fields;


    /* Hierarchical references.
     * format is
     * path reference multi
     * with:
     * path = /<timestamp1>/<timestamp2> (subsheet path, = / for the root scheet)
     * reference = reference for this path (C23, R5, U78 ... )
     * multi = part selection in multi parts per package (0 or 1 for àne part per package)
     */
    wxArrayString m_PathsAndReferences;

public:
    SCH_COMPONENT( const wxPoint& pos = wxPoint( 0, 0 ), SCH_ITEM* aParent = NULL );
    ~SCH_COMPONENT() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_COMPONENT" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool    Save( FILE* aFile ) const;


    /**
     * Function Load
     * reads a component in from a file.  The file stream must be positioned at the
     * first field of the file, not at the component tag.
     * @param aFile The FILE to read from.
     * @throw Error containing the error message text if there is a file format
     *   error or if the disk read has failed.
    void            Load( FILE* aFile ) throw( Error );
     */

    SCH_COMPONENT*  GenCopy();
    void            SetRotationMiroir( int type );
    int             GetRotationMiroir();
    wxPoint         GetScreenCoord( const wxPoint& coord );
    void            Display_Infos( WinEDA_DrawFrame* frame );

    /**
     * Suppress annotation ( i.i IC23 changed to IC? and part reset to 1)
     * @param aSheet: DrawSheetPath value: if NULL remove all annotations,
     *             else remove annotation relative to this sheetpath
     */
    void            ClearAnnotation( DrawSheetPath* aSheet );

    EDA_Rect        GetBoundaryBox() const;
    EDA_Rect        GetBoundingBox();

    /**
     * Function ReturnFieldName
     * returns the Field name given a field index like (REFERENCE, VALUE ..)
     * @reeturn wxString - the field name or wxEmptyString if invalid field index.
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
     * adds a field to the component.  The component takes over ownership
     * of the field.
     * @param aField A const reference to the SCH_CMP_FIELD to add.
     */
    void AddField( const SCH_CMP_FIELD& aField );

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


    //returns a unique ID, in the form of a path.
    wxString                GetPath( DrawSheetPath* sheet );
    //returns the reference, for the given sheet path.

    const wxString          GetRef( DrawSheetPath* sheet );

    //Set the reference, for the given sheet path.
    void                    SetRef( DrawSheetPath* sheet, const wxString& ref );

    /**
     * Function AddHierarchicalReference
     * adds a full hierachical reference (path + local reference)
     * @param aPath = hierarchical path (/<sheet timestamp>/component timestamp> like /05678E50/A23EF560)
     * @param aRef = local reference like C45, R56
     * @param aMulti = part selection, used in multi part per package (0 or 1 for non multi)
     */
    void                    AddHierarchicalReference( const wxString& aPath, const wxString& aRef, int aMulti );

    //returns the unit selection, for the given sheet path.
    int                     GetUnitSelection( DrawSheetPath* aSheet );

    //Set the unit selection, for the given sheet path.
    void                    SetUnitSelection( DrawSheetPath* aSheet, int aUnitSelection );

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
