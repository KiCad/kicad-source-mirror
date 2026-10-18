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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>
#include <ostream>
#include <string>
#include <vector>


// Structure for holding the ODB net point record.
struct ODB_NET_RECORD
{
    /**
    * Possible values for the "side" of an ODB net point.
    *
    * This is not the same as the subnet side (T/B)
    * or matrix layer TOP/BOTTOM/INNER.
    */
    enum class SIDE
    {
        TOP,    ///< T
        DOWN,   ///< D (i.e. bottom)
        BOTH,   ///< B
        INNER,  ///< I
    };

    bool        smd;
    bool        hole;
    bool        is_via;
    bool        testpoint;
    std::string refdes;
    int         drill_radius;
    bool        mechanical;
    SIDE        side;

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
class PCB_IO_ODBPP;
struct ODB_FORMAT;
class ODB_NET_LIST
{
public:
    ODB_NET_LIST( BOARD* aBoard, PCB_IO_ODBPP* aPlugin ) : m_board( aBoard ), m_plugin( aPlugin ) {}

    virtual ~ODB_NET_LIST() {}

    void Write( std::ostream& aStream, const ODB_FORMAT& aFormat );

private:
    BOARD*        m_board;
    PCB_IO_ODBPP* m_plugin;

    void InitNetPoints( std::map<size_t, std::vector<ODB_NET_RECORD>>& aRecords );
    /// Writes a list of records to the given output stream
    void WriteNetPointRecords( std::map<size_t, std::vector<ODB_NET_RECORD>>& aRecords,
                               std::ostream&                                  aStream,
                               const ODB_FORMAT&                              aFormat );
};
