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
    ID_NETLIST_NOTEBOOK,
    ID_CHANGE_NOTEBOOK_PAGE,
    ID_ADD_SUBCIRCUIT_PREFIX,
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
class NETLIST_PAGE_DIALOG : public wxPanel
{
public:
    int               m_IdNetType;
    wxCheckBox*       m_IsCurrentFormat;
    wxCheckBox*       m_AddSubPrefix;
    wxTextCtrl*       m_CommandStringCtrl;
    wxTextCtrl*       m_TitleStringCtrl;
    wxButton*         m_ButtonCancel;
    wxBoxSizer*       m_LeftBoxSizer;
    wxBoxSizer*       m_RightBoxSizer;
    wxBoxSizer*       m_RightOptionsBoxSizer;
    wxBoxSizer*       m_LowBoxSizer;
    wxRadioBox*       m_NetOption;
private:
    wxString          m_pageNetFmtName;

public:
    /** Constructor to create a setup page for one netlist format.
     * Used in Netlist format Dialog box creation
     * @param parent = wxNotebook * parent
     * @param title = title (name) of the notebook page
     * @param id_NetType = netlist type id
     * @param idCheckBox = event ID attached to the "format is default" check box
     * @param idCreateFile = event ID attached to the "create netlist" button
     * @param selected = true to have this notebook page selected when the dialog is opened
     *    Only one page can be created with selected = true.
     */
    NETLIST_PAGE_DIALOG( wxNotebook* parent, const wxString& title,
                         int id_NetType, int idCheckBox, int idCreateFile );
    ~NETLIST_PAGE_DIALOG() { };

    /**
     * function GetPageNetFmtName
     * @return the name of the netlist format for this page
     * This is usually the page label.
     * For the pcbnew netlist, this is the page label when the "old" format is selected
     * and "PcbnewAdvanced" when the advanced format is selected
     */
    const wxString GetPageNetFmtName();

    void SetPageNetFmtName( const wxString &aName ) { m_pageNetFmtName =aName; }
};


#define CUSTOMPANEL_COUNTMAX 8  // Max number of netlist plugins

// Id to select netlist type
enum  TypeNetForm {
    NET_TYPE_UNINIT = 0,
    NET_TYPE_PCBNEW,
    NET_TYPE_ORCADPCB2,
    NET_TYPE_CADSTAR,
    NET_TYPE_SPICE,
    NET_TYPE_CUSTOM1,   /* NET_TYPE_CUSTOM1
                         * is the first id for user netlist format
                         * NET_TYPE_CUSTOM1+CUSTOMPANEL_COUNTMAX-1
                         * is the last id for user netlist format
                         */
    NET_TYPE_CUSTOM_MAX = NET_TYPE_CUSTOM1 + CUSTOMPANEL_COUNTMAX - 1
};

// Options for Spice netlist generation (OR'ed bits
enum netlistOptions {
    NET_USE_NETNAMES = 1,           // for Spice netlist : use netnames instead of numbers
    NET_USE_X_PREFIX = 2,           // for Spice netlist : change "U" and "IC" reference prefix to "X"
    NET_PCBNEW_USE_NEW_FORMAT = 1,  // For Pcbnew use the new format (S expression and SWEET)
};

/* Dialog frame for creating netlists */
class NETLIST_DIALOG : public wxDialog
{
public:
    SCH_EDIT_FRAME*   m_Parent;
    wxString          m_NetFmtName;
    wxNotebook*       m_NoteBook;
    NETLIST_PAGE_DIALOG* m_PanelNetType[4 + CUSTOMPANEL_COUNTMAX];

public:

    // Constructor and destructor
    NETLIST_DIALOG( SCH_EDIT_FRAME* parent );
    ~NETLIST_DIALOG() { };

private:
    void    InstallCustomPages();
    void    InstallPageSpice();
    void    GenNetlist( wxCommandEvent& event );
    void    RunSimulator( wxCommandEvent& event );
    void    NetlistUpdateOpt();
    void    OnCancelClick( wxCommandEvent& event );
    void    SelectNetlistType( wxCommandEvent& event );
    void    EnableSubcircuitPrefix( wxCommandEvent& event );
    void    AddNewPluginPanel( wxCommandEvent& event );
    void    DeletePluginPanel( wxCommandEvent& event );
    void    ValidatePluginPanel( wxCommandEvent& event );

    void WriteCurrentNetlistSetup( void );

    DECLARE_EVENT_TABLE()
};

#endif  /* _NETLIST_CONTROL_H_ */
