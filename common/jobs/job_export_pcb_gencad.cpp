/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Julie Vairai <j.vairai@hexa-h.com>
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

#include <jobs/job_export_pcb_gencad.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>


JOB_EXPORT_PCB_GENCAD::JOB_EXPORT_PCB_GENCAD() :
    JOB( "gencad", false ),
    m_flipBottomPads( false ),
    m_useIndividualShapes( false ),
    m_storeOriginCoords( false ),
    m_useDrillOrigin( false ),
    m_useUniquePins( false )
{
    m_params.emplace_back(
            new JOB_PARAM<bool>( "flip_bottom_pads", &m_flipBottomPads, m_flipBottomPads ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "use_individual_shapes", &m_useIndividualShapes,
                                                m_useIndividualShapes ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "store_origin_coords", &m_storeOriginCoords,
                                                m_storeOriginCoords ) );
    m_params.emplace_back(
            new JOB_PARAM<bool>( "use_drill_origin", &m_useDrillOrigin, m_useDrillOrigin ) );
    m_params.emplace_back(
            new JOB_PARAM<bool>( "use_unique_pins", &m_useUniquePins, m_useUniquePins ) );
}


wxString JOB_EXPORT_PCB_GENCAD::GetDefaultDescription() const
{
    return _( "Export GenCAD File" );
}


wxString JOB_EXPORT_PCB_GENCAD::GetSettingsDialogTitle() const
{
    return _( "Export GenCAD Job Settings" );
}


REGISTER_JOB( pcb_export_gencad, _HKI( "PCB: Export GenCAD" ), KIWAY::FACE_PCB,
              JOB_EXPORT_PCB_GENCAD );