/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see change_log.txt for contributors.
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


#include <fctsys.h>
#include <drc/drc_rule.h>
#include <board_design_settings.h>
#include <class_board.h>
#include <class_board_item.h>

/*
 * Match tokens:
 *     match_netclass
 *     match_type
 *     match_layer
 *     match_all
 *     match_area
 *
 *     (selector (match_area "$board") (rule "OSHParkClass3") (priority 100))
 *
 *     (selector (match_netclass "HV") (rule "HV_internal"))
 *     (selector (match_netclass "HV") (match_layer "F_Cu") (rule "HV_external"))
 *     (selector (match_netclass "HV") (match_layer "B_Cu") (rule "HV_external"))
 *
 *     (selector (match_netclass "HV") (match_netclass "HV") (rule "HV2HV"))
 *     (selector (match_netclass "HV") (match_netclass "HV") (match_layer "F_Cu") (rule "HV2HV_external"))
 *     (selector (match_netclass "HV") (match_netclass "HV") (match_layer "B_Cu") (rule "HV2HV_external"))
 *
 *   TODO: pads for connector pins or wire pads have even larger clearances.  How to encode?
 *   User attributes on parent footprint?
 *
 *     (selector (match_netclass "HV") (match_type "pad") (match_netclass "HV") (match_type "pad") (rule "pad2PadHV"))
 *
 *     (selector (match_netclass "signal") (match_area "BGA") (rule "neckdown"))
 *
 * Type tokens:
 *     track
 *     via
 *     micro_via
 *     blind_via
 *     pad
 *     zone
 *     text
 *     graphic
 *     board_edge
 *     hole
 *     npth
 *     pth
 *
 * Rule tokens:
 *     allow
 *     clearance
 *     annulus_width
 *     track_width
 *     hole
 *
 * Rule modifiers:
 *     relaxed
 *
 *     (rule "HV" (clearance 200) (priority 200))
 *     (rule "HV_external" (clearance 400) (priority 200))
 *     (rule "HV2HV" (clearance 200) (priority 200))
 *     (rule "HV2HV_external" (clearance 500) (priority 200))
 *     (rule "pad2padHV" (clearance 500) (priority 200))
 *
 *     (rule "signal" (clearance 20))                       // implied priority of 1
 *     (rule "neckdown" (clearance relaxed 15) (priority 2))
 *
 *     (rule "allowMicrovias" (allow microvia))
 */


void MatchSelectors( const std::vector<DRC_SELECTOR*>& aSelectors,
                     const BOARD_ITEM* aItem, const NETCLASS* aNetclass,
                     const BOARD_ITEM* bItem, const NETCLASS* bNetclass,
                     std::vector<DRC_SELECTOR*>* aSelected )
{
    for( DRC_SELECTOR* candidate : aSelectors )
    {
        if( candidate->m_MatchNetclasses.size() == 2 )
        {
            if( !bItem )
                continue;

            NETCLASS* firstNetclass = candidate->m_MatchNetclasses[0].get();
            NETCLASS* secondNetclass = candidate->m_MatchNetclasses[1].get();

            if( !( aNetclass == firstNetclass && bNetclass == secondNetclass )
                    && !( aNetclass == secondNetclass && bNetclass == firstNetclass ) )
            {
                continue;
            }
        }
        else if( candidate->m_MatchNetclasses.size() == 1 )
        {
            NETCLASS* matchNetclass = candidate->m_MatchNetclasses[0].get();

            if( matchNetclass != aNetclass && !( bItem && matchNetclass == bNetclass ) )
                continue;
        }

        if( candidate->m_MatchTypes.size() == 2 )
        {
            if( !bItem )
                continue;

            KICAD_T firstType[2] = { candidate->m_MatchTypes[0], EOT };
            KICAD_T secondType[2] = { candidate->m_MatchTypes[1], EOT };

            if( !( aItem->IsType( firstType ) && bItem->IsType( secondType ) )
                    && !( aItem->IsType( secondType ) && bItem->IsType( firstType ) ) )
            {
                continue;
            }
        }
        else if( candidate->m_MatchTypes.size() == 1 )
        {
            KICAD_T matchType[2] = { candidate->m_MatchTypes[0], EOT };

            if( !aItem->IsType( matchType ) && !( bItem && bItem->IsType( matchType ) ) )
                continue;
        }

        if( candidate->m_MatchLayers.size() )
        {
            PCB_LAYER_ID matchLayer = candidate->m_MatchLayers[0];

            if( !aItem->GetLayerSet().test( matchLayer )
                    || ( bItem && !bItem->GetLayerSet().test( matchLayer ) ) )
            {
                continue;
            }
        }

        if( candidate->m_MatchAreas.size() )
        {
            if( candidate->m_MatchAreas[0] == "$board" )
            {
                // matches everything
            }
            else
            {
                // TODO: area/room matches...
            }
        }

        // All tests done; if we're still here then it matches
        aSelected->push_back( candidate );
    }
}


