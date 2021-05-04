/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Mikołaj Wielgus <wielgusmikolaj@gmail.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_sim_settings.h"
#include <sim/sim_panel_base.h>
#include <sim/sim_plot_panel.h>


///< Trace descriptor class
class TRACE_DESC
{
public:
    TRACE_DESC( const NETLIST_EXPORTER_PSPICE_SIM& aExporter, const wxString& aName,
            SIM_PLOT_TYPE aType, const wxString& aParam );

    ///< Modifies an existing TRACE_DESC simulation type
    TRACE_DESC( const NETLIST_EXPORTER_PSPICE_SIM& aExporter,
            const TRACE_DESC& aDescription, SIM_PLOT_TYPE aNewType )
        : TRACE_DESC( aExporter, aDescription.GetName(), aNewType, aDescription.GetParam() )
    {
    }

    const wxString& GetTitle() const
    {
        return m_title;
    }

    const wxString& GetName() const
    {
        return m_name;
    }

    const wxString& GetParam() const
    {
        return m_param;
    }

    SIM_PLOT_TYPE GetType() const
    {
        return m_type;
    }

private:
    // Three basic parameters
    ///< Name of the measured net/device
    wxString m_name;

    ///< Type of the signal
    SIM_PLOT_TYPE m_type;

    ///< Name of the signal parameter
    wxString m_param;

    // Generated data
    ///< Title displayed in the signal list/plot legend
    wxString m_title;
};


class SIM_WORKBOOK
{
public:
    typedef std::map<wxString, TRACE_DESC> TRACE_MAP;

    struct PLOT_INFO
    {
        ///< Map of the traces displayed on the plot
        TRACE_MAP m_traces;

        ///< Spice directive used to execute the simulation
        wxString m_simCommand;
    };

    typedef std::map<const SIM_PANEL_BASE*, PLOT_INFO> PLOT_MAP;

    SIM_WORKBOOK();

    void Clear();

    void AddPlotPanel( SIM_PANEL_BASE* aPlotPanel );
    void RemovePlotPanel( SIM_PANEL_BASE* aPlotPanel );


    bool HasPlotPanel( SIM_PANEL_BASE* aPlotPanel ) const
    {
        return m_plots.count( aPlotPanel ) == 1;
    }

    void AddTrace( const SIM_PANEL_BASE* aPlotPanel, const wxString& aName,
            const TRACE_DESC& aTrace );
    void RemoveTrace( const SIM_PANEL_BASE* aPlotPanel, const wxString& aName );
    TRACE_MAP::const_iterator RemoveTrace( const SIM_PANEL_BASE* aPlotPanel, TRACE_MAP::const_iterator aIt );

    TRACE_MAP::const_iterator TracesBegin( const SIM_PANEL_BASE* aPlotPanel ) const
    {
        return m_plots.at( aPlotPanel ).m_traces.cbegin();
    }

    TRACE_MAP::const_iterator TracesEnd( const SIM_PANEL_BASE* aPlotPanel ) const
    {
        return m_plots.at( aPlotPanel ).m_traces.cend();
    }

    void SetSimCommand( const SIM_PANEL_BASE* aPlotPanel, const wxString& aSimCommand )
    {
        m_plots.at( aPlotPanel ).m_simCommand = aSimCommand;
        m_dirty = true;
    }

    const wxString& GetSimCommand( const SIM_PANEL_BASE* aPlotPanel ) const
    {
        return m_plots.at( aPlotPanel ).m_simCommand;
    }

    const PLOT_MAP GetPlots() const { return m_plots; }

    const TRACE_MAP GetTraces( const SIM_PANEL_BASE* aPlotPanel ) const
    {
        return m_plots.at( aPlotPanel ).m_traces;
    }

    bool IsDirty() const { return m_dirty; }

private:
    ///< Dirty bit, indicates something in the workbook has changed
    bool m_dirty;

    ///< Map of plot panels and associated data
    std::map<const SIM_PANEL_BASE*, PLOT_INFO> m_plots;
};
