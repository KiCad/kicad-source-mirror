/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2023 CERN
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <grid_tricks.h>
#include <eda_pattern_match.h>
#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tool/action_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tools/simulator_control.h>
#include <tools/ee_actions.h>
#include <string_utils.h>
#include <pgm_base.h>
#include "ngspice.h"
#include "sim_plot_frame.h"
#include "sim_plot_panel.h"
#include "spice_simulator.h"
#include "spice_reporter.h"
#include <dialog_sim_format_value.h>
#include <eeschema_settings.h>

#include <memory>


SIM_TRACE_TYPE operator|( SIM_TRACE_TYPE aFirst, SIM_TRACE_TYPE aSecond )
{
    int res = (int) aFirst | (int) aSecond;

    return (SIM_TRACE_TYPE) res;
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
    SIM_PLOT_FRAME* m_parent;
};


enum SIGNALS_GRID_COLUMNS
{
    COL_SIGNAL_NAME = 0,
    COL_SIGNAL_SHOW,
    COL_SIGNAL_COLOR,
    COL_CURSOR_1,
    COL_CURSOR_2
};


enum CURSORS_GRID_COLUMNS
{
    COL_CURSOR_NAME = 0,
    COL_CURSOR_SIGNAL,
    COL_CURSOR_X,
    COL_CURSOR_Y
};


enum
{
    MYID_FORMAT_VALUE = GRIDTRICKS_FIRST_CLIENT_ID
};


class CURSORS_GRID_TRICKS : public GRID_TRICKS
{
public:
    CURSORS_GRID_TRICKS( SIM_PLOT_FRAME* aParent, WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid ),
            m_parent( aParent )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    SIM_PLOT_FRAME* m_parent;
    int             m_menuRow;
    int             m_menuCol;
};


void CURSORS_GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    m_menuRow = aEvent.GetRow();
    m_menuCol = aEvent.GetCol();

    if( m_menuCol == COL_CURSOR_X || m_menuCol == COL_CURSOR_Y )
    {
        wxString msg = m_grid->GetColLabelValue( m_menuCol );

        menu.Append( MYID_FORMAT_VALUE, wxString::Format( _( "Format..." ), msg ),
                     wxString::Format( _( "Specify %s precision and range" ), msg.Lower() ) );
        menu.AppendSeparator();
    }

    GRID_TRICKS::showPopupMenu( menu, aEvent );
}


void CURSORS_GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    if( event.GetId() == MYID_FORMAT_VALUE )
    {
        int      cursorId = m_menuRow;
        int      cursorAxis = m_menuCol - COL_CURSOR_X;
        int      precision = m_parent->GetCursorPrecision( cursorId, cursorAxis );
        wxString range = m_parent->GetCursorRange( cursorId, cursorAxis );

        DIALOG_SIM_FORMAT_VALUE formatDialog( m_parent, &precision, &range );

        if( formatDialog.ShowModal() == wxID_OK )
        {
            m_parent->SetCursorPrecision( cursorId, cursorAxis, precision );
            m_parent->SetCursorRange( cursorId, cursorAxis, range );
        }
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


SIM_PLOT_FRAME::SIM_PLOT_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        SIM_PLOT_FRAME_BASE( aParent ),
        m_lastSimPlot( nullptr ),
        m_darkMode( true ),
        m_plotNumber( 0 ),
        m_simFinished( false )
{
    SetKiway( this, aKiway );

    m_schematicFrame = (SCH_EDIT_FRAME*) Kiway().Player( FRAME_SCH, false );
    wxASSERT( m_schematicFrame );

    // Give an icon
    wxIcon icon;
    icon.CopyFromBitmap( KiBitmap( BITMAPS::simulator ) );
    SetIcon( icon );

    m_simulator = SIMULATOR::CreateInstance( "ngspice" );
    wxASSERT( m_simulator );

    // Get the previous size and position of windows:
    LoadSettings( config() );

    m_signalsGrid->wxGrid::SetLabelFont( KIUI::GetStatusFont( this ) );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_signalsGrid->SetColAttr( COL_SIGNAL_NAME, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_signalsGrid->SetColAttr( COL_SIGNAL_SHOW, attr );

    m_signalsGrid->PushEventHandler( new GRID_TRICKS( m_signalsGrid ) );

    m_cursorsGrid->wxGrid::SetLabelFont( KIUI::GetStatusFont( this ) );

    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_cursorsGrid->SetColAttr( COL_CURSOR_NAME, attr );

    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_cursorsGrid->SetColAttr( COL_CURSOR_SIGNAL, attr );

    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_cursorsGrid->SetColAttr( COL_CURSOR_Y, attr );

    m_cursorsGrid->PushEventHandler( new CURSORS_GRID_TRICKS( this, m_cursorsGrid ) );

    for( int cursorId = 0; cursorId < 3; ++cursorId )
    {
        m_cursorPrecision[ cursorId ][ 0 ] = 3;
        m_cursorRange[ cursorId ][ 0 ] = wxS( "~s" );
        m_cursorPrecision[ cursorId ][ 1 ] = 3;
        m_cursorRange[ cursorId ][ 1 ] = wxS( "~V" );
    }

    // Prepare the color list to plot traces
    SIM_PLOT_COLORS::FillDefaultColorList( m_darkMode );

    m_simulator->Init();

    m_reporter = new SIM_THREAD_REPORTER( this );
    m_simulator->SetReporter( m_reporter );

    m_circuitModel = std::make_shared<NGSPICE_CIRCUIT_MODEL>( &m_schematicFrame->Schematic(), this );

    setupTools();
    setupUIConditions();

    ReCreateHToolbar();
    ReCreateMenuBar();

    Bind( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIM_PLOT_FRAME::onExit ), this,
          wxID_EXIT );

    Bind( EVT_SIM_UPDATE, &SIM_PLOT_FRAME::onSimUpdate, this );
    Bind( EVT_SIM_REPORT, &SIM_PLOT_FRAME::onSimReport, this );
    Bind( EVT_SIM_STARTED, &SIM_PLOT_FRAME::onSimStarted, this );
    Bind( EVT_SIM_FINISHED, &SIM_PLOT_FRAME::onSimFinished, this );
    Bind( EVT_SIM_CURSOR_UPDATE, &SIM_PLOT_FRAME::onCursorUpdate, this );

    Bind( EVT_WORKBOOK_MODIFIED, &SIM_PLOT_FRAME::onWorkbookModified, this );
    Bind( EVT_WORKBOOK_CLR_MODIFIED, &SIM_PLOT_FRAME::onWorkbookClrModified, this );

#ifndef wxHAS_NATIVE_TABART
    // Default non-native tab art has ugly gradients we don't want
    m_workbook->SetArtProvider( new wxAuiSimpleTabArt() );
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
    updateTitle();
}


SIM_PLOT_FRAME::~SIM_PLOT_FRAME()
{
    // Delete the GRID_TRICKS.
    m_signalsGrid->PopEventHandler( true );
    m_cursorsGrid->PopEventHandler( true );

    NULL_REPORTER devnull;

    m_simulator->Attach( nullptr, devnull );
    m_simulator->SetReporter( nullptr );
    delete m_reporter;
}


void SIM_PLOT_FRAME::setupTools()
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


void SIM_PLOT_FRAME::ShowChangedLanguage()
{
    EDA_BASE_FRAME::ShowChangedLanguage();

    updateTitle();

    for( int ii = 0; ii < (int) m_workbook->GetPageCount(); ++ii )
    {
        SIM_PANEL_BASE* plot = dynamic_cast<SIM_PLOT_PANEL*>( m_workbook->GetPage( ii ) );

        plot->OnLanguageChanged();

        wxString pageTitle( m_simulator->TypeToName( plot->GetType(), true ) );
        pageTitle.Prepend( wxString::Format( _( "Plot%u - " ), ii+1 /* 1-based */ ) );

        m_workbook->SetPageText( ii, pageTitle );
    }

    m_signalsGrid->SetColLabelValue( COL_SIGNAL_NAME, _( "Signal" ) );
    m_signalsGrid->SetColLabelValue( COL_SIGNAL_SHOW, _( "Plot" ) );
    m_signalsGrid->SetColLabelValue( COL_SIGNAL_COLOR, _( "Color" ) );
    m_signalsGrid->SetColLabelValue( COL_CURSOR_1, _( "Cursor 1" ) );
    m_signalsGrid->SetColLabelValue( COL_CURSOR_2, _( "Cursor 2" ) );

    m_cursorsGrid->SetColLabelValue( COL_CURSOR_NAME, _( "Cursor" ) );
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_SIGNAL, _( "Signal" ) );
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_X, _( "Time" ) );
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_Y, _( "Voltage / Current" ) );
    wxCommandEvent dummy;
    onCursorUpdate( dummy );

    m_staticTextTune->SetLabel( _( "Tune" ) );
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
        m_darkMode                           = !cfg->m_Simulator.white_background;
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
        cfg->m_Simulator.white_background     = !m_darkMode;
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
    // Removed for the time being. We cannot run the simulation on simulator launch, as it may
    // take a lot of time, confusing the user.
    // TODO: Change workbook loading routines so that they don't run the simulation until the user
    // initiates it.

    /*if( !m_simulator->Settings()->GetWorkbookFilename().IsEmpty() )
    {
        wxFileName filename = m_simulator->Settings()->GetWorkbookFilename();
        filename.SetPath( Prj().GetProjectPath() );

        if( !loadWorkbook( filename.GetFullPath() ) )
            m_simulator->Settings()->SetWorkbookFilename( "" );
    }*/
}


