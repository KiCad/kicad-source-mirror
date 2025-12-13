/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 <Jean-Pierre Charras>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <base_units.h>
#include <trigo.h>
#include <bitmaps.h>
#include <eda_text.h>
#include <gerbview_frame.h>
#include <convert_basic_shapes_to_polygon.h>
#include <gerber_draw_item.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <string_utils.h>
#include <geometry/shape_arc.h>
#include <math/util.h>      // for KiROUND
#include <widgets/msgpanel.h>

#include <wx/msgdlg.h>

GERBER_DRAW_ITEM::GERBER_DRAW_ITEM( GERBER_FILE_IMAGE* aGerberImageFile ) :
    EDA_ITEM( nullptr, GERBER_DRAW_ITEM_T )
{
    m_GerberImageFile = aGerberImageFile;
    m_ShapeType     = GBR_SEGMENT;
    m_Flashed       = false;
    m_DCode         = 0;
    m_UnitsMetric   = false;
    m_LayerNegative = false;
    m_swapAxis      = false;
    m_mirrorA       = false;
    m_mirrorB       = false;
    m_drawScale.x   = m_drawScale.y = 1.0;
    m_lyrRotation   = 0;

    if( m_GerberImageFile )
        SetLayerParameters();
}


GERBER_DRAW_ITEM::~GERBER_DRAW_ITEM()
{
}


void GERBER_DRAW_ITEM::SetNetAttributes( const GBR_NETLIST_METADATA& aNetAttributes )
{
    m_netAttributes = aNetAttributes;

    if( ( m_netAttributes.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_CMP )
        || ( m_netAttributes.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_PAD ) )
    {
        m_GerberImageFile->m_ComponentsList.insert( std::make_pair( m_netAttributes.m_Cmpref, 0 ) );
    }

    if( ( m_netAttributes.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_NET ) )
        m_GerberImageFile->m_NetnamesList.insert( std::make_pair( m_netAttributes.m_Netname, 0 ) );
}


int GERBER_DRAW_ITEM::GetLayer() const
{
    // Return the layer this item is on, or 0 if the m_GerberImageFile is null.
    return m_GerberImageFile ? m_GerberImageFile->m_GraphicLayer : 0;
}


bool GERBER_DRAW_ITEM::GetTextD_CodePrms( int& aSize, VECTOR2I& aPos, EDA_ANGLE& aOrientation )
{
    // calculate the best size and orientation of the D_Code text

    if( m_DCode <= 0 )
        return false;       // No D_Code for this item

    if( m_Flashed || m_ShapeType == GBR_ARC )
        aPos = m_Start;
    else    // it is a line:
        aPos = ( m_Start + m_End) / 2;

    aPos = GetABPosition( aPos );

    int size;   // the best size for the text

    if( GetDcodeDescr() )
        size = GetDcodeDescr()->GetShapeDim( this );
    else
        size = std::min( m_Size.x, m_Size.y );

    aOrientation = ANGLE_HORIZONTAL;

    if( m_Flashed )
    {
        // A reasonable size for text is min_dim/3 because most of time this text has 3 chars.
        aSize = size / 3;
    }
    else        // this item is a line
    {
        VECTOR2I delta = m_Start - m_End;
        EDA_ANGLE angle( delta );

        aOrientation = angle.Normalize90();

        // A reasonable size for text is size/2 because text needs margin below and above it.
        // a margin = size/4 seems good, expecting the line len is large enough to show 3 chars,
        // that is the case most of time.
        aSize = size / 2;
    }

    return true;
}


