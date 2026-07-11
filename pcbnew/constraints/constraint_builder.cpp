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

#include <constraints/constraint_builder.h>

#include <cmath>

#include <board.h>
#include <footprint.h>
#include <geometry/eda_angle.h>
#include <pcb_shape.h>


namespace
{
bool isSegment( const BOARD_ITEM* aItem )
{
    return aItem->Type() == PCB_SHAPE_T
            && static_cast<const PCB_SHAPE*>( aItem )->GetShape() == SHAPE_T::SEGMENT;
}


bool isCircle( const BOARD_ITEM* aItem )
{
    return aItem->Type() == PCB_SHAPE_T
            && static_cast<const PCB_SHAPE*>( aItem )->GetShape() == SHAPE_T::CIRCLE;
}


bool allSegments( const std::vector<BOARD_ITEM*>& aItems )
{
    for( const BOARD_ITEM* item : aItems )
    {
        if( !isSegment( item ) )
            return false;
    }

    return true;
}


bool allCircles( const std::vector<BOARD_ITEM*>& aItems )
{
    for( const BOARD_ITEM* item : aItems )
    {
        if( !isCircle( item ) )
            return false;
    }

    return true;
}
}


std::unique_ptr<PCB_CONSTRAINT> BuildConstraintFromItems( BOARD_ITEM* aParent,
                                                          PCB_CONSTRAINT_TYPE aType,
                                                          const std::vector<BOARD_ITEM*>& aItems )
{
    auto make = [&]() { return std::make_unique<PCB_CONSTRAINT>( aParent, aType ); };

    auto addWhole = [&]( PCB_CONSTRAINT* aConstraint )
    {
        for( BOARD_ITEM* item : aItems )
            aConstraint->AddMember( item->m_Uuid, CONSTRAINT_ANCHOR::WHOLE );
    };

    switch( aType )
    {
    case PCB_CONSTRAINT_TYPE::PARALLEL:
    case PCB_CONSTRAINT_TYPE::PERPENDICULAR:
    case PCB_CONSTRAINT_TYPE::EQUAL_LENGTH:
    case PCB_CONSTRAINT_TYPE::COLLINEAR:
    {
        if( aItems.size() != 2 || !allSegments( aItems ) )
            return nullptr;

        std::unique_ptr<PCB_CONSTRAINT> c = make();
        addWhole( c.get() );
        return c;
    }

    case PCB_CONSTRAINT_TYPE::HORIZONTAL:
    case PCB_CONSTRAINT_TYPE::VERTICAL:
    {
        if( aItems.size() != 1 || !isSegment( aItems[0] ) )
            return nullptr;

        std::unique_ptr<PCB_CONSTRAINT> c = make();
        addWhole( c.get() );
        return c;
    }

    case PCB_CONSTRAINT_TYPE::FIXED_LENGTH:
    {
        if( aItems.size() != 1 || !isSegment( aItems[0] ) )
            return nullptr;

        const PCB_SHAPE* seg = static_cast<const PCB_SHAPE*>( aItems[0] );

        std::unique_ptr<PCB_CONSTRAINT> c = make();
        addWhole( c.get() );
        c->SetValue( ( seg->GetEnd() - seg->GetStart() ).EuclideanNorm() );
        return c;
    }

    case PCB_CONSTRAINT_TYPE::CONCENTRIC:
    case PCB_CONSTRAINT_TYPE::EQUAL_RADIUS:
    {
        if( aItems.size() != 2 || !allCircles( aItems ) )
            return nullptr;

        std::unique_ptr<PCB_CONSTRAINT> c = make();
        addWhole( c.get() );
        return c;
    }

    case PCB_CONSTRAINT_TYPE::ANGULAR_DIMENSION:
    {
        if( aItems.size() != 2 || !allSegments( aItems ) )
            return nullptr;

        const PCB_SHAPE* a = static_cast<const PCB_SHAPE*>( aItems[0] );
        const PCB_SHAPE* b = static_cast<const PCB_SHAPE*>( aItems[1] );

        EDA_ANGLE angleA( a->GetEnd() - a->GetStart() );
        EDA_ANGLE angleB( b->GetEnd() - b->GetStart() );

        std::unique_ptr<PCB_CONSTRAINT> c = make();
        addWhole( c.get() );

        // Store the signed directed angle (member[0] -> member[1]); the solver constrains the same
        // directed angle, so abs() here would snap the lines to the mirror configuration on solve.
        c->SetValue( ( angleB - angleA ).Normalize180().AsDegrees() );
        return c;
    }

    case PCB_CONSTRAINT_TYPE::FIXED_RADIUS:
    {
        if( aItems.size() != 1 || !isCircle( aItems[0] ) )
            return nullptr;

        std::unique_ptr<PCB_CONSTRAINT> c = make();
        addWhole( c.get() );
        c->SetValue( static_cast<const PCB_SHAPE*>( aItems[0] )->GetRadius() );
        return c;
    }

    default:
        // Point-anchored families (coincident, midpoint, symmetric, ...) need point selection,
        // which the whole-shape authoring tool does not yet provide.
        return nullptr;
    }
}


