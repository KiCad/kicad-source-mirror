/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

/* Functions relative to the dialog creating the netlist for Pcbnew.  The dialog is a notebook
 * with 4 fixed netlist formats:
 *   Pcbnew
 *   ORCADPCB2
 *   CADSTAR
 *   SPICE
 * and up to CUSTOMPANEL_COUNTMAX user programmable formats.  These external converters are
 * referred to as plugins, but they are really just external binaries.
 */

#include <pgm_base.h>
#include <kiface_base.h>
#include <gestfich.h>
#include <widgets/wx_html_report_panel.h>
#include <sch_edit_frame.h>
#include <netlist.h>
#include <dialogs/dialog_export_netlist_base.h>
#include <wildcards_and_files_ext.h>
#include <invoke_sch_dialog.h>
#include <netlist_exporters/netlist_exporter_spice.h>
#include <eeschema_settings.h>
#include <schematic.h>
#include <paths.h>

#include <eeschema_id.h>
#include <wx/checkbox.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/regex.h>



#define CUSTOMPANEL_COUNTMAX 8  // Max number of netlist plugins


/* panel (notebook page) identifiers
 * if modified, fix also the DEFINED_NETLISTS_COUNT value
 * PANEL_NETLIST_INDEX values are used as index in m_PanelNetType[]
 */
enum PANEL_NETLIST_INDEX
{
    PANELPCBNEW = 0, /* Handle Netlist format Pcbnew */
    PANELORCADPCB2,  /* Handle Netlist format OracdPcb2 */
    PANELALLEGRO,    /* Handle Netlist format Allegro */
    PANELCADSTAR,    /* Handle Netlist format CadStar */
    PANELSPICE,      /* Handle Netlist format Spice */
    PANELSPICEMODEL, /* Handle Netlist format Spice Model (subcircuit) */
    PANELCUSTOMBASE  /* First auxiliary panel (custom netlists).
                      * others use PANELCUSTOMBASE+1, PANELCUSTOMBASE+2.. */
};

// The count of panels for internally defined netlist formats
// (the max count of panel is DEFINED_NETLISTS_COUNT+CUSTOMPANEL_COUNTMAX)
#define DEFINED_NETLISTS_COUNT 6

/* wxPanels for creating the NoteBook pages for each netlist format: */
class EXPORT_NETLIST_PAGE : public wxPanel
{

public:
    /**
     * Create a setup page for one netlist format.
     *
     * Used in Netlist format dialog box creation.
     *
     * @param parent is the wxNotebook parent.
     * @param title is the title of the notebook page.
     * @param id_NetType is the netlist ID type.
     */
    EXPORT_NETLIST_PAGE( wxNotebook* aParent, const wxString& aTitle,
                         NETLIST_TYPE_ID aIdNetType, bool aCustom );
    ~EXPORT_NETLIST_PAGE() { };

    /**
     * @return the name of the netlist format for this page.
     */
    const wxString GetPageNetFmtName() { return m_pageNetFmtName; }

    NETLIST_TYPE_ID   m_IdNetType;
    // opt to reformat passive component values (e.g. 1M -> 1Meg):
    wxCheckBox*       m_CurSheetAsRoot;
    wxCheckBox*       m_SaveAllVoltages;
    wxCheckBox*       m_SaveAllCurrents;
    wxCheckBox*       m_SaveAllDissipations;
    wxCheckBox*       m_RunExternalSpiceCommand;
    wxTextCtrl*       m_CommandStringCtrl;
    wxTextCtrl*       m_TitleStringCtrl;
    wxBoxSizer*       m_LeftBoxSizer;
    wxBoxSizer*       m_RightBoxSizer;
    wxBoxSizer*       m_RightOptionsBoxSizer;
    wxBoxSizer*       m_LowBoxSizer;

    bool IsCustom() const { return m_custom; }

private:
    wxString          m_pageNetFmtName;

    bool              m_custom;
};