VECTOR2I GERBER_DRAW_ITEM::GetABPosition( const VECTOR2I& aXYPosition ) const
{
    /* Note: RS274Xrevd_e is obscure about the order of transforms:
     * For instance: Rotation must be made after or before mirroring ?
     * Note: if something is changed here, GetYXPosition must reflect changes
     */
    VECTOR2I abPos = aXYPosition + m_GerberImageFile->m_ImageJustifyOffset;

    // We have also a draw transform (rotation and offset)
    // order is rotation and after offset

    if( m_swapAxis )
        std::swap( abPos.x, abPos.y );

    abPos  += m_layerOffset + m_GerberImageFile->m_ImageOffset;
    abPos.x = KiROUND( abPos.x * m_drawScale.x );
    abPos.y = KiROUND( abPos.y * m_drawScale.y );
    EDA_ANGLE rotation( m_lyrRotation + m_GerberImageFile->m_ImageRotation, DEGREES_T );

    if( !rotation.IsZero() )
        RotatePoint( abPos, -rotation );

    // Negate A axis if mirrored
    if( m_mirrorA )
        abPos.x = -abPos.x;

    // abPos.y must be negated when no mirror, because draw axis is top to bottom
    if( !m_mirrorB )
        abPos.y = -abPos.y;

    // Now generate the draw transform
    if( !m_GerberImageFile->m_DisplayRotation.IsZero() )
        RotatePoint( abPos, m_GerberImageFile->m_DisplayRotation );

    abPos.x += KiROUND( m_GerberImageFile->m_DisplayOffset.x * m_drawScale.x );
    abPos.y += KiROUND( m_GerberImageFile->m_DisplayOffset.y * m_drawScale.y );

    return abPos;
}


VECTOR2I GERBER_DRAW_ITEM::GetXYPosition( const VECTOR2I& aABPosition ) const
{
    // do the inverse transform made by GetABPosition
    VECTOR2I xyPos = aABPosition;

    // First, undo the draw transform
    xyPos.x -= KiROUND( m_GerberImageFile->m_DisplayOffset.x * m_drawScale.x );
    xyPos.y -= KiROUND( m_GerberImageFile->m_DisplayOffset.y * m_drawScale.y );

    if( !m_GerberImageFile->m_DisplayRotation.IsZero() )
        RotatePoint( xyPos, -m_GerberImageFile->m_DisplayRotation );

    if( m_mirrorA )
        xyPos.x = -xyPos.x;

    if( !m_mirrorB )
        xyPos.y = -xyPos.y;

    EDA_ANGLE rotation( m_lyrRotation + m_GerberImageFile->m_ImageRotation, DEGREES_T );

    if( !rotation.IsZero() )
        RotatePoint( xyPos, rotation );

    xyPos.x = KiROUND( xyPos.x / m_drawScale.x );
    xyPos.y = KiROUND( xyPos.y / m_drawScale.y );
    xyPos  -= m_layerOffset + m_GerberImageFile->m_ImageOffset;

    if( m_swapAxis )
        std::swap( xyPos.x, xyPos.y );

    return xyPos - m_GerberImageFile->m_ImageJustifyOffset;
}


void GERBER_DRAW_ITEM::SetLayerParameters()
{
    m_UnitsMetric = m_GerberImageFile->m_GerbMetric;
    m_swapAxis    = m_GerberImageFile->m_SwapAxis;     // false if A = X, B = Y;

    // true if A =Y, B = Y
    m_mirrorA     = m_GerberImageFile->m_MirrorA;      // true: mirror / axe A
    m_mirrorB     = m_GerberImageFile->m_MirrorB;      // true: mirror / axe B
    m_drawScale   = m_GerberImageFile->m_Scale;        // A and B scaling factor
    m_layerOffset = m_GerberImageFile->m_Offset;       // Offset from OF command

    // Rotation from RO command:
    m_lyrRotation = m_GerberImageFile->m_LocalRotation;
    m_LayerNegative = m_GerberImageFile->GetLayerParams().m_LayerNegative;
}


wxString GERBER_DRAW_ITEM::ShowGBRShape() const
{
    switch( m_ShapeType )
    {
    case GBR_SEGMENT:     return _( "Line" );
    case GBR_ARC:         return _( "Arc" );
    case GBR_CIRCLE:      return _( "Circle" );
    case GBR_SPOT_OVAL:   return wxT( "spot_oval" );
    case GBR_SPOT_CIRCLE: return wxT( "spot_circle" );
    case GBR_SPOT_RECT:   return wxT( "spot_rect" );
    case GBR_SPOT_POLY:   return wxT( "spot_poly" );
    case GBR_POLYGON:     return wxT( "polygon" );

    case GBR_SPOT_MACRO:
    {
        wxString name = wxT( "apt_macro" );
        D_CODE* dcode = GetDcodeDescr();

        if( dcode && dcode->GetMacro() )
            name << wxT(" ") << dcode->GetMacro()->m_AmName;

        return name;
    }

    default:              return wxT( "??" );
    }
}


