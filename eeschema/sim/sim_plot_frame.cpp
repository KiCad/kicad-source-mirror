/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/stc/stc.h>

#include <project/project_file.h>
#include <sch_edit_frame.h>
#include <eeschema_id.h>
#include <kiway.h>
#include <confirm.h>
#include <bitmaps.h>
#include <wildcards_and_files_ext.h>
#include <widgets/tuner_slider.h>
#include <dialogs/dialog_signal_list.h>
#include "netlist_exporter_pspice_sim.h"
#include <pgm_base.h>
#include "ngspice.h"
#include "sim_plot_colors.h"
#include "sim_plot_frame.h"
#include "sim_plot_panel.h"
#include "spice_simulator.h"
#include "spice_reporter.h"
#include <menus_helpers.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <eeschema_settings.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <dialog_shim.h>

SIM_PLOT_TYPE operator|( SIM_PLOT_TYPE aFirst, SIM_PLOT_TYPE aSecond )
{
    int res = (int) aFirst | (int) aSecond;

    return (SIM_PLOT_TYPE) res;
}


class SIM_THREAD_REPORTER : public SPICE_REPORTER
{
public:
    SIM_THREAD_REPORTER( SIM_PLOT_FRAME* aParent ) :
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

    void OnSimStateChange( SPICE_SIMULATOR* aObject, SIM_STATE aNewState ) override
    {
        wxCommandEvent* event = NULL;

        switch( aNewState )
        {
        case SIM_IDLE:
            event = new wxCommandEvent( EVT_SIM_FINISHED );
            break;

        case SIM_RUNNING:
            event = new wxCommandEvent( EVT_SIM_STARTED );
            break;

        default:
            wxFAIL;
            return;
        }

        wxQueueEvent( m_parent, event );
    }

private:
    SIM_PLOT_FRAME* m_parent;
};


// Store the path of saved workbooks during the session
wxString SIM_PLOT_FRAME::m_savedWorkbooksPath;


SIM_PLOT_FRAME::SIM_PLOT_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        SIM_PLOT_FRAME_BASE( aParent ),
        m_lastSimPlot( nullptr ),
        m_plotNumber( 0 )
{
    SetKiway( this, aKiway );
    m_signalsIconColorList = NULL;

    m_schematicFrame = (SCH_EDIT_FRAME*) Kiway().Player( FRAME_SCH, false );

    if( m_schematicFrame == NULL )
        throw std::runtime_error( "There is no schematic window" );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::simulator ) );
    SetIcon( icon );

    m_simulator = SPICE_SIMULATOR::CreateInstance( "ngspice" );

    if( !m_simulator )
    {
        throw std::runtime_error( "Could not create simulator instance" );
        return;
    }

    // Get the previous size and position of windows:
    LoadSettings( config() );

    // Prepare the color list to plot traces
    SIM_PLOT_COLORS::FillDefaultColorList( GetPlotBgOpt() );

    // Give icons to menuitems
    setIconsForMenuItems();

    m_simulator->Init();

    if( m_savedWorkbooksPath.IsEmpty() )
        m_savedWorkbooksPath = Prj().GetProjectPath();

    m_reporter = new SIM_THREAD_REPORTER( this );
    m_simulator->SetReporter( m_reporter );

    // the settings dialog will be created later, on demand.
    // if created in the ctor, for some obscure reason, there is an issue
    // on Windows: when open it, the simulator frame is sent to the background.
    // instead of being behind the dialog frame (as it does)
    m_settingsDlg = nullptr;

    updateNetlistExporter();

    Connect( EVT_SIM_UPDATE, wxCommandEventHandler( SIM_PLOT_FRAME::onSimUpdate ), NULL, this );
    Connect( EVT_SIM_REPORT, wxCommandEventHandler( SIM_PLOT_FRAME::onSimReport ), NULL, this );
    Connect( EVT_SIM_STARTED, wxCommandEventHandler( SIM_PLOT_FRAME::onSimStarted ), NULL, this );
    Connect( EVT_SIM_FINISHED, wxCommandEventHandler( SIM_PLOT_FRAME::onSimFinished ), NULL, this );
    Connect( EVT_SIM_CURSOR_UPDATE, wxCommandEventHandler( SIM_PLOT_FRAME::onCursorUpdate ),
             NULL, this );

    // Toolbar buttons
    m_toolSimulate = m_toolBar->AddTool( ID_SIM_RUN, _( "Run/Stop Simulation" ),
            KiBitmap( BITMAPS::sim_run ), _( "Run Simulation" ), wxITEM_NORMAL );
    m_toolAddSignals = m_toolBar->AddTool( ID_SIM_ADD_SIGNALS, _( "Add Signals" ),
            KiBitmap( BITMAPS::sim_add_signal ), _( "Add signals to plot" ), wxITEM_NORMAL );
    m_toolProbe = m_toolBar->AddTool( ID_SIM_PROBE,  _( "Probe" ),
            KiBitmap( BITMAPS::sim_probe ), _( "Probe signals on the schematic" ), wxITEM_NORMAL );
    m_toolTune = m_toolBar->AddTool( ID_SIM_TUNE, _( "Tune" ),
            KiBitmap( BITMAPS::sim_tune ), _( "Tune component values" ), wxITEM_NORMAL );
    m_toolSettings = m_toolBar->AddTool( wxID_ANY, _( "Sim Parameters" ),
            KiBitmap( BITMAPS::config ), _( "Simulation parameters and settings" ), wxITEM_NORMAL );

    Connect( m_toolSimulate->GetId(), wxEVT_COMMAND_TOOL_CLICKED,
             wxCommandEventHandler( SIM_PLOT_FRAME::onSimulate ), NULL, this );
    Connect( m_toolAddSignals->GetId(), wxEVT_COMMAND_TOOL_CLICKED,
             wxCommandEventHandler( SIM_PLOT_FRAME::onAddSignal ), NULL, this );
    Connect( m_toolProbe->GetId(), wxEVT_COMMAND_TOOL_CLICKED,
             wxCommandEventHandler( SIM_PLOT_FRAME::onProbe ), NULL, this );
    Connect( m_toolTune->GetId(), wxEVT_COMMAND_TOOL_CLICKED,
             wxCommandEventHandler( SIM_PLOT_FRAME::onTune ), NULL, this );
    Connect( m_toolSettings->GetId(), wxEVT_COMMAND_TOOL_CLICKED,
             wxCommandEventHandler( SIM_PLOT_FRAME::onSettings ), NULL, this );

    // Bind toolbar buttons event to existing menu event handlers, so they behave the same
    Bind( wxEVT_COMMAND_MENU_SELECTED, &SIM_PLOT_FRAME::onSimulate, this,
          m_runSimulation->GetId() );
    Bind( wxEVT_COMMAND_MENU_SELECTED, &SIM_PLOT_FRAME::onAddSignal, this, m_addSignals->GetId() );
    Bind( wxEVT_COMMAND_MENU_SELECTED, &SIM_PLOT_FRAME::onProbe, this, m_probeSignals->GetId() );
    Bind( wxEVT_COMMAND_MENU_SELECTED, &SIM_PLOT_FRAME::onTune, this, m_tuneValue->GetId() );
    Bind( wxEVT_COMMAND_MENU_SELECTED, &SIM_PLOT_FRAME::onShowNetlist, this,
          m_showNetlist->GetId() );
    Bind( wxEVT_COMMAND_MENU_SELECTED, &SIM_PLOT_FRAME::onSettings, this, m_settings->GetId() );

    m_toolBar->Realize();

