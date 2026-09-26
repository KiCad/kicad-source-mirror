/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef JOB_EXPORT_PCB_IDF_H
#define JOB_EXPORT_PCB_IDF_H

#include <kicommon.h>
#include <math/vector2d.h>
#include "job.h"

class KICOMMON_API JOB_EXPORT_PCB_IDF : public JOB
{
public:
    JOB_EXPORT_PCB_IDF();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    void SetDefaultOutputPath( const wxString& aReferenceName );

    enum class UNITS
    {
        MM,
        MILS
    };

    enum class COORD_ORIGIN
    {
        DRILL,
        GRID,
        CENTER,
        USER
    };

    wxString m_filename;
    UNITS    m_units;

    COORD_ORIGIN m_originMode;
    VECTOR2D     m_userOrigin;

    bool     m_includeUnspecified;
    bool     m_includeDNP;
    bool     m_calculateHeightFromModels;
};

#endif
