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

#include "job.h"


/**
 * Shared base for the document/library diff jobs (PCB, schematic, symbol and
 * footprint library). Holds the input pair, output format, output path and
 * exit-code-only flag plus their JOB_PARAM registration; subclasses add only
 * their registry name and description. The two inputs are a file pair or a
 * directory pair depending on the document type, but serialize under the same
 * `input_a` / `input_b` keys.
 */
class KICOMMON_API JOB_DIFF_BASE : public JOB
{
public:
    enum class OUTPUT_FORMAT
    {
        JSON,
        TEXT,
        PNG,
        SVG
    };

    wxString GetSettingsDialogTitle() const override { return GetDefaultDescription(); }

    wxString      m_inputA;         ///< Reference document (file or directory)
    wxString      m_inputB;         ///< Comparison document (file or directory)
    OUTPUT_FORMAT m_format = OUTPUT_FORMAT::JSON;
    bool          m_exitCodeOnly = false;

protected:
    explicit JOB_DIFF_BASE( const std::string& aType );
};
