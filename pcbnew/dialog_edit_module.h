    /************************************************/
    /* Module editor: Dialog box for editing module	*/
    /*  properties and carateristics				*/
    /************************************************/

enum id_Module_properties
{
    ID_GOTO_MODULE_EDITOR =1900,
    ID_MODULE_PROPERTIES_EXCHANGE,
    ID_MODULE_EDIT_ADD_TEXT,
    ID_MODULE_EDIT_EDIT_TEXT,
    ID_MODULE_EDIT_DELETE_TEXT,
    ID_MODULE_LISTBOX_SELECT,
    ID_LISTBOX_ORIENT_SELECT,
    ID_BROWSE_3D_LIB,
    ID_ADD_3D_SHAPE,
    ID_REMOVE_3D_SHAPE,
    ID_NOTEBOOK,
    ID_MODULE_EDIT_X_POSITION,
    ID_MODULE_EDIT_Y_POSITION
};

class Panel3D_Ctrl;

    /**************************************/
    /* class WinEDA_ModulePropertiesFrame */
    /**************************************/

class WinEDA_ModulePropertiesFrame: public wxDialog
{
private:

    WinEDA_BasePcbFrame * m_Parent;
    wxDC * m_DC;
    MODULE * m_CurrentModule;
    wxNotebook* m_NoteBook;
    wxPanel * m_PanelProperties;
    Panel3D_Ctrl * m_Panel3D;
    WinEDAChoiceBox * m_TextListBox;
    wxRadioBox * m_LayerCtrl;
    wxRadioBox * m_OrientCtrl;
    wxTextCtrl * m_OrientValue;
    wxRadioBox * m_AttributsCtrl;
    wxRadioBox * m_AutoPlaceCtrl;
    wxSlider * m_CostRot90Ctrl, * m_CostRot180Ctrl;
    wxButton * m_DeleteFieddButton;
    wxTextCtrl *m_Doc, *m_Keyword;
    wxBoxSizer * m_GeneralBoxSizer;
    wxBoxSizer* m_PanelPropertiesBoxSizer;
    wxTextCtrl *m_ModPositionX, *m_ModPositionY;
    wxString *m_ModPosXStr, *m_ModPosYStr;


public:
    // Constructor and destructor
    WinEDA_ModulePropertiesFrame(WinEDA_BasePcbFrame *parent,
                            MODULE * Module, wxDC * DC, const wxPoint & pos);
    ~WinEDA_ModulePropertiesFrame()
        {
        }

private:
    void CreateControls();
    void OnCancelClick(wxCommandEvent& event);
    void OnOkClick(wxCommandEvent& event);
    void CreateTextModule(wxCommandEvent& event);
    void EditOrDelTextModule(wxCommandEvent& event);
    void SelectTextListBox(wxCommandEvent& event);
    void ReCreateFieldListBox();
    void SetTextListButtons();
    void BuildPanelModuleProperties(bool FullOptions);
    void ModuleOrientEvent(wxCommandEvent& event);
    void ExchangeModule(wxCommandEvent& event);
    void GotoModuleEditor(wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
};


/*********************************/
class Panel3D_Ctrl: public wxPanel
/*********************************/
/* panel d'entree des caract 3D */
{
public:
    Panel3D_Ctrl * m_Pnext, * m_Pback;	// Chainage
    wxNotebook * m_Parent;
    WinEDA_ModulePropertiesFrame * m_ParentFrame;
    wxBoxSizer* m_Panel3DBoxSizer;
    wxTextCtrl * m_3D_ShapeName;
    WinEDA_VertexCtrl *m_3D_Scale, *m_3D_Offset, *m_3D_Rotation;
public:
    Panel3D_Ctrl(WinEDA_ModulePropertiesFrame * parentframe,
        wxNotebook * parent, int id, S3D_MASTER * struct3D);
    ~Panel3D_Ctrl();
    void Browse3DLib(wxCommandEvent& event);
    void AddOrRemove3DShape(wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
};

