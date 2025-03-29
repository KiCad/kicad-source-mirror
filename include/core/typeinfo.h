/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

    TYPE_NOT_INIT = 0,
    PCB_T,
    SCREEN_T, ///< not really an item, used to identify a screen

    // Items in pcb
    PCB_FOOTPRINT_T,         ///< class FOOTPRINT, a footprint
    PCB_PAD_T,               ///< class PAD, a pad in a footprint
    PCB_SHAPE_T,             ///< class PCB_SHAPE, a segment not on copper layers
    PCB_REFERENCE_IMAGE_T,   ///< class PCB_REFERENCE_IMAGE, bitmap on a layer
    PCB_FIELD_T,             ///< class PCB_FIELD, text associated with a footprint property
    PCB_GENERATOR_T,         ///< class PCB_GENERATOR, generator on a layer
    PCB_TEXT_T,              ///< class PCB_TEXT, text on a layer
    PCB_TEXTBOX_T,           ///< class PCB_TEXTBOX, wrapped text on a layer
    PCB_TABLE_T,             ///< class PCB_TABLE, table of PCB_TABLECELLs
    PCB_TABLECELL_T,         ///< class PCB_TABLECELL, PCB_TEXTBOX for use in tables
    PCB_TRACE_T,             ///< class PCB_TRACK, a track segment (segment on a copper layer)
    PCB_VIA_T,               ///< class PCB_VIA, a via (like a track segment on a copper layer)
    PCB_ARC_T,               ///< class PCB_ARC, an arc track segment on a copper layer
    PCB_MARKER_T,            ///< class PCB_MARKER, a marker used to show something
    PCB_DIMENSION_T,         ///< class PCB_DIMENSION_BASE: abstract dimension meta-type
    PCB_DIM_ALIGNED_T,       ///< class PCB_DIM_ALIGNED, a linear dimension (graphic item)
    PCB_DIM_LEADER_T,        ///< class PCB_DIM_LEADER, a leader dimension (graphic item)
    PCB_DIM_CENTER_T,        ///< class PCB_DIM_CENTER, a center point marking (graphic item)
    PCB_DIM_RADIAL_T,        ///< class PCB_DIM_RADIAL, a radius or diameter dimension
    PCB_DIM_ORTHOGONAL_T,    ///< class PCB_DIM_ORTHOGONAL, a linear dimension constrained to x/y
    PCB_TARGET_T,            ///< class PCB_TARGET, a target (graphic item)
    PCB_ZONE_T,              ///< class ZONE, a copper pour area
    PCB_ITEM_LIST_T,         ///< class BOARD_ITEM_LIST, a list of board items
    PCB_NETINFO_T,           ///< class NETINFO_ITEM, a description of a net
    PCB_GROUP_T,             ///< class PCB_GROUP, a set of BOARD_ITEMs
    PCB_BOARD_OUTLINE_T,     ///< class PCB_BOARD_OUTLINE_T, a pcb board outline item
    PCB_POINT_T,             ///< class PCB_POINT, a 0-dimensional point

    // Be prudent with these types:
    // they should be used only to locate a specific field type among PCB_FIELD_Ts
    // N.B. If you add a type here, be sure to add it below to the BaseType()
    PCB_FIELD_LOCATE_REFERENCE_T,
    PCB_FIELD_LOCATE_VALUE_T,
    PCB_FIELD_LOCATE_FOOTPRINT_T,
    PCB_FIELD_LOCATE_DATASHEET_T,

    // Be prudent with these types:
    // they should be used only to locate specific item sub-types
    // N.B. If you add a type here, be sure to add it below to the BaseType()
    PCB_LOCATE_STDVIA_T,
    PCB_LOCATE_UVIA_T,
    PCB_LOCATE_BBVIA_T,
    PCB_LOCATE_TEXT_T,
    PCB_LOCATE_HOLE_T,
    PCB_LOCATE_PTH_T,
    PCB_LOCATE_NPTH_T,
    PCB_LOCATE_BOARD_EDGE_T,

    // Same for locating shapes types from PCB_SHAPE_T items
    PCB_SHAPE_LOCATE_SEGMENT_T,
    PCB_SHAPE_LOCATE_RECT_T,
    PCB_SHAPE_LOCATE_CIRCLE_T,
    PCB_SHAPE_LOCATE_ARC_T,
    PCB_SHAPE_LOCATE_POLY_T,
    PCB_SHAPE_LOCATE_BEZIER_T,

    /*
     * Draw items in library symbol.
     *
     * The order of these items effects the sort order for items inside the
     * "DRAW/ENDDRAW" section of the symbol definition in a library file.
     * If you add a new draw item, type, please make sure you add it so the
     * sort order is logical.
     */
    LIB_SYMBOL_T,
    SCH_SHAPE_T,
    SCH_FIELD_T,
    SCH_TEXT_T,
    SCH_TEXTBOX_T,
    SCH_PIN_T,

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
    SCH_TABLE_T,
    SCH_TABLECELL_T,
    SCH_LABEL_T,
    SCH_GLOBAL_LABEL_T,
    SCH_HIER_LABEL_T,
    SCH_RULE_AREA_T,
    SCH_DIRECTIVE_LABEL_T,
    SCH_SYMBOL_T,
    SCH_GROUP_T,
    SCH_SHEET_PIN_T,
    SCH_SHEET_T,

    // Be prudent with these types:
    // they should be used only to locate a specific field type among SCH_FIELD_Ts
    // N.B. If you add a type here, be sure to add it below to the BaseType()
    SCH_FIELD_LOCATE_REFERENCE_T,
    SCH_FIELD_LOCATE_VALUE_T,
    SCH_FIELD_LOCATE_FOOTPRINT_T,
    SCH_FIELD_LOCATE_DATASHEET_T,

    // Same for picking wires, buses and graphics from SCH_ITEM_T items
    SCH_ITEM_LOCATE_WIRE_T,
    SCH_ITEM_LOCATE_BUS_T,
    SCH_ITEM_LOCATE_GRAPHIC_LINE_T,

    // Same for picking labels, or labels attached to wires and/or buses
    SCH_LABEL_LOCATE_ANY_T,
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
    DESIGN_BLOCK_LIB_TABLE_T,
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
 * SCH_ITEM_LOCATE_WIRE_T.
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

    case SCH_ITEM_LOCATE_WIRE_T:
    case SCH_ITEM_LOCATE_BUS_T:
    case SCH_ITEM_LOCATE_GRAPHIC_LINE_T:
        return SCH_LINE_T;

    case SCH_LABEL_LOCATE_ANY_T:
    case SCH_LABEL_LOCATE_WIRE_T:
    case SCH_LABEL_LOCATE_BUS_T:
        return SCH_LABEL_T;

    case SCH_SYMBOL_LOCATE_POWER_T:
        return SCH_SYMBOL_T;

    case PCB_FIELD_LOCATE_REFERENCE_T:
    case PCB_FIELD_LOCATE_VALUE_T:
    case PCB_FIELD_LOCATE_FOOTPRINT_T:
    case PCB_FIELD_LOCATE_DATASHEET_T:
        return PCB_FIELD_T;

    case PCB_LOCATE_HOLE_T:
    case PCB_LOCATE_PTH_T:
    case PCB_LOCATE_NPTH_T:
        return PCB_LOCATE_HOLE_T;

    case PCB_SHAPE_LOCATE_SEGMENT_T:
    case PCB_SHAPE_LOCATE_RECT_T:
    case PCB_SHAPE_LOCATE_CIRCLE_T:
    case PCB_SHAPE_LOCATE_ARC_T:
    case PCB_SHAPE_LOCATE_POLY_T:
    case PCB_SHAPE_LOCATE_BEZIER_T:
        return PCB_SHAPE_T;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
        return PCB_DIMENSION_T;

    default:
        return aType;
    }
}

