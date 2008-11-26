
#ifndef __dialog_edit_component_in_schematic__
#define __dialog_edit_component_in_schematic__


#include "dialog_edit_component_in_schematic_fbp.h"

/**
 * class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC
 * is hand coded and implements DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP which is maintained by
 * wxFormBuilder.  Do not auto-generate this class or file, it is hand coded.
 */
class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC : public DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP
{
    WinEDA_SchematicFrame*  m_Parent;
    SCH_COMPONENT*          m_Cmp;
    EDA_LibComponentStruct* m_LibEntry;

    static int              s_SelectedRow;

    /// a copy of the edited component's SCH_CMP_FIELDs
    SCH_CMP_FIELDS          m_FieldsBuf;

    void setSelectedFieldNdx( int aFieldNdx );

    int getSelectedFieldNdx();

    /**
     * Function copySelectedFieldToPanel
     * sets the values displayed on the panel according to
     * the currently selected field row
     */
    void copySelectedFieldToPanel();


    /**
     * Function copyPanelToSelectedField
     * copies the values displayed on the panel fields to the currently selected field
     * @return bool - true if all fields are OK, else false if the user has put
     *   bad data into a field, and this value can be used to deny a row change.
     */
    bool copyPanelToSelectedField();

    void copyOptionsToPanel();

    void copyPanelToOptions();

    void setRowItem( int aFieldNdx, const SCH_CMP_FIELD& aField );

    // event handlers
    void OnListItemDeselected( wxListEvent& event );
    void OnListItemSelected( wxListEvent& event );
    void OnCancelButtonClick( wxCommandEvent& event );
    void OnOKButtonClick( wxCommandEvent& event );
    void SetInitCmp( wxCommandEvent& event );



protected:


public:
    /** Constructor */
    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC( wxWindow* parent );

    /**
     * Function InitBuffers
     * sets up to edit the given component.
     * @param aComponent The component to edit.
     */
    void InitBuffers( SCH_COMPONENT* aComponent );

};

#endif // __dialog_edit_component_in_schematic__
