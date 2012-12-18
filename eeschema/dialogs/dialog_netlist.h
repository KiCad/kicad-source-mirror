/**
 * @file eeschema/dialogs/dialog_netlist.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef _DIALOG_NETLIST_H_
#define _DIALOG_NETLIST_H_

#include <dialogs/dialog_netlist_base.h>

#define CUSTOMPANEL_COUNTMAX 8  // Max number of netlist plugins

// Id to select netlist type
enum  NETLIST_TYPE_ID {
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

/* panel (notebook page) identifiers */
enum panel_netlist_index {
    PANELPCBNEW = 0,    /* Handle Netlist format Pcbnew */
    PANELORCADPCB2,     /* Handle Netlist format OracdPcb2 */
    PANELCADSTAR,       /* Handle Netlist format CadStar */
    PANELSPICE,         /* Handle Netlist format Pspice */
    PANELCUSTOMBASE     /* First auxiliary panel (custom netlists).
                         * others use PANELCUSTOMBASE+1, PANELCUSTOMBASE+2.. */
};

// Values returned when the netlist dialog is dismissed
#define NET_PLUGIN_CHANGE 1
// other values in use are wxID_OK and wxID_CANCEL



/* wxPanels for creating the NoteBook pages for each netlist format: */
class NETLIST_PAGE_DIALOG : public wxPanel
{
public:
    NETLIST_TYPE_ID   m_IdNetType;
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
     */
    NETLIST_PAGE_DIALOG( wxNotebook* parent, const wxString& title,
                         NETLIST_TYPE_ID id_NetType );
    ~NETLIST_PAGE_DIALOG() { };

    /**
     * function GetPageNetFmtName
     * @return the name of the netlist format for this page
     * This is usually the page label.
     * For the pcbnew netlist, this is "LegacyPcbnew"
     * when the "old" format is selected
     * and "PcbnewAdvanced" when the advanced format (S expr fmt)is selected
     */
    const wxString GetPageNetFmtName();

    void SetPageNetFmtName( const wxString &aName ) { m_pageNetFmtName = aName; }
};


// Options for Spice netlist generation (OR'ed bits
enum netlistOptions {
    NET_USE_NETNAMES = 1,           // for Spice netlist : use netnames instead of numbers
    NET_USE_X_PREFIX = 2,           // for Spice netlist : change "U" and "IC" reference prefix to "X"
    NET_PCBNEW_USE_NEW_FORMAT = 1,  // For Pcbnew use the new format (S expression and SWEET)
};

/* Dialog frame for creating netlists */
class NETLIST_DIALOG : public NETLIST_DIALOG_BASE
{
public:
    SCH_EDIT_FRAME*   m_Parent;
    wxString          m_NetFmtName;
    NETLIST_PAGE_DIALOG* m_PanelNetType[4 + CUSTOMPANEL_COUNTMAX];

private:
    wxConfig* m_config;
    bool      m_spiceNetlistUseNames;   /* true to use names rather than net
                                         * numbers (PSPICE netlist only) */

public:

    // Constructor and destructor
    NETLIST_DIALOG( SCH_EDIT_FRAME* parent );
    ~NETLIST_DIALOG() { };

private:
    void    InstallCustomPages();
    NETLIST_PAGE_DIALOG* AddOneCustomPage( const wxString & aTitle,
                                           const wxString & aCommandString,
                                           NETLIST_TYPE_ID aNetTypeId );
    void    InstallPageSpice();
    void    GenNetlist( wxCommandEvent& event );
    void    RunSimulator( wxCommandEvent& event );
    void    NetlistUpdateOpt();
    void    OnCancelClick( wxCommandEvent& event );
    void    OnNetlistTypeSelection( wxNotebookEvent& event );
    void    SelectDefaultNetlistType( wxCommandEvent& event );
    void    EnableSubcircuitPrefix( wxCommandEvent& event );
	void    OnAddPlugin( wxCommandEvent& event );
    void    OnDelPlugin( wxCommandEvent& event );

    void WriteCurrentNetlistSetup( void );

    bool GetUseDefaultNetlistName()
    {
        return m_cbUseDefaultNetlistName->IsChecked();
    }

    /**
     * Function ReturnUserNetlistTypeName
     * to retrieve user netlist type names
     * @param first_item = true: return first name of the list, false = return next
     * @return a wxString : name of the type netlist or empty string
     * this function must be called first with "first_item" = true
     * and after with "first_item" = false to get all the other existing netlist names
     */
    const wxString ReturnUserNetlistTypeName( bool first_item );

    /**
     * Function ReturnFilenamePrms
     * returns the filename extension and the wildcard string for this curr
     * or a void name if there is no default name
     * @param aNetTypeId = the netlist type ( NET_TYPE_PCBNEW ... )
     * @param aExt = a reference to a wxString to return the default  file ext.
     * @param aWildCard =  reference to a wxString to return the default wildcard.
     * @return true for known netlist type, false for custom formats
     */
    bool ReturnFilenamePrms( NETLIST_TYPE_ID aNetTypeId,
                             wxString * aExt, wxString * aWildCard );

    DECLARE_EVENT_TABLE()
};


class NETLIST_DIALOG_ADD_PLUGIN : public NETLIST_DIALOG_ADD_PLUGIN_BASE
{
private:
   NETLIST_DIALOG* m_Parent;

public:
    NETLIST_DIALOG_ADD_PLUGIN( NETLIST_DIALOG* parent );
    const wxString GetPluginTitle()
    {
        return m_textCtrlName->GetValue();
    }
    const wxString GetPluginTCommandLine()
    {
        return m_textCtrlCommand->GetValue();
    }

private:
    void OnOKClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnBrowsePlugins( wxCommandEvent& event );
};

#endif  /* _DIALOG_NETLIST_H_ */
