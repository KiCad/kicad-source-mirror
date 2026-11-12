/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "sch_io_wire_detour.h"

#include <algorithm>

#include <base_units.h>

namespace
{
bool strictlyInside( const SEG& aSeg, const VECTOR2I& aPt )
{
    return aPt != aSeg.A && aPt != aSeg.B && aSeg.Contains( aPt );
}
} // namespace


void SCH_IO_WIRE_DETOUR::AddWire( const SEG& aWire, int aNet )
{
    m_wires.push_back( { aWire, aNet } );
}


void SCH_IO_WIRE_DETOUR::AddContact( const VECTOR2I& aPos, int aNet )
{
    m_contacts.push_back( { aPos, aNet } );
}


std::vector<SCH_IO_WIRE_DETOUR::CONTACT> SCH_IO_WIRE_DETOUR::contacts() const
{
    std::vector<CONTACT> all = m_contacts;

    for( const WIRE& wire : m_wires )
    {
        all.push_back( { wire.Seg.A, wire.Net } );
        all.push_back( { wire.Seg.B, wire.Net } );
    }

    return all;
}


bool SCH_IO_WIRE_DETOUR::TouchesOtherNet( const VECTOR2I& aPos, int aNet ) const
{
    for( const CONTACT& contact : contacts() )
    {
        if( contact.Net != aNet && contact.Pos == aPos )
            return true;
    }

    return false;
}


bool SCH_IO_WIRE_DETOUR::joinsOtherNet( const SEG& aPiece, const SEG& aWire, int aNet,
                                        const std::vector<CONTACT>& aContacts ) const
{
    // Whatever the wire already touches at its own ends is not made worse by a detour
    auto atWireEnd = [&]( const VECTOR2I& aPt )
    {
        return aPt == aWire.A || aPt == aWire.B;
    };

    for( const CONTACT& contact : aContacts )
    {
        if( contact.Net != aNet && !atWireEnd( contact.Pos ) && aPiece.Contains( contact.Pos ) )
            return true;
    }

    for( const WIRE& wire : m_wires )
    {
        if( wire.Net == aNet )
            continue;

        if( ( !atWireEnd( aPiece.A ) && wire.Seg.Contains( aPiece.A ) )
            || ( !atWireEnd( aPiece.B ) && wire.Seg.Contains( aPiece.B ) ) )
        {
            return true;
        }
    }

    return false;
}


std::vector<SEG> SCH_IO_WIRE_DETOUR::detour( const SEG& aWire, const VECTOR2I& aEnter, const VECTOR2I& aLeave,
                                             int aNet, const std::vector<CONTACT>& aContacts ) const
{
    const int      step = schIUScale.MilsToIU( 50 );
    const VECTOR2I normal = ( aWire.B - aWire.A ).Perpendicular().Resize( step );
    const SEG      replaced( aEnter, aLeave );

    // A branch of this net on the replaced stretch would be left hanging
    for( const CONTACT& contact : aContacts )
    {
        if( contact.Net == aNet && strictlyInside( replaced, contact.Pos ) )
            return {};
    }

    std::vector<SEG> best;
    int              bestCrossings = 0;

    for( int mult : { 1, -1, 2, -2, 3, -3 } )
    {
        const VECTOR2I enterOff = aEnter + normal * mult;
        const VECTOR2I leaveOff = aLeave + normal * mult;
        const SEG      pieces[] = { SEG( aEnter, enterOff ), SEG( enterOff, leaveOff ), SEG( leaveOff, aLeave ) };

        if( std::any_of( std::begin( pieces ), std::end( pieces ),
                         [&]( const SEG& aPiece )
                         {
                             return joinsOtherNet( aPiece, aWire, aNet, aContacts );
                         } ) )
        {
            continue;
        }

        // Prefer the side where the detour crosses the fewest other wires
        int crossings = 0;

        for( const WIRE& wire : m_wires )
        {
            if( wire.Net == aNet )
                continue;

            for( const SEG& piece : pieces )
            {
                if( piece.Intersect( wire.Seg, true ) )
                    crossings++;
            }
        }

        if( best.empty() || crossings < bestCrossings )
        {
            best.assign( std::begin( pieces ), std::end( pieces ) );
            bestCrossings = crossings;
        }
    }

    return best;
}


