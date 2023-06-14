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

#ifndef SIMULATOR_FRAME_H
#define SIMULATOR_FRAME_H


#include <sim/simulator_frame_base.h>
#include <sim/sim_types.h>

#include <kiway_player.h>
#include <dialogs/dialog_sim_command.h>

#include <wx/event.h>

#include <list>
#include <memory>
#include <map>

class SCH_EDIT_FRAME;
class SCH_SYMBOL;

class SPICE_SIMULATOR;
class SPICE_SIMULATOR_SETTINGS;
class NGSPICE_CIRCUIT_MODEL;

#include <sim/sim_plot_panel.h>
#include <sim/sim_plot_panel_base.h>
#include "widgets/sim_notebook.h"

class SIM_THREAD_REPORTER;
class TUNER_SLIDER;


/**
 *
 * The SIMULATOR_FRAME holds the main user-interface for running simulations.
 *
 * It contains a workbook with multiple tabs, each tab holding a SIM_PLOT_PANEL, a specific
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


class SIMULATOR_FRAME : public SIMULATOR_FRAME_BASE
{
public:
    SIMULATOR_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~SIMULATOR_FRAME();

    /**
     * Check and load the current netlist into the simulator.
     * @return true if document is fully annotated and netlist was loaded successfully.
     */
    bool LoadSimulator();

    void StartSimulation();

    /**
     * Create a new plot tab for a given simulation type.
     *
     * @param aSimCommand is requested simulation command.
     * @param aSimOptions netlisting options
     * @return The new plot panel.
     */
    SIM_PLOT_PANEL_BASE* NewPlotPanel( const wxString& aSimCommand, int aSimOptions );

    /**
     * Shows a dialog for editing the current tab's simulation command, or creating a new tab
     * with a different simulation command type.
     */
    bool EditSimCommand();

    const std::vector<wxString>& Signals() { return m_signals; }

    const std::map<int, wxString>& UserDefinedSignals() { return m_userDefinedSignals; }
    void SetUserDefinedSignals( const std::map<int, wxString>& aSignals );

    /**
     * Add a voltage trace for a given net to the current plot.
     *
     * @param aNetName is the net name for which a voltage plot should be created.
     */
    void AddVoltageTrace( const wxString& aNetName );

    /**
     * Add a current trace for a given device to the current plot.
     *
     * @param aDeviceName is the device name (e.g. R1, C1).
     * @param aParam is the current type (e.g. I, Ic, Id).
     */
    void AddCurrentTrace( const wxString& aDeviceName );

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
        onCursorUpdate( dummy );
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

    /**
     * Return the current tab (or NULL if there is none).
     */
    SIM_PLOT_PANEL* GetCurrentPlot() const;

    /**
     * Return the netlist exporter object used for simulations.
     */
    const NGSPICE_CIRCUIT_MODEL* GetExporter() const;

    /**
     * Toggle dark-mode of the plot tabs.
     */
    void ToggleDarkModePlots();

    void ShowChangedLanguage() override;

    void ReCreateHToolbar();

    /**
     * Load plot, signal, cursor, measurement, etc. settings from a file.
     */
    bool LoadWorkbook( const wxString& aPath );

    /**
     * Save plot, signal, cursor, measurement, etc. settings to a file.
     */
    bool SaveWorkbook( const wxString& aPath );

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;

    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    WINDOW_SETTINGS* GetWindowSettings( APP_SETTINGS_BASE* aCfg ) override;

    SCH_EDIT_FRAME* GetSchematicFrame() const { return m_schematicFrame; }

    std::shared_ptr<NGSPICE_CIRCUIT_MODEL> GetCircuitModel() const { return m_circuitModel; }

    std::shared_ptr<SPICE_SIMULATOR> GetSimulator() const { return m_simulator; }

    wxString GetCurrentSimCommand() const
    {
        if( getCurrentPlotWindow() )
            return getCurrentPlotWindow()->GetSimCommand();
        else
            return m_circuitModel->GetSchTextSimCommand();
    }

    int GetCurrentOptions() const
    {
        if( getCurrentPlotWindow() )
            return getCurrentPlotWindow()->GetSimOptions();
        else
            return m_circuitModel->GetSimOptions();
    }

    // Simulator doesn't host a canvas
    wxWindow* GetToolCanvas() const override { return nullptr; }

    void OnModify() override;

    DECLARE_EVENT_TABLE()

