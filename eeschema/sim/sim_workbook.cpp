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


SIM_WORKBOOK::SIM_WORKBOOK() :
    m_flagModified( false )
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

    m_flagModified = true;
}


void SIM_WORKBOOK::RemovePlotPanel( SIM_PANEL_BASE* aPlotPanel )
{
    wxASSERT( m_plots.count( aPlotPanel ) == 1 );
    m_plots.erase( aPlotPanel );

    m_flagModified = true;
}


std::vector<const SIM_PANEL_BASE*> SIM_WORKBOOK::GetSortedPlotPanels() const
{
    std::vector<const SIM_PANEL_BASE*> plotPanels;

    for( const auto& plot : m_plots )
        plotPanels.push_back( plot.first );

    std::sort( plotPanels.begin(), plotPanels.end(),
    [&]( const SIM_PANEL_BASE*& a, const SIM_PANEL_BASE*& b )
    {
        return m_plots.at( a ).pos < m_plots.at( b ).pos;
    });

    return plotPanels;
}


void SIM_WORKBOOK::AddTrace( const SIM_PANEL_BASE* aPlotPanel, const wxString& aName )
{
    m_flagModified = true;
}


void SIM_WORKBOOK::RemoveTrace( const SIM_PANEL_BASE* aPlotPanel, const wxString& aName )
{
    m_flagModified = true;
}

