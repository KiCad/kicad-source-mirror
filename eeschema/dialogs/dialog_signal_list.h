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

#ifndef DIALOG_SIGNAL_LIST_H
#define DIALOG_SIGNAL_LIST_H

#include "dialog_signal_list_base.h"

class SIM_PLOT_FRAME;
class NETLIST_EXPORTER_PSPICE_SIM;

class DIALOG_SIGNAL_LIST : public DIALOG_SIGNAL_LIST_BASE
{
public:
    DIALOG_SIGNAL_LIST( SIM_PLOT_FRAME* aParent, NETLIST_EXPORTER_PSPICE_SIM* aExporter );

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

private:
    void onSignalAdd( wxCommandEvent& event ) override
    {
        addSelectionToPlotFrame();
    }

    void addSelectionToPlotFrame();

    SIM_PLOT_FRAME* m_plotFrame;
    NETLIST_EXPORTER_PSPICE_SIM* m_exporter;
};

#endif /* DIALOG_SIGNAL_LIST_H */
