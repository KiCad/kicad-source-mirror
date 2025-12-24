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

#include <algorithm>
#include <memory>
#include <type_traits>

#include <wx/event.h>
#include <fmt/format.h>
#include <wx/wfstream.h>
#include <wx/stdstream.h>
#include <wx/debug.h>
#include <wx/clipbrd.h>
#include <wx/log.h>

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
#include <sim/simulator_frame_ui.h>
#include <sim/simulator_frame.h>
#include <sim/sim_plot_tab.h>
#include <sim/spice_simulator.h>
#include <dialogs/dialog_text_entry.h>
#include <dialogs/dialog_sim_format_value.h>
#include <eeschema_settings.h>
#include <advanced_config.h>
#include <magic_enum.hpp>


SIM_TRACE_TYPE operator|( SIM_TRACE_TYPE aFirst, SIM_TRACE_TYPE aSecond )
{
    int res = static_cast<int>( aFirst ) | static_cast<int>( aSecond);

    return static_cast<SIM_TRACE_TYPE>( res );
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
    MYID_FOURIER,

    MYID_FORMAT_VALUE,
    MYID_DELETE_MEASUREMENT
};


class SIGNALS_GRID_TRICKS : public GRID_TRICKS
{
public:
    SIGNALS_GRID_TRICKS( SIMULATOR_FRAME_UI* aParent, WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid ),
            m_parent( aParent ),
            m_menuRow( 0 ),
            m_menuCol( 0 )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    SIMULATOR_FRAME_UI* m_parent;
    int                 m_menuRow;
    int                 m_menuCol;
};


void SIGNALS_GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    SIM_TAB* panel = m_parent->GetCurrentSimTab();

    if( !panel )
        return;

    m_menuRow = aEvent.GetRow();
    m_menuCol = aEvent.GetCol();

    if( m_menuCol == COL_SIGNAL_NAME )
    {
        if( !( m_grid->IsInSelection( m_menuRow, m_menuCol ) ) )
            m_grid->ClearSelection();

        m_grid->SetGridCursor( m_menuRow, m_menuCol );

        if( panel->GetSimType() == ST_TRAN || panel->GetSimType() == ST_AC
            || panel->GetSimType() == ST_DC || panel->GetSimType() == ST_SP )
        {
            menu.Append( MYID_MEASURE_MIN, _( "Measure Min" ) );
            menu.Append( MYID_MEASURE_MAX, _( "Measure Max" ) );
            menu.Append( MYID_MEASURE_AVG, _( "Measure Average" ) );
            menu.Append( MYID_MEASURE_RMS, _( "Measure RMS" ) );
            menu.Append( MYID_MEASURE_PP, _( "Measure Peak-to-peak" ) );

            if( panel->GetSimType() == ST_AC || panel->GetSimType() == ST_SP )
            {
                menu.Append( MYID_MEASURE_MIN_AT, _( "Measure Frequency of Min" ) );
                menu.Append( MYID_MEASURE_MAX_AT, _( "Measure Frequency of Max" ) );
            }
            else
            {
                menu.Append( MYID_MEASURE_MIN_AT, _( "Measure Time of Min" ) );
                menu.Append( MYID_MEASURE_MAX_AT, _( "Measure Time of Max" ) );
            }

            menu.Append( MYID_MEASURE_INTEGRAL, _( "Measure Integral" ) );

            if( panel->GetSimType() == ST_TRAN )
            {
                menu.AppendSeparator();
                menu.Append( MYID_FOURIER, _( "Perform Fourier Analysis..." ) );
            }

            menu.AppendSeparator();
            menu.Append( GRIDTRICKS_ID_COPY, _( "Copy Signal Name" ) + "\tCtrl+C" );

            menu.AppendSeparator();
            menu.Append( GRIDTRICKS_ID_SELECT, _( "Create new cursor..." ) );

            m_grid->PopupMenu( &menu );
        }
    }
    else if( m_menuCol > static_cast<int>( COL_CURSOR_2 ) )
    {
        menu.Append( GRIDTRICKS_ID_SELECT, _( "Create new cursor..." ) );

        menu.AppendSeparator();

        wxString msg = m_grid->GetColLabelValue( m_grid->GetNumberCols() - 1 );

        menu.AppendSeparator();
        menu.Append( GRIDTRICKS_ID_DELETE, wxString::Format( _( "Delete %s..." ), msg ) );

        m_grid->PopupMenu( &menu );
    }
    else
    {
        menu.Append( GRIDTRICKS_ID_SELECT, _( "Create new cursor..." ) );

        m_grid->PopupMenu( &menu );
    }
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

    auto addMeasurement =
            [this]( const wxString& cmd, wxString signal )
            {
                if( signal.EndsWith( _( " (phase)" ) ) )
                    return;

                if( signal.EndsWith( _( " (gain)" ) ) || signal.EndsWith( _( " (amplitude)" ) ) )
                {
                    signal = signal.Left( signal.length() - 7 );

                    if( signal.Upper().StartsWith( wxS( "V(" ) ) )
                        signal = wxS( "vdb" ) + signal.Mid( 1 );
                }

                m_parent->AddMeasurement( cmd + wxS( " " ) + signal );
            };

    if( event.GetId() == MYID_MEASURE_MIN )
    {
        for( const wxString& signal : signals )
            addMeasurement( wxS( "MIN" ), signal );
    }
    else if( event.GetId() == MYID_MEASURE_MAX )
    {
        for( const wxString& signal : signals )
            addMeasurement( wxS( "MAX" ), signal );
    }
    else if( event.GetId() == MYID_MEASURE_AVG )
    {
        for( const wxString& signal : signals )
            addMeasurement( wxS( "AVG" ), signal );
    }
    else if( event.GetId() == MYID_MEASURE_RMS )
    {
        for( const wxString& signal : signals )
            addMeasurement( wxS( "RMS" ), signal );
    }
    else if( event.GetId() == MYID_MEASURE_PP )
    {
        for( const wxString& signal : signals )
            addMeasurement( wxS( "PP" ), signal );
    }
    else if( event.GetId() == MYID_MEASURE_MIN_AT )
    {
        for( const wxString& signal : signals )
            addMeasurement( wxS( "MIN_AT" ), signal );
    }
    else if( event.GetId() == MYID_MEASURE_MAX_AT )
    {
        for( const wxString& signal : signals )
            addMeasurement( wxS( "MAX_AT" ), signal );
    }
    else if( event.GetId() == MYID_MEASURE_INTEGRAL )
    {
        for( const wxString& signal : signals )
            addMeasurement( wxS( "INTEG" ), signal );
    }
    else if( event.GetId() == MYID_FOURIER )
    {
        wxString title;
        wxString fundamental = wxT( "1K" );

        if( signals.size() == 1 )
            title.Printf( _( "Fourier Analysis of %s" ), signals[0] );
        else
            title = _( "Fourier Analyses of Multiple Signals" );

        WX_TEXT_ENTRY_DIALOG dlg( m_parent, _( "Fundamental frequency:" ), title, fundamental );

        if( dlg.ShowModal() != wxID_OK )
            return;

        if( !dlg.GetValue().IsEmpty() )
            fundamental = dlg.GetValue();

        for( const wxString& signal : signals )
            m_parent->DoFourier( signal, fundamental );
    }
    else if( event.GetId() == GRIDTRICKS_ID_COPY )
    {
        wxLogNull doNotLog; // disable logging of failed clipboard actions
        wxString  txt;

        for( const wxString& signal : signals )
        {
            if( !txt.IsEmpty() )
                txt += '\r';

            txt += signal;
        }

        if( wxTheClipboard->Open() )
        {
            wxTheClipboard->SetData( new wxTextDataObject( txt ) );
            wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
            wxTheClipboard->Close();
        }
    }
    else if( event.GetId() == GRIDTRICKS_ID_SELECT )
    {
        m_parent->CreateNewCursor();
    }
    else if( event.GetId() == GRIDTRICKS_ID_DELETE )
    {
        m_parent->DeleteCursor();
    }
}


class CURSORS_GRID_TRICKS : public GRID_TRICKS
{
public:
    CURSORS_GRID_TRICKS( SIMULATOR_FRAME_UI* aParent, WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid ),
            m_parent( aParent ),
            m_menuRow( 0 ),
            m_menuCol( 0 )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    SIMULATOR_FRAME_UI* m_parent;
    int                 m_menuRow;
    int                 m_menuCol;
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
    auto getSignalName =
            [this]( int row ) -> wxString
            {
                wxString signal = m_grid->GetCellValue( row, COL_CURSOR_SIGNAL );

                if( signal.EndsWith( "[2 - 1]" ) )
                    signal = signal.Left( signal.length() - 7 );

                return signal;
            };

