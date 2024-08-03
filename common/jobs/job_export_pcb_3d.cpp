/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2023-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <jobs/job_export_pcb_3d.h>


wxString EXPORTER_STEP_PARAMS::GetDefaultExportExtension() const
{
    switch( m_Format )
    {
    case EXPORTER_STEP_PARAMS::FORMAT::STEP: return wxS( "step" );
    case EXPORTER_STEP_PARAMS::FORMAT::BREP: return wxS( "brep" );
    case EXPORTER_STEP_PARAMS::FORMAT::XAO:  return wxS( "xao" );
    case EXPORTER_STEP_PARAMS::FORMAT::GLB:  return wxS( "glb" );
    default:                                 return wxEmptyString; // shouldn't happen
    }
}


wxString EXPORTER_STEP_PARAMS::GetFormatName() const
{
    switch( m_Format )
    {
        // honestly these names shouldn't be translated since they are mostly industry standard acronyms
    case EXPORTER_STEP_PARAMS::FORMAT::STEP: return wxS( "STEP" );
    case EXPORTER_STEP_PARAMS::FORMAT::BREP: return wxS( "BREP" );
    case EXPORTER_STEP_PARAMS::FORMAT::XAO:  return wxS( "XAO" );
    case EXPORTER_STEP_PARAMS::FORMAT::GLB:  return wxS( "Binary GLTF" );
    default:                                 return wxEmptyString; // shouldn't happen
    }
}


JOB_EXPORT_PCB_3D::JOB_EXPORT_PCB_3D( bool aIsCli ) :
    JOB( "3d", aIsCli ),
    m_hasUserOrigin( false ),
    m_filename(),
    m_format( JOB_EXPORT_PCB_3D::FORMAT::UNKNOWN ),
    m_vrmlUnits( JOB_EXPORT_PCB_3D::VRML_UNITS::METERS ),
    m_vrmlModelDir( wxEmptyString ),
    m_vrmlRelativePaths( false )
{
}