#ifndef wxHAS_NATIVE_TABART
    // Non-native default tab art has ulgy gradients we don't want
    m_plotNotebook->SetArtProvider( new wxAuiSimpleTabArt() );
#endif

    // Ensure new items are taken in account by sizers:
    Layout();

    // resize the subwindows size. At least on Windows, calling wxSafeYield before
    // resizing the subwindows forces the wxSplitWindows size events automatically generated
    // by wxWidgets to be executed before our resize code.
    // Otherwise, the changes made by setSubWindowsSashSize are overwritten by one these
    // events
    wxSafeYield();
    setSubWindowsSashSize();

    // Ensure the window is on top
    Raise();

    initWorkbook();
}


SIM_PLOT_FRAME::~SIM_PLOT_FRAME()
{
    m_simulator->SetReporter( nullptr );
    delete m_reporter;
    delete m_signalsIconColorList;

    if( m_settingsDlg )
        m_settingsDlg->Destroy();
}


void SIM_PLOT_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( aCfg );
    wxASSERT( cfg );

    if( cfg )
    {
        EDA_BASE_FRAME::LoadSettings( cfg );

        // Read subwindows sizes (should be > 0 )
        m_splitterLeftRightSashPosition      = cfg->m_Simulator.plot_panel_width;
        m_splitterPlotAndConsoleSashPosition = cfg->m_Simulator.plot_panel_height;
        m_splitterSignalsSashPosition        = cfg->m_Simulator.signal_panel_height;
        m_splitterTuneValuesSashPosition     = cfg->m_Simulator.cursors_panel_height;
        m_plotUseWhiteBg                     = cfg->m_Simulator.white_background;
    }

    PROJECT_FILE& project = Prj().GetProjectFile();

    NGSPICE* currentSim = dynamic_cast<NGSPICE*>( m_simulator.get() );

    if( currentSim )
        m_simulator->Settings() = project.m_SchematicSettings->m_NgspiceSimulatorSettings;
}


void SIM_PLOT_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( aCfg );
    wxASSERT( cfg );

    if( cfg )
    {
        EDA_BASE_FRAME::SaveSettings( cfg );

        cfg->m_Simulator.plot_panel_width     = m_splitterLeftRight->GetSashPosition();
        cfg->m_Simulator.plot_panel_height    = m_splitterPlotAndConsole->GetSashPosition();
        cfg->m_Simulator.signal_panel_height  = m_splitterSignals->GetSashPosition();
        cfg->m_Simulator.cursors_panel_height = m_splitterTuneValues->GetSashPosition();
        cfg->m_Simulator.white_background     = m_plotUseWhiteBg;
    }

    if( !m_isNonUserClose )     // If we're exiting the project has already been released.
    {
        PROJECT_FILE& project = Prj().GetProjectFile();

        if( project.m_SchematicSettings )
            project.m_SchematicSettings->m_NgspiceSimulatorSettings->SaveToFile();

        if( m_schematicFrame )
            m_schematicFrame->SaveProjectSettings();
    }
}


WINDOW_SETTINGS* SIM_PLOT_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( aCfg );
    wxASSERT( cfg );

    return cfg ? &cfg->m_Simulator.window : nullptr;
}


void SIM_PLOT_FRAME::initWorkbook()
{
    m_workbook = std::make_unique<SIM_WORKBOOK>();

    if( !m_simulator->Settings()->GetWorkbookFilename().IsEmpty() )
    {
        wxFileName filename = m_simulator->Settings()->GetWorkbookFilename();
        filename.SetPath( Prj().GetProjectPath() );

        if( !loadWorkbook( filename.GetFullPath() ) )
        {
            // Empty the workbook filename to prevent error messages from appearing again
            m_simulator->Settings()->SetWorkbookFilename( "" );
        }
    }
}


void SIM_PLOT_FRAME::updateTitle()
{
    wxFileName filename = Prj().AbsolutePath(
            m_simulator->Settings()->GetWorkbookFilename() );

    bool readOnly = false;
    bool unsaved = false;

    if( filename.IsOk() && filename.FileExists() )
        readOnly = !filename.IsFileWritable();
    else
        unsaved = true;

    wxString title;

    if( m_workbook->IsModified() )
        title = wxT( "*" ) + filename.GetName();
    else
        title = filename.GetName();

    if( readOnly )
        title += wxS( " " ) + _( "[Read Only]" );

    if( unsaved )
        title += wxS( " " ) + _( "[Unsaved]" );

    title += wxT( " \u2014 " ) + _( "Spice Simulator" );

    SetTitle( title );
}


void SIM_PLOT_FRAME::updateWorkbook()
{
    // We need to keep track of the plot panel positions
    for( unsigned int i = 0; i < m_plotNotebook->GetPageCount(); i++ )
        m_workbook->SetPlotPanelPosition(
            dynamic_cast<SIM_PANEL_BASE*>( m_plotNotebook->GetPage( i ) ), i );
}


void SIM_PLOT_FRAME::updateFrame()
{
    updateTitle();
}


// A small helper struct to handle bitmaps initialisation in menus
struct BM_MENU_INIT_ITEM
{
    int m_MenuId;
    BITMAPS m_Bitmap;
};


void SIM_PLOT_FRAME::setIconsForMenuItems()
{
    // Give icons to menuitems of the main menubar
    BM_MENU_INIT_ITEM bm_list[]
    {
        // File menu:
        { wxID_NEW, BITMAPS::simulator },
        { wxID_OPEN, BITMAPS::directory_open },
        { wxID_SAVE, BITMAPS::save },
        { ID_SAVE_AS_IMAGE, BITMAPS::export_file },
        { ID_SAVE_AS_CSV, BITMAPS::export_file },
        { wxID_CLOSE, BITMAPS::exit },

        // simulator menu:
        { ID_MENU_RUN_SIM, BITMAPS::sim_run },
        { ID_MENU_ADD_SIGNAL, BITMAPS::sim_add_signal },
        { ID_MENU_PROBE_SIGNALS, BITMAPS::sim_probe },
        { ID_MENU_TUNE_SIGNALS, BITMAPS::sim_tune },
        { ID_MENU_SHOW_NETLIST, BITMAPS::netlist },
        { ID_MENU_SET_SIMUL, BITMAPS::config },

        // View menu
        { wxID_ZOOM_IN, BITMAPS::zoom_in },
        { wxID_ZOOM_OUT, BITMAPS::zoom_out },
        { wxID_ZOOM_FIT, BITMAPS::zoom_fit_in_page },
        { ID_MENU_SHOW_GRID, BITMAPS::grid },
        { ID_MENU_SHOW_LEGEND, BITMAPS::text },
        { ID_MENU_DOTTED, BITMAPS::add_dashed_line },
        { ID_MENU_WHITE_BG, BITMAPS::swap_layer },

        { 0, BITMAPS::INVALID_BITMAP }  // Sentinel
    };

    // wxMenuItems are already created and attached to the m_mainMenu wxMenuBar.
    // A problem is the fact setting bitmaps in wxMenuItems after they are attached
    // to a wxMenu do not work in all cases.
    // So the trick is:
    // Remove the wxMenuItem from its wxMenu
    // Set the bitmap
    // Insert the modified wxMenuItem to its previous place
    for( int ii = 0; bm_list[ii].m_MenuId; ++ii )
    {
        wxMenuItem* item = m_mainMenu->FindItem( bm_list[ii].m_MenuId );

        if( !item || ( bm_list[ii].m_Bitmap == BITMAPS::INVALID_BITMAP ) )
            continue;

        wxMenu* menu = item->GetMenu();

        // Calculate the initial index of item inside the wxMenu parent.
        wxMenuItemList& mlist = menu->GetMenuItems();
        int mpos = mlist.IndexOf( item );

        if( mpos >= 0 ) // Should be always the case
        {
            // Modify the bitmap
            menu->Remove( item );
            AddBitmapToMenuItem( item, KiBitmap( bm_list[ii].m_Bitmap ) );

            // Insert item to its the initial index
            menu->Insert( mpos, item );
        }
    }
}


