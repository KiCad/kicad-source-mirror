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

#include <sim/sim_workbook.h>

TRACE_DESC::TRACE_DESC( const NETLIST_EXPORTER_PSPICE_SIM& aExporter, const wxString& aName,
                        SIM_PLOT_TYPE aType, const wxString& aParam ) :
    m_name( aName ),
    m_type( aType ),
    m_param( aParam )
{
    // Title generation
    m_title = wxString::Format( "%s(%s)", aParam, aName );

    if( aType & SPT_AC_MAG )
        m_title += " (mag)";
    else if( aType & SPT_AC_PHASE )
        m_title += " (phase)";
}


SIM_WORKBOOK::SIM_WORKBOOK() :
    m_dirty( false )
{
}


void SIM_WORKBOOK::Clear()
{
    m_plots.clear();
}


void SIM_WORKBOOK::AddPlotPanel( SIM_PANEL_BASE* aPlotPanel )
{
    wxASSERT( m_plots.count( aPlotPanel ) == 0 );
    m_plots[aPlotPanel] = PLOT_INFO();

    m_dirty = true;
}


void SIM_WORKBOOK::RemovePlotPanel( SIM_PANEL_BASE* aPlotPanel )
{
    wxASSERT( m_plots.count( aPlotPanel ) == 1 );
    m_plots.erase( aPlotPanel );

    m_dirty = true;
}


void SIM_WORKBOOK::AddTrace( const SIM_PANEL_BASE* aPlotPanel, const wxString& aName,
        const TRACE_DESC& aTrace )
{
    // XXX: A plot is created automatically if there is none with this name yet.
    m_plots[aPlotPanel].m_traces.insert(
            std::make_pair( aName, aTrace ) );

    m_dirty = true;
}


void SIM_WORKBOOK::RemoveTrace( const SIM_PANEL_BASE* aPlotPanel, const wxString& aName )
{
    auto& traceMap = m_plots[aPlotPanel].m_traces;
    auto traceIt = traceMap.find( aName );
    wxASSERT( traceIt != traceMap.end() );
    traceMap.erase( traceIt );

    m_dirty = true;
}


SIM_WORKBOOK::TRACE_MAP::const_iterator SIM_WORKBOOK::RemoveTrace( const SIM_PANEL_BASE* aPlotPanel,
        TRACE_MAP::const_iterator aIt )
{
    m_dirty = true;
    return m_plots.at( aPlotPanel ).m_traces.erase( aIt );
}