D_CODE* GERBER_DRAW_ITEM::GetDcodeDescr() const
{
    if( !D_CODE::IsValidDcodeValue( m_DCode ) )
        return nullptr;

    if( m_GerberImageFile == nullptr )
        return nullptr;

    return m_GerberImageFile->GetDCODE( m_DCode );
}


const BOX2I GERBER_DRAW_ITEM::GetBoundingBox() const
{
    // return a rectangle which is (pos,dim) in nature.  therefore the +1
    BOX2I   bbox( m_Start, VECTOR2I( 1, 1 ) );
    D_CODE* code = GetDcodeDescr();

    // TODO(JE) GERBER_DRAW_ITEM maybe should actually be a number of subclasses.
    // Until/unless that is changed, we need to do different things depending on
    // what is actually being represented by this GERBER_DRAW_ITEM.

    switch( m_ShapeType )
    {
    case GBR_POLYGON:
    {
        BOX2I bb = m_ShapeAsPolygon.BBox();
        bbox.Inflate( bb.GetWidth() / 2, bb.GetHeight() / 2 );
        bbox.SetOrigin( bb.GetOrigin() );
        break;
    }

    case GBR_CIRCLE:
    {
        double radius = m_Start.Distance( m_End );
        bbox.Inflate( radius, radius );
        break;
    }

    case GBR_ARC:
    {
        EDA_ANGLE angle( atan2( double( m_End.y - m_ArcCentre.y ),
                                double( m_End.x - m_ArcCentre.x ) )
                         - atan2( double( m_Start.y - m_ArcCentre.y ),
                                  double( m_Start.x - m_ArcCentre.x ) ),
                         RADIANS_T );

        if( m_End == m_Start )  // Arc with the end point = start point is expected to be a circle.
            angle = ANGLE_360;
        else
            angle.Normalize();

        SHAPE_ARC arc( m_ArcCentre, m_Start, angle );
        bbox = arc.BBox( m_Size.x / 2 );  // m_Size.x is the line thickness
        break;
    }

    case GBR_SPOT_CIRCLE:
        if( code )
        {
            int radius = code->m_Size.x >> 1;
            bbox.Inflate( radius, radius );
        }

        break;

    case GBR_SPOT_RECT:
        if( code )
            bbox.Inflate( code->m_Size.x / 2, code->m_Size.y / 2 );

        break;

    case GBR_SPOT_OVAL:
        if( code )
            bbox.Inflate( code->m_Size.x /2, code->m_Size.y / 2 );

        break;

    case GBR_SPOT_MACRO:
    case GBR_SPOT_POLY:
        if( code )
        {
            if( code->m_Polygon.OutlineCount() == 0 )
                code->ConvertShapeToPolygon( this );

            bbox.Inflate( code->m_Polygon.BBox().GetWidth() / 2,
                          code->m_Polygon.BBox().GetHeight() / 2 );
        }

        break;

    case GBR_SEGMENT:
        if( code && code->m_ApertType == APT_RECT )
        {
            if( m_ShapeAsPolygon.OutlineCount() == 0 )
            {
                // We cannot initialize m_ShapeAsPolygon, because we are in a const function.
                // So use a temporary polygon
                SHAPE_POLY_SET poly_shape;
                ConvertSegmentToPolygon( &poly_shape );
                bbox = poly_shape.BBox();
            }

            else
            {
                bbox = m_ShapeAsPolygon.BBox();
            }
        }
        else
        {
            int radius = ( m_Size.x + 1 ) / 2;

            int ymax = std::max( m_Start.y, m_End.y ) + radius;
            int xmax = std::max( m_Start.x, m_End.x ) + radius;

            int ymin = std::min( m_Start.y, m_End.y ) - radius;
            int xmin = std::min( m_Start.x, m_End.x ) - radius;

            bbox = BOX2I( VECTOR2I( xmin, ymin ), VECTOR2I( xmax - xmin + 1, ymax - ymin + 1 ) );
        }

        break;

    default:
        wxASSERT_MSG( false, wxT( "GERBER_DRAW_ITEM shape is unknown!" ) );
        break;
    }

    // calculate the corners coordinates in current Gerber axis orientations
    // because the initial bbox is a horizontal rect, but the bbox in AB position
    // is the bbox image with perhaps a rotation, we need to calculate the coords of the
    // corners of the bbox rotated, and then calculate the final bounding box
    VECTOR2I corners[4];
    bbox.Normalize();

    // Shape:
    // 0...1
    // .   .
    // .   .
    // 3...2
    corners[0] = bbox.GetOrigin();  // top left
    corners[2] = bbox.GetEnd();     // bottom right
    corners[1] = VECTOR2I( corners[2].x, corners[0].y );    // top right
    corners[3] = VECTOR2I( corners[0].x, corners[2].y );    // bottom left

    VECTOR2I org = GetABPosition( bbox.GetOrigin() );;
    VECTOR2I end = GetABPosition( bbox.GetEnd() );;

    // Now calculate the bounding box of bbox, if the display image is rotated
    // It will be perhaps a bit bigger than a better bounding box calculation, but it is fast
    // and easy
    // (if not rotated, this is a nop)
    for( int ii = 0; ii < 4; ii++ )
    {
        corners[ii] = GetABPosition( corners[ii] );

        org.x = std::min( org.x, corners[ii].x );
        org.y = std::min( org.y, corners[ii].y );

        end.x = std::max( end.x, corners[ii].x );
        end.y = std::max( end.y, corners[ii].y );
    }

    // Set the corners position:
    bbox.SetOrigin( org );
    bbox.SetEnd( end );
    bbox.Normalize();

    return bbox;
}


