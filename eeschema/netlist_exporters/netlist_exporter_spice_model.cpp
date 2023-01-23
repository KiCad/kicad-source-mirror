/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus.
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "netlist_exporter_spice_model.h"
#include <sch_screen.h>
#include <sch_label.h>


void NETLIST_EXPORTER_SPICE_MODEL::WriteHead( OUTPUTFORMATTER& aFormatter,
                                              unsigned aNetlistOptions )
{
    aFormatter.Print( 0, "*\n" );
    aFormatter.Print( 0, "\n" );
    aFormatter.Print( 0, ".subckt %s\n", TO_UTF8( m_schematic->Prj().GetProjectName() ) );

    for( auto const& [key, port] : m_ports )
    {
        std::string portDir;

        switch( port.dir )
        {
            case L_INPUT:       portDir = "input";      break;
            case L_OUTPUT:      portDir = "output";     break;
            case L_BIDI:        portDir = "inout";      break;
            case L_TRISTATE:    portDir = "tristate";   break;
            case L_UNSPECIFIED: portDir = "passive";    break;

            default:
                wxFAIL_MSG( "Invalid port direction" );
                break;
        }

        aFormatter.Print( 0, "+       %s ; %s\n", port.name.c_str(), portDir.c_str() );
    }

    aFormatter.Print( 0, "\n\n" );
}


void NETLIST_EXPORTER_SPICE_MODEL::WriteTail( OUTPUTFORMATTER& aFormatter,
                                              unsigned aNetlistOptions )
{
    aFormatter.Print( 0, "\n.ends\n" );
}


bool NETLIST_EXPORTER_SPICE_MODEL::ReadSchematicAndLibraries( unsigned aNetlistOptions,
                                                              REPORTER& aReporter )
{
    readPorts( aNetlistOptions );

    return NETLIST_EXPORTER_SPICE::ReadSchematicAndLibraries( aNetlistOptions, aReporter );
}


std::string NETLIST_EXPORTER_SPICE_MODEL::GenerateItemPinNetName( const std::string& aNetName,
                                                                  int& aNcCounter ) const
{
    std::string netName = aNetName;

    if( m_ports.count( netName ) )
        netName = m_ports.at( netName ).name;

    return NETLIST_EXPORTER_SPICE::GenerateItemPinNetName( netName, aNcCounter );
}


void NETLIST_EXPORTER_SPICE_MODEL::readPorts( unsigned aNetlistOptions )
{
    for( const SCH_SHEET_PATH& sheet : GetSheets( aNetlistOptions ) )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
        {
            SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

            if( SCH_CONNECTION* conn = label->Connection( &sheet ) )
            {
                m_ports.insert( { std::string( conn->Name().ToUTF8() ),
                                  PORT_INFO{ std::string( label->GetText().ToUTF8() ),
                                             label->GetShape()
                                           }
                                } );
            }
        }
    }
}
