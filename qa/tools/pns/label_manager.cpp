/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include <gal/graphics_abstraction_layer.h>
#include <gal/color4d.h>
#include <view/view_overlay.h>
#include <string>

#include "label_manager.h"

using KIGFX::GAL;
using KIGFX::COLOR4D;
using KIGFX::VIEW_OVERLAY;

LABEL_MANAGER::LABEL_MANAGER( GAL* aGal ) : m_canvas( aGal )
{
};


LABEL_MANAGER::~LABEL_MANAGER()
{
}


void LABEL_MANAGER::Add( VECTOR2I target, wxString msg, COLOR4D color )
{
    LABEL lbl;

    lbl.m_target = target;
    lbl.m_msg = msg;
    lbl.m_color = color;
    m_canvas->SetGlyphSize( VECTOR2D( m_textSize, m_textSize ) );

    KIFONT::FONT* strokeFont = KIFONT::FONT::GetFont( wxEmptyString );
    UTF8 text( msg );
    VECTOR2I textDims = strokeFont->StringBoundaryLimits( text, VECTOR2D( m_textSize, m_textSize ),
                                                          m_textSize / 8, false, false,
                                                          KIFONT::METRICS::Default() );

    lbl.m_bbox.SetOrigin( lbl.m_target - textDims - VECTOR2I( m_textSize, m_textSize ) );
    lbl.m_bbox.SetSize( textDims );
    m_labels.push_back( lbl );
}


void LABEL_MANAGER::Add( const SHAPE_LINE_CHAIN& aL, COLOR4D color )
{
    for( int i = 0; i < aL.PointCount(); i++ )
    {
        Add( aL.CPoint( i ), std::to_string( i ), color );
    }
}

void LABEL_MANAGER::Redraw( VIEW_OVERLAY* aOvl )
{
    recalculate();

    for( auto& lbl : m_labels )
    {
        aOvl->SetIsFill( false );
        aOvl->SetIsStroke( true );
        aOvl->SetLineWidth( 10000 );
        aOvl->SetStrokeColor( lbl.m_color.Brighten( 0.7 ) );
        aOvl->Rectangle( lbl.m_bbox.GetOrigin(), lbl.m_bbox.GetEnd() );
        aOvl->BitmapText( lbl.m_msg, lbl.m_bbox.Centre(), ANGLE_HORIZONTAL );
        VECTOR2I nearest = nearestBoxCorner( lbl.m_bbox, lbl.m_target );
        aOvl->Line( lbl.m_target, nearest );
    }
}


VECTOR2I LABEL_MANAGER::nearestBoxCorner( BOX2I b, VECTOR2I p )
{
    VECTOR2I ptest[4] = { b.GetPosition(), b.GetPosition() + VECTOR2I( b.GetWidth(), 0 ),
                          b.GetPosition() + VECTOR2I( b.GetWidth(), b.GetHeight() ),
                          b.GetPosition() + VECTOR2I( 0, b.GetHeight() ) };

    int      bestDist = INT_MAX;
    VECTOR2I rv;

    for( int i = 0; i < 4; i++ )
    {
        int dist = ( ptest[i] - p ).EuclideanNorm();

        if( dist < bestDist )
        {
            bestDist = dist;
            rv = ptest[i];
        }
    }

    return rv;
}


VECTOR2I LABEL_MANAGER::boxMtv( BOX2I b1, BOX2I b2 )
{
    VECTOR2I rv( 0, 0 );

    b1.Normalize();
    b2.Normalize();

    if( !b1.Intersects( b2 ) )
        return rv;

    int bestDist = INT_MAX;

    VECTOR2I p[4] = { b2.GetPosition(), b2.GetPosition() + VECTOR2I( b2.GetWidth(), 0 ),
                      b2.GetPosition() + VECTOR2I( b2.GetWidth(), b2.GetHeight() ),
                      b2.GetPosition() + VECTOR2I( 0, b2.GetHeight() ) };

    for( int i = 0; i < 4; i++ )
    {
        if( b1.Contains( p[i] ) )
        {
            //   printf("CONT %d\n", i );
            VECTOR2I dp[4] = { VECTOR2I( b1.GetEnd().x - p[i].x + 1, 0 ),
                               VECTOR2I( b1.GetPosition().x - p[i].x - 1, 0 ),
                               VECTOR2I( 0, b1.GetEnd().y - p[i].y + 1 ),
                               VECTOR2I( 0, b1.GetPosition().y - p[i].y - 1 ) };

            for( int j = 0; j < 4; j++ )
            {
                BOX2I btest( b2 );
                btest.Move( dp[j] );

                if( !b1.Intersects( btest ) )
                {
                    int dist = dp[j].EuclideanNorm();

                    if( dist < bestDist )
                    {
                        bestDist = dist;
                        rv = dp[j];
                    }
                }
            }
        }
    }

    return rv;
}


void LABEL_MANAGER::recalculate()
{
    int iterLimit = 5;

    return;
    while( iterLimit > 0 )
    {

        bool collisionsFound = false;

        for( int i = 0; i < m_labels.size(); i++ )
        {
            for( int j = 0; j < m_labels.size(); j++ )
            {
                if( i == j )
                    continue;

                auto bb_i = m_labels[i].m_bbox;
                auto bb_j = m_labels[j].m_bbox;

                bb_i.Inflate( 100000 );
                bb_j.Inflate( 100000 );
                VECTOR2I mtv = boxMtv( bb_i, bb_j );

                if( mtv.x || mtv.y )
                {
                    m_labels[i].m_bbox.Move( -mtv );
                    collisionsFound = true;
                }
            }
        }

        if( !collisionsFound )
            break;

        iterLimit--;
    }
}
