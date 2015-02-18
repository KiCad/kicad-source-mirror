/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#include <boost/foreach.hpp>

#include <base_units.h> // God forgive me doing this...
#include <colors.h>

#include "trace.h"

#include "pns_node.h"
#include "pns_itemset.h"
#include "pns_topology.h"
#include "pns_meander.h"
#include "pns_meander_placer_base.h"
#include "pns_router.h"

const PNS_MEANDER_SETTINGS& PNS_MEANDER_SHAPE::Settings() const
{
    return m_placer->MeanderSettings();
}

const PNS_MEANDER_SETTINGS& PNS_MEANDERED_LINE::Settings() const
{
    return m_placer->MeanderSettings();
}

void PNS_MEANDERED_LINE::MeanderSegment( const SEG& aBase, int aBaseIndex )
{
    double base_len = aBase.Length();

    SHAPE_LINE_CHAIN lc;

    bool side = true;
    VECTOR2D dir( aBase.B - aBase.A );

    if( !m_dual )
        AddCorner( aBase.A );

    bool turning = false;
    bool started = false;

    m_last = aBase.A;

    do
    {
        PNS_MEANDER_SHAPE* m = new PNS_MEANDER_SHAPE( m_placer, m_width, m_dual );
        m->SetBaselineOffset( m_baselineOffset );
        m->SetBaseIndex( aBaseIndex );

        double thr = (double) m->spacing();

        bool fail = false;
        double remaining = base_len - ( m_last - aBase.A ).EuclideanNorm();

        if( remaining < Settings( ).m_step )
            break;

        if( remaining > 3.0 * thr )
        {
            if( !turning )
            {
                for( int i = 0; i < 2; i++ )
                {
                    if ( m->Fit( MT_CHECK_START, aBase, m_last, i ) )
                    {
                        turning = true;
                        AddMeander( m );
                        side = !i;
                        started = true;
                        break;
                    }
                }

                if( !turning )
                {
                    fail = true;

                    for( int i = 0; i < 2; i++ )
                    {
                        if ( m->Fit ( MT_SINGLE, aBase, m_last, i ) )
                        {
                            AddMeander( m );
                            fail = false;
                            started = false;
                            side = !i;
                            break;
                        }
                    }
                }
            } else {
                bool rv = m->Fit( MT_CHECK_FINISH, aBase, m_last, side );

                if( rv )
                {
                    m->Fit( MT_TURN, aBase, m_last, side );
                    AddMeander( m );
                    started = true;
                } else {
                    m->Fit( MT_FINISH, aBase, m_last, side );
                    started = false;
                    AddMeander( m );
                    turning = false;
                }

                side = !side;
            }
        } else if( started )
        {
            bool rv = m->Fit( MT_FINISH, aBase, m_last, side );
            if( rv )
                AddMeander( m );

            break;

        } else {
           fail = true;
        }

        remaining = base_len - ( m_last - aBase.A ).EuclideanNorm( );

        if( remaining < Settings( ).m_step )
            break;

        if( fail )
        {
            PNS_MEANDER_SHAPE tmp( m_placer, m_width, m_dual );
            tmp.SetBaselineOffset( m_baselineOffset );
            tmp.SetBaseIndex( aBaseIndex );

            int nextP = tmp.spacing() - 2 * tmp.cornerRadius() + Settings().m_step;
            VECTOR2I pn = m_last + dir.Resize( nextP );

            if( aBase.Contains( pn ) && !m_dual )
            {
                AddCorner( pn );
            } else
                break;
        }


    } while( true );

    if( !m_dual )
        AddCorner( aBase.B );
}


int PNS_MEANDER_SHAPE::cornerRadius() const
{
    int cr = (int64_t) spacing() * Settings().m_cornerRadiusPercentage / 200;

    return cr;
}


int PNS_MEANDER_SHAPE::spacing( ) const
{
    if ( !m_dual )
        return std::max( 2 * m_width, Settings().m_spacing );
    else
    {
        int sp = 2 * ( m_width + std::abs( m_baselineOffset ) );
        return std::max ( sp, Settings().m_spacing );
    }
}