void SIM_PLOT_FRAME::updateTitle()
{
    bool     unsaved = true;
    bool     readOnly = false;
    wxString title;

    if( m_simulator && m_simulator->Settings() )
    {
        wxFileName filename = Prj().AbsolutePath( m_simulator->Settings()->GetWorkbookFilename() );

        if( filename.IsOk() && filename.FileExists() )
        {
            unsaved = false;
            readOnly = !filename.IsFileWritable();
        }

        if( m_workbook->IsModified() )
            title = wxT( "*" ) + filename.GetName();
        else
            title = filename.GetName();
    }

    if( readOnly )
        title += wxS( " " ) + _( "[Read Only]" );

    if( unsaved )
        title += wxS( " " ) + _( "[Unsaved]" );

    title += wxT( " \u2014 " ) + _( "Spice Simulator" );

    SetTitle( title );
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


void SIM_PLOT_FRAME::rebuildSignalsGrid( wxString aFilter )
{
    m_signalsGrid->ClearRows();

    if( !GetCurrentPlot() )
        return;

    if( aFilter.IsEmpty() )
        aFilter = wxS( "*" );

    EDA_COMBINED_MATCHER matcher( aFilter, CTX_SIGNAL );
    int                  row = 0;

    for( const wxString& signal : m_signals )
    {
        int matches;
        int offset;

        if( matcher.Find( signal, matches, offset ) && offset == 0 )
        {
            m_signalsGrid->AppendRows( 1 );

            m_signalsGrid->SetCellValue( row, COL_SIGNAL_NAME, signal );

            if( TRACE* trace = GetCurrentPlot()->GetTrace( signal ) )
            {
                m_signalsGrid->SetCellValue( row, COL_SIGNAL_SHOW, wxS( "1" ) );

                wxGridCellAttr* attr = new wxGridCellAttr;
                attr->SetRenderer( new GRID_CELL_COLOR_RENDERER( this ) );
                attr->SetEditor( new GRID_CELL_COLOR_SELECTOR( this, m_signalsGrid ) );
                attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
                m_signalsGrid->SetAttr( row, COL_SIGNAL_COLOR, attr );
                KIGFX::COLOR4D color( trace->GetPen().GetColour() );
                m_signalsGrid->SetCellValue( row, COL_SIGNAL_COLOR, color.ToCSSString() );

                attr = new wxGridCellAttr;
                attr->SetRenderer( new wxGridCellBoolRenderer() );
                attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
                attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
                m_signalsGrid->SetAttr( row, COL_CURSOR_1, attr );

                attr = new wxGridCellAttr;
                attr->SetRenderer( new wxGridCellBoolRenderer() );
                attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
                attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
                m_signalsGrid->SetAttr( row, COL_CURSOR_2, attr );
            }
            else
            {
                wxGridCellAttr* attr = new wxGridCellAttr;
                attr->SetReadOnly();
                m_signalsGrid->SetAttr( row, COL_SIGNAL_COLOR, attr );
                m_signalsGrid->SetCellValue( row, COL_SIGNAL_COLOR, wxEmptyString );

                attr = new wxGridCellAttr;
                attr->SetReadOnly();
                m_signalsGrid->SetAttr( row, COL_CURSOR_1, attr );

                attr = new wxGridCellAttr;
                attr->SetReadOnly();
                m_signalsGrid->SetAttr( row, COL_CURSOR_2, attr );
            }

            row++;
        }
    }
}


void SIM_PLOT_FRAME::StartSimulation( const wxString& aSimCommand )
{
    if( m_circuitModel->CommandToSimType( GetCurrentSimCommand() ) == ST_UNKNOWN )
    {
        if( !EditSimCommand()
            || m_circuitModel->CommandToSimType( GetCurrentSimCommand() ) == ST_UNKNOWN )
        {
            return;
        }
    }

    m_simConsole->Clear();

    if( aSimCommand != wxEmptyString )
        m_circuitModel->SetSimCommandOverride( aSimCommand );

    m_circuitModel->SetSimOptions( GetCurrentOptions() );

    wxString           errors;
    WX_STRING_REPORTER reporter( &errors );

    if( !m_schematicFrame->ReadyToNetlist( _( "Simulator requires a fully annotated schematic." ) )
            || !m_simulator->Attach( m_circuitModel, reporter ) )
    {
        DisplayErrorMessage( this, _( "Errors during netlist generation; simulation aborted.\n\n" )
                                   + errors );
        return;
    }

    SIM_PANEL_BASE* plotWindow = getCurrentPlotWindow();
    wxString        sheetSimCommand = m_circuitModel->GetSheetSimCommand();

    if( plotWindow
            && plotWindow->GetType() == NGSPICE_CIRCUIT_MODEL::CommandToSimType( sheetSimCommand ) )
    {
        if( m_circuitModel->GetSimCommandOverride().IsEmpty() )
        {
            m_workbook->SetSimCommand( plotWindow, sheetSimCommand );
        }
        else if( sheetSimCommand != m_circuitModel->GetLastSheetSimCommand() )
        {
            if( IsOK( this, _( "Schematic sheet simulation command directive has changed.  Do you "
                               "wish to update the Simulation Command?" ) ) )
            {
                m_circuitModel->SetSimCommandOverride( wxEmptyString );
                m_workbook->SetSimCommand( plotWindow, sheetSimCommand );
            }
        }
    }

    if( !plotWindow || plotWindow->GetType() != m_circuitModel->GetSimType() )
    {
        plotWindow = NewPlotPanel( m_circuitModel->GetSimCommand(),
                                   m_circuitModel->GetSimOptions() );
    }

    std::unique_lock<std::mutex> simulatorLock( m_simulator->GetMutex(), std::try_to_lock );

    if( simulatorLock.owns_lock() )
    {
        wxBusyCursor toggle;
        wxString     unconnected = wxString( wxS( "unconnected-(" ) );

        unconnected.Replace( '(', '_' );    // Convert to SPICE markup
        m_signals.clear();

        auto simType = m_circuitModel->GetSimType();

        // Add voltages
        for( const std::string& net : m_circuitModel->GetNets() )
        {
            // netnames are escaped (can contain "{slash}" for '/') Unscape them:
            wxString netname = UnescapeString( net );

            if( netname != "GND" && netname != "0" && !netname.StartsWith( unconnected ) )
            {
                if( simType == ST_AC )
                {
                    m_signals.push_back( wxString::Format( _( "V(%s) (gain)" ), netname ) );
                    m_signals.push_back( wxString::Format( _( "V(%s) (phase)" ), netname ) );
                }
                else
                {
                    m_signals.push_back( wxString::Format( "V(%s)", netname ) );
                }
            }
        }

        // Add currents
        if( simType == ST_TRANSIENT || simType == ST_DC )
        {
            for( const SPICE_ITEM& item : m_circuitModel->GetItems() )
            {
                // Add all possible currents for the primitive.
                for( const std::string& name : item.model->SpiceGenerator().CurrentNames( item ) )
                    m_signals.push_back( name );
            }
        }

        rebuildSignalsGrid( m_filter->GetValue() );

        applyTuners();

        // Prevents memory leak on succeding simulations by deleting old vectors
        m_simulator->Clean();
        m_simulator->Run();
    }
    else
    {
        DisplayErrorMessage( this, _( "Another simulation is already running." ) );
    }
}


SIM_PANEL_BASE* SIM_PLOT_FRAME::NewPlotPanel( const wxString& aSimCommand, int aOptions )
{
    SIM_PANEL_BASE* plotPanel = nullptr;
    SIM_TYPE        simType   = NGSPICE_CIRCUIT_MODEL::CommandToSimType( aSimCommand );

    if( SIM_PANEL_BASE::IsPlottable( simType ) )
    {
        SIM_PLOT_PANEL* panel = new SIM_PLOT_PANEL( aSimCommand, aOptions, m_workbook, wxID_ANY );
        plotPanel = dynamic_cast<SIM_PANEL_BASE*>( panel );

        COMMON_SETTINGS::INPUT cfg = Pgm().GetCommonSettings()->m_Input;
        panel->GetPlotWin()->EnableMouseWheelPan( cfg.scroll_modifier_zoom != 0 );
    }
    else
    {
        SIM_NOPLOT_PANEL* panel = new SIM_NOPLOT_PANEL( aSimCommand, aOptions, m_workbook, wxID_ANY );
        plotPanel = dynamic_cast<SIM_PANEL_BASE*>( panel );
    }

    wxString pageTitle( m_simulator->TypeToName( simType, true ) );
    pageTitle.Prepend( wxString::Format( _( "Plot%u - " ), (unsigned int) ++m_plotNumber ) );

    m_workbook->AddPage( dynamic_cast<wxWindow*>( plotPanel ), pageTitle, true );

    return plotPanel;
}


void SIM_PLOT_FRAME::OnFilterText( wxCommandEvent& aEvent )
{
    rebuildSignalsGrid( m_filter->GetValue() );
}


void SIM_PLOT_FRAME::OnFilterMouseMoved( wxMouseEvent& aEvent )
{
    wxPoint pos = aEvent.GetPosition();
    wxRect  ctrlRect = m_filter->GetScreenRect();
    int     buttonWidth = ctrlRect.GetHeight();         // Presume buttons are square

    if( m_filter->IsSearchButtonVisible() && pos.x < buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else if( m_filter->IsCancelButtonVisible() && pos.x > ctrlRect.GetWidth() - buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else
        SetCursor( wxCURSOR_IBEAM );
}


void SIM_PLOT_FRAME::onSignalsGridCellChanged( wxGridEvent& aEvent )
{
    int             row = aEvent.GetRow();
    int             col = aEvent.GetCol();
    wxString        text = m_signalsGrid->GetCellValue( row, col );
    SIM_PLOT_PANEL* plot = GetCurrentPlot();

    if( col == COL_SIGNAL_SHOW )
    {
        if( text == wxS( "1" ) )
        {
            wxString signalName = m_signalsGrid->GetCellValue( row, COL_SIGNAL_NAME );
            wxString baseSignal = signalName;

            if( !signalName.IsEmpty() )
            {
                wxString  gainSuffix = _( " (gain)" );
                wxString  phaseSuffix = _( " (phase)" );
                wxUniChar firstChar = signalName[0];
                int       traceType = SPT_UNKNOWN;

                if( firstChar == 'V' || firstChar == 'v' )
                    traceType = SPT_VOLTAGE;
                else if( firstChar == 'I' || firstChar == 'i' )
                    traceType = SPT_CURRENT;

                if( signalName.EndsWith( gainSuffix ) )
                {
                    traceType |= SPT_AC_MAG;
                    baseSignal = signalName.Left( signalName.Length() - gainSuffix.Length() );
                }
                else if( signalName.EndsWith( phaseSuffix ) )
                {
                    traceType |= SPT_AC_PHASE;
                    baseSignal = signalName.Left( signalName.Length() - phaseSuffix.Length() );
                }

                if( traceType != SPT_UNKNOWN )
                    addTrace( baseSignal, (SIM_TRACE_TYPE) traceType );
            }

            if( !GetCurrentPlot()->GetTrace( signalName ) )
            {
                aEvent.Veto();
                return;
            }
        }
        else
        {
            removeTrace( m_signalsGrid->GetCellValue( row, COL_SIGNAL_NAME ) );
        }

        // Update enabled/visible states of other controls
        updateSignalsGrid();
    }
    else if( col == COL_SIGNAL_COLOR )
    {
        KIGFX::COLOR4D color( m_signalsGrid->GetCellValue( row, COL_SIGNAL_COLOR ) );
        wxString       signalName = m_signalsGrid->GetCellValue( row, COL_SIGNAL_NAME );
        TRACE*         trace = plot->GetTrace( signalName );

        if( trace )
        {
            trace->SetTraceColour( color.ToColour() );
            plot->UpdateTraceStyle( trace );
            plot->UpdatePlotColors();
        }
    }
    else if( col == COL_CURSOR_1 || col == COL_CURSOR_2 )
    {
        for( int ii = 0; ii < m_signalsGrid->GetNumberRows(); ++ii )
        {
            wxString signalName = m_signalsGrid->GetCellValue( ii, COL_SIGNAL_NAME );
            bool     enable = ii == row && text == wxS( "1" );

            plot->EnableCursor( signalName, col == COL_CURSOR_1 ? 1 : 2, enable );
        }

        // Update cursor checkboxes (which are really radio buttons)
        updateSignalsGrid();
    }
}


void SIM_PLOT_FRAME::onCursorsGridCellChanged( wxGridEvent& aEvent )
{
    SIM_PLOT_PANEL* plotPanel = GetCurrentPlot();

    if( !plotPanel )
        return;

    int      row = aEvent.GetRow();
    int      col = aEvent.GetCol();
    wxString text = m_cursorsGrid->GetCellValue( row, col );
    wxString cursorName = m_cursorsGrid->GetCellValue( row, COL_CURSOR_NAME );

    if( col == COL_CURSOR_X )
    {
        CURSOR* cursor1 = nullptr;
        CURSOR* cursor2 = nullptr;

        for( const auto& [name, trace] : plotPanel->GetTraces() )
        {
            if( CURSOR* cursor = trace->GetCursor( 1 ) )
                cursor1 = cursor;

            if( CURSOR* cursor = trace->GetCursor( 2 ) )
                cursor2 = cursor;
        }

        double value = SPICE_VALUE( text ).ToDouble();

        if( cursorName == wxS( "1" ) && cursor1 )
            cursor1->SetCoordX( value );
        else if( cursorName == wxS( "2" ) && cursor2 )
            cursor2->SetCoordX( value );
        else if( cursorName == _( "Diff" ) && cursor1 && cursor2 )
            cursor2->SetCoordX( cursor1->GetCoords().x + value );

        wxCommandEvent dummy;
        onCursorUpdate( dummy );
    }
    else
    {
        wxFAIL_MSG( wxT( "All other columns are supposed to be read-only!" ) );
    }
}


void SIM_PLOT_FRAME::AddVoltagePlot( const wxString& aNetName )
{
    addTrace( aNetName, SPT_VOLTAGE );
}


void SIM_PLOT_FRAME::AddCurrentPlot( const wxString& aDeviceName )
{
    addTrace( aDeviceName, SPT_CURRENT );
}


void SIM_PLOT_FRAME::AddTuner( const SCH_SHEET_PATH& aSheetPath, SCH_SYMBOL* aSymbol )
{
    SIM_PANEL_BASE* plotPanel = getCurrentPlotWindow();

    if( !plotPanel )
        return;

    wxString ref = aSymbol->GetRef( &aSheetPath );

    // Do not add multiple instances for the same component.
    for( TUNER_SLIDER* tuner : m_tuners )
    {
        if( tuner->GetSymbolRef() == ref )
            return;
    }

    const SPICE_ITEM* item = GetExporter()->FindItem( std::string( ref.ToUTF8() ) );

    // Do nothing if the symbol is not tunable.
    if( !item || !item->model->GetTunerParam() )
        return;

    try
    {
        TUNER_SLIDER* tuner = new TUNER_SLIDER( this, m_tunePanel, aSheetPath, aSymbol );
        m_tuneSizer->Add( tuner );
        m_tuners.push_back( tuner );
        m_tunePanel->Layout();
    }
    catch( const KI_PARAM_ERROR& e )
    {
        DisplayErrorMessage( nullptr, e.What() );
    }
}


void SIM_PLOT_FRAME::UpdateTunerValue( const SCH_SHEET_PATH& aSheetPath, const KIID& aSymbol,
                                       const wxString& aRef, const wxString& aValue )
{
    SCH_ITEM*   item = aSheetPath.GetItem( aSymbol );
    SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item );

    if( !symbol )
    {
        DisplayErrorMessage( this, _( "Could not apply tuned value(s):" ) + wxS( " " )
                                   + wxString::Format( _( "%s not found" ), aRef ) );
        return;
    }

    SIM_LIB_MGR mgr( &Prj() );
    SIM_MODEL&  model = mgr.CreateModel( &aSheetPath, *symbol ).model;

    const SIM_MODEL::PARAM* tunerParam = model.GetTunerParam();

    if( !tunerParam )
    {
        DisplayErrorMessage( this, _( "Could not apply tuned value(s):" ) + wxS( " " )
                                   + wxString::Format( _( "%s is not tunable" ), aRef ) );
        return;
    }

    model.SetParamValue( tunerParam->info.name, std::string( aValue.ToUTF8() ) );
    model.WriteFields( symbol->GetFields() );

    m_schematicFrame->UpdateItem( symbol, false, true );
    m_schematicFrame->OnModify();
}


void SIM_PLOT_FRAME::RemoveTuner( TUNER_SLIDER* aTuner, bool aErase )
{
    if( aErase )
        m_tuners.remove( aTuner );

    aTuner->Destroy();
    m_tunePanel->Layout();
}


SIM_PLOT_PANEL* SIM_PLOT_FRAME::GetCurrentPlot() const
{
    SIM_PANEL_BASE* curPage = getCurrentPlotWindow();

    return !curPage || curPage->GetType() == ST_UNKNOWN ? nullptr
                                                        : dynamic_cast<SIM_PLOT_PANEL*>( curPage );
}


const NGSPICE_CIRCUIT_MODEL* SIM_PLOT_FRAME::GetExporter() const
{
    return m_circuitModel.get();
}


void SIM_PLOT_FRAME::addTrace( const wxString& aName, SIM_TRACE_TYPE aType )
{
    SIM_TYPE simType = m_circuitModel->GetSimType();

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
    SIM_PLOT_PANEL* plotPanel = GetCurrentPlot();

    if( !plotPanel || plotPanel->GetType() != simType )
    {
        plotPanel = dynamic_cast<SIM_PLOT_PANEL*>( NewPlotPanel( m_circuitModel->GetSimCommand(),
                                                                 m_circuitModel->GetSimOptions() ) );
    }

    wxASSERT( plotPanel );

    if( !plotPanel )    // Something is wrong
        return;

    bool updated = false;
    SIM_TRACE_TYPE xAxisType = getXAxisType( simType );

    if( xAxisType == SPT_LIN_FREQUENCY || xAxisType == SPT_LOG_FREQUENCY )
    {
        int baseType = aType & ~( SPT_AC_MAG | SPT_AC_PHASE );

        // If magnitude or phase wasn't specified, then add both
        if( baseType == aType )
        {
            updated |= updateTrace( aName, (SIM_TRACE_TYPE) ( baseType | SPT_AC_MAG ), plotPanel );
            updated |= updateTrace( aName, (SIM_TRACE_TYPE) ( baseType | SPT_AC_PHASE ), plotPanel );
        }
        else
        {
            updated |= updateTrace( aName, (SIM_TRACE_TYPE) ( aType ), plotPanel );
        }
    }
    else
    {
        updated = updateTrace( aName, aType, plotPanel );
    }

    if( updated )
        updateSignalsGrid();
}


void SIM_PLOT_FRAME::removeTrace( const wxString& aSignalName )
{
    SIM_PLOT_PANEL* plotPanel = GetCurrentPlot();

    if( !plotPanel )
        return;

    wxASSERT( plotPanel->TraceShown( aSignalName ) );
    m_workbook->DeleteTrace( plotPanel, aSignalName );
    plotPanel->GetPlotWin()->Fit();

    updateSignalsGrid();
    wxCommandEvent dummy;
    onCursorUpdate( dummy );
}


bool SIM_PLOT_FRAME::updateTrace( const wxString& aName, SIM_TRACE_TYPE aType,
                                  SIM_PLOT_PANEL* aPlotPanel )
{
    SIM_TYPE simType = m_circuitModel->GetSimType();

    wxString traceTitle = aName;

    if( aType & SPT_AC_MAG )
        traceTitle += _( " (gain)" );
    else if( aType & SPT_AC_PHASE )
        traceTitle += _( " (phase)" );

    if( !SIM_PANEL_BASE::IsPlottable( simType ) )
    {
        // There is no plot to be shown
        m_simulator->Command( wxString::Format( wxT( "print %s" ), aName ).ToStdString() );

        return false;
    }

    // First, handle the x axis
    wxString xAxisName( m_simulator->GetXAxis( simType ) );

    if( xAxisName.IsEmpty() )
        return false;

    std::vector<double> data_x = m_simulator->GetMagPlot( (const char*) xAxisName.c_str() );
    unsigned int        size = data_x.size();

    std::vector<double> data_y;

    // Now, Y axis data
    switch( m_circuitModel->GetSimType() )
    {
    case ST_AC:
        wxASSERT_MSG( !( ( aType & SPT_AC_MAG ) && ( aType & SPT_AC_PHASE ) ),
                      wxT( "Cannot set both AC_PHASE and AC_MAG bits" ) );

        if( aType & SPT_AC_MAG )
            data_y = m_simulator->GetMagPlot( (const char*) aName.c_str() );
        else if( aType & SPT_AC_PHASE )
            data_y = m_simulator->GetPhasePlot( (const char*) aName.c_str() );
        else
            wxFAIL_MSG( wxT( "Plot type missing AC_PHASE or AC_MAG bit" ) );

        break;

    case ST_NOISE:
    case ST_DC:
    case ST_TRANSIENT:
        data_y = m_simulator->GetMagPlot( (const char*) aName.c_str() );
        break;

    default:
        wxFAIL_MSG( wxT( "Unhandled plot type" ) );
    }

    if( data_y.size() == 0 )
        return false;                   // Signal no longer exists
    else
        wxCHECK_MSG( data_y.size() >= size, false, wxT( "Not enough y data values to plot" ) );

    // If we did a two-source DC analysis, we need to split the resulting vector and add traces
    // for each input step
    SPICE_DC_PARAMS source1, source2;

    if( m_circuitModel->GetSimType() == ST_DC
        && m_circuitModel->ParseDCCommand( m_circuitModel->GetSimCommand(), &source1, &source2 ) )
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
                name = wxString::Format( wxT( "%s (%s = %s V)" ),
                                         traceTitle,
                                         source2.m_source,
                                         v.ToString() );

                std::vector<double> sub_x( data_x.begin() + offset,
                                           data_x.begin() + offset + inner );
                std::vector<double> sub_y( data_y.begin() + offset,
                                           data_y.begin() + offset + inner );

                m_workbook->AddTrace( aPlotPanel, name, aName, inner, sub_x.data(), sub_y.data(),
                                      aType );

                v = v + source2.m_vincrement;
                offset += inner;
            }

            return true;
        }
    }

    m_workbook->AddTrace( aPlotPanel, traceTitle, aName, size, data_x.data(), data_y.data(), aType );

    return true;
}


