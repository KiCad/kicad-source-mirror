/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#ifndef SIMULATOR_CONTROL_H
#define SIMULATOR_CONTROL_H

#include <tool/tool_interactive.h>

class SIMULATOR_FRAME;
class SCH_EDIT_FRAME;
class SPICE_CIRCUIT_MODEL;
class SPICE_SIMULATOR;
class SIM_TAB;


/**
 * Handle actions for the various symbol editor and viewers.
 */
class SIMULATOR_CONTROL : public wxEvtHandler, public TOOL_INTERACTIVE
{
public:
    SIMULATOR_CONTROL() :
            TOOL_INTERACTIVE( "eeschema.SimulatorControl" ),
            m_simulatorFrame( nullptr ),
            m_schematicFrame( nullptr )
    { }

    virtual ~SIMULATOR_CONTROL() { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int NewAnalysisTab( const TOOL_EVENT& aEvent );
    int OpenWorkbook( const TOOL_EVENT& aEvent );
    int SaveWorkbook( const TOOL_EVENT& aEvent );
    int ExportPlotAsPNG( const TOOL_EVENT& aEvent );
    int ExportPlotAsCSV( const TOOL_EVENT& aEvent );
    int ExportPlotToClipboard( const TOOL_EVENT& aEvent );
    int ExportPlotToSchematic( const TOOL_EVENT& aEvent );
    int Close( const TOOL_EVENT& aEvent );

    int ToggleSimConsolePanel( const TOOL_EVENT& aEvent );
    int ToggleSimSidePanel( const TOOL_EVENT& aEvent );
    int Zoom( const TOOL_EVENT& aEvent );
    int UndoZoom( const TOOL_EVENT& aEvent );
    int RedoZoom( const TOOL_EVENT& aEvent );
    int ToggleGrid( const TOOL_EVENT& aEvent );
    int ToggleLegend( const TOOL_EVENT& aEvent );
    int ToggleDottedSecondary( const TOOL_EVENT& aEvent );
    int ToggleDarkModePlots( const TOOL_EVENT& aEvent );

    int EditAnalysisTab( const TOOL_EVENT& aEvent );
    int RunSimulation( const TOOL_EVENT& aEvent );
    int Probe( const TOOL_EVENT& aEvent );
    int Tune( const TOOL_EVENT& aEvent );

    int EditUserDefinedSignals( const TOOL_EVENT& aEvent );
    int ShowNetlist( const TOOL_EVENT& aEvent );

private:
    /**
     * Return the default filename (with extension) to be used in file browser dialog.
     */
    wxString getDefaultFilename();

    /**
     * Return the default path to be used in file browser dialog.
     */
    wxString getDefaultPath();

    SIM_TAB* getCurrentSimTab();

    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    SIMULATOR_FRAME*                     m_simulatorFrame;
    SCH_EDIT_FRAME*                      m_schematicFrame;
    std::shared_ptr<SPICE_CIRCUIT_MODEL> m_circuitModel;
    std::shared_ptr<SPICE_SIMULATOR>     m_simulator;
};


#endif // SIMULATOR_CONTROL_H
