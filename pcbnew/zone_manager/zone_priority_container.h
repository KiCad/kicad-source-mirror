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


#ifndef ZONE_PRIORITY_CONTAINER_H
#define ZONE_PRIORITY_CONTAINER_H

#include <memory>
#include <utility>
#include <zone.h>
#include "zone_management_base.h"
/**
 * @brief Workaround to keep the original priorities if user didn't change any
 *
 */
class ZONE_PRIORITY_CONTAINER : public ZONE_MANAGEMENT_BASE
{
    friend class MODEL_ZONES_OVERVIEW_TABLE;

public:
    ZONE_PRIORITY_CONTAINER( std::shared_ptr<ZONE> aZone, unsigned aInitialIndex ) :
            m_zone( std::move( aZone ) ),
            m_initialPriority( aInitialIndex ),
            m_currentPriority( aInitialIndex )
    {
    }

    ZONE_PRIORITY_CONTAINER() = delete;

    ~ZONE_PRIORITY_CONTAINER() override = default;

    bool PriorityChanged() const { return m_initialPriority != m_currentPriority; }

    unsigned GetCurrentPriority() const { return m_currentPriority; }

    void OnUserConfirmChange() override { m_zone->SetAssignedPriority( m_currentPriority ); }

    ZONE const& GetZone() const { return *m_zone; }

    ZONE& GetZone() { return *m_zone; }

private:
    std::shared_ptr<ZONE> m_zone;
    const unsigned        m_initialPriority;
    unsigned              m_currentPriority;
};

#endif