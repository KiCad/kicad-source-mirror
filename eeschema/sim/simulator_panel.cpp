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

#include <project/project_file.h>
#include <sch_edit_frame.h>
#include <confirm.h>
#include <wildcards_and_files_ext.h>
#include <widgets/tuner_slider.h>
#include <widgets/grid_color_swatch_helpers.h>
#include <widgets/wx_grid.h>
#include <grid_tricks.h>
#include <eda_pattern_match.h>
#include <string_utils.h>
#include <pgm_base.h>
#include "ngspice.h"
#include "simulator_panel.h"
#include <sim/simulator_frame.h>
#include <sim/sim_plot_panel.h>
#include <sim/spice_simulator.h>
#include "fmt/format.h"
#include <dialogs/dialog_sim_format_value.h>
#include <eeschema_settings.h>

#include <memory>


SIM_TRACE_TYPE operator|( SIM_TRACE_TYPE aFirst, SIM_TRACE_TYPE aSecond )
{
    int res = (int) aFirst | (int) aSecond;

    return (SIM_TRACE_TYPE) res;
}


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
    SIGNALS_GRID_TRICKS( SIMULATOR_PANEL* aParent, WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid ),
            m_parent( aParent ),
            m_menuRow( 0 ),
            m_menuCol( 0 )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    SIMULATOR_PANEL* m_parent;
    int              m_menuRow;
    int              m_menuCol;
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
    CURSORS_GRID_TRICKS( SIMULATOR_PANEL* aParent, WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid ),
            m_parent( aParent ),
            m_menuRow( 0 ),
            m_menuCol( 0 )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    SIMULATOR_PANEL* m_parent;
    int              m_menuRow;
    int              m_menuCol;
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
    MEASUREMENTS_GRID_TRICKS( SIMULATOR_PANEL* aParent, WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid ),
            m_parent( aParent ),
            m_menuRow( 0 ),
            m_menuCol( 0 )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    SIMULATOR_PANEL* m_parent;
    int              m_menuRow;
    int              m_menuCol;
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
    SUPPRESS_GRID_CELL_EVENTS( SIMULATOR_PANEL* aFrame ) :
            m_frame( aFrame )
    {
        m_frame->m_SuppressGridEvents++;
    }

    ~SUPPRESS_GRID_CELL_EVENTS()
    {
        m_frame->m_SuppressGridEvents--;
    }

private:
    SIMULATOR_PANEL* m_frame;
};


SIMULATOR_PANEL::SIMULATOR_PANEL( SIMULATOR_FRAME* aSimulatorFrame,
                                  SCH_EDIT_FRAME* aSchematicFrame ) :
        SIMULATOR_PANEL_BASE( aSimulatorFrame ),
        m_SuppressGridEvents( 0 ),
        m_simulatorFrame( aSimulatorFrame ),
        m_schematicFrame( aSchematicFrame ),
        m_darkMode( true ),
        m_plotNumber( 0 )
{
    // Get the previous size and position of windows:
    LoadSettings( m_schematicFrame->eeconfig() );

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

    Bind( EVT_SIM_CURSOR_UPDATE, &SIMULATOR_PANEL::onPlotCursorUpdate, this );
    Bind( EVT_WORKBOOK_MODIFIED, &SIMULATOR_PANEL::onNotebookModified, this );

#ifndef wxHAS_NATIVE_TABART
    // Default non-native tab art has ugly gradients we don't want
    m_plotNotebook->SetArtProvider( new wxAuiSimpleTabArt() );
#endif
}


SIMULATOR_PANEL::~SIMULATOR_PANEL()
{
    // Delete the GRID_TRICKS.
    m_signalsGrid->PopEventHandler( true );
    m_cursorsGrid->PopEventHandler( true );
    m_measurementsGrid->PopEventHandler( true );
}