SCH_IO_WIRE_DETOUR::RESULT SCH_IO_WIRE_DETOUR::Route( const SEG& aWire, int aNet )
{
    const int step = schIUScale.MilsToIU( 50 );

    RESULT                     result;
    const std::vector<CONTACT> all = contacts();

    std::vector<VECTOR2I> foreign;

    for( const CONTACT& contact : all )
    {
        if( contact.Net != aNet && strictlyInside( aWire, contact.Pos )
            && std::find( foreign.begin(), foreign.end(), contact.Pos ) == foreign.end() )
        {
            foreign.push_back( contact.Pos );
        }
    }

    auto replaceRegistered =
            [&]()
            {
                std::erase_if( m_wires,
                               [&]( const WIRE& aEntry )
                               {
                                   return aEntry.Net == aNet && aEntry.Seg == aWire;
                               } );

                for( const SEG& piece : result.Pieces )
                    AddWire( piece, aNet );
            };

    if( foreign.empty() )
    {
        result.Pieces = { aWire };
        replaceRegistered();
        return result;
    }

    // Anchors on a diagonal cannot be placed exactly on the wire, which would detach contacts from it
    if( aWire.A.x != aWire.B.x && aWire.A.y != aWire.B.y )
    {
        result.Pieces = { aWire };
        result.Failed = true;
        replaceRegistered();
        return result;
    }

    const VECTOR2I along = aWire.B - aWire.A;
    const VECTOR2I stepVec = along.Resize( step );

    std::sort( foreign.begin(), foreign.end(),
               [&]( const VECTOR2I& a, const VECTOR2I& b )
               {
                   return ( a - aWire.A ).SquaredEuclideanNorm() < ( b - aWire.A ).SquaredEuclideanNorm();
               } );

    auto alongWire = [&]( const VECTOR2I& aPt )
    {
        return ( aPt - aWire.A ).Dot( along );
    };

    // A branch of this net joins the wire here, so a detour has to end there rather than pass it
    std::vector<VECTOR2I> branches;

    for( const CONTACT& contact : all )
    {
        if( contact.Net == aNet && strictlyInside( aWire, contact.Pos ) )
            branches.push_back( contact.Pos );
    }

    auto branchBetween = [&]( const VECTOR2I& aFrom, const VECTOR2I& aTo )
    {
        return std::any_of( branches.begin(), branches.end(),
                            [&]( const VECTOR2I& aPt )
                            {
                                return alongWire( aPt ) > alongWire( aFrom ) && alongWire( aPt ) < alongWire( aTo );
                            } );
    };

    // Items close together share one detour
    std::vector<std::pair<VECTOR2I, VECTOR2I>> clusters;

    for( const VECTOR2I& pt : foreign )
    {
        if( !clusters.empty() && ( pt - clusters.back().second ).EuclideanNorm() <= 3 * step
            && !branchBetween( clusters.back().second, pt ) )
        {
            clusters.back().second = pt;
        }
        else
        {
            clusters.emplace_back( pt, pt );
        }
    }

    std::vector<VECTOR2I> path{ aWire.A };

    for( const auto& [first, last] : clusters )
    {
        VECTOR2I enter = first - stepVec;
        VECTOR2I leave = last + stepVec;

        if( alongWire( enter ) < 0 )
            enter = aWire.A;

        if( alongWire( leave ) > alongWire( aWire.B ) )
            leave = aWire.B;

        for( const VECTOR2I& branch : branches )
        {
            if( alongWire( branch ) > alongWire( enter ) && alongWire( branch ) < alongWire( first ) )
                enter = branch;

            if( alongWire( branch ) < alongWire( leave ) && alongWire( branch ) > alongWire( last ) )
                leave = branch;
        }

        std::vector<SEG> pieces = detour( aWire, enter, leave, aNet, all );

        if( pieces.empty() )
        {
            result.Failed = true;
            continue;
        }

        for( const VECTOR2I& pt : { pieces[0].A, pieces[0].B, pieces[1].B, pieces[2].B } )
        {
            if( pt != path.back() )
                path.push_back( pt );
        }
    }

    if( aWire.B != path.back() )
        path.push_back( aWire.B );

    for( size_t i = 1; i < path.size(); i++ )
        result.Pieces.emplace_back( path[i - 1], path[i] );

    // Every contact of this net on the wire has to stay exactly on it
    for( const VECTOR2I& branch : branches )
    {
        if( std::none_of( result.Pieces.begin(), result.Pieces.end(),
                          [&]( const SEG& aPiece )
                          {
                              return aPiece.Contains( branch );
                          } ) )
        {
            result.Pieces = { aWire };
            result.Failed = true;
            break;
        }
    }

    replaceRegistered();

    return result;
}
