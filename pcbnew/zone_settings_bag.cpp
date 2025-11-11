/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "zone_settings_bag.h"

#include <zone_manager/managed_zone.h>
#include <zone.h>
#include <memory>
#include <algorithm>


ZONE_SETTINGS_BAG::ZONE_SETTINGS_BAG( BOARD* aBoard )
{
    std::vector<std::shared_ptr<ZONE>> clonedZones;

    for( ZONE* zone : aBoard->Zones() )
    {
        if( !zone->GetIsRuleArea() && !zone->IsTeardropArea() && zone->IsOnCopperLayer() )
        {
            auto zone_clone = std::shared_ptr<ZONE>( static_cast<ZONE*>( zone->Clone() ) );
            m_zonesCloneMap.try_emplace( zone, zone_clone );
            m_clonedZoneList.push_back( zone_clone.get() );
            clonedZones.push_back( std::move( zone_clone ) );
        }
    }

    std::sort( clonedZones.begin(), clonedZones.end(),
               []( std::shared_ptr<ZONE> const& l, std::shared_ptr<ZONE> const& r )
               {
                   return l->GetAssignedPriority() > r->GetAssignedPriority();
               } );

    unsigned currentPriority = clonedZones.size() - 1;

    for( const std::shared_ptr<ZONE>& zone : clonedZones )
    {
        m_managedZones.push_back( std::make_shared<MANAGED_ZONE>( zone, currentPriority ) );
        --currentPriority;
    }
}


ZONE_SETTINGS_BAG::ZONE_SETTINGS_BAG( ZONE* aZone, ZONE_SETTINGS* aSettings )
{
    m_zoneSettings[aZone] = std::make_shared<ZONE_SETTINGS>( *aSettings );
}


std::shared_ptr<ZONE_SETTINGS> ZONE_SETTINGS_BAG::GetZoneSettings( ZONE* aZone )
{
    if( auto ll = m_zoneSettings.find( aZone ); ll != m_zoneSettings.end() )
        return ll->second;

    std::shared_ptr<ZONE_SETTINGS> zoneSetting = std::make_shared<ZONE_SETTINGS>();
    *zoneSetting << *aZone;
    m_zoneSettings.try_emplace( aZone, zoneSetting );
    return zoneSetting;
}


void ZONE_SETTINGS_BAG::UpdateClonedZones()
{
    for( const std::shared_ptr<MANAGED_ZONE>& zone : m_managedZones )
    {
        if( auto ll = m_zoneSettings.find( &zone->GetZone() ); ll != m_zoneSettings.end() )
            ll->second->ExportSetting( zone->GetZone() );
    }

    // Prevent version-control churn by not updating sparse priorities if their order didn't
    // change.
    bool priorityChanged = false;

    for( const std::shared_ptr<MANAGED_ZONE>& zone : m_managedZones )
    {
        if( zone->PriorityChanged() )
        {
            priorityChanged = true;
            break;
        }
    }

    if( priorityChanged )
    {
        for( std::shared_ptr<MANAGED_ZONE>& zone : m_managedZones )
            zone->GetZone().SetAssignedPriority( zone->GetCurrentPriority() );
    }
}