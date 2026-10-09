/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <optional>

#include <board_item.h>
#include <geometry/shape_poly_set.h>
#include <pcb_shape.h>
#include <pcb_text.h>

namespace KICAD_FORMAT::LEGACY
{

/// The released comparators ordered footprint graphics in board coordinates. The current one uses
/// library coordinates, which sorts a flipped or rotated footprint's items differently, so the
/// frozen writers need the target's own ordering to reproduce its output.
inline std::optional<bool> orderPoints( const VECTOR2I& aFirst, const VECTOR2I& aSecond )
{
    if( aFirst.x != aSecond.x )
        return aFirst.x < aSecond.x;

    if( aFirst.y != aSecond.y )
        return aFirst.y < aSecond.y;

    return std::nullopt;
}


inline std::optional<bool> orderFpShapes( const PCB_SHAPE* aFirst, const PCB_SHAPE* aSecond )
{
    if( aFirst->GetShape() != aSecond->GetShape() )
        return aFirst->GetShape() < aSecond->GetShape();

    // Start and end have no meaning for a polygon, so they cannot order one.
    if( aFirst->GetShape() != SHAPE_T::POLY )
    {
        if( std::optional<bool> cmp = orderPoints( aFirst->GetStart(), aSecond->GetStart() ) )
            return cmp;

        if( std::optional<bool> cmp = orderPoints( aFirst->GetEnd(), aSecond->GetEnd() ) )
            return cmp;
    }

    if( aFirst->GetShape() == SHAPE_T::ARC )
    {
        if( std::optional<bool> cmp = orderPoints( aFirst->GetCenter(), aSecond->GetCenter() ) )
            return cmp;
    }
    else if( aFirst->GetShape() == SHAPE_T::BEZIER )
    {
        if( std::optional<bool> cmp = orderPoints( aFirst->GetBezierC1(), aSecond->GetBezierC1() ) )
            return cmp;

        if( std::optional<bool> cmp = orderPoints( aFirst->GetBezierC2(), aSecond->GetBezierC2() ) )
            return cmp;
    }
    else if( aFirst->GetShape() == SHAPE_T::POLY )
    {
        const SHAPE_POLY_SET& polyA = aFirst->GetPolyShape();
        const SHAPE_POLY_SET& polyB = aSecond->GetPolyShape();

        if( polyA.TotalVertices() != polyB.TotalVertices() )
            return polyA.TotalVertices() < polyB.TotalVertices();

        for( int ii = 0; ii < polyA.TotalVertices(); ++ii )
        {
            if( std::optional<bool> cmp = orderPoints( polyA.CVertex( ii ), polyB.CVertex( ii ) ) )
                return cmp;
        }
    }

    if( aFirst->GetWidth() != aSecond->GetWidth() )
        return aFirst->GetWidth() < aSecond->GetWidth();

    return std::nullopt;
}


inline std::optional<bool> orderFpTexts( const PCB_TEXT* aFirst, const PCB_TEXT* aSecond )
{
    if( std::optional<bool> cmp = orderPoints( aFirst->GetPosition(), aSecond->GetPosition() ) )
        return cmp;

    if( aFirst->GetTextAngle() != aSecond->GetTextAngle() )
        return aFirst->GetTextAngle() < aSecond->GetTextAngle();

    if( std::optional<bool> cmp = orderPoints( aFirst->GetTextSize(), aSecond->GetTextSize() ) )
        return cmp;

    if( aFirst->GetTextThickness() != aSecond->GetTextThickness() )
        return aFirst->GetTextThickness() < aSecond->GetTextThickness();

    if( aFirst->IsBold() != aSecond->IsBold() )
        return aFirst->IsBold() < aSecond->IsBold();

    if( aFirst->IsItalic() != aSecond->IsItalic() )
        return aFirst->IsItalic() < aSecond->IsItalic();

    if( aFirst->IsMirrored() != aSecond->IsMirrored() )
        return aFirst->IsMirrored() < aSecond->IsMirrored();

    if( aFirst->GetLineSpacing() != aSecond->GetLineSpacing() )
        return aFirst->GetLineSpacing() < aSecond->GetLineSpacing();

    if( aFirst->GetText() != aSecond->GetText() )
        return aFirst->GetText().Cmp( aSecond->GetText() ) < 0;

    return std::nullopt;
}


/// Every branch falls through to the uuid. A branch that reported two distinct items equal would
/// lose one of them, because the writers keep these in a std::set.
template <bool OrderTextByPosition>
struct FP_DRAWING_ORDER
{
    bool operator()( const BOARD_ITEM* aFirst, const BOARD_ITEM* aSecond ) const
    {
        if( aFirst->Type() != aSecond->Type() )
            return aFirst->Type() < aSecond->Type();

        if( aFirst->GetLayer() != aSecond->GetLayer() )
            return aFirst->GetLayer() < aSecond->GetLayer();

        if( aFirst->Type() == PCB_SHAPE_T )
        {
            if( std::optional<bool> cmp = orderFpShapes( static_cast<const PCB_SHAPE*>( aFirst ),
                                                         static_cast<const PCB_SHAPE*>( aSecond ) ) )
            {
                return *cmp;
            }
        }
        else if( OrderTextByPosition && aFirst->Type() == PCB_TEXT_T )
        {
            if( std::optional<bool> cmp = orderFpTexts( static_cast<const PCB_TEXT*>( aFirst ),
                                                        static_cast<const PCB_TEXT*>( aSecond ) ) )
            {
                return *cmp;
            }
        }

        if( aFirst->m_Uuid != aSecond->m_Uuid )
            return aFirst->m_Uuid < aSecond->m_Uuid;

        return aFirst < aSecond;
    }
};

/// KiCad 9 ordered only shapes by geometry. KiCad 10 also ordered text by position.
using FP_DRAWING_ORDER_V9 = FP_DRAWING_ORDER<false>;
using FP_DRAWING_ORDER_V10 = FP_DRAWING_ORDER<true>;

} // namespace KICAD_FORMAT::LEGACY