void SIMULATOR_PANEL::ShowChangedLanguage()
{
    for( int ii = 0; ii < (int) m_plotNotebook->GetPageCount(); ++ii )
    {
        auto plot = dynamic_cast<SIM_PLOT_PANEL_BASE*>( m_plotNotebook->GetPage( ii ) );

        wxCHECK( plot, /* void */ );

        plot->OnLanguageChanged();

        wxString pageTitle( simulator()->TypeToName( plot->GetType(), true ) );
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
    updatePlotCursors();

    for( TUNER_SLIDER* tuner : m_tuners )
        tuner->ShowChangedLanguage();
}


void SIMULATOR_PANEL::LoadSettings( EESCHEMA_SETTINGS* aCfg )
{
    // Read subwindows sizes (should be > 0 )
    m_splitterLeftRightSashPosition      = aCfg->m_Simulator.plot_panel_width;
    m_splitterPlotAndConsoleSashPosition = aCfg->m_Simulator.plot_panel_height;
    m_splitterSignalsSashPosition        = aCfg->m_Simulator.signal_panel_height;
    m_splitterCursorsSashPosition        = aCfg->m_Simulator.cursors_panel_height;
    m_splitterTuneValuesSashPosition     = aCfg->m_Simulator.measurements_panel_height;
    m_darkMode                           = !aCfg->m_Simulator.white_background;
}


void SIMULATOR_PANEL::SaveSettings( EESCHEMA_SETTINGS* aCfg )
{
    aCfg->m_Simulator.plot_panel_width          = m_splitterLeftRight->GetSashPosition();
    aCfg->m_Simulator.plot_panel_height         = m_splitterPlotAndConsole->GetSashPosition();
    aCfg->m_Simulator.signal_panel_height       = m_splitterSignals->GetSashPosition();
    aCfg->m_Simulator.cursors_panel_height      = m_splitterCursors->GetSashPosition();
    aCfg->m_Simulator.measurements_panel_height = m_splitterMeasurements->GetSashPosition();
    aCfg->m_Simulator.white_background          = !m_darkMode;
}


void SIMULATOR_PANEL::InitWorkbook()
{
    if( !simulator()->Settings()->GetWorkbookFilename().IsEmpty() )
    {
        wxFileName filename = simulator()->Settings()->GetWorkbookFilename();
        filename.SetPath( m_schematicFrame->Prj().GetProjectPath() );

        if( !LoadWorkbook( filename.GetFullPath() ) )
            simulator()->Settings()->SetWorkbookFilename( "" );
    }
    else if( m_simulatorFrame->LoadSimulator() )
    {
        if( !circuitModel()->GetSchTextSimCommand().IsEmpty() )
            NewPlotPanel( circuitModel()->GetSchTextSimCommand(), circuitModel()->GetSimOptions() );

        rebuildSignalsList();
        rebuildSignalsGrid( m_filter->GetValue() );
    }
}


void SIMULATOR_PANEL::SetSubWindowsSashSize()
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


void SIMULATOR_PANEL::rebuildSignalsGrid( wxString aFilter )
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


void SIMULATOR_PANEL::rebuildSignalsList()
{
    m_signals.clear();

    int      options = m_simulatorFrame->GetCurrentOptions();
    SIM_TYPE simType = m_simulatorFrame->GetCurrentSimType();
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
        for( const std::string& net : circuitModel()->GetNets() )
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
        for( const SPICE_ITEM& item : circuitModel()->GetItems() )
        {
            // Add all possible currents for the device.
            for( const std::string& name : item.model->SpiceGenerator().CurrentNames( item ) )
                addSignal( name );
        }
    }

    if( ( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS )
            && ( simType == ST_TRANSIENT || simType == ST_DC ) )
    {
        for( const SPICE_ITEM& item : circuitModel()->GetItems() )
        {
            if( item.model->GetPinCount() >= 2 )
            {
                wxString name = item.model->SpiceGenerator().ItemName( item );
                addSignal( wxString::Format( wxS( "P(%s)" ), name ) );
            }
        }
    }

    // Add .PROBE directives
    for( const wxString& directive : circuitModel()->GetDirectives() )
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


SIM_PLOT_PANEL_BASE* SIMULATOR_PANEL::NewPlotPanel( const wxString& aSimCommand, int aOptions )
{
    SIM_PLOT_PANEL_BASE* plotPanel = nullptr;
    SIM_TYPE             simType   = NGSPICE_CIRCUIT_MODEL::CommandToSimType( aSimCommand );

    if( SIM_PLOT_PANEL_BASE::IsPlottable( simType ) )
    {
        SIM_PLOT_PANEL* panel = new SIM_PLOT_PANEL( aSimCommand, aOptions, m_plotNotebook, wxID_ANY );
        plotPanel = panel;

        COMMON_SETTINGS::INPUT cfg = Pgm().GetCommonSettings()->m_Input;
        panel->GetPlotWin()->EnableMouseWheelPan( cfg.scroll_modifier_zoom != 0 );
    }
    else
    {
        plotPanel = new SIM_NOPLOT_PANEL( aSimCommand, aOptions, m_plotNotebook, wxID_ANY );
    }

    wxString pageTitle( simulator()->TypeToName( simType, true ) );
    pageTitle.Prepend( wxString::Format( _( "Plot%u - " ), (unsigned int) ++m_plotNumber ) );

    m_plotNotebook->AddPage( dynamic_cast<wxWindow*>( plotPanel ), pageTitle, true );

    m_simulatorFrame->OnModify();
    return plotPanel;
}


void SIMULATOR_PANEL::OnFilterText( wxCommandEvent& aEvent )
{
    rebuildSignalsGrid( m_filter->GetValue() );
}


void SIMULATOR_PANEL::OnFilterMouseMoved( wxMouseEvent& aEvent )
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
wxString SIMULATOR_PANEL::vectorNameFromSignalName( const wxString& aSignalName, int* aTraceType )
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


void SIMULATOR_PANEL::onSignalsGridCellChanged( wxGridEvent& aEvent )
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
        updatePlotCursors();
        m_simulatorFrame->OnModify();
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
            m_simulatorFrame->OnModify();
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
            m_simulatorFrame->OnModify();
        }

        // Update cursor checkboxes (which are really radio buttons)
        updateSignalsGrid();
    }
}


