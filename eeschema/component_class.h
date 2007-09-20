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


/* Definition de la representation du composant */
#define NUMBER_OF_FIELDS 12 /* Nombre de champs de texte affectes au composant */
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
    FIELD8
};


/* Class to manage component fields.
 *  component fields are texts attached to the component (not the graphic texts)
 *  There are 2 major fields : Reference and Value
 */
class PartTextStruct :  public EDA_BaseStruct, public EDA_TextStruct
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
        return wxT( "PartText" );
    }


    void        PartTextCopy( PartTextStruct* target );
    void        Place( WinEDA_DrawFrame* frame, wxDC* DC );

    EDA_Rect    GetBoundaryBox();
    bool        IsVoid();
    void        SwapData( PartTextStruct* copyitem );
};


/* the class DrawPartStruct describes a basic virtual component
 *  Not used directly:
 *  used classes are EDA_SchComponentStruct (the "classic" schematic component
 *  and the Pseudo component DrawSheetStruct
 */
class DrawPartStruct : public EDA_BaseStruct
{
public:
    int            m_Layer;
    wxString       m_ChipName;  /* Key to look for in the library, i.e. "74LS00". */
    PartTextStruct m_Field[NUMBER_OF_FIELDS];
    wxPoint        m_Pos;       /* Exact position of part. */

public:
    DrawPartStruct( KICAD_T struct_type, const wxPoint &pos );
    ~DrawPartStruct();
    
    virtual wxString GetClass() const
    {
        return wxT( "DrawPart" );
    }

    
    /**
     * Function GetReference
     * returns a reference to the Reference
     */
    const wxString& GetReference() { return m_Field[REFERENCE].m_Text; }
};


/* the class EDA_SchComponentStruct describes a real component */
class EDA_SchComponentStruct : public DrawPartStruct
{
public:
    int   m_RefIdNumber;        /* reference count: for U1, R2 .. it is the 1 or 2 value */
    int   m_Multi;              /* In multi unit chip - which unit to draw. */
    int   m_FlagControlMulti;
    int   m_Convert;            /* Gestion des mutiples representations (ex: conversion De Morgan) */
    int   m_Transform[2][2];    /* The rotation/mirror transformation matrix. */
    bool* m_PinIsDangling;      // liste des indicateurs de pin non connectee

public:
    EDA_SchComponentStruct( const wxPoint& pos = wxPoint( 0, 0 ) );
    ~EDA_SchComponentStruct( void ) { }
    
    virtual wxString GetClass() const
    {
        return wxT( "EDA_SchComponent" );
    }


    EDA_SchComponentStruct* GenCopy();
    void                    SetRotationMiroir( int type );
    int                     GetRotationMiroir();
    wxPoint                 GetScreenCoord( const wxPoint& coord );
    void                    Display_Infos( WinEDA_DrawFrame* frame );
    void                    ClearAnnotation();
    EDA_Rect                GetBoundaryBox();

    const wxString&         ReturnFieldName( int aFieldNdx ) const;
    

    /**
     * Function GetFieldValue
     * returns a reference to the field value.
     * @param aFieldNdx An index into the array of fields, 0 - FIELD8
     * @return const wxString& - the field value or wxEmptyString
     */
    const wxString&         GetFieldValue( int aFieldNdx ) const;

    
    virtual void            Draw( WinEDA_DrawPanel* panel,
                                  wxDC* DC,
                                  const wxPoint& offset,
                                  int draw_mode,
                                  int Color = -1 );
    void                    SwapData( EDA_SchComponentStruct* copyitem );

    virtual void            Place( WinEDA_DrawFrame* frame, wxDC* DC );
    
#if defined(DEBUG)    
    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level 
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void Show( int nestLevel, std::ostream& os );
#endif    
};


#endif /* COMPONENT_CLASS_H */
