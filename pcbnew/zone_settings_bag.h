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


#pragma once

#include "board.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <zone.h>
#include <zone_settings.h>

class MANAGED_ZONE;


class ZONE_SETTINGS_BAG
{
public:
    ZONE_SETTINGS_BAG( BOARD* aBoard );
    ZONE_SETTINGS_BAG( ZONE* aZone, ZONE_SETTINGS* aSettings );

    ZONE_SETTINGS_BAG() = default;

    std::vector<std::shared_ptr<MANAGED_ZONE>> GetManagedZones() const
    {
        return m_managedZones;
    }

    std::shared_ptr<ZONE_SETTINGS> GetZoneSettings( ZONE* zone );

    /**
     * The cloned list is the working storage.
     */
    void UpdateClonedZones();

    std::vector<ZONE*>& GetOriginalZoneList()                            { return m_originalZoneList; }
    std::vector<ZONE*>& GetClonedZoneList()                              { return m_clonedZoneList; }
    std::unordered_map<ZONE*, std::shared_ptr<ZONE>>& GetZonesCloneMap() { return m_zonesCloneMap; }

private:
    std::unordered_map<ZONE*, std::shared_ptr<ZONE>>          m_zonesCloneMap;
    std::unordered_map<ZONE*, std::shared_ptr<ZONE_SETTINGS>> m_zoneSettings;
    std::vector<std::shared_ptr<MANAGED_ZONE>>                m_managedZones;
    std::vector<ZONE*>                                        m_clonedZoneList;
    std::vector<ZONE*>                                        m_originalZoneList;
};