void SIMULATOR_PANEL::onCursorsGridCellChanged( wxGridEvent& aEvent )
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

        updatePlotCursors();
        m_simulatorFrame->OnModify();
    }
    else
    {
        wxFAIL_MSG( wxT( "All other columns are supposed to be read-only!" ) );
    }
}


SPICE_VALUE_FORMAT SIMULATOR_PANEL::GetMeasureFormat( int aRow ) const
{
    SPICE_VALUE_FORMAT result;
    result.FromString( m_measurementsGrid->GetCellValue( aRow, COL_MEASUREMENT_FORMAT ) );
    return result;
}


void SIMULATOR_PANEL::SetMeasureFormat( int aRow, const SPICE_VALUE_FORMAT& aFormat )
{
    m_measurementsGrid->SetCellValue( aRow, COL_MEASUREMENT_FORMAT, aFormat.ToString() );
    m_simulatorFrame->OnModify();
}


void SIMULATOR_PANEL::DeleteMeasurement( int aRow )
{
    if( aRow < ( m_measurementsGrid->GetNumberRows() - 1 ) )
    {
        m_measurementsGrid->DeleteRows( aRow, 1 );
        m_simulatorFrame->OnModify();
    }
}


void SIMULATOR_PANEL::onMeasurementsGridCellChanged( wxGridEvent& aEvent )
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
        m_simulatorFrame->OnModify();
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
void SIMULATOR_PANEL::UpdateMeasurement( int aRow )
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

    wxString simType = simulator()->TypeToName( plotPanel->GetType(), true );
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

    if( m_simulatorFrame->SimFinished() )
    {
        wxString cmd = wxString::Format( wxS( "meas %s %s %s" ), simType, resultName, text );
        simulator()->Command( "echo " + cmd.ToStdString() );
        simulator()->Command( cmd.ToStdString() );

        std::vector<double> resultVec = simulator()->GetMagPlot( resultName.ToStdString() );

        if( resultVec.size() > 0 )
            result = SPICE_VALUE( resultVec[0] ).ToString( GetMeasureFormat( aRow ) );
    }

    m_measurementsGrid->SetCellValue( aRow, COL_MEASUREMENT_VALUE, result );
}


