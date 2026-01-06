/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mike Williams <mike@mikebwilliams.com>
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

#ifndef JOB_EXPORT_SCH_BOM_H
#define JOB_EXPORT_SCH_BOM_H

#include <kicommon.h>
#include <vector>
#include "job.h"

class KICOMMON_API JOB_EXPORT_SCH_BOM : public JOB
{
public:
    JOB_EXPORT_SCH_BOM();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    // Basic options
    wxString m_filename;

    // Preset options (from schematic)
    wxString m_bomPresetName;
    wxString m_bomFmtPresetName;

    // Format options
    wxString m_fieldDelimiter;
    wxString m_stringDelimiter;
    wxString m_refDelimiter;
    wxString m_refRangeDelimiter;
    bool     m_keepTabs;
    bool     m_keepLineBreaks;

    // Fields options
    std::vector<wxString> m_fieldsOrdered;
    std::vector<wxString> m_fieldsLabels;
    std::vector<wxString> m_fieldsGroupBy;
    wxString              m_sortField;
    bool                  m_sortAsc;
    wxString              m_filterString;
    bool                  m_excludeDNP;

    // Variant names to export. Empty vector means default variant only.
    std::vector<wxString> m_variantNames;
};

#endif
