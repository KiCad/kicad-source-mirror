/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file dcode.cpp
 * @brief D_CODE class implementation
 */

#include <trigo.h>
#include <eda_rect.h>
#include <gerbview_frame.h>
#include <gerber_file_image.h>
#include <convert_to_biu.h>
#include <convert_basic_shapes_to_polygon.h>

#define DCODE_DEFAULT_SIZE Millimeter2iu( 0.1 )

/* Format Gerber: NOTES:
 * Tools and D_CODES
 *   tool number (identification of shapes)
 *   1 to 999
 *
 * D_CODES:
 *   D01 ... D9 = command codes:
 *      D01 = activating light (pen down) while moving
 *      D02 = light extinction (pen up) while moving
 *      D03 = Flash
 *      D04 to D09 = non used
 *   D10 ... D999 = Identification Tool (Shape id)
 *
 * For tools defining a shape):
 * DCode min = D10
 * DCode max = 999
 */


D_CODE::D_CODE( int num_dcode )
{
    m_Num_Dcode = num_dcode;
    Clear_D_CODE_Data();
}


D_CODE::~D_CODE()
{
}


void D_CODE::Clear_D_CODE_Data()
{
    m_Size.x     = DCODE_DEFAULT_SIZE;
    m_Size.y     = DCODE_DEFAULT_SIZE;
    m_Shape      = APT_CIRCLE;
    m_Drill.x    = m_Drill.y = 0;
    m_DrillShape = APT_DEF_NO_HOLE;
    m_InUse      = false;
    m_Defined    = false;
    m_Macro      = nullptr;
    m_Rotation   = 0.0;
    m_EdgesCount = 0;
    m_Polygon.RemoveAllContours();
}


const wxChar* D_CODE::ShowApertureType( APERTURE_T aType )
{
    const wxChar* ret;

    switch( aType )
    {
    case APT_CIRCLE:
        ret = wxT( "Round" );   break;

    case APT_RECT:
        ret = wxT( "Rect" );    break;

    case APT_OVAL:
        ret = wxT( "Oval" );    break;

    case APT_POLYGON:
        ret = wxT( "Poly" );    break;

    case APT_MACRO:
        ret = wxT( "Macro" );   break;

    default:
        ret = wxT( "???" );     break;
    }

    return ret;
}


int D_CODE::GetShapeDim( GERBER_DRAW_ITEM* aParent )
{
    int dim = -1;

    switch( m_Shape )
    {
    case APT_CIRCLE:
        dim = m_Size.x;
        break;

    case APT_RECT:
    case APT_OVAL:
        dim = std::min( m_Size.x, m_Size.y );
        break;

    case APT_POLYGON:
        dim = std::min( m_Size.x, m_Size.y );
        break;

    case APT_MACRO:
        if( m_Macro )
            dim = m_Macro->GetShapeDim( aParent );
        break;

    default:
        break;
    }

    return dim;
}