void SIMULATOR_PANEL::AddTuner( const SCH_SHEET_PATH& aSheetPath, SCH_SYMBOL* aSymbol )
{
    SIM_PLOT_PANEL_BASE* plotPanel = GetCurrentPlotWindow();

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
        m_simulatorFrame->OnModify();
    }
    catch( const KI_PARAM_ERROR& e )
    {
        DisplayErrorMessage( nullptr, e.What() );
    }
}


void SIMULATOR_PANEL::UpdateTunerValue( const SCH_SHEET_PATH& aSheetPath, const KIID& aSymbol,
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

    SIM_LIB_MGR mgr( &m_schematicFrame->Prj() );
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


void SIMULATOR_PANEL::RemoveTuner( TUNER_SLIDER* aTuner )
{
    m_tuners.remove( aTuner );
    aTuner->Destroy();
    m_panelTuners->Layout();
    m_simulatorFrame->OnModify();
}


void SIMULATOR_PANEL::AddMeasurement( const wxString& aCmd )
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

    wxString simType = simulator()->TypeToName( plotPanel->GetType(), true );
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
    m_simulatorFrame->OnModify();

    // Always leave at least one empty row for type-in:
    row = m_measurementsGrid->GetNumberRows() - 1;

    if( !m_measurementsGrid->GetCellValue( row, COL_MEASUREMENT ).IsEmpty() )
        m_measurementsGrid->AppendRows( 1 );
}


const NGSPICE_CIRCUIT_MODEL* SIMULATOR_PANEL::GetExporter() const
{
    return circuitModel().get();
}


void SIMULATOR_PANEL::AddTrace( const wxString& aName, SIM_TRACE_TYPE aType )
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
    else if( !SIM_PLOT_PANEL_BASE::IsPlottable( simType ) )
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
    m_simulatorFrame->OnModify();
}


void SIMULATOR_PANEL::SetUserDefinedSignals( const std::map<int, wxString>& aNewSignals )
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

    if( m_simulatorFrame->SimFinished() )
        applyUserDefinedSignals();

    rebuildSignalsList();
    rebuildSignalsGrid( m_filter->GetValue() );
    updateSignalsGrid();
    updatePlotCursors();
    m_simulatorFrame->OnModify();
}


