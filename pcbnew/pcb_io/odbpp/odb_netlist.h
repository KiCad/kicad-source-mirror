/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: SYSUEric <jzzhuang666@gmail.com>.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ODB_NETLIST_H_
#define _ODB_NETLIST_H_

#include <wx/string.h>

// Structure for holding the ODB net point record.
struct ODB_NET_RECORD
{
    bool        smd;
    bool        hole;
    bool        is_via;
    wxString    netname;
    std::string refdes;
    int         drill_radius;
    bool        mechanical;
    std::string side; // B: Both, T: Top, D: Down

    // All these in PCB units, will be output in decimils
    int x_location;
    int y_location;
    // Width and height of non-drilled pads (only when radius = 0).
    int x_size; // Width
    int y_size; // Height
    // int             rotation;

    std::string epoint; // e: net end point,  m: net mid point

    int soldermask; // !< e — Solder mask exposed point. soldermask = 0
                    // !< c — Solder mask covered point. = 3
                    // !< p — Solder mask covered point on top side of product model. = 1
                    // !< s — Solder mask covered point on bottom side of product model. = 2
};


class BOARD;
class ODB_NET_LIST
{
public:
    ODB_NET_LIST( BOARD* aBoard ) : m_board( aBoard ) {}

    virtual ~ODB_NET_LIST() {}

    void Write( std::ostream& aStream );

private:
    BOARD*      m_board;
    std::string ComputePadAccessSide( BOARD* aBoard, LSET aLayerMask );
    std::string ComputeViaAccessSide( BOARD* aBoard, int top_layer, int bottom_layer );

    void InitPadNetPoints( BOARD* aBoard, std::map<size_t, std::vector<ODB_NET_RECORD>>& aRecords );
    void InitViaNetPoints( BOARD* aBoard, std::map<size_t, std::vector<ODB_NET_RECORD>>& aRecords );
    /// Writes a list of records to the given output stream
    void WriteNetPointRecords( std::map<size_t, std::vector<ODB_NET_RECORD>>& aRecords,
                               std::ostream&                                  aStream );
};

#endif // _ODB_NETLIST_H_