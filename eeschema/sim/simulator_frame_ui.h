/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2023 CERN
 * Copyright (C) 2017-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef SIMULATOR_FRAME_UI_H
#define SIMULATOR_FRAME_UI_H


#include <sim/simulator_frame_ui_base.h>
#include <sim/sim_types.h>
#include <sim/sim_plot_tab.h>

#include <wx/event.h>

class SCH_EDIT_FRAME;
class SCH_SYMBOL;

class SPICE_SIMULATOR;
class SPICE_SETTINGS;
class EESCHEMA_SETTINGS;
class SPICE_CIRCUIT_MODEL;

class SIM_THREAD_REPORTER;
class TUNER_SLIDER;


/**
 *
 * The SIMULATOR_FRAME_UI holds the main user-interface for running simulations.
 *
 * It contains a workbook with multiple tabs, each tab holding a SIM_PLOT_TAB, a specific
 * simulation command (.TRAN, .AC, etc.), and simulation settings (save all currents, etc.).
 *
 * Each plot can have multiple TRACEs.  While internally each TRACE can have multiple cursors,
 * the GUI supports only two cursors (and a differential cursor) for each plot.
 *
 * TRACEs are identified by a signal (V(OUT), I(R2), etc.) and a type (SPT_VOLTAGE, SPT_AC_PHASE,
 * etc.).
 *
 * The simulator outputs simple signals in a vector of the same name.  Complex signals (such as
 * V(OUT) / V(IN)) are stored in vectors of the format "user%d".
 *
 */


class SIMULATOR_FRAME_UI : public SIMULATOR_FRAME_UI_BASE
{
public:
    SIMULATOR_FRAME_UI( SIMULATOR_FRAME* aSimulatorFrame, SCH_EDIT_FRAME* aSchematicFrame );
    ~SIMULATOR_FRAME_UI();

    /**
     * Create a new simulation tab for a given simulation type.
     *
     * @param aSimCommand is requested simulation command.
     */
    SIM_TAB* NewSimTab( const wxString& aSimCommand );

    std::vector<wxString> SimPlotVectors() const;

    std::vector<wxString> Signals() const;

    const std::map<int, wxString>& UserDefinedSignals() { return m_userDefinedSignals; }
    void SetUserDefinedSignals( const std::map<int, wxString>& aSignals );

    /**
     * Add a new trace to the current plot.
     *
     * @param aName is the device/net name.
     * @param aType describes the type of trace.
     */
    void AddTrace( const wxString& aName, SIM_TRACE_TYPE aType );

    /**
     * Get/Set the number of significant digits and the range for formatting a cursor value.
     * @param aValueCol 0 indicates the X value column; 1 the Y value.
     */
    SPICE_VALUE_FORMAT GetCursorFormat( int aCursorId, int aValueCol ) const
    {
        return m_cursorFormats[ aCursorId ][ aValueCol ];
    }

    void SetCursorFormat( int aCursorId, int aValueCol, const SPICE_VALUE_FORMAT& aFormat )
    {
        m_cursorFormats[ aCursorId ][ aValueCol ] = aFormat;

        wxCommandEvent dummy;
        onPlotCursorUpdate( dummy );
    }

    /**
     * Add a tuner for a symbol.
     */
    void AddTuner( const SCH_SHEET_PATH& aSheetPath, SCH_SYMBOL* aSymbol );

    /**
     * Remove an existing tuner.
     */
    void RemoveTuner( TUNER_SLIDER* aTuner );

    /**
     * Safely update a field of the associated symbol without dereferencing
     * the symbol.
     *
     * @param aSymbol id of the symbol needing updating
     * @param aId id of the symbol field
     * @param aValue new value of the symbol field
     */
    void UpdateTunerValue( const SCH_SHEET_PATH& aSheetPath, const KIID& aSymbol,
                           const wxString& aRef, const wxString& aValue );

    /**
     * Add a measurement to the measurements grid.
     */
    void AddMeasurement( const wxString& aCmd );

    /**
     * Delete a row from the measurements grid.
     */
    void DeleteMeasurement( int aRow );

    /**
     * Get/Set the format of a value in the measurements grid.
     */
    SPICE_VALUE_FORMAT GetMeasureFormat( int aRow ) const;
    void SetMeasureFormat( int aRow, const SPICE_VALUE_FORMAT& aFormat );

    /**
     * Update a measurement in the measurements grid.
     */
    void UpdateMeasurement( int aRow );

    void DoFourier( const wxString& aSignal, const wxString& aFundamental );

    /**
     * Return the netlist exporter object used for simulations.
     */
    const SPICE_CIRCUIT_MODEL* GetExporter() const;

    bool DarkModePlots() const { return m_darkMode; }
    void ToggleDarkModePlots();

    void ShowChangedLanguage();

    /**
     * Load the currently active workbook stored in the project settings. If there is none,
     * generate a filename for the currently active workbook and store it in the project settings.
     */
    void InitWorkbook();

    /**
     * Load plot, signal, cursor, measurement, etc. settings from a file.
     */
    bool LoadWorkbook( const wxString& aPath );

    /**
     * Save plot, signal, cursor, measurement, etc. settings to a file.
     */
    bool SaveWorkbook( const wxString& aPath );

    void LoadSettings( EESCHEMA_SETTINGS* aCfg );

