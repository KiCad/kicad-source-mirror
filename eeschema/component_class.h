/*****************************************************/
/* Definitions for the Component classes for EESchema */
/*****************************************************/

#ifndef COMPONENT_CLASS_H
#define COMPONENT_CLASS_H

#ifndef eda_global
#define eda_global extern
#endif

#include "macros.h"
#include "base_struct.h"
#include <wx/arrstr.h>
#include "class_screen.h"
#include <wx/dynarray.h>

/* Definition de la representation du composant */
enum  NumFieldType {
    REFERENCE = 0,          /* Champ Reference of part, i.e. "IC21" */
    VALUE,                  /* Champ Value of part, i.e. "3.3K" */
    FOOTPRINT,              /* Champ Name Module PCB, i.e. "16DIP300" */
    SHEET_FILENAME,         /* Champ Name Schema component, i.e. "cnt16.sch" */
    FIELD1,
    FIELD2,
    FIELD3,
    FIELD4,
    FIELD5,
    FIELD6,
    FIELD7,
    FIELD8,
    NUMBER_OF_FIELDS        /* Nombre de champs de texte affectes au composant */
};


/* Class to manage component fields.
 *  component fields are texts attached to the component (not the graphic texts)
 *  There are 2 major fields : Reference and Value
 */
class PartTextStruct :  public SCH_ITEM,
    public EDA_TextStruct
{
public:
    int      m_FieldId;         // Field indicator type (REFERENCE, VALUE or other id)
    wxString m_Name;    /* Field name (ref, value,pcb, sheet, filed 1..
                         *  and for fields 1 to 8 the name is editable */
    bool     m_AddExtraText;    // Mainly for REFERENCE, add extar info (for REFERENCE: add part selection text

public:
    PartTextStruct( const wxPoint& pos = wxPoint( 0, 0 ), const wxString& text = wxEmptyString );
    ~PartTextStruct();

    virtual wxString GetClass() const
    {
        return wxT( "PartTextStruct" );
    }


    void        PartTextCopy( PartTextStruct* target );
    void        Place( WinEDA_SchematicFrame* frame, wxDC* DC );

    EDA_Rect    GetBoundaryBox() const;
    bool        IsVoid();
    void        SwapData( PartTextStruct* copyitem );

    /**
     * Function Draw
     */
    void        Draw( WinEDA_DrawPanel* panel,
                      wxDC*                 DC,
                      const wxPoint&        offset,
                      int                   draw_mode,
                      int                   Color = -1 );
    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool    Save( FILE* aFile ) const;
};


WX_DECLARE_OBJARRAY( DrawSheetPath, ArrayOfSheetLists );

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
    PartTextStruct    m_Field[NUMBER_OF_FIELDS];

    int               m_Convert;                    /* Gestion (management) des mutiples representations (ex: conversion De Morgan) */
    int               m_Transform[2][2];            /* The rotation/mirror transformation matrix. */

private:

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
    SCH_COMPONENT( const wxPoint& pos = wxPoint( 0, 0 ) );
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

    const wxString& ReturnFieldName( int aFieldNdx ) const;


    /**
     * Function GetFieldValue
     * returns a reference to the field value.
     * @param aFieldNdx An index into the array of fields, 0 - FIELD8
     * @return const wxString& - the field value or wxEmptyString
     */
    const wxString& GetFieldValue( int aFieldNdx ) const;


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
    /** Function AddHierarchicalReference
     * Add a full hierachical reference (path + local reference)
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