std::vector<CONSTRAINT_ANCHOR_POINT> ConstraintShapeAnchors( const PCB_SHAPE* aShape )
{
    std::vector<CONSTRAINT_ANCHOR_POINT> anchors;

    if( !aShape )
        return anchors;

    switch( aShape->GetShape() )
    {
    case SHAPE_T::SEGMENT:
        anchors.push_back( { CONSTRAINT_ANCHOR::START, aShape->GetStart() } );
        anchors.push_back( { CONSTRAINT_ANCHOR::END, aShape->GetEnd() } );
        break;

    case SHAPE_T::ARC:
        anchors.push_back( { CONSTRAINT_ANCHOR::START, aShape->GetStart() } );
        anchors.push_back( { CONSTRAINT_ANCHOR::END, aShape->GetEnd() } );
        anchors.push_back( { CONSTRAINT_ANCHOR::CENTER, aShape->GetCenter() } );
        break;

    case SHAPE_T::CIRCLE:
        anchors.push_back( { CONSTRAINT_ANCHOR::CENTER, aShape->GetCenter() } );
        break;

    default:
        break;
    }

    return anchors;
}


std::optional<CONSTRAINT_MEMBER> NearestAnchorAmong( const std::vector<PCB_SHAPE*>& aShapes,
                                                     const VECTOR2I& aPos, double aMaxDist )
{
    double                           best = aMaxDist;
    std::optional<CONSTRAINT_MEMBER> result;

    for( const PCB_SHAPE* shape : aShapes )
    {
        for( const CONSTRAINT_ANCHOR_POINT& a : ConstraintShapeAnchors( shape ) )
        {
            double dist = ( a.pos - aPos ).EuclideanNorm();

            if( dist <= best )
            {
                best = dist;
                result = CONSTRAINT_MEMBER( shape->m_Uuid, a.anchor );
            }
        }
    }

    return result;
}


std::vector<PCB_SHAPE*> CollectConstraintShapes( BOARD* aBoard )
{
    std::vector<PCB_SHAPE*> shapes;

    if( !aBoard )
        return shapes;

    for( BOARD_ITEM* item : aBoard->Drawings() )
    {
        if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
            shapes.push_back( shape );
    }

    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        for( BOARD_ITEM* item : footprint->GraphicalItems() )
        {
            if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( item ) )
                shapes.push_back( shape );
        }
    }

    return shapes;
}


std::optional<CONSTRAINT_MEMBER> NearestConstraintAnchor( BOARD* aBoard, const VECTOR2I& aPos,
                                                          double aMaxDist )
{
    return NearestAnchorAmong( CollectConstraintShapes( aBoard ), aPos, aMaxDist );
}


std::optional<VECTOR2I> ConstraintAnchorPosition( BOARD* aBoard, const CONSTRAINT_MEMBER& aMember )
{
    if( !aBoard )
        return std::nullopt;

    PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aBoard->ResolveItem( aMember.m_item, true ) );

    if( !shape )
        return std::nullopt;

    for( const CONSTRAINT_ANCHOR_POINT& a : ConstraintShapeAnchors( shape ) )
    {
        if( a.anchor == aMember.m_anchor )
            return a.pos;
    }

    return std::nullopt;
}
