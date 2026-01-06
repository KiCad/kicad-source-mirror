/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef JOB_EXPORT_SCH_NETLIST_H
#define JOB_EXPORT_SCH_NETLIST_H

#include <kicommon.h>
#include <vector>
#include "job.h"

class KICOMMON_API JOB_EXPORT_SCH_NETLIST : public JOB
{
public:
    enum class FORMAT
    {
        KICADXML,
        KICADSEXPR,
        ORCADPCB2,
        CADSTAR,
        SPICE,
        SPICEMODEL,
        PADS,
        ALLEGRO
    };

    static std::map<JOB_EXPORT_SCH_NETLIST::FORMAT, wxString>& GetFormatNameMap();

public:
    JOB_EXPORT_SCH_NETLIST();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    wxString m_filename;

    FORMAT format;

    bool m_spiceSaveAllVoltages;
    bool m_spiceSaveAllCurrents;
    bool m_spiceSaveAllDissipations;
    bool m_spiceSaveAllEvents;

    // Variant names to export. Empty vector means default variant only.
    std::vector<wxString> m_variantNames;
};

#endif