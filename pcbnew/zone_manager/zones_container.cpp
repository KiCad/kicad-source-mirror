/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "zones_container.h"
#include "zone_priority_container.h"

#include <teardrop/teardrop_types.h>
#include <zone.h>
#include <memory>
#include <algorithm>


ZONES_CONTAINER::ZONES_CONTAINER( BOARD* aBoard ) : m_originalZoneList( aBoard->Zones() )
{
    std::vector<std::shared_ptr<ZONE>> clonedZones;

    for( ZONE* zone : aBoard->Zones() )
    {
        if( ( !zone->GetIsRuleArea() ) && IsCopperLayer( zone->GetFirstLayer() )
            && ( !zone->IsTeardropArea() ) )
        {
            std::shared_ptr<ZONE> zone_clone =
                    std::shared_ptr<ZONE>( static_cast<ZONE*>( zone->Clone() ) );
            m_zonesCloneMap.try_emplace( zone, zone_clone );
            m_clonedZoneList.push_back( zone_clone.get() );
            clonedZones.push_back( std::move( zone_clone ) );
        }
    }

    std::sort( clonedZones.begin(), clonedZones.end(),
               []( std::shared_ptr<ZONE> const& l, std::shared_ptr<ZONE> const& r

               )
               {
                   return l->GetAssignedPriority() > r->GetAssignedPriority();
               } );

    unsigned currentPriority = clonedZones.size() - 1;

    for( const std::shared_ptr<ZONE>& zone : clonedZones )
    {
        m_zonesPriorityContainer.push_back(
                std::make_shared<ZONE_PRIORITY_CONTAINER>( zone, currentPriority ) );
        --currentPriority;
    }
}

ZONES_CONTAINER::~ZONES_CONTAINER() = default;

std::shared_ptr<ZONE_SETTINGS> ZONES_CONTAINER::GetZoneSettings( ZONE* aZone )
{
    if( auto ll = m_zoneSettings.find( aZone ); ll != m_zoneSettings.end() )
        return ll->second;

    std::shared_ptr<ZONE_SETTINGS> zoneSetting = std::make_shared<ZONE_SETTINGS>();
    *zoneSetting << *aZone;
    m_zoneSettings.try_emplace( aZone, zoneSetting );
    return zoneSetting;
}

void ZONES_CONTAINER::OnUserConfirmChange()
{
    FlushZoneSettingsChange();
    FlushPriorityChange();

    for( auto& [zone, zoneClone] : m_zonesCloneMap )
    {
        std::map<PCB_LAYER_ID, std::shared_ptr<SHAPE_POLY_SET>> filled_zone_to_restore;
        zone->GetLayerSet().RunOnLayers(
                // Capture 'zone' by value and 'filled_zone_to_restore' by reference for MacOS clang
                [&filled_zone_to_restore, zone]( PCB_LAYER_ID layer )
                {
                    std::shared_ptr<SHAPE_POLY_SET> fill = zone->GetFilledPolysList( layer );
                    if( fill )
                    {
                        filled_zone_to_restore[layer] = fill;
                    }
                } );

        *zone = *zoneClone;

        for( auto& it : filled_zone_to_restore )
        {
            zone->SetFilledPolysList( it.first, *it.second.get() );
        }
    }
}

void ZONES_CONTAINER::FlushZoneSettingsChange()
{
    for( const std::shared_ptr<ZONE_PRIORITY_CONTAINER>& zone : m_zonesPriorityContainer )
    {
        if( auto ll = m_zoneSettings.find( &zone->GetZone() ); ll != m_zoneSettings.end() )
            ll->second->ExportSetting( zone->GetZone() );
    }
}

bool ZONES_CONTAINER::FlushPriorityChange()
{
    bool priorityChanged = false;

    for( const std::shared_ptr<ZONE_PRIORITY_CONTAINER>& c : m_zonesPriorityContainer )
    {
        if( c->PriorityChanged() )
        {
            priorityChanged = true;
            break;
        }
    }

    if( priorityChanged )
    {
        for( std::shared_ptr<ZONE_PRIORITY_CONTAINER>& c : m_zonesPriorityContainer )
            c->OnUserConfirmChange();
    }

    return priorityChanged;
}