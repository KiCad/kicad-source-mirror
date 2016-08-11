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

#include "sim_plot_frame.h"
#include "sim_plot_panel.h"
#include "spice_simulator.h"
#include "spice_reporter.h"

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
    // Spice vector generation
    m_spiceVector = aExporter.GetSpiceVector( aName, aType, aParam );

    // Title generation
    m_title = wxString::Format( "%s(%s)", aParam, aName );

    if( aType & SPT_AC_MAG )
        m_title += " (mag)";
    else if( aType & SPT_AC_PHASE )
        m_title += " (phase)";
}


SIM_PLOT_FRAME::SIM_PLOT_FRAME( KIWAY* aKiway, wxWindow* aParent )
    : SIM_PLOT_FRAME_BASE( aParent ), m_settingsDlg( this )
{
    SetKiway( this, aKiway );

    m_schematicFrame = (SCH_EDIT_FRAME*) Kiway().Player( FRAME_SCH, false );

    if( m_schematicFrame == NULL )
        throw std::runtime_error( "There is no schematic window" );

    updateNetlistExporter();

    Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( SIM_PLOT_FRAME::onClose ), NULL, this );
    Connect( EVT_SIM_UPDATE, wxCommandEventHandler( SIM_PLOT_FRAME::onSimUpdate ), NULL, this );
    Connect( EVT_SIM_REPORT, wxCommandEventHandler( SIM_PLOT_FRAME::onSimReport ), NULL, this );
    Connect( EVT_SIM_STARTED, wxCommandEventHandler( SIM_PLOT_FRAME::onSimStarted ), NULL, this );
    Connect( EVT_SIM_FINISHED, wxCommandEventHandler( SIM_PLOT_FRAME::onSimFinished ), NULL, this );
    Connect( EVT_SIM_CURSOR_UPDATE, wxCommandEventHandler( SIM_PLOT_FRAME::onCursorUpdate ), NULL, this );


    m_toolSimulate = m_toolBar->AddTool( ID_SIM_RUN, _("Run Simulation"), KiBitmap( sim_run_xpm ), _("Run Simulation"), wxITEM_NORMAL );
    m_toolAddSignals = m_toolBar->AddTool( ID_SIM_ADD_SIGNALS, _("Add Signals"), KiBitmap( sim_add_signal_xpm ), _("Add signals to plot"), wxITEM_NORMAL );
    m_toolProbe = m_toolBar->AddTool( ID_SIM_PROBE,  _("Probe"), KiBitmap( sim_probe_xpm ),_("Probe signals on the schematic"), wxITEM_NORMAL );
    m_toolTune = m_toolBar->AddTool( ID_SIM_TUNE, _("Tune"), KiBitmap( sim_tune_xpm ), _("Tune component values"), wxITEM_NORMAL );
    m_toolSettings = m_toolBar->AddTool( wxID_ANY, _("Settings"), KiBitmap( sim_settings_xpm ), _("Simulation settings"), wxITEM_NORMAL );

    Connect( m_toolSimulate->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME::onSimulate ), NULL, this );
    Connect( m_toolAddSignals->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME::onAddSignal ), NULL, this );
    Connect( m_toolProbe->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME::onProbe ), NULL, this );
    Connect( m_toolTune->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME::onTune ), NULL, this );
    Connect( m_toolSettings->GetId(), wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( SIM_PLOT_FRAME::onSettings ), NULL, this );

    m_toolBar->Realize();
    m_plotNotebook->SetPageText(0, _("Welcome!") );

    //relayout();
}


SIM_PLOT_FRAME::~SIM_PLOT_FRAME()
{
}


void SIM_PLOT_FRAME::StartSimulation()
{
    STRING_FORMATTER formatter;

    m_simConsole->Clear();

    updateNetlistExporter();
    m_exporter->SetSimCommand( m_settingsDlg.GetSimCommand() );
    m_exporter->Format( &formatter, m_settingsDlg.GetNetlistOptions() );

    if( m_exporter->GetSimType() == ST_UNKNOWN )
    {
        DisplayInfoMessage( this, wxT( "You need to select the simulation settings first" ) );
        return;
    }

    /// @todo is it necessary to recreate simulator every time?
    m_simulator.reset( SPICE_SIMULATOR::CreateInstance( "ngspice" ) );

    if( !m_simulator )
        return;

    m_simulator->SetReporter( new SIM_THREAD_REPORTER( this ) );
    m_simulator->Init();
    m_simulator->LoadNetlist( formatter.GetString() );
    m_simulator->Run();

    if ( m_welcomePanel )
    {
        m_plotNotebook->DeletePage( 0 );
        m_welcomePanel = nullptr;
    }

    Layout();
}