void GERBER_DRAW_ITEM::MoveXY( const VECTOR2I& aMoveVector )
{
    m_Start     += aMoveVector;
    m_End       += aMoveVector;
    m_ArcCentre += aMoveVector;

    m_ShapeAsPolygon.Move( aMoveVector );
}


bool GERBER_DRAW_ITEM::HasNegativeItems()
{
    bool isClear = m_LayerNegative ^ m_GerberImageFile->m_ImageNegative;

    // if isClear is true, this item has negative shape
    return isClear;
}


void GERBER_DRAW_ITEM::Print( wxDC* aDC, const VECTOR2I& aOffset, GBR_DISPLAY_OPTIONS* aOptions )
{
    // used when a D_CODE is not found. default D_CODE to draw a flashed item
    static D_CODE dummyD_CODE( 0 );
    bool          isFilled;
    int           radius;
    int           halfPenWidth;
    static bool   show_err;
    D_CODE*       d_codeDescr = GetDcodeDescr();

    if( d_codeDescr == nullptr )
        d_codeDescr = &dummyD_CODE;

    COLOR4D color = m_GerberImageFile->GetPositiveDrawColor();

    /* isDark is true if flash is positive and should use a drawing
     *   color other than the background color, else use the background color
     *   when drawing so that an erasure happens.
     */
    bool isDark = !(m_LayerNegative ^ m_GerberImageFile->m_ImageNegative);

    if( !isDark )
    {
        // draw in background color ("negative" color)
        color = aOptions->m_NegativeDrawColor;
    }

    isFilled = aOptions->m_DisplayLinesFill;

    switch( m_ShapeType )
    {
    case GBR_POLYGON:
        isFilled = aOptions->m_DisplayPolygonsFill;

        if( !isDark )
            isFilled = true;

        PrintGerberPoly( aDC, color, aOffset, isFilled );
        break;

    case GBR_CIRCLE:
        radius = KiROUND( m_Start.Distance( m_End ) );

        halfPenWidth = m_Size.x >> 1;

        if( !isFilled )
        {
            // draw the border of the pen's path using two circles, each as narrow as possible
            GRCircle( aDC, GetABPosition( m_Start ), radius - halfPenWidth, 0, color );
            GRCircle( aDC, GetABPosition( m_Start ), radius + halfPenWidth, 0, color );
        }
        else    // Filled mode
        {
            GRCircle( aDC, GetABPosition( m_Start ), radius, m_Size.x, color );
        }

        break;

    case GBR_ARC:
        // Currently, arcs plotted with a rectangular aperture are not supported.
        // a round pen only is expected.
        if( !isFilled )
        {
            GRArc( aDC, GetABPosition( m_Start ), GetABPosition( m_End ),
                   GetABPosition( m_ArcCentre ), 0, color );
        }
        else
        {
            GRArc( aDC, GetABPosition( m_Start ), GetABPosition( m_End ),
                   GetABPosition( m_ArcCentre ), m_Size.x, color );
        }

        break;

    case GBR_SPOT_CIRCLE:
    case GBR_SPOT_RECT:
    case GBR_SPOT_OVAL:
    case GBR_SPOT_POLY:
    case GBR_SPOT_MACRO:
        isFilled = aOptions->m_DisplayFlashedItemsFill;
        d_codeDescr->DrawFlashedShape( this, aDC, color, m_Start, isFilled );
        break;

    case GBR_SEGMENT:
        /* Plot a line from m_Start to m_End.
         * Usually, a round pen is used, but some Gerber files use a rectangular pen
         * In fact, any aperture can be used to plot a line.
         * currently: only a square pen is handled (I believe using a polygon gives a strange plot).
         */
        if( d_codeDescr->m_ApertType == APT_RECT )
        {
            if( m_ShapeAsPolygon.OutlineCount() == 0 )
                ConvertSegmentToPolygon();

            PrintGerberPoly( aDC, color, aOffset, isFilled );
        }
        else if( !isFilled )
        {
            GRCSegm( aDC, GetABPosition( m_Start ), GetABPosition( m_End ), m_Size.x, color );
        }
        else
        {
            GRFilledSegment( aDC, GetABPosition( m_Start ), GetABPosition( m_End ), m_Size.x,
                             color );
        }

        break;

    default:
        if( !show_err )
        {
            wxMessageBox( wxT( "Trace_Segment() type error" ) );
            show_err = true;
        }

        break;
    }
}


