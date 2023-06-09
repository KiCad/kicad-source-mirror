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
#include "simulator_frame.h"
#include "sim_plot_panel.h"
#include "spice_simulator.h"
#include "spice_reporter.h"
#include "core/kicad_algo.h"
#include "fmt/format.h"
#include <dialog_sim_format_value.h>
#include <eeschema_settings.h>
#include <advanced_config.h>

#include <memory>


SIM_TRACE_TYPE operator|( SIM_TRACE_TYPE aFirst, SIM_TRACE_TYPE aSecond )
{
    int res = (int) aFirst | (int) aSecond;

    return (SIM_TRACE_TYPE) res;
}


class SIM_THREAD_REPORTER : public SPICE_REPORTER
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
    SIMULATOR_FRAME* m_parent;
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


enum MEASUREMENTS_GIRD_COLUMNS
{
    COL_MEASUREMENT = 0,
    COL_MEASUREMENT_VALUE,
    COL_MEASUREMENT_FORMAT
};


enum
{
    MYID_MEASURE_MIN = GRIDTRICKS_FIRST_CLIENT_ID,
    MYID_MEASURE_MAX,
    MYID_MEASURE_AVG,
    MYID_MEASURE_RMS,
    MYID_MEASURE_PP,
    MYID_MEASURE_MIN_AT,
    MYID_MEASURE_MAX_AT,
    MYID_MEASURE_INTEGRAL,

    MYID_FORMAT_VALUE,
    MYID_DELETE_MEASUREMENT
};


class SIGNALS_GRID_TRICKS : public GRID_TRICKS
{
public:
    SIGNALS_GRID_TRICKS( SIMULATOR_FRAME* aParent, WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid ),
            m_parent( aParent ),
            m_menuRow( 0 ),
            m_menuCol( 0 )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    SIMULATOR_FRAME* m_parent;
    int             m_menuRow;
    int             m_menuCol;
};


void SIGNALS_GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    m_menuRow = aEvent.GetRow();
    m_menuCol = aEvent.GetCol();

    if( m_menuCol == COL_SIGNAL_NAME )
    {
        if( !( m_grid->IsInSelection( m_menuRow, m_menuCol ) ) )
            m_grid->ClearSelection();

        m_grid->SetGridCursor( m_menuRow, m_menuCol );

        wxString msg = m_grid->GetCellValue( m_menuRow, m_menuCol );

        menu.Append( MYID_MEASURE_MIN, _( "Measure Min" ) );
        menu.Append( MYID_MEASURE_MAX, _( "Measure Max" ) );
        menu.Append( MYID_MEASURE_AVG, _( "Measure Average" ) );
        menu.Append( MYID_MEASURE_RMS, _( "Measure RMS" ) );
        menu.Append( MYID_MEASURE_PP, _( "Measure Peak-to-peak" ) );
        menu.Append( MYID_MEASURE_MIN_AT, _( "Measure Time of Min" ) );
        menu.Append( MYID_MEASURE_MAX_AT, _( "Measure Time of Max" ) );
        menu.Append( MYID_MEASURE_INTEGRAL, _( "Measure Integral" ) );

        menu.AppendSeparator();
    }

    GRID_TRICKS::showPopupMenu( menu, aEvent );
}


void SIGNALS_GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    std::vector<wxString> signals;

    wxGridCellCoordsArray cells1 = m_grid->GetSelectionBlockTopLeft();
    wxGridCellCoordsArray cells2 = m_grid->GetSelectionBlockBottomRight();

    for( size_t i = 0; i < cells1.Count(); i++ )
    {
        if( cells1[i].GetCol() == COL_SIGNAL_NAME )
        {
            for( int j = cells1[i].GetRow(); j < cells2[i].GetRow() + 1; j++ )
            {
                signals.push_back( m_grid->GetCellValue( j, cells1[i].GetCol() ) );
            }
        }
    }

    wxGridCellCoordsArray cells3 = m_grid->GetSelectedCells();

    for( size_t i = 0; i < cells3.Count(); i++ )
    {
        if( cells3[i].GetCol() == COL_SIGNAL_NAME )
            signals.push_back( m_grid->GetCellValue( cells3[i].GetRow(), cells3[i].GetCol() ) );
    }

    if( signals.size() < 1 )
        signals.push_back( m_grid->GetCellValue( m_menuRow, m_menuCol ) );

    if( event.GetId() == MYID_MEASURE_MIN )
    {
        for( const wxString& signal : signals )
            m_parent->AddMeasurement( wxString::Format( wxS( "MIN %s" ), signal ) );
    }
    else if( event.GetId() == MYID_MEASURE_MAX )
    {
        for( const wxString& signal : signals )
            m_parent->AddMeasurement( wxString::Format( wxS( "MAX %s" ), signal ) );
    }
    else if( event.GetId() == MYID_MEASURE_AVG )
    {
        for( const wxString& signal : signals )
            m_parent->AddMeasurement( wxString::Format( wxS( "AVG %s" ), signal ) );
    }
    else if( event.GetId() == MYID_MEASURE_RMS )
    {
        for( const wxString& signal : signals )
            m_parent->AddMeasurement( wxString::Format( wxS( "RMS %s" ), signal ) );
    }
    else if( event.GetId() == MYID_MEASURE_PP )
    {
        for( const wxString& signal : signals )
            m_parent->AddMeasurement( wxString::Format( wxS( "PP %s" ), signal ) );
    }
    else if( event.GetId() == MYID_MEASURE_MIN_AT )
    {
        for( const wxString& signal : signals )
            m_parent->AddMeasurement( wxString::Format( wxS( "MIN_AT %s" ), signal ) );
    }
    else if( event.GetId() == MYID_MEASURE_MAX_AT )
    {
        for( const wxString& signal : signals )
            m_parent->AddMeasurement( wxString::Format( wxS( "MAX_AT %s" ), signal ) );
    }
    else if( event.GetId() == MYID_MEASURE_INTEGRAL )
    {
        for( const wxString& signal : signals )
            m_parent->AddMeasurement( wxString::Format( wxS( "INTEG %s" ), signal ) );
    }
    else
        GRID_TRICKS::doPopupSelection( event );
}


class CURSORS_GRID_TRICKS : public GRID_TRICKS
{
public:
    CURSORS_GRID_TRICKS( SIMULATOR_FRAME* aParent, WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid ),
            m_parent( aParent ),
            m_menuRow( 0 ),
            m_menuCol( 0 )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    SIMULATOR_FRAME* m_parent;
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

        menu.Append( MYID_FORMAT_VALUE, wxString::Format( _( "Format %s..." ), msg ) );
        menu.AppendSeparator();
    }

    GRID_TRICKS::showPopupMenu( menu, aEvent );
}


void CURSORS_GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    if( event.GetId() == MYID_FORMAT_VALUE )
    {
        int                     cursorId = m_menuRow;
        int                     cursorAxis = m_menuCol - COL_CURSOR_X;
        SPICE_VALUE_FORMAT      format = m_parent->GetCursorFormat( cursorId, cursorAxis );
        DIALOG_SIM_FORMAT_VALUE formatDialog( m_parent, &format );

        if( formatDialog.ShowModal() == wxID_OK )
            m_parent->SetCursorFormat( cursorId, cursorAxis, format );
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


class MEASUREMENTS_GRID_TRICKS : public GRID_TRICKS
{
public:
    MEASUREMENTS_GRID_TRICKS( SIMULATOR_FRAME* aParent, WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid ),
            m_parent( aParent ),
            m_menuRow( 0 ),
            m_menuCol( 0 )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    SIMULATOR_FRAME* m_parent;
    int             m_menuRow;
    int             m_menuCol;
};


void MEASUREMENTS_GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    m_menuRow = aEvent.GetRow();
    m_menuCol = aEvent.GetCol();

    if( !( m_grid->IsInSelection( m_menuRow, m_menuCol ) ) )
        m_grid->ClearSelection();

    m_grid->SetGridCursor( m_menuRow, m_menuCol );

    if( m_menuCol == COL_MEASUREMENT_VALUE )
        menu.Append( MYID_FORMAT_VALUE, _( "Format Value..." ) );

    if( m_menuRow < ( m_grid->GetNumberRows() - 1 ) )
        menu.Append( MYID_DELETE_MEASUREMENT, _( "Delete Measurement" ) );

    menu.AppendSeparator();

    GRID_TRICKS::showPopupMenu( menu, aEvent );
}


