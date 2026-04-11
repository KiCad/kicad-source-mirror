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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef NETLIST_EXPORTER_PADS_H
#define NETLIST_EXPORTER_PADS_H

#include "netlist_exporter_base.h"


/**
 * Generate a netlist compatible with PADS.
 */
class NETLIST_EXPORTER_PADS : public NETLIST_EXPORTER_BASE
{
public:
    NETLIST_EXPORTER_PADS( SCHEMATIC* aSchematic ) :
            NETLIST_EXPORTER_BASE( aSchematic )
    {
    }

    /**
     * Write to specified output file.
     */
    bool WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions,
                       REPORTER& aReporter ) override;

private:
    /**
     * Write a net list (ranked by Netcode), and pins connected to it.
     *
     * Format:
     *   - ADD_TER RR2 6 \"$42\"
     *   - B U1 100
     *   - 6 CA
     */
    bool writeListOfNets( FILE* f );
};

#endif
