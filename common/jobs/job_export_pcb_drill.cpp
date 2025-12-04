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

#include <jobs/job_export_pcb_drill.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_DRILL::DRILL_FORMAT,
                              {
                                    { JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::EXCELLON, "excellon" },
                                    { JOB_EXPORT_PCB_DRILL::DRILL_FORMAT::GERBER, "gerber" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN,
                              {
                                    { JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::ABS, "abs" },
                                    { JOB_EXPORT_PCB_DRILL::DRILL_ORIGIN::PLOT, "plot" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_DRILL::DRILL_UNITS,
                              {
                                    { JOB_EXPORT_PCB_DRILL::DRILL_UNITS::INCH, "in" },
                                    { JOB_EXPORT_PCB_DRILL::DRILL_UNITS::MM, "mm" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT,
                              {
                                    { JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::DECIMAL, "decimal" },
                                    { JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_LEADING, "surpress_leading" },
                                    { JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::SUPPRESS_TRAILING, "surpress_trailing" },
                                    { JOB_EXPORT_PCB_DRILL::ZEROS_FORMAT::KEEP_ZEROS, "keep_zeros" },
                              } )

NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_PCB_DRILL::MAP_FORMAT,
                              {
                                      { JOB_EXPORT_PCB_DRILL::MAP_FORMAT::DXF, "dxf" },
                                      { JOB_EXPORT_PCB_DRILL::MAP_FORMAT::GERBER_X2, "gerberx2" },
                                      { JOB_EXPORT_PCB_DRILL::MAP_FORMAT::PDF, "pdf" },
                                      { JOB_EXPORT_PCB_DRILL::MAP_FORMAT::POSTSCRIPT, "postscript" },
                                      { JOB_EXPORT_PCB_DRILL::MAP_FORMAT::SVG, "svg" },
                              } )

JOB_EXPORT_PCB_DRILL::JOB_EXPORT_PCB_DRILL() :
        JOB( "drill", true ),
        m_filename(),
        m_excellonMirrorY( false ),
        m_excellonMinimalHeader( false ),
        m_excellonCombinePTHNPTH( true ),
        m_excellonOvalDrillRoute( false ),
        m_format( DRILL_FORMAT::EXCELLON ),
        m_drillOrigin( DRILL_ORIGIN::ABS ),
        m_drillUnits( DRILL_UNITS::INCH ),
        m_zeroFormat( ZEROS_FORMAT::DECIMAL ),
        m_mapFormat( MAP_FORMAT::PDF ),
        m_gerberPrecision( 5 ),
        m_generateMap( false ),
        m_generateTenting( false ),
        m_generateReport( false )
{
    m_params.emplace_back( new JOB_PARAM<bool>( "excellon.mirror_y",
                                                &m_excellonMirrorY,
                                                m_excellonMirrorY ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "excellon.minimal_header",
                                                &m_excellonMinimalHeader,
                                                m_excellonMinimalHeader ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "excellon.combine_pth_npth",
                                                &m_excellonCombinePTHNPTH,
                                                m_excellonCombinePTHNPTH ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "excellon.oval_drill_route",
                                                &m_excellonOvalDrillRoute,
                                                m_excellonOvalDrillRoute ) );

    m_params.emplace_back( new JOB_PARAM<DRILL_FORMAT>( "format",
                                                        &m_format,
                                                        m_format ) );

    m_params.emplace_back( new JOB_PARAM<DRILL_ORIGIN>( "drill_origin",
                                                        &m_drillOrigin,
                                                        m_drillOrigin ) );

    m_params.emplace_back( new JOB_PARAM<DRILL_UNITS>( "units",
                                                       &m_drillUnits,
                                                       m_drillUnits ) );

    m_params.emplace_back( new JOB_PARAM<ZEROS_FORMAT>(  "zero_format",
                                                         &m_zeroFormat,
                                                         m_zeroFormat ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "generate_map",
                                                &m_generateMap,
                                                m_generateMap ) );

    m_params.emplace_back( new JOB_PARAM<bool>( "generate_tenting",
                                                &m_generateTenting,
                                                m_generateTenting ) );

    m_params.emplace_back( new JOB_PARAM<MAP_FORMAT>( "map_format",
                                                      &m_mapFormat,
                                                      m_mapFormat ) );

    m_params.emplace_back( new JOB_PARAM<int>( "gerber_precision",
                                               &m_gerberPrecision,
                                               m_gerberPrecision ) );
    
    m_params.emplace_back( new JOB_PARAM<bool>( "generate_report",
                                                &m_generateReport,
                                                m_generateReport ) );

    m_params.emplace_back( new JOB_PARAM<wxString>( "report_filename", 
                                                        &m_reportPath, 
                                                         m_reportPath ) );
}


wxString JOB_EXPORT_PCB_DRILL::GetDefaultDescription() const
{
    return _( "Export drill data" );
}


wxString JOB_EXPORT_PCB_DRILL::GetSettingsDialogTitle() const
{
    return _( "Export Drill Data Job Settings" );
}

REGISTER_JOB( pcb_export_drill, _HKI( "PCB: Export Drill Data" ), KIWAY::FACE_PCB,
              JOB_EXPORT_PCB_DRILL );