void MEASUREMENTS_GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    if( event.GetId() == MYID_FORMAT_VALUE )
    {
        SPICE_VALUE_FORMAT      format = m_parent->GetMeasureFormat( m_menuRow );
        DIALOG_SIM_FORMAT_VALUE formatDialog( m_parent, &format );

        if( formatDialog.ShowModal() == wxID_OK )
        {
            m_parent->SetMeasureFormat( m_menuRow, format );
            m_parent->UpdateMeasurement( m_menuRow );
        }
    }
    else if( event.GetId() == MYID_DELETE_MEASUREMENT )
    {
        std::vector<int> measurements;

        wxGridCellCoordsArray cells1 = m_grid->GetSelectionBlockTopLeft();
        wxGridCellCoordsArray cells2 = m_grid->GetSelectionBlockBottomRight();

        for( size_t i = 0; i < cells1.Count(); i++ )
        {
            if( cells1[i].GetCol() == COL_MEASUREMENT )
            {
                for( int j = cells1[i].GetRow(); j < cells2[i].GetRow() + 1; j++ )
                {
                    measurements.push_back( j );
                }
            }
        }

        wxGridCellCoordsArray cells3 = m_grid->GetSelectedCells();

        for( size_t i = 0; i < cells3.Count(); i++ )
        {
            if( cells3[i].GetCol() == COL_MEASUREMENT )
                measurements.push_back( cells3[i].GetRow() );
        }

        if( measurements.size() < 1 )
            measurements.push_back( m_menuRow );

        // When deleting a row, we'll change the indexes.
        // To avoid problems, we can start with the highest indexes.
        sort( measurements.begin(), measurements.end(), std::greater<>() );

        for( int row : measurements )
            m_parent->DeleteMeasurement( row );

        m_grid->ClearSelection();
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


class SUPPRESS_GRID_CELL_EVENTS
{
public:
    SUPPRESS_GRID_CELL_EVENTS( SIMULATOR_FRAME* aFrame ) :
            m_frame( aFrame )
    {
        m_frame->m_SuppressGridEvents++;
    }

    ~SUPPRESS_GRID_CELL_EVENTS()
    {
        m_frame->m_SuppressGridEvents--;
    }

private:
    SIMULATOR_FRAME* m_frame;
};


BEGIN_EVENT_TABLE( SIMULATOR_FRAME, SIMULATOR_FRAME_BASE )
    EVT_MENU( wxID_EXIT, SIMULATOR_FRAME::onExit )
    EVT_MENU( wxID_CLOSE, SIMULATOR_FRAME::onExit )
END_EVENT_TABLE()


SIMULATOR_FRAME::SIMULATOR_FRAME( KIWAY* aKiway, wxWindow* aParent ) :
        SIMULATOR_FRAME_BASE( aParent ),
        m_SuppressGridEvents( 0 ),
        m_lastSimPlot( nullptr ),
        m_darkMode( true ),
        m_plotNumber( 0 ),
        m_simFinished( false ),
        m_workbookModified( false )
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

    m_filter->SetHint( _( "Filter" ) );

    m_signalsGrid->wxGrid::SetLabelFont( KIUI::GetStatusFont( this ) );
    m_cursorsGrid->wxGrid::SetLabelFont( KIUI::GetStatusFont( this ) );
    m_measurementsGrid->wxGrid::SetLabelFont( KIUI::GetStatusFont( this ) );

    m_signalsGrid->PushEventHandler( new SIGNALS_GRID_TRICKS( this, m_signalsGrid ) );
    m_cursorsGrid->PushEventHandler( new CURSORS_GRID_TRICKS( this, m_cursorsGrid ) );
    m_measurementsGrid->PushEventHandler( new MEASUREMENTS_GRID_TRICKS( this, m_measurementsGrid ) );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_signalsGrid->SetColAttr( COL_SIGNAL_NAME, attr );

    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_cursorsGrid->SetColAttr( COL_CURSOR_NAME, attr );

    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_cursorsGrid->SetColAttr( COL_CURSOR_SIGNAL, attr );

    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_cursorsGrid->SetColAttr( COL_CURSOR_Y, attr );

    for( int cursorId = 0; cursorId < 3; ++cursorId )
    {
        m_cursorFormats[ cursorId ][ 0 ] = { 3, wxS( "~s" ) };
        m_cursorFormats[ cursorId ][ 1 ] = { 3, wxS( "~V" ) };
    }

    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_measurementsGrid->SetColAttr( COL_MEASUREMENT_VALUE, attr );

    // Prepare the color list to plot traces
    SIM_PLOT_COLORS::FillDefaultColorList( m_darkMode );

    NGSPICE_SIMULATOR_SETTINGS* settings =
            dynamic_cast<NGSPICE_SIMULATOR_SETTINGS*>( m_simulator->Settings().get() );

    wxCHECK2( settings, /* do nothing in release builds*/ );

    if( settings && settings->GetWorkbookFilename().IsEmpty() )
        settings->SetModelMode( NGSPICE_MODEL_MODE::LT_PSPICE );

    m_simulator->Init();

    m_reporter = new SIM_THREAD_REPORTER( this );
    m_simulator->SetReporter( m_reporter );

    m_circuitModel = std::make_shared<NGSPICE_CIRCUIT_MODEL>( &m_schematicFrame->Schematic(), this );

    setupTools();
    setupUIConditions();

    ReCreateHToolbar();
    ReCreateMenuBar();

    Bind( wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( SIMULATOR_FRAME::onExit ), this,
          wxID_EXIT );

    Bind( EVT_SIM_UPDATE, &SIMULATOR_FRAME::onSimUpdate, this );
    Bind( EVT_SIM_REPORT, &SIMULATOR_FRAME::onSimReport, this );
    Bind( EVT_SIM_STARTED, &SIMULATOR_FRAME::onSimStarted, this );
    Bind( EVT_SIM_FINISHED, &SIMULATOR_FRAME::onSimFinished, this );
    Bind( EVT_SIM_CURSOR_UPDATE, &SIMULATOR_FRAME::onCursorUpdate, this );

    Bind( EVT_WORKBOOK_MODIFIED, &SIMULATOR_FRAME::onNotebookModified, this );

#ifndef wxHAS_NATIVE_TABART
    // Default non-native tab art has ugly gradients we don't want
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
    updateTitle();
}


SIMULATOR_FRAME::~SIMULATOR_FRAME()
{
    // Delete the GRID_TRICKS.
    m_signalsGrid->PopEventHandler( true );
    m_cursorsGrid->PopEventHandler( true );
    m_measurementsGrid->PopEventHandler( true );

    NULL_REPORTER devnull;

    m_simulator->Attach( nullptr, devnull );
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

    updateTitle();

    for( int ii = 0; ii < (int) m_plotNotebook->GetPageCount(); ++ii )
    {
        SIM_PANEL_BASE* plot = dynamic_cast<SIM_PLOT_PANEL*>( m_plotNotebook->GetPage( ii ) );

        wxCHECK( plot, /* void */ );

        plot->OnLanguageChanged();

        wxString pageTitle( m_simulator->TypeToName( plot->GetType(), true ) );
        pageTitle.Prepend( wxString::Format( _( "Plot%u - " ), ii+1 /* 1-based */ ) );

        m_plotNotebook->SetPageText( ii, pageTitle );
    }

    m_filter->SetHint( _( "Filter" ) );

    m_signalsGrid->SetColLabelValue( COL_SIGNAL_NAME, _( "Signal" ) );
    m_signalsGrid->SetColLabelValue( COL_SIGNAL_SHOW, _( "Plot" ) );
    m_signalsGrid->SetColLabelValue( COL_SIGNAL_COLOR, _( "Color" ) );
    m_signalsGrid->SetColLabelValue( COL_CURSOR_1, _( "Cursor 1" ) );
    m_signalsGrid->SetColLabelValue( COL_CURSOR_2, _( "Cursor 2" ) );

    m_cursorsGrid->SetColLabelValue( COL_CURSOR_NAME, _( "Cursor" ) );
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_SIGNAL, _( "Signal" ) );
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_X, _( "Time" ) );
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_Y, _( "Value" ) );
    updateCursors();

    for( TUNER_SLIDER* tuner : m_tuners )
        tuner->ShowChangedLanguage();
}


void SIMULATOR_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
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
        m_splitterCursorsSashPosition        = cfg->m_Simulator.cursors_panel_height;
        m_splitterTuneValuesSashPosition     = cfg->m_Simulator.measurements_panel_height;
        m_darkMode                           = !cfg->m_Simulator.white_background;
    }

    PROJECT_FILE& project = Prj().GetProjectFile();

    NGSPICE* currentSim = dynamic_cast<NGSPICE*>( m_simulator.get() );

    if( currentSim )
        m_simulator->Settings() = project.m_SchematicSettings->m_NgspiceSimulatorSettings;
}


void SIMULATOR_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( aCfg );
    wxASSERT( cfg );

    if( cfg )
    {
        EDA_BASE_FRAME::SaveSettings( cfg );

        cfg->m_Simulator.plot_panel_width          = m_splitterLeftRight->GetSashPosition();
        cfg->m_Simulator.plot_panel_height         = m_splitterPlotAndConsole->GetSashPosition();
        cfg->m_Simulator.signal_panel_height       = m_splitterSignals->GetSashPosition();
        cfg->m_Simulator.cursors_panel_height      = m_splitterCursors->GetSashPosition();
        cfg->m_Simulator.measurements_panel_height = m_splitterMeasurements->GetSashPosition();
        cfg->m_Simulator.white_background          = !m_darkMode;
    }

    PROJECT_FILE& project = Prj().GetProjectFile();

    if( project.m_SchematicSettings )
    {
        bool modified = project.m_SchematicSettings->m_NgspiceSimulatorSettings->SaveToFile();

        if( m_schematicFrame && modified )
            m_schematicFrame->OnModify();
    }
}


WINDOW_SETTINGS* SIMULATOR_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( aCfg );
    wxASSERT( cfg );

    return cfg ? &cfg->m_Simulator.window : nullptr;
}


