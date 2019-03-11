/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ERC_SETTINGS_H
#define _ERC_SETTINGS_H

/**
 * Container for ERC settings
 *
 * Currently only stores flags about checks to run, but could later be expanded
 * to contain the matrix of electrical pin types.
 */
class ERC_SETTINGS
{
public:
    void LoadDefaults()
    {
        write_erc_file = false;
        check_similar_labels = true;
        check_unique_global_labels = true;
        check_bus_driver_conflicts = true;
        check_bus_entry_conflicts = true;
        check_bus_to_bus_conflicts = true;
        check_bus_to_net_conflicts = true;
    }

    bool operator==( const ERC_SETTINGS& other ) const
    {
        return ( other.write_erc_file == write_erc_file &&
                 other.check_similar_labels == check_similar_labels &&
                 other.check_unique_global_labels == check_unique_global_labels &&
                 other.check_bus_driver_conflicts == check_bus_driver_conflicts &&
                 other.check_bus_entry_conflicts == check_bus_entry_conflicts &&
                 other.check_bus_to_bus_conflicts == check_bus_to_bus_conflicts &&
                 other.check_bus_to_net_conflicts == check_bus_to_net_conflicts );
    }

    bool operator!=( const ERC_SETTINGS& other ) const
    {
        return !( other == *this );
    }

    /// If true, write ERC results to a file
    bool write_erc_file;

    /// If true, check each sheet for labels that differ only by letter case
    bool check_similar_labels;

    /// If true, check to ensure that each global label apperas more than once
    bool check_unique_global_labels;

    /// If true, check that buses don't have conflicting drivers
    bool check_bus_driver_conflicts;

    /// If true, check that wires connecting to buses actually exist in the bus
    bool check_bus_entry_conflicts;

    /// If true, check that bus-to-bus connections share at least one member
    bool check_bus_to_bus_conflicts;

    /// If true, check that bus wires don't graphically connect to net objects (or vice versa)
    bool check_bus_to_net_conflicts;
};

#endif
