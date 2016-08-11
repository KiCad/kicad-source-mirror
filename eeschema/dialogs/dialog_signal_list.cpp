/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
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

#include "dialog_signal_list.h"
#include <sim/sim_plot_frame.h>

#include <sim/netlist_exporter_pspice_sim.h>

DIALOG_SIGNAL_LIST::DIALOG_SIGNAL_LIST( SIM_PLOT_FRAME* aParent, NETLIST_EXPORTER_PSPICE_SIM* aExporter )
    : DIALOG_SIGNAL_LIST_BASE( aParent ), m_plotFrame( aParent ), m_exporter( aExporter )
{
}


bool DIALOG_SIGNAL_LIST::TransferDataFromWindow()
{
    if( !DIALOG_SIGNAL_LIST_BASE::TransferDataFromWindow() )
        return false;

    addSelectionToPlotFrame();

    return true;
}


bool DIALOG_SIGNAL_LIST::TransferDataToWindow()
{
    // Create a list of possible signals
    // TODO switch( m_exporter->GetSimType() )
    // {

    if( m_exporter )
    {
        for( const auto& net : m_exporter->GetNetIndexMap() )
        {
            if( net.first != "GND" )
                m_signals->Append( net.first );
        }
    }

    return DIALOG_SIGNAL_LIST_BASE::TransferDataToWindow();
}


void DIALOG_SIGNAL_LIST::addSelectionToPlotFrame()
{
    for( unsigned int i = 0; i < m_signals->GetCount(); ++i )
    {
        if( m_signals->IsSelected( i ) )
            m_plotFrame->AddVoltagePlot( m_signals->GetString( i ) );
    }
}
