/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <jobs/job_export_pcb_drill.h>

JOB_EXPORT_PCB_DRILL::JOB_EXPORT_PCB_DRILL( bool aIsCli ) :
    JOB( "drill", aIsCli ),
    m_filename(),
    m_outputDir(),
    m_excellonMirrorY( false ),
    m_excellonMinimalHeader( false ),
    m_excellonCombinePTHNPTH( true ),
    m_excellonOvalDrillRoute( false ),
    m_format( DRILL_FORMAT::EXCELLON ),
    m_drillOrigin( DRILL_ORIGIN::ABS ),
    m_drillUnits( DRILL_UNITS::INCHES ),
    m_zeroFormat( ZEROS_FORMAT::DECIMAL ),
    m_mapFormat( MAP_FORMAT::PDF ),
    m_gerberPrecision( 5 ),
    m_generateMap( false )
{
}