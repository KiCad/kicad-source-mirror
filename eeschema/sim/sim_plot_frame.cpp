/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
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

#include <schframe.h>
#include <eeschema_id.h>
#include <kiway.h>
#include <confirm.h>
#include <bitmaps.h>

#include <widgets/tuner_slider.h>
#include <dialogs/dialog_signal_list.h>
#include "netlist_exporter_pspice_sim.h"
#include <pgm_base.h>

#include "sim_plot_frame.h"
#include "sim_plot_panel.h"
#include "spice_simulator.h"
#include "spice_reporter.h"

#include <menus_helpers.h>

SIM_PLOT_TYPE operator|( SIM_PLOT_TYPE aFirst, SIM_PLOT_TYPE aSecond )
{
    int res = (int) aFirst | (int) aSecond;

    return (SIM_PLOT_TYPE) res;
}


class SIM_THREAD_REPORTER : public SPICE_REPORTER
{
public:
    SIM_THREAD_REPORTER( SIM_PLOT_FRAME* aParent )
        : m_parent( aParent )
    {
    }

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_UNDEFINED ) override
    {
        wxCommandEvent* event = new wxCommandEvent( EVT_SIM_REPORT );
        event->SetString( aText );
        wxQueueEvent( m_parent, event );
        return *this;
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
        }

        wxQueueEvent( m_parent, event );
    }

private:
    SIM_PLOT_FRAME* m_parent;
};


TRACE_DESC::TRACE_DESC( const NETLIST_EXPORTER_PSPICE_SIM& aExporter, const wxString& aName,
        SIM_PLOT_TYPE aType, const wxString& aParam )
    : m_name( aName ), m_type( aType ), m_param( aParam )
{
    // Title generation
    m_title = wxString::Format( "%s(%s)", aParam, aName );

    if( aType & SPT_AC_MAG )
        m_title += " (mag)";
    else if( aType & SPT_AC_PHASE )
        m_title += " (phase)";
}

// Store the path of saved workbooks during the session
wxString SIM_PLOT_FRAME::m_savedWorkbooksPath;

SIM_PLOT_FRAME::SIM_PLOT_FRAME( KIWAY* aKiway, wxWindow* aParent )
    : SIM_PLOT_FRAME_BASE( aParent ), m_lastSimPlot( nullptr )
{
    SetKiway( this, aKiway );
    m_signalsIconColorList = NULL;

    m_schematicFrame = (SCH_EDIT_FRAME*) Kiway().Player( FRAME_SCH, false );

    if( m_schematicFrame == NULL )
        throw std::runtime_error( "There is no schematic window" );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( simulator_xpm ) );
    SetIcon( icon );

    m_simulator = SPICE_SIMULATOR::CreateInstance( "ngspice" );

    if( !m_simulator )
    {
        throw std::runtime_error( "Could not create simulator instance" );
        return;
    }

    m_simulator->Init();

    if( m_savedWorkbooksPath.IsEmpty() )
    {
        m_savedWorkbooksPath = Prj().GetProjectPath();
    }

    m_reporter = new SIM_THREAD_REPORTER( this );
    m_simulator->SetReporter( m_reporter );

    updateNetlistExporter();

    Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SIM_PLOT_FRAME::onClose ), NULL, this );
    Connect( EVT_SIM_UPDATE, wxCommandEventHandler( SIM_PLOT_FRAME::onSimUpdate ), NULL, this );
    Connect( EVT_SIM_REPORT, wxCommandEventHandler( SIM_PLOT_FRAME::onSimReport ), NULL, this );
    Connect( EVT_SIM_STARTED, wxCommandEventHandler( SIM_PLOT_FRAME::onSimStarted ), NULL, this );
    Connect( EVT_SIM_FINISHED, wxCommandEventHandler( SIM_PLOT_FRAME::onSimFinished ), NULL, this );
    Connect( EVT_SIM_CURSOR_UPDATE, wxCommandEventHandler( SIM_PLOT_FRAME::onCursorUpdate ), NULL, this );

    // Toolbar buttons
    m_toolSimulate = m_toolBar->AddTool( ID_SIM_RUN, _( "Run/Stop Simulation" ),
            KiBitmap( sim_run_xpm ), _( "Run Simulation" ), wxITEM_NORMAL );
    m_toolAddSignals = m_toolBar->AddTool( ID_SIM_ADD_SIGNALS, _( "Add Signals" ),
            KiBitmap( sim_add_signal_xpm ), _( "Add signals to plot" ), wxITEM_NORMAL );
    m_toolProbe = m_toolBar->AddTool( ID_SIM_PROBE,  _( "Probe" ),
            KiBitmap( sim_probe_xpm ), _( "Probe signals on the schematic" ), wxITEM_NORMAL );
    m_toolTune = m_toolBar->AddTool( ID_SIM_TUNE, _( "Tune" ),
            KiBitmap( sim_tune_xpm ), _( "Tune component values" ), wxITEM_NORMAL );
    m_toolSettings = m_toolBar->AddTool( wxID_ANY, _( "Settings" ),
            KiBitmap( sim_settings_xpm ), _( "Simulation settings" ), wxITEM_NORMAL );

    Connect( m_toolSimulate->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME::onSimulate ), NULL, this );
    Connect( m_toolAddSignals->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME::onAddSignal ), NULL, this );
    Connect( m_toolProbe->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME::onProbe ), NULL, this );
    Connect( m_toolTune->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME::onTune ), NULL, this );
    Connect( m_toolSettings->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME::onSettings ), NULL, this );

    // Bind toolbar buttons event to existing menu event handlers, so they behave the same
    Bind( wxEVT_COMMAND_MENU_SELECTED, &SIM_PLOT_FRAME::onSimulate,  this, m_runSimulation->GetId() );
    Bind( wxEVT_COMMAND_MENU_SELECTED, &SIM_PLOT_FRAME::onAddSignal, this, m_addSignals->GetId() );
    Bind( wxEVT_COMMAND_MENU_SELECTED, &SIM_PLOT_FRAME::onProbe,     this, m_probeSignals->GetId() );
    Bind( wxEVT_COMMAND_MENU_SELECTED, &SIM_PLOT_FRAME::onTune,      this, m_tuneValue->GetId() );
    Bind( wxEVT_COMMAND_MENU_SELECTED, &SIM_PLOT_FRAME::onSettings,  this, m_settings->GetId() );

    m_toolBar->Realize();
    m_plotNotebook->SetPageText( 0, _( "Welcome!" ) );

    // the settings dialog will be created later, on demand.
    // if created in the ctor, for some obscure reason, there is an issue
    // on Windows: when open it, the simulator frame is sent to the background.
    // instead of beeing behind the dialog frame (as it does)
    m_settingsDlg = NULL;
}