void SIM_PLOT_FRAME::updateSignalsGrid()
{
    SIM_PLOT_PANEL* plot = GetCurrentPlot();

    for( int row = 0; row < m_signalsGrid->GetNumberRows(); ++row )
    {
        wxString signal = m_signalsGrid->GetCellValue( row, COL_SIGNAL_NAME );

        if( TRACE* trace = plot->GetTrace( signal ) )
        {
            m_signalsGrid->SetCellValue( row, COL_SIGNAL_SHOW, wxS( "1" ) );

            wxGridCellAttr* attr = new wxGridCellAttr;
            attr->SetRenderer( new GRID_CELL_COLOR_RENDERER( this ) );
            attr->SetEditor( new GRID_CELL_COLOR_SELECTOR( this, m_signalsGrid ) );
            attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
            m_signalsGrid->SetAttr( row, COL_SIGNAL_COLOR, attr );

            KIGFX::COLOR4D color( trace->GetPen().GetColour() );
            m_signalsGrid->SetCellValue( row, COL_SIGNAL_COLOR, color.ToCSSString() );

            attr = new wxGridCellAttr;
            attr->SetRenderer( new wxGridCellBoolRenderer() );
            attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
            attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
            m_signalsGrid->SetAttr( row, COL_CURSOR_1, attr );

            attr = new wxGridCellAttr;
            attr->SetRenderer( new wxGridCellBoolRenderer() );
            attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
            attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
            m_signalsGrid->SetAttr( row, COL_CURSOR_2, attr );

            if( trace->HasCursor( 1 ) )
                m_signalsGrid->SetCellValue( row, COL_CURSOR_1, wxS( "1" ) );
            else
                m_signalsGrid->SetCellValue( row, COL_CURSOR_1, wxEmptyString );

            if( trace->HasCursor( 2 ) )
                m_signalsGrid->SetCellValue( row, COL_CURSOR_2, wxS( "1" ) );
            else
                m_signalsGrid->SetCellValue( row, COL_CURSOR_2, wxEmptyString );
        }
        else
        {
            m_signalsGrid->SetCellValue( row, COL_SIGNAL_SHOW, wxEmptyString );

            wxGridCellAttr* attr = new wxGridCellAttr;
            attr->SetReadOnly();
            m_signalsGrid->SetAttr( row, COL_SIGNAL_COLOR, attr );
            m_signalsGrid->SetCellValue( row, COL_SIGNAL_COLOR, wxEmptyString );

            attr = new wxGridCellAttr;
            attr->SetReadOnly();
            m_signalsGrid->SetAttr( row, COL_CURSOR_1, attr );
            m_signalsGrid->SetCellValue( row, COL_CURSOR_1, wxEmptyString );

            attr = new wxGridCellAttr;
            attr->SetReadOnly();
            m_signalsGrid->SetAttr( row, COL_CURSOR_2, attr );
            m_signalsGrid->SetCellValue( row, COL_CURSOR_2, wxEmptyString );
        }
    }
}


