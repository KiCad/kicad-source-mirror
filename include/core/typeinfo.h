/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
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


#ifndef SWIG
#include <type_traits>

/**
 * Check if the type of aObject is T.
 *
 * @param aObject the object for type check.
 * @return true, if aObject type equals T.
 */
template <class T, class I>
bool IsA( const I* aObject )
{
    return aObject && std::remove_pointer<T>::type::ClassOf( aObject );
}

template <class T, class I>
bool IsA( const I& aObject )
{
    return std::remove_pointer<T>::type::ClassOf( &aObject );
}

/**
 * A lightweight dynamic downcast.
 *
 * Cast \a aObject to type Casted*.  Uses #EDA_ITEM::Type() and #EDA_ITEM::ClassOf() to
 * check if type matches.
 *
 * @param aObject object to be casted.
 * @return down-casted object or NULL if type doesn't match Casted.
 */
template<class Casted, class From>
Casted dyn_cast( From aObject )
{
    if( std::remove_pointer<Casted>::type::ClassOf ( aObject ) )
        return static_cast<Casted>( aObject );

    return nullptr;
}

class EDA_ITEM;

#endif  // SWIG


/**
 * The set of class identification values stored in #EDA_ITEM::m_structType
 */
enum KICAD_T
{
    NOT_USED = -1, ///< the 3d code uses this value

    EOT = 0, ///< search types array terminator (End Of Types)

    TYPE_NOT_INIT = 0,
    PCB_T,
    SCREEN_T, ///< not really an item, used to identify a screen

    // Items in pcb
    PCB_FOOTPRINT_T,        ///< class FOOTPRINT, a footprint
    PCB_PAD_T,              ///< class PAD, a pad in a footprint
    PCB_SHAPE_T,            ///< class PCB_SHAPE, a segment not on copper layers
    PCB_TEXT_T,             ///< class PCB_TEXT, text on a layer
    PCB_FP_TEXT_T,          ///< class FP_TEXT, text in a footprint
    PCB_FP_SHAPE_T,         ///< class FP_SHAPE, a footprint edge
    PCB_FP_ZONE_T,          ///< class ZONE, managed by a footprint
    PCB_TRACE_T,            ///< class PCB_TRACK, a track segment (segment on a copper layer)
    PCB_VIA_T,              ///< class PCB_VIA, a via (like a track segment on a copper layer)
    PCB_ARC_T,              ///< class PCB_ARC, an arc track segment on a copper layer
    PCB_MARKER_T,           ///< class PCB_MARKER, a marker used to show something
    PCB_DIMENSION_T,        ///< class PCB_DIMENSION_BASE: abstract dimension meta-type
    PCB_DIM_ALIGNED_T,      ///< class PCB_DIM_ALIGNED, a linear dimension (graphic item)
    PCB_DIM_LEADER_T,       ///< class PCB_DIM_LEADER, a leader dimension (graphic item)
    PCB_DIM_CENTER_T,       ///< class PCB_DIM_CENTER, a center point marking (graphic item)
    PCB_DIM_ORTHOGONAL_T,   ///< class PCB_DIM_ORTHOGONAL, a linear dimension constrained to x/y
    PCB_TARGET_T,           ///< class PCB_TARGET, a target (graphic item)
    PCB_ZONE_T,             ///< class ZONE, a copper pour area
    PCB_ITEM_LIST_T,        ///< class BOARD_ITEM_LIST, a list of board items
    PCB_NETINFO_T,          ///< class NETINFO_ITEM, a description of a net
    PCB_GROUP_T,            ///< class PCB_GROUP, a set of BOARD_ITEMs

    PCB_LOCATE_STDVIA_T,
    PCB_LOCATE_UVIA_T,
    PCB_LOCATE_BBVIA_T,
    PCB_LOCATE_TEXT_T,
    PCB_LOCATE_GRAPHIC_T,
    PCB_LOCATE_HOLE_T,
    PCB_LOCATE_PTH_T,
    PCB_LOCATE_NPTH_T,
    PCB_LOCATE_BOARD_EDGE_T,

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
    SCH_HIER_LABEL_T,
    SCH_FIELD_T,
    SCH_SYMBOL_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,
    SCH_PIN_T,

