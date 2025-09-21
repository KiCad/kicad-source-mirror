/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * with 7 fixed netlist formats:
 *   Pcbnew
 *   ORCADPCB2
 *   Allegro
 *   CADSTAR
 *   Pads
 *   SPICE
 *   SPICE model
 * and up to CUSTOMPANEL_COUNTMAX user programmable formats.  These external converters are
 * referred to as plugins, but they are really just external binaries.
 */

#include <pgm_base.h>
#include <kiface_base.h>
#include <string_utils.h>
#include <gestfich.h>
#include <widgets/wx_html_report_panel.h>
#include <sch_edit_frame.h>
#include <dialogs/dialog_export_netlist.h>
#include <wildcards_and_files_ext.h>
#include <invoke_sch_dialog.h>
#include <netlist_exporters/netlist_exporter_spice.h>
#include <paths.h>
#include <jobs/job_export_sch_netlist.h>

#include <eeschema_id.h>
#include <wx/checkbox.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/process.h>
#include <wx/regex.h>
#include <wx/txtstrm.h>
#include <wx/utils.h>

#include <thread>
#include <vector>


namespace
{
std::vector<wxString> SplitCommandLine( const wxString& aCommand )
{
    std::vector<wxString> args;
    wxString              current;
    bool                  inSingle = false;
    bool                  inDouble = false;
    bool                  argStarted = false;

    for( wxUniChar c : aCommand )
    {
        if( c == '"' && !inSingle )
        {
            inDouble = !inDouble;
            argStarted = true;
            continue;
        }

        if( c == '\'' && !inDouble )
        {
            inSingle = !inSingle;
            argStarted = true;
            continue;
        }

        if( ( c == ' ' || c == '\t' || c == '\n' || c == '\r' ) && !inSingle && !inDouble )
        {
            if( argStarted || !current.IsEmpty() )
            {
                args.emplace_back( current );
                current.clear();
                argStarted = false;
            }

            continue;
        }

        current.Append( c );
        argStarted = true;
    }

    if( argStarted || !current.IsEmpty() )
        args.emplace_back( current );

    return args;
}
} // namespace


#define CUSTOMPANEL_COUNTMAX 8  // Max number of netlist plugins

/*
 * PANEL_NETLIST_INDEX values are used as index in m_PanelNetType[]
 */
enum PANEL_NETLIST_INDEX
{
    PANELPCBNEW = 0,         /* Handle Netlist format Pcbnew */
    PANELORCADPCB2,          /* Handle Netlist format OracdPcb2 */
    PANELALLEGRO,            /* Handle Netlist format Allegro */
    PANELCADSTAR,            /* Handle Netlist format CadStar */
    PANELPADS,               /* Handle Netlist format PADS */
    PANELSPICE,              /* Handle Netlist format Spice */
    PANELSPICEMODEL,         /* Handle Netlist format Spice Model (subcircuit) */
    DEFINED_NETLISTS_COUNT,

    /* First auxiliary panel (custom netlists).  Subsequent ones use PANELCUSTOMBASE+1,
     * PANELCUSTOMBASE+2, etc., up to PANELCUSTOMBASE+CUSTOMPANEL_COUNTMAX-1 */
    PANELCUSTOMBASE = DEFINED_NETLISTS_COUNT
};


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
    EXPORT_NETLIST_PAGE( wxNotebook* aParent, const wxString& aTitle, NETLIST_TYPE_ID aIdNetType, bool aCustom );
    ~EXPORT_NETLIST_PAGE() = default;

    /**
     * @return the name of the netlist format for this page.
     */
    const wxString GetPageNetFmtName() { return m_pageNetFmtName; }

    bool IsCustom() const { return m_custom; }

