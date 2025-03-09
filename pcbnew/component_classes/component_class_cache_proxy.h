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


#ifndef PCBNEW_COMPONENT_CLASS_CACHE_PROXY_H
#define PCBNEW_COMPONENT_CLASS_CACHE_PROXY_H

#include <component_classes/component_class.h>
#include <footprint.h>

/*
 * A class which acts as a cache-aware proxy for a FOOTPRINT's component class.
 * Creating dynamic component classes (from component class generators) is an
 * expensive operation, so we want to cache the results. This class is a cache
 * proxy which tracks the validity of the cached component class with respect
 * to loaded static and dynamic component class rules
 */
class COMPONENT_CLASS_CACHE_PROXY
{
public:
    explicit COMPONENT_CLASS_CACHE_PROXY( FOOTPRINT* footprint ) : m_footprint( footprint ) {}

    /// Sets the static component class
    /// Static component classes are assigned in the schematic, and are transferred through the
    /// netlist
    void SetStaticComponentClass( const COMPONENT_CLASS* compClass )
    {
        m_staticComponentClass = compClass;
    }

    /// Gets the static component class
    const COMPONENT_CLASS* GetStaticComponentClass() const { return m_staticComponentClass; }

    /// Gets the full component class (static + dynamic resultant component class)
    const COMPONENT_CLASS* GetComponentClass() const;

    /// Forces recomputation of the component class
    void RecomputeComponentClass( COMPONENT_CLASS_MANAGER* manager = nullptr ) const;

    /// Invalidates the cache
    /// The component class will be recalculated on the next access
    void InvalidateCache() { m_lastTickerValue = -1; }

protected:
    FOOTPRINT* m_footprint;

    const COMPONENT_CLASS*         m_staticComponentClass{ nullptr };
    mutable const COMPONENT_CLASS* m_dynamicComponentClass{ nullptr };
    mutable const COMPONENT_CLASS* m_finalComponentClass{ nullptr };

    mutable long long int m_lastTickerValue{ -1 };
};

#endif //PCBNEW_COMPONENT_CLASS_CACHE_PROXY_H