void SIM_PLOT_FRAME::setSubWindowsSashSize()
{
    if( m_splitterLeftRightSashPosition > 0 )
        m_splitterLeftRight->SetSashPosition( m_splitterLeftRightSashPosition );

    if( m_splitterPlotAndConsoleSashPosition > 0 )
        m_splitterPlotAndConsole->SetSashPosition( m_splitterPlotAndConsoleSashPosition );

    if( m_splitterSignalsSashPosition > 0 )
        m_splitterSignals->SetSashPosition( m_splitterSignalsSashPosition );

    if( m_splitterTuneValuesSashPosition > 0 )
        m_splitterTuneValues->SetSashPosition( m_splitterTuneValuesSashPosition );
}


void SIM_PLOT_FRAME::StartSimulation( const wxString& aSimCommand )
{
    STRING_FORMATTER formatter;

    if( !m_settingsDlg )
        m_settingsDlg = new DIALOG_SIM_SETTINGS( this, m_simulator->Settings() );

    m_simConsole->Clear();
    updateNetlistExporter();

    if( aSimCommand.IsEmpty() )
    {
        SIM_PANEL_BASE* plotPanel = currentPlotWindow();

        if( plotPanel && m_workbook->HasPlotPanel( plotPanel ) )
            m_exporter->SetSimCommand( m_workbook->GetSimCommand( plotPanel ) );
    }
    else
    {
        m_exporter->SetSimCommand( aSimCommand );
    }

    if( !m_exporter->Format( &formatter, m_settingsDlg->GetNetlistOptions() ) )
    {
        DisplayErrorMessage( this, _( "There were errors during netlist export, aborted." ) );
        return;
    }

    if( m_exporter->GetSimType() == ST_UNKNOWN )
    {
        DisplayInfoMessage( this, _( "You need to select the simulation settings first." ) );
        return;
    }

    m_simulator->LoadNetlist( formatter.GetString() );
    updateTuners();
    applyTuners();
    m_simulator->Run();
}


void SIM_PLOT_FRAME::StopSimulation()
{
    m_simulator->Stop();
}


bool SIM_PLOT_FRAME::IsSimulationRunning()
{
    return m_simulator ? m_simulator->IsRunning() : false;
}


SIM_PANEL_BASE* SIM_PLOT_FRAME::NewPlotPanel( wxString aSimCommand )
{
    SIM_PANEL_BASE* plotPanel = nullptr;
    SIM_TYPE        simType   = NETLIST_EXPORTER_PSPICE_SIM::CommandToSimType( aSimCommand );

    if( SIM_PANEL_BASE::IsPlottable( simType ) )
    {
        SIM_PLOT_PANEL* panel;
        panel = new SIM_PLOT_PANEL( aSimCommand, m_plotNotebook, this, wxID_ANY );

        panel->GetPlotWin()->EnableMouseWheelPan(
                Pgm().GetCommonSettings()->m_Input.scroll_modifier_zoom != 0 );

        plotPanel = dynamic_cast<SIM_PANEL_BASE*>( panel );
    }
    else
    {
        SIM_NOPLOT_PANEL* panel;
        panel = new SIM_NOPLOT_PANEL( aSimCommand, m_plotNotebook, wxID_ANY );
        plotPanel = dynamic_cast<SIM_PANEL_BASE*>( panel );
    }

    wxString pageTitle( m_simulator->TypeToName( simType, true ) );
    pageTitle.Prepend( wxString::Format( _( "Plot%u - " ), (unsigned int) ++m_plotNumber ) );

    m_workbook->AddPlotPanel( plotPanel );

    m_plotNotebook->AddPage( dynamic_cast<wxWindow*>( plotPanel ), pageTitle, true );

    updateFrame();
    return plotPanel;
}


void SIM_PLOT_FRAME::AddVoltagePlot( const wxString& aNetName )
{
    addPlot( aNetName, SPT_VOLTAGE, "V" );
}


void SIM_PLOT_FRAME::AddCurrentPlot( const wxString& aDeviceName, const wxString& aParam )
{
    addPlot( aDeviceName, SPT_CURRENT, aParam );
}


void SIM_PLOT_FRAME::AddTuner( SCH_SYMBOL* aSymbol )
{
    SIM_PANEL_BASE* plotPanel = currentPlotWindow();

    if( !plotPanel )
        return;

    // For now limit the tuner tool to RLC components
    char primitiveType = NETLIST_EXPORTER_PSPICE::GetSpiceField( SF_PRIMITIVE, aSymbol, 0 )[0];

    if( primitiveType != SP_RESISTOR && primitiveType != SP_CAPACITOR
      && primitiveType != SP_INDUCTOR )
        return;

    const wxString componentName = aSymbol->GetField( REFERENCE_FIELD )->GetText();

    // Do not add multiple instances for the same component
    auto tunerIt = std::find_if( m_tuners.begin(), m_tuners.end(), [&]( const TUNER_SLIDER* t )
        {
            return t->GetComponentName() == componentName;
        }
    );

    if( tunerIt != m_tuners.end() )
        return;     // We already have it

    try
    {
        TUNER_SLIDER* tuner = new TUNER_SLIDER( this, m_tunePanel, aSymbol );
        m_tuneSizer->Add( tuner );
        m_tuners.push_back( tuner );
        m_tunePanel->Layout();
    }
    catch( const KI_PARAM_ERROR& e )
    {
        // Sorry, no bonus
        DisplayErrorMessage( nullptr, e.What() );
    }
}


void SIM_PLOT_FRAME::RemoveTuner( TUNER_SLIDER* aTuner, bool aErase )
{
    if( aErase )
        m_tuners.remove( aTuner );

    aTuner->Destroy();
    m_tunePanel->Layout();
}


SIM_PLOT_PANEL* SIM_PLOT_FRAME::CurrentPlot() const
{
    SIM_PANEL_BASE* curPage = currentPlotWindow();

    return ( ( !curPage || curPage->GetType() == ST_UNKNOWN ) ?
                     nullptr :
                     dynamic_cast<SIM_PLOT_PANEL*>( curPage ) );
}


const NETLIST_EXPORTER_PSPICE_SIM* SIM_PLOT_FRAME::GetExporter() const
{
    return m_exporter.get();
}


std::shared_ptr<SPICE_SIMULATOR_SETTINGS>& SIM_PLOT_FRAME::GetSimulatorSettings()
{
    wxASSERT( m_simulator->Settings() );

    return m_simulator->Settings();
}


