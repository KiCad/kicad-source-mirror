/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <math/box2.h>
#include <sch_pin.h>


/**
 * A pin layout helper is a class that manages the layout of the parts of
 * a pin on a schematic symbol:
 *
 * including, extents of:
 * - the pin itself
 * - the pin number, number, type
 * - decorations
 * - alternate mode icons
 *
 * This is useful, because this information is used in multiple places,
 * and regenerating it in multiple places is error-prone. It can also
 * be cached if it's encapsulated in one place.
 */
class PIN_LAYOUT_CACHE
{
public:
    PIN_LAYOUT_CACHE( const SCH_PIN& aPin );

    enum DIRTY_FLAGS
    {
        NAME      = 1,
        NUMBER    = 2,

        ALL       = NAME | NUMBER,
    };

    /**
     * Recompute all the layout information.
     */
    void MarkDirty( int aFlags );

    /**
     * Get the bounding box of the pin itself.
     */
    BOX2I GetPinBoundingBox( bool aIncludeLabelsOnInvisiblePins, bool aIncludeNameAndNumber,
                             bool aIncludeElectricalType );

private:

    bool isDirty( int aMask ) const
    {
        return m_dirtyFlags & aMask;
    }

    void setClean( int aMask )
    {
        m_dirtyFlags &= ~aMask;
    }

    /**
     * Cached extent of a text item.
     */
    struct TEXT_EXTENTS_CACHE
    {
        KIFONT::FONT* m_Font = nullptr;
        int           m_FontSize = 0;
        VECTOR2I      m_Extents;
    };

    static void recomputeExtentsCache( bool aDefinitelyDirty, KIFONT::FONT* aFont, int aSize,
                                       const wxString& aText, const KIFONT::METRICS& aFontMetrics,
                                       TEXT_EXTENTS_CACHE& aCache );

    /// The pin in question
    const SCH_PIN& m_pin;

    int m_dirtyFlags;

    // Various cache members
    TEXT_EXTENTS_CACHE m_numExtentsCache;
    TEXT_EXTENTS_CACHE m_nameExtentsCache;
};