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


class SIM_WORKBOOK
{
public:

    struct PLOT_INFO
    {
        ///< The current position of the plot in the notebook
        unsigned int pos;
    };

    SIM_WORKBOOK();

    void Clear();

    void AddPlotPanel( SIM_PANEL_BASE* aPlotPanel );
    void RemovePlotPanel( SIM_PANEL_BASE* aPlotPanel );

    std::vector<const SIM_PANEL_BASE*> GetSortedPlotPanels() const;

    bool HasPlotPanel( SIM_PANEL_BASE* aPlotPanel ) const
    {
        return m_plots.count( aPlotPanel ) == 1;
    }

    void AddTrace( const SIM_PANEL_BASE* aPlotPanel, const wxString& aName );
    void RemoveTrace( const SIM_PANEL_BASE* aPlotPanel, const wxString& aName );

    void SetPlotPanelPosition( const SIM_PANEL_BASE* aPlotPanel, unsigned int pos )
    {
        if( pos != m_plots.at( aPlotPanel ).pos )
            m_flagModified = true;

        m_plots.at( aPlotPanel ).pos = pos;
    }

    void ClrModified() { m_flagModified = false; }
    bool IsModified() const { return m_flagModified; }

private:
    ///< Dirty bit, indicates something in the workbook has changed
    bool m_flagModified;

    ///< Map of plot panels and associated data
    std::map<const SIM_PANEL_BASE*, PLOT_INFO> m_plots;
};
