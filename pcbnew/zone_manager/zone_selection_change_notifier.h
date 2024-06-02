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


#ifndef ZONE_SELECTION_CHANGE_NOTIFIER_H
#define ZONE_SELECTION_CHANGE_NOTIFIER_H

class ZONE;

/**
 * @brief Subscriber who is interested in the zone selection change
 *
 */
class ZONE_SELECTION_CHANGE_NOTIFIER
{
public:
    virtual ~ZONE_SELECTION_CHANGE_NOTIFIER() = default;

    ZONE* GetZone() const { return m_zone; }
    /**
     * @brief Inform the subscriber about the zone selection change
     *
     * @param aZone The current zone selection from  the publisher
     */
    void OnZoneSelectionChanged( ZONE* aZone )
    {
        if( m_zone == aZone )
            return;
        m_zone = aZone;
        ActivateSelectedZone( m_zone );
    }

    ZONE* GetSelectedZone() const { return m_zone; }

protected:
    virtual void ActivateSelectedZone( ZONE* new_zone ) = 0;

private:
    ZONE* m_zone{};
};

#endif