public:
    NETLIST_TYPE_ID   m_IdNetType;

    // opt to reformat passive component values (e.g. 1M -> 1Meg):
    wxCheckBox*       m_CurSheetAsRoot;
    wxCheckBox*       m_SaveAllVoltages;
    wxCheckBox*       m_SaveAllCurrents;
    wxCheckBox*       m_SaveAllDissipations;
    wxCheckBox*       m_SaveAllEvents;
    wxCheckBox*       m_RunExternalSpiceCommand;
    wxTextCtrl*       m_CommandStringCtrl;
    wxTextCtrl*       m_TitleStringCtrl;
    wxBoxSizer*       m_LeftBoxSizer;
    wxBoxSizer*       m_RightBoxSizer;
    wxBoxSizer*       m_RightOptionsBoxSizer;
    wxBoxSizer*       m_LowBoxSizer;

private:
    wxString          m_pageNetFmtName;
    bool              m_custom;
};


class NETLIST_DIALOG_ADD_GENERATOR : public NETLIST_DIALOG_ADD_GENERATOR_BASE
{
public:
    NETLIST_DIALOG_ADD_GENERATOR( DIALOG_EXPORT_NETLIST* parent );

    const wxString GetGeneratorTitle()  { return m_textCtrlName->GetValue(); }
    const wxString GetGeneratorTCommandLine() { return m_textCtrlCommand->GetValue(); }