void SIMULATOR_PANEL::updateTrace( const wxString& aVectorName, int aTraceType,
                                  SIM_PLOT_PANEL* aPlotPanel )
{
    SIM_TYPE simType = NGSPICE_CIRCUIT_MODEL::CommandToSimType( aPlotPanel->GetSimCommand() );

    aTraceType &= aTraceType & SPT_Y_AXIS_MASK;
    aTraceType |= getXAxisType( simType );

    wxString simVectorName = aVectorName;

    if( aTraceType & SPT_POWER )
        simVectorName = simVectorName.AfterFirst( '(' ).BeforeLast( ')' ) + wxS( ":power" );

    if( !SIM_PLOT_PANEL_BASE::IsPlottable( simType ) )
    {
        // There is no plot to be shown
        simulator()->Command( wxString::Format( wxT( "print %s" ), aVectorName ).ToStdString() );

        return;
    }

    // First, handle the x axis
    wxString xAxisName( simulator()->GetXAxis( simType ) );

    if( xAxisName.IsEmpty() )
        return;

    std::vector<double> data_x;
    std::vector<double> data_y;

    if( m_simulatorFrame->SimFinished() )
    {
        data_x = simulator()->GetMagPlot( (const char*) xAxisName.c_str() );

        switch( simType )
        {
        case ST_AC:
            if( aTraceType & SPT_AC_MAG )
                data_y = simulator()->GetMagPlot( (const char*) simVectorName.c_str() );
            else if( aTraceType & SPT_AC_PHASE )
                data_y = simulator()->GetPhasePlot( (const char*) simVectorName.c_str() );
            else
                wxFAIL_MSG( wxT( "Plot type missing AC_PHASE or AC_MAG bit" ) );

            break;

        case ST_NOISE:
        case ST_DC:
        case ST_TRANSIENT:
            data_y = simulator()->GetMagPlot( (const char*) simVectorName.c_str() );
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
        && circuitModel()->ParseDCCommand( circuitModel()->GetSimCommand(), &source1, &source2 )
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


void SIMULATOR_PANEL::updateSignalsGrid()
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


void SIMULATOR_PANEL::applyUserDefinedSignals()
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

        simulator()->Command( "echo " + fmt::format( cmd, id, signal.ToStdString() ) );
        simulator()->Command( fmt::format( cmd, id, quoteNetNames( signal ).ToStdString() ) );
    }
}


void SIMULATOR_PANEL::applyTuners()
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

        simulator()->Command( item->model->SpiceGenerator().TunerCommand( *item, floatVal ) );
    }

    if( reporter.HasMessage() )
        DisplayErrorMessage( this, _( "Could not apply tuned value(s):" ) + wxS( "\n" ) + errors );
}


void SIMULATOR_PANEL::parseTraceParams( SIM_PLOT_PANEL* aPlotPanel, TRACE* aTrace,
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


bool SIMULATOR_PANEL::LoadWorkbook( const wxString& aPath )
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

    m_simulatorFrame->LoadSimulator();

    wxString schTextSimCommand = circuitModel()->GetSchTextSimCommand().Lower();
    SIM_TYPE schTextSimType = NGSPICE_CIRCUIT_MODEL::CommandToSimType( schTextSimCommand );

    if( schTextSimType != ST_UNKNOWN )
    {
        bool found = false;

        for( int ii = 0; ii < (int) m_plotNotebook->GetPageCount(); ++ii )
        {
            auto* plot = dynamic_cast<const SIM_PLOT_PANEL_BASE*>( m_plotNotebook->GetPage( ii ) );

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
            NewPlotPanel( schTextSimCommand, circuitModel()->GetSimOptions() );
    }

    rebuildSignalsList();

    rebuildSignalsGrid( m_filter->GetValue() );
    updateSignalsGrid();
    updatePlotCursors();

    file.Close();

    wxFileName filename( aPath );
    filename.MakeRelativeTo( m_schematicFrame->Prj().GetProjectPath() );

    // Remember the loaded workbook filename.
    simulator()->Settings()->SetWorkbookFilename( filename.GetFullPath() );

    return true;
}


bool SIMULATOR_PANEL::SaveWorkbook( const wxString& aPath )
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
        auto* basePanel = dynamic_cast<const SIM_PLOT_PANEL_BASE*>( m_plotNotebook->GetPage( i ) );

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
        filename.MakeRelativeTo( m_schematicFrame->Prj().GetProjectPath() );
        simulator()->Settings()->SetWorkbookFilename( filename.GetFullPath() );
    }

    return res;
}


SIM_TRACE_TYPE SIMULATOR_PANEL::getXAxisType( SIM_TYPE aType ) const
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


void SIMULATOR_PANEL::ToggleDarkModePlots()
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


