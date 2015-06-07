/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers
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

#ifndef NETLIST_EXPORTER_CADSTAR_H
#define NETLIST_EXPORTER_CADSTAR_H

#include "netlist_exporter.h"

/**
 * Class NETLIST_EXPORTER_CADSTAR
 * generates a netlist compatible with CADSTAR
 */
class NETLIST_EXPORTER_CADSTAR : public NETLIST_EXPORTER
{
    /**
     * Function writeListOfNetsCADSTAR
     * writes a net list (ranked by Netcode), and pins connected to it.
     * <p>
     * Format:
     *   - ADD_TER RR2 6 \"$42\"
     *   - B U1 100
     *   - 6 CA
     * </p>
     */
    bool writeListOfNets( FILE* f );

public:
    NETLIST_EXPORTER_CADSTAR( NETLIST_OBJECT_LIST* aMasterList, PART_LIBS* aLibs ) :
        NETLIST_EXPORTER( aMasterList, aLibs )
    {
    }

    /**
     * Function WriteList
     * writes to specified output file
     */
    bool WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions );
};

#endif
