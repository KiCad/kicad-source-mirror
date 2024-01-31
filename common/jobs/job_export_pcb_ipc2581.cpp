/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <jobs/job_export_pcb_ipc2581.h>

JOB_EXPORT_PCB_IPC2581::JOB_EXPORT_PCB_IPC2581( bool aIsCli ) :
    JOB( "ipc2581", aIsCli ),
    m_filename(),
    m_outputFile(),
    m_drawingSheet(),
    m_units( IPC2581_UNITS::MILLIMETERS ),
    m_version( IPC2581_VERSION::C ),
    m_precision( 3 ),
    m_compress( false ),
    m_colInternalId(),
    m_colMfgPn(),
    m_colMfg(),
    m_colDistPn(),
    m_colDist()
{
}