    // Be prudent with these types:
    // they should be used only to locate a specific field type among SCH_FIELD_Ts
    // N.B. If you add a type here, be sure to add it below to the BaseType()
    SCH_FIELD_LOCATE_REFERENCE_T,
    SCH_FIELD_LOCATE_VALUE_T,
    SCH_FIELD_LOCATE_FOOTPRINT_T,
    SCH_FIELD_LOCATE_DATASHEET_T,

    // Same for picking wires and buses from SCH_LINE_T items
    SCH_LINE_LOCATE_WIRE_T,
    SCH_LINE_LOCATE_BUS_T,
    SCH_LINE_LOCATE_GRAPHIC_LINE_T,

    // Same for picking labels attached to wires and/or buses
    SCH_LABEL_LOCATE_WIRE_T,
    SCH_LABEL_LOCATE_BUS_T,

    // Same for picking symbols which are power symbols
    SCH_SYMBOL_LOCATE_POWER_T,

    // matches any type
    SCH_LOCATE_ANY_T,

    // General
    SCH_SCREEN_T,

    SCHEMATIC_T,

    /*
     * Draw items in library symbol.
     *
     * The order of these items effects the sort order for items inside the
     * "DRAW/ENDDRAW" section of the symbol definition in a library file.
     * If you add a new draw item, type, please make sure you add it so the
     * sort order is logical.
     */
    LIB_SYMBOL_T,
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
     * For GerbView: item types:
     */
    GERBER_LAYOUT_T,
    GERBER_DRAW_ITEM_T,
    GERBER_IMAGE_T,

    /*
     * For Pl_Editor: item types:
     */
    WSG_LINE_T,
    WSG_RECT_T,
    WSG_POLY_T,
    WSG_TEXT_T,
    WSG_BITMAP_T,
    WSG_PAGE_T,

    // serialized layout used in undo/redo commands
    WS_PROXY_UNDO_ITEM_T,      // serialized layout used in undo/redo commands
    WS_PROXY_UNDO_ITEM_PLUS_T, // serialized layout plus page and title block settings

    /*
     * FOR PROJECT::_ELEMs
     */
    SYMBOL_LIB_TABLE_T,
    FP_LIB_TABLE_T,
    SYMBOL_LIBS_T,
    SEARCH_STACK_T,
    S3D_CACHE_T,

    // End value
    MAX_STRUCT_TYPE_ID
};

/**
 * Return the underlying type of the given type.
 *
 * This is useful for finding the element type given one of the "non-type" types such as
 * SCH_LINE_LOCATE_WIRE_T.
 *
 * @param aType Given type to resolve.
 * @return Base type.
 */
constexpr KICAD_T BaseType( const KICAD_T aType )
{
    switch( aType )
    {
    case SCH_FIELD_LOCATE_REFERENCE_T:
    case SCH_FIELD_LOCATE_VALUE_T:
    case SCH_FIELD_LOCATE_FOOTPRINT_T:
    case SCH_FIELD_LOCATE_DATASHEET_T:
        return SCH_FIELD_T;

    case SCH_LINE_LOCATE_WIRE_T:
    case SCH_LINE_LOCATE_BUS_T:
    case SCH_LINE_LOCATE_GRAPHIC_LINE_T:
        return SCH_LINE_T;

    case SCH_LABEL_LOCATE_WIRE_T:
    case SCH_LABEL_LOCATE_BUS_T:
        return SCH_LABEL_T;

    case SCH_SYMBOL_LOCATE_POWER_T:
        return SCH_SYMBOL_T;

    case PCB_LOCATE_HOLE_T:
    case PCB_LOCATE_PTH_T:
    case PCB_LOCATE_NPTH_T:
        return PCB_LOCATE_HOLE_T;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
        return PCB_DIMENSION_T;

    default:
        return aType;
    }
}

#endif // __KICAD_TYPEINFO_H
