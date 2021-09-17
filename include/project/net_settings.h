/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_NET_SETTINGS_H
#define KICAD_NET_SETTINGS_H

#include <netclass.h>
#include <settings/nested_settings.h>

/**
 * NET_SETTINGS stores various net-related settings in a project context.  These settings are
 * accessible and editable from both the schematic and PCB editors.
 */
class NET_SETTINGS : public NESTED_SETTINGS
{
public:
    NET_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );

    virtual ~NET_SETTINGS();

public:
    NETCLASSES m_NetClasses;

    // Runtime map of label to netclass-name for quick lookup.  Includes both composite labels
    // (buses) and atomic net names (including individual bus members).
    std::map<wxString, wxString> m_NetClassAssignments;

    /**
     * A map of fully-qualified net names to colors used in the board context.
     * Since these color overrides are for the board, buses are not included here.
     * Only nets that the user has assigned custom colors to will be in this list.
     * Nets that no longer exist will be deleted during a netlist read in Pcbnew.
     */
    std::map<wxString, KIGFX::COLOR4D> m_PcbNetColors;

public:
    const wxString& GetNetclassName( const wxString& aNetName ) const;

    /**
     * Parse a bus vector (e.g. A[7..0]) into name, begin, and end.
     *
     * Ensure that begin and end are positive and that end > begin.
     *
     * @param aBus is a bus vector label string
     * @param aName out is the bus name, e.g. "A"
     * @param aMemberList is a list of member strings, e.g. "A7", "A6", and so on
     * @return true if aBus was successfully parsed
     */
    static bool ParseBusVector( const wxString& aBus, wxString* aName,
                                std::vector<wxString>* aMemberList );

    /**
     * Parse a bus group label into the name and a list of components.
     *
     * @param aGroup is the input label, e.g. "USB{DP DM}"
     * @param name is the output group name, e.g. "USB"
     * @param aMemberList is a list of member strings, e.g. "DP", "DM"
     * @return true if aGroup was successfully parsed
     */
    static bool ParseBusGroup( const wxString& aGroup, wxString* name,
                               std::vector<wxString>* aMemberList );

    /**
     * Rebuild netclass assignments from the netclass membership lists.
     */
    void RebuildNetClassAssignments();

private:
    bool migrateSchema0to1();

    // TODO: Add diff pairs, bus information, etc.
};

#endif // KICAD_NET_SETTINGS_H
