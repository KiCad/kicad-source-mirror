/*************************************************************/
/* Definitions for the component fields classes for EESchema */
/*************************************************************/

#ifndef CLASS_SCH_FIELD_H
#define CLASS_SCH_FIELD_H

/* Fields are texts attached to a component, having a special meaning
 * Fields 0 and 1 are very important: reference and value
 * Field 2 is used as default footprint name.
 * Field 3 is reserved (not currently used
 * Fields 4 and more are user fields.
 * They can be renamed and can appear in reports
 */


class SCH_COMPONENT;
class LIB_FIELD;


/**
 * Class SCH_FIELD
 * instances are attached to a component and provide a place for the
 * component's value,
 * reference designator, footprint, and user definable name-value pairs of
 * arbitrary purpose.
 */
class SCH_FIELD : public SCH_ITEM, public EDA_TextStruct
{
public:
    int      m_FieldId;         /* Field indicator type (REFERENCE, VALUE or
                                 * other id) */

    wxString m_Name;            /* Field name (ref, value,pcb, sheet, filed 1..
                                 *  and for fields 1 to 8 the name is editable
                                 */

    bool     m_AddExtraText;    /* Mainly for REFERENCE, add extra info
                                 * (for REFERENCE: add part selection text */

public:
    SCH_FIELD( const wxPoint& aPos,  int aFieldId, SCH_COMPONENT* aParent,
               wxString aName = wxEmptyString );
    ~SCH_FIELD();

    virtual wxString GetClass() const
    {
        return wxT( "SCH_FIELD" );
    }


    void     Place( WinEDA_SchematicFrame* frame, wxDC* DC );

    EDA_Rect GetBoundaryBox() const;
    bool     IsVoid();
    void     SwapData( SCH_FIELD* copyitem );

    /** Function ImportValues
     * copy parameters from a source.
     * Pointers and specific values (position) are not copied
     * @param aSource = the LIB_FIELD to read
     */
    void     ImportValues( const LIB_FIELD& aSource );

    /** Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    int GetPenSize( );

    /** Function IsVisible
     * @return true is this field is visible, false if flagged invisible
     */
    bool IsVisible()
    {
        return (m_Attributs & TEXT_NO_VISIBLE) == 0 ? true : false;
    }

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
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    // Geometric transforms (used in block operations):
    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move(const wxPoint& aMoveVector)
    {
        m_Pos += aMoveVector;
    }

    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y(int aYaxis_position)
    {
        /* Do Nothing: fields are never mirrored alone.
         * they are moved when the parent component is mirrored
         * this function is only needed by the virtual pure function of the
         * master class */
    }

    /**
     * Compare schematic field text against search string.
     *
     * @param aSearchData - Criteria to search against.
     * @return True if this field text matches the search criteria.
     */
    virtual bool Matches( wxFindReplaceData& aSearchData );
};


#endif /* CLASS_SCH_FIELD_H */