SIM_PLOT_FRAME::~SIM_PLOT_FRAME()
{
    m_simulator->SetReporter( nullptr );
    delete m_reporter;
    delete m_signalsIconColorList;

    if( m_settingsDlg )
        m_settingsDlg->Destroy();
}


void SIM_PLOT_FRAME::StartSimulation()
{
    STRING_FORMATTER formatter;
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( !m_settingsDlg )
        m_settingsDlg = new DIALOG_SIM_SETTINGS( this );

    m_simConsole->Clear();
    updateNetlistExporter();

    if( plotPanel )
        m_exporter->SetSimCommand( m_plots[plotPanel].m_simCommand );

    if( !m_exporter->Format( &formatter, m_settingsDlg->GetNetlistOptions() ) )
    {
        DisplayError( this, _( "There were errors during netlist export, aborted." ) );
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


SIM_PLOT_PANEL* SIM_PLOT_FRAME::NewPlotPanel( SIM_TYPE aSimType )
{
    SIM_PLOT_PANEL* plotPanel = new SIM_PLOT_PANEL( aSimType, m_plotNotebook, wxID_ANY );

    if( m_welcomePanel )
    {
        m_plotNotebook->DeletePage( 0 );
        m_welcomePanel = nullptr;
    }

    m_plotNotebook->AddPage( plotPanel, wxString::Format( wxT( "Plot%u" ),
            (unsigned int) m_plotNotebook->GetPageCount() + 1 ), true );

    m_plots[plotPanel] = PLOT_INFO();

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


void SIM_PLOT_FRAME::AddTuner( SCH_COMPONENT* aComponent )
{
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( !plotPanel )
        return;

    // For now limit the tuner tool to RLC components
    char primitiveType = NETLIST_EXPORTER_PSPICE::GetSpiceField( SF_PRIMITIVE, aComponent, 0 )[0];

    if( primitiveType != SP_RESISTOR && primitiveType != SP_CAPACITOR && primitiveType != SP_INDUCTOR )
        return;

    const wxString& componentName = aComponent->GetField( REFERENCE )->GetText();

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
        TUNER_SLIDER* tuner = new TUNER_SLIDER( this, m_sidePanel, aComponent );
        m_tuneSizer->Add( tuner );
        m_tuners.push_back( tuner );
        m_sidePanel->Layout();
    }
    catch( ... )
    {
        // Sorry, no bonus
    }
}


void SIM_PLOT_FRAME::RemoveTuner( TUNER_SLIDER* aTuner, bool aErase )
{
    if( aErase )
        m_tuners.remove( aTuner );

    aTuner->Destroy();
    m_sidePanel->Layout();
}


SIM_PLOT_PANEL* SIM_PLOT_FRAME::CurrentPlot() const
{
    wxWindow* curPage = m_plotNotebook->GetCurrentPage();

    return ( curPage == m_welcomePanel ) ? nullptr : static_cast<SIM_PLOT_PANEL*>( curPage );
}


void SIM_PLOT_FRAME::addPlot( const wxString& aName, SIM_PLOT_TYPE aType, const wxString& aParam )
{
    SIM_TYPE simType = m_exporter->GetSimType();

    if( !SIM_PLOT_PANEL::IsPlottable( simType ) )
        return; // TODO else write out in console?

    // Create a new plot if the current one displays a different type
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( !plotPanel || plotPanel->GetType() != simType )
        plotPanel = NewPlotPanel( simType );

    TRACE_DESC descriptor( *m_exporter, aName, aType, aParam );

    bool updated = false;
    SIM_PLOT_TYPE xAxisType = GetXAxisType( simType );

    if( xAxisType == SPT_LIN_FREQUENCY || xAxisType == SPT_LOG_FREQUENCY )
    {
        int baseType = descriptor.GetType() & ~( SPT_AC_MAG | SPT_AC_PHASE );

        // Add two plots: magnitude & phase
        TRACE_DESC mag_desc( *m_exporter, descriptor, (SIM_PLOT_TYPE)( baseType | SPT_AC_MAG ) );
        TRACE_DESC phase_desc( *m_exporter, descriptor, (SIM_PLOT_TYPE)( baseType | SPT_AC_PHASE ) );

        updated |= updatePlot( mag_desc, plotPanel );
        updated |= updatePlot( phase_desc, plotPanel );
    }
    else
    {
        updated = updatePlot( descriptor, plotPanel );
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
    {
        auto& traceMap = m_plots[plotPanel].m_traces;
        auto traceIt = traceMap.find( aPlotName );
        wxASSERT( traceIt != traceMap.end() );
        traceMap.erase( traceIt );
    }

    wxASSERT( plotPanel->IsShown( aPlotName ) );
    plotPanel->DeleteTrace( aPlotName );
    plotPanel->Fit();

    updateSignalList();
    updateCursors();
}


void SIM_PLOT_FRAME::updateNetlistExporter()
{
    m_exporter.reset( new NETLIST_EXPORTER_PSPICE_SIM( m_schematicFrame->BuildNetListBase(),
        Prj().SchLibs(), Prj().SchSearchS() ) );
}


bool SIM_PLOT_FRAME::updatePlot( const TRACE_DESC& aDescriptor, SIM_PLOT_PANEL* aPanel )
{
    SIM_TYPE simType = m_exporter->GetSimType();
    wxString spiceVector = m_exporter->GetSpiceVector( aDescriptor.GetName(),
            aDescriptor.GetType(), aDescriptor.GetParam() );

    if( !SIM_PLOT_PANEL::IsPlottable( simType ) )
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

    SIM_PLOT_TYPE plotType = aDescriptor.GetType();
    std::vector<double> data_y;

    // Now, Y axis data
    switch( m_exporter->GetSimType() )
    {
        case ST_AC:
        {
            wxASSERT_MSG( !( ( plotType & SPT_AC_MAG ) && ( plotType & SPT_AC_PHASE ) ),
                    "Cannot set both AC_PHASE and AC_MAG bits" );

            if( plotType & SPT_AC_MAG )
                data_y = m_simulator->GetMagPlot( (const char*) spiceVector.c_str() );
            else if( plotType & SPT_AC_PHASE )
                data_y = m_simulator->GetPhasePlot( (const char*) spiceVector.c_str() );
            else
                wxASSERT_MSG( false, "Plot type missing AC_PHASE or AC_MAG bit" );
        }
        break;

        case ST_NOISE:
        case ST_DC:
        case ST_TRANSIENT:
        {
            data_y = m_simulator->GetMagPlot( (const char*) spiceVector.c_str() );
        }
        break;

        default:
            wxASSERT_MSG( false, "Unhandled plot type" );
            return false;
    }

    if( data_y.size() != size )
        return false;

    if( aPanel->AddTrace( aDescriptor.GetTitle(), size,
                data_x.data(), data_y.data(), aDescriptor.GetType() ) )
    {
        m_plots[aPanel].m_traces.insert( std::make_pair( aDescriptor.GetTitle(), aDescriptor ) );
    }

    return true;
}


void SIM_PLOT_FRAME::updateSignalList()
{
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( !plotPanel )
        return;

    m_signals->ClearAll();

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
        wxColor tcolor = trace.second->GetTraceColour();

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

    for( const auto& trace : m_plots[plotPanel].m_traces )
    {
        m_signals->InsertItem( imgidx, trace.first, imgidx );
        imgidx++;
    }
}


void SIM_PLOT_FRAME::updateCursors()
{
    wxQueueEvent( this, new wxCommandEvent( EVT_SIM_CURSOR_UPDATE ) );
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
    m_plots.clear();
    m_plotNotebook->DeleteAllPages();

    wxTextFile file( aPath );

    if( !file.Open() )
        return false;

    long plotsCount;

    if( !file.GetFirstLine().ToLong( &plotsCount ) )        // GetFirstLine instead of GetNextLine
        return false;

    for( long i = 0; i < plotsCount; ++i )
    {
        long plotType, tracesCount;

        if( !file.GetNextLine().ToLong( &plotType ) )
            return false;

        SIM_PLOT_PANEL* plotPanel = NewPlotPanel( (SIM_TYPE) plotType );
        m_plots[plotPanel].m_simCommand = file.GetNextLine();
        StartSimulation();

        // Perform simulation, so plots can be added with values
        do
        {
            wxThread::This()->Sleep( 50 );
        }
        while( IsSimulationRunning() );

        if( !file.GetNextLine().ToLong( &tracesCount ) )
            return false;

        for( long j = 0; j < tracesCount; ++j )
        {
            long traceType;
            wxString name, param;

            if( !file.GetNextLine().ToLong( &traceType ) )
                return false;

            name = file.GetNextLine();
            param = file.GetNextLine();

            if( name.IsEmpty() || param.IsEmpty() )
                return false;

            addPlot( name, (SIM_PLOT_TYPE) traceType, param );
        }
    }

    return true;
}


bool SIM_PLOT_FRAME::saveWorkbook( const wxString& aPath )
{
    wxTextFile file( aPath );

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

    file.AddLine( wxString::Format( "%lu", m_plots.size() ) );

    for( const auto& plot : m_plots )
    {
        file.AddLine( wxString::Format( "%d", plot.first->GetType() ) );
        file.AddLine( plot.second.m_simCommand );
        file.AddLine( wxString::Format( "%lu", plot.second.m_traces.size() ) );

        for( const auto& trace : plot.second.m_traces )
        {
            file.AddLine( wxString::Format( "%d", trace.second.GetType() ) );
            file.AddLine( trace.second.GetName() );
            file.AddLine( trace.second.GetParam() );
        }
    }

    bool res = file.Write();
    file.Close();

    return res;
}


SIM_PLOT_TYPE SIM_PLOT_FRAME::GetXAxisType( SIM_TYPE aType ) const
{
    switch( aType )
    {
        case ST_AC:
            return SPT_LIN_FREQUENCY;
            /// @todo SPT_LOG_FREQUENCY

        case ST_DC:
            return SPT_SWEEP;

        case ST_TRANSIENT:
            return SPT_TIME;

        default:
            wxASSERT_MSG( false, "Unhandled simulation type" );
            return (SIM_PLOT_TYPE) 0;
    }
}


void SIM_PLOT_FRAME::menuNewPlot( wxCommandEvent& aEvent )
{
    SIM_TYPE type = m_exporter->GetSimType();

    if( SIM_PLOT_PANEL::IsPlottable( type ) )
    {
        SIM_PLOT_PANEL* prevPlot = CurrentPlot();
        SIM_PLOT_PANEL* newPlot = NewPlotPanel( type );

        // If the previous plot had the same type, copy the simulation command
        if( prevPlot )
            m_plots[newPlot].m_simCommand = m_plots[prevPlot].m_simCommand;
    }
}


void SIM_PLOT_FRAME::menuOpenWorkbook( wxCommandEvent& event )
{
    wxFileDialog openDlg( this, _( "Open simulation workbook" ), m_savedWorkbooksPath, "",
            _( "Workbook file (*.wbk)|*.wbk" ), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( openDlg.ShowModal() == wxID_CANCEL )
        return;

    m_savedWorkbooksPath = openDlg.GetDirectory();

    if( !loadWorkbook( openDlg.GetPath() ) )
        DisplayError( this, _( "There was an error while opening the workbook file" ) );
}


void SIM_PLOT_FRAME::menuSaveWorkbook( wxCommandEvent& event )
{
    if( !CurrentPlot() )
        return;

    wxFileDialog saveDlg( this, _( "Save simulation workbook" ), m_savedWorkbooksPath, "",
                _( "Workbook file (*.wbk)|*.wbk" ), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveDlg.ShowModal() == wxID_CANCEL )
        return;

    m_savedWorkbooksPath = saveDlg.GetDirectory();

    if( !saveWorkbook( saveDlg.GetPath() ) )
        DisplayError( this, _( "There was an error while saving the workbook file" ) );
}


void SIM_PLOT_FRAME::menuSaveImage( wxCommandEvent& event )
{
    if( !CurrentPlot() )
        return;

    wxFileDialog saveDlg( this, _( "Save plot as image" ), "", "",
                _( "PNG file (*.png)|*.png" ), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveDlg.ShowModal() == wxID_CANCEL )
        return;

    CurrentPlot()->SaveScreenshot( saveDlg.GetPath(), wxBITMAP_TYPE_PNG );
}


void SIM_PLOT_FRAME::menuSaveCsv( wxCommandEvent& event )
{
    if( !CurrentPlot() )
        return;

    const wxChar SEPARATOR = ';';

    wxFileDialog saveDlg( this, _( "Save plot data" ), "", "",
                "CSV file (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveDlg.ShowModal() == wxID_CANCEL )
        return;

    wxFile out( saveDlg.GetPath(), wxFile::write );
    bool timeWritten = false;

    for( const auto& t : CurrentPlot()->GetTraces() )
    {
        const TRACE* trace = t.second;

        if( !timeWritten )
        {
            out.Write( wxString::Format( "Time%c", SEPARATOR ) );

            for( double v : trace->GetDataX() )
                out.Write( wxString::Format( "%f%c", v, SEPARATOR ) );

            out.Write( "\r\n" );
            timeWritten = true;
        }

        out.Write( wxString::Format( "%s%c", t.first, SEPARATOR ) );

        for( double v : trace->GetDataY() )
            out.Write( wxString::Format( "%f%c", v, SEPARATOR ) );

        out.Write( "\r\n" );
    }

    out.Close();
}


void SIM_PLOT_FRAME::menuZoomIn( wxCommandEvent& event )
{
    if( CurrentPlot() )
        CurrentPlot()->ZoomIn();
}


void SIM_PLOT_FRAME::menuZoomOut( wxCommandEvent& event )
{
    if( CurrentPlot() )
        CurrentPlot()->ZoomOut();
}


void SIM_PLOT_FRAME::menuZoomFit( wxCommandEvent& event )
{
    if( CurrentPlot() )
        CurrentPlot()->Fit();
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

    if( plot )
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


void SIM_PLOT_FRAME::onPlotClose( wxAuiNotebookEvent& event )
{
    int idx = event.GetSelection();

    if( idx == wxNOT_FOUND )
        return;

    SIM_PLOT_PANEL* plotPanel = dynamic_cast<SIM_PLOT_PANEL*>( m_plotNotebook->GetPage( idx ) );

    if( !plotPanel )
        return;

    m_plots.erase( plotPanel );
    updateSignalList();
    updateCursors();
}


void SIM_PLOT_FRAME::onPlotChanged( wxAuiNotebookEvent& event )
{
    updateSignalList();
    updateCursors();
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
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    // Initial processing is required to e.g. display a list of power sources
    updateNetlistExporter();

    if( !m_exporter->ProcessNetlist( NET_ALL_FLAGS ) )
    {
        DisplayError( this, _( "There were errors during netlist export, aborted." ) );
        return;
    }

    if( !m_settingsDlg )
        m_settingsDlg = new DIALOG_SIM_SETTINGS( this );

    if( plotPanel )
        m_settingsDlg->SetSimCommand( m_plots[plotPanel].m_simCommand );

    m_settingsDlg->SetNetlistExporter( m_exporter.get() );

    if( m_settingsDlg->ShowModal() == wxID_OK )
    {
        wxString newCommand = m_settingsDlg->GetSimCommand();
        SIM_TYPE newSimType = NETLIST_EXPORTER_PSPICE_SIM::CommandToSimType( newCommand );

        // If it is a new simulation type, open a new plot
        if( !plotPanel || ( plotPanel && plotPanel->GetType() != newSimType ) )
        {
            plotPanel = NewPlotPanel( newSimType );
        }

        m_plots[plotPanel].m_simCommand = newCommand;
    }
}


void SIM_PLOT_FRAME::onAddSignal( wxCommandEvent& event )
{
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( !plotPanel || !m_exporter || plotPanel->GetType() != m_exporter->GetSimType() )
    {
        DisplayInfoMessage( this, _( "You need to run simulation first." ) );
        return;
    }

    DIALOG_SIGNAL_LIST dialog( this, m_exporter.get() );
    dialog.ShowModal();
}


void SIM_PLOT_FRAME::onProbe( wxCommandEvent& event )
{
    if( m_schematicFrame == NULL )
        return;

    wxQueueEvent( m_schematicFrame, new wxCommandEvent( wxEVT_TOOL, ID_SIM_PROBE ) );
    m_schematicFrame->Raise();
}


void SIM_PLOT_FRAME::onTune( wxCommandEvent& event )
{
    if( m_schematicFrame == NULL )
        return;

    wxQueueEvent( m_schematicFrame, new wxCommandEvent( wxEVT_TOOL, ID_SIM_TUNE ) );
    m_schematicFrame->Raise();
}


void SIM_PLOT_FRAME::onClose( wxCloseEvent& aEvent )
{
    if( IsSimulationRunning() )
        m_simulator->Stop();

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
    const long X_COL = m_cursors->AppendColumn( plotPanel->GetLabelX(), wxLIST_FORMAT_LEFT, size.x / 4 );

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
    m_toolBar->SetToolNormalBitmap( ID_SIM_RUN, KiBitmap( sim_stop_xpm ) );
    SetCursor( wxCURSOR_ARROWWAIT );
}


void SIM_PLOT_FRAME::onSimFinished( wxCommandEvent& aEvent )
{
    m_toolBar->SetToolNormalBitmap( ID_SIM_RUN, KiBitmap( sim_run_xpm ) );
    SetCursor( wxCURSOR_ARROW );

    SIM_TYPE simType = m_exporter->GetSimType();

    if( simType == ST_UNKNOWN )
        return;

    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( !plotPanel || plotPanel->GetType() != simType )
        plotPanel = NewPlotPanel( simType );

    if( IsSimulationRunning() )
        return;

    // If there are any signals plotted, update them
    if( SIM_PLOT_PANEL::IsPlottable( simType ) )
    {
        TRACE_MAP& traceMap = m_plots[plotPanel].m_traces;

        for( auto it = traceMap.begin(); it != traceMap.end(); /* iteration occurs in the loop */)
        {
            if( !updatePlot( it->second, plotPanel ) )
            {
                removePlot( it->first, false );
                it = traceMap.erase( it );       // remove a plot that does not exist anymore
            }
            else
            {
                ++it;
            }
        }

        updateSignalList();
        plotPanel->UpdateAll();
        plotPanel->ResetScales();
    }
    else
    {
        /// @todo do not make it hardcoded for ngspice
        for( const auto& net : m_exporter->GetNetIndexMap() )
        {
            int node = net.second;

            if( node > 0 )
                m_simulator->Command( wxString::Format( "print v(%d)", node ).ToStdString() );
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
        SIM_PLOT_FRAME* aPlotFrame )
    : m_signal( aSignal ), m_plotFrame( aPlotFrame )
{
    SIM_PLOT_PANEL* plot = m_plotFrame->CurrentPlot();

    AddMenuItem( this, HIDE_SIGNAL, _( "Hide signal" ),
                 _( "Erase the signal from plot screen" ),
                 KiBitmap( delete_xpm ) );

    TRACE* trace = plot->GetTrace( m_signal );

    if( trace->HasCursor() )
        AddMenuItem( this, HIDE_CURSOR, _( "Hide cursor" ),
                     wxEmptyString, KiBitmap( mirepcb_xpm ) );
    else
        AddMenuItem( this, SHOW_CURSOR, _( "Show cursor" ),
                     wxEmptyString, KiBitmap( mirepcb_xpm ) );

    Connect( wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler( SIGNAL_CONTEXT_MENU::onMenuEvent ), NULL, this );
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
