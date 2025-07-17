/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <wx/debug.h>

// For some obscure reason, needed on msys2 with some wxWidgets versions (3.0) to avoid
// undefined symbol at link stage (due to use of #include <pegtl.hpp>)
// Should not create issues on other platforms
#include <wx/menu.h>

#include <project/project_file.h>
#include <sch_edit_frame.h>
#include <kiway.h>
#include <confirm.h>
#include <bitmaps.h>
#include <wildcards_and_files_ext.h>
#include <widgets/tuner_slider.h>
#include <widgets/grid_color_swatch_helpers.h>
#include <widgets/wx_grid.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tools/simulator_control.h>
#include <tools/sch_actions.h>
#include <string_utils.h>
#include <pgm_base.h>
#include "ngspice.h"
#include <sim/simulator_frame.h>
#include <sim/simulator_frame_ui.h>
#include <sim/sim_plot_tab.h>
#include <sim/spice_simulator.h>
#include <sim/simulator_reporter.h>
#include <eeschema_settings.h>
#include <advanced_config.h>
#include <sim/toolbars_simulator_frame.h>
#include <settings/settings_manager.h>

#include <memory>


// Reporter is stored by pointer in KIBIS, so keep this here to avoid crashes
static WX_STRING_REPORTER s_reporter;


class SIM_THREAD_REPORTER : public SIMULATOR_REPORTER
{
public:
    SIM_THREAD_REPORTER( SIMULATOR_FRAME* aParent ) :
        m_parent( aParent )
    {
    }

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override
    {
        wxCommandEvent* event = new wxCommandEvent( EVT_SIM_REPORT );
        event->SetString( aText );
        wxQueueEvent( m_parent, event );
        return *this;
    }

    bool HasMessage() const override
    {
        return false;       // Technically "indeterminate" rather than false.
    }

    void OnSimStateChange( SIMULATOR* aObject, SIM_STATE aNewState ) override
    {
        wxCommandEvent* event = nullptr;

        switch( aNewState )
        {
        case SIM_IDLE:    event = new wxCommandEvent( EVT_SIM_FINISHED ); break;
        case SIM_RUNNING: event = new wxCommandEvent( EVT_SIM_STARTED );  break;
        default:          wxFAIL;                                         return;
        }

        wxQueueEvent( m_parent, event );
    }

private:
    SIMULATOR_FRAME* m_parent;
};


BEGIN_EVENT_TABLE( SIMULATOR_FRAME, KIWAY_PLAYER )
    EVT_MENU( wxID_EXIT, SIMULATOR_FRAME::onExit )
    EVT_MENU( wxID_CLOSE, SIMULATOR_FRAME::onExit )
END_EVENT_TABLE()