void SIMULATOR_FRAME::initWorkbook()
{
    if( !m_simulator->Settings()->GetWorkbookFilename().IsEmpty() )
    {
        wxFileName filename = m_simulator->Settings()->GetWorkbookFilename();
        filename.SetPath( Prj().GetProjectPath() );

        if( !LoadWorkbook( filename.GetFullPath() ) )
            m_simulator->Settings()->SetWorkbookFilename( "" );
    }
    else if( LoadSimulator() )
    {
        if( !m_circuitModel->GetSchTextSimCommand().IsEmpty() )
            NewPlotPanel( m_circuitModel->GetSchTextSimCommand(), m_circuitModel->GetSimOptions() );

        rebuildSignalsList();
        rebuildSignalsGrid( m_filter->GetValue() );
    }
}


void SIMULATOR_FRAME::updateTitle()
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

        if( m_workbookModified )
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


void SIMULATOR_FRAME::setSubWindowsSashSize()
{
    if( m_splitterLeftRightSashPosition > 0 )
        m_splitterLeftRight->SetSashPosition( m_splitterLeftRightSashPosition );

    if( m_splitterPlotAndConsoleSashPosition > 0 )
        m_splitterPlotAndConsole->SetSashPosition( m_splitterPlotAndConsoleSashPosition );

    if( m_splitterSignalsSashPosition > 0 )
        m_splitterSignals->SetSashPosition( m_splitterSignalsSashPosition );

    if( m_splitterCursorsSashPosition > 0 )
        m_splitterCursors->SetSashPosition( m_splitterCursorsSashPosition );

    if( m_splitterTuneValuesSashPosition > 0 )
        m_splitterMeasurements->SetSashPosition( m_splitterTuneValuesSashPosition );
}