constexpr bool IsNullType( const KICAD_T aType )
{
    return aType <= 0;
}

constexpr bool IsInstantiableType( const KICAD_T aType )
{
    if( IsNullType( aType ) )
        return false;

    switch( aType )
    {
    case SCH_LOCATE_ANY_T:

    case SCH_FIELD_LOCATE_REFERENCE_T:
    case SCH_FIELD_LOCATE_VALUE_T:
    case SCH_FIELD_LOCATE_FOOTPRINT_T:
    case SCH_FIELD_LOCATE_DATASHEET_T:

    case SCH_ITEM_LOCATE_WIRE_T:
    case SCH_ITEM_LOCATE_BUS_T:
    case SCH_ITEM_LOCATE_GRAPHIC_LINE_T:

    case SCH_LABEL_LOCATE_ANY_T:
    case SCH_LABEL_LOCATE_WIRE_T:
    case SCH_LABEL_LOCATE_BUS_T:

    case SCH_SYMBOL_LOCATE_POWER_T:

    case PCB_FIELD_LOCATE_REFERENCE_T:
    case PCB_FIELD_LOCATE_VALUE_T:
    case PCB_FIELD_LOCATE_FOOTPRINT_T:
    case PCB_FIELD_LOCATE_DATASHEET_T:

    case PCB_LOCATE_STDVIA_T:
    case PCB_LOCATE_UVIA_T:
    case PCB_LOCATE_BBVIA_T:
    case PCB_LOCATE_TEXT_T:
    case PCB_LOCATE_HOLE_T:
    case PCB_LOCATE_PTH_T:
    case PCB_LOCATE_NPTH_T:
    case PCB_LOCATE_BOARD_EDGE_T:

    case PCB_SHAPE_LOCATE_SEGMENT_T:
    case PCB_SHAPE_LOCATE_RECT_T:
    case PCB_SHAPE_LOCATE_CIRCLE_T:
    case PCB_SHAPE_LOCATE_ARC_T:
    case PCB_SHAPE_LOCATE_POLY_T:
    case PCB_SHAPE_LOCATE_BEZIER_T:

    case PCB_DIMENSION_T:

    case SCH_SCREEN_T:
    case PCB_ITEM_LIST_T:
        return false;

    default:
        break;
    }

    return true;

}

