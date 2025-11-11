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

#include <zone.h>
#include <memory>
#include <algorithm>


ZONE_SETTINGS_BAG::ZONE_SETTINGS_BAG( BOARD* aBoard )
{
    for( ZONE* zone : aBoard->Zones() )
    {
        if( !zone->GetIsRuleArea() && !zone->IsTeardropArea() && zone->IsOnCopperLayer() )
        {
            auto zone_clone = std::shared_ptr<ZONE>( static_cast<ZONE*>( zone->Clone() ) );
            m_zonesCloneMap.try_emplace( zone, zone_clone );
            m_clonedZoneList.push_back( zone_clone.get() );
        }
    }

    for( ZONE* zone : m_clonedZoneList )
    {
        m_zoneSettings[zone] = std::make_shared<ZONE_SETTINGS>();
        *m_zoneSettings[zone] << *zone;
    }

    std::vector<ZONE*> sortedClonedZones = m_clonedZoneList;

    std::sort( sortedClonedZones.begin(), sortedClonedZones.end(),
               []( ZONE* const& l, ZONE* const& r )
               {
                   return l->GetAssignedPriority() > r->GetAssignedPriority();
               } );

    unsigned currentPriority = sortedClonedZones.size() - 1;

    for( ZONE* zone : sortedClonedZones )
    {
        m_zonePriorities[zone] = std::make_pair<>( currentPriority, currentPriority );
        --currentPriority;
    }
}


ZONE_SETTINGS_BAG::ZONE_SETTINGS_BAG( ZONE* aZone, ZONE_SETTINGS* aSettings )
{
    m_zoneSettings[aZone] = std::make_shared<ZONE_SETTINGS>( *aSettings );
}


std::shared_ptr<ZONE_SETTINGS> ZONE_SETTINGS_BAG::GetZoneSettings( ZONE* aZone )
{
    return m_zoneSettings[aZone];
}


unsigned ZONE_SETTINGS_BAG::GetZonePriority( ZONE* aZone )
{
    return m_zonePriorities[aZone].second;
}


void ZONE_SETTINGS_BAG::SwapPriority( ZONE* aZone, ZONE* otherZone )
{
    std::swap( m_zonePriorities[aZone].second, m_zonePriorities[otherZone].second );
}


void ZONE_SETTINGS_BAG::UpdateClonedZones()
{
    for( ZONE* zone : m_clonedZoneList )
    {
        if( m_zoneSettings.contains( zone ) )
            m_zoneSettings[zone]->ExportSetting( *zone );
    }

    // Prevent version-control churn by not updating potentially sparse priorities if their
    // order didn't change.
    bool priorityChanged = false;

    for( ZONE* zone : m_clonedZoneList )
    {
        if( m_zonePriorities[zone].first != m_zonePriorities[zone].second )
        {
            priorityChanged = true;
            break;
        }
    }

    if( priorityChanged )
    {
        for( ZONE* zone : m_clonedZoneList )
            zone->SetAssignedPriority( m_zonePriorities[zone].second );
    }
}