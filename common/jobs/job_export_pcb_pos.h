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

#ifndef JOB_EXPORT_PCB_POS_H
#define JOB_EXPORT_PCB_POS_H

#include <kicommon.h>
#include <layer_ids.h>
#include "job.h"

class KICOMMON_API JOB_EXPORT_PCB_POS : public JOB
{
public:
    JOB_EXPORT_PCB_POS();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    void SetDefaultOutputPath( const wxString& aReferenceName );

    enum class SIDE
    {
        FRONT,
        BACK,
        BOTH
    };

    enum class UNITS
    {
        INCH,       // Do not use IN: it conflicts with a Windows header
        MM
    };

    enum class FORMAT
    {
        ASCII,
        CSV,
        GERBER
    };

public:
    wxString m_filename;

    bool     m_useDrillPlaceFileOrigin;
    bool     m_smdOnly;
    bool     m_excludeFootprintsWithTh;
    bool     m_excludeDNP;
    bool     m_excludeBOM;
    bool     m_negateBottomX;
    bool     m_singleFile;
    bool     m_nakedFilename;
    SIDE     m_side;
    UNITS    m_units;
    FORMAT   m_format;
    bool     m_gerberBoardEdge;

    wxString m_variant;         ///< Variant name for variant-aware filtering
};

#endif