/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
 * Copyright (C) 2016-2022 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include "pns_meander_placer_base.h"
#include "pns_meander.h"
#include "pns_router.h"
#include "pns_solid.h"
#include "pns_arc.h"

namespace PNS {

const int LENGTH_TARGET_TOLERANCE = 20;

MEANDER_PLACER_BASE::MEANDER_PLACER_BASE( ROUTER* aRouter ) :
        PLACEMENT_ALGO( aRouter )
{
    m_world = nullptr;
    m_currentWidth = 0;
    m_startPad_n = nullptr;
    m_startPad_p = nullptr;
    m_endPad_n = nullptr;
    m_endPad_p = nullptr;
}


MEANDER_PLACER_BASE::~MEANDER_PLACER_BASE()
{
}


void MEANDER_PLACER_BASE::AmplitudeStep( int aSign )
{
    int a = m_settings.m_maxAmplitude + aSign * m_settings.m_step;
    a = std::max( a,  m_settings.m_minAmplitude );

    m_settings.m_maxAmplitude = a;
}


void MEANDER_PLACER_BASE::SpacingStep( int aSign )
{
    int s = m_settings.m_spacing + aSign * m_settings.m_step;
    s = std::max( s, m_currentWidth + Clearance() );

    m_settings.m_spacing = s;
}


int MEANDER_PLACER_BASE::Clearance()
{
    // Assumption: All tracks are part of the same net class.
    // It shouldn't matter which track we pick. They should all have the same clearance if
    // they are part of the same net class. Therefore, pick the first one on the list.
    ITEM*           itemToCheck = Traces().CItems().front();
    PNS::CONSTRAINT constraint;

    Router()->GetRuleResolver()->QueryConstraint( PNS::CONSTRAINT_TYPE::CT_CLEARANCE, itemToCheck,
                                                  nullptr, CurrentLayer(), &constraint );

    wxCHECK_MSG( constraint.m_Value.HasMin(), m_currentWidth, wxT( "No minimum clearance?" ) );

    return constraint.m_Value.Min();
}


void MEANDER_PLACER_BASE::UpdateSettings( const MEANDER_SETTINGS& aSettings )
{
    m_settings = aSettings;
}


void MEANDER_PLACER_BASE::cutTunedLine( const SHAPE_LINE_CHAIN& aOrigin, const VECTOR2I& aTuneStart,
                                        const VECTOR2I& aCursorPos, SHAPE_LINE_CHAIN& aPre,
                                        SHAPE_LINE_CHAIN& aTuned, SHAPE_LINE_CHAIN& aPost )
{
    VECTOR2I cp ( aCursorPos );

    if( cp == aTuneStart ) // we don't like tuning segments with 0 length
    {
        int idx = aOrigin.FindSegment( cp );

        if( idx >= 0 )
        {
            const SEG& s = aOrigin.CSegment( idx );
            cp += ( s.B - s.A ).Resize( 2 );
        }
        else
        {
            cp += VECTOR2I( 2, 5 ); // some arbitrary value that is not 45 degrees oriented
        }
    }

    VECTOR2I n = aOrigin.NearestPoint( cp, false );
    VECTOR2I m = aOrigin.NearestPoint( aTuneStart, false );

    SHAPE_LINE_CHAIN l( aOrigin );
    l.Split( n );
    l.Split( m );

    int i_start = l.Find( m );
    int i_end = l.Find( n );

    if( i_start > i_end )
    {
        l = l.Reverse();
        i_start = l.Find( m );
        i_end = l.Find( n );
    }

    aPre = l.Slice( 0, i_start );
    aPost = l.Slice( i_end, -1 );
    aTuned = l.Slice( i_start, i_end );

    aTuned.Simplify();
}


int findAmplitudeBinarySearch( MEANDER_SHAPE& aCopy, int targetLength, int minAmp, int maxAmp )
{
    if( minAmp == maxAmp )
        return maxAmp;

    aCopy.Resize( minAmp );
    int minLen = aCopy.CurrentLength();

    aCopy.Resize( maxAmp );
    int maxLen = aCopy.CurrentLength();

    if( minLen > targetLength )
        return 0;

    if( maxLen < targetLength )
        return 0;

    int minError = minLen - targetLength;
    int maxError = maxLen - targetLength;

    if( std::abs( minError ) < LENGTH_TARGET_TOLERANCE
        || std::abs( maxError ) < LENGTH_TARGET_TOLERANCE )
    {
        return std::abs( minError ) < std::abs( maxError ) ? minAmp : maxAmp;
    }
    else
    {
        int left =
                findAmplitudeBinarySearch( aCopy, targetLength, minAmp, ( minAmp + maxAmp ) / 2 );

        if( left )
            return left;

        int right =
                findAmplitudeBinarySearch( aCopy, targetLength, ( minAmp + maxAmp ) / 2, maxAmp );

        if( right )
            return right;
    }

    return 0;
}


int findAmplitudeForLength( MEANDER_SHAPE* m, int targetLength, int minAmp, int maxAmp )
{
    MEANDER_SHAPE copy = *m;

    // Try to keep the same baseline length
    copy.SetTargetBaselineLength( m->BaselineLength() );

    long long initialGuess = m->Amplitude() - ( m->CurrentLength() - targetLength ) / 2;

    if( initialGuess >= minAmp && initialGuess <= maxAmp )
    {
        copy.Resize( minAmp );

        if( std::abs( copy.CurrentLength() - targetLength ) < LENGTH_TARGET_TOLERANCE )
            return initialGuess;
    }

    // The length is non-trivial, use binary search
    return findAmplitudeBinarySearch( copy, targetLength, minAmp, maxAmp );
}


void MEANDER_PLACER_BASE::tuneLineLength( MEANDERED_LINE& aTuned, long long int aElongation )
{
    long long int maxElongation = 0;
    long long int minElongation = 0;
    bool          finished = false;

    for( MEANDER_SHAPE* m : aTuned.Meanders() )
    {
        if( m->Type() != MT_CORNER && m->Type() != MT_ARC )
        {
            MEANDER_SHAPE end = *m;
            MEANDER_TYPE  endType;

            if( m->Type() == MT_START || m->Type() == MT_SINGLE )
                endType = MT_SINGLE;
            else
                endType = MT_FINISH;

            end.SetType( endType );
            end.Recalculate();

            long long int maxEndElongation = end.CurrentLength() - end.BaselineLength();

            if( maxElongation + maxEndElongation > aElongation )
            {
                if( !finished )
                {
                    m->SetType( endType );
                    m->Recalculate();

                    if( endType == MT_SINGLE )
                    {
                        // Check if we need to fit this meander
                        long long int endMinElongation =
                                ( m->MinTunableLength() - m->BaselineLength() );

                        if( minElongation + endMinElongation >= aElongation )
                            m->MakeEmpty();
                    }

                    finished = true;
                }
                else
                {
                    m->MakeEmpty();
                }
            }

            maxElongation += m->CurrentLength() - m->BaselineLength();
            minElongation += m->MinTunableLength() - m->BaselineLength();
        }
    }

    long long int remainingElongation = aElongation;
    int           meanderCount = 0;

    for( MEANDER_SHAPE* m : aTuned.Meanders() )
    {
        if( m->Type() != MT_CORNER && m->Type() != MT_ARC && m->Type() != MT_EMPTY )
        {
            remainingElongation -= m->CurrentLength() - m->BaselineLength();
            meanderCount++;
        }
    }

    long long int lenReductionLeft = -remainingElongation;
    int           meandersLeft = meanderCount;

    if( lenReductionLeft < 0 || !meandersLeft )
        return;

    for( MEANDER_SHAPE* m : aTuned.Meanders() )
    {
        if( m->Type() != MT_CORNER && m->Type() != MT_ARC && m->Type() != MT_EMPTY )
        {
            long long int lenReductionHere = lenReductionLeft / meandersLeft;
            long long int initialLen = m->CurrentLength();
            int           minAmpl = m->MinAmplitude();

            int amp = findAmplitudeForLength( m, initialLen - lenReductionHere, minAmpl,
                                              m->Amplitude() );

            if( amp < minAmpl )
                amp = minAmpl;

            m->SetTargetBaselineLength( m->BaselineLength() );
            m->Resize( amp );

            lenReductionLeft -= initialLen - m->CurrentLength();
            meandersLeft--;

            if( !meandersLeft )
                break;
        }
    }
}


int MEANDER_PLACER_BASE::GetTotalPadToDieLength( const LINE& aLine ) const
{
    int   length = 0;
    JOINT start;
    JOINT end;

    m_world->FindLineEnds( aLine, start, end );

    // Extract the length of the pad to die for start and end pads
    for( auto& link : start.LinkList() )
    {
        if( const SOLID* solid = dyn_cast<const SOLID*>( link ) )
        {
            // If there are overlapping pads, choose the first with a non-zero length
            if( solid->GetPadToDie() > 0 )
            {
                length += solid->GetPadToDie();
                break;
            }
        }
    }

    for( auto& link : end.LinkList() )
    {
        if( const SOLID* solid = dyn_cast<const SOLID*>( link ) )
        {
            if( solid->GetPadToDie() > 0 )
            {
                length += solid->GetPadToDie();
                break;
            }
        }
    }

    return length;
}


const MEANDER_SETTINGS& MEANDER_PLACER_BASE::MeanderSettings() const
{
    return m_settings;
}


int MEANDER_PLACER_BASE::compareWithTolerance(
        long long int aValue, long long int aExpected, long long int aTolerance ) const
{
    if( aValue < aExpected - aTolerance )
        return -1;
    else if( aValue > aExpected + aTolerance )
        return 1;
    else
        return 0;
}


VECTOR2I MEANDER_PLACER_BASE::getSnappedStartPoint( LINKED_ITEM* aStartItem, VECTOR2I aStartPoint )
{
    if( aStartItem->Kind() == ITEM::SEGMENT_T )
    {
        return static_cast<SEGMENT*>( aStartItem )->Seg().NearestPoint( aStartPoint );
    }
    else
    {
        wxASSERT( aStartItem->Kind() == ITEM::ARC_T );
        ARC* arc = static_cast<ARC*>( aStartItem );

        if( ( VECTOR2I( arc->Anchor( 0 ) - aStartPoint ) ).SquaredEuclideanNorm() <=
            ( VECTOR2I( arc->Anchor( 1 ) - aStartPoint ) ).SquaredEuclideanNorm() )
        {
            return arc->Anchor( 0 );
        }
        else
        {
            return arc->Anchor( 1 );
        }
    }
}


long long int MEANDER_PLACER_BASE::lineLength( const ITEM_SET& aLine, const SOLID* aStartPad, const SOLID* aEndPad ) const
{
    long long int total = 0;

    if( aLine.Empty() )
        return 0;

    const ITEM* start_item = aLine[0];
    const ITEM* end_item = aLine[aLine.Size() - 1];
    bool start_via = false;
    bool end_via = false;


    /**
     * If there is a start pad but the pad's layers do not overlap the first track layer, then there must be a
     * fanout via on the line.  If there isn't, we still need to have the via back to the pad, so count the distance
     * in the line tuning
     */
    start_via = aStartPad && ( !aStartPad->LayersOverlap( start_item ) );
    end_via = aEndPad && ( !aEndPad->LayersOverlap( end_item ) );

    for( int idx = 0; idx < aLine.Size(); idx++ )
    {
        const ITEM* item = aLine[idx];

        if( const LINE* l = dyn_cast<const LINE*>( item ) )
        {
            total += l->CLine().Length();
        }
        else if( item->OfKind( ITEM::VIA_T ) && idx > 0 && idx < aLine.Size() - 1 )
        {
            int layerPrev = aLine[idx - 1]->Layer();
            int layerNext = aLine[idx + 1]->Layer();

            if( layerPrev != layerNext )
                total += m_router->GetInterface()->StackupHeight( layerPrev, layerNext );
        }
    }

    if( start_via )
    {
        int layerPrev = aStartPad->Layer();
        int layerNext = start_item->Layer();

        total += m_router->GetInterface()->StackupHeight( layerPrev, layerNext );
    }

    if( end_via )
    {
        int layerPrev = end_item->Layer();
        int layerNext = aEndPad->Layer();

        total += m_router->GetInterface()->StackupHeight( layerPrev, layerNext );
    }

    return total;
}

}