    if( event.GetId() == MYID_FORMAT_VALUE )
    {
        int                     axis = m_menuCol - COL_CURSOR_X;
        SPICE_VALUE_FORMAT      format = m_parent->GetCursorFormat( m_menuRow, axis );
        DIALOG_SIM_FORMAT_VALUE formatDialog( m_parent, &format );

        if( formatDialog.ShowModal() == wxID_OK )
        {
            for( int row = 0; row < m_grid->GetNumberRows(); ++row )
            {
                if( getSignalName( row ) == getSignalName( m_menuRow ) )
                    m_parent->SetCursorFormat( row, axis, format );
            }
        }
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


class MEASUREMENTS_GRID_TRICKS : public GRID_TRICKS
{
public:
    MEASUREMENTS_GRID_TRICKS( SIMULATOR_FRAME_UI* aParent, WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid ),
            m_parent( aParent ),
            m_menuRow( 0 ),
            m_menuCol( 0 )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    SIMULATOR_FRAME_UI* m_parent;
    int                 m_menuRow;
    int                 m_menuCol;
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
            m_parent->OnModify();
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
                    measurements.push_back( j );
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

        m_parent->OnModify();
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


class SUPPRESS_GRID_CELL_EVENTS
{
public:
    SUPPRESS_GRID_CELL_EVENTS( SIMULATOR_FRAME_UI* aFrame ) :
            m_frame( aFrame )
    {
        m_frame->m_SuppressGridEvents++;
    }

    ~SUPPRESS_GRID_CELL_EVENTS()
    {
        m_frame->m_SuppressGridEvents--;
    }

private:
    SIMULATOR_FRAME_UI* m_frame;
};


#define ID_SIM_REFRESH 10207
#define REFRESH_INTERVAL 50   // 20 frames/second.


SIMULATOR_FRAME_UI::SIMULATOR_FRAME_UI( SIMULATOR_FRAME* aSimulatorFrame, SCH_EDIT_FRAME* aSchematicFrame ) :
        SIMULATOR_FRAME_UI_BASE( aSimulatorFrame ),
        m_SuppressGridEvents( 0 ),
        m_simulatorFrame( aSimulatorFrame ),
        m_schematicFrame( aSchematicFrame ),
        m_darkMode( true ),
        m_plotNumber( 0 ),
        m_refreshTimer( this, ID_SIM_REFRESH )
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

    CustomCursorsInit();

    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_measurementsGrid->SetColAttr( COL_MEASUREMENT_VALUE, attr );

    // Prepare the color list to plot traces
    SIM_PLOT_COLORS::FillDefaultColorList( m_darkMode );

    Bind( EVT_SIM_CURSOR_UPDATE, &SIMULATOR_FRAME_UI::onPlotCursorUpdate, this );

    Bind( wxEVT_TIMER,
            [&]( wxTimerEvent& aEvent )
            {
                OnSimRefresh( false );

                if( m_simulatorFrame->GetSimulator()->IsRunning() )
                    m_refreshTimer.Start( REFRESH_INTERVAL, wxTIMER_ONE_SHOT );
            },
            m_refreshTimer.GetId() );

#ifndef wxHAS_NATIVE_TABART
    // Default non-native tab art has ugly gradients we don't want
    m_plotNotebook->SetArtProvider( new wxAuiSimpleTabArt() );
#endif
}


SIMULATOR_FRAME_UI::~SIMULATOR_FRAME_UI()
{
    // Delete the GRID_TRICKS.
    m_signalsGrid->PopEventHandler( true );
    m_cursorsGrid->PopEventHandler( true );
    m_measurementsGrid->PopEventHandler( true );
}


void SIMULATOR_FRAME_UI::CustomCursorsInit()
{
    for( auto& m_cursorFormat : m_cursorFormats )
    {
        m_cursorFormat[0] = { 3, wxS( "~s" ) };
        m_cursorFormat[1] = { 3, wxS( "~V" ) };
    }

    // proper init and transfer/copy m_cursorFormats
    // we work on m_cursorFormatsDyn from now on.
    // TODO: rework +- LOC when m_cursorFormatsDyn and m_cursorFormats get merged.
    m_cursorFormatsDyn.clear();
    m_cursorFormatsDyn.resize( std::size( m_cursorFormats ) );

    for( size_t index = 0; index < std::size( m_cursorFormats ); index++ )
    {
        for( size_t index2 = 0; index2 < std::size( m_cursorFormats[0] ); index2++ )
            m_cursorFormatsDyn[index].push_back( m_cursorFormats[index][index2] );
    }

    // Dump string helper, tries to get the current higher cursor name to form the next one.
    // Based on the column labeling
    // TODO: "Cursor n" may translate as "n Cursor" in other languages
    //       TBD how to handle; just forbid for now.
    int nameMax = 0;

    for( int i = 0; i < m_signalsGrid->GetNumberCols(); i++ )
    {
        wxString maxCursor = m_signalsGrid->GetColLabelValue( i );

        maxCursor.Replace( _( "Cursor " ), "" );

        int tmpMax = wxAtoi( maxCursor );

        if( nameMax < tmpMax )
            nameMax = tmpMax;
    }

    m_customCursorsCnt = nameMax + 1; // Init with a +1 on top of current cursor 2, defaults to 3
}


void SIMULATOR_FRAME_UI::CreateNewCursor()
{
    std::vector<SPICE_VALUE_FORMAT> tmp;
    // m_cursorFormatsDyn should be equal with m_cursorFormats on first entry here.
    m_cursorFormatsDyn.emplace_back( tmp );

    m_cursorFormatsDyn[m_customCursorsCnt].push_back( { 3, wxS( "~s" ) } );
    m_cursorFormatsDyn[m_customCursorsCnt].push_back( { 3, wxS( "~V" ) } );

    wxString cursor_name = wxString( _( "Cursor " ) ) << m_customCursorsCnt;

    m_signalsGrid->InsertCols( m_signalsGrid->GetNumberCols() , 1, true );
    m_signalsGrid->SetColLabelValue( m_signalsGrid->GetNumberCols() - 1, cursor_name );

    wxGridCellAttr* attr = new wxGridCellAttr;
    m_signalsGrid->SetColAttr( COL_CURSOR_2 + m_customCursorsCnt, attr );

    m_customCursorsCnt++;

    updateSignalsGrid();
    updatePlotCursors();
    OnModify();
}


void SIMULATOR_FRAME_UI::DeleteCursor()
{
    int col = m_signalsGrid->GetNumberCols();
    int rows = m_signalsGrid->GetNumberRows();

    if( col > COL_CURSOR_2 )
    {
        // Now we need to find the active cursor and deactivate before removing the column,
        // Send the dummy event to update the UI
        for( int i = 0; i < rows; i++ )
        {
            if( m_signalsGrid->GetCellValue( i, col - 1 ) == wxS( "1" ) )
            {
                m_signalsGrid->SetCellValue( i, col - 1, wxEmptyString );
                wxGridEvent aDummy( wxID_ANY, wxEVT_GRID_CELL_CHANGED, m_signalsGrid, i, col - 1 );
                onSignalsGridCellChanged( aDummy );
                break;
            }

        }

        m_signalsGrid->DeleteCols( col - 1, 1, false );
        m_cursorFormatsDyn.pop_back();
        m_customCursorsCnt--;
        m_plotNotebook->Refresh();
        updateSignalsGrid();
        updatePlotCursors();
        OnModify();
    }
}


void SIMULATOR_FRAME_UI::ShowChangedLanguage()
{
    for( int ii = 0; ii < static_cast<int>( m_plotNotebook->GetPageCount() ); ++ii )
    {
        if( SIM_TAB* simTab = dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( ii ) ) )
        {
            simTab->OnLanguageChanged();

            wxString pageTitle( simulator()->TypeToName( simTab->GetSimType(), true ) );
            pageTitle.Prepend( wxString::Format( _( "Analysis %u - " ), ii+1 /* 1-based */ ) );

            m_plotNotebook->SetPageText( ii, pageTitle );
        }
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


void SIMULATOR_FRAME_UI::LoadSettings( EESCHEMA_SETTINGS* aCfg )
{
    const EESCHEMA_SETTINGS::SIMULATOR& settings = aCfg->m_Simulator;

    // Read subwindows sizes (should be > 0 )
    m_splitterLeftRightSashPosition      = settings.view.plot_panel_width;
    m_splitterPlotAndConsoleSashPosition = settings.view.plot_panel_height;
    m_splitterSignalsSashPosition        = settings.view.signal_panel_height;
    m_splitterCursorsSashPosition        = settings.view.cursors_panel_height;
    m_splitterTuneValuesSashPosition     = settings.view.measurements_panel_height;
    m_darkMode                           = !settings.view.white_background;

    m_preferences = settings.preferences;
}


void SIMULATOR_FRAME_UI::SaveSettings( EESCHEMA_SETTINGS* aCfg )
{
    EESCHEMA_SETTINGS::SIMULATOR& settings = aCfg->m_Simulator;

    settings.view.plot_panel_width          = m_splitterLeftRight->GetSashPosition();
    settings.view.plot_panel_height         = m_splitterPlotAndConsole->GetSashPosition();
    settings.view.signal_panel_height       = m_splitterSignals->GetSashPosition();
    settings.view.cursors_panel_height      = m_splitterCursors->GetSashPosition();
    settings.view.measurements_panel_height = m_splitterMeasurements->GetSashPosition();
    settings.view.white_background          = !m_darkMode;
}


void SIMULATOR_FRAME_UI::ApplyPreferences( const SIM_PREFERENCES& aPrefs )
{
    m_preferences = aPrefs;

    for( std::size_t i = 0; i < m_plotNotebook->GetPageCount(); ++i )
    {
        if( SIM_TAB* simTab = dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( i ) ) )
            simTab->ApplyPreferences( aPrefs );
    }
}


void SIMULATOR_FRAME_UI::InitWorkbook()
{
    if( !simulator()->Settings()->GetWorkbookFilename().IsEmpty() )
    {
        wxFileName filename = simulator()->Settings()->GetWorkbookFilename();
        filename.SetPath( m_schematicFrame->Prj().GetProjectPath() );

        if( !LoadWorkbook( filename.GetFullPath() ) )
            simulator()->Settings()->SetWorkbookFilename( "" );
    }
    else if( m_simulatorFrame->LoadSimulator( wxEmptyString, 0 ) )
    {
        wxString schTextSimCommand = circuitModel()->GetSchTextSimCommand();

        if( !schTextSimCommand.IsEmpty() )
        {
            SIM_TAB* simTab = NewSimTab( schTextSimCommand );
            simTab->SetSimOptions( NETLIST_EXPORTER_SPICE::OPTION_DEFAULT_FLAGS );
        }

        rebuildSignalsList();
        rebuildSignalsGrid( m_filter->GetValue() );
    }
}


void SIMULATOR_FRAME_UI::SetSubWindowsSashSize()
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


void sortSignals( std::vector<wxString>& signals )
{
    std::sort( signals.begin(), signals.end(),
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


void SIMULATOR_FRAME_UI::rebuildSignalsGrid( wxString aFilter )
{
    SUPPRESS_GRID_CELL_EVENTS raii( this );

    m_signalsGrid->ClearRows();

    SIM_PLOT_TAB*  plotPanel = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotPanel )
        return;

    SIM_TYPE              simType = plotPanel->GetSimType();
    std::vector<wxString> signals;

    if( plotPanel->GetSimType() == ST_FFT )
    {
        wxStringTokenizer tokenizer( plotPanel->GetSimCommand(), " \t\r\n", wxTOKEN_STRTOK );

        while( tokenizer.HasMoreTokens() && tokenizer.GetNextToken().Lower() != wxT( "fft" ) )
        {};

        while( tokenizer.HasMoreTokens() )
            signals.emplace_back( tokenizer.GetNextToken() );
    }
    else
    {
        // NB: m_signals are already broken out into gain/phase, but m_userDefinedSignals are
        // as the user typed them

        for( const wxString& signal : m_signals )
            signals.push_back( signal );

        for( const auto& [ id, signal ] : m_userDefinedSignals )
        {
            if( simType == ST_AC )
            {
                signals.push_back( signal + _( " (gain)" ) );
                signals.push_back( signal + _( " (phase)" ) );
            }
            else if( simType == ST_SP )
            {
                signals.push_back( signal + _( " (amplitude)" ) );
                signals.push_back( signal + _( " (phase)" ) );
            }
            else
            {
                signals.push_back( signal );
            }
        }

        sortSignals( signals );
    }

    if( aFilter.IsEmpty() )
        aFilter = wxS( "*" );

    EDA_COMBINED_MATCHER  matcher( aFilter.Upper(), CTX_SIGNAL );
    int                   row = 0;

    for( const wxString& signal : signals )
    {
        if( matcher.Find( signal.Upper() ) )
        {
            int      traceType = SPT_UNKNOWN;
            wxString vectorName = vectorNameFromSignalName( plotPanel, signal, &traceType );
            TRACE*   trace = plotPanel->GetTrace( vectorName, traceType );

            m_signalsGrid->AppendRows( 1 );
            m_signalsGrid->SetCellValue( row, COL_SIGNAL_NAME, signal );

            wxGridCellAttr* attr = new wxGridCellAttr;
            attr->SetRenderer( new wxGridCellBoolRenderer() );
            attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
            attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
            m_signalsGrid->SetAttr( row, COL_SIGNAL_SHOW, attr );

            if( !trace )
            {
                attr = new wxGridCellAttr;
                attr->SetReadOnly();
                m_signalsGrid->SetAttr( row, COL_SIGNAL_COLOR, attr );
                m_signalsGrid->SetCellValue( row, COL_SIGNAL_COLOR, wxEmptyString );

                attr = new wxGridCellAttr;
                attr->SetReadOnly();
                m_signalsGrid->SetAttr( row, COL_CURSOR_1, attr );

                attr = new wxGridCellAttr;
                attr->SetReadOnly();
                m_signalsGrid->SetAttr( row, COL_CURSOR_2, attr );

                if( m_customCursorsCnt > 3 )
                {
                    for( int i = 1; i <= m_customCursorsCnt - 3; i++ )
                    {
                        attr = new wxGridCellAttr;
                        attr->SetReadOnly();
                        m_signalsGrid->SetAttr( row, COL_CURSOR_2 + i, attr );
                    }
                }
            }
            else
            {
                m_signalsGrid->SetCellValue( row, COL_SIGNAL_SHOW, wxS( "1" ) );

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
                m_signalsGrid->SetCellValue( row, COL_CURSOR_1, trace->GetCursor( 1 ) ? "1" : "0" );

                attr = new wxGridCellAttr;
                attr->SetRenderer( new wxGridCellBoolRenderer() );
                attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
                attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
                m_signalsGrid->SetAttr( row, COL_CURSOR_2, attr );
                m_signalsGrid->SetCellValue( row, COL_CURSOR_2, trace->GetCursor( 2 ) ? "1" : "0" );

                if( m_customCursorsCnt > 3 )
                {
                    for( int i = 1; i <= m_customCursorsCnt - 3; i++ )
                    {
                        attr = new wxGridCellAttr;
                        attr->SetRenderer( new wxGridCellBoolRenderer() );
                        attr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS
                        attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
                        m_signalsGrid->SetAttr( row, COL_CURSOR_2 + i, attr );
                        m_signalsGrid->SetCellValue( row, COL_CURSOR_2 + i, trace->GetCursor( i ) ? "1" : "0" );
                    }
                }
            }
            row++;
        }
    }
}


void SIMULATOR_FRAME_UI::rebuildSignalsList()
{
    m_signals.clear();

    int      options = m_simulatorFrame->GetCurrentOptions();
    SIM_TYPE simType = m_simulatorFrame->GetCurrentSimType();
    wxString unconnected = wxString( wxS( "unconnected-(" ) );

    if( simType == ST_UNKNOWN )
        simType = ST_TRAN;

    unconnected.Replace( '(', '_' );    // Convert to SPICE markup

    auto addSignal =
            [&]( const wxString& aSignalName )
            {
                if( simType == ST_AC )
                {
                    m_signals.push_back( aSignalName + _( " (gain)" ) );
                    m_signals.push_back( aSignalName + _( " (phase)" ) );
                }
                else if( simType == ST_SP )
                {
                    m_signals.push_back( aSignalName + _( " (amplitude)" ) );
                    m_signals.push_back( aSignalName + _( " (phase)" ) );
                }
                else
                {
                    m_signals.push_back( aSignalName );
                }
            };

    if( ( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES )
            && ( simType == ST_TRAN || simType == ST_DC || simType == ST_AC || simType == ST_FFT) )
    {
        for( const wxString& net : circuitModel()->GetNets() )
        {
            // netnames are escaped (can contain "{slash}" for '/') Unscape them:
            wxString netname = UnescapeString( net );
            NETLIST_EXPORTER_SPICE::ConvertToSpiceMarkup( &netname );

            if( netname == "GND" || netname == "0" || netname.StartsWith( unconnected ) )
                continue;

            m_netnames.emplace_back( netname );
            addSignal( wxString::Format( wxS( "V(%s)" ), netname ) );
        }
    }

    if( ( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS )
            && ( simType == ST_TRAN || simType == ST_DC || simType == ST_AC ) )
    {
        for( const SPICE_ITEM& item : circuitModel()->GetItems() )
        {
            // Add all possible currents for the device.
            for( const std::string& name : item.model->SpiceGenerator().CurrentNames( item ) )
                addSignal( name );
        }
    }

    if( ( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS )
            && ( simType == ST_TRAN || simType == ST_DC ) )
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

    if( simType == ST_NOISE )
    {
        addSignal( wxS( "inoise_spectrum" ) );
        addSignal( wxS( "onoise_spectrum" ) );
    }

    if( simType == ST_SP )
    {
        std::vector<std::string> portnums;

        for( const SPICE_ITEM& item : circuitModel()->GetItems() )
        {
            wxString name = item.model->SpiceGenerator().ItemName( item );

            // We are only looking for voltage sources in .SP mode
            if( !name.StartsWith( "V" ) )
                continue;

            std::string portnum = "";

            if( const SIM_MODEL::PARAM* portnum_param = item.model->FindParam( "portnum" ) )
                portnum = SIM_VALUE::ToSpice( portnum_param->value );

            if( portnum != "" )
                portnums.push_back( portnum );
        }

        for( const std::string& portnum1 : portnums )
        {
            for( const std::string& portnum2 : portnums )
                addSignal( wxString::Format( wxS( "S_%s_%s" ), portnum1, portnum2 ) );
        }
    }

    // Add .SAVE and .PROBE directives
    for( const wxString& directive : circuitModel()->GetDirectives() )
    {
        wxStringTokenizer directivesTokenizer( directive, "\r\n", wxTOKEN_STRTOK );

        while( directivesTokenizer.HasMoreTokens() )
        {
            wxString line = directivesTokenizer.GetNextToken().Upper();
            wxString directiveParams;

            if( line.StartsWith( wxS( ".SAVE" ), &directiveParams )
                    || line.StartsWith( wxS( ".PROBE" ), &directiveParams ) )
            {
                wxStringTokenizer paramsTokenizer( directiveParams, " \t", wxTOKEN_STRTOK );

                while( paramsTokenizer.HasMoreTokens() )
                    addSignal( paramsTokenizer.GetNextToken() );
            }
        }
    }
}


SIM_TAB* SIMULATOR_FRAME_UI::NewSimTab( const wxString& aSimCommand )
{
    SIM_TAB* simTab = nullptr;
    SIM_TYPE simType = SPICE_CIRCUIT_MODEL::CommandToSimType( aSimCommand );

    if( SIM_TAB::IsPlottable( simType ) )
    {
        SIM_PLOT_TAB* panel = new SIM_PLOT_TAB( aSimCommand, m_plotNotebook );
        simTab = panel;
        panel->ApplyPreferences( m_preferences );
    }
    else
    {
        simTab = new SIM_NOPLOT_TAB( aSimCommand, m_plotNotebook );
    }

    wxString pageTitle( simulator()->TypeToName( simType, true ) );
    pageTitle.Prepend( wxString::Format( _( "Analysis %u - " ), static_cast<unsigned int>( ++m_plotNumber ) ) );

    m_plotNotebook->AddPage( simTab, pageTitle, true );

    return simTab;
}


void SIMULATOR_FRAME_UI::OnFilterText( wxCommandEvent& aEvent )
{
    rebuildSignalsGrid( m_filter->GetValue() );
}


void SIMULATOR_FRAME_UI::OnFilterMouseMoved( wxMouseEvent& aEvent )
{
#if defined( __WXOSX__ ) // Doesn't work properly on other ports
    wxPoint pos = aEvent.GetPosition();
    wxRect  ctrlRect = m_filter->GetScreenRect();
    int     buttonWidth = ctrlRect.GetHeight();         // Presume buttons are square

    if( m_filter->IsSearchButtonVisible() && pos.x < buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else if( m_filter->IsCancelButtonVisible() && pos.x > ctrlRect.GetWidth() - buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else
        SetCursor( wxCURSOR_IBEAM );
#endif
}


wxString vectorNameFromSignalId( int aUserDefinedSignalId )
{
    return wxString::Format( wxS( "user%d" ), aUserDefinedSignalId );
}


/**
 * For user-defined signals we display the user-oriented signal name such as "V(out)-V(in)",
 * but the simulator vector we actually have to plot will be "user0" or some-such.
 */
wxString SIMULATOR_FRAME_UI::vectorNameFromSignalName( SIM_PLOT_TAB* aPlotTab, const wxString& aSignalName,
                                                       int* aTraceType )
{
    auto looksLikePower = []( const wxString& aExpression ) -> bool
    {
        wxString exprUpper = aExpression.Upper();

        if( exprUpper.Contains( wxS( ":POWER" ) ) )
            return true;

        if( exprUpper.Find( '*' ) == wxNOT_FOUND )
            return false;

        if( !exprUpper.Contains( wxS( "V(" ) ) )
            return false;

        if( !exprUpper.Contains( wxS( "I(" ) ) )
            return false;

        return true;
    };

    std::map<wxString, int> suffixes;
    suffixes[ _( " (amplitude)" ) ] = SPT_SP_AMP;
    suffixes[ _( " (gain)" ) ] = SPT_AC_GAIN;
    suffixes[ _( " (phase)" ) ] = SPT_AC_PHASE;

    if( aTraceType )
    {
        if( aPlotTab && aPlotTab->GetSimType() == ST_NOISE )
        {
            if( getNoiseSource().Upper().StartsWith( 'I' ) )
                *aTraceType = SPT_CURRENT;
            else
                *aTraceType = SPT_VOLTAGE;
        }
        else
        {
            wxUniChar firstChar = aSignalName.Upper()[0];

            if( firstChar == 'V' )
                *aTraceType = SPT_VOLTAGE;
            else if( firstChar == 'I' )
                *aTraceType = SPT_CURRENT;
            else if( firstChar == 'P' )
                *aTraceType = SPT_POWER;
        }
    }

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
        {
            if( aTraceType && looksLikePower( signal ) )
            {
                int suffixBits = *aTraceType & ( SPT_AC_GAIN | SPT_AC_PHASE | SPT_SP_AMP );
                *aTraceType = suffixBits | SPT_POWER;
            }

            return vectorNameFromSignalId( id );
        }
    }

    return name;
};


void SIMULATOR_FRAME_UI::onSignalsGridCellChanged( wxGridEvent& aEvent )
{
    if( m_SuppressGridEvents > 0 )
        return;

    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
        return;

    int           row = aEvent.GetRow();
    int           col = aEvent.GetCol();
    wxString      text = m_signalsGrid->GetCellValue( row, col );
    wxString      signalName = m_signalsGrid->GetCellValue( row, COL_SIGNAL_NAME );
    int           traceType = SPT_UNKNOWN;
    wxString      vectorName = vectorNameFromSignalName( plotTab, signalName, &traceType );

    if( col == COL_SIGNAL_SHOW )
    {
        if( text == wxS( "1" ) )
            updateTrace( vectorName, traceType, plotTab );
        else
            plotTab->DeleteTrace( vectorName, traceType );

        plotTab->GetPlotWin()->UpdateAll();

        // Update enabled/visible states of other controls
        updateSignalsGrid();
        updatePlotCursors();
        OnModify();
    }
    else if( col == COL_SIGNAL_COLOR )
    {
        KIGFX::COLOR4D color( m_signalsGrid->GetCellValue( row, COL_SIGNAL_COLOR ) );
        TRACE*         trace = plotTab->GetTrace( vectorName, traceType );

        if( trace )
        {
            trace->SetTraceColour( color.ToColour() );
            plotTab->UpdateTraceStyle( trace );
            plotTab->UpdatePlotColors();
            OnModify();
        }
    }
    else if( col == COL_CURSOR_1 || col == COL_CURSOR_2
             || ( std::size( m_cursorFormatsDyn ) > std::size( m_cursorFormats ) && col > COL_CURSOR_2 ) )
    {
        int    id = col == COL_CURSOR_1 ? 1 : 2;

        if( col > COL_CURSOR_2 ) // TODO: clean up logic
        {
            id = col - 2; // enum SIGNALS_GRID_COLUMNS offset for Cursor n
        }

        TRACE* activeTrace = nullptr;

        if( text == wxS( "1" ) )
        {
            signalName = m_signalsGrid->GetCellValue( row, COL_SIGNAL_NAME );
            vectorName = vectorNameFromSignalName( plotTab, signalName, &traceType );
            activeTrace = plotTab->GetTrace( vectorName, traceType );

            if( activeTrace )
                plotTab->EnableCursor( activeTrace, id, signalName );

            OnModify();
        }

        // Turn off cursor on other signals.
        for( const auto& [name, trace] : plotTab->GetTraces() )
        {
            if( trace != activeTrace && trace->HasCursor( id ) )
            {
                plotTab->DisableCursor( trace, id );
                OnModify();
            }
        }

        // Update cursor checkboxes (which are really radio buttons)
        updateSignalsGrid();
    }
}


void SIMULATOR_FRAME_UI::onCursorsGridCellChanged( wxGridEvent& aEvent )
{
    if( m_SuppressGridEvents > 0 )
        return;

    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
        return;

    int      row = aEvent.GetRow();
    int      col = aEvent.GetCol();
    wxString text = m_cursorsGrid->GetCellValue( row, col );
    wxString cursorName = m_cursorsGrid->GetCellValue( row, COL_CURSOR_NAME );

    double value = SPICE_VALUE( text ).ToDouble();

    if( col == COL_CURSOR_X )
    {
        CURSOR* cursor1 = nullptr;
        CURSOR* cursor2 = nullptr;

        std::vector<CURSOR*> cursorsVec;
        cursorsVec.clear();

        for( const auto& [name, trace] : plotTab->GetTraces() )
        {
            if( CURSOR* cursor = trace->GetCursor( 1 ) )
                cursor1 = cursor;

            if( CURSOR* cursor = trace->GetCursor( 2 ) )
                cursor2 = cursor;

            int tmp = 3;

            if( !cursor1 )
                tmp--;
            if( !cursor2 )
                tmp--;

            for( int i = tmp; i < m_customCursorsCnt; i++ )
            {
                if( CURSOR* cursor = trace->GetCursor( i ) )
                {
                    cursorsVec.emplace_back( cursor );

                    if( cursorName == ( wxString( "" ) << i ) && cursor )
                        cursor->SetCoordX( value );
                }
            }
        }

        //double value = SPICE_VALUE( text ).ToDouble();

        if( cursorName == wxS( "1" ) && cursor1 )
            cursor1->SetCoordX( value );
        else if( cursorName == wxS( "2" ) && cursor2 )
            cursor2->SetCoordX( value );
        else if( cursorName == _( "Diff" ) && cursor1 && cursor2 )
            cursor2->SetCoordX( cursor1->GetCoords().x + value );

        updatePlotCursors();
        OnModify();
    }
    else
    {
        wxFAIL_MSG( wxT( "All other columns are supposed to be read-only!" ) );
    }
}


SPICE_VALUE_FORMAT SIMULATOR_FRAME_UI::GetMeasureFormat( int aRow ) const
{
    SPICE_VALUE_FORMAT result;
    result.FromString( m_measurementsGrid->GetCellValue( aRow, COL_MEASUREMENT_FORMAT ) );
    return result;
}


void SIMULATOR_FRAME_UI::SetMeasureFormat( int aRow, const SPICE_VALUE_FORMAT& aFormat )
{
    m_measurementsGrid->SetCellValue( aRow, COL_MEASUREMENT_FORMAT, aFormat.ToString() );
}


void SIMULATOR_FRAME_UI::DeleteMeasurement( int aRow )
{
    if( aRow < ( m_measurementsGrid->GetNumberRows() - 1 ) )
        m_measurementsGrid->DeleteRows( aRow, 1 );
}


void SIMULATOR_FRAME_UI::onMeasurementsGridCellChanged( wxGridEvent& aEvent )
{
    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
        return;

    int row = aEvent.GetRow();
    int col = aEvent.GetCol();

    if( col == COL_MEASUREMENT )
    {
        UpdateMeasurement( row );
        updateMeasurementsFromGrid();
        OnModify();
    }
    else
    {
        wxFAIL_MSG( wxT( "All other columns are supposed to be read-only!" ) );
    }

    // Always leave a single empty row for type-in

    int rowCount = static_cast<int>( m_measurementsGrid->GetNumberRows() );
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


void SIMULATOR_FRAME_UI::OnUpdateUI( wxUpdateUIEvent& event )
{
    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() ) )
    {
        if( plotTab->GetLegendPosition() != plotTab->m_LastLegendPosition )
        {
            plotTab->m_LastLegendPosition = plotTab->GetLegendPosition();
            OnModify();
        }
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
void SIMULATOR_FRAME_UI::UpdateMeasurement( int aRow )
{
    static wxRegEx measureParamsRegEx( wxT( "^"
                                            " *"
                                            "([a-zA-Z_]+)"
                                            " +"
                                            "([a-zA-Z]*)\\(([^\\)]+)\\)" ) );

    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
        return;

    wxString text = m_measurementsGrid->GetCellValue( aRow, COL_MEASUREMENT );

    if( text.IsEmpty() )
    {
        m_measurementsGrid->SetCellValue( aRow, COL_MEASUREMENT_VALUE, wxEmptyString );
        return;
    }

    wxString simType = simulator()->TypeToName( plotTab->GetSimType(), true );
    wxString resultName = wxString::Format( wxS( "meas_result_%u" ), aRow );
    wxString result = wxS( "?" );

    if( measureParamsRegEx.Matches( text ) )
    {
        wxString           func = measureParamsRegEx.GetMatch( text, 1 ).Upper();
        wxString           signalType = measureParamsRegEx.GetMatch( text, 2 ).Upper();
        wxString           deviceName = measureParamsRegEx.GetMatch( text, 3 );
        wxString           units;
        SPICE_VALUE_FORMAT fmt = GetMeasureFormat( aRow );

        if( signalType.EndsWith( wxS( "DB" ) ) )
        {
            units = wxS( "dB" );
        }
        else if( signalType.StartsWith( 'I' ) )
        {
            units = wxS( "A" );
        }
        else if( signalType.StartsWith( 'P' ) )
        {
            units = wxS( "W" );
            // Our syntax is different from ngspice for power signals
            text = func + " " + deviceName + ":power";
        }
        else
        {
            units = wxS( "V" );
        }

        if( func.EndsWith( wxS( "_AT" ) ) )
        {
            if( plotTab->GetSimType() == ST_AC || plotTab->GetSimType() == ST_SP )
                units = wxS( "Hz" );
            else
                units = wxS( "s" );
        }
        else if( func.StartsWith( wxS( "INTEG" ) ) )
        {
            switch( plotTab->GetSimType() )
            {
                case ST_TRAN:
                    if ( signalType.StartsWith( 'P' ) )
                        units = wxS( "J" );
                    else
                        units += wxS( ".s" );

                    break;

                case ST_AC:
                case ST_SP:
                case ST_DISTO:
                case ST_NOISE:
                case ST_FFT:
                case ST_SENS:       // If there is a vector, it is frequency
                    units += wxS( "·Hz" );
                    break;

                case ST_DC:         // Could be a lot of things : V, A, deg C, ohm, ...
                case ST_OP:         // There is no vector for integration
                case ST_PZ:         // There is no vector for integration
                case ST_TF:         // There is no vector for integration
                default:
                    units += wxS( "·?" );
                    break;
            }
        }

        fmt.UpdateUnits( units );
        SetMeasureFormat( aRow, fmt );

        updateMeasurementsFromGrid();
    }

    if( m_simulatorFrame->SimFinished() )
    {
        wxString cmd = wxString::Format( wxS( "meas %s %s %s" ), simType, resultName, text );
        simulator()->Command( "echo " + cmd.ToStdString() );
        simulator()->Command( cmd.ToStdString() );

        std::vector<double> resultVec = simulator()->GetGainVector( resultName.ToStdString() );

        if( resultVec.size() > 0 )
            result = SPICE_VALUE( resultVec[0] ).ToString( GetMeasureFormat( aRow ) );
    }

    m_measurementsGrid->SetCellValue( aRow, COL_MEASUREMENT_VALUE, result );
}


void SIMULATOR_FRAME_UI::AddTuner( const SCH_SHEET_PATH& aSheetPath, SCH_SYMBOL* aSymbol )
{
    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
    {
        DisplayErrorMessage( nullptr, _( "The current analysis must have a plot in order to tune "
                                         "the value of a passive R, L, C model or voltage or "
                                         "current source." ) );
        return;
    }

    wxString ref = aSymbol->GetRef( &aSheetPath );

    // Do not add multiple instances for the same component.
    for( TUNER_SLIDER* tuner : m_tuners )
    {
        if( tuner->GetSymbolRef() == ref )
            return;
    }

    if( [[maybe_unused]] const SPICE_ITEM* item = GetExporter()->FindItem( ref ) )
    {
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
}


void SIMULATOR_FRAME_UI::UpdateTunerValue( const SCH_SHEET_PATH& aSheetPath, const KIID& aSymbol,
                                           const wxString& aRef, const wxString& aValue )
{
    SCH_ITEM*   item = aSheetPath.ResolveItem( aSymbol );
    SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( item );

    if( !symbol )
    {
        DisplayErrorMessage( this, _( "Could not apply tuned value(s):" ) + wxS( " " )
                                   + wxString::Format( _( "%s not found" ), aRef ) );
        return;
    }

    NULL_REPORTER devnull;
    SIM_LIB_MGR   mgr( &m_schematicFrame->Prj() );

    std::vector<EMBEDDED_FILES*> embeddedFilesStack;
    embeddedFilesStack.push_back( m_schematicFrame->Schematic().GetEmbeddedFiles() );

    if( EMBEDDED_FILES* symbolEmbeddedFiles = symbol->GetEmbeddedFiles() )
    {
        embeddedFilesStack.push_back( symbolEmbeddedFiles );
        symbol->GetLibSymbolRef()->AppendParentEmbeddedFiles( embeddedFilesStack );
    }

    mgr.SetFilesStack( std::move( embeddedFilesStack ) );

    SIM_MODEL& model = mgr.CreateModel( &aSheetPath, *symbol, true, 0, devnull ).model;

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


void SIMULATOR_FRAME_UI::RemoveTuner( TUNER_SLIDER* aTuner )
{
    m_tuners.remove( aTuner );

    if( std::find( m_multiRunState.tuners.begin(), m_multiRunState.tuners.end(), aTuner )
            != m_multiRunState.tuners.end() )
    {
        clearMultiRunState( true );
    }

    m_tunerOverrides.erase( aTuner );

    aTuner->Destroy();
    m_panelTuners->Layout();
    OnModify();
}


void SIMULATOR_FRAME_UI::AddMeasurement( const wxString& aCmd )
{
    // -1 because the last one is for user input
    for( int i = 0; i < m_measurementsGrid->GetNumberRows(); i++ )
    {
        if ( m_measurementsGrid->GetCellValue( i, COL_MEASUREMENT ) == aCmd )
            return; // Don't create duplicates
    }

    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
        return;

    int row;

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

    UpdateMeasurement( row );
    updateMeasurementsFromGrid();
    OnModify();

    // Always leave at least one empty row for type-in:
    row = m_measurementsGrid->GetNumberRows() - 1;

    if( !m_measurementsGrid->GetCellValue( row, COL_MEASUREMENT ).IsEmpty() )
        m_measurementsGrid->AppendRows( 1 );
}


void SIMULATOR_FRAME_UI::DoFourier( const wxString& aSignal, const wxString& aFundamental )
{
    wxString cmd = wxString::Format( wxS( "fourier %s %s" ),
                                     SPICE_VALUE( aFundamental ).ToSpiceString(),
                                     aSignal );

    simulator()->Command( cmd.ToStdString() );
}


const SPICE_CIRCUIT_MODEL* SIMULATOR_FRAME_UI::GetExporter() const
{
    return circuitModel().get();
}


void SIMULATOR_FRAME_UI::AddTrace( const wxString& aName, SIM_TRACE_TYPE aType )
{
    if( !GetCurrentSimTab() )
    {
        m_simConsole->AppendText( _( "Error: no current simulation.\n" ) );
        m_simConsole->SetInsertionPointEnd();
        return;
    }

    SIM_TYPE simType = SPICE_CIRCUIT_MODEL::CommandToSimType( GetCurrentSimTab()->GetSimCommand() );

    if( simType == ST_UNKNOWN )
    {
        m_simConsole->AppendText( _( "Error: simulation type not defined.\n" ) );
        m_simConsole->SetInsertionPointEnd();
        return;
    }
    else if( !SIM_TAB::IsPlottable( simType ) )
    {
        m_simConsole->AppendText( _( "Error: simulation type doesn't support plotting.\n" ) );
        m_simConsole->SetInsertionPointEnd();
        return;
    }

    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() ) )
    {
        if( simType == ST_AC )
        {
            updateTrace( aName, aType | SPT_AC_GAIN, plotTab );
            updateTrace( aName, aType | SPT_AC_PHASE, plotTab );
        }
        else if( simType == ST_SP )
        {
            updateTrace( aName, aType | SPT_AC_GAIN, plotTab );
            updateTrace( aName, aType | SPT_AC_PHASE, plotTab );
        }
        else
        {
            updateTrace( aName, aType, plotTab );
        }

        plotTab->GetPlotWin()->UpdateAll();
    }

    updateSignalsGrid();
    OnModify();
}


void SIMULATOR_FRAME_UI::SetUserDefinedSignals( const std::map<int, wxString>& aNewSignals )
{
    for( size_t ii = 0; ii < m_plotNotebook->GetPageCount(); ++ii )
    {
        SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( m_plotNotebook->GetPage( ii ) );

        if( !plotTab )
            continue;

        for( const auto& [ id, existingSignal ] : m_userDefinedSignals )
        {
            int      traceType = SPT_UNKNOWN;
            wxString vectorName = vectorNameFromSignalName( plotTab, existingSignal, &traceType );

            if( aNewSignals.count( id ) == 0 )
            {
                if( plotTab->GetSimType() == ST_AC )
                {
                    for( int subType : { SPT_AC_GAIN, SPT_AC_PHASE } )
                        plotTab->DeleteTrace( vectorName, traceType | subType );
                }
                else if( plotTab->GetSimType() == ST_SP )
                {
                    for( int subType : { SPT_SP_AMP, SPT_AC_PHASE } )
                        plotTab->DeleteTrace( vectorName, traceType | subType );
                }
                else
                {
                    plotTab->DeleteTrace( vectorName, traceType );
                }
            }
            else
            {
                if( plotTab->GetSimType() == ST_AC )
                {
                    for( int subType : { SPT_AC_GAIN, SPT_AC_PHASE } )
                    {
                        if( TRACE* trace = plotTab->GetTrace( vectorName, traceType | subType ) )
                            trace->SetName( aNewSignals.at( id ) );
                    }
                }
                else if( plotTab->GetSimType() == ST_SP )
                {
                    for( int subType : { SPT_SP_AMP, SPT_AC_PHASE } )
                    {
                        if( TRACE* trace = plotTab->GetTrace( vectorName, traceType | subType ) )
                            trace->SetName( aNewSignals.at( id ) );
                    }
                }
                else
                {
                    if( TRACE* trace = plotTab->GetTrace( vectorName, traceType ) )
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
    OnModify();
}


void SIMULATOR_FRAME_UI::updateTrace( const wxString& aVectorName, int aTraceType, SIM_PLOT_TAB* aPlotTab,
                                      std::vector<double>* aDataX, bool aClearData )
{
    if( !m_simulatorFrame->SimFinished() && !simulator()->IsRunning())
    {
        aPlotTab->GetOrAddTrace( aVectorName, aTraceType );
        return;
    }

    SIM_TYPE simType = SPICE_CIRCUIT_MODEL::CommandToSimType( aPlotTab->GetSimCommand() );

    aTraceType &= aTraceType & SPT_Y_AXIS_MASK;
    aTraceType |= getXAxisType( simType );

    wxString simVectorName = aVectorName;

    if( aTraceType & SPT_POWER )
        simVectorName = simVectorName.AfterFirst( '(' ).BeforeLast( ')' ) + wxS( ":power" );

    if( !SIM_TAB::IsPlottable( simType ) )
    {
        // There is no plot to be shown
        simulator()->Command( wxString::Format( wxT( "print %s" ), aVectorName ).ToStdString() );

        return;
    }

    std::vector<double> data_x;
    std::vector<double> data_y;

    if( !aDataX || aClearData )
        aDataX = &data_x;

    // First, handle the x axis
    if( aDataX->empty() && !aClearData )
    {
        wxString xAxisName( simulator()->GetXAxis( simType ) );

        if( xAxisName.IsEmpty() )
            return;

        *aDataX = simulator()->GetGainVector( (const char*) xAxisName.c_str() );
    }

    unsigned int size = aDataX->size();

    switch( simType )
    {
    case ST_AC:
        if( aTraceType & SPT_AC_GAIN )
            data_y = simulator()->GetGainVector( (const char*) simVectorName.c_str(), size );
        else if( aTraceType & SPT_AC_PHASE )
            data_y = simulator()->GetPhaseVector( (const char*) simVectorName.c_str(), size );
        else
            wxFAIL_MSG( wxT( "Plot type missing AC_PHASE or AC_MAG bit" ) );

        break;
    case ST_SP:
        if( aTraceType & SPT_SP_AMP )
            data_y = simulator()->GetGainVector( (const char*) simVectorName.c_str(), size );
        else if( aTraceType & SPT_AC_PHASE )
            data_y = simulator()->GetPhaseVector( (const char*) simVectorName.c_str(), size );
        else
            wxFAIL_MSG( wxT( "Plot type missing AC_PHASE or SPT_SP_AMP bit" ) );

        break;

    case ST_DC:
        data_y = simulator()->GetGainVector( (const char*) simVectorName.c_str(), -1 );
        break;

    case ST_NOISE:
    case ST_TRAN:
    case ST_FFT:
        data_y = simulator()->GetGainVector( (const char*) simVectorName.c_str(), size );
        break;

    default:
        wxFAIL_MSG( wxT( "Unhandled plot type" ) );
    }

    SPICE_DC_PARAMS source1, source2;
    int             sweepCount = 1;
    size_t          sweepSize = std::numeric_limits<size_t>::max();

    if( simType == ST_DC
            && circuitModel()->ParseDCCommand( aPlotTab->GetSimCommand(), &source1, &source2 )
            && !source2.m_source.IsEmpty() )
    {
        SPICE_VALUE v = ( source2.m_vend - source2.m_vstart ) / source2.m_vincrement;

        sweepCount = KiROUND( v.ToDouble() ) + 1;
        sweepSize = aDataX->size() / sweepCount;
    }

    if( m_multiRunState.storePending )
        recordMultiRunData( aVectorName, aTraceType, *aDataX, data_y );

    if( hasMultiRunTrace( aVectorName, aTraceType ) )
    {
        const std::string key = multiRunTraceKey( aVectorName, aTraceType );
        const auto        traceIt = m_multiRunState.traces.find( key );

        if( traceIt != m_multiRunState.traces.end() )
        {
            const MULTI_RUN_TRACE& traceData = traceIt->second;

            if( !traceData.xValues.empty() && !traceData.yValues.empty() )
            {
                size_t sweepSizeMulti = traceData.xValues.size();
                size_t runCount = traceData.yValues.size();

                if( sweepSizeMulti > 0 && runCount > 0 )
                {
                    std::vector<double> combinedX;
                    std::vector<double> combinedY;

                    combinedX.reserve( sweepSizeMulti * runCount );
                    combinedY.reserve( sweepSizeMulti * runCount );

                    for( const std::vector<double>& runY : traceData.yValues )
                    {
                        if( runY.size() != sweepSizeMulti )
                            continue;

                        combinedX.insert( combinedX.end(), traceData.xValues.begin(), traceData.xValues.end() );
                        combinedY.insert( combinedY.end(), runY.begin(), runY.end() );
                    }

                    if( TRACE* trace = aPlotTab->GetOrAddTrace( aVectorName, aTraceType ) )
                    {
                        if( combinedY.size() >= combinedX.size() && sweepSizeMulti > 0 )
                        {
                            int sweepCountCombined = combinedX.empty() ? 0 : static_cast<int>( combinedY.size() / sweepSizeMulti );

                            if( sweepCountCombined > 0 )
                            {
                                // Generate labels for each run based on tuner values
                                std::vector<wxString> labels;
                                labels.reserve( sweepCountCombined );
                                
                                for( int i = 0; i < sweepCountCombined && i < (int)m_multiRunState.steps.size(); ++i )
                                {
                                    const MULTI_RUN_STEP& step = m_multiRunState.steps[i];
                                    wxString label;
                                    
                                    for( auto it = step.overrides.begin(); it != step.overrides.end(); ++it )
                                    {
                                        if( it != step.overrides.begin() )
                                            label += wxS( ", " );
                                        
                                        const TUNER_SLIDER* tuner = it->first;
                                        double value = it->second;
                                        
                                        SPICE_VALUE spiceVal( value );
                                        label += tuner->GetSymbolRef() + wxS( "=" ) + spiceVal.ToSpiceString();
                                    }
                                    
                                    labels.push_back( label );
                                }
                                
                                aPlotTab->SetTraceData( trace, combinedX, combinedY, sweepCountCombined, 
                                                       sweepSizeMulti, true, labels );
                            }
                        }
                    }

                    return;
                }
            }
        }
    }

    if( TRACE* trace = aPlotTab->GetOrAddTrace( aVectorName, aTraceType ) )
    {
        if( data_y.size() >= size )
            aPlotTab->SetTraceData( trace, *aDataX, data_y, sweepCount, sweepSize );
    }
}


// TODO make sure where to instantiate and how to style correct
// Better ask someone..
template void SIMULATOR_FRAME_UI::signalsGridCursorUpdate<SIGNALS_GRID_COLUMNS, int, int>(
        SIGNALS_GRID_COLUMNS, int, int );

template <typename T, typename U, typename R>
void SIMULATOR_FRAME_UI::signalsGridCursorUpdate( T t, U u, R r ) // t=cursor type/signals' grid col, u=cursor number/cursor "id", r=table's row
{
    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );
    wxString      signalName = m_signalsGrid->GetCellValue( r, COL_SIGNAL_NAME );
    int           traceType = SPT_UNKNOWN;
    wxString      vectorName = vectorNameFromSignalName( plotTab, signalName, &traceType );

    wxGridCellAttrPtr attr = m_signalsGrid->GetOrCreateCellAttrPtr( r, static_cast<int>( t ) );

    if( TRACE* trace = plotTab ? plotTab->GetTrace( vectorName, traceType ) : nullptr )
    {
        attr->SetReadOnly(); // not really; we delegate interactivity to GRID_TRICKS

        if( t >= SIGNALS_GRID_COLUMNS::COL_SIGNAL_SHOW )
        {
            attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
        }

        if constexpr ( std::is_enum<T>::value )
        {
            if( t == SIGNALS_GRID_COLUMNS::COL_SIGNAL_SHOW )
            {
                m_signalsGrid->SetCellValue( r, static_cast<int>( t ), wxS( "1" ) );
            }
            else if( t == SIGNALS_GRID_COLUMNS::COL_SIGNAL_COLOR )
            {
                if( !attr->HasRenderer() )
                    attr->SetRenderer( new GRID_CELL_COLOR_RENDERER( this ) );

                if( !attr->HasEditor() )
                    attr->SetEditor( new GRID_CELL_COLOR_SELECTOR( this, m_signalsGrid ) );

                attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
                attr->SetReadOnly( false );

                KIGFX::COLOR4D color( trace->GetPen().GetColour() );
                m_signalsGrid->SetCellValue( r, COL_SIGNAL_COLOR, color.ToCSSString() );
            }
            else if( t == SIGNALS_GRID_COLUMNS::COL_CURSOR_1
                     || t == SIGNALS_GRID_COLUMNS::COL_CURSOR_2
                     || t > SIGNALS_GRID_COLUMNS::COL_CURSOR_2 )
            {
                if( !attr->HasRenderer() )
                    attr->SetRenderer( new wxGridCellBoolRenderer() );

                if( u > 0 && trace->HasCursor( u ) )
                    m_signalsGrid->SetCellValue( r, static_cast<int>( t ), wxS( "1" ) );
                else
                    m_signalsGrid->SetCellValue( r, static_cast<int>( t ), wxEmptyString );
            }
        }
    }
    else
    {
        if constexpr ( std::is_enum<T>::value )
        {
            if( t == SIGNALS_GRID_COLUMNS::COL_SIGNAL_SHOW )
            {
                m_signalsGrid->SetCellValue( r, static_cast<int>( t ), wxEmptyString );
            }
            else if( t == SIGNALS_GRID_COLUMNS::COL_SIGNAL_COLOR
                     || t == SIGNALS_GRID_COLUMNS::COL_CURSOR_1
                     || t == SIGNALS_GRID_COLUMNS::COL_CURSOR_2
                     || t > SIGNALS_GRID_COLUMNS::COL_CURSOR_2 )
            {
                attr->SetEditor( nullptr );
                attr->SetRenderer( nullptr );
                attr->SetReadOnly();
                m_signalsGrid->SetCellValue( r, static_cast<int>( t ), wxEmptyString );
            }
        }
    }
}


void SIMULATOR_FRAME_UI::updateSignalsGrid()
{
    for( int row = 0; row < m_signalsGrid->GetNumberRows(); ++row )
    {
        signalsGridCursorUpdate( COL_SIGNAL_SHOW, 0, row );
        signalsGridCursorUpdate( COL_SIGNAL_COLOR, 0, row );
        signalsGridCursorUpdate( COL_CURSOR_1, 1, row );
        signalsGridCursorUpdate( COL_CURSOR_2, 2, row );

        if( ( m_signalsGrid->GetNumberCols() - 1 ) > COL_CURSOR_2 )
        {
            for( int i = 3; i < m_customCursorsCnt; i++ )
            {
                int tm = i + 2;
                signalsGridCursorUpdate( static_cast<SIGNALS_GRID_COLUMNS>( tm ), i, row );
            }
        }
    }
    m_signalsGrid->Refresh();
}


void SIMULATOR_FRAME_UI::applyUserDefinedSignals()
{
    auto quoteNetNames =
            [&]( wxString aExpression ) -> wxString
            {
                std::vector<bool> mask( aExpression.length(), false );

                auto isNetnameChar =
                        []( wxUniChar aChar ) -> bool
                        {
                            wxUint32 value = aChar.GetValue();

                            if( ( value >= '0' && value <= '9' ) || ( value >= 'A' && value <= 'Z' )
                                || ( value >= 'a' && value <= 'z' ) )
                            {
                                return true;
                            }

                            switch( value )
                            {
                            case '_':
                            case '/':
                            case '+':
                            case '-':
                            case '~':
                            case '.':
                                return true;
                            default:
                                break;
                            }

                            return false;
                        };

                for( const wxString& netname : m_netnames )
                {
                    size_t pos = aExpression.find( netname );

                    while( pos != wxString::npos )
                    {
                        for( size_t i = 0; i < netname.length(); ++i )
                            mask[pos + i] = true; // Mark the positions of the netname

                        pos = aExpression.find( netname, pos + 1 ); // Find the next occurrence
                    }
                }

                for( size_t i = 0; i < aExpression.length(); ++i )
                {
                    if( !mask[i] || ( i > 0 && mask[i - 1] ) )
                        continue;

                    size_t j = i + 1;

                    while( j < aExpression.length() )
                    {
                        if( mask[j] )
                        {
                            ++j;
                            continue;
                        }

                        if( isNetnameChar( aExpression[j] ) )
                        {
                            mask[j] = true;
                            ++j;
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                wxString quotedNetnames = "";
                bool     startQuote = true;

                // put quotes around all the positions that were found above
                for( size_t i = 0; i < aExpression.length(); i++ )
                {
                    if( mask[i] && startQuote )
                    {
                        quotedNetnames = quotedNetnames + "\"";
                        startQuote = false;
                    }
                    else if( !mask[i] && !startQuote )
                    {
                        quotedNetnames = quotedNetnames + "\"";
                        startQuote = true;
                    }

                    wxString ch = aExpression[i];
                    quotedNetnames = quotedNetnames + ch;
                }

                if( !startQuote )
                    quotedNetnames = quotedNetnames + "\"";

                return quotedNetnames;
            };

    for( const auto& [ id, signal ] : m_userDefinedSignals )
    {
        constexpr const char* cmd = "let user{} = {}";

        simulator()->Command( "echo " + fmt::format( cmd, id, signal.ToStdString() ) );
        simulator()->Command( fmt::format( cmd, id, quoteNetNames( signal ).ToStdString() ) );
    }
}


void SIMULATOR_FRAME_UI::applyTuners()
{
    WX_STRING_REPORTER reporter;

    for( const TUNER_SLIDER* tuner : m_tuners )
    {
        SCH_SHEET_PATH sheetPath;
        wxString       ref = tuner->GetSymbolRef();
        KIID           symbolId = tuner->GetSymbol( &sheetPath );
        SCH_ITEM*      schItem = sheetPath.ResolveItem( symbolId );
        SCH_SYMBOL*    symbol = dynamic_cast<SCH_SYMBOL*>( schItem );

        if( !symbol )
        {
            reporter.Report( wxString::Format( _( "%s not found" ), ref ) );
            continue;
        }

        const SPICE_ITEM* item = GetExporter()->FindItem( tuner->GetSymbolRef() );

        if( !item || !item->model->GetTunerParam() )
        {
            reporter.Report( wxString::Format( _( "%s is not tunable" ), ref ) );
            continue;
        }

        double floatVal;

        auto overrideIt = m_tunerOverrides.find( tuner );

        if( overrideIt != m_tunerOverrides.end() )
            floatVal = overrideIt->second;
        else
            floatVal = tuner->GetValue().ToDouble();

        simulator()->Command( item->model->SpiceGenerator().TunerCommand( *item, floatVal ) );
    }

    if( reporter.HasMessage() )
        DisplayErrorMessage( this, _( "Could not apply tuned value(s):" ) + wxS( "\n" ) + reporter.GetMessages() );
}

bool SIMULATOR_FRAME_UI::LoadWorkbook( const wxString& aPath )
{
    wxTextFile file( aPath );

    if( !file.Open() )
        return false;

    wxString firstLine = file.GetFirstLine();
    long     dummy;
    bool     legacy = firstLine.StartsWith( wxT( "version " ) ) || firstLine.ToLong( &dummy );

    file.Close();

    m_plotNotebook->DeleteAllPages();
    m_userDefinedSignals.clear();

    if( legacy )
    {
        if( !loadLegacyWorkbook( aPath ) )
            return false;
    }
    else
    {
        if( !loadJsonWorkbook( aPath ) )
            return false;
    }

    rebuildSignalsList();

    rebuildSignalsGrid( m_filter->GetValue() );
    updateSignalsGrid();
    updatePlotCursors();
    rebuildMeasurementsGrid();

    wxFileName filename( aPath );
    filename.MakeRelativeTo( m_schematicFrame->Prj().GetProjectPath() );

    // Remember the loaded workbook filename.
    simulator()->Settings()->SetWorkbookFilename( filename.GetFullPath() );

    return true;
}


bool SIMULATOR_FRAME_UI::loadJsonWorkbook( const wxString& aPath )
{
    wxFFileInputStream fp( aPath, wxT( "rt" ) );
    wxStdInputStream   fstream( fp );

    if( !fp.IsOk() )
        return false;

    try
    {
        nlohmann::json js = nlohmann::json::parse( fstream, nullptr, true, true );

        std::map<SIM_PLOT_TAB*, nlohmann::json> traceInfo;

        for( const nlohmann::json& tab_js : js[ "tabs" ] )
        {
            wxString simCommand;
            int      simOptions = NETLIST_EXPORTER_SPICE::OPTION_ADJUST_PASSIVE_VALS
                                    | NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_EVENTS;

            for( const nlohmann::json& cmd : tab_js[ "commands" ] )
            {
                if( cmd == ".kicad adjustpaths" )
                    simOptions |= NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS;
                else if( cmd == ".save all" )
                    simOptions |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES;
                else if( cmd == ".probe alli" )
                    simOptions |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS;
                else if( cmd == ".probe allp" )
                    simOptions |= NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS;
                else if( cmd == ".kicad esavenone" )
                    simOptions &= ~NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_EVENTS;
                else
                    simCommand += wxString( cmd.get<wxString>() ).Trim();
            }

            SIM_TAB*      simTab = NewSimTab( simCommand );
            SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( simTab );

            simTab->SetSimOptions( simOptions );

            if( plotTab )
            {
                if( tab_js.contains( "traces" ) )
                    traceInfo[plotTab] = tab_js[ "traces" ];

                if( tab_js.contains( "measurements" ) )
                {
                    for( const nlohmann::json& m_js : tab_js[ "measurements" ] )
                        plotTab->Measurements().emplace_back( m_js[ "expr" ], m_js[ "format" ] );
                }

                plotTab->SetDottedSecondary( tab_js[ "dottedSecondary" ] );
                plotTab->ShowGrid( tab_js[ "showGrid" ] );

                if( tab_js.contains( "fixedY1scale" ) )
                {
                    const nlohmann::json& scale_js = tab_js[ "fixedY1scale" ];
                    plotTab->SetY1Scale( true, scale_js[ "min" ], scale_js[ "max" ] );
                    plotTab->GetPlotWin()->LockY( true );
                }

                if( tab_js.contains( "fixedY2scale" ) )
                {
                    const nlohmann::json& scale_js = tab_js[ "fixedY2scale" ];
                    plotTab->SetY2Scale( true, scale_js[ "min" ], scale_js[ "max" ] );
                    plotTab->GetPlotWin()->LockY( true );
                }

                if( tab_js.contains( "fixedY3scale" ) )
                {
                    plotTab->EnsureThirdYAxisExists();
                    const nlohmann::json& scale_js = tab_js[ "fixedY3scale" ];
                    plotTab->SetY3Scale( true, scale_js[ "min" ], scale_js[ "max" ] );
                    plotTab->GetPlotWin()->LockY( true );
                }

                if( tab_js.contains( "legend" ) )
                {
                    const nlohmann::json& legend_js = tab_js[ "legend" ];
                    plotTab->SetLegendPosition( wxPoint( legend_js[ "x" ], legend_js[ "y" ] ) );
                    plotTab->ShowLegend( true );
                }

                if( tab_js.contains( "margins" ) )
                {
                    const nlohmann::json& margins_js = tab_js[ "margins" ];
                    plotTab->GetPlotWin()->SetMargins( margins_js[ "top" ],
                                                       margins_js[ "right" ],
                                                       margins_js[ "bottom" ],
                                                       margins_js[ "left" ] );
                }
            }
        }

        int ii = 0;

        if( js.contains( "user_defined_signals" ) )
        {
            for( const nlohmann::json& signal_js : js[ "user_defined_signals" ] )
                m_userDefinedSignals[ii++] = wxString( signal_js.get<wxString>() );
        }

        if( SIM_TAB* simTab = GetCurrentSimTab() )
        {
            m_simulatorFrame->LoadSimulator( simTab->GetSimCommand(), simTab->GetSimOptions() );

            if( SIM_TAB* firstTab = dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( 0 ) ) )
                firstTab->SetLastSchTextSimCommand( js["last_sch_text_sim_command"] );
        }

        int tempCustomCursorsCnt = 0;

        if( js.contains( "custom_cursors" ) )
            tempCustomCursorsCnt = js["custom_cursors"];
        else
            tempCustomCursorsCnt = 2; // Kind of virtual, for the initial loading of the new setting

        if( ( tempCustomCursorsCnt > m_customCursorsCnt ) && m_customCursorsCnt > 2 )
            tempCustomCursorsCnt = 2 * tempCustomCursorsCnt - m_customCursorsCnt;

        for( int yy = 0; yy <= ( tempCustomCursorsCnt - m_customCursorsCnt ); yy++ )
            CreateNewCursor();

        auto addCursor =
                [=,this]( SIM_PLOT_TAB* aPlotTab, TRACE* aTrace, const wxString& aSignalName,
                        int aCursorId, const nlohmann::json& aCursor_js )
                {
                    if( aCursorId >= 1 )
                    {
                        CURSOR* cursor = new CURSOR( aTrace, aPlotTab );

                        cursor->SetName( aSignalName );
                        cursor->SetCoordX( aCursor_js[ "position" ] );

                        aTrace->SetCursor( aCursorId, cursor );
                        aPlotTab->GetPlotWin()->AddLayer( cursor );
                    }

                    if( aCursorId == -1 )
                    {
                        // We are a "cursorD"
                        m_cursorFormatsDyn[2][0].FromString( aCursor_js["x_format"] );
                        m_cursorFormatsDyn[2][1].FromString( aCursor_js["y_format"] );
                    }
                    else
                    {
                        if( aCursorId < 3 )
                        {
                            m_cursorFormatsDyn[aCursorId - 1][0].FromString( aCursor_js["x_format"] );
                            m_cursorFormatsDyn[aCursorId - 1][1].FromString( aCursor_js["y_format"] );
                        }
                        else
                        {
                            m_cursorFormatsDyn[aCursorId][0].FromString( aCursor_js["x_format"] );
                            m_cursorFormatsDyn[aCursorId][1].FromString( aCursor_js["y_format"] );
                        }
                    }
                };

        for( const auto& [ plotTab, traces_js ] : traceInfo )
        {
            for( const nlohmann::json& trace_js : traces_js )
            {
                wxString signalName = trace_js[ "signal" ];
                wxString vectorName = vectorNameFromSignalName( plotTab, signalName, nullptr );
                TRACE*   trace = plotTab->GetOrAddTrace( vectorName, trace_js[ "trace_type" ] );

                if( trace )
                {
                    if( trace_js.contains( "cursorD" ) )
                        addCursor( plotTab, trace, signalName, -1, trace_js[ "cursorD" ] );

                    std::vector<const char*> aVec;
                    aVec.clear();

                    for( int i = 1; i <= tempCustomCursorsCnt; i++ )
                    {
                        wxString str = "cursor" + std::to_string( i );
                        aVec.emplace_back( str.c_str() );

                        if( trace_js.contains( aVec[i - 1] ) )
                            addCursor( plotTab, trace, signalName, i, trace_js[aVec[i - 1]] );
                    }

                    if( trace_js.contains( "color" ) )
                    {
                        wxColour color;
                        color.Set( wxString( trace_js["color"].get<wxString>() ) );
                        trace->SetTraceColour( color );
                        plotTab->UpdateTraceStyle( trace );
                    }
                }
            }

            plotTab->UpdatePlotColors();
        }
    }
    catch( nlohmann::json::parse_error& error )
    {
        wxLogTrace( traceSettings, wxT( "Json parse error reading %s: %s" ), aPath, error.what() );

        return false;
    }
    catch( nlohmann::json::type_error& error )
    {
        wxLogTrace( traceSettings, wxT( "Json type error reading %s: %s" ), aPath, error.what() );

        return false;
    }
    catch( nlohmann::json::invalid_iterator& error )
    {
        wxLogTrace( traceSettings, wxT( "Json invalid_iterator error reading %s: %s" ), aPath, error.what() );

        return false;
    }
    catch( nlohmann::json::out_of_range& error )
    {
        wxLogTrace( traceSettings, wxT( "Json out_of_range error reading %s: %s" ), aPath, error.what() );

        return false;
    }
    catch( ... )
    {
        wxLogTrace( traceSettings, wxT( "Error reading %s" ), aPath );
        return false;
    }

    return true;
}

void SIMULATOR_FRAME_UI::SaveCursorToWorkbook( nlohmann::json& aTraceJs, TRACE* aTrace, int aCursorId )
{
    int cursorIdAfterD = aCursorId;

    if( aCursorId > 3 )
        cursorIdAfterD = cursorIdAfterD - 1;


    if( CURSOR* cursor = aTrace->GetCursor( aCursorId ) )
    {
        aTraceJs["cursor" + wxString( "" ) << aCursorId] =
                nlohmann::json( { { "position", cursor->GetCoords().x },
                                  { "x_format", m_cursorFormatsDyn[cursorIdAfterD][0].ToString() },
                                  { "y_format", m_cursorFormatsDyn[cursorIdAfterD][1].ToString() } } );
    }

    if( cursorIdAfterD < 3 && ( aTrace->GetCursor( 1 ) || aTrace->GetCursor( 2 ) ) )
    {
        aTraceJs["cursorD"] =
                nlohmann::json( { { "x_format", m_cursorFormatsDyn[2][0].ToString() },
                                  { "y_format", m_cursorFormatsDyn[2][1].ToString() } } );
    }
}


bool SIMULATOR_FRAME_UI::SaveWorkbook( const wxString& aPath )
{
    updateMeasurementsFromGrid();

    wxFileName filename = aPath;
    filename.SetExt( FILEEXT::WorkbookFileExtension );

    wxFile file;

    file.Create( filename.GetFullPath(), true /* overwrite */ );

    if( !file.IsOpened() )
        return false;

    nlohmann::json tabs_js = nlohmann::json::array();

    for( size_t i = 0; i < m_plotNotebook->GetPageCount(); i++ )
    {
        SIM_TAB* simTab = dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( i ) );

        if( !simTab )
            continue;

        SIM_TYPE simType = simTab->GetSimType();

        nlohmann::json commands_js = nlohmann::json::array();

        commands_js.push_back( simTab->GetSimCommand() );

        int options = simTab->GetSimOptions();

        if( options & NETLIST_EXPORTER_SPICE::OPTION_ADJUST_INCLUDE_PATHS )
            commands_js.push_back( ".kicad adjustpaths" );

        if( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_VOLTAGES )
            commands_js.push_back( ".save all" );

        if( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_CURRENTS )
            commands_js.push_back( ".probe alli" );

        if( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_DISSIPATIONS )
            commands_js.push_back( ".probe allp" );

        if( !( options & NETLIST_EXPORTER_SPICE::OPTION_SAVE_ALL_EVENTS ) )
            commands_js.push_back( ".kicad esavenone" );

        nlohmann::json tab_js = nlohmann::json(
                                    { { "analysis", SPICE_SIMULATOR::TypeToName( simType, true ) },
                                      { "commands", commands_js } } );

        if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( simTab ) )
        {
            nlohmann::json traces_js = nlohmann::json::array();

            auto findSignalName =
                    [&]( const wxString& aVectorName ) -> wxString
                    {
                        wxString vectorName;
                        wxString suffix;

                        if( aVectorName.EndsWith( _( " (phase)" ) ) )
                            suffix = _( " (phase)" );
                        else if( aVectorName.EndsWith( _( " (gain)" ) ) )
                            suffix = _( " (gain)" );

                        vectorName = aVectorName.Left( aVectorName.Length() - suffix.Length() );

                        for( const auto& [ id, signal ] : m_userDefinedSignals )
                        {
                            if( vectorName == vectorNameFromSignalId( id ) )
                                return signal + suffix;
                        }

                        return aVectorName;
                    };

            for( const auto& [name, trace] : plotTab->GetTraces() )
            {
                nlohmann::json trace_js = nlohmann::json(
                            { { "trace_type", (int) trace->GetType() },
                              { "signal",     findSignalName( trace->GetDisplayName() ) },
                              { "color",      COLOR4D( trace->GetTraceColour() ).ToCSSString() } } );

                for( int ii = 1; ii <= m_customCursorsCnt; ii++ )
                    SaveCursorToWorkbook( trace_js, trace, ii );

                if( trace->GetCursor( 1 ) || trace->GetCursor( 2 ) )
                {
                    trace_js["cursorD"] = nlohmann::json(
                                            { { "x_format", m_cursorFormatsDyn[2][0].ToString() },
                                              { "y_format", m_cursorFormatsDyn[2][1].ToString() } } );
                }

                traces_js.push_back( trace_js );
            }

            nlohmann::json measurements_js = nlohmann::json::array();

            for( const auto& [ measurement, format ] : plotTab->Measurements() )
            {
                measurements_js.push_back( nlohmann::json( { { "expr",   measurement },
                                                             { "format", format } } ) );
            }

            tab_js[ "traces" ]          = traces_js;
            tab_js[ "measurements" ]    = measurements_js;
            tab_js[ "dottedSecondary" ] = plotTab->GetDottedSecondary();
            tab_js[ "showGrid" ]        = plotTab->IsGridShown();

            double min, max;

            if( plotTab->GetY1Scale( &min, &max ) )
                tab_js[ "fixedY1scale" ] = nlohmann::json( { { "min", min }, { "max", max } } );

            if( plotTab->GetY2Scale( &min, &max ) )
                tab_js[ "fixedY2scale" ] = nlohmann::json( { { "min", min }, { "max", max } } );

            if( plotTab->GetY3Scale( &min, &max ) )
                tab_js[ "fixedY3scale" ] = nlohmann::json( { { "min", min }, { "max", max } } );

            if( plotTab->IsLegendShown() )
            {
                tab_js[ "legend" ] = nlohmann::json( { { "x", plotTab->GetLegendPosition().x },
                                                       { "y", plotTab->GetLegendPosition().y } } );
            }

            mpWindow* plotWin = plotTab->GetPlotWin();

            tab_js[ "margins" ] = nlohmann::json( { { "left",   plotWin->GetMarginLeft() },
                                                    { "right",  plotWin->GetMarginRight() },
                                                    { "top",    plotWin->GetMarginTop() },
                                                    { "bottom", plotWin->GetMarginBottom() } } );
        }

        tabs_js.push_back( tab_js );
    }

    nlohmann::json userDefinedSignals_js = nlohmann::json::array();

    for( const auto& [ id, signal ] : m_userDefinedSignals )
        userDefinedSignals_js.push_back( signal );

    // clang-format off
    nlohmann::json js = nlohmann::json( { { "version",              7 },
                                          { "tabs",                 tabs_js },
                                          { "user_defined_signals", userDefinedSignals_js },
                                          { "custom_cursors",        m_customCursorsCnt - 1 } } ); // Since we start +1 on init
    // clang-format on

    // Store the value of any simulation command found on the schematic sheet in a SCH_TEXT
    // object.  If this changes we want to warn the user and ask them if they want to update
    // the corresponding panel's sim command.
    if( m_plotNotebook->GetPageCount() > 0 )
    {
        SIM_TAB* simTab = dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( 0 ) );
        js[ "last_sch_text_sim_command" ] = simTab->GetLastSchTextSimCommand();
    }

    std::stringstream buffer;
    buffer << std::setw( 2 ) << js << std::endl;

    bool res = file.Write( buffer.str() );
    file.Close();

    // Store the filename of the last saved workbook.
    if( res )
    {
        filename.MakeRelativeTo( m_schematicFrame->Prj().GetProjectPath() );
        simulator()->Settings()->SetWorkbookFilename( filename.GetFullPath() );
    }

    return res;
}


SIM_TRACE_TYPE SIMULATOR_FRAME_UI::getXAxisType( SIM_TYPE aType ) const
{
    switch( aType )
    {
    /// @todo SPT_LOG_FREQUENCY
    case ST_AC:    return SPT_LIN_FREQUENCY;
    case ST_SP:    return SPT_LIN_FREQUENCY;
    case ST_FFT:   return SPT_LIN_FREQUENCY;
    case ST_DC:    return SPT_SWEEP;
    case ST_TRAN:  return SPT_TIME;
    case ST_NOISE: return SPT_LIN_FREQUENCY;

    default:
        wxFAIL_MSG( wxString::Format( wxS( "Unhandled simulation type: %d" ), (int) aType ) );
        return SPT_UNKNOWN;
    }
}


wxString SIMULATOR_FRAME_UI::getNoiseSource() const
{
    wxString    output;
    wxString    ref;
    wxString    source;
    wxString    scale;
    SPICE_VALUE pts;
    SPICE_VALUE fStart;
    SPICE_VALUE fStop;
    bool        saveAll;

    if( GetCurrentSimTab() )
    {
        circuitModel()->ParseNoiseCommand( GetCurrentSimTab()->GetSimCommand(), &output, &ref,
                                           &source, &scale, &pts, &fStart, &fStop, &saveAll );
    }

    return source;
}


void SIMULATOR_FRAME_UI::TogglePanel( wxPanel* aPanel, wxSplitterWindow* aSplitterWindow,
                                      int& aSashPosition )
{
    bool isShown = aPanel->IsShown();

    if( isShown )
        aSashPosition = aSplitterWindow->GetSashPosition();

    aPanel->Show( !isShown );

    aSplitterWindow->SetSashInvisible( isShown );
    aSplitterWindow->SetSashPosition( isShown ? -1 : aSashPosition, true );

    aSplitterWindow->UpdateSize();
    m_parent->Refresh();
    m_parent->Layout();
}


bool SIMULATOR_FRAME_UI::IsSimConsoleShown()
{
    return m_panelConsole->IsShown();
}


void SIMULATOR_FRAME_UI::ToggleSimConsole()
{
    TogglePanel( m_panelConsole, m_splitterPlotAndConsole, m_splitterPlotAndConsoleSashPosition );
}


bool SIMULATOR_FRAME_UI::IsSimSidePanelShown()
{
    return m_sidePanel->IsShown();
}


void SIMULATOR_FRAME_UI::ToggleSimSidePanel()
{
    TogglePanel( m_sidePanel, m_splitterLeftRight, m_splitterLeftRightSashPosition );
}


void SIMULATOR_FRAME_UI::ToggleDarkModePlots()
{
    m_darkMode = !m_darkMode;

    // Rebuild the color list to plot traces
    SIM_PLOT_COLORS::FillDefaultColorList( m_darkMode );

    // Now send changes to all SIM_PLOT_TAB
    for( size_t page = 0; page < m_plotNotebook->GetPageCount(); page++ )
    {
        wxWindow* curPage = m_plotNotebook->GetPage( page );

        // ensure it is truly a plot plotTab and not the (zero plots) placeholder
        // which is only SIM_TAB
        SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( curPage );

        if( plotTab )
            plotTab->UpdatePlotColors();
    }
}


void SIMULATOR_FRAME_UI::onPlotClose( wxAuiNotebookEvent& event )
{
    OnModify();
}


void SIMULATOR_FRAME_UI::onPlotClosed( wxAuiNotebookEvent& event )
{
    CallAfter( [this]()
               {
                   rebuildSignalsList();
                   rebuildSignalsGrid( m_filter->GetValue() );
                   updatePlotCursors();

                   //To avoid a current side effect in dynamic cursors while closing one out of many sim tabs
                   updateSignalsGrid();

                   SIM_TAB* panel = GetCurrentSimTab();

                   if( !panel || panel->GetSimType() != ST_OP )
                   {
                       SCHEMATIC& schematic = m_schematicFrame->Schematic();
                       schematic.ClearOperatingPoints();
                       m_schematicFrame->RefreshOperatingPointDisplay();
                       m_schematicFrame->GetCanvas()->Refresh();
                   }
               } );
}


void SIMULATOR_FRAME_UI::updateMeasurementsFromGrid()
{
    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() ) )
    {
        std::vector<std::pair<wxString, wxString>>& measurements = plotTab->Measurements();

        measurements.clear();

        for( int row = 0; row < m_measurementsGrid->GetNumberRows(); ++row )
        {
            if( !m_measurementsGrid->GetCellValue( row, COL_MEASUREMENT ).IsEmpty() )
            {
                measurements.emplace_back( m_measurementsGrid->GetCellValue( row, COL_MEASUREMENT ),
                                           m_measurementsGrid->GetCellValue( row, COL_MEASUREMENT_FORMAT ) );
            }
        }
    }
}


void SIMULATOR_FRAME_UI::onPlotChanging( wxAuiNotebookEvent& event )
{
    m_measurementsGrid->ClearRows();

    event.Skip();
}


void SIMULATOR_FRAME_UI::OnPlotSettingsChanged()
{
    rebuildSignalsList();
    rebuildSignalsGrid( m_filter->GetValue() );
    updatePlotCursors();

    rebuildMeasurementsGrid();

    for( int row = 0; row < m_measurementsGrid->GetNumberRows(); ++row )
        UpdateMeasurement( row );
}


void SIMULATOR_FRAME_UI::onPlotChanged( wxAuiNotebookEvent& event )
{
    if( SIM_TAB* simTab = GetCurrentSimTab() )
        simulator()->Command( "setplot " + simTab->GetSpicePlotName().ToStdString() );

    OnPlotSettingsChanged();

    //To avoid a current side effect in dynamic cursors while switching sim tabs
    updateSignalsGrid();

    event.Skip();
}


void SIMULATOR_FRAME_UI::rebuildMeasurementsGrid()
{
    m_measurementsGrid->ClearRows();

    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() ) )
    {
        for( const auto& [ measurement, format ] : plotTab->Measurements() )
        {
            int row = m_measurementsGrid->GetNumberRows();
            m_measurementsGrid->AppendRows();
            m_measurementsGrid->SetCellValue( row, COL_MEASUREMENT, measurement );
            m_measurementsGrid->SetCellValue( row, COL_MEASUREMENT_FORMAT, format );
        }

        if( plotTab->GetSimType() == ST_TRAN || plotTab->GetSimType() == ST_AC
            || plotTab->GetSimType() == ST_DC || plotTab->GetSimType() == ST_SP )
        {
            m_measurementsGrid->AppendRows();   // Empty row at end
        }
    }
}


void SIMULATOR_FRAME_UI::onPlotDragged( wxAuiNotebookEvent& event )
{
}


std::shared_ptr<SPICE_SIMULATOR> SIMULATOR_FRAME_UI::simulator() const
{
    return m_simulatorFrame->GetSimulator();
}


std::shared_ptr<SPICE_CIRCUIT_MODEL> SIMULATOR_FRAME_UI::circuitModel() const
{
    return m_simulatorFrame->GetCircuitModel();
}


void SIMULATOR_FRAME_UI::updatePlotCursors()
{
    SUPPRESS_GRID_CELL_EVENTS raii( this );

    m_cursorsGrid->ClearRows();

    SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() );

    if( !plotTab )
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
                if( plotTab->GetSimType() == ST_AC )
                {
                    if( aTrace->GetType() & SPT_AC_PHASE )
                        return plotTab->GetUnitsY2();
                    else
                        return plotTab->GetUnitsY1();
                }
                else
                {
                    if( aTrace->GetType() & SPT_POWER )
                        return plotTab->GetUnitsY3();
                    else if( aTrace->GetType() & SPT_CURRENT )
                        return plotTab->GetUnitsY2();
                    else
                        return plotTab->GetUnitsY1();
                }
            };

    auto getNameY =
            [&]( TRACE* aTrace ) -> wxString
            {
                if( plotTab->GetSimType() == ST_AC )
                {
                    if( aTrace->GetType() & SPT_AC_PHASE )
                        return plotTab->GetLabelY2();
                    else
                        return plotTab->GetLabelY1();
                }
                else
                {
                    if( aTrace->GetType() & SPT_POWER )
                        return plotTab->GetLabelY3();
                    else if( aTrace->GetType() & SPT_CURRENT )
                        return plotTab->GetLabelY2();
                    else
                        return plotTab->GetLabelY1();
                }
            };

    auto formatValue =
            [this]( double aValue, int aCursorId, int aCol ) -> wxString
            {
                if( ( !m_simulatorFrame->SimFinished() && aCol == 1 ) || std::isnan( aValue ) )
                    return wxS( "--" );
                else
                    return SPICE_VALUE( aValue ).ToString( m_cursorFormatsDyn[ aCursorId ][ aCol ] );
            };

    for( const auto& [name, trace] : plotTab->GetTraces() )
    {
        if( CURSOR* cursor = trace->GetCursor( 1 ) )
        {
            cursor1 = cursor;
            cursor1Name = getNameY( trace );
            cursor1Units = getUnitsY( trace );

            wxRealPoint coords = cursor->GetCoords();
            int         row = m_cursorsGrid->GetNumberRows();

            m_cursorFormatsDyn[0][0].UpdateUnits( plotTab->GetUnitsX() );
            m_cursorFormatsDyn[0][1].UpdateUnits( cursor1Units );

            m_cursorsGrid->AppendRows( 1 );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_NAME, wxS( "1" ) );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_SIGNAL, cursor->GetName() );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_X, formatValue( coords.x, 0, 0 ) );
            m_cursorsGrid->SetCellValue( row, COL_CURSOR_Y, formatValue( coords.y, 0, 1 ) );
            break;
        }
    }

    for( const auto& [name, trace] : plotTab->GetTraces() )
    {
        if( CURSOR* cursor = trace->GetCursor( 2 ) )
        {
            cursor2 = cursor;
            cursor2Name = getNameY( trace );
            cursor2Units = getUnitsY( trace );

            wxRealPoint coords = cursor->GetCoords();
            int         row = m_cursorsGrid->GetNumberRows();

            m_cursorFormatsDyn[1][0].UpdateUnits( plotTab->GetUnitsX() );
            m_cursorFormatsDyn[1][1].UpdateUnits( cursor2Units );

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

        m_cursorFormatsDyn[2][0].UpdateUnits( plotTab->GetUnitsX() );
        m_cursorFormatsDyn[2][1].UpdateUnits( cursor1Units );

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
    m_cursorsGrid->SetColLabelValue( COL_CURSOR_X, plotTab->GetLabelX() );

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

    if( m_customCursorsCnt > 3 ) // 2 for the default hardocded cursors plus the initial + 1
    {
        for( int i = 3; i < m_customCursorsCnt; i++ )
        {
            for( const auto& [name, trace] : plotTab->GetTraces() )
            {
                if( CURSOR* cursor = trace->GetCursor( i ) )
                {
                    CURSOR* curs = cursor;
                    wxString cursName = getNameY( trace );
                    wxString cursUnits = getUnitsY( trace );

                    wxRealPoint coords = cursor->GetCoords();
                    int         row = m_cursorsGrid->GetNumberRows();

                    m_cursorFormatsDyn[i][0].UpdateUnits( plotTab->GetUnitsX() );
                    m_cursorFormatsDyn[i][1].UpdateUnits( cursUnits );

                    m_cursorsGrid->AppendRows( 1 );
                    m_cursorsGrid->SetCellValue( row, COL_CURSOR_NAME, wxS( "" ) + wxString( "" ) << i );
                    m_cursorsGrid->SetCellValue( row, COL_CURSOR_SIGNAL, curs->GetName() );
                    m_cursorsGrid->SetCellValue( row, COL_CURSOR_X, formatValue( coords.x, i, 0 ) );
                    m_cursorsGrid->SetCellValue( row, COL_CURSOR_Y, formatValue( coords.y, i, 1 ) );

                    // Set up the labels
                    m_cursorsGrid->SetColLabelValue( COL_CURSOR_X, plotTab->GetLabelX() );

                    valColName = _( "Value" );

                    if( !cursName.IsEmpty() && m_cursorsGrid->GetColLabelValue( COL_CURSOR_Y ) == cursName )
                        valColName = cursName;

                    m_cursorsGrid->SetColLabelValue( COL_CURSOR_Y, valColName );
                    break;
                }
            }
        }
    }
}


void SIMULATOR_FRAME_UI::onPlotCursorUpdate( wxCommandEvent& aEvent )
{
    updatePlotCursors();
    OnModify();
}


void SIMULATOR_FRAME_UI::OnSimUpdate()
{
    if( SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( GetCurrentSimTab() ) )
        plotTab->ResetScales( true );

    m_simConsole->Clear();

    prepareMultiRunState();

    // Do not export netlist, it is already stored in the simulator
    applyTuners();

    m_refreshTimer.Start( REFRESH_INTERVAL, wxTIMER_ONE_SHOT );
}


void SIMULATOR_FRAME_UI::OnSimReport( const wxString& aMsg )
{
    m_simConsole->AppendText( aMsg + "\n" );
    m_simConsole->SetInsertionPointEnd();
}


std::vector<wxString> SIMULATOR_FRAME_UI::SimPlotVectors() const
{
    std::vector<wxString> signals;

    for( const std::string& vec : simulator()->AllVectors() )
        signals.emplace_back( vec );

    return signals;
}


std::vector<wxString> SIMULATOR_FRAME_UI::Signals() const
{
    std::vector<wxString> signals;

    for( const wxString& signal : m_signals )
        signals.emplace_back( signal );

    for( const auto& [ id, signal ] : m_userDefinedSignals )
        signals.emplace_back( signal );

    sortSignals( signals );

    return signals;
}


void SIMULATOR_FRAME_UI::OnSimRefresh( bool aFinal )
{
    if( aFinal )
        m_refreshTimer.Stop();

    SIM_TAB* simTab = GetCurrentSimTab();

    if( !simTab )
        return;

    bool storeMultiRun = false;

    if( aFinal && m_multiRunState.active )
    {
        if( m_multiRunState.currentStep < m_multiRunState.steps.size() )
        {
            storeMultiRun = true;
            m_multiRunState.storePending = true;
        }
    }
    else
    {
        m_multiRunState.storePending = false;
    }

    SIM_TYPE simType = simTab->GetSimType();
    wxString msg;

    if( aFinal )
    {
        applyUserDefinedSignals();
        updateSignalsGrid();
    }

    // If there are any signals plotted, update them
    if( SIM_TAB::IsPlottable( simType ) )
    {
        simTab->SetSpicePlotName( simulator()->CurrentPlotName() );

        if( simType == ST_NOISE && aFinal )
        {
            m_simConsole->AppendText( _( "\n\nSimulation results:\n\n" ) );
            m_simConsole->SetInsertionPointEnd();

            // The simulator will create noise1 & noise2 on the first run, noise3 and noise4
            // on the second, etc.  The first plot for each run contains the spectral density
            // noise vectors and second contains the integrated noise.
            long number;
            simulator()->CurrentPlotName().Mid( 5 ).ToLong( &number );

            for( const std::string& vec : simulator()->AllVectors() )
            {
                std::vector<double> val_list = simulator()->GetRealVector( vec, 1 );
                wxString            value = SPICE_VALUE( val_list[ 0 ] ).ToSpiceString();

                msg.Printf( wxS( "%s: %sV\n" ), vec, value );

                m_simConsole->AppendText( msg );
                m_simConsole->SetInsertionPointEnd();
            }

            simulator()->Command( fmt::format( "setplot noise{}", number - 1 ) );
            simTab->SetSpicePlotName( simulator()->CurrentPlotName() );
        }

        SIM_PLOT_TAB* plotTab = dynamic_cast<SIM_PLOT_TAB*>( simTab );
        wxCHECK_RET( plotTab, wxString::Format( wxT( "No SIM_PLOT_TAB for: %s" ),
                                                magic_enum::enum_name( simType ) ) );

        struct TRACE_INFO
        {
            wxString Vector;
            int      TraceType;
            bool     ClearData;
        };

        std::map<TRACE*, TRACE_INFO> traceMap;

        for( const auto& [ name, trace ] : plotTab->GetTraces() )
            traceMap[ trace ] = { wxEmptyString, SPT_UNKNOWN, false };

        // NB: m_signals are already broken out into gain/phase, but m_userDefinedSignals are
        // as the user typed them

        for( const wxString& signal : m_signals )
        {
            int      traceType = SPT_UNKNOWN;
            wxString vectorName = vectorNameFromSignalName( plotTab, signal, &traceType );

            if( TRACE* trace = plotTab->GetTrace( vectorName, traceType ) )
                traceMap[ trace ] = { vectorName, traceType, false };
        }

        for( const auto& [ id, signal ] : m_userDefinedSignals )
        {
            int      traceType = SPT_UNKNOWN;
            wxString vectorName = vectorNameFromSignalName( plotTab, signal, &traceType );

            if( simType == ST_AC )
            {
                int baseType = traceType &= ~( SPT_AC_GAIN | SPT_AC_PHASE );

                for( int subType : { baseType | SPT_AC_GAIN, baseType | SPT_AC_PHASE } )
                {
                    if( TRACE* trace = plotTab->GetTrace( vectorName, subType ) )
                        traceMap[ trace ] = { vectorName, subType, !aFinal };
                }
            }
            else if( simType == ST_SP )
            {
                int baseType = traceType &= ~( SPT_SP_AMP | SPT_AC_PHASE );

                for( int subType : { baseType | SPT_SP_AMP, baseType | SPT_AC_PHASE } )
                {
                    if( TRACE* trace = plotTab->GetTrace( vectorName, subType ) )
                        traceMap[trace] = { vectorName, subType, !aFinal };
                }
            }
            else
            {
                if( TRACE* trace = plotTab->GetTrace( vectorName, traceType ) )
                    traceMap[ trace ] = { vectorName, traceType, !aFinal };
            }
        }

        // Two passes so that DC-sweep sub-traces get deleted and re-created:

        for( const auto& [ trace, traceInfo ] : traceMap )
        {
            if( traceInfo.Vector.IsEmpty() )
                plotTab->DeleteTrace( trace );
        }

        for( const auto& [ trace, info ] : traceMap )
        {
            std::vector<double> data_x;

            if( !info.Vector.IsEmpty() )
                updateTrace( info.Vector, info.TraceType, plotTab, &data_x, info.ClearData );
        }

        plotTab->GetPlotWin()->UpdateAll();

        if( aFinal )
        {
            for( int row = 0; row < m_measurementsGrid->GetNumberRows(); ++row )
                UpdateMeasurement( row );

            plotTab->ResetScales( true );
        }

        plotTab->GetPlotWin()->Fit();

        updatePlotCursors();
    }
    else if( simType == ST_OP && aFinal )
    {
        m_simConsole->AppendText( _( "\n\nSimulation results:\n\n" ) );
        m_simConsole->SetInsertionPointEnd();

        for( const std::string& vec : simulator()->AllVectors() )
        {
            std::vector<double> val_list = simulator()->GetRealVector( vec, 1 );

            if( val_list.empty() )
                continue;

            wxString            value = SPICE_VALUE( val_list[ 0 ] ).ToSpiceString();
            wxString            signal;
            SIM_TRACE_TYPE      type = circuitModel()->VectorToSignal( vec, signal );

            const size_t tab = 25; //characters
            size_t       padding = ( signal.length() < tab ) ? ( tab - signal.length() ) : 1;

            switch( type )
            {
            case SPT_VOLTAGE: value.Append( wxS( "V" ) ); break;
            case SPT_CURRENT: value.Append( wxS( "A" ) ); break;
            case SPT_POWER:   value.Append( wxS( "W" ) ); break;
            default:          value.Append( wxS( "?" ) ); break;
            }

            msg.Printf( wxT( "%s%s\n" ),
                        ( signal + wxT( ":" ) ).Pad( padding, wxUniChar( ' ' ) ),
                        value );

            m_simConsole->AppendText( msg );
            m_simConsole->SetInsertionPointEnd();

            if( type == SPT_VOLTAGE || type == SPT_CURRENT || type == SPT_POWER )
                signal = signal.SubString( 2, signal.Length() - 2 );

            if( type == SPT_POWER )
                signal += wxS( ":power" );

            m_schematicFrame->Schematic().SetOperatingPoint( signal, val_list.at( 0 ) );
        }
    }
    else if( simType == ST_PZ && aFinal )
    {
        m_simConsole->AppendText( _( "\n\nSimulation results:\n\n" ) );
        m_simConsole->SetInsertionPointEnd();
        simulator()->Command( "print all" );
    }

    if( storeMultiRun )
    {
        m_multiRunState.storePending = false;
        m_multiRunState.storedSteps = m_multiRunState.currentStep + 1;
    }

    if( aFinal && m_multiRunState.active )
    {
        if( m_multiRunState.currentStep + 1 < m_multiRunState.steps.size() )
        {
            m_multiRunState.currentStep++;

            wxQueueEvent( m_simulatorFrame, new wxCommandEvent( EVT_SIM_UPDATE ) );
        }
        else
        {
            m_multiRunState.active = false;
            m_multiRunState.steps.clear();
            m_multiRunState.currentStep = 0;
            m_multiRunState.storePending = false;
            m_tunerOverrides.clear();

            if( !m_multiRunState.traces.empty() )
            {
                auto iter = m_multiRunState.traces.begin();

                if( iter != m_multiRunState.traces.end() )
                    m_multiRunState.storedSteps = iter->second.yValues.size();
            }
        }
    }
}


void SIMULATOR_FRAME_UI::clearMultiRunState( bool aClearTraces )
{
    m_multiRunState.active = false;
    m_multiRunState.tuners.clear();
    m_multiRunState.steps.clear();
    m_multiRunState.currentStep = 0;
    m_multiRunState.storePending = false;

    if( aClearTraces )
    {
        m_multiRunState.traces.clear();
        m_multiRunState.storedSteps = 0;
    }

    m_tunerOverrides.clear();
}


void SIMULATOR_FRAME_UI::prepareMultiRunState()
{
    m_tunerOverrides.clear();

    std::vector<TUNER_SLIDER*> multiTuners;

    for( TUNER_SLIDER* tuner : m_tuners )
    {
        if( tuner->GetRunMode() == TUNER_SLIDER::RUN_MODE::MULTI )
            multiTuners.push_back( tuner );
    }

    if( multiTuners.empty() )
    {
        clearMultiRunState( true );
        return;
    }

    bool tunersChanged = multiTuners != m_multiRunState.tuners;

    if( m_multiRunState.active && tunersChanged )
        clearMultiRunState( true );

    if( !m_multiRunState.active )
    {
        if( tunersChanged || m_multiRunState.storedSteps > 0 || !m_multiRunState.traces.empty() )
            clearMultiRunState( true );

        m_multiRunState.tuners = multiTuners;
        m_multiRunState.steps = calculateMultiRunSteps( multiTuners );
        m_multiRunState.currentStep = 0;
        m_multiRunState.storePending = false;

        if( m_multiRunState.steps.size() >= 2 )
        {
            m_multiRunState.active = true;
            m_multiRunState.storedSteps = 0;
        }
        else
        {
            m_multiRunState.steps.clear();
            return;
        }
    }
    else if( tunersChanged )
    {
        m_multiRunState.tuners = multiTuners;
    }

    if( m_multiRunState.active && m_multiRunState.currentStep < m_multiRunState.steps.size() )
    {
        const MULTI_RUN_STEP& step = m_multiRunState.steps[m_multiRunState.currentStep];

        for( const auto& entry : step.overrides )
            m_tunerOverrides[entry.first] = entry.second;
    }
}


std::vector<SIMULATOR_FRAME_UI::MULTI_RUN_STEP> SIMULATOR_FRAME_UI::calculateMultiRunSteps(
        const std::vector<TUNER_SLIDER*>& aTuners ) const
{
    std::vector<MULTI_RUN_STEP> steps;

    if( aTuners.empty() )
        return steps;

    std::vector<std::vector<double>> tunerValues;
    tunerValues.reserve( aTuners.size() );

    for( TUNER_SLIDER* tuner : aTuners )
    {
        if( !tuner )
            return steps;

        double startValue = tuner->GetMin().ToDouble();
        double endValue = tuner->GetMax().ToDouble();
        int    stepCount = std::max( 2, tuner->GetStepCount() );

        if( stepCount < 2 )
            stepCount = 2;

        double increment = ( endValue - startValue ) / static_cast<double>( stepCount - 1 );

        std::vector<double> values;
        values.reserve( stepCount );

        for( int ii = 0; ii < stepCount; ++ii )
            values.push_back( startValue + increment * ii );

        tunerValues.push_back( std::move( values ) );
    }

    int limit = ADVANCED_CFG::GetCfg().m_SimulatorMultiRunCombinationLimit;

    if( limit < 1 )
        limit = 1;

    std::vector<double> currentValues( aTuners.size(), 0.0 );

    auto generate = [&]( auto&& self, size_t depth ) -> void
    {
        if( steps.size() >= static_cast<size_t>( limit ) )
            return;

        if( depth == aTuners.size() )
        {
            MULTI_RUN_STEP step;

            for( size_t ii = 0; ii < aTuners.size(); ++ii )
                step.overrides.emplace( aTuners[ii], currentValues[ii] );

            steps.push_back( std::move( step ) );
            return;
        }

        for( double value : tunerValues[depth] )
        {
            currentValues[depth] = value;
            self( self, depth + 1 );

            if( steps.size() >= static_cast<size_t>( limit ) )
                return;
        }
    };

    generate( generate, 0 );

    return steps;
}


std::string SIMULATOR_FRAME_UI::multiRunTraceKey( const wxString& aVectorName, int aTraceType ) const
{
    return fmt::format( "{}|{}", aVectorName.ToStdString(), aTraceType );
}


void SIMULATOR_FRAME_UI::recordMultiRunData( const wxString& aVectorName, int aTraceType,
                                             const std::vector<double>& aX,
                                             const std::vector<double>& aY )
{
    if( aX.empty() || aY.empty() )
        return;

    std::string key = multiRunTraceKey( aVectorName, aTraceType );
    MULTI_RUN_TRACE& trace = m_multiRunState.traces[key];

    trace.traceType = aTraceType;

    if( trace.xValues.empty() )
        trace.xValues = aX;

    if( trace.xValues.size() != aX.size() )
        return;

    size_t index = m_multiRunState.currentStep;

    if( trace.yValues.size() <= index )
        trace.yValues.resize( index + 1 );

    trace.yValues[index] = aY;
}


bool SIMULATOR_FRAME_UI::hasMultiRunTrace( const wxString& aVectorName, int aTraceType ) const
{
    std::string key = multiRunTraceKey( aVectorName, aTraceType );
    auto        it = m_multiRunState.traces.find( key );

    if( it == m_multiRunState.traces.end() )
        return false;

    const MULTI_RUN_TRACE& trace = it->second;

    return !trace.xValues.empty() && !trace.yValues.empty();
}


void SIMULATOR_FRAME_UI::OnModify()
{
    m_simulatorFrame->OnModify();
}
