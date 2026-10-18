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

#ifndef JOB_SCH_LINK_FOOTPRINTS_H
#define JOB_SCH_LINK_FOOTPRINTS_H

#include <map>

#include <kicommon.h>
#include "job.h"

/**
 * Job to point imported symbols at the footprints of an imported board.
 */
class KICOMMON_API JOB_SCH_LINK_FOOTPRINTS : public JOB
{
public:
    JOB_SCH_LINK_FOOTPRINTS();

    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    /// Root schematic to update in place.
    wxString m_filename;

    /// Board reference to the LIB_ID of its footprint.
    std::map<wxString, wxString> m_footprintsByRef;
};

#endif