void SIM_PLOT_FRAME::applyTuners()
{
    wxString            errors;
    WX_STRING_REPORTER  reporter( &errors );

    for( const TUNER_SLIDER* tuner : m_tuners )
    {
        SCH_SHEET_PATH sheetPath;
        wxString       ref = tuner->GetSymbolRef();
        KIID           symbolId = tuner->GetSymbol( &sheetPath );
        SCH_ITEM*      schItem = sheetPath.GetItem( symbolId );
        SCH_SYMBOL*    symbol = dynamic_cast<SCH_SYMBOL*>( schItem );

        if( !symbol )
        {
            reporter.Report( wxString::Format( _( "%s not found" ), ref ) );
            continue;
        }

        const SPICE_ITEM* item = GetExporter()->FindItem( tuner->GetSymbolRef().ToStdString() );

        if( !item || !item->model->GetTunerParam() )
        {
            reporter.Report( wxString::Format( _( "%s is not tunable" ), ref ) );
            continue;
        }

        SIM_VALUE_FLOAT floatVal( tuner->GetValue().ToDouble() );

        m_simulator->Command( item->model->SpiceGenerator().TunerCommand( *item, floatVal ) );
    }

    if( reporter.HasMessage() )
        DisplayErrorMessage( this, _( "Could not apply tuned value(s):" ) + wxS( "\n" ) + errors );
}