SIMULATOR_FRAME::SIMULATOR_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        KIWAY_PLAYER( aKiway, aParent, FRAME_SIMULATOR, _( "Simulator" ), wxDefaultPosition,
                      wxDefaultSize, wxDEFAULT_FRAME_STYLE, wxT( "simulator" ), unityScale ),
        m_schematicFrame( nullptr ),
        m_toolBar( nullptr ),
        m_ui( nullptr ),
        m_simFinished( false ),
        m_workbookModified( false )
{
    m_schematicFrame = (SCH_EDIT_FRAME*) Kiway().Player( FRAME_SCH, false );
    wxASSERT( m_schematicFrame );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::simulator ) );
    SetIcon( icon );

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( mainSizer );

    m_infoBar = new WX_INFOBAR( this );
    mainSizer->Add( m_infoBar, 0, wxEXPAND, 0 );

    m_tbTopMain = new ACTION_TOOLBAR( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE|wxAUI_TB_HORZ_LAYOUT|wxAUI_TB_PLAIN_BACKGROUND );
    m_tbTopMain->Realize();
    mainSizer->Add( m_tbTopMain, 0, wxEXPAND, 5 );

    m_ui = new SIMULATOR_FRAME_UI( this, m_schematicFrame );
    mainSizer->Add( m_ui, 1, wxEXPAND, 5 );

    m_simulator = SIMULATOR::CreateInstance( "ngspice" );

    if( !m_simulator )
        throw SIMULATOR_INIT_ERR( "Failed to create simulator instance" );

    LoadSettings( config() );

    std::shared_ptr<NGSPICE_SETTINGS> cfg = Prj().GetProjectFile().m_SchematicSettings->m_NgspiceSettings;

    if( cfg->GetWorkbookFilename().IsEmpty() )
        cfg->SetCompatibilityMode( NGSPICE_COMPATIBILITY_MODE::LT_PSPICE );

    m_simulator->Init();

    m_reporter = new SIM_THREAD_REPORTER( this );
    m_simulator->SetReporter( m_reporter );

    m_circuitModel = std::make_shared<SPICE_CIRCUIT_MODEL>( &m_schematicFrame->Schematic() );

    setupTools();
    setupUIConditions();

    // Set the tool manager for the toolbar here, since the tool manager didn't exist when the toolbar
    // was created.
    m_tbTopMain->SetToolManager( m_toolManager );

    m_toolbarSettings = GetToolbarSettings<SIMULATOR_TOOLBAR_SETTINGS>( "sim-toolbars" );
    configureToolbars();
    RecreateToolbars();
    ReCreateMenuBar();

    Bind( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIMULATOR_FRAME::onExit ), this, wxID_EXIT );

    Bind( EVT_SIM_UPDATE, &SIMULATOR_FRAME::onUpdateSim, this );
    Bind( EVT_SIM_REPORT, &SIMULATOR_FRAME::onSimReport, this );
    Bind( EVT_SIM_STARTED, &SIMULATOR_FRAME::onSimStarted, this );
    Bind( EVT_SIM_FINISHED, &SIMULATOR_FRAME::onSimFinished, this );

    // Ensure new items are taken in account by sizers:
    Layout();

    // resize the subwindows size. At least on Windows, calling wxSafeYield before
    // resizing the subwindows forces the wxSplitWindows size events automatically generated
    // by wxWidgets to be executed before our resize code.
    // Otherwise, the changes made by setSubWindowsSashSize are overwritten by one these
    // events
    wxSafeYield();
    m_ui->SetSubWindowsSashSize();

    // Ensure the window is on top
    Raise();

    m_ui->InitWorkbook();
    UpdateTitle();
}


SIMULATOR_FRAME::~SIMULATOR_FRAME()
{
    NULL_REPORTER devnull;

    m_simulator->Attach( nullptr, wxEmptyString, 0, wxEmptyString, devnull );
    m_simulator->SetReporter( nullptr );
    delete m_reporter;
}


void SIMULATOR_FRAME::setupTools()
{
    // Create the manager
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( nullptr, nullptr, nullptr, config(), this );

    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    // Attach the events to the tool dispatcher
    Bind( wxEVT_CHAR, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );
    Bind( wxEVT_CHAR_HOOK, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new SIMULATOR_CONTROL );
    m_toolManager->InitTools();
}


void SIMULATOR_FRAME::ShowChangedLanguage()
{
    EDA_BASE_FRAME::ShowChangedLanguage();

    UpdateTitle();

    m_ui->ShowChangedLanguage();
}


void SIMULATOR_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( aCfg ) )
    {
        EDA_BASE_FRAME::LoadSettings( cfg );
        m_ui->LoadSettings( cfg );
    }

    if( m_simulator )
        m_simulator->Settings() = Prj().GetProjectFile().m_SchematicSettings->m_NgspiceSettings;
}


void SIMULATOR_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( aCfg ) )
    {
        EDA_BASE_FRAME::SaveSettings( cfg );
        m_ui->SaveSettings( cfg );
    }

    bool modified = Prj().GetProjectFile().m_SchematicSettings->m_NgspiceSettings->SaveToFile();

    if( m_schematicFrame && modified )
        m_schematicFrame->OnModify();
}