void SIMULATOR_FRAME::rebuildSignalsGrid( wxString aFilter )
{
    SUPPRESS_GRID_CELL_EVENTS raii( this );

    m_signalsGrid->ClearRows();

    if( aFilter.IsEmpty() )
        aFilter = wxS( "*" );

    EDA_COMBINED_MATCHER matcher( aFilter, CTX_SIGNAL );
    SIM_PLOT_PANEL*      plotPanel = GetCurrentPlot();
    int                  row = 0;

    for( const wxString& signal : m_signals )
    {
        if( matcher.StartsWith( signal ) )
        {
            int      traceType = SPT_UNKNOWN;
            wxString vectorName = vectorNameFromSignalName( signal, &traceType );
            TRACE*   trace = plotPanel ? plotPanel->GetTrace( vectorName, traceType ) : nullptr;

            m_signalsGrid->AppendRows( 1 );
            m_signalsGrid->SetCellValue( row, COL_SIGNAL_NAME, signal );

            if( !plotPanel )
            {
                wxGridCellAttr* attr = new wxGridCellAttr;
                attr->SetReadOnly();
                m_signalsGrid->SetAttr( row, COL_SIGNAL_SHOW, attr );
            }
            else
            {
                wxGridCellAttr* attr = new wxGridCellAttr;
                attr->SetRenderer( new wxGridCellBoolRenderer() );
                attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
                attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
                m_signalsGrid->SetAttr( row, COL_SIGNAL_SHOW, attr );
            }

            if( trace )
                m_signalsGrid->SetCellValue( row, COL_SIGNAL_SHOW, wxS( "1" ) );

            if( !plotPanel || !trace )
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
            else
            {
                wxGridCellAttr* attr = new wxGridCellAttr;
                attr = new wxGridCellAttr;
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

            row++;
        }
    }
}


void SIMULATOR_FRAME::rebuildSignalsList()
{
    m_signals.clear();

    int      options = GetCurrentOptions();
    SIM_TYPE simType = NGSPICE_CIRCUIT_MODEL::CommandToSimType( GetCurrentSimCommand() );
    wxString unconnected = wxString( wxS( "unconnected-(" ) );

    if( simType == ST_UNKNOWN )
        simType = ST_TRANSIENT;

    unconnected.Replace( '(', '_' );    // Convert to SPICE markup

    auto addSignal =
            [&]( const wxString& aSignalName )
            {
                if( simType == ST_AC )
                {
                    m_signals.push_back( aSignalName + _( " (gain)" ) );
                    m_signals.push_back( aSignalName + _( " (phase)" ) );
                }
                else
                {
                    m_signals.push_back( aSignalName );
                }
            };

    if( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES )
    {
        for( const std::string& net : m_circuitModel->GetNets() )
        {
            // netnames are escaped (can contain "{slash}" for '/') Unscape them:
            wxString netname = UnescapeString( net );

            if( netname == "GND" || netname == "0" || netname.StartsWith( unconnected ) )
                continue;

            m_quotedNetnames[ netname ] = wxString::Format( wxS( "\"%s\"" ), netname );
            addSignal( wxString::Format( wxS( "V(%s)" ), netname ) );
        }
    }

    if( ( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS )
            && ( simType == ST_TRANSIENT || simType == ST_DC ) )
    {
        for( const SPICE_ITEM& item : m_circuitModel->GetItems() )
        {
            // Add all possible currents for the device.
            for( const std::string& name : item.model->SpiceGenerator().CurrentNames( item ) )
                addSignal( name );
        }
    }

    if( ( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS )
            && ( simType == ST_TRANSIENT || simType == ST_DC ) )
    {
        for( const SPICE_ITEM& item : m_circuitModel->GetItems() )
        {
            if( item.model->GetPinCount() >= 2 )
            {
                wxString name = item.model->SpiceGenerator().ItemName( item );
                addSignal( wxString::Format( wxS( "P(%s)" ), name ) );
            }
        }
    }

    // Add .PROBE directives
    for( const wxString& directive : m_circuitModel->GetDirectives() )
    {
        wxStringTokenizer tokenizer( directive, wxT( "\r\n" ), wxTOKEN_STRTOK );

        while( tokenizer.HasMoreTokens() )
        {
            wxString line = tokenizer.GetNextToken();
            wxString directiveParams;

            if( line.Upper().StartsWith( wxS( ".PROBE" ), &directiveParams ) )
                addSignal( directiveParams.Trim( true ).Trim( false ) );
        }
    }

    // JEY TODO: find and add SPICE "LET" commands

    // Add user-defined signals
    for( const auto& [ signalId, signalName ] : m_userDefinedSignals )
        addSignal( signalName );

    std::sort( m_signals.begin(), m_signals.end(),
            []( const wxString& lhs, const wxString& rhs )
            {
                // Sort voltages first
                if( lhs.Upper().StartsWith( 'V' ) && !rhs.Upper().StartsWith( 'V' ) )
                    return true;
                else if( !lhs.Upper().StartsWith( 'V' ) && rhs.Upper().StartsWith( 'V' ) )
                    return false;

                return StrNumCmp( lhs, rhs, true /* ignore case */ ) < 0;
            } );
}


bool SIMULATOR_FRAME::LoadSimulator()
{
    wxString           errors;
    WX_STRING_REPORTER reporter( &errors );

    if( !m_schematicFrame->ReadyToNetlist( _( "Simulator requires a fully annotated schematic." ) ) )
        return false;

    // If we are using the new connectivity, make sure that we do a full-rebuild
    if( ADVANCED_CFG::GetCfg().m_IncrementalConnectivity )
        m_schematicFrame->RecalculateConnections( nullptr, GLOBAL_CLEANUP );

    if( !m_simulator->Attach( m_circuitModel, reporter ) )
    {
        DisplayErrorMessage( this, _( "Errors during netlist generation.\n\n" ) + errors );
        return false;
    }

    return true;
}


void SIMULATOR_FRAME::StartSimulation()
{
    if( m_circuitModel->CommandToSimType( GetCurrentSimCommand() ) == ST_UNKNOWN )
    {
        if( !EditSimCommand() )
            return;

        if( m_circuitModel->CommandToSimType( GetCurrentSimCommand() ) == ST_UNKNOWN )
            return;
    }

    wxString        schTextSimCommand = m_circuitModel->GetSchTextSimCommand();
    SIM_TYPE        schTextSimType = NGSPICE_CIRCUIT_MODEL::CommandToSimType( schTextSimCommand );
    SIM_PANEL_BASE* plotWindow = getCurrentPlotWindow();

    if( !plotWindow )
    {
        plotWindow = NewPlotPanel( schTextSimCommand, m_circuitModel->GetSimOptions() );
        OnModify();
    }
    else
    {
        m_circuitModel->SetSimCommandOverride( plotWindow->GetSimCommand() );

        if( plotWindow->GetType() == schTextSimType
                && schTextSimCommand != m_circuitModel->GetLastSchTextSimCommand() )
        {
            if( IsOK( this, _( "Schematic sheet simulation command directive has changed.  "
                               "Do you wish to update the Simulation Command?" ) ) )
            {
                m_circuitModel->SetSimCommandOverride( wxEmptyString );
                plotWindow->SetSimCommand( schTextSimCommand );
                OnModify();
            }
        }
    }

    m_circuitModel->SetSimOptions( GetCurrentOptions() );

    if( !LoadSimulator() )
        return;

    std::unique_lock<std::mutex> simulatorLock( m_simulator->GetMutex(), std::try_to_lock );

    if( simulatorLock.owns_lock() )
    {
        wxBusyCursor toggle;

        m_simConsole->Clear();

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


SIM_PANEL_BASE* SIMULATOR_FRAME::NewPlotPanel( const wxString& aSimCommand, int aOptions )
{
    SIM_PANEL_BASE* plotPanel = nullptr;
    SIM_TYPE        simType   = NGSPICE_CIRCUIT_MODEL::CommandToSimType( aSimCommand );

    if( SIM_PANEL_BASE::IsPlottable( simType ) )
    {
        SIM_PLOT_PANEL* panel = new SIM_PLOT_PANEL( aSimCommand, aOptions, m_plotNotebook, wxID_ANY );
        plotPanel = dynamic_cast<SIM_PANEL_BASE*>( panel );

        COMMON_SETTINGS::INPUT cfg = Pgm().GetCommonSettings()->m_Input;
        panel->GetPlotWin()->EnableMouseWheelPan( cfg.scroll_modifier_zoom != 0 );
    }
    else
    {
        SIM_NOPLOT_PANEL* panel = new SIM_NOPLOT_PANEL( aSimCommand, aOptions, m_plotNotebook, wxID_ANY );
        plotPanel = dynamic_cast<SIM_PANEL_BASE*>( panel );
    }

    wxString pageTitle( m_simulator->TypeToName( simType, true ) );
    pageTitle.Prepend( wxString::Format( _( "Plot%u - " ), (unsigned int) ++m_plotNumber ) );

    m_plotNotebook->AddPage( dynamic_cast<wxWindow*>( plotPanel ), pageTitle, true );

    OnModify();
    return plotPanel;
}


void SIMULATOR_FRAME::OnFilterText( wxCommandEvent& aEvent )
{
    rebuildSignalsGrid( m_filter->GetValue() );
}


void SIMULATOR_FRAME::OnFilterMouseMoved( wxMouseEvent& aEvent )
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


wxString vectorNameFromSignalId( int aUserDefinedSignalId )
{
    return wxString::Format( wxS( "user%d" ), aUserDefinedSignalId );
}


/**
 * For user-defined signals we display the user-oriented signal name such as "V(out)-V(in)",
 * but the simulator vector we actually have to plot will be "user0" or some-such.
 */
wxString SIMULATOR_FRAME::vectorNameFromSignalName( const wxString& aSignalName, int* aTraceType )
{
    std::map<wxString, int> suffixes;
    suffixes[ _( " (gain)" ) ] = SPT_AC_MAG;
    suffixes[ _( " (phase)" ) ] = SPT_AC_PHASE;

    if( aTraceType )
    {
        wxUniChar firstChar = aSignalName.Upper()[0];

        if( firstChar == 'V' )
            *aTraceType = SPT_VOLTAGE;
        else if( firstChar == 'I' )
            *aTraceType = SPT_CURRENT;
        else if( firstChar == 'P' )
            *aTraceType = SPT_POWER;
    }

    wxString suffix;
    wxString name = aSignalName;

    for( const auto& [ candidate, type ] : suffixes )
    {
        if( name.EndsWith( candidate ) )
        {
            name = name.Left( name.Length() - candidate.Length() );

            if( aTraceType )
                *aTraceType |= type;

            break;
        }
    }

    for( const auto& [ id, signal ] : m_userDefinedSignals )
    {
        if( name == signal )
            return vectorNameFromSignalId( id );
    }

    return name;
};


void SIMULATOR_FRAME::onSignalsGridCellChanged( wxGridEvent& aEvent )
{
    if( m_SuppressGridEvents > 0 )
        return;

    int             row = aEvent.GetRow();
    int             col = aEvent.GetCol();
    wxString        text = m_signalsGrid->GetCellValue( row, col );
    SIM_PLOT_PANEL* plot = GetCurrentPlot();
    wxString        signalName = m_signalsGrid->GetCellValue( row, COL_SIGNAL_NAME );
    int             traceType = SPT_UNKNOWN;
    wxString        vectorName = vectorNameFromSignalName( signalName, &traceType );

    if( col == COL_SIGNAL_SHOW )
    {
        if( text == wxS( "1" ) )
            updateTrace( vectorName, traceType, plot );
        else
            plot->DeleteTrace( vectorName, traceType );

        // Update enabled/visible states of other controls
        updateSignalsGrid();
        updateCursors();
        OnModify();
    }
    else if( col == COL_SIGNAL_COLOR )
    {
        KIGFX::COLOR4D color( m_signalsGrid->GetCellValue( row, COL_SIGNAL_COLOR ) );
        TRACE*         trace = plot->GetTrace( vectorName, traceType );

        if( trace )
        {
            trace->SetTraceColour( color.ToColour() );
            plot->UpdateTraceStyle( trace );
            plot->UpdatePlotColors();
            OnModify();
        }
    }
    else if( col == COL_CURSOR_1 || col == COL_CURSOR_2 )
    {
        for( int ii = 0; ii < m_signalsGrid->GetNumberRows(); ++ii )
        {
            signalName = m_signalsGrid->GetCellValue( ii, COL_SIGNAL_NAME );
            vectorName = vectorNameFromSignalName( signalName, &traceType );

            int  id = col == COL_CURSOR_1 ? 1 : 2;
            bool enable = ii == row && text == wxS( "1" );

            plot->EnableCursor( vectorName, traceType, id, enable, signalName );
            OnModify();
        }

        // Update cursor checkboxes (which are really radio buttons)
        updateSignalsGrid();
    }
}


void SIMULATOR_FRAME::onCursorsGridCellChanged( wxGridEvent& aEvent )
{
    if( m_SuppressGridEvents > 0 )
        return;

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

        updateCursors();
        OnModify();
    }
    else
    {
        wxFAIL_MSG( wxT( "All other columns are supposed to be read-only!" ) );
    }
}


SPICE_VALUE_FORMAT SIMULATOR_FRAME::GetMeasureFormat( int aRow ) const
{
    SPICE_VALUE_FORMAT result;
    result.FromString( m_measurementsGrid->GetCellValue( aRow, COL_MEASUREMENT_FORMAT ) );
    return result;
}


void SIMULATOR_FRAME::SetMeasureFormat( int aRow, const SPICE_VALUE_FORMAT& aFormat )
{
    m_measurementsGrid->SetCellValue( aRow, COL_MEASUREMENT_FORMAT, aFormat.ToString() );
    OnModify();
}


void SIMULATOR_FRAME::DeleteMeasurement( int aRow )
{
    if( aRow < ( m_measurementsGrid->GetNumberRows() - 1 ) )
    {
        m_measurementsGrid->DeleteRows( aRow, 1 );
        OnModify();
    }
}


void SIMULATOR_FRAME::onMeasurementsGridCellChanged( wxGridEvent& aEvent )
{
    SIM_PLOT_PANEL* plotPanel = GetCurrentPlot();

    if( !plotPanel )
        return;

    int      row = aEvent.GetRow();
    int      col = aEvent.GetCol();
    wxString text = m_measurementsGrid->GetCellValue( row, col );

    if( col == COL_MEASUREMENT )
    {
        UpdateMeasurement( row );
        OnModify();
    }
    else
    {
        wxFAIL_MSG( wxT( "All other columns are supposed to be read-only!" ) );
    }

    // Always leave a single empty row for type-in

    int rowCount = (int) m_measurementsGrid->GetNumberRows();
    int emptyRows = 0;

    for( row = rowCount - 1; row >= 0; row-- )
    {
        if( m_measurementsGrid->GetCellValue( row, COL_MEASUREMENT ).IsEmpty() )
            emptyRows++;
        else
            break;
    }

    if( emptyRows > 1 )
    {
        int killRows = emptyRows - 1;
        m_measurementsGrid->DeleteRows( rowCount - killRows, killRows );
    }
    else if( emptyRows == 0 )
    {
        m_measurementsGrid->AppendRows( 1 );
    }
}


/**
 * The user measurement looks something like:
 *    MAX V(out)
 *
 * We need to send ngspice a "MEAS" command with the analysis type, an output variable name,
 * and the signal name.  For our example above, this looks something like:
 *    MEAS TRAN meas_result_0 MAX V(out)
 *
 * This is also a good time to harvest the signal name prefix so we know what units to show on
 * the result.  For instance, for:
 *    MAX P(out)
 * we want to show:
 *    15W
 */
void SIMULATOR_FRAME::UpdateMeasurement( int aRow )
{
    static wxRegEx measureParamsRegEx( wxT( "^"
                                            " *"
                                            "([a-zA-Z_]+)"
                                            " +"
                                            "([a-zA-Z])\\(([^\\)]+)\\)" ) );

    SIM_PLOT_PANEL* plotPanel = GetCurrentPlot();

    if( !plotPanel )
        return;

    wxString text = m_measurementsGrid->GetCellValue( aRow, COL_MEASUREMENT );

    if( text.IsEmpty() )
    {
        m_measurementsGrid->SetCellValue( aRow, COL_MEASUREMENT_VALUE, wxEmptyString );
        return;
    }

    wxString simType = m_simulator->TypeToName( plotPanel->GetType(), true );
    wxString resultName = wxString::Format( wxS( "meas_result_%u" ), aRow );
    wxString result = wxS( "?" );

    if( measureParamsRegEx.Matches( text ) )
    {
        wxString           func = measureParamsRegEx.GetMatch( text, 1 ).Upper();
        wxUniChar          signalType = measureParamsRegEx.GetMatch( text, 2 ).Upper()[0];
        wxString           deviceName = measureParamsRegEx.GetMatch( text, 3 );
        wxString           units;
        SPICE_VALUE_FORMAT fmt = GetMeasureFormat( aRow );

        if( signalType == 'I' )
            units = wxS( "A" );
        else if( signalType == 'P' )
        {
            units = wxS( "W" );
            // Our syntax is different from ngspice for power signals
            text = func + " " + deviceName + ":power";
        }
        else
            units = wxS( "V" );

        if( func.EndsWith( wxS( "_AT" ) ) )
            units = wxS( "s" );
        else if( func.StartsWith( wxS( "INTEG" ) ) )
        {
            switch( plotPanel->GetType() )
            {
                case SIM_TYPE::ST_TRANSIENT:
                    if ( signalType == 'P' )
                        units = wxS( "J" );
                    else
                        units += wxS( ".s" );
                    break;
                case SIM_TYPE::ST_AC:
                case SIM_TYPE::ST_DISTORTION:
                case SIM_TYPE::ST_NOISE:
                case SIM_TYPE::ST_SENSITIVITY: // If there is a vector, it is frequency
                    units += wxS( "·Hz" );
                    break;
                case SIM_TYPE::ST_DC: // Could be a lot of things : V, A, deg C, ohm, ...
                case SIM_TYPE::ST_OP: // There is no vector for integration
                case SIM_TYPE::ST_POLE_ZERO: // There is no vector for integration
                case SIM_TYPE::ST_TRANS_FUNC: // There is no vector for integration
                default:

                    units += wxS( "·?" );
                    break;
            }
        }

        fmt.UpdateUnits( units );
        SetMeasureFormat( aRow, fmt );
    }

    if( m_simFinished )
    {
        wxString cmd = wxString::Format( wxS( "meas %s %s %s" ), simType, resultName, text );
        m_simulator->Command( "echo " + cmd.ToStdString() );
        m_simulator->Command( cmd.ToStdString() );

        std::vector<double> resultVec = m_simulator->GetMagPlot( resultName.ToStdString() );

        if( resultVec.size() > 0 )
            result = SPICE_VALUE( resultVec[0] ).ToString( GetMeasureFormat( aRow ) );
    }

    m_measurementsGrid->SetCellValue( aRow, COL_MEASUREMENT_VALUE, result );
}


void SIMULATOR_FRAME::AddVoltageTrace( const wxString& aNetName )
{
    doAddTrace( aNetName, SPT_VOLTAGE );
}


void SIMULATOR_FRAME::AddCurrentTrace( const wxString& aDeviceName )
{
    doAddTrace( aDeviceName, SPT_CURRENT );
}


void SIMULATOR_FRAME::AddTuner( const SCH_SHEET_PATH& aSheetPath, SCH_SYMBOL* aSymbol )
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
        TUNER_SLIDER* tuner = new TUNER_SLIDER( this, m_panelTuners, aSheetPath, aSymbol );
        m_sizerTuners->Add( tuner );
        m_tuners.push_back( tuner );
        m_panelTuners->Layout();
        OnModify();
    }
    catch( const KI_PARAM_ERROR& e )
    {
        DisplayErrorMessage( nullptr, e.What() );
    }
}