void GERBER_DRAW_ITEM::ConvertSegmentToPolygon( SHAPE_POLY_SET* aPolygon ) const
{
    aPolygon->RemoveAllContours();
    aPolygon->NewOutline();

    VECTOR2I start = m_Start;
    VECTOR2I end = m_End;

    // make calculations more easy if ensure start.x < end.x
    // (only 2 quadrants to consider)
    if( start.x > end.x )
        std::swap( start, end );

    // calculate values relative to start point:
    VECTOR2I delta = end - start;

    // calculate corners for the first quadrant only (delta.x and delta.y > 0 )
    // currently, delta.x already is > 0.
    // make delta.y > 0
    bool change = delta.y < 0;

    if( change )
        delta.y = -delta.y;

    // Now create the full polygon.
    // Due to previous changes, the shape is always something like
    // 3 4
    // 2 5
    // 1 6
    VECTOR2I corner;
    corner.x -= m_Size.x/2;
    corner.y -= m_Size.y/2;
    VECTOR2I close = corner;
    aPolygon->Append( VECTOR2I( corner ) );  // Lower left corner, start point (1)
    corner.y += m_Size.y;
    aPolygon->Append( VECTOR2I( corner ) );  // upper left corner, start point (2)

    if( delta.x || delta.y )
    {
        corner += delta;
        aPolygon->Append( VECTOR2I( corner ) );  // upper left corner, end point (3)
    }

    corner.x += m_Size.x;
    aPolygon->Append( VECTOR2I( corner ) );  // upper right corner, end point (4)
    corner.y -= m_Size.y;
    aPolygon->Append( VECTOR2I( corner ) );  // lower right corner, end point (5)

    if( delta.x || delta.y )
    {
        corner -= delta;
        aPolygon->Append( VECTOR2I( corner ) );  // lower left corner, start point (6)
    }

    aPolygon->Append( VECTOR2I( close ) );  // close the shape

    // Create final polygon:
    if( change )
        aPolygon->Mirror( { 0, 0 }, FLIP_DIRECTION::TOP_BOTTOM );

    aPolygon->Move( VECTOR2I( start ) );
}


void GERBER_DRAW_ITEM::ConvertSegmentToPolygon()
{
    ConvertSegmentToPolygon( &m_ShapeAsPolygon );
}


void GERBER_DRAW_ITEM::PrintGerberPoly( wxDC* aDC, const COLOR4D& aColor, const VECTOR2I& aOffset,
                                        bool aFilledShape )
{
    std::vector<VECTOR2I> points;
    SHAPE_LINE_CHAIN& poly = m_ShapeAsPolygon.Outline( 0 );
    int pointCount = poly.PointCount() - 1;

    points.reserve( pointCount );

    for( int ii = 0; ii < pointCount; ii++ )
    {
        VECTOR2I p( poly.CPoint( ii ).x, poly.CPoint( ii ).y );
        points[ii] = p + aOffset;
        points[ii] = GetABPosition( points[ii] );
    }

    GRClosedPoly( aDC, pointCount, &points[0], aFilledShape, aColor );
}


