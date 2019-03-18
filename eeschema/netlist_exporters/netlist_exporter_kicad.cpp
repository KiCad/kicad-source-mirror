/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.TXT for contributors.
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


#include <algorithm>
#include <fctsys.h>
#include <build_version.h>
#include <confirm.h>

#include <sch_edit_frame.h>
#include <xnode.h>
#include <connection_graph.h>
#include "netlist_exporter_kicad.h"

bool NETLIST_EXPORTER_KICAD::WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions )
{
    wxASSERT( m_graph );

    try
    {
        FILE_OUTPUTFORMATTER formatter( aOutFileName );
        Format( &formatter, GNL_ALL );
    }

    catch( const IO_ERROR& ioe )
    {
        DisplayError( NULL, ioe.What() );
        return false;
    }

    /**
     * Temporary QC measure:
     * Generate the netlist again using the old algorithm and compare.
     * In theory, if the schematic does not use any of the new bus techniques
     * (bus aliases, bus groups, etc) they should match.  If not, we can throw
     * a warning and generate some debug output to fix the new netlister.
     *
     * This whole block can be removed once we are confident in the new code.
     */

    if( !m_graph->UsesNewBusFeatures() )
    {
        auto old_nets = makeListOfNets( false );

        bool different = false;

        for( auto it : m_graph->m_net_code_to_subgraphs_map )
        {
            // auto code = it.first;
            auto subgraphs = it.second;
            auto net_name = subgraphs[0]->GetNetName();

            std::set<wxString> net_pins;

            for( auto subgraph : subgraphs )
            {
                auto sheet = subgraph->m_sheet;

                for( auto item : subgraph->m_items )
                {
                    if( item->Type() == SCH_PIN_CONNECTION_T )
                    {
                        auto pc = static_cast<SCH_PIN_CONNECTION*>( item );

                        if( pc->m_pin->IsPowerConnection() ||
                            (LIB_PART*)( pc->m_pin->GetParent() )->IsPower() )
                            continue;

                        wxString refText = pc->m_comp->GetRef( &sheet );
                        wxString pinText = pc->m_pin->GetNumber();

                        net_pins.insert( refText + "-" + pinText );
                    }
                }
            }

            bool found = false;

            // Yes this is slow, but it's a temporary debugging thing.
            for( auto kid = old_nets->GetChildren(); kid; kid = kid->GetNext() )
            {
                for( auto attr = kid->GetAttributes(); attr; attr = attr->GetNext() )
                {
                    if( attr->GetName() == "name" && attr->GetValue() == net_name )
                    {
                        found = true;

                        // Check members of this net
                        std::set<wxString> old_net_pins;

                        for( auto pin_node = kid->GetChildren();
                             pin_node; pin_node = pin_node->GetNext() )
                        {
                            wxString ref, pin;

                            for( auto pin_attr = pin_node->GetAttributes();
                                 pin_attr; pin_attr = pin_attr->GetNext() )
                            {
                                if( pin_attr->GetName() == "ref" )
                                    ref = pin_attr->GetValue();

                                if( pin_attr->GetName() == "pin" )
                                    pin = pin_attr->GetValue();
                            }

                            old_net_pins.insert( ref + "-" + pin );
                        }

                        std::vector<wxString> difference( std::max( net_pins.size(),
                                                                    old_net_pins.size() ) );

                        auto end = std::set_symmetric_difference( net_pins.begin(),
                                                                  net_pins.end(),
                                                                  old_net_pins.begin(),
                                                                  old_net_pins.end(),
                                                                  difference.begin() );

                        difference.resize( end - difference.begin() );

                        if( difference.size() > 0 )
                        {
                            different = true;
                        }
                    }
                }
            }

            if( !found )
            {
                different = true;
            }
        }

        if( different )
        {
            wxLogDebug( "NOTE: New netlist algorithm is inconsistent with old! "
                        "Please contact Jon Evans <jon@craftyjon.com> "
                        "to help debug this issue" );

            try
            {
                FILE_OUTPUTFORMATTER formatter( aOutFileName + ".old_algo" );
                Format( &formatter, GNL_ALL );
            }

            catch( const IO_ERROR& ioe )
            {
                DisplayError( NULL, ioe.What() );
                return false;
            }
        }
    }

    return true;
}


void NETLIST_EXPORTER_KICAD::Format( OUTPUTFORMATTER* aOut, int aCtl )
{
    // Prepare list of nets generation
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    std::unique_ptr<XNODE> xroot( makeRoot( aCtl ) );

    xroot->Format( aOut, 0 );
}