void SIMULATOR_FRAME::UpdateTunerValue( const SCH_SHEET_PATH& aSheetPath, const KIID& aSymbol,
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


void SIMULATOR_FRAME::RemoveTuner( TUNER_SLIDER* aTuner )
{
    m_tuners.remove( aTuner );
    aTuner->Destroy();
    m_panelTuners->Layout();
    OnModify();
}


void SIMULATOR_FRAME::AddMeasurement( const wxString& aCmd )
{
    // -1 because the last one is for user input
    for( int i = 0; i < m_measurementsGrid->GetNumberRows(); i++ )
    {
        if ( m_measurementsGrid->GetCellValue( i, COL_MEASUREMENT ) == aCmd )
            return; // Don't create duplicates
    }

    SIM_PLOT_PANEL* plotPanel = GetCurrentPlot();

    if( !plotPanel )
        return;

    wxString simType = m_simulator->TypeToName( plotPanel->GetType(), true );
    int      row;

    for( row = 0; row < m_measurementsGrid->GetNumberRows(); ++row )
    {
        if( m_measurementsGrid->GetCellValue( row, COL_MEASUREMENT ).IsEmpty() )
            break;
    }

    if( !m_measurementsGrid->GetCellValue( row, COL_MEASUREMENT ).IsEmpty() )
    {
        m_measurementsGrid->AppendRows( 1 );
        row = m_measurementsGrid->GetNumberRows() - 1;
    }

    m_measurementsGrid->SetCellValue( row, COL_MEASUREMENT, aCmd );
    SetMeasureFormat( row, { 3, wxS( "~V" ) } );

    UpdateMeasurement( row );
    OnModify();

    // Always leave at least one empty row for type-in:
    row = m_measurementsGrid->GetNumberRows() - 1;

    if( !m_measurementsGrid->GetCellValue( row, COL_MEASUREMENT ).IsEmpty() )
        m_measurementsGrid->AppendRows( 1 );
}


SIM_PLOT_PANEL* SIMULATOR_FRAME::GetCurrentPlot() const
{
    SIM_PANEL_BASE* curPage = getCurrentPlotWindow();

    return !curPage || curPage->GetType() == ST_UNKNOWN ? nullptr
                                                        : dynamic_cast<SIM_PLOT_PANEL*>( curPage );
}


const NGSPICE_CIRCUIT_MODEL* SIMULATOR_FRAME::GetExporter() const
{
    return m_circuitModel.get();
}


void SIMULATOR_FRAME::doAddTrace( const wxString& aName, SIM_TRACE_TYPE aType )
{
    SIM_PLOT_PANEL* plotPanel = GetCurrentPlot();

    if( !plotPanel )
    {
        m_simConsole->AppendText( _( "Error: no current simulation.\n" ) );
        m_simConsole->SetInsertionPointEnd();
        return;
    }

    SIM_TYPE simType = NGSPICE_CIRCUIT_MODEL::CommandToSimType( plotPanel->GetSimCommand() );

    if( simType == ST_UNKNOWN )
    {
        m_simConsole->AppendText( _( "Error: simulation type not defined.\n" ) );
        m_simConsole->SetInsertionPointEnd();
        return;
    }
    else if( !SIM_PANEL_BASE::IsPlottable( simType ) )
    {
        m_simConsole->AppendText( _( "Error: simulation type doesn't support plotting.\n" ) );
        m_simConsole->SetInsertionPointEnd();
        return;
    }

    if( simType == ST_AC )
    {
        updateTrace( aName, aType | SPT_AC_MAG, plotPanel );
        updateTrace( aName, aType | SPT_AC_PHASE, plotPanel );
    }
    else
    {
        updateTrace( aName, aType, plotPanel );
    }

    updateSignalsGrid();
    OnModify();
}


void SIMULATOR_FRAME::SetUserDefinedSignals( const std::map<int, wxString>& aNewSignals )
{
    for( size_t ii = 0; ii < m_plotNotebook->GetPageCount(); ++ii )
    {
        SIM_PLOT_PANEL* plotPanel = dynamic_cast<SIM_PLOT_PANEL*>( m_plotNotebook->GetPage( ii ) );

        if( !plotPanel )
            continue;

        for( const auto& [ id, existingSignal ] : m_userDefinedSignals )
        {
            int      traceType = SPT_UNKNOWN;
            wxString vectorName = vectorNameFromSignalName( existingSignal, &traceType );

            if( aNewSignals.count( id ) == 0 )
            {
                if( plotPanel->GetType() == ST_AC )
                {
                    for( int subType : { SPT_AC_MAG, SPT_AC_PHASE } )
                        plotPanel->DeleteTrace( vectorName, traceType | subType );
                }
                else
                {
                    plotPanel->DeleteTrace( vectorName, traceType );
                }
            }
            else
            {
                if( plotPanel->GetType() == ST_AC )
                {
                    for( int subType : { SPT_AC_MAG, SPT_AC_PHASE } )
                    {
                        if( TRACE* trace = plotPanel->GetTrace( vectorName, traceType | subType ) )
                            trace->SetName( aNewSignals.at( id ) );
                    }
                }
                else
                {
                    if( TRACE* trace = plotPanel->GetTrace( vectorName, traceType ) )
                        trace->SetName( aNewSignals.at( id ) );
                }
            }
        }
    }

    m_userDefinedSignals = aNewSignals;

    if( m_simFinished )
        applyUserDefinedSignals();

    rebuildSignalsList();
    rebuildSignalsGrid( m_filter->GetValue() );
    updateSignalsGrid();
    updateCursors();
    OnModify();
}


void SIMULATOR_FRAME::updateTrace( const wxString& aVectorName, int aTraceType,
                                  SIM_PLOT_PANEL* aPlotPanel )
{
    SIM_TYPE simType = NGSPICE_CIRCUIT_MODEL::CommandToSimType( aPlotPanel->GetSimCommand() );

    aTraceType &= aTraceType & SPT_Y_AXIS_MASK;
    aTraceType |= getXAxisType( simType );

    wxString simVectorName = aVectorName;

    if( aTraceType & SPT_POWER )
        simVectorName = simVectorName.AfterFirst( '(' ).BeforeLast( ')' ) + wxS( ":power" );

    if( !SIM_PANEL_BASE::IsPlottable( simType ) )
    {
        // There is no plot to be shown
        m_simulator->Command( wxString::Format( wxT( "print %s" ), aVectorName ).ToStdString() );

        return;
    }

    // First, handle the x axis
    wxString xAxisName( m_simulator->GetXAxis( simType ) );

    if( xAxisName.IsEmpty() )
        return;

    std::vector<double> data_x;
    std::vector<double> data_y;

    if( m_simFinished )
    {
        data_x = m_simulator->GetMagPlot( (const char*) xAxisName.c_str() );

        switch( simType )
        {
        case ST_AC:
            if( aTraceType & SPT_AC_MAG )
                data_y = m_simulator->GetMagPlot( (const char*) simVectorName.c_str() );
            else if( aTraceType & SPT_AC_PHASE )
                data_y = m_simulator->GetPhasePlot( (const char*) simVectorName.c_str() );
            else
                wxFAIL_MSG( wxT( "Plot type missing AC_PHASE or AC_MAG bit" ) );

            break;

        case ST_NOISE:
        case ST_DC:
        case ST_TRANSIENT:
            data_y = m_simulator->GetMagPlot( (const char*) simVectorName.c_str() );
            break;

        default:
            wxFAIL_MSG( wxT( "Unhandled plot type" ) );
        }
    }

    unsigned int size = data_x.size();

    // If we did a two-source DC analysis, we need to split the resulting vector and add traces
    // for each input step
    SPICE_DC_PARAMS source1, source2;

    if( simType == ST_DC
        && m_circuitModel->ParseDCCommand( m_circuitModel->GetSimCommand(), &source1, &source2 )
        && !source2.m_source.IsEmpty() )
    {
        // Source 1 is the inner loop, so lets add traces for each Source 2 (outer loop) step
        SPICE_VALUE v = source2.m_vstart;

        size_t offset = 0;
        size_t outer = ( size_t )( ( source2.m_vend - v ) / source2.m_vincrement ).ToDouble();
        size_t inner = data_x.size() / ( outer + 1 );

        wxASSERT( data_x.size() % ( outer + 1 ) == 0 );

        for( size_t idx = 0; idx <= outer; idx++ )
        {
            if( TRACE* trace = aPlotPanel->AddTrace( aVectorName, aTraceType ) )
            {
                if( data_y.size() >= size )
                {
                    std::vector<double> sub_x( data_x.begin() + offset,
                                               data_x.begin() + offset + inner );
                    std::vector<double> sub_y( data_y.begin() + offset,
                                               data_y.begin() + offset + inner );

                    aPlotPanel->SetTraceData( trace, inner, sub_x.data(), sub_y.data() );
                }
            }

            v = v + source2.m_vincrement;
            offset += inner;
        }
    }
    else if( TRACE* trace = aPlotPanel->AddTrace( aVectorName, aTraceType ) )
    {
        if( data_y.size() >= size )
            aPlotPanel->SetTraceData( trace, size, data_x.data(), data_y.data() );
    }
}


void SIMULATOR_FRAME::updateSignalsGrid()
{
    SIM_PLOT_PANEL* plot = GetCurrentPlot();

    for( int row = 0; row < m_signalsGrid->GetNumberRows(); ++row )
    {
        wxString signalName = m_signalsGrid->GetCellValue( row, COL_SIGNAL_NAME );
        int      traceType = SPT_UNKNOWN;
        wxString vectorName = vectorNameFromSignalName( signalName, &traceType );

        if( TRACE* trace = plot ? plot->GetTrace( vectorName, traceType ) : nullptr )
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


void SIMULATOR_FRAME::applyUserDefinedSignals()
{
    auto quoteNetNames =
            [&]( wxString aExpression ) -> wxString
            {
                for( const auto& [netname, quotedNetname] : m_quotedNetnames )
                    aExpression.Replace( netname, quotedNetname );

                return aExpression;
            };

    for( const auto& [ id, signal ] : m_userDefinedSignals )
    {
        std::string cmd = "let user{} = {}";

        m_simulator->Command( "echo " + fmt::format( cmd, id, signal.ToStdString() ) );
        m_simulator->Command( fmt::format( cmd, id, quoteNetNames( signal ).ToStdString() ) );
    }
}


void SIMULATOR_FRAME::applyTuners()
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

        double floatVal = tuner->GetValue().ToDouble();

        m_simulator->Command( item->model->SpiceGenerator().TunerCommand( *item, floatVal ) );
    }

    if( reporter.HasMessage() )
        DisplayErrorMessage( this, _( "Could not apply tuned value(s):" ) + wxS( "\n" ) + errors );
}


void SIMULATOR_FRAME::parseTraceParams( SIM_PLOT_PANEL* aPlotPanel, TRACE* aTrace,
                                       const wxString& aSignalName, const wxString& aParams )
{
    auto addCursor =
            [&]( int aCursorId, double x )
            {
                CURSOR* cursor = new CURSOR( aTrace, aPlotPanel );

                cursor->SetName( aSignalName );
                cursor->SetPen( wxPen( aTrace->GetTraceColour() ) );
                cursor->SetCoordX( x );

                aTrace->SetCursor( aCursorId, cursor );
                aPlotPanel->GetPlotWin()->AddLayer( cursor );
            };

    wxArrayString items = wxSplit( aParams, '|' );

    for( const wxString& item : items )
    {
        if( item.StartsWith( wxS( "rgb" ) ) )
        {
            wxColour color;
            color.Set( item );
            aTrace->SetTraceColour( color );
            aPlotPanel->UpdateTraceStyle( aTrace );
        }
        else if( item.StartsWith( wxS( "cursor1" ) ) )
        {
            wxArrayString parts = wxSplit( item, ':' );
            double        val;

            if( parts.size() == 3 )
            {
                parts[0].AfterFirst( '=' ).ToDouble( &val );
                m_cursorFormats[0][0].FromString( parts[1] );
                m_cursorFormats[0][1].FromString( parts[2] );
                addCursor( 1, val );
            }
        }
        else if( item.StartsWith( wxS( "cursor2" ) ) )
        {
            wxArrayString parts = wxSplit( item, ':' );
            double        val;

            if( parts.size() == 3 )
            {
                parts[0].AfterFirst( '=' ).ToDouble( &val );
                m_cursorFormats[1][0].FromString( parts[1] );
                m_cursorFormats[1][1].FromString( parts[2] );
                addCursor( 2, val );
            }
        }
        else if( item.StartsWith( wxS( "cursorD" ) ) )
        {
            wxArrayString parts = wxSplit( item, ':' );

            if( parts.size() == 3 )
            {
                m_cursorFormats[2][0].FromString( parts[1] );
                m_cursorFormats[2][1].FromString( parts[2] );
            }
        }
        else if( item == wxS( "dottedSecondary" ) )
        {
            aPlotPanel->SetDottedSecondary( true );
        }
        else if( item == wxS( "hideGrid" ) )
        {
            aPlotPanel->ShowGrid( false );
        }
    }
}


bool SIMULATOR_FRAME::LoadWorkbook( const wxString& aPath )
{
    m_plotNotebook->DeleteAllPages();

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

    if( !plotCountLine.ToLong( &plotsCount ) )
    {
        DISPLAY_LOAD_ERROR( "Error loading workbook: Line %d is not an integer." );
        file.Close();

        return false;
    }

    std::map<SIM_PLOT_PANEL*, std::vector<std::tuple<long, wxString, wxString>>> traceInfo;

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

        if( version >= 3 )
        {
            simOptions &= ~NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS;
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
            else if( line.StartsWith( wxT( ".probe allp" ) ) )
                simOptions |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS;
            else
                simCommand += line + wxT( "\n" );
        }

        SIM_PLOT_PANEL* plotPanel = dynamic_cast<SIM_PLOT_PANEL*>( NewPlotPanel( simCommand,
                                                                                 simOptions ) );

        if( !file.GetNextLine().ToLong( &tracesCount ) )
        {
            DISPLAY_LOAD_ERROR( "Error loading workbook: Line %d is not an integer." );
            file.Close();

            return false;
        }

        if( plotPanel )
            traceInfo[ plotPanel ] = {};

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

            if( plotPanel )
                traceInfo[ plotPanel ].emplace_back( std::make_tuple( traceType, name, param ) );
        }
    }

    long userDefinedSignalCount;
    long measurementCount;

    if( file.GetNextLine().ToLong( &userDefinedSignalCount ) )
    {
        for( int ii = 0; ii < (int) userDefinedSignalCount; ++ii )
            m_userDefinedSignals[ ii ] = file.GetNextLine();

        file.GetNextLine().ToLong( &measurementCount );

        m_measurementsGrid->ClearRows();
        m_measurementsGrid->AppendRows( (int) measurementCount + 1 /* empty row at end */ );

        for( int row = 0; row < (int) measurementCount; ++row )
        {
            m_measurementsGrid->SetCellValue( row, COL_MEASUREMENT, file.GetNextLine() );
            m_measurementsGrid->SetCellValue( row, COL_MEASUREMENT_FORMAT, file.GetNextLine() );
        }
    }

    for( const auto& [ plotPanel, traceInfoVector ] : traceInfo )
    {
        for( const auto& [ traceType, signalName, param ] : traceInfoVector )
        {
            if( traceType == SPT_UNKNOWN && signalName == wxS( "$LEGEND" ) )
            {
                wxArrayString coords = wxSplit( param, ' ' );

                if( coords.size() >= 2 )
                {
                    long x = 0;
                    long y = 0;

                    coords[0].ToLong( &x );
                    coords[1].ToLong( &y );
                    plotPanel->SetLegendPosition( wxPoint( (int) x, (int) y ) );
                }

                plotPanel->ShowLegend( true );
            }
            else
            {
                wxString vectorName = vectorNameFromSignalName( signalName, nullptr );
                TRACE*   trace = plotPanel->AddTrace( vectorName, (int) traceType );

                if( version >= 4 && trace )
                    parseTraceParams( plotPanel, trace, signalName, param );
            }
        }

        plotPanel->UpdatePlotColors();
    }

    LoadSimulator();

    wxString schTextSimCommand = m_circuitModel->GetSchTextSimCommand().Lower();
    SIM_TYPE schTextSimType = NGSPICE_CIRCUIT_MODEL::CommandToSimType( schTextSimCommand );

    if( schTextSimType != ST_UNKNOWN )
    {
        bool found = false;

        for( int ii = 0; ii < (int) m_plotNotebook->GetPageCount(); ++ii )
        {
            auto* plot = dynamic_cast<const SIM_PANEL_BASE*>( m_plotNotebook->GetPage( ii ) );

            if( plot && plot->GetType() == schTextSimType )
            {
                if( schTextSimType == ST_DC )
                {
                    wxString plotSimCommand = plot->GetSimCommand().Lower();
                    found = plotSimCommand.GetChar( 4 ) == schTextSimCommand.GetChar( 4 );
                }
                else
                {
                    found = true;
                }
            }
        }

        if( !found )
            NewPlotPanel( schTextSimCommand, m_circuitModel->GetSimOptions() );
    }

    rebuildSignalsList();

    rebuildSignalsGrid( m_filter->GetValue() );
    updateSignalsGrid();
    updateCursors();

    file.Close();

    wxFileName filename( aPath );
    filename.MakeRelativeTo( Prj().GetProjectPath() );

    // Remember the loaded workbook filename.
    m_simulator->Settings()->SetWorkbookFilename( filename.GetFullPath() );

    updateTitle();

    // Successfully loading a workbook does not count as modifying it.  Clear the modified
    // flag after all the EVT_WORKBOOK_MODIFIED events have been processed.
    CallAfter( [=]()
               {
                   m_workbookModified = false;
               } );

    return true;
}