void SIMULATOR_FRAME::CommonSettingsChanged( int aFlags )
{
    KIWAY_PLAYER::CommonSettingsChanged( aFlags );

    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        m_ui->ApplyPreferences( cfg->m_Simulator.preferences );
}


WINDOW_SETTINGS* SIMULATOR_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        return &cfg->m_Simulator.window;

    wxFAIL_MSG( wxT( "SIMULATOR not running with EESCHEMA_SETTINGS" ) );
    return &aCfg->m_Window;     // non-null fail-safe
}


wxString SIMULATOR_FRAME::GetCurrentSimCommand() const
{
    if( m_ui->GetCurrentSimTab() )
        return m_ui->GetCurrentSimTab()->GetSimCommand();
    else
        return m_circuitModel->GetSchTextSimCommand();
}


SIM_TYPE SIMULATOR_FRAME::GetCurrentSimType() const
{
    return SPICE_CIRCUIT_MODEL::CommandToSimType( GetCurrentSimCommand() );
}


int SIMULATOR_FRAME::GetCurrentOptions() const
{
    if( SIM_TAB* simTab = m_ui->GetCurrentSimTab() )
        return simTab->GetSimOptions();
    else
        return NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS;
}


void SIMULATOR_FRAME::UpdateTitle()
{
    bool                              unsaved = true;
    bool                              readOnly = false;
    wxString                          title;
    std::shared_ptr<NGSPICE_SETTINGS> cfg = Prj().GetProjectFile().m_SchematicSettings->m_NgspiceSettings;
    wxFileName                        filename = Prj().AbsolutePath( cfg->GetWorkbookFilename() );

    if( filename.IsOk() && filename.FileExists() )
    {
        unsaved = false;
        readOnly = !filename.IsFileWritable();
    }

    if( m_workbookModified )
        title = wxT( "*" ) + filename.GetName();
    else
        title = filename.GetName();

    if( readOnly )
        title += wxS( " " ) + _( "[Read Only]" );

    if( unsaved )
        title += wxS( " " ) + _( "[Unsaved]" );

    title += wxT( " \u2014 " ) + _( "SPICE Simulator" );

    SetTitle( title );
}


// Don't let the dialog grow too tall: you may not be able to get to the OK button
#define MAX_MESSAGES 20

void SIMULATOR_FRAME::showNetlistErrors( const WX_STRING_REPORTER& aReporter )
{
    if( !aReporter.HasMessage() )
        return;

    wxArrayString lines = wxSplit( aReporter.GetMessages(), '\n' );

    if( lines.size() > MAX_MESSAGES )
    {
        lines.RemoveAt( MAX_MESSAGES, lines.size() - MAX_MESSAGES );
        lines.Add( wxS( "..." ) );
    }

    if( aReporter.HasMessageOfSeverity( RPT_SEVERITY_UNDEFINED | RPT_SEVERITY_ERROR ) )
    {
        DisplayErrorMessage( this, _( "Errors during netlist generation." ),
                             wxJoin( lines, '\n' ) );
    }
    else if( aReporter.HasMessageOfSeverity( RPT_SEVERITY_WARNING ) )
    {
        DisplayInfoMessage( this, _( "Warnings during netlist generation." ),
                             wxJoin( lines, '\n' ) );
    }
}


bool SIMULATOR_FRAME::LoadSimulator( const wxString& aSimCommand, unsigned aSimOptions )
{
    s_reporter.Clear();

    if( !m_schematicFrame->ReadyToNetlist( _( "Simulator requires a fully annotated schematic." ) ) )
        return false;

    // If we are using the new connectivity, make sure that we do a full-rebuild
    if( ADVANCED_CFG::GetCfg().m_IncrementalConnectivity )
        m_schematicFrame->RecalculateConnections( nullptr, GLOBAL_CLEANUP );

    bool success = m_simulator->Attach( m_circuitModel, aSimCommand, aSimOptions,
                                        Prj().GetProjectPath(), s_reporter );

    showNetlistErrors( s_reporter );

    return success;
}