void SIM_PLOT_FRAME::addPlot( const wxString& aName, SIM_PLOT_TYPE aType, const wxString& aParam )
{
    SIM_TYPE simType = m_exporter->GetSimType();

    if( simType == ST_UNKNOWN )
    {
        m_simConsole->AppendText( _( "Error: simulation type not defined!\n" ) );
        m_simConsole->SetInsertionPointEnd();
        return;
    }
    else if( !SIM_PANEL_BASE::IsPlottable( simType ) )
    {
        m_simConsole->AppendText( _( "Error: simulation type doesn't support plotting!\n" ) );
        m_simConsole->SetInsertionPointEnd();
        return;
    }

    // Create a new plot if the current one displays a different type
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( !plotPanel || plotPanel->GetType() != simType )
    {
        plotPanel =
                dynamic_cast<SIM_PLOT_PANEL*>( NewPlotPanel( m_exporter->GetUsedSimCommand() ) );
    }

    wxASSERT( plotPanel );

    if( !plotPanel )    // Something is wrong
        return;

    bool updated = false;
    SIM_PLOT_TYPE xAxisType = GetXAxisType( simType );

    if( xAxisType == SPT_LIN_FREQUENCY || xAxisType == SPT_LOG_FREQUENCY )
    {
        int baseType = aType & ~( SPT_AC_MAG | SPT_AC_PHASE );

        // Add two plots: magnitude & phase
        updated |= updatePlot( aName, ( SIM_PLOT_TYPE )( baseType | SPT_AC_MAG ), aParam,
                               plotPanel );
        updated |= updatePlot( aName, ( SIM_PLOT_TYPE )( baseType | SPT_AC_PHASE ), aParam,
                               plotPanel );
    }
    else
    {
        updated = updatePlot( aName, aType, aParam, plotPanel );
    }

    if( updated )
    {
        updateSignalList();
    }
}


void SIM_PLOT_FRAME::removePlot( const wxString& aPlotName, bool aErase )
{
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( !plotPanel )
        return;

    if( aErase )
        m_workbook->RemoveTrace( plotPanel, aPlotName );

    wxASSERT( plotPanel->TraceShown( aPlotName ) );
    plotPanel->DeleteTrace( aPlotName );
    plotPanel->GetPlotWin()->Fit();

    updateSignalList();
    wxCommandEvent dummy;
    onCursorUpdate( dummy );
    updateFrame();
}


void SIM_PLOT_FRAME::updateNetlistExporter()
{
    m_exporter.reset( new NETLIST_EXPORTER_PSPICE_SIM( &m_schematicFrame->Schematic() ) );

    if( m_settingsDlg )
        m_settingsDlg->SetNetlistExporter( m_exporter.get() );
}


bool SIM_PLOT_FRAME::updatePlot( const wxString& aName, SIM_PLOT_TYPE aType, const wxString& aParam,
                                 SIM_PLOT_PANEL* aPanel )
{
    SIM_TYPE simType = m_exporter->GetSimType();
    wxString spiceVector = m_exporter->ComponentToVector( aName, aType, aParam );

    if( !SIM_PANEL_BASE::IsPlottable( simType ) )
    {
        // There is no plot to be shown
        m_simulator->Command( wxString::Format( "print %s", spiceVector ).ToStdString() );

        return false;
    }

    // First, handle the x axis
    wxString xAxisName( m_simulator->GetXAxis( simType ) );

    if( xAxisName.IsEmpty() )
        return false;

    auto data_x = m_simulator->GetMagPlot( (const char*) xAxisName.c_str() );
    unsigned int size = data_x.size();

    if( data_x.empty() )
        return false;

    std::vector<double> data_y;

    // Now, Y axis data
    switch( m_exporter->GetSimType() )
    {
    case ST_AC:
        wxASSERT_MSG( !( ( aType & SPT_AC_MAG ) && ( aType & SPT_AC_PHASE ) ),
                      "Cannot set both AC_PHASE and AC_MAG bits" );

        if( aType & SPT_AC_MAG )
            data_y = m_simulator->GetMagPlot( (const char*) spiceVector.c_str() );
        else if( aType & SPT_AC_PHASE )
            data_y = m_simulator->GetPhasePlot( (const char*) spiceVector.c_str() );
        else
            wxASSERT_MSG( false, "Plot type missing AC_PHASE or AC_MAG bit" );

        break;

    case ST_NOISE:
    case ST_DC:
    case ST_TRANSIENT:
        data_y = m_simulator->GetMagPlot( (const char*) spiceVector.c_str() );
        break;

    default:
        wxASSERT_MSG( false, "Unhandled plot type" );
        return false;
    }

    if( data_y.size() != size )
        return false;

    // If we did a two-source DC analysis, we need to split the resulting vector and add traces
    // for each input step
    SPICE_DC_PARAMS source1, source2;

    if( m_exporter->GetSimType() == ST_DC &&
        m_exporter->ParseDCCommand( m_exporter->GetUsedSimCommand(), &source1, &source2 ) )
    {
        if( !source2.m_source.IsEmpty() )
        {
            // Source 1 is the inner loop, so lets add traces for each Source 2 (outer loop) step
            SPICE_VALUE v = source2.m_vstart;
            wxString name;

            size_t offset = 0;
            size_t outer = ( size_t )( ( source2.m_vend - v ) / source2.m_vincrement ).ToDouble();
            size_t inner = data_x.size() / ( outer + 1 );

            wxASSERT( data_x.size() % ( outer + 1 ) == 0 );

            for( size_t idx = 0; idx <= outer; idx++ )
            {
                name = wxString::Format( "%s (%s = %s V)", aName, source2.m_source, v.ToString() );

                std::vector<double> sub_x( data_x.begin() + offset,
                                           data_x.begin() + offset + inner );
                std::vector<double> sub_y( data_y.begin() + offset,
                                           data_y.begin() + offset + inner );

                if( aPanel->AddTrace( name, inner, sub_x.data(), sub_y.data(), aType, aParam ) )
                    m_workbook->AddTrace( aPanel, name );

                v = v + source2.m_vincrement;
                offset += inner;
            }

            updateFrame();
            return true;
        }
    }

    if( aPanel->AddTrace( aName, size, data_x.data(), data_y.data(), aType, aParam ) )
        m_workbook->AddTrace( aPanel, aName );

    updateFrame();
    return true;
}