bool SIM_PLOT_FRAME::LoadWorkbook( const wxString& aPath )
{
    m_workbook->DeleteAllPages();

    wxTextFile file( aPath );

#define DISPLAY_LOAD_ERROR( fmt ) DisplayErrorMessage( this, wxString::Format( _( fmt ), \
            file.GetCurrentLine()+1 ) )

    if( !file.Open() )
        return false;

    long     version = 1;
    wxString firstLine = file.GetFirstLine();
    wxString plotCountLine;

    if( firstLine.StartsWith( wxT( "version " ) ) )
    {
        if( !firstLine.substr( 8 ).ToLong( &version ) )
        {
            DISPLAY_LOAD_ERROR( "Error loading workbook: Line %d is not an integer." );
            file.Close();

            return false;
        }

        plotCountLine = file.GetNextLine();
    }
    else
    {
        plotCountLine = firstLine;
    }

    long plotsCount;

    if( !plotCountLine.ToLong( &plotsCount ) ) // GetFirstLine instead of GetNextLine
    {
        DISPLAY_LOAD_ERROR( "Error loading workbook: Line %d is not an integer." );
        file.Close();

        return false;
    }

    for( long i = 0; i < plotsCount; ++i )
    {
        long plotType, tracesCount;

        if( !file.GetNextLine().ToLong( &plotType ) )
        {
            DISPLAY_LOAD_ERROR( "Error loading workbook: Line %d is not an integer." );
            file.Close();

            return false;
        }

        wxString          command = UnescapeString( file.GetNextLine() );
        wxString          simCommand;
        int               simOptions = NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS;
        wxStringTokenizer tokenizer( command, wxT( "\r\n" ), wxTOKEN_STRTOK );

        if( version >= 2 )
        {
            simOptions &= ~NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS;
            simOptions &= ~NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES;
            simOptions &= ~NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS;
        }

        while( tokenizer.HasMoreTokens() )
        {
            wxString line = tokenizer.GetNextToken();

            if( line.StartsWith( wxT( ".kicad adjustpaths" ) ) )
                simOptions |= NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS;
            else if( line.StartsWith( wxT( ".save all" ) ) )
                simOptions |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES;
            else if( line.StartsWith( wxT( ".probe alli" ) ) )
                simOptions |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS;
            else
                simCommand += line + wxT( "\n" );
        }

        NewPlotPanel( simCommand, simOptions );
        StartSimulation( simCommand );

        // Perform simulation, so plots can be added with values
        do
        {
            wxThread::This()->Sleep( 50 );
        }
        while( m_simulator->IsRunning() );

        if( !file.GetNextLine().ToLong( &tracesCount ) )
        {
            DISPLAY_LOAD_ERROR( "Error loading workbook: Line %d is not an integer." );
            file.Close();

            return false;
        }

        for( long j = 0; j < tracesCount; ++j )
        {
            long traceType;
            wxString name, param;

            if( !file.GetNextLine().ToLong( &traceType ) )
            {
                DISPLAY_LOAD_ERROR( "Error loading workbook: Line %d is not an integer." );
                file.Close();

                return false;
            }

            name = file.GetNextLine();

            if( name.IsEmpty() )
            {
                DISPLAY_LOAD_ERROR( "Error loading workbook: Line %d is empty." );
                file.Close();

                return false;
            }

            param = file.GetNextLine();

            #if 0   // no longer in use
            if( param.IsEmpty() )
            {
                DISPLAY_LOAD_ERROR( "Error loading workbook: Line %d is empty." );
                file.Close();

                return false;
            }
            #endif

            addTrace( name, (SIM_TRACE_TYPE) traceType );
        }
    }

    file.Close();

    wxFileName filename( aPath );
    filename.MakeRelativeTo( Prj().GetProjectPath() );

    // Remember the loaded workbook filename.
    m_simulator->Settings()->SetWorkbookFilename( filename.GetFullPath() );

    // Successfully loading a workbook does not count as modifying it.
    m_workbook->ClrModified();
    return true;
}