void SIMULATOR_FRAME::ReloadSimulator( const wxString& aSimCommand, unsigned aSimOptions )
{
    s_reporter.Clear();

    m_simulator->Attach( m_circuitModel, aSimCommand, aSimOptions, Prj().GetProjectPath(),
                         s_reporter );

    showNetlistErrors( s_reporter );
}


void SIMULATOR_FRAME::StartSimulation()
{
    SIM_TAB* simTab = m_ui->GetCurrentSimTab();

    if( !simTab )
        return;

    if( simTab->GetSimCommand().Upper().StartsWith( wxT( "FFT" ) )
        || simTab->GetSimCommand().Upper().Contains( wxT( "\nFFT" ) ) )
    {
        wxString tranSpicePlot;

        if( SIM_TAB* tranPlotTab = m_ui->GetSimTab( ST_TRAN ) )
            tranSpicePlot = tranPlotTab->GetSpicePlotName();

        if( tranSpicePlot.IsEmpty() )
        {
            DisplayErrorMessage( this, _( "You must run a TRAN simulation first; its results"
                                          "will be used for the fast Fourier transform." ) );
        }
        else
        {
            m_simulator->Command( "setplot " + tranSpicePlot.ToStdString() );

            wxArrayString commands = wxSplit( simTab->GetSimCommand(), '\n' );

            for( const wxString& command : commands )
            {
                wxBusyCursor wait;
                m_simulator->Command( command.ToStdString() );
            }

            simTab->SetSpicePlotName( m_simulator->CurrentPlotName() );
            m_ui->OnSimRefresh( true );

#if 0
            m_simulator->Command( "setplot" );  // Print available plots to console
            m_simulator->Command( "display" );  // Print vectors in current plot to console
#endif
        }

        return;
    }
    else
    {
        if( m_ui->GetSimTabIndex( simTab ) == 0
                && m_circuitModel->GetSchTextSimCommand() != simTab->GetLastSchTextSimCommand() )
        {
            if( simTab->GetLastSchTextSimCommand().IsEmpty()
                || IsOK( this, _( "Schematic sheet simulation command directive has changed.  "
                                  "Do you wish to update the Simulation Command?" ) ) )
            {
                simTab->SetSimCommand( m_circuitModel->GetSchTextSimCommand() );
                simTab->SetLastSchTextSimCommand( simTab->GetSimCommand() );
                OnModify();
            }
        }
    }

    if( !LoadSimulator( simTab->GetSimCommand(), simTab->GetSimOptions() ) )
        return;

    std::unique_lock<std::mutex> simulatorLock( m_simulator->GetMutex(), std::try_to_lock );

    if( simulatorLock.owns_lock() )
    {
        m_simFinished = false;

        m_ui->OnSimUpdate();
        m_simulator->Run();

        // Netlist from schematic may have changed; update signals list, measurements list,
        // etc.
        m_ui->OnPlotSettingsChanged();
    }
    else
    {
        DisplayErrorMessage( this, _( "Another simulation is already running." ) );
    }
}


SIM_TAB* SIMULATOR_FRAME::NewSimTab( const wxString& aSimCommand )
{
    return m_ui->NewSimTab( aSimCommand );
}


const std::vector<wxString> SIMULATOR_FRAME::SimPlotVectors()
{
    return m_ui->SimPlotVectors();
}


const std::vector<wxString> SIMULATOR_FRAME::Signals()
{
    return m_ui->Signals();
}


const std::map<int, wxString>& SIMULATOR_FRAME::UserDefinedSignals()
{
    return m_ui->UserDefinedSignals();
}


