/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus, see AUTHORS.TXT for contributors.
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

#ifndef NETLIST_EXPORTER_SPICE_MODEL_H
#define NETLIST_EXPORTER_SPICE_MODEL_H

#include "netlist_exporter_spice.h"


class NETLIST_EXPORTER_SPICE_MODEL : public NETLIST_EXPORTER_SPICE
{
public:
    NETLIST_EXPORTER_SPICE_MODEL( SCHEMATIC_IFACE* aSchematic )
        : NETLIST_EXPORTER_SPICE( aSchematic )
    {
    }

    void WriteHead( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions ) override;
    void WriteTail( OUTPUTFORMATTER& aFormatter, unsigned aNetlistOptions ) override;
    bool ReadSchematicAndLibraries( unsigned aNetlistOptions, REPORTER& aReporter ) override;

protected:
    std::string GenerateItemPinNetName( const std::string& aNetName, int& aNcCounter ) const override;

private:
    struct PORT_INFO
    {
        std::string name;
        LABEL_FLAG_SHAPE dir;
    };

    void readPorts( unsigned aNetlistOptions );

    std::map<std::string, PORT_INFO> m_ports;
};

#endif // NETLIST_EXPORTER_SPICE_MODEL_H
