/*****************************************************************************
 *
 * netlist_control.h
 *
 *****************************************************************************/

#ifndef _NETLIST_CONTROL_H_
#define _NETLIST_CONTROL_H_

/* Event id for notebook page buttons: */
enum id_netlist {
    ID_CREATE_NETLIST = 1550,
    ID_CURRENT_FORMAT_IS_DEFAULT,
    ID_RUN_SIMULATOR,
    ID_SETUP_PLUGIN,
    ID_VALIDATE_PLUGIN,
    ID_DELETE_PLUGIN,
    ID_NETLIST_NOTEBOOK
};

/* panel (notebook page) identifiers */
enum panel_netlist_index {
    PANELPCBNEW = 0,    /* Handle Netlist format Pcbnew */
    PANELORCADPCB2,     /* Handle Netlist format OracdPcb2 */
    PANELCADSTAR,       /* Handle Netlist format CadStar */
    PANELSPICE,         /* Handle Netlist format Pspice */
    PANELCUSTOMBASE     /* First auxiliary panel (custom netlists).
                         * others use PANELCUSTOMBASE+1, PANELCUSTOMBASE+2.. */
};

/* Values returned when the netlist dialog is demiss */
enum gen_netlist_diag {
    NET_OK,
    NET_ABORT,
    NET_PLUGIN_CHANGE
};


/* wxPanels for creating the NoteBook pages for each netlist format: */
class EDA_NoteBookPage : public wxPanel
{
public:
    int               m_IdNetType;
    wxCheckBox*       m_IsCurrentFormat;
    WinEDA_EnterText* m_CommandStringCtrl;
    WinEDA_EnterText* m_TitleStringCtrl;
    wxButton*         m_ButtonCancel;
    wxBoxSizer*       m_LeftBoxSizer;
    wxBoxSizer*       m_RightBoxSizer;
    wxBoxSizer*       m_RightOptionsBoxSizer;
    wxBoxSizer*       m_LowBoxSizer;

    /** Contructor to create a setup page for one netlist format.
     * Used in Netlist format Dialog box creation
     * @param parent = wxNotebook * parent
     * @param title = title (name) of the notebook page
     * @param id_NetType = netlist type id
     * @param idCheckBox = event ID attached to the "format is default" check box
     * @param idCreateFile = event ID attached to the "create netlist" button
     * @param selected = true to have this notebook page selected when the dialog is opened
     *    Only one page can be created with selected = true.
     */
    EDA_NoteBookPage( wxNotebook* parent, const wxString& title,
                      int id_NetType, int idCheckBox, int idCreateFile,
                      bool selected );
    ~EDA_NoteBookPage() { };
};


/* Dialog frame for creating netlists */
class WinEDA_NetlistFrame : public wxDialog
{
public:
    SCH_EDIT_FRAME*   m_Parent;
    wxNotebook*       m_NoteBook;
    EDA_NoteBookPage* m_PanelNetType[4 + CUSTOMPANEL_COUNTMAX];
    wxRadioBox*       m_UseNetNamesInNetlist;

public:

    // Constructor and destructor
    WinEDA_NetlistFrame( SCH_EDIT_FRAME* parent );
    ~WinEDA_NetlistFrame() { };

private:
    void    InstallCustomPages();
    void    InstallPageSpice();
    void    GenNetlist( wxCommandEvent& event );
    void    RunSimulator( wxCommandEvent& event );
    void    NetlistUpdateOpt();
    void    OnCancelClick( wxCommandEvent& event );
    void    SelectNetlistType( wxCommandEvent& event );
    void    SetupPluginData( wxCommandEvent& event );
    void    DeletePluginPanel( wxCommandEvent& event );
    void    ValidatePluginPanel( wxCommandEvent& event );

    void WriteCurrentNetlistSetup( void );

    DECLARE_EVENT_TABLE()
};

#endif  /* _NETLIST_CONTROL_H_ */
