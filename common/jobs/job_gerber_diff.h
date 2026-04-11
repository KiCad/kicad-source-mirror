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

#ifndef JOB_GERBER_DIFF_H
#define JOB_GERBER_DIFF_H

#include <kicommon.h>
#include "job.h"

/**
 * Job to compare two Gerber files and highlight differences.
 *
 * Performs geometric comparison by converting both files to SHAPE_POLY_SET and
 * computing XOR to find added/removed regions.
 */
class KICOMMON_API JOB_GERBER_DIFF : public JOB
{
public:
    JOB_GERBER_DIFF();

    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    enum class OUTPUT_FORMAT
    {
        PNG,
        TEXT,
        JSON
    };

    wxString      m_inputFileA;
    wxString      m_inputFileB;
    OUTPUT_FORMAT m_outputFormat = OUTPUT_FORMAT::PNG;

    // PNG output options
    int  m_dpi = 300;
    bool m_antialias = true;
    bool m_transparentBackground = true;

    // When true, only report whether files differ (exit code 0 = identical, 1 = different)
    bool m_exitCodeOnly = false;

    // Tolerance for floating point comparisons in IU
    int m_tolerance = 0;

    // In strict mode, fail on any parse warnings/errors
    bool m_strict = false;

    // When true, skip bounding-box origin alignment before computing the diff.
    // Use this in regression tests to catch absolute-placement errors (wrong origin,
    // sign flip, unit conversion mistake, etc.) that auto-alignment would hide.
    bool m_noAlign = false;
};

#endif
