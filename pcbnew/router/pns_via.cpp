/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#include "pns_via.h"
#include "pns_node.h"
#include "pns_utils.h"

#include <geometry/shape_rect.h>

static bool Circle2Circle( VECTOR2I p1, VECTOR2I p2, int r1, int r2, VECTOR2I& force )
{
    int mindist = r1 + r2;
    VECTOR2I delta = p2 - p1;
    int dist = delta.EuclideanNorm();

    if( dist >= mindist )
        return false;

    force = delta.Resize( abs( mindist - dist ) + 1 );
    return true;
};

static bool Rect2Circle( VECTOR2I rp0, VECTOR2I rsize, VECTOR2I cc, int cr, VECTOR2I& force )
{
    VECTOR2I vts[] =
    {
        VECTOR2I( rp0.x,           rp0.y ),
        VECTOR2I( rp0.x,           rp0.y + rsize.y ),
        VECTOR2I( rp0.x + rsize.x, rp0.y + rsize.y ),
        VECTOR2I( rp0.x + rsize.x, rp0.y ),
        VECTOR2I( rp0.x,           rp0.y )
    };

    int dist = INT_MAX;
    VECTOR2I nearest;

    for( int i = 0; i < 4; i++ )
    {
        SEG s( vts[i], vts[i + 1] );

        VECTOR2I pn = s.NearestPoint( cc );

        int d = (pn - cc).EuclideanNorm();

        if( d < dist )
        {
            nearest = pn;
            dist = d;
        }
    }

    bool inside = cc.x >= rp0.x && cc.x <= (rp0.x + rsize.x)
                  && cc.y >= rp0.y && cc.y <= (rp0.y + rsize.y);

    VECTOR2I delta = cc - nearest;

    if( dist >= cr && !inside )
        return false;

    if( inside )
        force = -delta.Resize( abs( cr + dist ) + 1 );
    else
        force = delta.Resize( abs( cr - dist ) + 1 );

    return true;
};


static bool ShPushoutForce( const SHAPE* shape, VECTOR2I p, int r, VECTOR2I& force, int clearance )
{
    switch( shape->Type() )
    {
    case SH_CIRCLE:
        {
            const SHAPE_CIRCLE* cir = static_cast<const SHAPE_CIRCLE*>(shape);
            return Circle2Circle( cir->GetCenter(), p, cir->GetRadius(), r + clearance + 1, force );
        }

    case SH_RECT:
        {
            const SHAPE_RECT* rect = static_cast<const SHAPE_RECT*>(shape);
            return Rect2Circle( rect->GetPosition(), rect->GetSize(), p, r + clearance + 1, force );
        }

    default:
        return false;
    }

    return false;
}


bool PNS_VIA::PushoutForce( PNS_NODE* aNode,
        const VECTOR2I& aDirection,
        VECTOR2I& aForce,
        bool aSolidsOnly,
        int aMaxIterations )
{
    int iter = 0;
    PNS_VIA mv( *this );
    VECTOR2I force, totalForce;

    while( iter < aMaxIterations )
    {
        PNS_NODE::OptObstacle obs = aNode->CheckColliding( &mv,
                aSolidsOnly ? PNS_ITEM::SOLID : PNS_ITEM::ANY );

        if( !obs )
            break;

        int clearance = aNode->GetClearance( obs->item, &mv );

        if( iter > 10 )
        {
            VECTOR2I l = -aDirection.Resize( m_diameter / 4 );
            totalForce += l;
            mv.SetPos( mv.GetPos() + l );
        }

        if( ShPushoutForce( obs->item->GetShape(), mv.GetPos(), mv.GetDiameter() / 2, force,
                    clearance ) )
        {
            totalForce += force;
            mv.SetPos( mv.GetPos() + force );
        }


        iter++;
    }

    if( iter == aMaxIterations )
        return false;

    aForce = totalForce;
    return true;
}


const SHAPE_LINE_CHAIN PNS_VIA::Hull( int aClearance, int aWalkaroundThickness ) const
{
    return OctagonalHull( m_pos -
            VECTOR2I( m_diameter / 2, m_diameter / 2 ), VECTOR2I( m_diameter,
                    m_diameter ), aClearance + 1, (2 * aClearance + m_diameter) * 0.26 );
}