SHAPE_LINE_CHAIN PNS_MEANDER_SHAPE::circleQuad( VECTOR2D aP, VECTOR2D aDir, bool aSide )
{
    SHAPE_LINE_CHAIN lc;

    if( aDir.EuclideanNorm( ) == 0.0f )
    {
        lc.Append( aP );
        return lc;
    }

    VECTOR2D dir_u( aDir );
    VECTOR2D dir_v( aDir.Perpendicular( ) );

    const int ArcSegments = Settings().m_cornerArcSegments;

    for( int i = ArcSegments - 1; i >= 0; i-- )
    {
        VECTOR2D p;
        double alpha = (double) i / (double) ( ArcSegments - 1 ) * M_PI / 2.0;
        p = aP + dir_u * cos( alpha ) + dir_v * ( aSide ? -1.0 : 1.0 ) * ( 1.0 - sin( alpha ) );
        lc.Append( ( int ) p.x, ( int ) p.y );
    }

    return lc;
}


VECTOR2I PNS_MEANDER_SHAPE::reflect( VECTOR2I p, const SEG& line )
{
    typedef int64_t ecoord;
    VECTOR2I d = line.B - line.A;
    ecoord l_squared = d.Dot( d );
    ecoord t = d.Dot( p - line.A );
    VECTOR2I c, rv;

    if( !l_squared )
        c = p;
    else {
        c.x = line.A.x + rescale( t, (ecoord) d.x, l_squared );
        c.y = line.A.y + rescale( t, (ecoord) d.y, l_squared );
    }

    return 2 * c - p;
}


void PNS_MEANDER_SHAPE::start( SHAPE_LINE_CHAIN* aTarget, const VECTOR2D& aWhere, const VECTOR2D& aDir )
{
    m_currentTarget = aTarget;
    m_currentTarget->Clear();
    m_currentTarget->Append( aWhere );
    m_currentDir = aDir;
    m_currentPos = aWhere;
}


void PNS_MEANDER_SHAPE::forward( int aLength )
{
    m_currentPos += m_currentDir.Resize( aLength );
    m_currentTarget->Append( m_currentPos );
}


void PNS_MEANDER_SHAPE::turn( int aAngle )
{
    m_currentDir = m_currentDir.Rotate( (double) aAngle * M_PI / 180.0 );
}


void PNS_MEANDER_SHAPE::arc( int aRadius, bool aSide )
{
    if( aRadius <= 0 )
    {
        turn( aSide ? -90 : 90 );
        return;
    }

    VECTOR2D dir = m_currentDir.Resize( (double) aRadius );
    SHAPE_LINE_CHAIN arc = circleQuad( m_currentPos, dir, aSide );
    m_currentPos = arc.CPoint( -1 );
    m_currentDir = dir.Rotate( aSide ? -M_PI / 2.0 : M_PI / 2.0 );

    m_currentTarget->Append ( arc );
}


void PNS_MEANDER_SHAPE::uShape( int aSides, int aCorner, int aTop )
{
    forward( aSides );
    arc( aCorner, true );
    forward( aTop );
    arc( aCorner, true );
    forward( aSides );
}


SHAPE_LINE_CHAIN PNS_MEANDER_SHAPE::genMeanderShape( VECTOR2D aP, VECTOR2D aDir,
        bool aSide, PNS_MEANDER_TYPE aType, int aAmpl, int aBaselineOffset )
{
    const PNS_MEANDER_SETTINGS& st = Settings();
    int cr = cornerRadius();
    int offset = aBaselineOffset;
    int spc = spacing();

    if( aSide )
        offset *= -1;

    VECTOR2D dir_u_b( aDir.Resize( offset ) );
    VECTOR2D dir_v_b( dir_u_b.Perpendicular() );

    if( 2 * cr > aAmpl )
    {
        cr = aAmpl / 2;
    }

    if( 2 * cr > spc )
    {
        cr = spc / 2;
    }

    SHAPE_LINE_CHAIN lc;

    start( &lc, aP + dir_v_b, aDir );

    switch( aType )
    {
        case MT_EMPTY:
        {
            lc.Append( aP + dir_v_b + aDir );
            break;
        }
        case MT_START:
        {
            arc( cr - offset, false );
            uShape( aAmpl - 2 * cr + std::abs( offset ), cr + offset, spc - 2 * cr );
            forward( std::min( cr - offset, cr + offset ) );
            forward( std::abs( offset ) );

            break;
        }

        case MT_FINISH:
        {
            start( &lc, aP - dir_u_b, aDir );
            turn ( 90 );
            forward( std::min( cr - offset, cr + offset ) );
            forward( std::abs( offset ) );
            uShape( aAmpl - 2 * cr + std::abs( offset ), cr + offset, spc - 2 * cr );
            arc( cr - offset, false );
            break;
        }

        case MT_TURN:
        {
            start( &lc, aP - dir_u_b, aDir );
            turn( 90 );
            forward( std::abs( offset ) );
            uShape ( aAmpl - cr, cr + offset, spc - 2 * cr );
            forward( std::abs( offset ) );
            break;
        }

        case MT_SINGLE:
        {
            arc( cr - offset, false );
            uShape( aAmpl - 2 * cr + std::abs( offset ), cr + offset, spc - 2 * cr );
            arc( cr - offset, false );
            lc.Append( aP + dir_v_b + aDir.Resize ( 2 * st.m_spacing ) );
            break;
        }

        default:
            break;
    }

    if( aSide )
    {
        SEG axis ( aP, aP + aDir );

        for( int i = 0; i < lc.PointCount(); i++ )
            lc.Point( i ) = reflect( lc.CPoint( i ), axis );
    }

    return lc;
}


