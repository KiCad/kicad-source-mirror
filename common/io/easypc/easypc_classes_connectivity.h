/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file easypc_classes_connectivity.h
 * @brief Placed instances and connectivity: component, symbol and pad instances, nets, nodes, connections,
 *        tracks, vias, buses and boards.
 */

#ifndef EASYPC_CLASSES_CONNECTIVITY_H
#define EASYPC_CLASSES_CONNECTIVITY_H

#include <cstdint>
#include <memory>
#include <vector>

#include <wx/string.h>

#include <io/easypc/easypc_classes_geometry.h>


namespace EASYPC
{

/// Component instance suppress bits
enum COMPONENT_SUPPRESS : int32_t
{
    SUPPRESS_PARTS_LIST = 0x01,
    SUPPRESS_SCM_PCB_TRANSLATION = 0x20
};


/// Value position and component instance value type bits
enum VALUE_TYPE : uint64_t
{
    VALUE_REFERENCE_NAME = 0x1,
    VALUE_COMPONENT_NAME = 0x2,
    VALUE_PACKAGE_NAME = 0x4,
    VALUE_SYMBOL_NAME = 0x8,
    VALUE_VALUES = 0x10,
    VALUE_PIN_NAME = 0x20,
    VALUE_PIN_NUMBER = 0x40,
    VALUE_DESCRIPTION = 0x8000,
    VALUE_ATTRIBUTE = 0x10000, ///< the attribute the value position names
    VALUE_NET_NAME = 0x80000
};


/// A placed footprint (gate -1) or a placed schematic gate
struct SOURCE_SYMBOL_INSTANCE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    POINT32 Position;
    OBJECT* Symbol = nullptr;
    OBJECT* Coppers = nullptr;
    OBJECT* Pads = nullptr;
    OBJECT* Texts = nullptr;
    OBJECT* ValuePositions = nullptr; ///< below 10003 built from the stored symbol name
    int32_t Angle = 0;
    int32_t Gate = -1;
    bool    Mirrored = false; ///< PCB bottom side
    bool    Placed = false;

    /// Below 10003 the stored symbol name becomes the first value position of a new list
    std::unique_ptr<SOURCE_DESIGN_LIST>    ConvertedValuePositions;
    std::unique_ptr<SOURCE_VALUE_POSITION> ConvertedSymbolName;
};


/// A placed component
struct SOURCE_COMPONENT_INSTANCE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT*  Component = nullptr;
    wxString Name; ///< reference designator
    OBJECT*  SymbolInstances = nullptr;
    uint64_t ValueTypes = VALUE_REFERENCE_NAME | VALUE_VALUES;
    int32_t  Suppress = 0;
};


/// A placed pad
struct SOURCE_PAD_INSTANCE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT* Node = nullptr;           ///< null when unconnected
    OBJECT* Pad = nullptr;            ///< the definition's free pad
    OBJECT* PinName = nullptr;        ///< placed pin name, below 10003
    OBJECT* PinNumber = nullptr;      ///< placed pin number, below 10003
    OBJECT* ValuePositions = nullptr; ///< from 10003
    OBJECT* StyleException = nullptr; ///< pad style overriding the pad's own
};


struct SOURCE_COPPER_INSTANCE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT* Copper = nullptr; ///< the definition's free copper
};


struct SOURCE_NET : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    wxString Name;
    OBJECT*  Connections = nullptr;
    OBJECT*  Nodes = nullptr;
    OBJECT*  NetClass = nullptr; ///< the design's first class when the file stores none
    int32_t  GuardSpacing = 0;
};


/// A schematic net label owned by a node
struct SOURCE_NET_NAME : SOURCE_TEXT_POSITION
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    bool Displayed = true;
};


struct SOURCE_NODE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT* Item = nullptr; ///< pad instance, via, junction, copper or bus terminal, or free pad
    OBJECT* NetName = nullptr;
    POINT32 Position;
};


struct SOURCE_CONNECTION : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT* Node1 = nullptr;
    OBJECT* Node2 = nullptr;
    OBJECT* Track = nullptr; ///< null for an unrouted connection
};


/// Also the whole body of junctions and copper terminals
struct SOURCE_CONNECT_POINT : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    POINT32 Position;
    OBJECT* Style = nullptr; ///< pad style
    OBJECT* Node = nullptr;
    int32_t Angle = 0;
};


struct SOURCE_JUNCTION : SOURCE_CONNECT_POINT
{
};


struct SOURCE_COPPER_TERMINAL : SOURCE_CONNECT_POINT
{
};


struct SOURCE_VIA : SOURCE_CONNECT_POINT
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT* LayerSpan = nullptr;
    bool    Tented = false;
};


struct SOURCE_BUS_TERMINAL : SOURCE_CONNECT_POINT
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    POINT32 BusEndPosition;
};


struct SOURCE_TRACK : SOURCE_COMMON_SHAPE
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT* Layer = nullptr; ///< null in a schematic
};


struct SOURCE_TRACK_SEGMENT : SOURCE_COMMON_SEGMENT
{
    LOAD_TASK LoadTail( ARCHIVE& aAr ) override;

    OBJECT* Style = nullptr; ///< track style of the span leaving this vertex
};


struct SOURCE_BUS : SOURCE_SHAPE_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    wxString             Name;
    std::vector<OBJECT*> Nets;
    OBJECT*              Terminals = nullptr;
    OBJECT*              ValuePositions = nullptr;
};


struct SOURCE_BOARD : SOURCE_SHAPE_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    int32_t Plated = 0; ///< bit 0 plated
};

} // namespace EASYPC

#endif // EASYPC_CLASSES_CONNECTIVITY_H
