
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

    /// a copy of the edited component's SCH_CMP_FIELDs
    SCH_CMP_FIELDS          m_FieldBuf;

    void setSelectedFieldNdx( int aFieldNdx );

    int getSelectedFieldNdx();


    /**
     * Function CopyDataToPanel
     * sets the values displayed on the panel according to
     * the current field number
     */
    void copyDataToPanel();


    void fillTableModel();

protected:

    // Handlers for DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP events.
//    void OnGridCellLeftClick( wxGridEvent& event );

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