    bool TransferDataFromWindow() override;

private:
    /**
     * Browse plugin files, and set m_CommandStringCtrl field.
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
    ID_SAVE_ALL_EVENTS,
    ID_RUN_SIMULATOR
};


EXPORT_NETLIST_PAGE::EXPORT_NETLIST_PAGE( wxNotebook* aParent, const wxString& aTitle,
                                          NETLIST_TYPE_ID aIdNetType, bool aCustom ) :
        wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL ),
        m_IdNetType( aIdNetType ),
        m_CurSheetAsRoot( nullptr ),
        m_SaveAllVoltages( nullptr ),
        m_SaveAllCurrents( nullptr ),
        m_SaveAllDissipations( nullptr ),
        m_SaveAllEvents( nullptr ),
        m_RunExternalSpiceCommand( nullptr ),
        m_CommandStringCtrl( nullptr ),
        m_TitleStringCtrl( nullptr ),
        m_pageNetFmtName( aTitle ),
        m_custom( aCustom )
{
    aParent->AddPage( this, aTitle, false );

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* UpperBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    m_LowBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( UpperBoxSizer, 0, wxEXPAND | wxALL, 5 );
    MainBoxSizer->Add( m_LowBoxSizer, 0, wxEXPAND | wxALL, 5 );

    m_LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    m_RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    m_RightOptionsBoxSizer = new wxBoxSizer( wxVERTICAL );
    UpperBoxSizer->Add( m_LeftBoxSizer, 0, wxEXPAND | wxALL, 5 );
    UpperBoxSizer->Add( m_RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
    UpperBoxSizer->Add( m_RightOptionsBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
}


DIALOG_EXPORT_NETLIST::DIALOG_EXPORT_NETLIST( SCH_EDIT_FRAME* aEditFrame ) :
        DIALOG_EXPORT_NETLIST( aEditFrame, aEditFrame )
{
}


DIALOG_EXPORT_NETLIST::DIALOG_EXPORT_NETLIST( SCH_EDIT_FRAME* aEditFrame, wxWindow* aParent,
                                              JOB_EXPORT_SCH_NETLIST* aJob ) :
        DIALOG_EXPORT_NETLIST_BASE( aParent ),
        m_job( aJob )
{
    m_editFrame = aEditFrame;

    // Initialize the array of netlist pages
    m_PanelNetType.resize( DEFINED_NETLISTS_COUNT + CUSTOMPANEL_COUNTMAX, nullptr );

    // Add notebook pages:
    EXPORT_NETLIST_PAGE* page = nullptr;
    wxStaticText*        label = nullptr;

    page = new EXPORT_NETLIST_PAGE( m_NoteBook, wxT( "KiCad" ), NET_TYPE_PCBNEW, false );
    label = new wxStaticText( page, wxID_ANY, _( "Export netlist in KiCad format" ) );
    page->m_LeftBoxSizer->Add( label, 0, wxBOTTOM, 10 );
    m_PanelNetType[PANELPCBNEW] = page;

    page = new EXPORT_NETLIST_PAGE( m_NoteBook, wxT( "OrcadPCB2" ), NET_TYPE_ORCADPCB2, false );
    label = new wxStaticText( page, wxID_ANY, _( "Export netlist in OrcadPCB2 format" ) );
    page->m_LeftBoxSizer->Add( label, 0, wxBOTTOM, 10 );
    m_PanelNetType[PANELORCADPCB2] = page;

    page = new EXPORT_NETLIST_PAGE( m_NoteBook, wxT( "Allegro" ), NET_TYPE_ALLEGRO, false );
    label = new wxStaticText( page, wxID_ANY, _( "Export netlist in Allegro format" ) );
    page->m_LeftBoxSizer->Add( label, 0, wxBOTTOM, 10 );
    m_PanelNetType[PANELALLEGRO] = page;

    page = new EXPORT_NETLIST_PAGE( m_NoteBook, wxT( "CadStar" ), NET_TYPE_CADSTAR, false );
    label = new wxStaticText( page, wxID_ANY, _( "Export netlist in CadStar format" ) );
    page->m_LeftBoxSizer->Add( label, 0, wxBOTTOM, 10 );
    m_PanelNetType[PANELCADSTAR] = page;

    page = new EXPORT_NETLIST_PAGE( m_NoteBook, wxT( "PADS" ), NET_TYPE_PADS, false );
    label = new wxStaticText( page, wxID_ANY, _( "Export netlist in PADS format" ) );
    page->m_LeftBoxSizer->Add( label, 0, wxBOTTOM, 10 );
    m_PanelNetType[PANELPADS] = page;

    InstallPageSpice();
    InstallPageSpiceModel();

    if( !m_job )
    {
        m_outputPath->Hide();
        m_staticTextOutputPath->Hide();
        InstallCustomPages();

        SetupStandardButtons( { { wxID_OK,     _( "Export Netlist" ) },
                                { wxID_CANCEL, _( "Close" )          } } );
    }
    else
    {
        SetTitle( m_job->GetSettingsDialogTitle() );

        m_MessagesBox->Hide();
        m_outputPath->SetValue( m_job->GetConfiguredOutputPath() );

        SetupStandardButtons();

        // custom netlist (external invokes, not supported)
        for( int ii = 0; ii < DEFINED_NETLISTS_COUNT + CUSTOMPANEL_COUNTMAX; ++ii )
        {
            if( EXPORT_NETLIST_PAGE* candidate = m_PanelNetType[ii] )
            {
                if( candidate->GetPageNetFmtName() == JOB_EXPORT_SCH_NETLIST::GetFormatNameMap()[m_job->format] )
                {
                    m_NoteBook->ChangeSelection( ii );
                    break;
                }
            }
        }

        m_buttonAddGenerator->Hide();
        m_buttonDelGenerator->Hide();
    }

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions.
    m_hash_key = TO_UTF8( GetTitle() );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();

    updateGeneratorButtons();
}


void DIALOG_EXPORT_NETLIST::InstallPageSpice()
{
    EXPORT_NETLIST_PAGE* pg = new EXPORT_NETLIST_PAGE( m_NoteBook, wxT( "SPICE" ), NET_TYPE_SPICE, false );

    wxStaticText* label = new wxStaticText( pg, wxID_ANY, _( "Export netlist in SPICE format" ) );
    pg->m_LeftBoxSizer->Add( label, 0, wxBOTTOM, 10 );

    pg->m_CurSheetAsRoot = new wxCheckBox( pg, ID_CUR_SHEET_AS_ROOT, _( "Use current sheet as root" ) );
    pg->m_CurSheetAsRoot->SetToolTip( _( "Export netlist only for the current sheet" ) );
    pg->m_LeftBoxSizer->Add( pg->m_CurSheetAsRoot, 0, wxGROW | wxBOTTOM | wxRIGHT, 5 );

    pg->m_SaveAllVoltages = new wxCheckBox( pg, ID_SAVE_ALL_VOLTAGES, _( "Save all voltages" ) );
    pg->m_SaveAllVoltages->SetToolTip( _( "Write a directive to save all voltages (.save all)" ) );
    pg->m_LeftBoxSizer->Add( pg->m_SaveAllVoltages, 0, wxBOTTOM | wxRIGHT, 5 );

    pg->m_SaveAllCurrents = new wxCheckBox( pg, ID_SAVE_ALL_CURRENTS, _( "Save all currents" ) );
    pg->m_SaveAllCurrents->SetToolTip( _( "Write a directive to save all currents (.probe alli)" ) );
    pg->m_LeftBoxSizer->Add( pg->m_SaveAllCurrents, 0, wxBOTTOM | wxRIGHT, 5 );

    pg->m_SaveAllDissipations = new wxCheckBox( pg, ID_SAVE_ALL_DISSIPATIONS, _( "Save all power dissipations" ) );
    pg->m_SaveAllDissipations->SetToolTip( _( "Write directives to save power dissipation of all items "
                                              "(.probe p(<item>))" ) );
    pg->m_LeftBoxSizer->Add( pg->m_SaveAllDissipations, 0, wxBOTTOM | wxRIGHT, 5 );

    pg->m_SaveAllEvents = new wxCheckBox( pg, ID_SAVE_ALL_EVENTS, _( "Save all digital event data" ) );
    pg->m_SaveAllEvents->SetToolTip( _( "If not set, write a directive to prevent the saving of digital event data "
                                        "(esave none)" ) );
    pg->m_LeftBoxSizer->Add( pg->m_SaveAllEvents, 0, wxBOTTOM | wxRIGHT, 5 );


    pg->m_RunExternalSpiceCommand = new wxCheckBox( pg, ID_RUN_SIMULATOR, _( "Run external simulator command:" ) );
    pg->m_RunExternalSpiceCommand->SetToolTip( _( "Enter the command line to run SPICE\n"
                                                  "Usually '<path to SPICE binary> \"%I\"'\n"
                                                  "%I will be replaced by the netlist filepath" ) );
    pg->m_LowBoxSizer->Add( pg->m_RunExternalSpiceCommand, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    pg->m_CommandStringCtrl = new wxTextCtrl( pg, wxID_ANY, wxT( "spice \"%I\"" ) );

    pg->m_CommandStringCtrl->SetInsertionPoint( 1 );
    pg->m_LowBoxSizer->Add( pg->m_CommandStringCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    m_PanelNetType[PANELSPICE] = pg;
}


void DIALOG_EXPORT_NETLIST::InstallPageSpiceModel()
{
    auto* pg = new EXPORT_NETLIST_PAGE( m_NoteBook, wxT( "SPICE Model" ), NET_TYPE_SPICE_MODEL, false );

    wxStaticText* label = new wxStaticText( pg, wxID_ANY, _( "Export netlist as a SPICE .subckt model" ) );
    pg->m_LeftBoxSizer->Add( label, 0, wxBOTTOM, 10 );

    pg->m_CurSheetAsRoot = new wxCheckBox( pg, ID_CUR_SHEET_AS_ROOT, _( "Use current sheet as root" ) );
    pg->m_CurSheetAsRoot->SetToolTip( _( "Export netlist only for the current sheet" ) );
    pg->m_LeftBoxSizer->Add( pg->m_CurSheetAsRoot, 0, wxEXPAND | wxBOTTOM | wxRIGHT, 5 );

    m_PanelNetType[PANELSPICEMODEL] = pg;
}


void DIALOG_EXPORT_NETLIST::InstallCustomPages()
{
    EXPORT_NETLIST_PAGE* currPage;
    EESCHEMA_SETTINGS*   cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxCHECK( cfg, /* void */ );

    for( size_t i = 0; i < CUSTOMPANEL_COUNTMAX && i < cfg->m_NetlistPanel.plugins.size(); i++ )
    {
        // pairs of (title, command) are stored
        currPage = AddOneCustomPage( cfg->m_NetlistPanel.plugins[i].name,
                                     cfg->m_NetlistPanel.plugins[i].command,
                                     static_cast<NETLIST_TYPE_ID>( NET_TYPE_CUSTOM1 + i ) );

        m_PanelNetType[PANELCUSTOMBASE + i] = currPage;
    }
}


