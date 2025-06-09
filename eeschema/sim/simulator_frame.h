/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#include <sim/simulator_frame_ui_base.h>
#include <sim/sim_types.h>

#include <kiway_player.h>
#include <dialogs/dialog_sim_command.h>

#include <wx/event.h>

#include <list>
#include <memory>
#include <map>

class SCH_EDIT_FRAME;
class SCH_SYMBOL;
class SIMULATOR_FRAME_UI;
class SIM_THREAD_REPORTER;
class ACTION_TOOLBAR;
class SPICE_SIMULATOR;


/**
 * Simple error container for failure to init the simulation engine
 * and ultimately abort the frame construction
 */
class SIMULATOR_INIT_ERR : public std::runtime_error
{
public:
    explicit SIMULATOR_INIT_ERR(const std::string& what_arg)
        : std::runtime_error(what_arg) {}
};


/**
 *
 * The SIMULATOR_FRAME holds the main user-interface for running simulations.
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


class SIMULATOR_FRAME : public KIWAY_PLAYER
{
public:
    SIMULATOR_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~SIMULATOR_FRAME();

    /**
     * Check and load the current netlist into the simulator.
     * @return true if document is fully annotated and netlist was loaded successfully.
     */
    bool LoadSimulator( const wxString& aSimCommand, unsigned aSimOptions );

    /**
     * Re-send the current command and settings to the simulator.  Use the existing netlist.
     */
    void ReloadSimulator( const wxString& aSimCommand, unsigned aSimOptions );

    void StartSimulation();

    /**
     * Create a new plot tab for a given simulation type.
     *
     * @param aSimCommand is requested simulation command.
     */
    SIM_TAB* NewSimTab( const wxString& aSimCommand );

    /**
     * Shows a dialog for editing the current tab's simulation command, or creating a new tab
     * with a different simulation command type.
     */
    bool EditAnalysis();

    /**
     * @return the list of vectors (signals) in the current simulation results.
     */
    const std::vector<wxString> SimPlotVectors();

    /**
     * @return the list of schematic signals + any user defined signals.
     */
    const std::vector<wxString> Signals();

    const std::map<int, wxString>& UserDefinedSignals();

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
     * Add a tuner for a symbol.
     */
    void AddTuner( const SCH_SHEET_PATH& aSheetPath, SCH_SYMBOL* aSymbol );

    /**
     * Return the current tab (or NULL if there is none).
     */
    SIM_TAB* GetCurrentSimTab() const;

    void ToggleSimConsole();

    void ToggleSimSidePanel();

    /**
     * Toggle dark-mode of the plot tabs.
     */
    void ToggleDarkModePlots();

    void ShowChangedLanguage() override;

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

    void CommonSettingsChanged( int aFlags ) override;

    WINDOW_SETTINGS* GetWindowSettings( APP_SETTINGS_BASE* aCfg ) override;

    SCH_EDIT_FRAME* GetSchematicFrame() const { return m_schematicFrame; }

    std::shared_ptr<SPICE_CIRCUIT_MODEL> GetCircuitModel() const { return m_circuitModel; }

    std::shared_ptr<SPICE_SIMULATOR> GetSimulator() const { return m_simulator; }

    wxString GetCurrentSimCommand() const;
    SIM_TYPE GetCurrentSimType() const;
    int GetCurrentOptions() const;

    bool SimFinished() const { return m_simFinished; }

    // Simulator doesn't host a canvas
    wxWindow* GetToolCanvas() const override { return nullptr; }

    /**
     * Set the main window title bar text.
     */
    void UpdateTitle();

    void OnModify() override;

    DECLARE_EVENT_TABLE()

private:
    void setupTools();
    void doReCreateMenuBar() override;

    void setupUIConditions() override;

    void showNetlistErrors( const WX_STRING_REPORTER& aReporter );

    bool canCloseWindow( wxCloseEvent& aEvent ) override;
    void doCloseWindow() override;

    void onUpdateSim( wxCommandEvent& aEvent );
    void onSimReport( wxCommandEvent& aEvent );
    void onSimStarted( wxCommandEvent& aEvent );
    void onSimFinished( wxCommandEvent& aEvent );

    void onExit( wxCommandEvent& event );

private:
    SCH_EDIT_FRAME*                      m_schematicFrame;
    ACTION_TOOLBAR*                      m_toolBar;
    SIMULATOR_FRAME_UI*                  m_ui;

    std::shared_ptr<SPICE_SIMULATOR>     m_simulator;
    SIM_THREAD_REPORTER*                 m_reporter;
    std::shared_ptr<SPICE_CIRCUIT_MODEL> m_circuitModel;

    bool                                 m_simFinished;
    bool                                 m_workbookModified;
};

// Commands
wxDECLARE_EVENT( EVT_SIM_UPDATE, wxCommandEvent );
wxDECLARE_EVENT( EVT_SIM_REPORT, wxCommandEvent );

// Notifications
wxDECLARE_EVENT( EVT_SIM_STARTED, wxCommandEvent );
wxDECLARE_EVENT( EVT_SIM_FINISHED, wxCommandEvent );

#endif // SIMULATOR_FRAME_H