void D_CODE::DrawFlashedShape( GERBER_DRAW_ITEM* aParent, EDA_RECT* aClipBox, wxDC* aDC,
                               const COLOR4D& aColor, const wxPoint& aShapePos, bool aFilledShape )
{
    int radius;

    switch( m_Shape )
    {
    case APT_MACRO:
        GetMacro()->DrawApertureMacroShape( aParent, aClipBox, aDC, aColor, aShapePos,
                                            aFilledShape );
        break;

    case APT_CIRCLE:
        radius = m_Size.x >> 1;

        if( !aFilledShape )
        {
            GRCircle( aClipBox, aDC, aParent->GetABPosition(aShapePos), radius, 0, aColor );
        }
        else if( m_DrillShape == APT_DEF_NO_HOLE )
        {
            GRFilledCircle( aClipBox, aDC, aParent->GetABPosition(aShapePos), radius, aColor );
        }
        else if( m_DrillShape == APT_DEF_ROUND_HOLE )    // round hole in shape
        {
            int width = (m_Size.x - m_Drill.x ) / 2;
            GRCircle( aClipBox, aDC,  aParent->GetABPosition(aShapePos),
                      radius - (width / 2), width, aColor );
        }
        else                            // rectangular hole
        {
            if( m_Polygon.OutlineCount() == 0 )
                ConvertShapeToPolygon();

            DrawFlashedPolygon( aParent, aClipBox, aDC, aColor, aFilledShape, aShapePos );
        }

        break;

    case APT_RECT:
    {
        wxPoint start;
        start.x = aShapePos.x - m_Size.x / 2;
        start.y = aShapePos.y - m_Size.y / 2;
        wxPoint end = start + m_Size;
        start = aParent->GetABPosition( start );
        end = aParent->GetABPosition( end );

        if( !aFilledShape )
        {
            GRRect( aClipBox, aDC, start.x, start.y, end.x, end.y, 0, aColor );
        }
        else if( m_DrillShape == APT_DEF_NO_HOLE )
        {
            GRFilledRect( aClipBox, aDC, start.x, start.y, end.x, end.y, 0, aColor, aColor );
        }
        else
        {
            if( m_Polygon.OutlineCount() == 0 )
                ConvertShapeToPolygon();

            DrawFlashedPolygon( aParent, aClipBox, aDC, aColor, aFilledShape, aShapePos );
        }
    }
    break;

    case APT_OVAL:
    {
        wxPoint start = aShapePos;
        wxPoint end   = aShapePos;

        if( m_Size.x > m_Size.y )   // horizontal oval
        {
            int delta = ( m_Size.x - m_Size.y ) / 2;
            start.x -= delta;
            end.x   += delta;
            radius   = m_Size.y;    // Width in fact
        }
        else   // vertical oval
        {
            int delta = ( m_Size.y - m_Size.x ) / 2;
            start.y -= delta;
            end.y   += delta;
            radius   = m_Size.x;    // Width in fact
        }

        start = aParent->GetABPosition( start );
        end = aParent->GetABPosition( end );

        if( !aFilledShape )
        {
            GRCSegm( aClipBox, aDC, start.x, start.y, end.x, end.y, radius, aColor );
        }
        else if( m_DrillShape == APT_DEF_NO_HOLE )
        {
            GRFillCSegm( aClipBox, aDC, start.x, start.y, end.x, end.y, radius, aColor );
        }
        else
        {
            if( m_Polygon.OutlineCount() == 0 )
                ConvertShapeToPolygon();

            DrawFlashedPolygon( aParent, aClipBox, aDC, aColor, aFilledShape, aShapePos );
        }
    }

    break;

    case APT_POLYGON:
        if( m_Polygon.OutlineCount() == 0 )
            ConvertShapeToPolygon();

        DrawFlashedPolygon( aParent, aClipBox, aDC, aColor, aFilledShape, aShapePos );
        break;
    }
}


void D_CODE::DrawFlashedPolygon( GERBER_DRAW_ITEM* aParent, EDA_RECT* aClipBox, wxDC* aDC,
                                 const COLOR4D& aColor, bool aFilled, const wxPoint& aPosition )
{
    if( m_Polygon.OutlineCount() == 0 )
        return;

    int pointCount = m_Polygon.VertexCount();
    std::vector<wxPoint> points;
    points.reserve( pointCount );

    for( int ii = 0; ii < pointCount; ii++ )
    {
        wxPoint p( m_Polygon.CVertex( ii ).x, m_Polygon.CVertex( ii ).y );
        points[ii] = p + aPosition;
        points[ii] = aParent->GetABPosition( points[ii] );
    }

    GRClosedPoly( aClipBox, aDC, pointCount, &points[0], aFilled, aColor, aColor );
}


// TODO(snh): Remove the hard-coded count
#define SEGS_CNT 64     // number of segments to approximate a circle


// A helper function for D_CODE::ConvertShapeToPolygon().   Add a hole to a polygon
static void addHoleToPolygon( SHAPE_POLY_SET*       aPolygon,
                              APERTURE_DEF_HOLETYPE aHoleShape,
                              const wxSize&         aSize,
                              const wxPoint&        aAnchorPos );


