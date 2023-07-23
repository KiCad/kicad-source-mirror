/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mike Williams <mike@mikebwilliams.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/string.h>
#include "job.h"

class JOB_EXPORT_SCH_BOM : public JOB
{
public:
    JOB_EXPORT_SCH_BOM( bool aIsCli ) :
            JOB( "bom", aIsCli ),
            m_filename(),
            m_outputFile(),

            m_fieldDelimiter(),
            m_stringDelimiter(),
            m_refDelimiter(),
            m_refRangeDelimiter(),
            m_keepTabs( false ),
            m_keepLineBreaks( false ),

            m_fieldsOrdered(),
            m_fieldsLabels(),
            m_fieldsGroupBy(),
            m_sortField(),
            m_sortAsc( true ),
            m_filterString(),
            m_excludeDNP( false )
    {
    }

    // Basic options
    wxString m_filename;
    wxString m_outputFile;

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
};

#endif