void SIM_PLOT_FRAME::updateSignalList()
{
    m_signals->ClearAll();

    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( !plotPanel )
        return;

    wxSize size = m_signals->GetClientSize();
    m_signals->AppendColumn( _( "Signal" ), wxLIST_FORMAT_LEFT, size.x );

    // Build an image list, to show the color of the corresponding trace
    // in the plot panel
    // This image list is used for trace and cursor lists
    wxMemoryDC bmDC;
    const int isize = bmDC.GetCharHeight();

    if( m_signalsIconColorList == NULL )
        m_signalsIconColorList = new wxImageList( isize, isize, false );
    else
        m_signalsIconColorList->RemoveAll();

    for( const auto& trace : CurrentPlot()->GetTraces() )
    {
        wxBitmap bitmap( isize, isize );
        bmDC.SelectObject( bitmap );
        wxColour tcolor = trace.second->GetPen().GetColour();

        wxColour bgColor = m_signals->wxWindow::GetBackgroundColour();
        bmDC.SetPen( wxPen( bgColor ) );
        bmDC.SetBrush( wxBrush( bgColor ) );
        bmDC.DrawRectangle( 0, 0, isize, isize ); // because bmDC.Clear() does not work in wxGTK

        bmDC.SetPen( wxPen( tcolor ) );
        bmDC.SetBrush( wxBrush( tcolor ) );
        bmDC.DrawRectangle( 0, isize / 4 + 1, isize, isize / 2 );

        bmDC.SelectObject( wxNullBitmap );  // Needed to initialize bitmap

        bitmap.SetMask( new wxMask( bitmap, *wxBLACK ) );
        m_signalsIconColorList->Add( bitmap );
    }

    if( bmDC.IsOk() )
    {
        bmDC.SetBrush( wxNullBrush );
        bmDC.SetPen( wxNullPen );
    }

    m_signals->SetImageList( m_signalsIconColorList, wxIMAGE_LIST_SMALL );

    // Fill the signals listctrl. Keep the order of names and
    // the order of icon color identical, because the icons
    // are also used in cursor list, and the color index is
    // calculated from the trace name index
    int imgidx = 0;

    for( const auto& trace : plotPanel->GetTraces() )
    {
        m_signals->InsertItem( imgidx, trace.first, imgidx );
        imgidx++;
    }
}


void SIM_PLOT_FRAME::updateTuners()
{
    const auto& spiceItems = m_exporter->GetSpiceItems();

    for( auto it = m_tuners.begin(); it != m_tuners.end(); /* iteration inside the loop */ )
    {
        const wxString& ref = (*it)->GetComponentName();

        if( std::find_if( spiceItems.begin(), spiceItems.end(), [&]( const SPICE_ITEM& item )
                {
                    return item.m_refName == ref;
                }) == spiceItems.end() )
        {
            // The component does not exist anymore, remove the associated tuner
            TUNER_SLIDER* tuner = *it;
            it = m_tuners.erase( it );
            RemoveTuner( tuner, false );
        }
        else
        {
            ++it;
        }
    }
}


void SIM_PLOT_FRAME::applyTuners()
{
    for( auto& tuner : m_tuners )
    {
        /// @todo no ngspice hardcoding
        std::string command( "alter @" + tuner->GetSpiceName()
                + "=" + tuner->GetValue().ToSpiceString() );

        m_simulator->Command( command );
    }
}


bool SIM_PLOT_FRAME::loadWorkbook( const wxString& aPath )
{
    m_workbook->Clear();
    m_plotNotebook->DeleteAllPages();

    wxTextFile file( aPath );

    if( !file.Open() )
    {
        updateFrame();
        return false;
    }

    long plotsCount;

    if( !file.GetFirstLine().ToLong( &plotsCount ) )        // GetFirstLine instead of GetNextLine
    {
        file.Close();
        updateFrame();
        return false;
    }

    for( long i = 0; i < plotsCount; ++i )
    {
        long plotType, tracesCount;

        if( !file.GetNextLine().ToLong( &plotType ) )
        {
            updateFrame();
            return false;
        }

        wxString        simCommand = file.GetNextLine();
        NewPlotPanel( simCommand );
        StartSimulation( simCommand );

        // Perform simulation, so plots can be added with values
        do
        {
            wxThread::This()->Sleep( 50 );
        }
        while( IsSimulationRunning() );

        if( !file.GetNextLine().ToLong( &tracesCount ) )
        {
            updateFrame();
            return false;
        }

        for( long j = 0; j < tracesCount; ++j )
        {
            long traceType;
            wxString name, param;

            if( !file.GetNextLine().ToLong( &traceType ) )
            {
                file.Close();
                updateFrame();
                return false;
            }

            name = file.GetNextLine();
            param = file.GetNextLine();

            if( name.IsEmpty() || param.IsEmpty() )
            {
                file.Close();
                updateFrame();
                return false;
            }

            addPlot( name, (SIM_PLOT_TYPE) traceType, param );
        }
    }

    file.Close();

    // Successfully loading a workbook does not count as modyfying it.
    m_workbook->ClrModified();

    updateFrame();
    return true;
}


bool SIM_PLOT_FRAME::saveWorkbook( const wxString& aPath )
{
    wxString savePath = aPath;

    if( !savePath.Lower().EndsWith(".wbk") )
        savePath += ".wbk";

    wxTextFile file( savePath );

    if( file.Exists() )
    {
        if( !file.Open() )
            return false;

        file.Clear();
    }
    else
    {
        file.Create();
    }

    std::vector<const SIM_PANEL_BASE*> plotPanels = m_workbook->GetSortedPlotPanels();

    file.AddLine( wxString::Format( "%llu", plotPanels.size() ) );

    for( const SIM_PANEL_BASE*& plotPanel : plotPanels )
    {
        file.AddLine( wxString::Format( "%d", plotPanel->GetType() ) );
        file.AddLine( m_workbook->GetSimCommand( plotPanel ) );

        const SIM_PLOT_PANEL* panel = dynamic_cast<const SIM_PLOT_PANEL*>( plotPanel );

        if( !panel )
        {
            file.AddLine( wxString::Format( "%llu", 0ull ) );
        }
        else
        {
            file.AddLine( wxString::Format( "%llu", panel->GetTraces().size() ) );

            for( const auto& trace : panel->GetTraces() )
            {
                file.AddLine( wxString::Format( "%d", trace.second->GetType() ) );
                file.AddLine( trace.second->GetName() );
                file.AddLine( trace.second->GetParam() );
            }
        }
    }

    bool res = file.Write();
    file.Close();

    // Store the filename of the last saved workbook. It will be used to restore the simulation if
    // the frame is closed and then opened again.
    m_simulator->Settings()->SetWorkbookFilename( wxFileName( savePath ).GetFullName() );
    m_workbook->ClrModified();
    updateFrame();

    return res;
}


SIM_PLOT_TYPE SIM_PLOT_FRAME::GetXAxisType( SIM_TYPE aType ) const
{
    switch( aType )
    {
        /// @todo SPT_LOG_FREQUENCY
        case ST_AC:        return SPT_LIN_FREQUENCY;
        case ST_DC:        return SPT_SWEEP;
        case ST_TRANSIENT: return SPT_TIME;
        default:
            wxASSERT_MSG( false, "Unhandled simulation type" );
            return (SIM_PLOT_TYPE) 0;
    }
}


void SIM_PLOT_FRAME::menuNewPlot( wxCommandEvent& aEvent )
{
    SIM_TYPE type = m_exporter->GetSimType();

    if( SIM_PANEL_BASE::IsPlottable( type ) )
    {
        NewPlotPanel( m_exporter->GetUsedSimCommand() );
        updateFrame();
    }
}