EXPORT_NETLIST_PAGE* DIALOG_EXPORT_NETLIST::AddOneCustomPage( const wxString& aTitle,
                                                              const wxString& aCommandString,
                                                              NETLIST_TYPE_ID aNetTypeId )
{
    EXPORT_NETLIST_PAGE* pg = new EXPORT_NETLIST_PAGE( m_NoteBook, aTitle, aNetTypeId, true );

    pg->m_LowBoxSizer->Add( new wxStaticText( pg, wxID_ANY, _( "Title:" ) ), 0,
                            wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5 );

    pg->m_LowBoxSizer->AddSpacer( 2 );

    pg->m_TitleStringCtrl = new wxTextCtrl( pg, wxID_ANY, aTitle );

    pg->m_TitleStringCtrl->SetInsertionPoint( 1 );
    pg->m_LowBoxSizer->Add( pg->m_TitleStringCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT, 5 );

    pg->m_LowBoxSizer->AddSpacer( 10 );

    pg->m_LowBoxSizer->Add( new wxStaticText( pg, wxID_ANY, _( "Netlist command:" ) ), 0,
                            wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5 );

    pg->m_LowBoxSizer->AddSpacer( 2 );

    pg->m_CommandStringCtrl = new wxTextCtrl( pg, wxID_ANY, aCommandString );

    pg->m_CommandStringCtrl->SetInsertionPoint( 1 );
    pg->m_LowBoxSizer->Add( pg->m_CommandStringCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT, 5 );

    return pg;
}


