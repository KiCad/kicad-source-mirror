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

#include <jobs/job_export_sch_netlist.h>
#include <jobs/job_registry.h>
#include <i18n_utility.h>


NLOHMANN_JSON_SERIALIZE_ENUM( JOB_EXPORT_SCH_NETLIST::FORMAT,
                              {
                                      { JOB_EXPORT_SCH_NETLIST::FORMAT::KICADSEXPR, "kicad" },
                                      { JOB_EXPORT_SCH_NETLIST::FORMAT::KICADXML, "xml" },
                                      { JOB_EXPORT_SCH_NETLIST::FORMAT::ALLEGRO, "allegro" },
                                      { JOB_EXPORT_SCH_NETLIST::FORMAT::PADS, "pads" },
                                      { JOB_EXPORT_SCH_NETLIST::FORMAT::CADSTAR, "cadstar" },
                                      { JOB_EXPORT_SCH_NETLIST::FORMAT::ORCADPCB2, "orcadpcb2" },
                                      { JOB_EXPORT_SCH_NETLIST::FORMAT::SPICE, "spice" },
                                      { JOB_EXPORT_SCH_NETLIST::FORMAT::SPICEMODEL, "spicemodel" },
                              } )

JOB_EXPORT_SCH_NETLIST::JOB_EXPORT_SCH_NETLIST( bool aIsCli ) :
    JOB( "netlist", false, aIsCli ),
    m_filename(),
    format( FORMAT::KICADSEXPR ),
    m_spiceSaveAllVoltages( false ),
    m_spiceSaveAllCurrents( false ),
    m_spiceSaveAllDissipations( false ),
    m_spiceSaveAllEvents( false )
{
    m_params.emplace_back( new JOB_PARAM<FORMAT>( "format", &format, format ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "spice.save_all_voltages", &m_spiceSaveAllVoltages,
                                                m_spiceSaveAllVoltages ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "spice.save_all_currents", &m_spiceSaveAllCurrents,
                                                m_spiceSaveAllCurrents ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "spice.save_all_events", &m_spiceSaveAllEvents,
                                                m_spiceSaveAllEvents ) );
    m_params.emplace_back( new JOB_PARAM<bool>( "spice.save_all_dissipations", &m_spiceSaveAllDissipations,
                                                m_spiceSaveAllDissipations ) );
}


wxString JOB_EXPORT_SCH_NETLIST::GetDescription()
{
    return wxString::Format( _( "Schematic Netlist Export" ) );
}


REGISTER_JOB( sch_export_netlist, _HKI( "Schematic: Export Netlist" ), KIWAY::FACE_SCH,
              JOB_EXPORT_SCH_NETLIST );