void SIM_PLOT_FRAME::menuOpenWorkbook( wxCommandEvent& event )
{
    wxFileDialog openDlg( this, _( "Open simulation workbook" ), m_savedWorkbooksPath, "",
                          WorkbookFileWildcard(), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( openDlg.ShowModal() == wxID_CANCEL )
        return;

    m_savedWorkbooksPath = openDlg.GetDirectory();

    if( !loadWorkbook( openDlg.GetPath() ) )
        DisplayErrorMessage( this, _( "There was an error while opening the workbook file" ) );
}


void SIM_PLOT_FRAME::menuSaveWorkbook( wxCommandEvent& event )
{
    if( !m_workbook->IsModified() )
        return;

    wxString filename = m_simulator->Settings()->GetWorkbookFilename();

    if( filename.IsEmpty() )
    {
        menuSaveWorkbookAs( event );
        return;
    }

    if ( !saveWorkbook( Prj().AbsolutePath( m_simulator->Settings()->GetWorkbookFilename() ) ) )
        DisplayErrorMessage( this, _( "There was an error while saving the workbook file" ) );
}


void SIM_PLOT_FRAME::menuSaveWorkbookAs( wxCommandEvent& event )
{
    wxString defaultFilename = m_simulator->Settings()->GetWorkbookFilename();

    if( defaultFilename.IsEmpty() )
        defaultFilename = Prj().GetProjectName() + wxT( ".wbk" );

    wxFileDialog saveAsDlg( this, _( "Save Simulation Workbook As" ), m_savedWorkbooksPath,
                            defaultFilename, WorkbookFileWildcard(),
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveAsDlg.ShowModal() == wxID_CANCEL )
        return;

    m_savedWorkbooksPath = saveAsDlg.GetDirectory();

    if( !saveWorkbook( saveAsDlg.GetPath() ) )
        DisplayErrorMessage( this, _( "There was an error while saving the workbook file" ) );
}


void SIM_PLOT_FRAME::menuSaveImage( wxCommandEvent& event )
{
    if( !CurrentPlot() )
        return;

    wxFileDialog saveDlg( this, _( "Save Plot as Image" ), "", "", PngFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveDlg.ShowModal() == wxID_CANCEL )
        return;

    CurrentPlot()->GetPlotWin()->SaveScreenshot( saveDlg.GetPath(), wxBITMAP_TYPE_PNG );
}


void SIM_PLOT_FRAME::menuSaveCsv( wxCommandEvent& event )
{
    if( !CurrentPlot() )
        return;

    const wxChar SEPARATOR = ';';

    wxFileDialog saveDlg( this, _( "Save Plot Data" ), "", "", CsvFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveDlg.ShowModal() == wxID_CANCEL )
        return;

    wxFFile out( saveDlg.GetPath(), "wb" );
    bool timeWritten = false;

    for( const auto& t : CurrentPlot()->GetTraces() )
    {
        const TRACE* trace = t.second;

        if( !timeWritten )
        {
            out.Write( wxString::Format( "Time%c", SEPARATOR ) );

            for( double v : trace->GetDataX() )
                out.Write( wxString::Format( "%g%c", v, SEPARATOR ) );

            out.Write( "\r\n" );
            timeWritten = true;
        }

        out.Write( wxString::Format( "%s%c", t.first, SEPARATOR ) );

        for( double v : trace->GetDataY() )
            out.Write( wxString::Format( "%g%c", v, SEPARATOR ) );

        out.Write( "\r\n" );
    }

    out.Close();
}


void SIM_PLOT_FRAME::menuZoomIn( wxCommandEvent& event )
{
    if( CurrentPlot() )
        CurrentPlot()->GetPlotWin()->ZoomIn();
}


void SIM_PLOT_FRAME::menuZoomOut( wxCommandEvent& event )
{
    if( CurrentPlot() )
        CurrentPlot()->GetPlotWin()->ZoomOut();
}


void SIM_PLOT_FRAME::menuZoomFit( wxCommandEvent& event )
{
    if( CurrentPlot() )
        CurrentPlot()->GetPlotWin()->Fit();
}


void SIM_PLOT_FRAME::menuShowGrid( wxCommandEvent& event )
{
    SIM_PLOT_PANEL* plot = CurrentPlot();

    if( plot )
        plot->ShowGrid( !plot->IsGridShown() );
}


void SIM_PLOT_FRAME::menuShowGridUpdate( wxUpdateUIEvent& event )
{
    SIM_PLOT_PANEL* plot = CurrentPlot();

    event.Check( plot ? plot->IsGridShown() : false );
}


void SIM_PLOT_FRAME::menuShowLegend( wxCommandEvent& event )
{
    SIM_PLOT_PANEL* plot = CurrentPlot();

    if( plot )
        plot->ShowLegend( !plot->IsLegendShown() );
}


void SIM_PLOT_FRAME::menuShowLegendUpdate( wxUpdateUIEvent& event )
{
    SIM_PLOT_PANEL* plot = CurrentPlot();
    event.Check( plot ? plot->IsLegendShown() : false );
}


void SIM_PLOT_FRAME::menuShowDotted( wxCommandEvent& event )
{
    SIM_PLOT_PANEL* plot = CurrentPlot();

    if( plot )
        plot->SetDottedCurrentPhase( !plot->GetDottedCurrentPhase() );
}


void SIM_PLOT_FRAME::menuShowDottedUpdate( wxUpdateUIEvent& event )
{
    SIM_PLOT_PANEL* plot = CurrentPlot();

    event.Check( plot ? plot->GetDottedCurrentPhase() : false );
}


void SIM_PLOT_FRAME::menuWhiteBackground( wxCommandEvent& event )
{
    m_plotUseWhiteBg = not m_plotUseWhiteBg;

    // Rebuild the color list to plot traces
    SIM_PLOT_COLORS::FillDefaultColorList( GetPlotBgOpt() );

    // Now send changes to all SIM_PLOT_PANEL
    for( size_t page = 0; page < m_plotNotebook->GetPageCount(); page++ )
    {
        wxWindow* curPage = m_plotNotebook->GetPage( page );

        // ensure it is truly a plot panel and not the (zero plots) placeholder
        // which is only SIM_PLOT_PANEL_BASE
        SIM_PLOT_PANEL* panel = dynamic_cast<SIM_PLOT_PANEL*>( curPage );

        if( panel )
            panel->UpdatePlotColors();
    }
}


void SIM_PLOT_FRAME::onPlotClose( wxAuiNotebookEvent& event )
{
    int idx = event.GetSelection();

    if( idx == wxNOT_FOUND )
        return;

    SIM_PANEL_BASE* plotPanel = dynamic_cast<SIM_PANEL_BASE*>( m_plotNotebook->GetPage( idx ) );

    m_workbook->RemovePlotPanel( plotPanel );
    wxCommandEvent dummy;
    onCursorUpdate( dummy );
}


void SIM_PLOT_FRAME::onPlotClosed( wxAuiNotebookEvent& event )
{
    updateWorkbook();
    updateFrame();
}


void SIM_PLOT_FRAME::onPlotChanged( wxAuiNotebookEvent& event )
{
    updateSignalList();
    wxCommandEvent dummy;
    onCursorUpdate( dummy );

    updateWorkbook();
    updateFrame();
}


void SIM_PLOT_FRAME::onPlotDragged( wxAuiNotebookEvent& event )
{
    updateWorkbook();
    updateFrame();
}


void SIM_PLOT_FRAME::onSignalDblClick( wxMouseEvent& event )
{
    // Remove signal from the plot panel when double clicked
    long idx = m_signals->GetFocusedItem();

    if( idx != wxNOT_FOUND )
        removePlot( m_signals->GetItemText( idx, 0 ) );
}


void SIM_PLOT_FRAME::onSignalRClick( wxListEvent& event )
{
    int idx = event.GetIndex();

    if( idx != wxNOT_FOUND )
        m_signals->Select( idx );

    idx = m_signals->GetFirstSelected();

    if( idx != wxNOT_FOUND )
    {
        const wxString& netName = m_signals->GetItemText( idx, 0 );
        SIGNAL_CONTEXT_MENU ctxMenu( netName, this );
        m_signals->PopupMenu( &ctxMenu );
    }
}


void SIM_PLOT_FRAME::onSimulate( wxCommandEvent& event )
{
    if( IsSimulationRunning() )
        StopSimulation();
    else
        StartSimulation();
}


void SIM_PLOT_FRAME::onSettings( wxCommandEvent& event )
{
    SIM_PANEL_BASE* plotPanelWindow = currentPlotWindow();

    if( !m_settingsDlg )
        m_settingsDlg = new DIALOG_SIM_SETTINGS( this, m_simulator->Settings() );

    // Initial processing is required to e.g. display a list of power sources
    updateNetlistExporter();

    if( !m_exporter->ProcessNetlist( NET_ALL_FLAGS ) )
    {
        DisplayErrorMessage( this, _( "There were errors during netlist export, aborted." ) );
        return;
    }

    if( m_workbook->HasPlotPanel( plotPanelWindow ) )
        m_settingsDlg->SetSimCommand( m_workbook->GetSimCommand( plotPanelWindow ) );

    if( m_settingsDlg->ShowModal() == wxID_OK )
    {
        wxString oldCommand;

        if( m_workbook->HasPlotPanel( plotPanelWindow ) )
            oldCommand = m_workbook->GetSimCommand( plotPanelWindow );
        else
            oldCommand = wxString();

        wxString newCommand = m_settingsDlg->GetSimCommand();
        SIM_TYPE newSimType = NETLIST_EXPORTER_PSPICE_SIM::CommandToSimType( newCommand );

        // If it is a new simulation type, open a new plot
        // For the DC sim, check if sweep source type has changed (char 4 will contain 'v',
        // 'i', 'r' or 't'.
        if( !plotPanelWindow
            || ( plotPanelWindow && plotPanelWindow->GetType() != newSimType )
            || ( newSimType == ST_DC
                 && oldCommand.Lower().GetChar( 4 ) != newCommand.Lower().GetChar( 4 ) ) )
        {
            plotPanelWindow = NewPlotPanel( newCommand );
        }
        else
        {
            // Update simulation command in the current plot
            m_workbook->SetSimCommand( plotPanelWindow, newCommand );
        }

        m_simulator->Init();
        updateFrame();
    }
}


void SIM_PLOT_FRAME::onAddSignal( wxCommandEvent& event )
{
    if( IsSimulationRunning() )
    {
        DisplayInfoMessage( this, _( "Simulator is running. Try later" ) );
        return;
    }

    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( !plotPanel || !m_exporter || plotPanel->GetType() != m_exporter->GetSimType() )
    {
        DisplayInfoMessage( this, _( "You need to run plot-providing simulation first." ) );
        return;
    }

    DIALOG_SIGNAL_LIST dialog( this, m_exporter.get() );
    dialog.ShowModal();
}


void SIM_PLOT_FRAME::onProbe( wxCommandEvent& event )
{
    if( m_schematicFrame == NULL )
        return;

    if( IsSimulationRunning() )
    {
        DisplayInfoMessage( this, _( "Simulator is running. Try later" ) );
        return;
    }

    m_schematicFrame->GetToolManager()->RunAction( EE_ACTIONS::simProbe );
    m_schematicFrame->Raise();
}


void SIM_PLOT_FRAME::onTune( wxCommandEvent& event )
{
    if( m_schematicFrame == NULL )
        return;

    m_schematicFrame->GetToolManager()->RunAction( EE_ACTIONS::simTune );
    m_schematicFrame->Raise();
}


void SIM_PLOT_FRAME::onShowNetlist( wxCommandEvent& event )
{
    class NETLIST_VIEW_DIALOG : public DIALOG_SHIM
    {
    public:
        enum
        {
            MARGIN_LINE_NUMBERS
        };

        void onClose( wxCloseEvent& evt )
        {
            EndModal( GetReturnCode() );
        }

        NETLIST_VIEW_DIALOG( wxWindow* parent, wxString source) :
            DIALOG_SHIM( parent, wxID_ANY, "SPICE Netlist",
                         wxDefaultPosition, wxDefaultSize,
                         wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
        {
            wxStyledTextCtrl* text = new wxStyledTextCtrl( this, wxID_ANY );
            text->SetMinSize( wxSize( 600, 400 ) );

            text->SetMarginWidth( MARGIN_LINE_NUMBERS, 50 );
            text->StyleSetForeground( wxSTC_STYLE_LINENUMBER, wxColour( 75, 75, 75 ) );
            text->StyleSetBackground( wxSTC_STYLE_LINENUMBER, wxColour( 220, 220, 220 ) );
            text->SetMarginType( MARGIN_LINE_NUMBERS, wxSTC_MARGIN_NUMBER );

            wxFont fixedFont = KIUI::GetMonospacedUIFont();

            for( size_t i = 0; i < wxSTC_STYLE_MAX; ++i )
                text->StyleSetFont( i, fixedFont );

            text->StyleClearAll();  // Addresses a bug in wx3.0 where styles are not correctly set

            text->SetWrapMode( wxSTC_WRAP_WORD );

            text->SetText( source );

            text->SetLexer( wxSTC_LEX_SPICE );

            wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
            sizer->Add( text, 1, wxEXPAND );
            SetSizer( sizer );

            Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( NETLIST_VIEW_DIALOG::onClose ), NULL,
                     this );

            finishDialogSettings();
        }
    };

    if( m_schematicFrame == NULL || m_simulator == NULL )
        return;

    NETLIST_VIEW_DIALOG dlg( this, m_simulator->GetNetlist() );
    dlg.ShowModal();
}


bool SIM_PLOT_FRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    if( m_workbook->IsModified() )
    {
        wxFileName filename = m_simulator->Settings()->GetWorkbookFilename();

        if( filename.GetName().IsEmpty() )
            filename.SetFullName( Prj().GetProjectName() + wxT( ".wbk" ) );

        wxString msg = _( "Save changes to '%s' before closing?" );

        return HandleUnsavedChanges( this, wxString::Format( msg, filename.GetFullName() ), 
                [&]()->bool
                {
                    return saveWorkbook( Prj().AbsolutePath ( filename.GetFullName() ) );
                } );
    }

    return true;
}