void GERBER_DRAW_ITEM::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;
    wxString text;

    msg = ShowGBRShape();
    aList.emplace_back( _( "Type" ), msg );

    // Display D_Code value with its attributes for items using a DCode:
    if( m_ShapeType == GBR_POLYGON )    // Has no DCode, but can have an attribute
    {
        msg = _( "Attribute" );

        if( m_AperFunction.IsEmpty() )
            text = _( "No attribute" );
        else
            text = m_AperFunction;
    }
    else
    {
        msg.Printf( _( "D Code %d" ), m_DCode );
        D_CODE* apertDescr = GetDcodeDescr();

        if( !apertDescr || apertDescr->m_AperFunction.IsEmpty() )
            text = _( "No attribute" );
        else
            text = apertDescr->m_AperFunction;
    }

    aList.emplace_back( msg, text );

    // Display graphic layer name
    msg = GERBER_FILE_IMAGE_LIST::GetImagesList().GetDisplayName( GetLayer(), true );
    aList.emplace_back( _( "Graphic Layer" ), msg );

    // Display item position
    auto xStart = EDA_UNIT_UTILS::UI::ToUserUnit( gerbIUScale, aFrame->GetUserUnits(), m_Start.x );
    auto yStart = EDA_UNIT_UTILS::UI::ToUserUnit( gerbIUScale, aFrame->GetUserUnits(), m_Start.y );
    auto xEnd = EDA_UNIT_UTILS::UI::ToUserUnit( gerbIUScale, aFrame->GetUserUnits(), m_End.x );
    auto yEnd = EDA_UNIT_UTILS::UI::ToUserUnit( gerbIUScale, aFrame->GetUserUnits(), m_End.y );

    if( m_Flashed )
    {
        msg.Printf( wxT( "(%.4f, %.4f)" ), xStart, yStart );
        aList.emplace_back( _( "Position" ), msg );
    }
    else
    {
        msg.Printf( wxT( "(%.4f, %.4f)" ), xStart, yStart );
        aList.emplace_back( _( "Start" ), msg );

        msg.Printf( wxT( "(%.4f, %.4f)" ), xEnd, yEnd );
        aList.emplace_back( _( "End" ), msg );
    }

    // Display item rotation
    // The full rotation is Image rotation + m_lyrRotation
    // but m_lyrRotation is specific to this object
    // so we display only this parameter
    msg.Printf( wxT( "%f" ), m_lyrRotation );
    aList.emplace_back( _( "Rotation" ), msg );

    // Display item polarity (item specific)
    msg = m_LayerNegative ? _("Clear") : _("Dark");
    aList.emplace_back( _( "Polarity" ), msg );

    // Display mirroring (item specific)
    msg.Printf( wxT( "A:%s B:%s" ), m_mirrorA ? _( "Yes" ) : _( "No" ),
                m_mirrorB ? _( "Yes" ) : _( "No" ) );
    aList.emplace_back( _( "Mirror" ), msg );

    // Display AB axis swap (item specific)
    msg = m_swapAxis ? wxT( "A=Y B=X" ) : wxT( "A=X B=Y" );
    aList.emplace_back( _( "AB axis" ), msg );

    // Display net info, if exists
    if( m_netAttributes.m_NetAttribType == GBR_NETLIST_METADATA::GBR_NETINFO_UNSPECIFIED )
        return;

    // Build full net info:
    wxString net_msg;
    wxString cmp_pad_msg;

    if( ( m_netAttributes.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_NET ) )
    {
        net_msg = _( "Net:" );
        net_msg << wxS( " " );

        if( m_netAttributes.m_Netname.IsEmpty() )
            net_msg << _( "<no net>" );
        else
            net_msg << UnescapeString( m_netAttributes.m_Netname );
    }

    if( ( m_netAttributes.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_PAD ) )
    {
        if( m_netAttributes.m_PadPinFunction.IsEmpty() )
        {
            cmp_pad_msg.Printf( _( "Cmp: %s  Pad: %s" ),
                                m_netAttributes.m_Cmpref,
                                m_netAttributes.m_Padname.GetValue() );
        }
        else
        {
            cmp_pad_msg.Printf( _( "Cmp: %s  Pad: %s  Fct %s" ),
                                m_netAttributes.m_Cmpref,
                                m_netAttributes.m_Padname.GetValue(),
                                m_netAttributes.m_PadPinFunction.GetValue() );
        }
    }

    else if( ( m_netAttributes.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_CMP ) )
    {
        cmp_pad_msg = _( "Cmp:" );
        cmp_pad_msg << wxS( " " ) << m_netAttributes.m_Cmpref;
    }

    aList.emplace_back( net_msg, cmp_pad_msg );
}


