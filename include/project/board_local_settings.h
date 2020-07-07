/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_BOARD_LOCAL_SETTINGS_H
#define KICAD_BOARD_LOCAL_SETTINGS_H

/**
 * Selection filtering that applies all the time (not the "filter selection" dialog that modifies
 * the current selection)
 */
struct SELECTION_FILTER_OPTIONS
{
    bool lockedItems;   ///< Allow selecting locked items
    bool footprints;    ///< Allow selecting entire footprints
    bool text;          ///< Text (free or attached to a footprint)
    bool tracks;        ///< Copper tracks
    bool vias;          ///< Vias (all types>
    bool pads;          ///< Footprint pads
    bool graphics;      ///< Graphic lines, shapes, polygons
    bool zones;         ///< Copper zones
    bool keepouts;      ///< Keepout zones
    bool dimensions;    ///< Dimension items
    bool otherItems;    ///< Anything not fitting one of the above categories

    SELECTION_FILTER_OPTIONS()
    {
        lockedItems = true;
        footprints  = true;
        text        = true;
        tracks      = true;
        vias        = true;
        pads        = true;
        graphics    = true;
        zones       = true;
        keepouts    = true;
        dimensions  = true;
        otherItems  = true;
    }

    bool Any()
    {
        return ( lockedItems || footprints || text || tracks || vias || pads || graphics || zones
                 || keepouts || dimensions || otherItems );
    }

    bool All()
    {
        return ( lockedItems && footprints && text && tracks && vias && pads && graphics && zones
                 && keepouts && dimensions && otherItems );
    }
};

#endif // KICAD_BOARD_LOCAL_SETTINGS_H
