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

#ifndef JOB_SYM_EXPORT_SVG_H
#define JOB_SYM_EXPORT_SVG_H

#include <kicommon.h>
#include "job.h"

class KICOMMON_API JOB_SYM_EXPORT_SVG : public JOB
{
public:
    JOB_SYM_EXPORT_SVG();

    wxString m_libraryPath;
    wxString m_symbol;

    wxString m_outputDirectory;

    wxString m_colorTheme;

    bool m_blackAndWhite;

    bool m_includeHiddenPins;
    bool m_includeHiddenFields;
};

#endif