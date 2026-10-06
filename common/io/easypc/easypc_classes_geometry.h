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
 * @file easypc_classes_geometry.h
 * @brief Shapes, vertices and the free-standing drawn items.
 *
 * Coordinates are int32 design units of 100 nm, X right, Y up; angles thousandths of a degree, anticlockwise.  A
 * shape is a vertex list: each segment holds its start point and the arc of the span that leaves it.
 */

#ifndef EASYPC_CLASSES_GEOMETRY_H
#define EASYPC_CLASSES_GEOMETRY_H

#include <cstdint>
#include <vector>

#include <wx/string.h>

#include <io/easypc/easypc_classes_root.h>


namespace EASYPC
{

enum AREA_TYPE : int32_t
{
    AREA_ROUTING = 0,
    AREA_COPPER = 1,
    AREA_COMPONENT = 3
};


/// Area flags; track and via bits restrict routing areas, the pour keepout applies to every area
enum AREA_FLAG : int32_t
{
    AREA_TRACKS_MASK = 0x3, ///< 0 unrestricted, 1 keep in, 2 keep out
    AREA_TRACKS_IN = 0x1,
    AREA_TRACKS_OUT = 0x2,
    AREA_VIAS_MASK = 0xc,
    AREA_VIAS_IN = 0x4,
    AREA_VIAS_OUT = 0x8,
    AREA_POUR_KEEPOUT = 0x100
};


enum TEXT_ALIGNMENT : int32_t
{
    TEXT_ALIGN_LEFT = 0,
    TEXT_ALIGN_RIGHT = 1,
    TEXT_ALIGN_CENTRE = 2
};


/**
 * A vertex.  Below shape format 3 each vertex is followed by its next
 * vertex, written in full at that point, so a shape's vertices nest; the frame stack keeps that off the C stack.
 */
struct SOURCE_SEGMENT : OBJECT
{
    LOAD_TASK Load( ARCHIVE& aAr ) final;

    /// The fields a subclass reads after the next vertex
    virtual LOAD_TASK LoadTail( ARCHIVE& aAr );

    int32_t         X = 0;
    int32_t         Y = 0;
    int32_t         ArcAngle = 0; ///< swept angle, 0 for a straight span
    bool            Anticlockwise = true;
    bool            HasNextRef = false; ///< the stream carried a next vertex (shape format below 3)
    SOURCE_SEGMENT* Next = nullptr;     ///< null on an open shape's terminator
};


/// Also shape segments
struct SOURCE_COMMON_SEGMENT : SOURCE_SEGMENT
{
    LOAD_TASK LoadTail( ARCHIVE& aAr ) override;

    /// The owning shape's parent's format, 0 without one
    int ItemFormat( const ARCHIVE& aAr ) const override;

    OBJECT* Shape = nullptr;
};


/// A vertex list
struct SOURCE_SHAPE : OBJECT
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;
    LOAD_TASK LoadShape( ARCHIVE& aAr, int aFormat );

    /// The last vertex links back to the first (formats 1 and 2) or the closed byte is set (format 3)
    bool IsClosed() const;

    int                  Format = 0;
    std::vector<OBJECT*> Segments; ///< the vertex list as read
    bool                 Closed = false;
    bool                 HasClosed = false;
};


struct SOURCE_COMMON_SHAPE : SOURCE_SHAPE
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;
    int       ItemFormat( const ARCHIVE& aAr ) const override;

    OBJECT* Parent = nullptr;
};


/// Also design cutouts
struct SOURCE_DESIGN_SHAPE : SOURCE_COMMON_SHAPE
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    std::vector<OBJECT*> Cutouts;
};


struct SOURCE_SHAPE_ITEM : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT* Style = nullptr; ///< line style
    bool    Filled = false;
    OBJECT* Shape = nullptr; ///< design shape
};


struct SOURCE_LAYERED_SHAPE_ITEM : SOURCE_SHAPE_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT* Layer = nullptr;
};


struct SOURCE_AREA : SOURCE_LAYERED_SHAPE_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    int32_t  Type = AREA_ROUTING;
    OBJECT*  Net = nullptr;
    OBJECT*  Copper = nullptr; ///< design list of the stored pour
    int32_t  Flags = 5;
    int32_t  Height = 0;
    wxString Name;
};


/// Every drawn shape, pour and dimension part
struct SOURCE_FREE_COPPER : SOURCE_LAYERED_SHAPE_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT* Terminals = nullptr;
    int32_t Type = 0; ///< 0 shape, 1 poured, 2 teardrop, 5 to 7 dimension parts
    int32_t PinNumber = 0;
};


struct SOURCE_TEXT_POSITION : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    /// The text position fields that follow the item fields
    LOAD_TASK LoadRemainder( ARCHIVE& aAr );

    OBJECT* TextStyle = nullptr;
    bool    Mirrored = false;
    POINT32 Position;
    int32_t Rotation = 0;
    OBJECT* Layer = nullptr;
    int32_t Alignment = TEXT_ALIGN_LEFT;
};


struct SOURCE_FREE_TEXT : SOURCE_TEXT_POSITION
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    wxString Text;
};


struct SOURCE_VALUE_POSITION : SOURCE_TEXT_POSITION
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    uint64_t Types = 0;
    bool     Displayed = true;
    wxString Attribute;
};


struct SOURCE_TEXT_INSTANCE : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    OBJECT* Text = nullptr; ///< free text
};


/// Also the start of a dimension
struct SOURCE_GROUP : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    wxString             Name;
    int32_t              Flags = 1; ///< bit 0 tight, bit 1 local
    std::vector<OBJECT*> Members;
};


/// Also placement and symbol name origins
struct SOURCE_ORIGIN : SOURCE_DESIGN_ITEM
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;

    POINT32 Position;
    OBJECT* TextStyle = nullptr;
    OBJECT* Layer = nullptr;
    int32_t Rotation = 0;
    bool    Mirrored = false;
};

} // namespace EASYPC

#endif // EASYPC_CLASSES_GEOMETRY_H
