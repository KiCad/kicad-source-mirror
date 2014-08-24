    /************************************************/
    /* Module editor: Dialog box for editing module	*/
    /*  properties and carateristics				*/
    /************************************************/

#include <dialog_edit_module_for_BoardEditor_base.h>

    /**************************************/
    /* class DIALOG_MODULE_BOARD_EDITOR */
    /**************************************/

class DIALOG_MODULE_BOARD_EDITOR: public DIALOG_MODULE_BOARD_EDITOR_BASE
{
private:

    PCB_EDIT_FRAME * m_Parent;
    wxDC * m_DC;
    MODULE* m_CurrentModule;
    TEXTE_MODULE* m_ReferenceCopy;
    TEXTE_MODULE* m_ValueCopy;
    std::vector <S3D_MASTER*>   m_Shapes3D_list;
    int m_LastSelected3DShapeIndex;
    S3DPOINT_VALUE_CTRL * m_3D_Scale;
    S3DPOINT_VALUE_CTRL * m_3D_Offset;
    S3DPOINT_VALUE_CTRL * m_3D_Rotation;

public:

    // Constructor and destructor
    DIALOG_MODULE_BOARD_EDITOR( PCB_EDIT_FRAME* aParent, MODULE* aModule, wxDC* aDC );
    ~DIALOG_MODULE_BOARD_EDITOR();

private:
    void BrowseAndAdd3DShapeFile();
    void InitBoardProperties();
    void InitModeditProperties();
    void Transfert3DValuesToDisplay( S3D_MASTER * aStruct3DSource );
    void TransfertDisplayTo3DValues( int aIndexSelection );

    // virtual event functions
    void OnEditValue( wxCommandEvent& event );
    void OnEditReference( wxCommandEvent& event );
    void On3DShapeSelection( wxCommandEvent& event );
    void On3DShapeNameSelected( wxCommandEvent& event );
    void Add3DShape( wxCommandEvent& event )
    {
        BrowseAndAdd3DShapeFile();
    }
    void Remove3DShape( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void GotoModuleEditor( wxCommandEvent& event );
    void ExchangeModule( wxCommandEvent& event );
    void ModuleOrientEvent( wxCommandEvent& event );
};