bool SIMULATOR_FRAME::SaveWorkbook( const wxString& aPath )
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

    file.AddLine( wxT( "version 4" ) );

    file.AddLine( wxString::Format( wxT( "%llu" ), m_plotNotebook->GetPageCount() ) );

    for( size_t i = 0; i < m_plotNotebook->GetPageCount(); i++ )
    {
        const SIM_PANEL_BASE* basePanel = dynamic_cast<const SIM_PANEL_BASE*>( m_plotNotebook->GetPage( i ) );

        if( !basePanel )
        {
            file.AddLine( wxString::Format( wxT( "%llu" ), 0ull ) );
            continue;
        }

        file.AddLine( wxString::Format( wxT( "%d" ), basePanel->GetType() ) );

        wxString command = basePanel->GetSimCommand();
        int      options = basePanel->GetSimOptions();

        if( options & NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS )
            command += wxT( "\n.kicad adjustpaths" );

        if( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES )
            command += wxT( "\n.save all" );

        if( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS )
            command += wxT( "\n.probe alli" );

        if( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS )
            command += wxT( "\n.probe allp" );

        file.AddLine( EscapeString( command, CTX_LINE ) );

        const SIM_PLOT_PANEL* plotPanel = dynamic_cast<const SIM_PLOT_PANEL*>( basePanel );

        if( !plotPanel )
        {
            file.AddLine( wxString::Format( wxT( "%llu" ), 0ull ) );
            continue;
        }

        size_t traceCount = plotPanel->GetTraces().size();

        if( plotPanel->IsLegendShown() )
            traceCount++;

        file.AddLine( wxString::Format( wxT( "%llu" ), traceCount ) );

        auto findSignalName =
                [&]( const wxString& aVectorName ) -> wxString
                {
                    for( const auto& [ id, signal ] : m_userDefinedSignals )
                    {
                        if( aVectorName == vectorNameFromSignalId( id ) )
                            return signal;
                    }

                    return aVectorName;
                };

        for( const auto& [name, trace] : plotPanel->GetTraces() )
        {
            file.AddLine( wxString::Format( wxT( "%d" ), trace->GetType() ) );
            file.AddLine( findSignalName( trace->GetName() ) );

            wxString msg = COLOR4D( trace->GetTraceColour() ).ToCSSString();

            if( CURSOR* cursor = trace->GetCursor( 1 ) )
            {
                msg += wxString::Format( wxS( "|cursor1=%E:%s:%s" ),
                                         cursor->GetCoords().x,
                                         m_cursorFormats[0][0].ToString(),
                                         m_cursorFormats[0][1].ToString() );
            }

            if( CURSOR* cursor = trace->GetCursor( 2 ) )
            {
                msg += wxString::Format( wxS( "|cursor2=%E:%s:%s" ),
                                         cursor->GetCoords().x,
                                         m_cursorFormats[1][0].ToString(),
                                         m_cursorFormats[1][1].ToString() );
            }

            if( trace->GetCursor( 1 ) || trace->GetCursor( 2 ) )
            {
                msg += wxString::Format( wxS( "|cursorD:%s:%s" ),
                                         m_cursorFormats[2][0].ToString(),
                                         m_cursorFormats[2][1].ToString() );
            }

            if( plotPanel->GetDottedSecondary() )
                msg += wxS( "|dottedSecondary" );

            if( !plotPanel->IsGridShown() )
                msg += wxS( "|hideGrid" );

            file.AddLine( msg );
        }

        if( plotPanel->IsLegendShown() )
        {
            file.AddLine( wxString::Format( wxT( "%d" ), SPT_UNKNOWN ) );
            file.AddLine( wxT( "$LEGEND" ) );
            file.AddLine( wxString::Format( wxT( "%d %d" ),
                                            plotPanel->GetLegendPosition().x,
                                            plotPanel->GetLegendPosition().y - 40 ) );
        }
    }

    file.AddLine( wxString::Format( wxT( "%llu" ), m_userDefinedSignals.size() ) );

    for( const auto& [ id, signal ] : m_userDefinedSignals )
        file.AddLine( signal );

    std::vector<wxString> measurements;
    std::vector<wxString> formats;

    for( int i = 0; i < m_measurementsGrid->GetNumberRows(); ++i )
    {
        if( !m_measurementsGrid->GetCellValue( i, COL_MEASUREMENT ).IsEmpty() )
        {
            measurements.push_back( m_measurementsGrid->GetCellValue( i, COL_MEASUREMENT ) );
            formats.push_back( m_measurementsGrid->GetCellValue( i, COL_MEASUREMENT_FORMAT ) );
        }
    }

    file.AddLine( wxString::Format( wxT( "%llu" ), measurements.size() ) );

    for( size_t i = 0; i < measurements.size(); i++ )
    {
        file.AddLine( measurements[i] );
        file.AddLine( formats[i] );
    }

    bool res = file.Write();
    file.Close();

    // Store the filename of the last saved workbook.
    if( res )
    {
        filename.MakeRelativeTo( Prj().GetProjectPath() );
        m_simulator->Settings()->SetWorkbookFilename( filename.GetFullPath() );
    }

    m_workbookModified = false;
    updateTitle();

    return res;
}