bool PNS_MEANDERED_LINE::CheckSelfIntersections( PNS_MEANDER_SHAPE* aShape, int aClearance )
{
    for( int i = m_meanders.size() - 1; i >= 0; i-- )
    {
        PNS_MEANDER_SHAPE* m = m_meanders[i];

        if( m->Type() == MT_EMPTY || m->Type() == MT_CORNER )
            continue;

        const SEG& b1 = aShape->BaseSegment();
        const SEG& b2 = m->BaseSegment();

        if( b1.ApproxParallel( b2 ) )
            continue;

        int n = m->CLine( 0 ).SegmentCount();

        for( int j = n - 1; j >= 0; j-- )
            if( aShape->CLine( 0 ).Collide ( m->CLine( 0 ) .CSegment( j ), aClearance ) )
                return false;
    }

    return true;
}


bool PNS_MEANDER_SHAPE::Fit( PNS_MEANDER_TYPE aType, const SEG& aSeg, const VECTOR2I& aP, bool aSide )
{
    const PNS_MEANDER_SETTINGS& st = Settings();

    bool checkMode = false;
    PNS_MEANDER_TYPE prim1, prim2;

    if( aType == MT_CHECK_START )
    {
        prim1 = MT_START;
        prim2 = MT_TURN;
        checkMode = true;
    }
    else if( aType == MT_CHECK_FINISH )
    {
        prim1 = MT_TURN;
        prim2 = MT_FINISH;
        checkMode = true;
    }

    if( checkMode )
    {
        PNS_MEANDER_SHAPE m1( m_placer, m_width, m_dual );
        PNS_MEANDER_SHAPE m2( m_placer, m_width, m_dual );

        m1.SetBaselineOffset( m_baselineOffset );
        m2.SetBaselineOffset( m_baselineOffset );

        bool c1 = m1.Fit( prim1, aSeg, aP, aSide );
        bool c2 = false;

        if( c1 )
            c2 = m2.Fit( prim2, aSeg, m1.End(), !aSide );

        if( c1 && c2 )
        {
            m_type = prim1;
            m_shapes[0] = m1.m_shapes[0];
            m_shapes[1] = m1.m_shapes[1];
            m_baseSeg =aSeg;
            m_p0 = aP;
            m_side = aSide;
            m_amplitude = m1.Amplitude();
            m_dual = m1.m_dual;
            m_baseSeg = m1.m_baseSeg;
            m_baseIndex = m1.m_baseIndex;
            updateBaseSegment();
            m_baselineOffset = m1.m_baselineOffset;
            return true;
        } else
            return false;
    }

    int minAmpl = st.m_minAmplitude;
    int maxAmpl = st.m_maxAmplitude;

    if( m_dual )
    {
        minAmpl = std::max( minAmpl, 2 * std::abs( m_baselineOffset ) );
        maxAmpl = std::max( maxAmpl, 2 * std::abs( m_baselineOffset ) );
    }

    for( int ampl = maxAmpl; ampl >= minAmpl; ampl -= st.m_step )
    {
        if( m_dual )
        {
            m_shapes[0] = genMeanderShape( aP, aSeg.B - aSeg.A, aSide, aType, ampl, m_baselineOffset );
            m_shapes[1] = genMeanderShape( aP, aSeg.B - aSeg.A, aSide, aType, ampl, -m_baselineOffset );
        }
        else
        {
            m_shapes[0] = genMeanderShape( aP, aSeg.B - aSeg.A, aSide, aType, ampl, 0 );
        }

        m_type = aType;
        m_baseSeg = aSeg;
        m_p0 = aP;
        m_side = aSide;
        m_amplitude = ampl;

        updateBaseSegment();

        if( m_placer->CheckFit( this ) )
            return true;
    }

    return false;
}


