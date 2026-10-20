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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once


#include <sim/simulator_frame_ui_base.h>
#include <sim/sim_types.h>
#include <sim/sim_plot_tab.h>
#include <sim/sim_preferences.h>
#include <sim/simulator_frame.h>

#include <wx/event.h>

#include <map>
#include <set>
#include <wx/notebook.h>
#include <wx/scrolwin.h>

class SCH_EDIT_FRAME;
class SCH_SYMBOL;

class SPICE_SIMULATOR;
class SPICE_SETTINGS;
class EESCHEMA_SETTINGS;
class SPICE_CIRCUIT_MODEL;

class TUNER_SLIDER;
class STD_BITMAP_BUTTON;
class SIM_MODEL;
class SIM_TREE_MODEL;
class SIM_VIEW;


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

    const std::vector<wxString>& Netnames() const { return m_netnames; }

    std::vector<wxString> DeviceVoltages() const;

    const std::map<int, wxString>& UserDefinedSignals() { return m_userDefinedSignals; }
    void SetUserDefinedSignals( const std::map<int, wxString>& aSignals );

    /**
     * Creates a column at the end of m_signalsGrid named "Cursor n" ( n = m_customCursorsCnt ),
     * increases m_customCursorsCnt,
     * emplaces a vector to m_cursorFormatsDyn,
     * and update widgets
     */
    void CreateNewCursor();

    /**
     * Deletes last m_signalsGrid "Cursor n" column,
     * removes vector's m_cursorFormatsDyn last entry,
     * reduces m_customCursorsCnt by one, and update widgets
     */
    void DeleteCursor();

    /**
     * Add a new trace to the current plot.
     *
     * @param aName is the device/net name.
     * @param aType describes the type of trace.
     */
    void AddTrace( const wxString& aName, SIM_TRACE_TYPE aType );

    /**
     * Get/Set the number of significant digits and the range for formatting a cursor value.
     *
     * @param aCursorId is the cursor ID of the format to get.
     * @param aValueCol 0 indicates the X value column; 1 the Y value.
     */
    SPICE_VALUE_FORMAT GetCursorFormat( int aCursorId, int aValueCol ) const
    {
        return m_cursorFormatsDyn[aCursorId][aValueCol];
    }

    void SetCursorFormat( int aCursorId, int aValueCol, const SPICE_VALUE_FORMAT& aFormat )
    {
        m_cursorFormatsDyn[ aCursorId ][ aValueCol ] = aFormat;

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
     * @param aSheetPath is the instance of the symbol to update
     * @param aSymbol id of the symbol needing updating
     * @param aRef is the name of the  symbol field
     * @param aValue new value of the symbol field
     */
    void UpdateTunerValue( const SCH_SHEET_PATH& aSheetPath, const KIID& aSymbol,
                           const wxString& aRef, const wxString& aValue );

    /// Add a measurement to the measurements tree.
    void AddMeasurement( const wxString& aCmd );

    /// Add the \a aFunctions measurements ("MIN", "RMS", ...) of \a aSignal that don't exist yet.
    void AddMeasurements( const wxString& aSignal, const std::vector<wxString>& aFunctions );

    /// Delete a measurement by its index in SIM_PLOT_TAB::Measurements().
    void DeleteMeasurement( int aIndex );

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

    bool IsSimConsoleShown();
    void ToggleSimConsole();
    bool IsSimSidePanelShown();
    void ToggleSimSidePanel();

    bool DarkModePlots() const { return m_darkMode; }
    void ToggleDarkModePlots();

    /// Toggle the current S-parameter tab between Smith chart and amplitude/phase views.
    void ToggleSmithChart();

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

    /**
     * Called when settings are changed via the common Preferences dialog.
     */
    void ApplyPreferences( const SIM_PREFERENCES& aPrefs );

    /**
     * Adjust the sash dimension of splitter windows after reading
     * the config settings
     * must be called after the config settings are read, and once the
     * frame is initialized (end of the Ctor)
     */
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

    /**
     * Return true if a tab other than @p aExcept records @p aPlotName as its ngspice plot.
     *
     * Guards plot destruction so a rerun never frees a plot another tab still references (e.g. an
     * FFT that consumes a TRAN plot, or a run that produced no new plot and reported a stale one).
     */
    bool IsPlotOwnedByOtherTab( const SIM_TAB* aExcept, const wxString& aPlotName ) const
    {
        if( aPlotName.IsEmpty() )
            return false;

        for( int ii = 0; ii < (int) m_plotNotebook->GetPageCount(); ++ii )
        {
            SIM_TAB* candidate = dynamic_cast<SIM_TAB*>( m_plotNotebook->GetPage( ii ) );

            if( candidate && candidate != aExcept && candidate->GetSpicePlotName() == aPlotName )
                return true;
        }

        return false;
    }

    void OnPlotSettingsChanged();

    void OnSimUpdate();
    void OnSimRefresh( bool aFinal );
    void FlushSimConsole();

    void OnModify();

private:
    /**
     * Get the simulator output vector name for a given signal name and type.
     */
    wxString vectorNameFromSignalName( SIM_PLOT_TAB* aPlotTab, const wxString& aSignalName, int* aTraceType );

    /// The inverse of vectorNameFromSignalName().
    wxString signalNameFromVectorName( const wxString& aVectorName ) const;

    /**
     * Update a trace in a particular SIM_PLOT_TAB.  If the panel does not contain the given
     * trace, then add it.
     *
     * @param aVectorName is the SPICE vector name, such as "I(Net-C1-Pad1)".
     * @param aTraceType describes the type of plot.
     * @param aPlotTab is the tab that should receive the update.
     * @param aDataX
     * @param aClearData
     * @param aView is the view the trace should be plotted on; if null, the trace's existing
     *              view is kept (or the tab's default view is used, for a new trace).
     */
    void updateTrace( const wxString& aVectorName, int aTraceType, SIM_PLOT_TAB* aPlotTab,
                      std::vector<double>* aDataX = nullptr, bool aClearData = false, SIM_VIEW* aView = nullptr );

    /// Reference impedance of the response port for an S-parameter vector, zero when unresolved.
    double getSmithPortImpedance( const wxString& aVectorName );

    /**
     * A common toggler for the two main wxSplitterWindow s
     */
    void TogglePanel( wxPanel* aPanel, wxSplitterWindow* aSplitterWindow, int& aSashPosition );

    /**
     * Init handler for custom cursors
     *
     * Called once in class's body
     */
    void CustomCursorsInit();

    /**
     * Rebuild the list of signals available from the netlist.
     */
    void rebuildSignalsList();

    void rebuildSignalsTree( wxString aFilter );

    void updateSignalsTree();

    /// The selected signals, or only the clicked one if it isn't selected.
    std::vector<wxString> getSelectedSignals( const wxDataViewItem& aClickedItem ) const;

    /// Put the signal names on the clipboard, one per line.
    void copySignalNames( const std::vector<wxString>& aSignals );

    /// Plot \a aSignal on \a aView, or remove its trace if \a aView is null.
    void setSignalView( const wxString& aSignal, SIM_VIEW* aView );

    /// The view \a aSignal returns to when re-enabled: its last view, or the first.
    SIM_VIEW* lastSignalView( SIM_PLOT_TAB* aPlotTab, const wxString& aSignal ) const;

    /**
     * Update the cursor values (in the grid) and graphics (in the plot window).
     */
    void updatePlotCursors();

    /**
     * Add or remove the Smith-only cursor columns, carrying their shown/hidden state across.
     */
    void updateSmithCursorColumns( bool aSmithMode );

    void fillSmithCursorRow( int aRow, CURSOR* aCursor, TRACE* aTrace );

    void setSmithCursorColumnLabels();

    void rememberSmithCursorColumns();

    void applySmithCursorColumns();

    /**
     * Apply user-defined signals to the SPICE session.
     */
    void applyUserDefinedSignals();

    /**
     * Rebuild the measurements grid for the current plot.
     */
    void rebuildMeasurementsTree( const std::map<wxString, wxString>* aKeepValues = nullptr );

    /// Shown values by command.  Take them before changing Measurements(): the tree indexes into it.
    std::map<wxString, wxString> shownMeasurementValues() const;

    void updateMeasurements();

    /// Refresh each measurement group's trace dot without notifying; true if any changed.
    bool refreshMeasurementGroupIcons();

    /// Cached plot indicator for \a aColor, filled when plotted.
    wxBitmapBundle signalDot( const wxColour& aColor, bool aFilled );

    void updateMeasurementValues();

    std::vector<int> getSelectedMeasurements( const wxDataViewItem& aClickedItem ) const;

    /// Map a tree signal to the name a MEAS command wants; false for phase traces.
    bool measurementSignal( const wxString& aSignal, wxString& aResult ) const;

    /// Push a measurement value into the tree.
    void setMeasurementValue( int aIndex, const wxString& aValue );

    /// Uniform row height; re-apply once realized, or wxGTK keeps an early measurement.
    void applyTreeRowHeight();

    /**
     * Grow (but never shrink below \a aMinWidth) a grid column to fit its current contents.
     *
     * @param aGrid
     * @param aCol
     * @param aMinWidth
     * @param aExtraPadding additional width to reserve beyond the content, e.g. for a combo
     *                      box column's dropdown arrow so it doesn't crowd the text.
     */
    void autoSizeGridColumn( WX_GRID* aGrid, int aCol, int aMinWidth, int aExtraPadding = 0 );

    /// The Signals grid's "Plot" column choices: "Disabled" plus one entry per current view.
    wxArrayString getViewChoices( SIM_PLOT_TAB* aPlotTab ) const;

    /// The Signals grid's "Plot" column label for a given view ("Disabled" if null).
    wxString getViewLabel( SIM_PLOT_TAB* aPlotTab, SIM_VIEW* aView ) const;

    /// The views a trace's Y-axis scale could be linked to: every view except its own (since
    /// linking to its own view is equivalent to -- and represented by -- "Default").
    std::vector<SIM_VIEW*> getYScaleTargetViews( SIM_PLOT_TAB* aPlotTab, SIM_VIEW* aOwnView ) const;

    /// The Signals grid's "Y Scale" column choices: "Default" plus one entry per eligible view.
    wxArrayString getYScaleChoices( SIM_PLOT_TAB* aPlotTab, SIM_VIEW* aOwnView ) const;

    /// The Signals grid's "Y Scale" column label for a trace ("Default" if not explicitly linked).
    wxString getYScaleLabel( SIM_PLOT_TAB* aPlotTab, TRACE* aTrace ) const;

    /**
     * Apply component values specified using tuner sliders to the current netlist.
     */
    void applyTuners();

    /**
     * Return X axis for a given simulation type.
     */
    SIM_TRACE_TYPE getXAxisType( SIM_TYPE aType ) const;

    struct MULTI_RUN_STEP;

    void clearMultiRunState( bool aClearTraces );
    void prepareMultiRunState();
    std::vector<MULTI_RUN_STEP> calculateMultiRunSteps( const std::vector<TUNER_SLIDER*>& aTuners ) const;
    std::string multiRunTraceKey( const wxString& aVectorName, int aTraceType ) const;
    void recordMultiRunData( const wxString& aVectorName, int aTraceType,
                             const std::vector<double>& aX, const std::vector<double>& aY );
    bool hasMultiRunTrace( const wxString& aVectorName, int aTraceType ) const;

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

    void onSignalsTreeActivated( wxDataViewEvent& aEvent ) override;
    void onSignalsTreeContextMenu( wxDataViewEvent& aEvent ) override;
    void onSignalsTreeMenuSelection( wxCommandEvent& aEvent );
    void OnConsoleFilterChanged( wxCommandEvent& aEvent ) override;
    void onCursorsGridCellChanged( wxGridEvent& aEvent ) override;
    void onMeasurementsTreeContextMenu( wxDataViewEvent& aEvent ) override;
    void onMeasurementsTreeMenuSelection( wxCommandEvent& aEvent );

    enum class SIM_CONSOLE_FILTER
    {
        ALL,
        WARNINGS,
        ERRORS
    };

    bool consoleEntryMatchesFilter( SEVERITY aSeverity ) const;

    /// Record a console line and append it to m_simConsole if it passes the filter.
    void appendConsoleText( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED );

    void OnUpdateUI( wxUpdateUIEvent& event ) override;

    bool loadLegacyWorkbook( const wxString & aPath );
    bool loadJsonWorkbook( const wxString & aPath );

    void SaveCursorToWorkbook( nlohmann::json& aTraceJs, TRACE* aTrace, int aCursorId );

    void onPlotCursorUpdate( wxCommandEvent& aEvent );

public:
    int                          m_SuppressGridEvents;

private:
    SIMULATOR_FRAME* m_simulatorFrame;
    SCH_EDIT_FRAME*  m_schematicFrame;

    std::vector<SIM_CONSOLE_ENTRY> m_consoleEntries;
    SIM_CONSOLE_FILTER             m_consoleFilter = SIM_CONSOLE_FILTER::ALL;

    STD_BITMAP_BUTTON* m_addViewButton = nullptr;
    STD_BITMAP_BUTTON* m_removeViewButton = nullptr;

    SIM_TREE_MODEL* m_signalsModel = nullptr;
    SIM_TREE_MODEL* m_measurementsModel = nullptr;

    std::vector<int> m_measurementsMenuRows;

    std::vector<wxString> m_signalsMenuSignals;

    /// Last view per signal, per tab; keyed by signalViewKey().
    std::map<const SIM_TAB*, std::map<wxString, int>> m_lastSignalView;

    /// User collapse choices, keyed by signalTreeGroupKey().
    std::map<wxString, bool> m_signalGroupExpanded;

    /// True while the signals tree shows a filtered (force-expanded) set.
    bool m_signalsTreeFiltered = false;

    struct SIGNAL_DOT
    {
        wxColour       colour;
        bool           filled;
        wxBitmapBundle bitmap;
    };

    static constexpr size_t SIGNAL_DOT_CACHE_LIMIT = 256;

    std::vector<SIGNAL_DOT> m_signalDotCache;

    std::vector<wxString>                 m_signals;
    std::map<int, wxString>               m_userDefinedSignals;
    std::list<TUNER_SLIDER*>              m_tuners;
    std::map<const TUNER_SLIDER*, double> m_tunerOverrides;

    /// One notebook page per device class, created on demand and dropped when empty.
    enum TUNER_GROUP
    {
        TUNERGRP_RESISTORS = 0,
        TUNERGRP_CAPACITORS,
        TUNERGRP_INDUCTORS,
        TUNERGRP_SOURCES,
        TUNERGRP_OTHER,

        TUNERGRP_COUNT
    };

    static TUNER_GROUP tunerGroup( const SIM_MODEL& aModel );
    static wxString    tunerGroupLabel( TUNER_GROUP aGroup );

    /// The page for \a aGroup, created in tab order if missing.
    wxScrolledWindow* getOrCreateTunerPage( TUNER_GROUP aGroup );

    void pruneEmptyTunerPages();

    wxNotebook*       m_tunerNotebook;
    wxScrolledWindow* m_tunerPages[TUNERGRP_COUNT];

    struct MULTI_RUN_TRACE
    {
        int traceType = SPT_UNKNOWN;
        std::vector<double> xValues;
        std::vector<std::vector<double>> yValues;

        // smith traces carry Re(gamma) in x, different every run, stored per run here
        std::vector<std::vector<double>> xRuns;
    };

    struct MULTI_RUN_STEP
    {
        std::map<const TUNER_SLIDER*, double> overrides;
    };

    struct MULTI_RUN_STATE
    {
        bool active = false;
        std::vector<TUNER_SLIDER*> tuners;
        std::vector<MULTI_RUN_STEP> steps;
        size_t currentStep = 0;
        size_t storedSteps = 0;
        bool storePending = false;
        std::map<std::string, MULTI_RUN_TRACE> traces;
    };

    MULTI_RUN_STATE m_multiRunState;

    void rebuildDeviceVoltages();

    /// Evaluate component voltage \a aId into the current plot, once.  On demand: there can be hundreds.
    void ensureDeviceVoltage( int aId );

    /// As above, by vector name.  Does nothing for other vectors.
    void ensureDeviceVoltage( const wxString& aVectorName );

    /// Replace V(R1)-style component voltages with their node-voltage difference.
    wxString expandDeviceVoltages( const wxString& aExpression ) const;

    /// A signal or expression as ngspice can evaluate it: user-defined signals and component voltages map
    /// to their vectors, also when parenthesised in an expression; other component voltages are expanded.
    wxString spiceSignal( const wxString& aSignal );

    /// Quote the schematic net names in a SPICE expression ('-' and '/' are legal in net names).
    wxString quoteNetNames( const wxString& aExpression ) const;

    /// First component-voltage id, clear of the user-defined ids.
    static constexpr int DEVICE_VOLTAGE_ID_BASE = 1000000;

    std::map<wxString, int> m_deviceVoltageIds;

    /// Every id handed out this session; ids are never reused.
    std::map<wxString, int> m_deviceVoltageIdRegistry;
    int                     m_nextDeviceVoltageId = DEVICE_VOLTAGE_ID_BASE;

    std::map<int, wxString> m_deviceVoltageExprs;

    std::map<wxString, int> m_deviceVoltageVectors;

    /// Component voltages already evaluated, per ngspice plot (a "let" only lands in the current plot).
    std::map<wxString, std::set<int>> m_appliedDeviceVoltages;

    /// SPICE expressions need quoted versions of the netnames since KiCad allows '-' and '/'
    /// in netnames.
    std::vector<wxString>        m_netnames;

    SPICE_VALUE_FORMAT           m_cursorFormats[3][2];

    // Holds cursor formating for m_cursorsGrid, includes m_cursorFormats[3][2], TODO: merge.
    std::vector<std::vector<SPICE_VALUE_FORMAT>> m_cursorFormatsDyn;

    wxString m_smithCursorColumns;
    std::vector<int> m_smithCursorWidths;

    // Variables for temporary storage:
    int m_splitterLeftRightSashPosition;
    int m_splitterPlotAndConsoleSashPosition;
    int m_splitterSignalsSashPosition;
    int m_splitterCursorsSashPosition;
    int m_splitterCenterRightSashPosition;

    /// For each splitter with a hidden panel: whether that panel was its first (left or top) window.
    std::map<wxSplitterWindow*, bool> m_hiddenPanelFirst;
    bool                              m_darkMode;
    unsigned int                      m_plotNumber;
    wxTimer                           m_refreshTimer;
    SIM_PREFERENCES                   m_preferences;

    // Count all available cursors in m_signalsGrid
    int                          m_customCursorsCnt; // Defaults to 2 + 1
};