void SIMULATOR_PANEL::onPlotClose( wxAuiNotebookEvent& event )
{
}


void SIMULATOR_PANEL::onPlotClosed( wxAuiNotebookEvent& event )
{
    CallAfter( [this]()
               {
                   rebuildSignalsList();
                   rebuildSignalsGrid( m_filter->GetValue() );
                   updatePlotCursors();

                   SIM_PLOT_PANEL_BASE* panel = GetCurrentPlotWindow();

                   if( !panel || panel->GetType() != ST_OP )
                   {
                       SCHEMATIC& schematic = m_schematicFrame->Schematic();
                       schematic.ClearOperatingPoints();
                       m_schematicFrame->RefreshOperatingPointDisplay();
                       m_schematicFrame->GetCanvas()->Refresh();
                   }
               } );
}


void SIMULATOR_PANEL::onPlotChanged( wxAuiNotebookEvent& event )
{
    rebuildSignalsList();
    rebuildSignalsGrid( m_filter->GetValue() );
    updatePlotCursors();
}


void SIMULATOR_PANEL::onPlotDragged( wxAuiNotebookEvent& event )
{
}


void SIMULATOR_PANEL::onNotebookModified( wxCommandEvent& event )
{
    m_simulatorFrame->OnModify();
    m_simulatorFrame->UpdateTitle();
}


std::shared_ptr<SPICE_SIMULATOR> SIMULATOR_PANEL::simulator() const
{
    return m_simulatorFrame->GetSimulator();
}


std::shared_ptr<NGSPICE_CIRCUIT_MODEL> SIMULATOR_PANEL::circuitModel() const
{
    return m_simulatorFrame->GetCircuitModel();
}


void SIMULATOR_PANEL::updatePlotCursors()
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
                if( !m_simulatorFrame->SimFinished() && aCol == 1 )
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


void SIMULATOR_PANEL::onPlotCursorUpdate( wxCommandEvent& aEvent )
{
    updatePlotCursors();
    m_simulatorFrame->OnModify();
}


void SIMULATOR_PANEL::OnSimUpdate()
{
    // Incremental update
    m_simConsole->Clear();

    // Do not export netlist, it is already stored in the simulator
    applyTuners();
}


void SIMULATOR_PANEL::OnSimReport( const wxString& aMsg )
{
    m_simConsole->AppendText( aMsg + "\n" );
    m_simConsole->SetInsertionPointEnd();
}


void SIMULATOR_PANEL::OnSimFinished()
{
    SIM_TYPE             simType = circuitModel()->GetSimType();
    SIM_PLOT_PANEL_BASE* plotPanelWindow = GetCurrentPlotWindow();

    if( !plotPanelWindow || plotPanelWindow->GetType() != simType )
    {
        plotPanelWindow = NewPlotPanel( circuitModel()->GetSimCommand(),
                                        circuitModel()->GetSimOptions() );
    }

    std::vector<wxString> oldSignals = m_signals;

    applyUserDefinedSignals();
    rebuildSignalsList();

    // If there are any signals plotted, update them
    if( SIM_PLOT_PANEL_BASE::IsPlottable( simType ) )
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

        for( const std::string& vec : simulator()->AllPlots() )
        {
            std::vector<double> val_list = simulator()->GetRealPlot( vec, 1 );

            if( val_list.size() == 0 )      // The list of values can be empty!
                continue;

            wxString       value = SPICE_VALUE( val_list.at( 0 ) ).ToSpiceString();
            wxString       msg;
            wxString       signal;
            SIM_TRACE_TYPE type = circuitModel()->VectorToSignal( vec, signal );

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

            m_schematicFrame->Schematic().SetOperatingPoint( signal, val_list.at( 0 ) );
        }
    }

    updatePlotCursors();

    for( int row = 0; row < m_measurementsGrid->GetNumberRows(); ++row )
        UpdateMeasurement( row );
}