void D_CODE::ConvertShapeToPolygon()
{
    wxPoint initialpos;
    wxPoint currpos;

    m_Polygon.RemoveAllContours();

    switch( m_Shape )
    {
    case APT_CIRCLE:        // creates only a circle with rectangular hole
        TransformCircleToPolygon( m_Polygon, initialpos, m_Size.x >> 1, ARC_HIGH_DEF,
                                  ERROR_INSIDE );
        addHoleToPolygon( &m_Polygon, m_DrillShape, m_Drill, initialpos );
        break;

    case APT_RECT:
        m_Polygon.NewOutline();
        currpos.x  = m_Size.x / 2;
        currpos.y  = m_Size.y / 2;
        initialpos = currpos;
        m_Polygon.Append( VECTOR2I( currpos ) );
        currpos.x -= m_Size.x;
        m_Polygon.Append( VECTOR2I( currpos ) );
        currpos.y -= m_Size.y;
        m_Polygon.Append( VECTOR2I( currpos ) );
        currpos.x += m_Size.x;
        m_Polygon.Append( VECTOR2I( currpos ) );
        currpos.y += m_Size.y;
        m_Polygon.Append( VECTOR2I( currpos ) );    // close polygon
        m_Polygon.Append( VECTOR2I( initialpos ) );

        addHoleToPolygon( &m_Polygon, m_DrillShape, m_Drill, initialpos );
        break;

    case APT_OVAL:
    {
        m_Polygon.NewOutline();
        int delta, radius;

        // we create an horizontal oval shape. then rotate if needed
        if( m_Size.x > m_Size.y )   // horizontal oval
        {
            delta = ( m_Size.x - m_Size.y ) / 2;
            radius = m_Size.y / 2;
        }
        else   // vertical oval
        {
            delta  = (m_Size.y - m_Size.x) / 2;
            radius = m_Size.x / 2;
        }

        currpos.y  = radius;
        initialpos = currpos;
        m_Polygon.Append( VECTOR2I( currpos ) );

        // build the right arc of the shape
        unsigned ii = 0;

        for( ; ii <= SEGS_CNT / 2; ii++ )
        {
            currpos = initialpos;
            RotatePoint( &currpos, ii * 3600.0 / SEGS_CNT );
            currpos.x += delta;
            m_Polygon.Append( VECTOR2I( currpos ) );
        }

        // build the left arc of the shape
        for( ii = SEGS_CNT / 2; ii <= SEGS_CNT; ii++ )
        {
            currpos = initialpos;
            RotatePoint( &currpos, ii * 3600.0 / SEGS_CNT );
            currpos.x -= delta;
            m_Polygon.Append( VECTOR2I( currpos ) );
        }

        m_Polygon.Append( VECTOR2I( initialpos ) );      // close outline

        if( m_Size.y > m_Size.x )                   // vertical oval, rotate polygon.
            m_Polygon.Rotate( -M_PI / 2 );

        addHoleToPolygon( &m_Polygon, m_DrillShape, m_Drill, initialpos );
    }

    break;

    case APT_POLYGON:
        m_Polygon.NewOutline();
        currpos.x  = m_Size.x >> 1;     // first point is on X axis
        initialpos = currpos;

        // rs274x said: m_EdgesCount = 3 ... 12
        if( m_EdgesCount < 3 )
            m_EdgesCount = 3;

        if( m_EdgesCount > 12 )
            m_EdgesCount = 12;

        for( int ii = 0; ii < m_EdgesCount; ii++ )
        {
            currpos = initialpos;
            RotatePoint( &currpos, ii * 3600.0 / m_EdgesCount );
            m_Polygon.Append( VECTOR2I( currpos ) );
        }

        addHoleToPolygon( &m_Polygon, m_DrillShape, m_Drill, initialpos );

        if( m_Rotation )    // rotate polygonal shape:
        {
            double angle = m_Rotation * M_PI / 180;
            m_Polygon.Rotate( angle, VECTOR2I( 0, 0 ) );
        }

        break;

    case APT_MACRO:

        // TODO
        break;
    }
}


// The helper function for D_CODE::ConvertShapeToPolygon().
// Add a hole to a polygon
static void addHoleToPolygon( SHAPE_POLY_SET*       aPolygon,
                              APERTURE_DEF_HOLETYPE aHoleShape,
                              const wxSize&         aSize,
                              const wxPoint&        aAnchorPos )
{
    wxPoint currpos;
    SHAPE_POLY_SET holeBuffer;

    if( aHoleShape == APT_DEF_ROUND_HOLE )
    {
        TransformCircleToPolygon( holeBuffer, wxPoint( 0, 0 ), aSize.x / 2, ARC_HIGH_DEF,
                                  ERROR_INSIDE );
    }
    else if( aHoleShape == APT_DEF_RECT_HOLE )
    {
        holeBuffer.NewOutline();
        currpos.x = aSize.x / 2;
        currpos.y = aSize.y / 2;
        holeBuffer.Append( VECTOR2I( currpos ) );       // link to hole and begin hole
        currpos.x -= aSize.x;
        holeBuffer.Append( VECTOR2I( currpos ) );
        currpos.y -= aSize.y;
        holeBuffer.Append( VECTOR2I( currpos ) );
        currpos.x += aSize.x;
        holeBuffer.Append( VECTOR2I( currpos ) );
        currpos.y += aSize.y;
        holeBuffer.Append( VECTOR2I( currpos ) );       // close hole
    }

    aPolygon->BooleanSubtract( holeBuffer, SHAPE_POLY_SET::PM_FAST );
    aPolygon->Fracture( SHAPE_POLY_SET::PM_FAST );
}
