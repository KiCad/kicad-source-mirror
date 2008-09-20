/*************************************************************/
/* Definitions for the component fields classes for EESchema */
/*************************************************************/

#ifndef CLASS_SCH_CMP_FIELD_H
#define CLASS_SCH_CMP_FIELD_H

/*Fields are texts attached to a component, having a specuial meaning
  * Fields 0 and 1 are very important: reference and value
  * Field 2 is used as default footprint name.
  * Field 3 is reserved (not currently used
  * Fields 4 to 11 are user fields.
  * They can be renamed and can appear in reports
 */

/* Fields identifiers */
enum  NumFieldType {
    REFERENCE = 0,          /* Field Reference of part, i.e. "IC21" */
    VALUE,                  /* Field Value of part, i.e. "3.3K" */
    FOOTPRINT,              /* Field Name Module PCB, i.e. "16DIP300" */
    SHEET_FILENAME,         /* Field Name Schema component, i.e. "cnt16.sch" */
    FIELD1,
    FIELD2,
    FIELD3,
    FIELD4,
    FIELD5,
    FIELD6,
    FIELD7,
    FIELD8,
    NUMBER_OF_FIELDS        /* used as Field count, not a field identifier */
};

/*************************************************************/
class SCH_CMP_FIELD :  public SCH_ITEM, public EDA_TextStruct
/*************************************************************/
{
public:
    int      m_FieldId;         // Field indicator type (REFERENCE, VALUE or other id)
    wxString m_Name;    /* Field name (ref, value,pcb, sheet, filed 1..
                         *  and for fields 1 to 8 the name is editable */
    bool     m_AddExtraText;    // Mainly for REFERENCE, add extar info (for REFERENCE: add part selection text

public:
    SCH_CMP_FIELD( const wxPoint& pos = wxPoint( 0, 0 ), const wxString& text = wxEmptyString );
    ~SCH_CMP_FIELD();

    virtual wxString GetClass() const
    {
        return wxT( "SCH_CMP_FIELD" );
    }


    void     PartTextCopy( SCH_CMP_FIELD* target );
    void     Place( WinEDA_SchematicFrame* frame, wxDC* DC );

    EDA_Rect GetBoundaryBox() const;
    bool     IsVoid();
    void     SwapData( SCH_CMP_FIELD* copyitem );

    /**
     * Function Draw
     */
    void     Draw( WinEDA_DrawPanel* panel,
                   wxDC*             DC,
                   const wxPoint&    offset,
                   int               draw_mode,
                   int               Color = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;
};


#endif /* CLASS_SCH_CMP_FIELD_H */