void SIM_PLOT_FRAME::StopSimulation()
{
    if( m_simulator )
        m_simulator->Stop();
}


bool SIM_PLOT_FRAME::IsSimulationRunning()
{
    return m_simulator ? m_simulator->IsRunning() : false;
}


SIM_PLOT_PANEL* SIM_PLOT_FRAME::NewPlotPanel( SIM_TYPE aSimType )
{
    SIM_PLOT_PANEL* plot = new SIM_PLOT_PANEL( aSimType, m_plotNotebook, wxID_ANY );

    m_plotNotebook->AddPage( plot, wxString::Format( wxT( "Plot%u" ),
            (unsigned int) m_plotNotebook->GetPageCount() + 1 ), true );

    return plot;
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
    auto& tunerList = m_plots[plotPanel].m_tuners;

    // Do not add multiple instances for the same component
    auto tunerIt = std::find_if( tunerList.begin(), tunerList.end(), [&]( const TUNER_SLIDER* t )
        {
            return t->GetComponentName() == componentName;
        }
    );

    if( tunerIt != tunerList.end() )
        return;     // We already have it

    try
    {
        TUNER_SLIDER* tuner = new TUNER_SLIDER( this, m_sidePanel, aComponent );
        m_tuneSizer->Add( tuner );
        tunerList.push_back( tuner );
        m_sidePanel->Layout();
    }
    catch( ... )
    {
        // Sorry, no bonus
    }
}


void SIM_PLOT_FRAME::RemoveTuner( TUNER_SLIDER* aTuner )
{
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( !plotPanel )
        return;

    m_plots[plotPanel].m_tuners.remove( aTuner );
    aTuner->Destroy();
    m_sidePanel->Layout();
}


SIM_PLOT_PANEL* SIM_PLOT_FRAME::CurrentPlot() const
{
    return static_cast<SIM_PLOT_PANEL*>( m_plotNotebook->GetCurrentPage() );
}


