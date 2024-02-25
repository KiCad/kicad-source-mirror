/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_SCH_PROJECT_SETTINGS_H
#define KICAD_SCH_PROJECT_SETTINGS_H

struct SCH_SELECTION_FILTER_OPTIONS
{
    bool lockedItems;   ///< Allow selecting locked items
    bool symbols;       ///< Allow selecting symbols and sheet symbols
    bool text;          ///< Text and fields
    bool wires;         ///< Net and bus wires and junctions
    bool labels;        ///< Net and bus labels
    bool pins;          ///< Symbol and sheet pins
    bool graphics;      ///< Graphic lines, shapes, polygons
    bool images;        ///< Bitmap/vector images
    bool otherItems;    ///< Anything not fitting one of the above categories

    SCH_SELECTION_FILTER_OPTIONS()
    {
        lockedItems = true;
        symbols     = true;
        text        = true;
        wires       = true;
        labels      = true;
        pins        = true;
        graphics    = true;
        images      = true;
        otherItems  = true;
    }

    /**
     * @return true if any of the item types are enabled (excluding "locked items" which is special)
     */
    bool Any()
    {
        return ( symbols || text || wires || labels || pins || graphics || images || otherItems );
    }

    /**
     * @return true if all the item types are enabled (excluding "locked items" which is special)
     */
    bool All()
    {
        return ( symbols && text && wires && labels && pins && graphics && images && otherItems );
    }

    void SetDefaults()
    {
        lockedItems = false;
        symbols     = true;
        text        = true;
        wires       = true;
        labels      = true;
        pins        = true;
        graphics    = true;
        images      = true;
        otherItems  = true;
    }
};

#endif //KICAD_SCH_PROJECT_SETTINGS_H