/* Dialog frame for creating netlists */
class DIALOG_EXPORT_NETLIST : public DIALOG_EXPORT_NETLIST_BASE
{
public:
    DIALOG_EXPORT_NETLIST( SCH_EDIT_FRAME* parent );
    ~DIALOG_EXPORT_NETLIST() { };

private:
    void InstallCustomPages();
    EXPORT_NETLIST_PAGE* AddOneCustomPage( const wxString& aTitle, const wxString& aCommandString,
                                           NETLIST_TYPE_ID aNetTypeId );
    void InstallPageSpice();
    void InstallPageSpiceModel();

    bool TransferDataFromWindow() override;
    bool NetlistUpdateOpt();

    void updateGeneratorButtons();

    // Called when changing the notebook page (and therefore the current netlist format)
    void OnNetlistTypeSelection( wxNotebookEvent& event ) override;

    /**
     * Add a new panel for a new netlist plugin.
     */
    void OnAddGenerator( wxCommandEvent& event ) override;

    /**
     * Remove a panel relative to a netlist plugin.
     */
    void OnDelGenerator( wxCommandEvent& event ) override;

    /**
     * Write the current netlist options setup in the configuration.
     */
    void WriteCurrentNetlistSetup();

    /**
     * Return the filename extension and the wildcard string for this page or a void name
     * if there is no default name.
     *
     * @param aType is the netlist type ( NET_TYPE_PCBNEW ... ).
     * @param aExt [in] is a holder for the netlist file extension.
     * @param aWildCard [in] is a holder for netlist file dialog wildcard.
     * @return true for known netlist type, false for custom formats.
     */
    bool FilenamePrms( NETLIST_TYPE_ID aType,  wxString* aExt, wxString* aWildCard );

public:
    SCH_EDIT_FRAME*      m_Parent;
    EXPORT_NETLIST_PAGE* m_PanelNetType[DEFINED_NETLISTS_COUNT + CUSTOMPANEL_COUNTMAX];
};


class NETLIST_DIALOG_ADD_GENERATOR : public NETLIST_DIALOG_ADD_GENERATOR_BASE
{
public:
    NETLIST_DIALOG_ADD_GENERATOR( DIALOG_EXPORT_NETLIST* parent );

    const wxString GetGeneratorTitle()  { return m_textCtrlName->GetValue(); }
    const wxString GetGeneratorTCommandLine() { return m_textCtrlCommand->GetValue(); }

    bool TransferDataFromWindow() override;

private:
    /*
     * Browse plugin files, and set m_CommandStringCtrl field
     */
    void OnBrowseGenerators( wxCommandEvent& event ) override;

    DIALOG_EXPORT_NETLIST* m_Parent;
};


/* Event id for notebook page buttons: */
enum id_netlist {
    ID_CREATE_NETLIST = ID_END_EESCHEMA_ID_LIST + 1,
    ID_CUR_SHEET_AS_ROOT,
    ID_SAVE_ALL_VOLTAGES,
    ID_SAVE_ALL_CURRENTS,
    ID_SAVE_ALL_DISSIPATIONS,
    ID_RUN_SIMULATOR
};


