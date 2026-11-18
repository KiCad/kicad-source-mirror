/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 Krishna Swaroop <krishna.swaroop@pixxel.co.in>
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

#ifndef JOB_EXPORT_PCB_HYPERLYNX_H
#define JOB_EXPORT_PCB_HYPERLYNX_H

#include <kicommon.h>
#include "job.h"

class KICOMMON_API JOB_EXPORT_PCB_HYPERLYNX : public JOB
{
public:
    JOB_EXPORT_PCB_HYPERLYNX();
    wxString GetDefaultDescription() const override;
    wxString GetSettingsDialogTitle() const override;

    void SetDefaultOutputPath( const wxString& aReferenceName );

    wxString m_filename;
};

#endif
