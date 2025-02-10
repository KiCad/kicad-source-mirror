/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


const MEANDER_SETTINGS& MEANDER_PLACER_BASE::MeanderSettings() const
{
    return m_settings;
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
    if( aLine.Empty() )
        return 0;

    ROUTER_IFACE* iface = Router()->GetInterface();
    return iface->CalculateRoutedPathLength( aLine, aStartPad, aEndPad, m_settings.m_netClass );
}


int64_t MEANDER_PLACER_BASE::lineDelay( const ITEM_SET& aLine, const SOLID* aStartPad, const SOLID* aEndPad ) const
{
    if( aLine.Empty() )
        return 0;

    ROUTER_IFACE* iface = Router()->GetInterface();
    return iface->CalculateRoutedPathDelay( aLine, aStartPad, aEndPad, m_settings.m_netClass );
}
}