BITMAPS GERBER_DRAW_ITEM::GetMenuImage() const
{
    if( m_Flashed )
        return BITMAPS::pad;

    switch( m_ShapeType )
    {
    case GBR_SEGMENT:
    case GBR_ARC:
    case GBR_CIRCLE:
        return BITMAPS::add_line;

    case GBR_SPOT_OVAL:
    case GBR_SPOT_CIRCLE:
    case GBR_SPOT_RECT:
    case GBR_SPOT_POLY:
    case GBR_SPOT_MACRO:
        // should be handles by m_Flashed == true
        return BITMAPS::pad;

    case GBR_POLYGON:
        return BITMAPS::add_graphical_polygon;
    }

    return BITMAPS::info;
}


bool GERBER_DRAW_ITEM::HitTest( const VECTOR2I& aRefPos, int aAccuracy ) const
{
    // In case the item has a very tiny width defined, allow it to be selected
    const int MIN_HIT_TEST_RADIUS = gerbIUScale.mmToIU( 0.01 );

    // calculate aRefPos in XY Gerber axis:
    VECTOR2I ref_pos = GetXYPosition( aRefPos );

    SHAPE_POLY_SET poly;

    switch( m_ShapeType )
    {
    case GBR_POLYGON:
        poly = m_ShapeAsPolygon;
        return poly.Contains( VECTOR2I( ref_pos ), 0, aAccuracy );

    case GBR_SPOT_POLY:
        poly = GetDcodeDescr()->m_Polygon;
        poly.Move( VECTOR2I( m_Start ) );
        return poly.Contains( VECTOR2I( ref_pos ), 0, aAccuracy );

    case GBR_SPOT_RECT:
        return GetBoundingBox().Contains( aRefPos );

    case GBR_SPOT_OVAL:
    {
        BOX2I bbox = GetBoundingBox();

        if( ! bbox.Contains( aRefPos ) )
            return false;

        // This is similar to a segment with thickness = min( m_Size.x, m_Size.y )
        int radius = std::min( m_Size.x, m_Size.y )/2;
        VECTOR2I start, end;

        if( m_Size.x > m_Size.y )   // Horizontal oval
        {
            int len = m_Size.y - m_Size.x;
            start.x = -len/2;
            end.x = len/2;
        }
        else   // Vertical oval
        {
            int len = m_Size.x - m_Size.y;
            start.y = -len/2;
            end.y = len/2;
        }

        start += bbox.Centre();
        end += bbox.Centre();

        if( radius < MIN_HIT_TEST_RADIUS )
            radius = MIN_HIT_TEST_RADIUS;

        return TestSegmentHit( aRefPos, start, end, radius );
    }

    case GBR_ARC:
    {
        double radius = m_Start.Distance( m_ArcCentre );
        VECTOR2D test_radius = VECTOR2D( ref_pos ) - VECTOR2D( m_ArcCentre );

        int size = ( ( m_Size.x < MIN_HIT_TEST_RADIUS ) ? MIN_HIT_TEST_RADIUS : m_Size.x );

        // Are we close enough to the radius?
        bool radius_hit = ( std::fabs( test_radius.EuclideanNorm() - radius) < size );

        if( radius_hit )
        {
            // Now check that we are within the arc angle

            VECTOR2D  start = VECTOR2D( m_Start ) - VECTOR2D( m_ArcCentre );
            VECTOR2D  end = VECTOR2D( m_End ) - VECTOR2D( m_ArcCentre );
            EDA_ANGLE start_angle( start );
            EDA_ANGLE end_angle( end );

            start_angle.Normalize();
            end_angle.Normalize();

            if( m_Start == m_End )
            {
                start_angle = ANGLE_0;
                end_angle = ANGLE_360;
            }
            else if( end_angle < start_angle )
            {
                end_angle += ANGLE_360;
            }

            EDA_ANGLE test_angle( test_radius );
            test_angle.Normalize();

            return ( test_angle > start_angle && test_angle < end_angle );
        }

        return false;
    }

    case GBR_SPOT_MACRO:
        return m_AbsolutePolygon.Contains( VECTOR2I( aRefPos ), -1, aAccuracy );

    case GBR_SEGMENT:
    case GBR_CIRCLE:
    case GBR_SPOT_CIRCLE:
        break;  // handled below.
    }

    // TODO: a better analyze of the shape (perhaps create a D_CODE::HitTest for flashed items)
    int radius = std::min( m_Size.x, m_Size.y ) >> 1;

    if( radius < MIN_HIT_TEST_RADIUS )
        radius = MIN_HIT_TEST_RADIUS;

    if( m_Flashed )
        return m_Start.Distance( ref_pos ) <= radius;
    else
        return TestSegmentHit( ref_pos, m_Start, m_End, radius );
}


