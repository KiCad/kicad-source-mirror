/*************************************************************/
/* Definitions for the component fields classes for EESchema */
/*************************************************************/

#ifndef CLASS_SCH_CMP_FIELD_H
#define CLASS_SCH_CMP_FIELD_H

/* Fields are texts attached to a component, having a specuial meaning
 * Fields 0 and 1 are very important: reference and value
 * Field 2 is used as default footprint name.
 * Field 3 is reserved (not currently used
 * Fields 4 and more are user fields.
 * They can be renamed and can appear in reports
 */


class SCH_COMPONENT;


/**
 * Class SCH_CMP_FIELD
 * instances are attached to a component and provide a place for the component's value,
 * reference designator, footprint, and user definable name-value pairs of
 * arbitrary purpose.
 */
class SCH_CMP_FIELD :  public SCH_ITEM, public EDA_TextStruct
{
public:
    int      m_FieldId;         // Field indicator type (REFERENCE, VALUE or other id)

    wxString m_Name;            /* Field name (ref, value,pcb, sheet, filed 1..
                                 *  and for fields 1 to 8 the name is editable
                                 */

    bool     m_AddExtraText;    // Mainly for REFERENCE, add extra info (for REFERENCE: add part selection text

public:
    SCH_CMP_FIELD( const wxPoint& aPos,  int aFieldId, SCH_COMPONENT* aParent, wxString aName = wxEmptyString );
    ~SCH_CMP_FIELD();

    virtual wxString GetClass() const
    {
        return wxT( "SCH_CMP_FIELD" );
    }


    void     Place( WinEDA_SchematicFrame* frame, wxDC* DC );

    EDA_Rect GetBoundaryBox() const;
    bool     IsVoid();
    void     SwapData( SCH_CMP_FIELD* copyitem );

    /** Function ImportValues
     * copy parameters from a source.
     * Pointers and specific values (position) are not copied
     * @param aSource = the LibDrawField to read
     */
    void     ImportValues( const LibDrawField& aSource );

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
     * writes the data structures for this object out to a FILE in "*.sch" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;
};


#endif /* CLASS_SCH_CMP_FIELD_H */