private:
    void setupTools();
    void doReCreateMenuBar() override;

    void setupUIConditions() override;

    /**
     * Load the currently active workbook stored in the project settings. If there is none,
     * generate a filename for the currently active workbook and store it in the project settings.
     */
    void initWorkbook();

    /**
     * Set the main window title bar text.
     */
    void updateTitle();

    /**
     * Add a new trace to the current plot.
     *
     * @param aName is the device/net name.
     * @param aType describes the type of trace.
     */
    void doAddTrace( const wxString& aName, SIM_TRACE_TYPE aType );

    /**
     * Get the simulator output vector name for a given signal name and type.
     */
    wxString vectorNameFromSignalName( const wxString& aSignalName, int* aTraceType );

    /**
     * Update a trace in a particular SIM_PLOT_PANEL.  If the panel does not contain the given
     * trace, then add it.
     *
     * @param aVectorName is the SPICE vector name, such as "I(Net-C1-Pad1)".
     * @param aTraceType describes the type of plot.
     * @param aPlotPanel is the panel that should receive the update.
     */
    void updateTrace( const wxString& aVectorName, int aTraceType, SIM_PLOT_PANEL* aPlotPanel );

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
    void updateCursors();

    /**
     * Apply user-defined signals to the SPICE session.
     */
    void applyUserDefinedSignals();

    /**
     * Apply component values specified using tuner sliders to the current netlist.
     */
    void applyTuners();

    /**
     * Return the currently opened plot panel (or NULL if there is none).
     */
    SIM_PLOT_PANEL_BASE* getCurrentPlotWindow() const
    {
        return dynamic_cast<SIM_PLOT_PANEL_BASE*>( m_plotNotebook->GetCurrentPage() );
    }

    /**
     * Return X axis for a given simulation type.
     */
    SIM_TRACE_TYPE getXAxisType( SIM_TYPE aType ) const;

    void parseTraceParams( SIM_PLOT_PANEL* aPlotPanel, TRACE* aTrace, const wxString& aSignalName,
                           const wxString& aParams );

    // Event handlers
    void onPlotClose( wxAuiNotebookEvent& event ) override;
    void onPlotClosed( wxAuiNotebookEvent& event ) override;
    void onPlotChanged( wxAuiNotebookEvent& event ) override;
    void onPlotDragged( wxAuiNotebookEvent& event ) override;

    void OnFilterText( wxCommandEvent& aEvent ) override;
    void OnFilterMouseMoved( wxMouseEvent& aEvent ) override;

    void onSignalsGridCellChanged( wxGridEvent& aEvent ) override;
    void onCursorsGridCellChanged( wxGridEvent& aEvent ) override;
    void onMeasurementsGridCellChanged( wxGridEvent& aEvent ) override;

    void onNotebookModified( wxCommandEvent& event );

    bool canCloseWindow( wxCloseEvent& aEvent ) override;
    void doCloseWindow() override;

    void onCursorUpdate( wxCommandEvent& aEvent );
    void onSimUpdate( wxCommandEvent& aEvent );
    void onSimReport( wxCommandEvent& aEvent );
    void onSimStarted( wxCommandEvent& aEvent );
    void onSimFinished( wxCommandEvent& aEvent );

    void onExit( wxCommandEvent& event );

    // adjust the sash dimension of splitter windows after reading
    // the config settings
    // must be called after the config settings are read, and once the
    // frame is initialized (end of the Ctor)
    void setSubWindowsSashSize();

public:
    int                                    m_SuppressGridEvents;

private:
    SCH_EDIT_FRAME*                        m_schematicFrame;
    std::shared_ptr<NGSPICE_CIRCUIT_MODEL> m_circuitModel;
    std::shared_ptr<SPICE_SIMULATOR>       m_simulator;
    SIM_THREAD_REPORTER*                   m_reporter;

    std::vector<wxString>                  m_signals;
    std::map<int, wxString>                m_userDefinedSignals;
    std::list<TUNER_SLIDER*>               m_tuners;

    ///< SPICE expressions need quoted versions of the netnames since KiCad allows '-' and '/'
    ///< in netnames.
    std::map<wxString, wxString>           m_quotedNetnames;


    ///< Panel that was used as the most recent one for simulations
    SIM_PLOT_PANEL_BASE*                        m_lastSimPlot;

    SPICE_VALUE_FORMAT                     m_cursorFormats[3][2];

    // Variables for temporary storage:
    int                   m_splitterLeftRightSashPosition;
    int                   m_splitterPlotAndConsoleSashPosition;
    int                   m_splitterSignalsSashPosition;
    int                   m_splitterCursorsSashPosition;
    int                   m_splitterTuneValuesSashPosition;
    bool                  m_darkMode;
    unsigned int          m_plotNumber;
    bool                  m_simFinished;
    bool                  m_workbookModified;
};

// Commands
wxDECLARE_EVENT( EVT_SIM_UPDATE, wxCommandEvent );
wxDECLARE_EVENT( EVT_SIM_REPORT, wxCommandEvent );

// Notifications
wxDECLARE_EVENT( EVT_SIM_STARTED, wxCommandEvent );
wxDECLARE_EVENT( EVT_SIM_FINISHED, wxCommandEvent );

#endif // SIMULATOR_FRAME_H
