/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#include <component_classes/component_class_cache_proxy.h>

#include <board.h>
#include <footprint.h>

const COMPONENT_CLASS* COMPONENT_CLASS_CACHE_PROXY::GetComponentClass() const
{
    COMPONENT_CLASS_MANAGER& mgr = m_footprint->GetBoard()->GetComponentClassManager();

    if( mgr.GetTicker() > m_lastTickerValue )
    {
        RecomputeComponentClass( &mgr );
    }

    return m_finalComponentClass;
}


void COMPONENT_CLASS_CACHE_PROXY::RecomputeComponentClass( COMPONENT_CLASS_MANAGER* manager ) const
{
    if( !manager )
        manager = &m_footprint->GetBoard()->GetComponentClassManager();

    m_dynamicComponentClass = manager->GetDynamicComponentClassesForFootprint( m_footprint );
    m_finalComponentClass =
            manager->GetCombinedComponentClass( m_staticComponentClass, m_dynamicComponentClass );
    m_lastTickerValue = manager->GetTicker();
}