void SIM_PLOT_FRAME::doCloseWindow()
{
    if( IsSimulationRunning() )
        m_simulator->Stop();

    // Cancel a running simProbe or simTune tool
    m_schematicFrame->GetToolManager()->RunAction( ACTIONS::cancelInteractive );

    SaveSettings( config() );
    Destroy();
}


void SIM_PLOT_FRAME::onCursorUpdate( wxCommandEvent& event )
{
    wxSize size = m_cursors->GetClientSize();
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();
    m_cursors->ClearAll();

    if( !plotPanel )
        return;

    if( m_signalsIconColorList )
        m_cursors->SetImageList(m_signalsIconColorList, wxIMAGE_LIST_SMALL);

    // Fill the signals listctrl
    m_cursors->AppendColumn( _( "Signal" ), wxLIST_FORMAT_LEFT, size.x / 2 );
    const long X_COL = m_cursors->AppendColumn( plotPanel->GetLabelX(), wxLIST_FORMAT_LEFT,
                                                size.x / 4 );

    wxString labelY1 = plotPanel->GetLabelY1();
    wxString labelY2 = plotPanel->GetLabelY2();
    wxString labelY;

    if( !labelY2.IsEmpty() )
        labelY = labelY1 + " / " + labelY2;
    else
        labelY = labelY1;

    const long Y_COL = m_cursors->AppendColumn( labelY, wxLIST_FORMAT_LEFT, size.x / 4 );

    // Update cursor values
    int itemidx = 0;

    for( const auto& trace : plotPanel->GetTraces() )
    {
        if( CURSOR* cursor = trace.second->GetCursor() )
        {
           // Find the right icon color in list.
            // It is the icon used in m_signals list for the same trace
            long iconColor = m_signals->FindItem( -1, trace.first );

            const wxRealPoint coords = cursor->GetCoords();
            long idx = m_cursors->InsertItem( itemidx++, trace.first, iconColor );
            m_cursors->SetItem( idx, X_COL, SPICE_VALUE( coords.x ).ToSpiceString() );
            m_cursors->SetItem( idx, Y_COL, SPICE_VALUE( coords.y ).ToSpiceString() );
        }
    }
}