bool GERBER_DRAW_ITEM::HitTest( const BOX2I& aRefArea, bool aContained, int aAccuracy ) const
{
    VECTOR2I pos = GetABPosition( m_Start );

    if( aRefArea.Contains( pos ) )
        return true;

    pos = GetABPosition( m_End );

    if( aRefArea.Contains( pos ) )
        return true;

    return false;
}


#if defined(DEBUG)

void GERBER_DRAW_ITEM::Show( int nestLevel, std::ostream& os ) const
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<
                                 " shape=\"" << m_ShapeType << '"' <<
                                 " addr=\"" << std::hex << this << std::dec << '"' <<
                                 " layer=\"" << GetLayer() << '"' <<
                                 " size=\"" << m_Size << '"' <<
                                 " flags=\"" << m_flags << '"' <<
    "<start" << m_Start << "/>" <<
    "<end" << m_End << "/>";

    os << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif


std::vector<int> GERBER_DRAW_ITEM::ViewGetLayers() const
{
    std::vector<int> layers( 2 );
    layers[0] = GERBER_DRAW_LAYER( GetLayer() );
    layers[1] = GERBER_DCODE_LAYER( layers[0] );

    return layers;
}


const BOX2I GERBER_DRAW_ITEM::ViewBBox() const
{
    return GetBoundingBox();
}


double GERBER_DRAW_ITEM::ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const
{
    // DCodes will be shown only if zoom is appropriate:
    // Returns the level of detail of the item.
    // A level of detail (LOD) is the minimal VIEW scale that
    // is sufficient for an item to be shown on a given layer.
    if( IsDCodeLayer( aLayer ) )
    {
        int size = 0;

        switch( m_ShapeType )
        {
        case GBR_SPOT_MACRO:
            size = GetDcodeDescr()->m_Polygon.BBox().GetWidth();
            break;

        case GBR_ARC:
            size = m_Start.Distance( m_ArcCentre );
            break;

        default:
            size = m_Size.x;
        }

        // the level of details is chosen experimentally, to show
        // only a readable text:
        return lodScaleForThreshold( aView, size, gerbIUScale.mmToIU( 3.0 ) );
    }

    // Other layers are shown without any conditions
    return LOD_SHOW;
}


INSPECT_RESULT GERBER_DRAW_ITEM::Visit( INSPECTOR inspector, void* testData,
                                        const std::vector<KICAD_T>& aScanTypes )
{
    for( KICAD_T scanType : aScanTypes )
    {
        if( scanType == Type() )
        {
            if( INSPECT_RESULT::QUIT == inspector( this, testData ) )
                return INSPECT_RESULT::QUIT;
        }
    }

    return INSPECT_RESULT::CONTINUE;
}


wxString GERBER_DRAW_ITEM::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    wxString layerName = GERBER_FILE_IMAGE_LIST::GetImagesList().GetDisplayName( GetLayer(), true );

    return wxString::Format( _( "%s (D%d) on layer %d: %s" ),
                             ShowGBRShape(),
                             m_DCode,
                             GetLayer() + 1,
                             layerName );
}