void SIM_PLOT_FRAME::addPlot( const wxString& aName, SIM_PLOT_TYPE aType, const wxString& aParam )
{
    SIM_TYPE simType = m_exporter->GetSimType();

    if( !SIM_PLOT_PANEL::IsPlottable( simType ) )
        return; // TODO else write out in console?

    // Create a new plot if the current one displays a different type
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( plotPanel == nullptr || plotPanel->GetType() != simType )
        plotPanel = NewPlotPanel( simType );

    TRACE_DESC descriptor( *m_exporter, aName, aType, aParam );

    bool updated = false;
    SIM_PLOT_TYPE xAxisType = GetXAxisType( simType );

    if( xAxisType == SPT_LIN_FREQUENCY || xAxisType == SPT_LOG_FREQUENCY )
    {
        // Add two plots: magnitude & phase
        TRACE_DESC mag_desc( *m_exporter, descriptor, descriptor.GetType() | SPT_AC_MAG );
        TRACE_DESC phase_desc( *m_exporter, descriptor, descriptor.GetType() | SPT_AC_PHASE );

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

void SIM_PLOT_FRAME::removePlot( const wxString& aPlotName )
{
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();
    auto& traceMap = m_plots[plotPanel].m_traces;
    auto traceIt = traceMap.find( aPlotName );

    wxASSERT( traceIt != traceMap.end() );
    traceMap.erase( traceIt );

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
    if( !m_simulator )
        return false;

    SIM_TYPE simType = m_exporter->GetSimType();
    wxString spiceVector = aDescriptor.GetSpiceVector();

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

    // Fill the signals listbox
    m_signals->Clear();

    for( const auto& trace : m_plots[plotPanel].m_traces )
        m_signals->Append( trace.first );
}


void SIM_PLOT_FRAME::updateTuners()
{
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( !plotPanel )
        return;

    for( unsigned int i = 0; i < m_tuneSizer->GetItemCount(); ++i )
        m_tuneSizer->Hide( i );

    m_tuneSizer->Clear();

    for( auto tuner : m_plots[plotPanel].m_tuners )
    {
        m_tuneSizer->Add( tuner );
        tuner->Show();
    }

    Layout();
}


void SIM_PLOT_FRAME::updateCursors()
{
    wxQueueEvent( this, new wxCommandEvent( EVT_SIM_CURSOR_UPDATE ) );
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
        NewPlotPanel( type );
}


void SIM_PLOT_FRAME::menuSaveImage( wxCommandEvent& event )
{
    wxFileDialog saveDlg( this, wxT( "Save plot as image" ), "", "",
                "PNG file (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveDlg.ShowModal() == wxID_CANCEL )
        return;

    CurrentPlot()->SaveScreenshot( saveDlg.GetPath(), wxBITMAP_TYPE_PNG );
}


void SIM_PLOT_FRAME::menuSaveCsv( wxCommandEvent& event )
{
    const wxChar SEPARATOR = ';';

    wxFileDialog saveDlg( this, wxT( "Save plot data" ), "", "",
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
    CurrentPlot()->ZoomIn();
}


void SIM_PLOT_FRAME::menuZoomOut( wxCommandEvent& event )
{
    CurrentPlot()->ZoomOut();
}


void SIM_PLOT_FRAME::menuZoomFit( wxCommandEvent& event )
{
    CurrentPlot()->Fit();
}


void SIM_PLOT_FRAME::menuShowGrid( wxCommandEvent& event )
{
    SIM_PLOT_PANEL* plot = CurrentPlot();
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
    plot->ShowLegend( !plot->IsLegendShown() );
}


void SIM_PLOT_FRAME::menuShowLegendUpdate( wxUpdateUIEvent& event )
{
    SIM_PLOT_PANEL* plot = CurrentPlot();
    event.Check( plot ? plot->IsLegendShown() : false );
}

#if 0
void SIM_PLOT_FRAME::menuShowCoords( wxCommandEvent& event )
{
    SIM_PLOT_PANEL* plot = CurrentPlot();
    plot->ShowCoords( !plot->IsCoordsShown() );
}


void SIM_PLOT_FRAME::menuShowCoordsUpdate( wxUpdateUIEvent& event )
{
    SIM_PLOT_PANEL* plot = CurrentPlot();
    event.Check( plot ? plot->IsCoordsShown() : false );
}
#endif

void SIM_PLOT_FRAME::onPlotChanged( wxNotebookEvent& event )
{
    updateSignalList();
    updateTuners();
    updateCursors();
}


void SIM_PLOT_FRAME::onSignalDblClick( wxCommandEvent& event )
{
    // Remove signal from the plot panel when double clicked
    int idx = m_signals->GetSelection();

    if( idx != wxNOT_FOUND )
        removePlot( m_signals->GetString( idx ) );
}


void SIM_PLOT_FRAME::onSignalRClick( wxMouseEvent& event )
{
    int idx = m_signals->HitTest( event.GetPosition() );

    if( idx != wxNOT_FOUND )
        m_signals->SetSelection( idx );

    idx = m_signals->GetSelection();

    if( idx != wxNOT_FOUND )
    {
        const wxString& netName = m_signals->GetString( idx );
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
    updateNetlistExporter();
    m_exporter->ProcessNetlist( NET_ALL_FLAGS );
    m_settingsDlg.SetNetlistExporter( m_exporter.get() );
    m_settingsDlg.ShowModal();
}


void SIM_PLOT_FRAME::onAddSignal( wxCommandEvent& event )
{
    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( !plotPanel || !m_exporter || plotPanel->GetType() != m_exporter->GetSimType() )
    {
        DisplayInfoMessage( this, wxT( "You need to run simulation first" ) );
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
}


void SIM_PLOT_FRAME::onTune( wxCommandEvent& event )
{
    if( m_schematicFrame == NULL )
        return;

    wxQueueEvent( m_schematicFrame, new wxCommandEvent( wxEVT_TOOL, ID_SIM_TUNE ) );
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
    m_cursors->ClearAll();

    const long SIGNAL_COL = m_cursors->AppendColumn( wxT( "Signal" ), wxLIST_FORMAT_LEFT, size.x / 2 );
    const long X_COL = m_cursors->AppendColumn( CurrentPlot()->GetLabelX(), wxLIST_FORMAT_LEFT, size.x / 4 );

    wxString labelY1 = CurrentPlot()->GetLabelY1();
    wxString labelY2 = CurrentPlot()->GetLabelY2();
    wxString labelY;

    if( !labelY2.IsEmpty() )
        labelY = labelY1 + " / " + labelY2;
    else
        labelY = labelY1;

    const long Y_COL = m_cursors->AppendColumn( labelY, wxLIST_FORMAT_LEFT, size.x / 4 );

    // Update cursor values
    for( const auto& trace : CurrentPlot()->GetTraces() )
    {
        if( CURSOR* cursor = trace.second->GetCursor() )
        {
            const wxRealPoint coords = cursor->GetCoords();
            long idx = m_cursors->InsertItem( SIGNAL_COL, trace.first );
            m_cursors->SetItem( idx, X_COL, SPICE_VALUE( coords.x ).ToSpiceString() );
            m_cursors->SetItem( idx, Y_COL, SPICE_VALUE( coords.y ).ToSpiceString() );
        }
    }
}


void SIM_PLOT_FRAME::onSimStarted( wxCommandEvent& aEvent )
{
    //m_simulateBtn->SetLabel( wxT( "Stop" ) );
    SetCursor( wxCURSOR_ARROWWAIT );
}


void SIM_PLOT_FRAME::onSimFinished( wxCommandEvent& aEvent )
{
    //m_simulateBtn->SetLabel( wxT( "Simulate" ) );
    SetCursor( wxCURSOR_ARROW );

    SIM_TYPE simType = m_exporter->GetSimType();

    if( simType == ST_UNKNOWN )
        return;

    SIM_PLOT_PANEL* plotPanel = CurrentPlot();

    if( plotPanel == nullptr || plotPanel->GetType() != simType )
        plotPanel = NewPlotPanel( simType );

    // If there are any signals plotted, update them
    if( SIM_PLOT_PANEL::IsPlottable( simType ) )
    {
        for( const auto& trace : m_plots[plotPanel].m_traces )
            updatePlot( trace.second, plotPanel );

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
    if( !m_simulator )
        return;

    if( IsSimulationRunning() )
        StopSimulation();

    m_simConsole->Clear();

    // Apply tuned values
    if( SIM_PLOT_PANEL* plotPanel = CurrentPlot() )
    {
        for( auto tuner : m_plots[plotPanel].m_tuners )
        {
            /// @todo no ngspice hardcoding
            std::string command( "alter @" + tuner->GetSpiceName()
                    + "=" + tuner->GetValue().ToSpiceString() );
            m_simulator->Command( command );

//            printf("CMD: %s\n", command.c_str() );
        }
    }

    m_simulator->Run();
}


void SIM_PLOT_FRAME::onSimReport( wxCommandEvent& aEvent )
{
    std::cout << aEvent.GetString() << std::endl;
    m_simConsole->AppendText( aEvent.GetString() + "\n" );
    m_simConsole->SetInsertionPointEnd();
}


SIM_PLOT_FRAME::SIGNAL_CONTEXT_MENU::SIGNAL_CONTEXT_MENU( const wxString& aSignal,
        SIM_PLOT_FRAME* aPlotFrame )
    : m_signal( aSignal ), m_plotFrame( aPlotFrame )
{
    SIM_PLOT_PANEL* plot = m_plotFrame->CurrentPlot();

    Append( HIDE_SIGNAL, wxT( "Hide signal" ) );

    TRACE* trace = plot->GetTrace( m_signal );

    if( trace->HasCursor() )
        Append( HIDE_CURSOR, wxT( "Hide cursor" ) );
    else
        Append( SHOW_CURSOR, wxT( "Show cursor" ) );

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