SIM_TRACE_TYPE SIMULATOR_FRAME::getXAxisType( SIM_TYPE aType ) const
{
    switch( aType )
    {
    /// @todo SPT_LOG_FREQUENCY
    case ST_AC:        return SPT_LIN_FREQUENCY;
    case ST_DC:        return SPT_SWEEP;
    case ST_TRANSIENT: return SPT_TIME;
    default:           wxFAIL_MSG( wxS( "Unhandled simulation type" ) ); return SPT_UNKNOWN;
    }
}


void SIMULATOR_FRAME::ToggleDarkModePlots()
{
    m_darkMode = !m_darkMode;

    // Rebuild the color list to plot traces
    SIM_PLOT_COLORS::FillDefaultColorList( m_darkMode );

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


void SIMULATOR_FRAME::onPlotClose( wxAuiNotebookEvent& event )
{
}


void SIMULATOR_FRAME::onPlotClosed( wxAuiNotebookEvent& event )
{
    CallAfter( [this]()
               {
                   rebuildSignalsList();
                   rebuildSignalsGrid( m_filter->GetValue() );
                   updateCursors();

                   SIM_PANEL_BASE* panel = getCurrentPlotWindow();

                   if( !panel || panel->GetType() != ST_OP )
                   {
                       SCHEMATIC& schematic = m_schematicFrame->Schematic();
                       schematic.ClearOperatingPoints();
                       m_schematicFrame->RefreshOperatingPointDisplay();
                       m_schematicFrame->GetCanvas()->Refresh();
                   }
               } );
}


void SIMULATOR_FRAME::onPlotChanged( wxAuiNotebookEvent& event )
{
    rebuildSignalsList();
    rebuildSignalsGrid( m_filter->GetValue() );
    updateCursors();
}


void SIMULATOR_FRAME::onPlotDragged( wxAuiNotebookEvent& event )
{
}


void SIMULATOR_FRAME::onNotebookModified( wxCommandEvent& event )
{
    OnModify();
    updateTitle();
}


bool SIMULATOR_FRAME::EditSimCommand()
{
    SIM_PANEL_BASE*     plotPanelWindow = getCurrentPlotWindow();
    DIALOG_SIM_COMMAND  dlg( this, m_circuitModel, m_simulator->Settings() );
    wxString            errors;
    WX_STRING_REPORTER  reporter( &errors );

    if( !m_circuitModel->ReadSchematicAndLibraries( NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS,
                                                    reporter ) )
    {
        DisplayErrorMessage( this, _( "Errors during netlist generation.\n\n" )
                                   + errors );
    }

    if( m_plotNotebook->GetPageIndex( plotPanelWindow ) != wxNOT_FOUND )
    {
        dlg.SetSimCommand( plotPanelWindow->GetSimCommand() );
        dlg.SetSimOptions( plotPanelWindow->GetSimOptions() );
    }
    else
    {
        dlg.SetSimOptions( NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS );
    }

    if( dlg.ShowModal() == wxID_OK )
    {
        wxString oldCommand;

        if( m_plotNotebook->GetPageIndex( plotPanelWindow ) != wxNOT_FOUND )
            oldCommand = plotPanelWindow->GetSimCommand();
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
            if( m_plotNotebook->GetPageIndex( plotPanelWindow ) == 0 )
                m_circuitModel->SetSimCommandOverride( newCommand );

            // Update simulation command in the current plot
            plotPanelWindow->SetSimCommand( newCommand );
            plotPanelWindow->SetSimOptions( newOptions );
        }

        OnModify();
        m_simulator->Init();
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
    m_schematicFrame->GetToolManager()->RunAction( ACTIONS::cancelInteractive );

    SaveSettings( config() );

    m_simulator->Settings() = nullptr;

    Destroy();
}


void SIMULATOR_FRAME::updateCursors()
{
    SUPPRESS_GRID_CELL_EVENTS raii( this );

    m_cursorsGrid->ClearRows();

    SIM_PLOT_PANEL* plotPanel = GetCurrentPlot();

    if( !plotPanel )
        return;

    // Update cursor values
    CURSOR*  cursor1 = nullptr;
    wxString cursor1Name;
    wxString cursor1Units;
    CURSOR*  cursor2 = nullptr;
    wxString cursor2Name;
    wxString cursor2Units;

    auto getUnitsY =
            [&]( TRACE* aTrace ) -> wxString
            {
                if( ( aTrace->GetType() & SPT_AC_PHASE ) || ( aTrace->GetType() & SPT_CURRENT ) )
                    return plotPanel->GetUnitsY2();
                else if( aTrace->GetType() & SPT_POWER )
                    return plotPanel->GetUnitsY3();
                else
                    return plotPanel->GetUnitsY1();
            };

    auto getNameY =
            [&]( TRACE* aTrace ) -> wxString
            {
                if( ( aTrace->GetType() & SPT_AC_PHASE ) || ( aTrace->GetType() & SPT_CURRENT ) )
                    return plotPanel->GetLabelY2();
                else if( aTrace->GetType() & SPT_POWER )
                    return plotPanel->GetLabelY3();
                else
                    return plotPanel->GetLabelY1();
            };

    auto formatValue =
            [this]( double aValue, int aCursorId, int aCol ) -> wxString
            {
                if( !m_simFinished && aCol == 1 )
                    return wxS( "--" );
                else
                    return SPICE_VALUE( aValue ).ToString( m_cursorFormats[ aCursorId ][ aCol ] );
            };

    for( const auto& [name, trace] : plotPanel->GetTraces() )
    {
        if( CURSOR* cursor = trace->GetCursor( 1 ) )
        {
            cursor1 = cursor;
            cursor1Name = getNameY( trace );
            cursor1Units = getUnitsY( trace );

            wxRealPoint coords = cursor->GetCoords();
            int         row = m_cursorsGrid->GetNumberRows();

            m_cursorFormats[0][0].UpdateUnits( plotPanel->GetUnitsX() );
            m_cursorFormats[0][1].UpdateUnits( cursor1Units );

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
            cursor2Name = getNameY( trace );
            cursor2Units = getUnitsY( trace );

            wxRealPoint coords = cursor->GetCoords();
            int         row = m_cursorsGrid->GetNumberRows();

            m_cursorFormats[1][0].UpdateUnits( plotPanel->GetUnitsX() );
            m_cursorFormats[1][1].UpdateUnits( cursor2Units );

            m_cursorsGrid->AppendRows( 1 );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_NAME, wxS( "2" ) );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_SIGNAL, cursor->GetName() );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_X, formatValue( coords.x, 1, 0 ) );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_Y, formatValue( coords.y, 1, 1 ) );
            break;
        }
    }

    if( cursor1 && cursor2 && cursor1Units == cursor2Units )
    {
        wxRealPoint coords = cursor2->GetCoords() - cursor1->GetCoords();
        wxString    signal;

        m_cursorFormats[2][0].UpdateUnits( plotPanel->GetUnitsX() );
        m_cursorFormats[2][1].UpdateUnits( cursor1Units );

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

    // Set up the labels
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_X, plotPanel->GetLabelX() );

    wxString valColName = _( "Value" );

    if( !cursor1Name.IsEmpty() )
    {
        if( cursor2Name.IsEmpty() || cursor1Name == cursor2Name )
            valColName = cursor1Name;
    }
    else if( !cursor2Name.IsEmpty() )
    {
        valColName = cursor2Name;
    }

    m_cursorsGrid->SetColLabelValue( COL_CURSOR_Y, valColName );
}


