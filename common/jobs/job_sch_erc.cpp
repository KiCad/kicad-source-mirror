/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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

#include <jobs/job_sch_erc.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>

JOB_SCH_ERC::JOB_SCH_ERC() :
    JOB_RC( "erc" )
{
}


wxString JOB_SCH_ERC::GetDefaultDescription() const
{
    return _( "Perform ERC" );
}


wxString JOB_SCH_ERC::GetSettingsDialogTitle() const
{
    return _( "Schematic ERC Job Settings" );
}


REGISTER_JOB( sch_erc, _HKI( "Schematic: Perform ERC" ), KIWAY::FACE_SCH, JOB_SCH_ERC );