    void SaveSettings( EESCHEMA_SETTINGS* aCfg );

    // adjust the sash dimension of splitter windows after reading
    // the config settings
    // must be called after the config settings are read, and once the
    // frame is initialized (end of the Ctor)
    void SetSubWindowsSashSize();

    /**
     * Return the currently opened plot panel (or NULL if there is none).
     */
    SIM_TAB* GetCurrentSimTab() const
    {
        return dynamic_cast<SIM_TAB*>( m_plotNotebook->GetCurrentPage() );
    }

    SIM_TAB* GetSimTab( SIM_TYPE aType ) const
    {
        for( int ii = 0; ii < (int) m_plotNotebook->GetPageCount(); ++ii )
        {
            SIM_TAB* candidate = dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( ii ) );

            if( candidate && candidate->GetSimType() == aType )
                return candidate;
        }

        return nullptr;
    }

    int GetSimTabIndex( SIM_TAB* aPlot ) const
    {
        return m_plotNotebook->GetPageIndex( aPlot );
    }

    void OnPlotSettingsChanged();

    void OnSimUpdate();
    void OnSimReport( const wxString& aMsg );
    void OnSimRefresh( bool aFinal );

private:
    /**
     * Get the simulator output vector name for a given signal name and type.
     */
    wxString vectorNameFromSignalName( SIM_PLOT_TAB* aPlotTab, const wxString& aSignalName,
                                       int* aTraceType );

    /**
     * Update a trace in a particular SIM_PLOT_TAB.  If the panel does not contain the given
     * trace, then add it.
     *
     * @param aVectorName is the SPICE vector name, such as "I(Net-C1-Pad1)".
     * @param aTraceType describes the type of plot.
     * @param aPlotTab is the tab that should receive the update.
     */
    void updateTrace( const wxString& aVectorName, int aTraceType, SIM_PLOT_TAB* aPlotTab );

    /**
     * Rebuild the list of signals available from the netlist.
     *
     * Note: this is not the filtered list.  See rebuildSignalsGrid() for that.
     */
    void rebuildSignalsList();

    /**
     * Rebuild the filtered list of signals in the signals grid.
     */
    void rebuildSignalsGrid( wxString aFilter );

    /**
     * Update the values in the signals grid.
     */
    void updateSignalsGrid();

    /**
     * Update the cursor values (in the grid) and graphics (in the plot window).
     */
    void updatePlotCursors();

    /**
     * Apply user-defined signals to the SPICE session.
     */
    void applyUserDefinedSignals();

    /**
     * Rebuild the measurements grid for the current plot.
     */
    void rebuildMeasurementsGrid();

    /**
     * Apply component values specified using tuner sliders to the current netlist.
     */
    void applyTuners();

    /**
     * Return X axis for a given simulation type.
     */
    SIM_TRACE_TYPE getXAxisType( SIM_TYPE aType ) const;

    wxString getNoiseSource() const;

    void parseTraceParams( SIM_PLOT_TAB* aPlotTab, TRACE* aTrace, const wxString& aSignalName,
                           const wxString& aParams );

    std::shared_ptr<SPICE_SIMULATOR> simulator() const;
    std::shared_ptr<SPICE_CIRCUIT_MODEL> circuitModel() const;

    // Event handlers
    void onPlotClose( wxAuiNotebookEvent& event ) override;
    void onPlotClosed( wxAuiNotebookEvent& event ) override;
    void onPlotChanging( wxAuiNotebookEvent& event ) override;
    void onPlotChanged( wxAuiNotebookEvent& event ) override;
    void onPlotDragged( wxAuiNotebookEvent& event ) override;

    void OnFilterText( wxCommandEvent& aEvent ) override;
    void OnFilterMouseMoved( wxMouseEvent& aEvent ) override;

    void onSignalsGridCellChanged( wxGridEvent& aEvent ) override;
    void onCursorsGridCellChanged( wxGridEvent& aEvent ) override;
    void onMeasurementsGridCellChanged( wxGridEvent& aEvent ) override;

    bool loadLegacyWorkbook( const wxString & aPath );
    bool loadJsonWorkbook( const wxString & aPath );

    void onNotebookModified( wxCommandEvent& event );

    void onPlotCursorUpdate( wxCommandEvent& aEvent );

public:
    int                          m_SuppressGridEvents;

private:
    SIMULATOR_FRAME*             m_simulatorFrame;
    SCH_EDIT_FRAME*              m_schematicFrame;

    std::vector<wxString>        m_signals;
    std::map<int, wxString>      m_userDefinedSignals;
    std::list<TUNER_SLIDER*>     m_tuners;

    ///< SPICE expressions need quoted versions of the netnames since KiCad allows '-' and '/'
    ///< in netnames.
    std::map<wxString, wxString> m_quotedNetnames;

    SPICE_VALUE_FORMAT           m_cursorFormats[3][2];

    // Variables for temporary storage:
    int                          m_splitterLeftRightSashPosition;
    int                          m_splitterPlotAndConsoleSashPosition;
    int                          m_splitterSignalsSashPosition;
    int                          m_splitterCursorsSashPosition;
    int                          m_splitterTuneValuesSashPosition;
    bool                         m_darkMode;
    unsigned int                 m_plotNumber;
    wxTimer                      m_refreshTimer;
};

#endif // SIMULATOR_FRAME_UI_H