void SIMULATOR_FRAME::onCursorUpdate( wxCommandEvent& aEvent )
{
    updateCursors();
    OnModify();
}


void SIMULATOR_FRAME::setupUIConditions()
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


void SIMULATOR_FRAME::onSimStarted( wxCommandEvent& aEvent )
{
    SetCursor( wxCURSOR_ARROWWAIT );
}


void SIMULATOR_FRAME::onSimFinished( wxCommandEvent& aEvent )
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

    m_simFinished = true;

    std::vector<wxString> oldSignals = m_signals;

    applyUserDefinedSignals();
    rebuildSignalsList();

    SCHEMATIC& schematic = m_schematicFrame->Schematic();
    schematic.ClearOperatingPoints();

    // If there are any signals plotted, update them
    if( SIM_PANEL_BASE::IsPlottable( simType ) )
    {
        SIM_PLOT_PANEL* plotPanel = dynamic_cast<SIM_PLOT_PANEL*>( plotPanelWindow );
        wxCHECK_RET( plotPanel, wxT( "not a SIM_PLOT_PANEL" ) );

        // Map of TRACE* to { vectorName, traceType }
        std::map<TRACE*, std::pair<wxString, int>> traceMap;

        for( const auto& [ name, trace ] : plotPanel->GetTraces() )
            traceMap[ trace ] = { wxEmptyString, SPT_UNKNOWN };

        for( const wxString& signal : m_signals )
        {
            int      traceType = SPT_UNKNOWN;
            wxString vectorName = vectorNameFromSignalName( signal, &traceType );

            if( simType == ST_AC )
            {
                for( int subType : { SPT_AC_MAG, SPT_AC_PHASE } )
                {
                    if( TRACE* trace = plotPanel->GetTrace( vectorName, traceType | subType ) )
                        traceMap[ trace ] = { vectorName, traceType };
                }
            }
            else
            {
                if( TRACE* trace = plotPanel->GetTrace( vectorName, traceType ) )
                    traceMap[ trace ] = { vectorName, traceType };
            }
        }

        // Two passes so that DC-sweep sub-traces get deleted and re-created:

        for( const auto& [ trace, traceInfo ] : traceMap )
        {
            if( traceInfo.first.IsEmpty() )
                plotPanel->DeleteTrace( trace );
        }

        for( const auto& [ trace, traceInfo ] : traceMap )
        {
            if( !traceInfo.first.IsEmpty() )
                updateTrace( traceInfo.first, traceInfo.second, plotPanel );
        }

        rebuildSignalsGrid( m_filter->GetValue() );
        updateSignalsGrid();

        plotPanel->GetPlotWin()->UpdateAll();
        plotPanel->ResetScales();
        plotPanel->GetPlotWin()->Fit();
    }
    else if( simType == ST_OP )
    {
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
    }

    m_schematicFrame->RefreshOperatingPointDisplay();
    m_schematicFrame->GetCanvas()->Refresh();

    updateCursors();

    for( int row = 0; row < m_measurementsGrid->GetNumberRows(); ++row )
        UpdateMeasurement( row );

    m_lastSimPlot = plotPanelWindow;
}


void SIMULATOR_FRAME::onSimUpdate( wxCommandEvent& aEvent )
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


void SIMULATOR_FRAME::onSimReport( wxCommandEvent& aEvent )
{
    m_simConsole->AppendText( aEvent.GetString() + "\n" );
    m_simConsole->SetInsertionPointEnd();
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
}


wxDEFINE_EVENT( EVT_SIM_UPDATE, wxCommandEvent );
wxDEFINE_EVENT( EVT_SIM_REPORT, wxCommandEvent );

wxDEFINE_EVENT( EVT_SIM_STARTED, wxCommandEvent );
wxDEFINE_EVENT( EVT_SIM_FINISHED, wxCommandEvent );