bool SIM_PLOT_FRAME::SaveWorkbook( const wxString& aPath )
{
    wxFileName filename = aPath;
    filename.SetExt( WorkbookFileExtension );

    wxTextFile file( filename.GetFullPath() );

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

    file.AddLine( wxT( "version 2" ) );

    file.AddLine( wxString::Format( wxT( "%llu" ), m_workbook->GetPageCount() ) );

    for( size_t i = 0; i < m_workbook->GetPageCount(); i++ )
    {
        const SIM_PANEL_BASE* basePanel = dynamic_cast<const SIM_PANEL_BASE*>( m_workbook->GetPage( i ) );

        if( !basePanel )
        {
            file.AddLine( wxString::Format( wxT( "%llu" ), 0ull ) );
            continue;
        }

        file.AddLine( wxString::Format( wxT( "%d" ), basePanel->GetType() ) );

        wxString command = m_workbook->GetSimCommand( basePanel );
        int      options = m_workbook->GetSimOptions( basePanel );

        if( options & NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS )
            command += wxT( "\n.kicad adjustpaths" );

        if( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES )
            command += wxT( "\n.save all" );

        if( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS )
            command += wxT( "\n.probe alli" );

        file.AddLine( EscapeString( command, CTX_LINE ) );

        const SIM_PLOT_PANEL* plotPanel = dynamic_cast<const SIM_PLOT_PANEL*>( basePanel );

        if( !plotPanel )
        {
            file.AddLine( wxString::Format( wxT( "%llu" ), 0ull ) );
            continue;
        }

        file.AddLine( wxString::Format( wxT( "%llu" ), plotPanel->GetTraces().size() ) );

        for( const auto& [name, trace] : plotPanel->GetTraces() )
        {
            file.AddLine( wxString::Format( wxT( "%d" ), trace->GetType() ) );
            file.AddLine( trace->GetName() );
            file.AddLine( trace->GetParam().IsEmpty() ? wxS( " " ) : trace->GetParam() );
        }
    }

    bool res = file.Write();
    file.Close();

    // Store the filename of the last saved workbook.
    if( res )
    {
        filename.MakeRelativeTo( Prj().GetProjectPath() );
        m_simulator->Settings()->SetWorkbookFilename( filename.GetFullPath() );
    }

    m_workbook->ClrModified();
    return res;
}


SIM_TRACE_TYPE SIM_PLOT_FRAME::getXAxisType( SIM_TYPE aType ) const
{
    switch( aType )
    {
        /// @todo SPT_LOG_FREQUENCY
        case ST_AC:        return SPT_LIN_FREQUENCY;
        case ST_DC:        return SPT_SWEEP;
        case ST_TRANSIENT: return SPT_TIME;
        default:
            wxASSERT_MSG( false, wxT( "Unhandled simulation type" ) );
            return (SIM_TRACE_TYPE) 0;
    }
}


void SIM_PLOT_FRAME::ToggleDarkModePlots()
{
    m_darkMode = !m_darkMode;

    // Rebuild the color list to plot traces
    SIM_PLOT_COLORS::FillDefaultColorList( m_darkMode );

    // Now send changes to all SIM_PLOT_PANEL
    for( size_t page = 0; page < m_workbook->GetPageCount(); page++ )
    {
        wxWindow* curPage = m_workbook->GetPage( page );

        // ensure it is truly a plot panel and not the (zero plots) placeholder
        // which is only SIM_PLOT_PANEL_BASE
        SIM_PLOT_PANEL* panel = dynamic_cast<SIM_PLOT_PANEL*>( curPage );

        if( panel )
            panel->UpdatePlotColors();
    }
}


void SIM_PLOT_FRAME::onPlotClose( wxAuiNotebookEvent& event )
{
}


void SIM_PLOT_FRAME::onPlotClosed( wxAuiNotebookEvent& event )
{
    m_signals.clear();
    rebuildSignalsGrid( m_filter->GetValue() );

    wxCommandEvent dummy;
    onCursorUpdate( dummy );
}


void SIM_PLOT_FRAME::onPlotChanged( wxAuiNotebookEvent& event )
{
    rebuildSignalsGrid( m_filter->GetValue() );
    wxCommandEvent dummy;
    onCursorUpdate( dummy );
}


void SIM_PLOT_FRAME::onPlotDragged( wxAuiNotebookEvent& event )
{
}


void SIM_PLOT_FRAME::onWorkbookModified( wxCommandEvent& event )
{
    updateTitle();
}


void SIM_PLOT_FRAME::onWorkbookClrModified( wxCommandEvent& event )
{
    updateTitle();
}