void SIMULATOR_FRAME::SetUserDefinedSignals( const std::map<int, wxString>& aSignals )
{
    m_ui->SetUserDefinedSignals( aSignals );
}


void SIMULATOR_FRAME::AddVoltageTrace( const wxString& aNetName )
{
    m_ui->AddTrace( aNetName, SPT_VOLTAGE );
}


void SIMULATOR_FRAME::AddCurrentTrace( const wxString& aDeviceName )
{
    m_ui->AddTrace( aDeviceName, SPT_CURRENT );
}


void SIMULATOR_FRAME::AddTuner( const SCH_SHEET_PATH& aSheetPath, SCH_SYMBOL* aSymbol )
{
    m_ui->AddTuner( aSheetPath, aSymbol );
}


SIM_TAB* SIMULATOR_FRAME::GetCurrentSimTab() const
{
    return m_ui->GetCurrentSimTab();
}


bool SIMULATOR_FRAME::LoadWorkbook( const wxString& aPath )
{
    if( m_ui->LoadWorkbook( aPath ) )
    {
        UpdateTitle();

        // Successfully loading a workbook does not count as modifying it.  Clear the modified
        // flag after all the EVT_WORKBOOK_MODIFIED events have been processed.
        CallAfter( [this]()
                   {
                       m_workbookModified = false;
                   } );

        return true;
    }

    DisplayErrorMessage( this, wxString::Format( _( "Unable to load or parse file %s" ), aPath ) );
    return false;
}


bool SIMULATOR_FRAME::SaveWorkbook( const wxString& aPath )
{
    if( m_ui->SaveWorkbook( aPath ) )
    {
        m_workbookModified = false;
        UpdateTitle();

        return true;
    }

    return false;
}


void SIMULATOR_FRAME::ToggleSimConsole()
{
    m_ui->ToggleSimConsole();
}


void SIMULATOR_FRAME::ToggleSimSidePanel()
{
    m_ui->ToggleSimSidePanel();
}


void SIMULATOR_FRAME::ToggleDarkModePlots()
{
    m_ui->ToggleDarkModePlots();
}


bool SIMULATOR_FRAME::EditAnalysis()
{
    SIM_TAB*           simTab = m_ui->GetCurrentSimTab();
    DIALOG_SIM_COMMAND dlg( this, m_circuitModel, m_simulator->Settings() );

    s_reporter.Clear();

    if( !simTab )
        return false;

    m_circuitModel->ReadSchematicAndLibraries( NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS,
                                               s_reporter );

    showNetlistErrors( s_reporter );

    dlg.SetSimCommand( simTab->GetSimCommand() );
    dlg.SetSimOptions( simTab->GetSimOptions() );
    dlg.SetPlotSettings( simTab );

    if( dlg.ShowModal() == wxID_OK )
    {
        simTab->SetSimCommand( dlg.GetSimCommand() );
        dlg.ApplySettings( simTab );
        m_ui->OnPlotSettingsChanged();
        OnModify();
        return true;
    }

    return false;
}


bool SIMULATOR_FRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    if( m_workbookModified )
    {
        wxFileName filename = m_simulator->Settings()->GetWorkbookFilename();

        if( filename.GetName().IsEmpty() )
        {
            if( Prj().GetProjectName().IsEmpty() )
                filename.SetFullName( wxT( "noname.wbk" ) );
            else
                filename.SetFullName( Prj().GetProjectName() + wxT( ".wbk" ) );
        }

        return HandleUnsavedChanges( this, _( "Save changes to workbook?" ),
                [&]() -> bool
                {
                    return SaveWorkbook( Prj().AbsolutePath( filename.GetFullName() ) );
                } );
    }

    return true;
}


void SIMULATOR_FRAME::doCloseWindow()
{
    if( m_simulator->IsRunning() )
        m_simulator->Stop();

    // Prevent memory leak on exit by deleting all simulation vectors
    m_simulator->Clean();

    // Cancel a running simProbe or simTune tool
    m_schematicFrame->GetToolManager()->PostAction( ACTIONS::cancelInteractive );

    SaveSettings( config() );

    m_simulator->Settings().reset();

    Destroy();
}