constexpr bool IsEeschemaType( const KICAD_T aType )
{
    switch( aType )
    {
    case SCH_MARKER_T:
    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_BUS_BUS_ENTRY_T:
    case SCH_LINE_T:
    case SCH_SHAPE_T:
    case SCH_RULE_AREA_T:
    case SCH_BITMAP_T:
    case SCH_TEXT_T:
    case SCH_TEXTBOX_T:
    case SCH_TABLE_T:
    case SCH_TABLECELL_T:
    case SCH_LABEL_T:
    case SCH_DIRECTIVE_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_FIELD_T:
    case SCH_SYMBOL_T:
    case SCH_SHEET_PIN_T:
    case SCH_GROUP_T:
    case SCH_SHEET_T:
    case SCH_PIN_T:

    case SCH_FIELD_LOCATE_REFERENCE_T:
    case SCH_FIELD_LOCATE_VALUE_T:
    case SCH_FIELD_LOCATE_FOOTPRINT_T:
    case SCH_FIELD_LOCATE_DATASHEET_T:

    case SCH_ITEM_LOCATE_WIRE_T:
    case SCH_ITEM_LOCATE_BUS_T:
    case SCH_ITEM_LOCATE_GRAPHIC_LINE_T:

    case SCH_LABEL_LOCATE_ANY_T:
    case SCH_LABEL_LOCATE_WIRE_T:
    case SCH_LABEL_LOCATE_BUS_T:

    case SCH_SYMBOL_LOCATE_POWER_T:
    case SCH_LOCATE_ANY_T:

    case SCH_SCREEN_T:
    case SCHEMATIC_T:

    case LIB_SYMBOL_T:
        return true;

    default:
        return false;
    }
}