bool SIM_PLOT_FRAME::EditSimCommand()
{
    SIM_PANEL_BASE*     plotPanelWindow = getCurrentPlotWindow();
    DIALOG_SIM_COMMAND  dlg( this, m_circuitModel, m_simulator->Settings() );
    wxString            errors;
    WX_STRING_REPORTER  reporter( &errors );

    if( !m_circuitModel->ReadSchematicAndLibraries( NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS,
                                                    reporter ) )
    {
        DisplayErrorMessage( this, _( "Errors during netlist generation; simulation aborted.\n\n" )
                                   + errors );
        return false;
    }

    if( m_workbook->GetPageIndex( plotPanelWindow ) != wxNOT_FOUND )
    {
        dlg.SetSimCommand( m_workbook->GetSimCommand( plotPanelWindow ) );
        dlg.SetSimOptions( m_workbook->GetSimOptions( plotPanelWindow ) );
    }
    else
    {
        dlg.SetSimOptions( NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS );
    }

    if( dlg.ShowModal() == wxID_OK )
    {
        wxString oldCommand;

        if( m_workbook->GetPageIndex( plotPanelWindow ) != wxNOT_FOUND )
            oldCommand = m_workbook->GetSimCommand( plotPanelWindow );
        else
            oldCommand = wxString();

        const wxString& newCommand = dlg.GetSimCommand();
        int             newOptions = dlg.GetSimOptions();
        SIM_TYPE        newSimType = NGSPICE_CIRCUIT_MODEL::CommandToSimType( newCommand );

        if( !plotPanelWindow )
        {
            m_circuitModel->SetSimCommandOverride( newCommand );
            m_circuitModel->SetSimOptions( newOptions );
            plotPanelWindow = NewPlotPanel( newCommand, newOptions );
        }
        // If it is a new simulation type, open a new plot.  For the DC sim, check if sweep
        // source type has changed (char 4 will contain 'v', 'i', 'r' or 't'.
        else if( plotPanelWindow->GetType() != newSimType
                    || ( newSimType == ST_DC
                         && oldCommand.Lower().GetChar( 4 ) != newCommand.Lower().GetChar( 4 ) ) )
        {
            plotPanelWindow = NewPlotPanel( newCommand, newOptions );
        }
        else
        {
            if( m_workbook->GetPageIndex( plotPanelWindow ) == 0 )
                m_circuitModel->SetSimCommandOverride( newCommand );

            // Update simulation command in the current plot
            m_workbook->SetSimCommand( plotPanelWindow, newCommand );
            m_workbook->SetSimOptions( plotPanelWindow, newOptions );
        }

        m_simulator->Init();
        return true;
    }

    return false;
}


bool SIM_PLOT_FRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    if( m_workbook->IsModified() )
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


void SIM_PLOT_FRAME::doCloseWindow()
{
    if( m_simulator->IsRunning() )
        m_simulator->Stop();

    // Prevent memory leak on exit by deleting all simulation vectors
    m_simulator->Clean();

    // Cancel a running simProbe or simTune tool
    m_schematicFrame->GetToolManager()->RunAction( ACTIONS::cancelInteractive );

    SaveSettings( config() );

    m_simulator->Settings() = nullptr;

    Destroy();
}


void SIM_PLOT_FRAME::onCursorUpdate( wxCommandEvent& event )
{
    m_cursorsGrid->ClearRows();

    SIM_PLOT_PANEL* plotPanel = GetCurrentPlot();

    if( !plotPanel )
        return;

    // Set up the labels
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_X, plotPanel->GetLabelX() );

    wxString labelY1 = plotPanel->GetLabelY1();
    wxString labelY2 = plotPanel->GetLabelY2();

    if( labelY2.IsEmpty() )
        m_cursorsGrid->SetColLabelValue( COL_CURSOR_Y, labelY1 );
    else
        m_cursorsGrid->SetColLabelValue( COL_CURSOR_Y, labelY1 + wxT( " / " ) + labelY2 );

    // Update cursor values
    wxString unitsX = plotPanel->GetUnitsX();
    CURSOR*  cursor1 = nullptr;
    wxString unitsY1;
    CURSOR*  cursor2 = nullptr;
    wxString unitsY2;

    auto getUnitsY =
            [&]( TRACE* aTrace ) -> wxString
            {
                if( ( aTrace->GetType() & SPT_AC_PHASE ) || ( aTrace->GetType() & SPT_CURRENT ) )
                    return plotPanel->GetUnitsY2();
                else
                    return plotPanel->GetUnitsY1();
            };

    auto updateRangeUnits =
            []( wxString* aRange, const wxString& aUnits )
            {
                if( aRange->GetChar( 0 ) == '~' )
                    *aRange = aRange->Left( 1 ) + aUnits;
                else if( SPICE_VALUE::ParseSIPrefix( aRange->GetChar( 0 ) ) != SPICE_VALUE::PFX_NONE )
                    *aRange = aRange->Left( 1 ) + aUnits;
                else
                    *aRange = aUnits;
            };

    auto formatValue =
            [this]( double aValue, int aCursorId, int aCol ) -> wxString
            {
                return SPICE_VALUE( aValue ).ToString( m_cursorPrecision[ aCursorId ][ aCol ],
                                                       m_cursorRange[ aCursorId ][ aCol ] );
            };

    for( const auto& [name, trace] : plotPanel->GetTraces() )
    {
        if( CURSOR* cursor = trace->GetCursor( 1 ) )
        {
            cursor1 = cursor;
            unitsY1 = getUnitsY( trace );

            wxRealPoint coords = cursor->GetCoords();
            int         row = m_cursorsGrid->GetNumberRows();

            updateRangeUnits( &m_cursorRange[0][0], unitsX );
            updateRangeUnits( &m_cursorRange[0][1], unitsY1 );

            m_cursorsGrid->AppendRows( 1 );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_NAME, wxS( "1" ) );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_SIGNAL, cursor->GetName() );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_X, formatValue( coords.x, 0, 0 ) );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_Y, formatValue( coords.y, 0, 1 ) );
            break;
        }
    }

    for( const auto& [name, trace] : plotPanel->GetTraces() )
    {
        if( CURSOR* cursor = trace->GetCursor( 2 ) )
        {
            cursor2 = cursor;
            unitsY2 = getUnitsY( trace );

            wxRealPoint coords = cursor->GetCoords();
            int         row = m_cursorsGrid->GetNumberRows();

            updateRangeUnits( &m_cursorRange[1][0], unitsX );
            updateRangeUnits( &m_cursorRange[1][1], unitsY2 );

            m_cursorsGrid->AppendRows( 1 );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_NAME, wxS( "2" ) );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_SIGNAL, cursor->GetName() );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_X, formatValue( coords.x, 1, 0 ) );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_Y, formatValue( coords.y, 1, 1 ) );
            break;
        }
    }

    if( cursor1 && cursor2 && unitsY1 == unitsY2 )
    {
        wxRealPoint coords = cursor2->GetCoords() - cursor1->GetCoords();
        wxString    signal;

        updateRangeUnits( &m_cursorRange[2][0], unitsX );
        updateRangeUnits( &m_cursorRange[2][1], unitsY1 );

        if( cursor1->GetName() == cursor2->GetName() )
            signal = wxString::Format( wxS( "%s[2 - 1]" ), cursor2->GetName() );
        else
            signal = wxString::Format( wxS( "%s - %s" ), cursor2->GetName(), cursor1->GetName() );

        m_cursorsGrid->AppendRows( 1 );
        m_cursorsGrid->SetCellValue( 2, COL_CURSOR_NAME, _( "Diff" ) );
        m_cursorsGrid->SetCellValue( 2, COL_CURSOR_SIGNAL, signal );
        m_cursorsGrid->SetCellValue( 2, COL_CURSOR_X, formatValue( coords.x, 2, 0 ) );
        m_cursorsGrid->SetCellValue( 2, COL_CURSOR_Y, formatValue( coords.y, 2, 1 ) );
    }
}


