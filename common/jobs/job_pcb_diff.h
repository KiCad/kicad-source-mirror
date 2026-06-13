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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <kicommon.h>

#include "job_diff_base.h"


/**
 * Job: diff two PCB files end-to-end via PCB_DIFFER.
 *
 * Inputs are paths (the kiface side loads BOARDs). Output is either written
 * to the configured output path (JSON or text), or printed to stdout when the
 * path is empty. Exit code follows the existing KiCad CLI convention used by gerber
 * diff: 0 = identical, ERR_RC_VIOLATIONS (5) = differences found, non-zero
 * for I/O or argument errors.
 */
class KICOMMON_API JOB_PCB_DIFF : public JOB_DIFF_BASE
{
public:
    JOB_PCB_DIFF();

    wxString GetDefaultDescription() const override;
};