void PNS_MEANDER_SHAPE::Recalculate()
{
    m_shapes[0] = genMeanderShape( m_p0, m_baseSeg.B - m_baseSeg.A, m_side, m_type, m_amplitude, m_dual ? m_baselineOffset : 0 );

    if( m_dual )
        m_shapes[1] = genMeanderShape( m_p0, m_baseSeg.B - m_baseSeg.A, m_side, m_type, m_amplitude, -m_baselineOffset );

    updateBaseSegment();
}


void PNS_MEANDER_SHAPE::Resize( int aAmpl )
{
    if( aAmpl < 0 )
        return;

    m_amplitude = aAmpl;

    Recalculate();
}


void PNS_MEANDER_SHAPE::MakeEmpty()
{
    updateBaseSegment();

    VECTOR2I dir = m_clippedBaseSeg.B - m_clippedBaseSeg.A;

    m_type = MT_EMPTY;

    m_shapes[0] = genMeanderShape ( m_p0, dir, m_side, m_type, 0, m_dual ? m_baselineOffset : 0 );

    if( m_dual )
        m_shapes[1] = genMeanderShape( m_p0, dir, m_side, m_type, 0, -m_baselineOffset );
}


void PNS_MEANDERED_LINE::AddCorner( const VECTOR2I& aA, const VECTOR2I& aB )
{
    PNS_MEANDER_SHAPE* m = new PNS_MEANDER_SHAPE( m_placer, m_width, m_dual );

    m->MakeCorner( aA, aB );
    m_last = aA;

    m_meanders.push_back( m );
}


void PNS_MEANDER_SHAPE::MakeCorner( VECTOR2I aP1, VECTOR2I aP2 )
{
    SetType( MT_CORNER );
    m_shapes[0].Clear();
    m_shapes[1].Clear();
    m_shapes[0].Append( aP1 );
    m_shapes[1].Append( aP2 );
    m_clippedBaseSeg.A = aP1;
    m_clippedBaseSeg.B = aP1;
}


void PNS_MEANDERED_LINE::AddMeander( PNS_MEANDER_SHAPE* aShape )
{
    m_last = aShape->BaseSegment().B;
    m_meanders.push_back( aShape );
}


void PNS_MEANDERED_LINE::Clear()
{
    BOOST_FOREACH( PNS_MEANDER_SHAPE* m, m_meanders )
    {
        delete m;
    }

    m_meanders.clear( );
}


int PNS_MEANDER_SHAPE::BaselineLength() const
{
    return m_clippedBaseSeg.Length();
}


int PNS_MEANDER_SHAPE::MaxTunableLength() const
{
    return CLine( 0 ).Length();
}


void PNS_MEANDER_SHAPE::updateBaseSegment( )
{
    if( m_dual )
    {
        VECTOR2I midpA = ( CLine( 0 ).CPoint( 0 )  + CLine( 1 ).CPoint( 0  ) ) / 2;
        VECTOR2I midpB = ( CLine( 0 ).CPoint( -1 ) + CLine( 1 ).CPoint( -1 ) ) / 2;

        m_clippedBaseSeg.A = m_baseSeg.LineProject( midpA );
        m_clippedBaseSeg.B = m_baseSeg.LineProject( midpB );
    }
    else
    {
        m_clippedBaseSeg.A = m_baseSeg.LineProject( CLine( 0 ).CPoint( 0 ) );
        m_clippedBaseSeg.B = m_baseSeg.LineProject( CLine( 0 ).CPoint( -1 ) );
    }
}