void DIALOG_EXPORT_NETLIST::OnNetlistTypeSelection( wxNotebookEvent& event )
{
    updateGeneratorButtons();
}


bool DIALOG_EXPORT_NETLIST::TransferDataFromWindow()
{
    wxFileName  fn;
    wxString    fileWildcard;
    wxString    fileExt;
    wxString    title = _( "Save Netlist File" );

    if( m_job )
    {
        for( const auto& [format, name] : JOB_EXPORT_SCH_NETLIST::GetFormatNameMap() )
        {
            if( name == m_PanelNetType[m_NoteBook->GetSelection()]->GetPageNetFmtName() )
            {
                m_job->format = format;
                break;
            }
        }

        m_job->SetConfiguredOutputPath( m_outputPath->GetValue() );
        m_job->m_spiceSaveAllVoltages = m_PanelNetType[ PANELSPICE ]->m_SaveAllVoltages->IsChecked();
        m_job->m_spiceSaveAllCurrents = m_PanelNetType[ PANELSPICE ]->m_SaveAllCurrents->IsChecked();
        m_job->m_spiceSaveAllDissipations = m_PanelNetType[ PANELSPICE ]->m_SaveAllDissipations->IsChecked();
        m_job->m_spiceSaveAllEvents = m_PanelNetType[ PANELSPICE ]->m_SaveAllEvents->IsChecked();

        return true;
    }

    EXPORT_NETLIST_PAGE* currPage;
    currPage = (EXPORT_NETLIST_PAGE*) m_NoteBook->GetCurrentPage();

    bool     runExternalSpiceCommand = false;
    unsigned netlist_opt = 0;

    // Calculate the netlist filename
    fn = m_editFrame->Schematic().GetFileName();
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

        if( currPage->m_SaveAllEvents->GetValue() )
            netlist_opt |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_EVENTS;

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

    case NET_TYPE_PADS:
        break;

    default:    // custom, NET_TYPE_CUSTOM1 and greater
        title.Printf( _( "%s Export" ), currPage->m_TitleStringCtrl->GetValue() );
        break;
    }

    wxString fullpath;

    if( runExternalSpiceCommand )
    {
        fn.SetExt( FILEEXT::SpiceFileExtension );
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

    m_editFrame->ClearMsgPanel();
    REPORTER& reporter = m_MessagesBox->Reporter();

    if( currPage->m_CommandStringCtrl )
        m_editFrame->SetNetListerCommand( currPage->m_CommandStringCtrl->GetValue() );
    else
        m_editFrame->SetNetListerCommand( wxEmptyString );

    if( !m_editFrame->ReadyToNetlist( _( "Exporting netlist requires a fully annotated schematic." ) ) )
        return false;

    m_editFrame->WriteNetListFile( currPage->m_IdNetType, fullpath, netlist_opt, &reporter );

    if( runExternalSpiceCommand )
    {
        // Build the command line
        wxString commandLine = m_PanelNetType[ PANELSPICE ]->m_CommandStringCtrl->GetValue();
        commandLine.Replace( wxS( "%I" ), fullpath, true );
        commandLine.Trim( true ).Trim( false );

        if( !commandLine.IsEmpty() )
        {
            std::vector<wxString> argsStrings = SplitCommandLine( commandLine );

            if( !argsStrings.empty() )
            {
                std::vector<const wxChar*> argv;
                argv.reserve( argsStrings.size() + 1 );

                for( wxString& arg : argsStrings )
                    argv.emplace_back( arg.wc_str() );

                argv.emplace_back( nullptr );

                wxExecuteEnv env;
                wxGetEnvMap( &env.env );

                const ENV_VAR_MAP& envVars = Pgm().GetLocalEnvVariables();

                for( const auto& [key, value] : envVars )
                {
                    if( !value.GetValue().IsEmpty() )
                        env.env[key] = value.GetValue();
                }

                wxFileName netlistFile( fullpath );

                if( !netlistFile.IsAbsolute() )
                    netlistFile.MakeAbsolute( Prj().GetProjectPath() );
                else
                    netlistFile.MakeAbsolute();

                wxString cwd = netlistFile.GetPath();

                if( cwd.IsEmpty() )
                    cwd = Prj().GetProjectPath();

                env.cwd = cwd;

                wxProcess* process = new wxProcess( GetEventHandler(), wxID_ANY );
                process->Redirect();

                long pid = wxExecute( argv.data(), wxEXEC_ASYNC, process, &env );

                reporter.ReportHead( commandLine, RPT_SEVERITY_ACTION );

                if( pid <= 0 )
                {
                    reporter.Report( _( "external simulator not found" ), RPT_SEVERITY_ERROR );
                    reporter.Report( _( "Note: command line is usually: "
                                        "<tt>&lt;path to SPICE binary&gt; \"%I\"</tt>" ),
                                     RPT_SEVERITY_INFO );
                    delete process;
                }
                else
                {
                    process->Activate();

                    std::this_thread::sleep_for( std::chrono::seconds( 1 ) ); // give the process time to start and output any data or errors

                    if( process->IsInputAvailable() )
                    {
                        wxInputStream* in = process->GetInputStream();
                        wxTextInputStream textstream( *in );

                        while( in->CanRead() )
                        {
                            wxString line = textstream.ReadLine();

                            if( !line.IsEmpty() )
                                reporter.Report( line, RPT_SEVERITY_INFO );
                        }
                    }

                    if( process->IsErrorAvailable() )
                    {
                        wxInputStream* err = process->GetErrorStream();
                        wxTextInputStream textstream( *err );

                        while( err->CanRead() )
                        {
                            wxString line = textstream.ReadLine();

                            if( !line.IsEmpty() )
                            {
                                if( line.EndsWith( wxS( "failed with error 2!" ) ) )      // ENOENT
                                {
                                    reporter.Report( _( "external simulator not found" ), RPT_SEVERITY_ERROR );
                                    reporter.Report( _( "Note: command line is usually: "
                                                        "<tt>&lt;path to SPICE binary&gt; \"%I\"</tt>" ),
                                                     RPT_SEVERITY_INFO );
                                }
                                else if( line.EndsWith( wxS( "failed with error 8!" ) ) ) // ENOEXEC
                                {
                                    reporter.Report( _( "external simulator has the wrong format or "
                                                        "architecture" ), RPT_SEVERITY_ERROR );
                                }
                                else if( line.EndsWith( "failed with error 13!" ) ) // EACCES
                                {
                                    reporter.Report( _( "permission denied" ), RPT_SEVERITY_ERROR );
                                }
                                else
                                {
                                    reporter.Report( line, RPT_SEVERITY_ERROR );
                                }
                            }
                        }
                    }

                    process->CloseOutput();
                    process->Detach();

                    // Do not delete process, it will delete itself when it terminates
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
        fileExt = FILEEXT::SpiceFileExtension;
        fileWildcard = FILEEXT::SpiceNetlistFileWildcard();
        break;

    case NET_TYPE_CADSTAR:
        fileExt = FILEEXT::CadstarNetlistFileExtension;
        fileWildcard = FILEEXT::CadstarNetlistFileWildcard();
        break;

    case NET_TYPE_ORCADPCB2:
        fileExt = FILEEXT::OrCadPcb2NetlistFileExtension;
        fileWildcard = FILEEXT::OrCadPcb2NetlistFileWildcard();
        break;

    case NET_TYPE_PCBNEW:
        fileExt = FILEEXT::NetlistFileExtension;
        fileWildcard = FILEEXT::NetlistFileWildcard();
        break;

    case NET_TYPE_ALLEGRO:
        fileExt = FILEEXT::AllegroNetlistFileExtension;
        fileWildcard = FILEEXT::AllegroNetlistFileWildcard();
        break;

    case NET_TYPE_PADS:
        fileExt = FILEEXT::PADSNetlistFileExtension;
        fileWildcard = FILEEXT::PADSNetlistFileWildcard();
        break;


    default:    // custom, NET_TYPE_CUSTOM1 and greater
        fileWildcard = FILEEXT::AllFilesWildcard();
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
    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
    wxCHECK( cfg, /* void */ );

    cfg->m_NetlistPanel.plugins.clear();

    // Update existing custom pages
    for( int ii = PANELCUSTOMBASE; ii < PANELCUSTOMBASE + CUSTOMPANEL_COUNTMAX; ++ii )
    {
        if( EXPORT_NETLIST_PAGE* currPage = m_PanelNetType[ii] )
        {
            wxString title = currPage->m_TitleStringCtrl->GetValue();
            wxString command = currPage->m_CommandStringCtrl->GetValue();

            if( title.IsEmpty() || command.IsEmpty() )
                continue;

            cfg->m_NetlistPanel.plugins.emplace_back( title, wxEmptyString );
            cfg->m_NetlistPanel.plugins.back().command = command;
        }
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

    wxString title = dlg.GetGeneratorTitle();
    wxString cmd = dlg.GetGeneratorTCommandLine();

    // Verify it does not exists
    for( int ii = PANELCUSTOMBASE; ii < PANELCUSTOMBASE + CUSTOMPANEL_COUNTMAX; ++ii )
    {
        if( m_PanelNetType[ii] && m_PanelNetType[ii]->GetPageNetFmtName() == title )
        {
            wxMessageBox( _( "This plugin already exists." ) );
            return;
        }
    }

    // Find the first empty slot
    int netTypeId = PANELCUSTOMBASE;

    while( m_PanelNetType[netTypeId] )
    {
        netTypeId++;

        if( netTypeId == PANELCUSTOMBASE + CUSTOMPANEL_COUNTMAX )
        {
            wxMessageBox( _( "Maximum number of plugins already added to dialog." ) );
            return;
        }
    }

    m_PanelNetType[netTypeId] = AddOneCustomPage( title, cmd, (NETLIST_TYPE_ID)netTypeId );

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
    m_initialFocusTarget = m_textCtrlName;

    SetupStandardButtons();
    finishDialogSettings();
}


bool NETLIST_DIALOG_ADD_GENERATOR::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

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

    FullFileName = wxFileSelector( _( "Generator File" ), Path, FullFileName, wxEmptyString,
                                   wxFileSelectorDefaultWildcardStr, wxFD_OPEN, this );

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
