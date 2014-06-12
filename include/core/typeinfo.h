/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2004-2013 KiCad Developers, see change_log.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __KICAD_TYPEINFO_H
#define __KICAD_TYPEINFO_H

#include <cstdio>

class EDA_ITEM;

/**
 * Enum KICAD_T
 * is the set of class identification values, stored in EDA_ITEM::m_StructType
 */
enum KICAD_T
{
    NOT_USED = -1,          ///< the 3d code uses this value

    EOT = 0,                ///< search types array terminator (End Of Types)

    TYPE_NOT_INIT = 0,
    PCB_T,
    SCREEN_T,               ///< not really an item, used to identify a screen

    // Items in pcb
    PCB_MODULE_T,           ///< class MODULE, a footprint
    PCB_PAD_T,              ///< class D_PAD, a pad in a footprint
    PCB_LINE_T,             ///< class DRAWSEGMENT, a segment not on copper layers
    PCB_TEXT_T,             ///< class TEXTE_PCB, text on a layer
    PCB_MODULE_TEXT_T,      ///< class TEXTE_MODULE, text in a footprint
    PCB_MODULE_EDGE_T,      ///< class EDGE_MODULE, a footprint edge
    PCB_TRACE_T,            ///< class TRACK, a track segment (segment on a copper layer)
    PCB_VIA_T,              ///< class VIA, a via (like a track segment on a copper layer)
    PCB_ZONE_T,             ///< class SEGZONE, a segment used to fill a zone area (segment on a
                            ///< copper layer)
    PCB_MARKER_T,           ///< class MARKER_PCB, a marker used to show something
    PCB_DIMENSION_T,        ///< class DIMENSION, a dimension (graphic item)
    PCB_TARGET_T,           ///< class PCB_TARGET, a target (graphic item)
    PCB_ZONE_AREA_T,        ///< class ZONE_CONTAINER, a zone area
    PCB_ITEM_LIST_T,        ///< class BOARD_ITEM_LIST, a list of board items

    // Schematic draw Items.  The order of these items effects the sort order.
    // It is currently ordered to mimic the old Eeschema locate behavior where
    // the smallest item is the selected item.
    SCH_MARKER_T,
    SCH_JUNCTION_T,
    SCH_NO_CONNECT_T,
    SCH_BUS_WIRE_ENTRY_T,
    SCH_BUS_BUS_ENTRY_T,
    SCH_LINE_T,
    SCH_BITMAP_T,
    SCH_TEXT_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIERARCHICAL_LABEL_T,
    SCH_FIELD_T,
    SCH_COMPONENT_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,

    // Be prudent with these 3 types:
    // they should be used only to locate a specific field type
    // among SCH_FIELD_T items types
    SCH_FIELD_LOCATE_REFERENCE_T,
    SCH_FIELD_LOCATE_VALUE_T,
    SCH_FIELD_LOCATE_FOOTPRINT_T,

    // General
    SCH_SCREEN_T,

    /*
     * Draw items in library component.
     *
     * The order of these items effects the sort order for items inside the
     * "DRAW/ENDDRAW" section of the component definition in a library file.
     * If you add a new draw item, type, please make sure you add it so the
     * sort order is logical.
     */
    LIB_COMPONENT_T,
    LIB_ALIAS_T,
    LIB_ARC_T,
    LIB_CIRCLE_T,
    LIB_TEXT_T,
    LIB_RECTANGLE_T,
    LIB_POLYLINE_T,
    LIB_BEZIER_T,
    LIB_PIN_T,

    /*
     * Fields are not saved inside the "DRAW/ENDDRAW".  Add new draw item
     * types before this line.
     */
    LIB_FIELD_T,

    /*
     * For GerbView: items type:
     */
    TYPE_GERBER_DRAW_ITEM,

    /*
     * for Pl_Editor, in undo/redo commands
     */
    TYPE_PL_EDITOR_LAYOUT,

    // End value
    MAX_STRUCT_TYPE_ID
};

template<typename T>
struct remove_pointer
{
    typedef T type;
};

template<typename T>
struct remove_pointer<T*>
{
    typedef typename remove_pointer<T>::type type;
};

/**
 * Function IsA()
 *
 * Checks if the type of aObject is T.
 * @param aObject object for type check
 * @return true, if aObject type equals T.
 */
template <class T, class I>
bool IsA(const I *aObject)
{
    return aObject && remove_pointer<T>::type::ClassOf(aObject);
}

template <class T, class I>
bool IsA(const I& aObject)
{
    return remove_pointer<T>::type::ClassOf(&aObject);
}

/**
 * Function dyn_cast()
 *
 * A lightweight dynamic downcast. Casts aObject to type Casted*.
 * Uses EDA_ITEM::Type() and EDA_ITEM::ClassOf() to check if type matches.
 * @param aObject object to be casted
 * @return down-casted object or NULL if type doesn't match Casted.
 */
template<class Casted, class From>
Casted dyn_cast(From aObject)
{
    if( remove_pointer<Casted>::type::ClassOf ( aObject ) )
        return static_cast<Casted>( aObject );

    return NULL;
}

#endif // __KICAD_TYPEINFO_H