void SIMULATOR_FRAME::setupUIConditions()
{
    EDA_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*   mgr = m_toolManager->GetActionManager();
    wxASSERT( mgr );

    auto showGridCondition =
            [this]( const SELECTION& aSel )
            {
                SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );
                return plotTab && plotTab->IsGridShown();
            };

    auto showLegendCondition =
            [this]( const SELECTION& aSel )
            {
                SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );
                return plotTab && plotTab->IsLegendShown();
            };

    auto showDottedCondition =
            [this]( const SELECTION& aSel )
            {
                SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );
                return plotTab && plotTab->GetDottedSecondary();
            };

    auto darkModePlotCondition =
            [this]( const SELECTION& aSel )
            {
                return m_ui->DarkModePlots();
            };

    auto simRunning =
            [this]( const SELECTION& aSel )
            {
                return m_simulator && m_simulator->IsRunning();
            };

    auto simFinished =
            [this]( const SELECTION& aSel )
            {
                return m_simFinished;
            };

    auto haveSim =
            [this]( const SELECTION& aSel )
            {
                return GetCurrentSimTab() != nullptr;
            };

    auto havePlot =
            [this]( const SELECTION& aSel )
            {
                return dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() ) != nullptr;
            };

    auto haveZoomUndo =
            [this]( const SELECTION& aSel )
            {
                SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );
                return plotTab && plotTab->GetPlotWin()->UndoZoomStackSize() > 0;
            };

    auto haveZoomRedo =
            [this]( const SELECTION& aSel )
            {
                SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );
                return plotTab && plotTab->GetPlotWin()->RedoZoomStackSize() > 0;
            };

    // clang-format off
    auto isSimConsoleShown =
            [this]( const SELECTION& aSel )
            {
                bool aBool = false;

                if( m_simulator )
                    return m_ui->IsSimConsoleShown();

                return aBool;
            };

    auto isSimSidePanelShown =
            [this]( const SELECTION& aSel )
            {
                bool aBool = false;

                if( m_simulator )
                    return m_ui->IsSimSidePanelShown();

                return aBool;
            };
    // clang-format on

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )
    // clang-format off
    mgr->SetConditions( SCH_ACTIONS::openWorkbook,          ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( SCH_ACTIONS::saveWorkbook,          ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( SCH_ACTIONS::saveWorkbookAs,        ENABLE( SELECTION_CONDITIONS::ShowAlways ) );

    mgr->SetConditions( SCH_ACTIONS::exportPlotAsPNG,       ENABLE( havePlot ) );
    mgr->SetConditions( SCH_ACTIONS::exportPlotAsCSV,       ENABLE( havePlot ) );
    mgr->SetConditions( SCH_ACTIONS::exportPlotToClipboard, ENABLE( havePlot ) );
    mgr->SetConditions( SCH_ACTIONS::exportPlotToSchematic, ENABLE( havePlot ) );

    mgr->SetConditions( SCH_ACTIONS::toggleSimSidePanel,    CHECK( isSimSidePanelShown ) );
    mgr->SetConditions( SCH_ACTIONS::toggleSimConsole,      CHECK( isSimConsoleShown ) );

    mgr->SetConditions( ACTIONS::zoomUndo,                  ENABLE( haveZoomUndo ) );
    mgr->SetConditions( ACTIONS::zoomRedo,                  ENABLE( haveZoomRedo ) );
    mgr->SetConditions( SCH_ACTIONS::toggleGrid,            CHECK( showGridCondition ) );
    mgr->SetConditions( SCH_ACTIONS::toggleLegend,          CHECK( showLegendCondition ) );
    mgr->SetConditions( SCH_ACTIONS::toggleDottedSecondary, CHECK( showDottedCondition ) );
    mgr->SetConditions( SCH_ACTIONS::toggleDarkModePlots,   CHECK( darkModePlotCondition ) );

    mgr->SetConditions( SCH_ACTIONS::newAnalysisTab,        ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( SCH_ACTIONS::simAnalysisProperties, ENABLE( haveSim ) );
    mgr->SetConditions( SCH_ACTIONS::runSimulation,         ENABLE( !simRunning ) );
    mgr->SetConditions( SCH_ACTIONS::stopSimulation,        ENABLE( simRunning ) );
    mgr->SetConditions( SCH_ACTIONS::simProbe,              ENABLE( simFinished ) );
    mgr->SetConditions( SCH_ACTIONS::simTune,               ENABLE( simFinished ) );
    mgr->SetConditions( SCH_ACTIONS::showNetlist,           ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    // clang-format on
#undef CHECK
#undef ENABLE
}


void SIMULATOR_FRAME::onSimStarted( wxCommandEvent& aEvent )
{
    SetCursor( wxCURSOR_ARROWWAIT );
}


void SIMULATOR_FRAME::onSimFinished( wxCommandEvent& aEvent )
{
    // Sometimes (for instance with a directive like wrdata my_file.csv "my_signal")
    // the simulator is in idle state (simulation is finished), but still running, during
    // the time the file is written. So gives a slice of time to fully finish the work:
    if( m_simulator->IsRunning() )
    {
        int max_time = 40;      // For a max timeout = 2s

        do
        {
            wxMilliSleep( 50 );
            wxYield();

            if( max_time )
                max_time--;

        } while( max_time && m_simulator->IsRunning() );
    }

    // ensure the shown cursor is the default cursor, not the wxCURSOR_ARROWWAIT set when
    // staring the simulator in onSimStarted:
    SetCursor( wxNullCursor );

    // Is a warning message useful if the simulatior is still running?
    SCHEMATIC& schematic = m_schematicFrame->Schematic();
    schematic.ClearOperatingPoints();

    m_simFinished = true;

    m_ui->OnSimRefresh( true );

    m_schematicFrame->RefreshOperatingPointDisplay();
    m_schematicFrame->GetCanvas()->Refresh();
}


void SIMULATOR_FRAME::onUpdateSim( wxCommandEvent& aEvent )
{
    static bool updateInProgress = false;

    // skip update when events are triggered too often and previous call didn't end yet
    if( updateInProgress )
        return;

    updateInProgress = true;

    if( m_simulator->IsRunning() )
        m_simulator->Stop();

    std::unique_lock<std::mutex> simulatorLock( m_simulator->GetMutex(), std::try_to_lock );

    if( simulatorLock.owns_lock() )
    {
        m_ui->OnSimUpdate();
        m_simulator->Run();
    }
    else
    {
        DisplayErrorMessage( this, _( "Another simulation is already running." ) );
    }

    updateInProgress = false;
}


void SIMULATOR_FRAME::onSimReport( wxCommandEvent& aEvent )
{
    m_ui->OnSimReport( aEvent.GetString() );
}


void SIMULATOR_FRAME::onExit( wxCommandEvent& aEvent )
{
    if( aEvent.GetId() == wxID_EXIT )
        Kiway().OnKiCadExit();

    if( aEvent.GetId() == wxID_CLOSE )
        Close( false );
}


void SIMULATOR_FRAME::OnModify()
{
    KIWAY_PLAYER::OnModify();
    m_workbookModified = true;
    UpdateTitle();
}


wxDEFINE_EVENT( EVT_SIM_UPDATE, wxCommandEvent );
wxDEFINE_EVENT( EVT_SIM_REPORT, wxCommandEvent );

wxDEFINE_EVENT( EVT_SIM_STARTED, wxCommandEvent );
wxDEFINE_EVENT( EVT_SIM_FINISHED, wxCommandEvent );
