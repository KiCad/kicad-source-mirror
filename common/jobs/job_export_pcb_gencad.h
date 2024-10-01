/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Julie Vairai <j.vairai@hexa-h.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef JOB_EXPORT_PCB_GENCAD_H
#define JOB_EXPORT_PCB_GENCAD_H

#include <kicommon.h>
#include <kicommon.h>
#include <layer_ids.h>
#include <wx/string.h>
#include "job.h"

class KICOMMON_API JOB_EXPORT_PCB_GENCAD : public JOB
{
public:
    JOB_EXPORT_PCB_GENCAD( bool aIsCli );
    wxString m_filename;

    bool m_flipBottomPads;
    bool m_useIndividualShapes;
    bool m_storeOriginCoords;
    bool m_useDrillOrigin;
    bool m_useUniquePins;
};

#endif