void SIM_PLOT_FRAME::setupUIConditions()
{
    EDA_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER*   mgr = m_toolManager->GetActionManager();
    wxASSERT( mgr );

    auto showGridCondition =
            [this]( const SELECTION& aSel )
            {
                SIM_PLOT_PANEL* plot = GetCurrentPlot();
                return plot && plot->IsGridShown();
            };

    auto showLegendCondition =
            [this]( const SELECTION& aSel )
            {
                SIM_PLOT_PANEL* plot = GetCurrentPlot();
                return plot && plot->IsLegendShown();
            };

    auto showDottedCondition =
            [this]( const SELECTION& aSel )
            {
                SIM_PLOT_PANEL* plot = GetCurrentPlot();
                return plot && plot->GetDottedSecondary();
            };

    auto darkModePlotCondition =
            [this]( const SELECTION& aSel )
            {
                return m_darkMode;
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

    auto havePlot =
            [this]( const SELECTION& aSel )
            {
                return GetCurrentPlot() != nullptr;
            };

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )
#define CHECK( x )  ACTION_CONDITIONS().Check( x )

    mgr->SetConditions( EE_ACTIONS::openWorkbook,          ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( EE_ACTIONS::saveWorkbook,          ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( EE_ACTIONS::saveWorkbookAs,        ENABLE( SELECTION_CONDITIONS::ShowAlways ) );

    mgr->SetConditions( EE_ACTIONS::exportPlotAsPNG,       ENABLE( havePlot ) );
    mgr->SetConditions( EE_ACTIONS::exportPlotAsCSV,       ENABLE( havePlot ) );

    mgr->SetConditions( EE_ACTIONS::toggleGrid,            CHECK( showGridCondition ) );
    mgr->SetConditions( EE_ACTIONS::toggleLegend,          CHECK( showLegendCondition ) );
    mgr->SetConditions( EE_ACTIONS::toggleDottedSecondary, CHECK( showDottedCondition ) );
    mgr->SetConditions( EE_ACTIONS::toggleDarkModePlots,   CHECK( darkModePlotCondition ) );

    mgr->SetConditions( EE_ACTIONS::simCommand,            ENABLE( SELECTION_CONDITIONS::ShowAlways ) );
    mgr->SetConditions( EE_ACTIONS::runSimulation,         ENABLE( !simRunning ) );
    mgr->SetConditions( EE_ACTIONS::stopSimulation,        ENABLE( simRunning ) );
    mgr->SetConditions( EE_ACTIONS::simProbe,              ENABLE( simFinished ) );
    mgr->SetConditions( EE_ACTIONS::simTune,               ENABLE( simFinished ) );
    mgr->SetConditions( EE_ACTIONS::showNetlist,           ENABLE( SELECTION_CONDITIONS::ShowAlways ) );

#undef CHECK
#undef ENABLE
}


void SIM_PLOT_FRAME::onSimStarted( wxCommandEvent& aEvent )
{
    SetCursor( wxCURSOR_ARROWWAIT );
}


void SIM_PLOT_FRAME::onSimFinished( wxCommandEvent& aEvent )
{
    SetCursor( wxCURSOR_ARROW );

    SIM_TYPE simType = m_circuitModel->GetSimType();

    if( simType == ST_UNKNOWN )
        return;

    SIM_PANEL_BASE* plotPanelWindow = getCurrentPlotWindow();

    if( !plotPanelWindow || plotPanelWindow->GetType() != simType )
    {
        plotPanelWindow = NewPlotPanel( m_circuitModel->GetSimCommand(),
                                        m_circuitModel->GetSimOptions() );
    }

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
    // Is a warning message useful if the simulatior is still running?

    // If there are any signals plotted, update them
    if( SIM_PANEL_BASE::IsPlottable( simType ) )
    {
        SIM_PLOT_PANEL* plotPanel = dynamic_cast<SIM_PLOT_PANEL*>( plotPanelWindow );
        wxCHECK_RET( plotPanel, wxT( "not a SIM_PLOT_PANEL" ) );

        struct TRACE_DESC
        {
            wxString      m_name;    ///< Name of the measured net/device
            SIM_TRACE_TYPE m_type;    ///< Type of the signal
        };

        std::vector<struct TRACE_DESC> traceInfo;

        // Get information about all the traces on the plot, remove and add again
        for( const auto& [name, trace] : plotPanel->GetTraces() )
        {
            struct TRACE_DESC placeholder;
            placeholder.m_name = trace->GetName();
            placeholder.m_type = trace->GetType();

            traceInfo.push_back( placeholder );
        }

        for( const struct TRACE_DESC& trace : traceInfo )
        {
            if( !updateTrace( trace.m_name, trace.m_type, plotPanel ) )
                removeTrace( trace.m_name );
        }

        rebuildSignalsGrid( m_filter->GetValue() );
        plotPanel->GetPlotWin()->UpdateAll();
        plotPanel->ResetScales();
    }
    else if( simType == ST_OP )
    {
        SCHEMATIC& schematic = m_schematicFrame->Schematic();
        schematic.ClearOperatingPoints();

        m_simConsole->AppendText( _( "\n\nSimulation results:\n\n" ) );
        m_simConsole->SetInsertionPointEnd();

        for( const std::string& vec : m_simulator->AllPlots() )
        {
            std::vector<double> val_list = m_simulator->GetRealPlot( vec, 1 );

            if( val_list.size() == 0 )      // The list of values can be empty!
                continue;

            wxString       value = SPICE_VALUE( val_list.at( 0 ) ).ToSpiceString();
            wxString       msg;
            wxString       signal;
            SIM_TRACE_TYPE type = m_circuitModel->VectorToSignal( vec, signal );

            const size_t   tab = 25; //characters
            size_t         padding = ( signal.length() < tab ) ? ( tab - signal.length() ) : 1;

            value.Append( type == SPT_CURRENT ? wxS( "A" ) : wxS( "V" ) );

            msg.Printf( wxT( "%s%s\n" ),
                        ( signal + wxT( ":" ) ).Pad( padding, wxUniChar( ' ' ) ),
                        value );

            m_simConsole->AppendText( msg );
            m_simConsole->SetInsertionPointEnd();

            if( signal.StartsWith( wxS( "V(" ) ) || signal.StartsWith( wxS( "I(" ) ) )
                signal = signal.SubString( 2, signal.Length() - 2 );

            schematic.SetOperatingPoint( signal, val_list.at( 0 ) );
        }

        m_schematicFrame->RefreshOperatingPointDisplay();
    }

    m_lastSimPlot = plotPanelWindow;
    m_simFinished = true;
}


void SIM_PLOT_FRAME::onSimUpdate( wxCommandEvent& aEvent )
{
    static bool updateInProgress = false;

    // skip update when events are triggered too often and previous call didn't end yet
    if( updateInProgress )
        return;

    updateInProgress = true;

    if( m_simulator->IsRunning() )
        m_simulator->Stop();

    if( getCurrentPlotWindow() != m_lastSimPlot )
    {
        // We need to rerun simulation, as the simulator currently stores
        // results for another plot
        StartSimulation();
    }
    else
    {
        std::unique_lock<std::mutex> simulatorLock( m_simulator->GetMutex(), std::try_to_lock );

        if( simulatorLock.owns_lock() )
        {
            // Incremental update
            m_simConsole->Clear();

            // Do not export netlist, it is already stored in the simulator
            applyTuners();

            m_simulator->Run();
        }
        else
        {
            DisplayErrorMessage( this, _( "Another simulation is already running." ) );
        }
    }
    updateInProgress = false;
}


void SIM_PLOT_FRAME::onSimReport( wxCommandEvent& aEvent )
{
    m_simConsole->AppendText( aEvent.GetString() + "\n" );
    m_simConsole->SetInsertionPointEnd();
}


void SIM_PLOT_FRAME::onExit( wxCommandEvent& event )
{
    Kiway().OnKiCadExit();
}


wxDEFINE_EVENT( EVT_SIM_UPDATE, wxCommandEvent );
wxDEFINE_EVENT( EVT_SIM_REPORT, wxCommandEvent );

wxDEFINE_EVENT( EVT_SIM_STARTED, wxCommandEvent );
wxDEFINE_EVENT( EVT_SIM_FINISHED, wxCommandEvent );
