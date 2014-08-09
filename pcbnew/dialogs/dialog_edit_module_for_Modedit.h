/*****************************************************************************/
/* Module editor: Dialog box for editing module	properties and carateristics */
/*****************************************************************************/

#ifndef __DIALOG_EDIT_MODULE_FOR_MODEDIT__
#define __DIALOG_EDIT_MODULE_FOR_MODEDIT__

// Include the wxFormBuider header base:
#include <vector>
#include <dialog_edit_module_for_Modedit_base.h>

/**************************************/
/* class DIALOG_MODULE_MODULE_EDITOR */
/**************************************/

class DIALOG_MODULE_MODULE_EDITOR : public DIALOG_MODULE_MODULE_EDITOR_BASE
{
private:

    FOOTPRINT_EDIT_FRAME*   m_parent;
    MODULE* m_currentModule;
    TEXTE_MODULE* m_referenceCopy;
    TEXTE_MODULE* m_valueCopy;
    std::vector <S3D_MASTER*> m_shapes3D_list;
    int m_lastSelected3DShapeIndex;
    S3DPOINT_VALUE_CTRL * m_3D_Scale;
    S3DPOINT_VALUE_CTRL * m_3D_Offset;
    S3DPOINT_VALUE_CTRL * m_3D_Rotation;

public:

    // Constructor and destructor
    DIALOG_MODULE_MODULE_EDITOR( FOOTPRINT_EDIT_FRAME* aParent, MODULE* aModule );
    ~DIALOG_MODULE_MODULE_EDITOR();

private:
    void initModeditProperties();
    void Transfert3DValuesToDisplay( S3D_MASTER * aStruct3DSource );
    void TransfertDisplayTo3DValues( int aIndexSelection );
    void OnEditValue( wxCommandEvent& event );
    void OnEditReference( wxCommandEvent& event );
    void On3DShapeSelection( wxCommandEvent& event );
    void On3DShapeNameSelected( wxCommandEvent& event );
    void BrowseAndAdd3DLib( wxCommandEvent& event );
    void Remove3DShape( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
};


#endif      //  __DIALOG_EDIT_MODULE_FOR_MODEDIT__