void SIM_PLOT_FRAME::onSimStarted( wxCommandEvent& aEvent )
{
    m_toolBar->SetToolNormalBitmap( ID_SIM_RUN, KiBitmap( BITMAPS::sim_stop ) );
    SetCursor( wxCURSOR_ARROWWAIT );
}


void SIM_PLOT_FRAME::onSimFinished( wxCommandEvent& aEvent )
{
    m_toolBar->SetToolNormalBitmap( ID_SIM_RUN, KiBitmap( BITMAPS::sim_run ) );
    SetCursor( wxCURSOR_ARROW );

    SIM_TYPE simType = m_exporter->GetSimType();

    if( simType == ST_UNKNOWN )
        return;

    SIM_PANEL_BASE* plotPanelWindow = currentPlotWindow();

    if( !plotPanelWindow || plotPanelWindow->GetType() != simType )
        plotPanelWindow = NewPlotPanel( m_exporter->GetUsedSimCommand() );

    if( IsSimulationRunning() )
        return;

    // If there are any signals plotted, update them
    if( SIM_PANEL_BASE::IsPlottable( simType ) )
    {
        SIM_PLOT_PANEL* plotPanel = dynamic_cast<SIM_PLOT_PANEL*>( plotPanelWindow );
        wxCHECK_RET( plotPanel, "not a SIM_PLOT_PANEL"  );

        struct TRACE_DESC
        {
            wxString      m_name;    ///< Name of the measured net/device
            SIM_PLOT_TYPE m_type;    ///< Type of the signal
            wxString      m_param;   ///< Name of the signal parameter
        };

        std::vector<struct TRACE_DESC> traceInfo;

        // Get information about all the traces on the plot, remove and add again
        for( auto& trace : plotPanel->GetTraces() )
        {
            struct TRACE_DESC placeholder;
            placeholder.m_name = trace.second->GetName();
            placeholder.m_type = trace.second->GetType();
            placeholder.m_param = trace.second->GetParam();

            traceInfo.push_back( placeholder );
        }

        for( auto& trace : traceInfo )
        {
            if( !updatePlot( trace.m_name, trace.m_type, trace.m_param, plotPanel ) )
            {
                removePlot( trace.m_name, false );
                m_workbook->RemoveTrace( plotPanel, trace.m_name );
            }
        }

        updateSignalList();
        plotPanel->GetPlotWin()->UpdateAll();
        plotPanel->ResetScales();
        updateFrame();
    }
    else if( simType == ST_OP )
    {
        m_simConsole->AppendText( _( "\n\nSimulation results:\n\n" ) );
        m_simConsole->SetInsertionPointEnd();

        for( const auto& vec : m_simulator->AllPlots() )
        {
            std::vector<double> val_list = m_simulator->GetRealPlot( vec, 1 );

            if( val_list.size() == 0 )      // The list of values can be empty!
                continue;

            double val = val_list.at( 0 );
            wxString      outLine, signal;
            SIM_PLOT_TYPE type = m_exporter->VectorToSignal( vec, signal );

            const size_t tab     = 25; //characters
            size_t padding = ( signal.length() < tab ) ? ( tab - signal.length() ) : 1;

            outLine.Printf( wxT( "%s%s" ),
                            ( signal + wxT( ":" ) ).Pad( padding, wxUniChar( ' ' ) ),
                            SPICE_VALUE( val ).ToSpiceString() );

            outLine.Append( type == SPT_CURRENT ? "A\n" : "V\n" );

            m_simConsole->AppendText( outLine );
            m_simConsole->SetInsertionPointEnd();

            // @todo display calculated values on the schematic
        }
    }
}


void SIM_PLOT_FRAME::onSimUpdate( wxCommandEvent& aEvent )
{
    if( IsSimulationRunning() )
        StopSimulation();

    if( CurrentPlot() != m_lastSimPlot )
    {
        // We need to rerun simulation, as the simulator currently stores
        // results for another plot
        StartSimulation();
    }
    else
    {
        // Incremental update
        m_simConsole->Clear();
        // Do not export netlist, it is already stored in the simulator
        applyTuners();
        m_simulator->Run();
    }
}


void SIM_PLOT_FRAME::onSimReport( wxCommandEvent& aEvent )
{
    m_simConsole->AppendText( aEvent.GetString() + "\n" );
    m_simConsole->SetInsertionPointEnd();
}


SIM_PLOT_FRAME::SIGNAL_CONTEXT_MENU::SIGNAL_CONTEXT_MENU( const wxString& aSignal,
                                                          SIM_PLOT_FRAME* aPlotFrame ) :
        m_signal( aSignal ),
        m_plotFrame( aPlotFrame )
{
    SIM_PLOT_PANEL* plot = m_plotFrame->CurrentPlot();

    AddMenuItem( this, HIDE_SIGNAL, _( "Hide Signal" ), _( "Erase the signal from plot screen" ),
                 KiBitmap( BITMAPS::trash ) );

    TRACE* trace = plot->GetTrace( m_signal );

    if( trace->HasCursor() )
        AddMenuItem( this, HIDE_CURSOR, _( "Hide Cursor" ), KiBitmap( BITMAPS::pcb_target ) );
    else
        AddMenuItem( this, SHOW_CURSOR, _( "Show Cursor" ), KiBitmap( BITMAPS::pcb_target ) );

    Connect( wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler( SIGNAL_CONTEXT_MENU::onMenuEvent ),
             NULL, this );
}


void SIM_PLOT_FRAME::SIGNAL_CONTEXT_MENU::onMenuEvent( wxMenuEvent& aEvent )
{
    SIM_PLOT_PANEL* plot = m_plotFrame->CurrentPlot();

    switch( aEvent.GetId() )
    {
    case HIDE_SIGNAL:
        m_plotFrame->removePlot( m_signal );
        break;

    case SHOW_CURSOR:
        plot->EnableCursor( m_signal, true );
        break;

    case HIDE_CURSOR:
        plot->EnableCursor( m_signal, false );
        break;
    }
}


wxDEFINE_EVENT( EVT_SIM_UPDATE, wxCommandEvent );
wxDEFINE_EVENT( EVT_SIM_REPORT, wxCommandEvent );

wxDEFINE_EVENT( EVT_SIM_STARTED, wxCommandEvent );
wxDEFINE_EVENT( EVT_SIM_FINISHED, wxCommandEvent );
