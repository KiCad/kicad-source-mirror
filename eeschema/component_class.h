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
class PartTextStruct :  public EDA_BaseStruct,
    public EDA_TextStruct
{
public:
    int      m_Layer;
    int      m_FieldId;
    wxString m_Name;    /* Field name (ref, value,pcb, sheet, filed 1..
                         *  and for fields 1 to 8 the name is editable */

public:
    PartTextStruct( const wxPoint& pos = wxPoint( 0, 0 ), const wxString& text = wxEmptyString );
    ~PartTextStruct();

    virtual wxString GetClass() const
    {
        return wxT( "PartTextStruct" );
    }


    void        PartTextCopy( PartTextStruct* target );
    void        Place( WinEDA_DrawFrame* frame, wxDC* DC );

    EDA_Rect    GetBoundaryBox() const;
    bool        IsVoid();
    void        SwapData( PartTextStruct* copyitem );
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

    //int   m_FlagControlMulti;
    ArrayOfSheetLists m_UsedOnSheets;               // Used as flags when calculating netlist
    int               m_Convert;                    /* Gestion (management) des mutiples representations (ex: conversion De Morgan) */
    int               m_Transform[2][2];            /* The rotation/mirror transformation matrix. */

private:
    wxArrayString     m_Paths;                      // /sheet1/C102, /sh2/sh1/U32 etc.
    wxArrayString     m_References;                 // C102, U32 etc.
    wxArrayString     m_PartPerPackageSelections;   // "1", "2" etc. when a component has more than 1 partper package

public:
    SCH_COMPONENT( const wxPoint& pos = wxPoint( 0, 0 ) );
    ~SCH_COMPONENT() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_COMPONENT" );
    }

    /** Function Save
     * Write on file a SCH_COMPONENT decscription
     * @param f = output file
     * return an error: false if ok, true if error
    */
    bool Save( FILE *f );

    SCH_COMPONENT*  GenCopy();
    void                    SetRotationMiroir( int type );
    int                     GetRotationMiroir();
    wxPoint                 GetScreenCoord( const wxPoint& coord );
    void                    Display_Infos( WinEDA_DrawFrame* frame );
    void                    ClearAnnotation();
    EDA_Rect                GetBoundaryBox() const;
    EDA_Rect                GetBoundingBox();

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

    virtual void            Place( WinEDA_DrawFrame* frame, wxDC* DC );


    //returns a unique ID, in the form of a path.
    wxString                GetPath( DrawSheetPath* sheet );
    const wxString          GetRef( DrawSheetPath* sheet );
    void                    SetRef( DrawSheetPath* sheet, const wxString & ref );
    void                    ClearRefs();
    void                    AddHierarchicalReference(const wxString & path, const wxString & ref);
    int                     GetUnitSelection( DrawSheetPath* aSheet );
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