constexpr bool IsPcbnewType( const KICAD_T aType )
{
    switch( aType )
    {
    case PCB_T:

    case PCB_FOOTPRINT_T:
    case PCB_PAD_T:
    case PCB_SHAPE_T:
    case PCB_REFERENCE_IMAGE_T:
    case PCB_FIELD_T:
    case PCB_TEXT_T:
    case PCB_TEXTBOX_T:
    case PCB_TABLE_T:
    case PCB_TABLECELL_T:
    case PCB_TRACE_T:
    case PCB_VIA_T:
    case PCB_ARC_T:
    case PCB_MARKER_T:
    case PCB_DIMENSION_T:
    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_LEADER_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_TARGET_T:
    case PCB_POINT_T:
    case PCB_ZONE_T:
    case PCB_ITEM_LIST_T:
    case PCB_NETINFO_T:
    case PCB_GROUP_T:
    case PCB_GENERATOR_T:

    case PCB_FIELD_LOCATE_REFERENCE_T:
    case PCB_FIELD_LOCATE_VALUE_T:
    case PCB_FIELD_LOCATE_FOOTPRINT_T:
    case PCB_FIELD_LOCATE_DATASHEET_T:
    case PCB_LOCATE_STDVIA_T:
    case PCB_LOCATE_UVIA_T:
    case PCB_LOCATE_BBVIA_T:
    case PCB_LOCATE_TEXT_T:
    case PCB_LOCATE_HOLE_T:
    case PCB_LOCATE_PTH_T:
    case PCB_LOCATE_NPTH_T:
    case PCB_LOCATE_BOARD_EDGE_T:
    case PCB_SHAPE_LOCATE_SEGMENT_T:
    case PCB_SHAPE_LOCATE_RECT_T:
    case PCB_SHAPE_LOCATE_CIRCLE_T:
    case PCB_SHAPE_LOCATE_ARC_T:
    case PCB_SHAPE_LOCATE_POLY_T:
    case PCB_SHAPE_LOCATE_BEZIER_T:
    case PCB_BOARD_OUTLINE_T:
        return true;

    default:
        return false;
    }
}

constexpr bool IsGerbviewType( const KICAD_T aType )
{
    switch( aType )
    {
    case GERBER_LAYOUT_T:
    case GERBER_DRAW_ITEM_T:
    case GERBER_IMAGE_T:
        return true;

    default:
        return false;
    }
}

constexpr bool IsPageLayoutEditorType( const KICAD_T aType )
{
    switch( aType )
    {
    case WSG_LINE_T:
    case WSG_RECT_T:
    case WSG_POLY_T:
    case WSG_TEXT_T:
    case WSG_BITMAP_T:
    case WSG_PAGE_T:

    case WS_PROXY_UNDO_ITEM_T:
    case WS_PROXY_UNDO_ITEM_PLUS_T:
        return true;

    default:
        return false;
    }
}

constexpr bool IsMiscType( const KICAD_T aType )
{
    switch( aType )
    {
    case SCREEN_T:

    case SYMBOL_LIB_TABLE_T:
    case FP_LIB_TABLE_T:
    case DESIGN_BLOCK_LIB_TABLE_T:
    case SYMBOL_LIBS_T:
    case SEARCH_STACK_T:
    case S3D_CACHE_T:
        return true;

    default:
        return false;
    }
}

constexpr bool IsTypeCorrect( KICAD_T aType )
{
    return IsNullType( aType )
        || IsEeschemaType( aType )
        || IsPcbnewType( aType )
        || IsGerbviewType( aType )
        || IsPageLayoutEditorType( aType )
        || IsMiscType( aType );
}

#endif // __KICAD_TYPEINFO_H