EXPORT_NETLIST_PAGE::EXPORT_NETLIST_PAGE( wxNotebook* aParent, const wxString& aTitle,
                                          NETLIST_TYPE_ID aIdNetType, bool aCustom ) :
        wxPanel( aParent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL )
{
    m_IdNetType           = aIdNetType;
    m_pageNetFmtName      = aTitle;
    m_CommandStringCtrl   = nullptr;
    m_CurSheetAsRoot      = nullptr;
    m_TitleStringCtrl     = nullptr;
    m_SaveAllVoltages     = nullptr;
    m_SaveAllCurrents     = nullptr;
    m_SaveAllDissipations = nullptr;
    m_RunExternalSpiceCommand = nullptr;
    m_custom              = aCustom;

    aParent->AddPage( this, aTitle, false );

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* UpperBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    m_LowBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( UpperBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( m_LowBoxSizer, 0, wxGROW | wxALL, 5 );

    m_LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    m_RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    m_RightOptionsBoxSizer = new wxBoxSizer( wxVERTICAL );
    UpperBoxSizer->Add( m_LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    UpperBoxSizer->Add( m_RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
    UpperBoxSizer->Add( m_RightOptionsBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
}


DIALOG_EXPORT_NETLIST::DIALOG_EXPORT_NETLIST( SCH_EDIT_FRAME* parent ) :
        DIALOG_EXPORT_NETLIST_BASE( parent )
{
    m_Parent = parent;

    SCHEMATIC_SETTINGS& settings = m_Parent->Schematic().Settings();

    for( EXPORT_NETLIST_PAGE*& page : m_PanelNetType )
        page = nullptr;

    // Add notebook pages:
    m_PanelNetType[PANELPCBNEW] =
            new EXPORT_NETLIST_PAGE( m_NoteBook, wxT( "KiCad" ), NET_TYPE_PCBNEW, false );

    m_PanelNetType[PANELORCADPCB2] =
            new EXPORT_NETLIST_PAGE( m_NoteBook, wxT( "OrcadPCB2" ), NET_TYPE_ORCADPCB2, false );

    m_PanelNetType[PANELALLEGRO] =
            new EXPORT_NETLIST_PAGE( m_NoteBook, wxT( "Allegro" ), NET_TYPE_ALLEGRO, false );

    m_PanelNetType[PANELCADSTAR] =
            new EXPORT_NETLIST_PAGE( m_NoteBook, wxT( "CadStar" ), NET_TYPE_CADSTAR, false );

    InstallPageSpice();
    InstallPageSpiceModel();
    InstallCustomPages();

    SetupStandardButtons( { { wxID_OK,     _( "Export Netlist" ) },
                            { wxID_CANCEL, _( "Close" )          } } );

    for( int ii = 0; (ii < 4 + CUSTOMPANEL_COUNTMAX) && m_PanelNetType[ii]; ++ii )
    {
        if( m_PanelNetType[ii]->GetPageNetFmtName() == settings.m_NetFormatName )
        {
            m_NoteBook->ChangeSelection( ii );
            break;
        }
    }

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();

    updateGeneratorButtons();
}


void DIALOG_EXPORT_NETLIST::InstallPageSpice()
{
    EXPORT_NETLIST_PAGE* page = m_PanelNetType[PANELSPICE] =
            new EXPORT_NETLIST_PAGE( m_NoteBook, wxT( "Spice" ), NET_TYPE_SPICE, false );

    SCHEMATIC_SETTINGS& settings = m_Parent->Schematic().Settings();

    page->m_CurSheetAsRoot = new wxCheckBox( page, ID_CUR_SHEET_AS_ROOT,
                                             _( "Use current sheet as root" ) );
    page->m_CurSheetAsRoot->SetToolTip( _( "Export netlist only for the current sheet" ) );
    page->m_CurSheetAsRoot->SetValue( settings.m_SpiceCurSheetAsRoot );
    page->m_LeftBoxSizer->Add( page->m_CurSheetAsRoot, 0, wxGROW | wxBOTTOM | wxRIGHT, 5 );

    page->m_SaveAllVoltages = new wxCheckBox( page, ID_SAVE_ALL_VOLTAGES,
                                              _( "Save all voltages" ) );
    page->m_SaveAllVoltages->SetToolTip( _( "Write a directive to save all voltages (.save all)" ) );
    page->m_SaveAllVoltages->SetValue( settings.m_SpiceSaveAllVoltages );
    page->m_LeftBoxSizer->Add( page->m_SaveAllVoltages, 0, wxBOTTOM | wxRIGHT, 5 );

    page->m_SaveAllCurrents = new wxCheckBox( page, ID_SAVE_ALL_CURRENTS,
                                              _( "Save all currents" ) );
    page->m_SaveAllCurrents->SetToolTip( _( "Write a directive to save all currents (.probe alli)" ) );
    page->m_SaveAllCurrents->SetValue( settings.m_SpiceSaveAllCurrents );
    page->m_LeftBoxSizer->Add( page->m_SaveAllCurrents, 0, wxBOTTOM | wxRIGHT, 5 );

    page->m_SaveAllDissipations = new wxCheckBox( page, ID_SAVE_ALL_DISSIPATIONS,
                                                  _( "Save all power dissipations" ) );
    page->m_SaveAllDissipations->SetToolTip( _( "Write directives to save power dissipation of all items (.probe p(<item>))" ) );
    page->m_SaveAllDissipations->SetValue( settings.m_SpiceSaveAllDissipations );
    page->m_LeftBoxSizer->Add( page->m_SaveAllDissipations, 0, wxBOTTOM | wxRIGHT, 5 );


    page->m_RunExternalSpiceCommand = new wxCheckBox( page, ID_RUN_SIMULATOR,
                                                      _( "Run external simulator command:" ) );
    wxString simulatorCommand = settings.m_SpiceCommandString;
    page->m_RunExternalSpiceCommand->SetToolTip( _( "Enter the command line to run SPICE\n"
                                                    "Usually '<path to SPICE binary> \"%I\"'\n"
                                                    "%I will be replaced by the netlist filepath" ) );
    page->m_LowBoxSizer->Add( page->m_RunExternalSpiceCommand, 0,
                              wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    page->m_CommandStringCtrl = new wxTextCtrl( page, -1, simulatorCommand,
                                                wxDefaultPosition, wxDefaultSize );

    page->m_CommandStringCtrl->SetInsertionPoint( 1 );
    page->m_LowBoxSizer->Add( page->m_CommandStringCtrl, 0,
                              wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
}


void DIALOG_EXPORT_NETLIST::InstallPageSpiceModel()
{
    EXPORT_NETLIST_PAGE* page = m_PanelNetType[PANELSPICEMODEL] =
            new EXPORT_NETLIST_PAGE( m_NoteBook, wxT( "Spice Model" ), NET_TYPE_SPICE_MODEL, false );

    SCHEMATIC_SETTINGS& settings = m_Parent->Schematic().Settings();

    page->m_CurSheetAsRoot = new wxCheckBox( page, ID_CUR_SHEET_AS_ROOT,
                                             _( "Use current sheet as root" ) );
    page->m_CurSheetAsRoot->SetToolTip( _( "Export netlist only for the current sheet" ) );
    page->m_CurSheetAsRoot->SetValue( settings.m_SpiceModelCurSheetAsRoot );
    page->m_LeftBoxSizer->Add( page->m_CurSheetAsRoot, 0, wxGROW | wxBOTTOM | wxRIGHT, 5 );
}


void DIALOG_EXPORT_NETLIST::InstallCustomPages()
{
    EXPORT_NETLIST_PAGE* currPage;
    EESCHEMA_SETTINGS*   cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( cfg );

    if( cfg )
    {
        for( size_t i = 0; i < CUSTOMPANEL_COUNTMAX && i < cfg->m_NetlistPanel.plugins.size(); i++ )
        {
            // pairs of (title, command) are stored
            wxString title = cfg->m_NetlistPanel.plugins[i].name;

            if( i >= cfg->m_NetlistPanel.plugins.size() )
                break; // No more panel to install

            wxString command = cfg->m_NetlistPanel.plugins[i].command;

            currPage = AddOneCustomPage( title, command,
                    static_cast<NETLIST_TYPE_ID>( NET_TYPE_CUSTOM1 + i ) );

            m_PanelNetType[PANELCUSTOMBASE + i] = currPage;
        }
    }
}


EXPORT_NETLIST_PAGE* DIALOG_EXPORT_NETLIST::AddOneCustomPage( const wxString& aTitle,
                                                              const wxString& aCommandString,
                                                              NETLIST_TYPE_ID aNetTypeId )
{
    EXPORT_NETLIST_PAGE* currPage = new EXPORT_NETLIST_PAGE( m_NoteBook, aTitle, aNetTypeId, true );

    currPage->m_LowBoxSizer->Add( new wxStaticText( currPage, -1, _( "Title:" ) ), 0,
                                  wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    currPage->m_TitleStringCtrl = new wxTextCtrl( currPage, -1, aTitle,
                                                  wxDefaultPosition, wxDefaultSize );

    currPage->m_TitleStringCtrl->SetInsertionPoint( 1 );
    currPage->m_LowBoxSizer->Add( currPage->m_TitleStringCtrl, 0,
                                  wxGROW | wxTOP | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    currPage->m_LowBoxSizer->Add( new wxStaticText( currPage, -1, _( "Netlist command:" ) ), 0,
                                                    wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    currPage->m_CommandStringCtrl = new wxTextCtrl( currPage, -1, aCommandString,
                                                    wxDefaultPosition, wxDefaultSize );

    currPage->m_CommandStringCtrl->SetInsertionPoint( 1 );
    currPage->m_LowBoxSizer->Add( currPage->m_CommandStringCtrl, 0,
                                  wxGROW | wxTOP | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    return currPage;
}


void DIALOG_EXPORT_NETLIST::OnNetlistTypeSelection( wxNotebookEvent& event )
{
    updateGeneratorButtons();
}


bool DIALOG_EXPORT_NETLIST::NetlistUpdateOpt()
{
    bool changed = false;

    bool saveAllVoltages = m_PanelNetType[ PANELSPICE ]->m_SaveAllVoltages->IsChecked();
    bool saveAllCurrents = m_PanelNetType[ PANELSPICE ]->m_SaveAllCurrents->IsChecked();
    bool saveAllDissipations = m_PanelNetType[ PANELSPICE ]->m_SaveAllDissipations->IsChecked();
    wxString spiceCmdString = m_PanelNetType[ PANELSPICE ]->m_CommandStringCtrl->GetValue();
    bool curSheetAsRoot = m_PanelNetType[ PANELSPICE ]->m_CurSheetAsRoot->GetValue();
    bool spiceModelCurSheetAsRoot = m_PanelNetType[ PANELSPICEMODEL ]->m_CurSheetAsRoot->GetValue();

    SCHEMATIC_SETTINGS& settings = m_Parent->Schematic().Settings();
    wxString netFormatName = m_PanelNetType[m_NoteBook->GetSelection()]->GetPageNetFmtName();

    changed |= ( settings.m_SpiceSaveAllVoltages != saveAllVoltages );
    changed |= ( settings.m_SpiceSaveAllCurrents != saveAllCurrents );
    changed |= ( settings.m_SpiceSaveAllDissipations != saveAllDissipations );
    changed |= ( settings.m_SpiceCommandString != spiceCmdString );
    changed |= ( settings.m_SpiceCurSheetAsRoot != curSheetAsRoot );
    changed |= ( settings.m_SpiceModelCurSheetAsRoot != spiceModelCurSheetAsRoot );
    changed |= ( settings.m_NetFormatName != netFormatName );

    settings.m_SpiceSaveAllVoltages     = saveAllVoltages;
    settings.m_SpiceSaveAllCurrents     = saveAllCurrents;
    settings.m_SpiceSaveAllDissipations = saveAllDissipations;
    settings.m_SpiceCommandString       = spiceCmdString;
    settings.m_SpiceCurSheetAsRoot      = curSheetAsRoot;
    settings.m_SpiceModelCurSheetAsRoot = spiceModelCurSheetAsRoot;
    settings.m_NetFormatName            = netFormatName;

    return changed;
}


bool DIALOG_EXPORT_NETLIST::TransferDataFromWindow()
{
    wxFileName  fn;
    wxString    fileWildcard;
    wxString    fileExt;
    wxString    title = _( "Save Netlist File" );

    if( NetlistUpdateOpt() )
        m_Parent->OnModify();

    EXPORT_NETLIST_PAGE* currPage;
    currPage = (EXPORT_NETLIST_PAGE*) m_NoteBook->GetCurrentPage();

    bool     runExternalSpiceCommand = false;
    unsigned netlist_opt = 0;

    // Calculate the netlist filename
    fn = m_Parent->Schematic().GetFileName();
    FilenamePrms( currPage->m_IdNetType, &fileExt, &fileWildcard );

    // Set some parameters
    switch( currPage->m_IdNetType )
    {
    case NET_TYPE_SPICE:
        // Set spice netlist options:
        netlist_opt |= NETLIST_EXPORTER_SPICE::OPTION_SIM_COMMAND;

        if( currPage->m_SaveAllVoltages->GetValue() )
            netlist_opt |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES;

        if( currPage->m_SaveAllCurrents->GetValue() )
            netlist_opt |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS;

        if( currPage->m_SaveAllDissipations->GetValue() )
            netlist_opt |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS;

        if( currPage->m_CurSheetAsRoot->GetValue() )
            netlist_opt |= NETLIST_EXPORTER_SPICE::OPTION_CUR_SHEET_AS_ROOT;

        runExternalSpiceCommand = currPage->m_RunExternalSpiceCommand->GetValue();
        break;

    case NET_TYPE_SPICE_MODEL:
        if( currPage->m_CurSheetAsRoot->GetValue() )
            netlist_opt |= NETLIST_EXPORTER_SPICE::OPTION_CUR_SHEET_AS_ROOT;

        break;

    case NET_TYPE_CADSTAR:
        break;

    case NET_TYPE_PCBNEW:
        break;

    case NET_TYPE_ORCADPCB2:
        break;

    case NET_TYPE_ALLEGRO:
        break;

    default:    // custom, NET_TYPE_CUSTOM1 and greater
    {
        title.Printf( _( "%s Export" ), currPage->m_TitleStringCtrl->GetValue() );
        break;
    }
    }

    wxString fullpath;

    if( runExternalSpiceCommand )
    {
        fn.SetExt( SpiceFileExtension );
        fullpath = fn.GetFullPath();
    }
    else
    {
        fn.SetExt( fileExt );

        if( fn.GetPath().IsEmpty() )
           fn.SetPath( wxPathOnly( Prj().GetProjectFullName() ) );

        wxString fullname = fn.GetFullName();
        wxString path     = fn.GetPath();

        // full name does not and should not include the path, per wx docs.
        wxFileDialog dlg( this, title, path, fullname, fileWildcard, wxFD_SAVE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        fullpath = dlg.GetPath();   // directory + filename
    }

    m_Parent->ClearMsgPanel();
    REPORTER& reporter = m_MessagesBox->Reporter();

    if( currPage->m_CommandStringCtrl )
        m_Parent->SetNetListerCommand( currPage->m_CommandStringCtrl->GetValue() );
    else
        m_Parent->SetNetListerCommand( wxEmptyString );

    if( !m_Parent->ReadyToNetlist( _( "Exporting netlist requires a fully annotated schematic." ) ) )
        return false;

    m_Parent->WriteNetListFile( currPage->m_IdNetType, fullpath, netlist_opt, &reporter );

    if( runExternalSpiceCommand )
    {
        // Build the command line
        wxString commandLine = m_Parent->Schematic().Settings().m_SpiceCommandString;
        commandLine.Replace( wxS( "%I" ), fullpath, true );
        commandLine.Trim( true ).Trim( false );

        if( !commandLine.IsEmpty() )
        {
            wxArrayString output;
            wxArrayString errors;
            wxExecute( commandLine, output, errors, wxEXEC_ASYNC );

            reporter.ReportHead( commandLine, RPT_SEVERITY_ACTION );

            if( output.GetCount() )
            {
                for( unsigned ii = 0; ii < output.GetCount(); ii++ )
                    reporter.Report( output[ii], RPT_SEVERITY_INFO );
            }

            if( errors.GetCount() )
            {
                for( unsigned ii = 0; ii < errors.GetCount(); ii++ )
                {
                    // wxExecute returns -1 for all error conditions, so we've no choice but
                    // to scrape the stderr messages for the error code(s).

                    if( errors[ii].EndsWith( wxS( "failed with error 2!" ) ) )      // ENOENT
                    {
                        reporter.Report( _( "external simulator not found" ), RPT_SEVERITY_ERROR );
                        reporter.Report( _( "Note: command line is usually: "
                                            "<tt>&lt;path to SPICE binary&gt; \"%I\"</tt>" ),
                                         RPT_SEVERITY_INFO );
                    }
                    else if( errors[ii].EndsWith( wxS( "failed with error 8!" ) ) ) // ENOEXEC
                    {
                        reporter.Report( _( "external simulator has the wrong format or "
                                            "architecture" ), RPT_SEVERITY_ERROR );
                    }
                    else if( errors[ii].EndsWith( "failed with error 13!" ) ) // EACCES
                    {
                        reporter.Report( _( "permission denied" ), RPT_SEVERITY_ERROR );
                    }
                    else
                    {
                        reporter.Report( errors[ii], RPT_SEVERITY_ERROR );
                    }
                }
            }
        }
    }

    WriteCurrentNetlistSetup();

    return !runExternalSpiceCommand;
}


bool DIALOG_EXPORT_NETLIST::FilenamePrms( NETLIST_TYPE_ID aType, wxString * aExt, wxString * aWildCard )
{
    wxString fileExt;
    wxString fileWildcard;
    bool     ret = true;

    switch( aType )
    {
    case NET_TYPE_SPICE:
        fileExt = SpiceFileExtension;
        fileWildcard = SpiceNetlistFileWildcard();
        break;

    case NET_TYPE_CADSTAR:
        fileExt = CadstarNetlistFileExtension;
        fileWildcard = CadstarNetlistFileWildcard();
        break;

    case NET_TYPE_ORCADPCB2:
        fileExt = OrCadPcb2NetlistFileExtension;
        fileWildcard = OrCadPcb2NetlistFileWildcard();
        break;

    case NET_TYPE_PCBNEW:
        fileExt = NetlistFileExtension;
        fileWildcard = NetlistFileWildcard();
        break;

    case NET_TYPE_ALLEGRO:
        fileExt = AllegroNetlistFileExtension;
        fileWildcard = AllegroNetlistFileWildcard();
        break;

    default:    // custom, NET_TYPE_CUSTOM1 and greater
        fileWildcard = AllFilesWildcard();
        ret = false;
    }

    if( aExt )
        *aExt = fileExt;

    if( aWildCard )
        *aWildCard = fileWildcard;

    return ret;
}


void DIALOG_EXPORT_NETLIST::WriteCurrentNetlistSetup()
{
    if( NetlistUpdateOpt() )
        m_Parent->OnModify();

    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxASSERT( cfg );

    if( !cfg )
        return;

    cfg->m_NetlistPanel.plugins.clear();

    // Update existing custom pages
    for( int ii = 0; ii < CUSTOMPANEL_COUNTMAX; ii++ )
    {
        EXPORT_NETLIST_PAGE* currPage = m_PanelNetType[ii + PANELCUSTOMBASE];

        if( currPage == nullptr )
            break;

        wxString title = currPage->m_TitleStringCtrl->GetValue();
        wxString command = currPage->m_CommandStringCtrl->GetValue();

        if( title.IsEmpty() || command.IsEmpty() )
            continue;

        cfg->m_NetlistPanel.plugins.emplace_back( title, wxEmptyString );
        cfg->m_NetlistPanel.plugins.back().command = command;
    }
}


void DIALOG_EXPORT_NETLIST::OnDelGenerator( wxCommandEvent& event )
{
    EXPORT_NETLIST_PAGE* currPage = (EXPORT_NETLIST_PAGE*) m_NoteBook->GetCurrentPage();

    if( !currPage->IsCustom() )
        return;

    currPage->m_CommandStringCtrl->SetValue( wxEmptyString );
    currPage->m_TitleStringCtrl->SetValue( wxEmptyString );

    WriteCurrentNetlistSetup();

    if( IsQuasiModal() )
        EndQuasiModal( NET_PLUGIN_CHANGE );
    else
        EndDialog( NET_PLUGIN_CHANGE );
}


void DIALOG_EXPORT_NETLIST::OnAddGenerator( wxCommandEvent& event )
{
    NETLIST_DIALOG_ADD_GENERATOR dlg( this );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // Creates a new custom plugin page
    wxString title = dlg.GetGeneratorTitle();

    // Verify it does not exists
    int netTypeId = PANELCUSTOMBASE;    // the first not used type id
    EXPORT_NETLIST_PAGE* currPage;

    for( int ii = 0; ii < CUSTOMPANEL_COUNTMAX; ii++ )
    {
        netTypeId = PANELCUSTOMBASE + ii;
        currPage = m_PanelNetType[ii + PANELCUSTOMBASE];

        if( currPage == nullptr )
            break;

        if( currPage->GetPageNetFmtName() == title )
        {
            wxMessageBox( _("This plugin already exists.") );
            return;
        }
    }

    wxString cmd = dlg.GetGeneratorTCommandLine();
    currPage = AddOneCustomPage( title,cmd, (NETLIST_TYPE_ID)netTypeId );
    m_PanelNetType[netTypeId] = currPage;
    WriteCurrentNetlistSetup();

    if( IsQuasiModal() )
        EndQuasiModal( NET_PLUGIN_CHANGE );
    else
        EndDialog( NET_PLUGIN_CHANGE );
}


NETLIST_DIALOG_ADD_GENERATOR::NETLIST_DIALOG_ADD_GENERATOR( DIALOG_EXPORT_NETLIST* parent ) :
    NETLIST_DIALOG_ADD_GENERATOR_BASE( parent )
{
    m_Parent = parent;
    SetupStandardButtons();
    GetSizer()->SetSizeHints( this );
}


bool NETLIST_DIALOG_ADD_GENERATOR::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( m_textCtrlCommand->GetValue() == wxEmptyString )
    {
        wxMessageBox( _( "You must provide a netlist generator command string" ) );
        return false;
    }

    if( m_textCtrlName->GetValue() == wxEmptyString )
    {
        wxMessageBox( _( "You must provide a netlist generator title" ) );
        return false;
    }

    return true;
}


void NETLIST_DIALOG_ADD_GENERATOR::OnBrowseGenerators( wxCommandEvent& event )
{
    wxString FullFileName, Path;

#ifndef __WXMAC__
    Path = Pgm().GetExecutablePath();
#else
    Path = PATHS::GetOSXKicadDataDir() + wxT( "/plugins" );
#endif

    FullFileName = wxFileSelector( _( "Generator File" ), Path, FullFileName,
                                   wxEmptyString, wxFileSelectorDefaultWildcardStr,
                                   wxFD_OPEN, this );

    if( FullFileName.IsEmpty() )
        return;

    // Creates a default command line, suitable for external tool xslproc or python, based on
    // the plugin extension ("xsl" or "exe" or "py")
    wxString cmdLine;
    wxFileName fn( FullFileName );
    wxString ext = fn.GetExt();

    if( ext == wxT( "xsl" ) )
        cmdLine.Printf( wxT( "xsltproc -o \"%%O\" \"%s\" \"%%I\"" ), FullFileName );
    else if( ext == wxT( "exe" ) || ext.IsEmpty() )
        cmdLine.Printf( wxT( "\"%s\" > \"%%O\" < \"%%I\"" ), FullFileName );
    else if( ext == wxT( "py" ) || ext.IsEmpty() )
        cmdLine.Printf( wxT( "python \"%s\" \"%%I\" \"%%O\"" ), FullFileName );
    else
        cmdLine.Printf( wxT( "\"%s\"" ), FullFileName );

    m_textCtrlCommand->SetValue( cmdLine );

    // We need a title for this panel
    // Propose a default value if empty ( i.e. the short filename of the script)
    if( m_textCtrlName->GetValue().IsEmpty() )
        m_textCtrlName->SetValue( fn.GetName() );
}


void DIALOG_EXPORT_NETLIST::updateGeneratorButtons()
{
    EXPORT_NETLIST_PAGE* currPage = (EXPORT_NETLIST_PAGE*) m_NoteBook->GetCurrentPage();

    if( currPage == nullptr )
        return;

    m_buttonDelGenerator->Enable( currPage->IsCustom() );
}


int InvokeDialogNetList( SCH_EDIT_FRAME* aCaller )
{
    DIALOG_EXPORT_NETLIST dlg( aCaller );

    int ret = dlg.ShowModal();
    aCaller->SaveProjectLocalSettings();

    return ret;
}
