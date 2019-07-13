/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 <Jean-Pierre Charras>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <trigo.h>
#include <bitmaps.h>
#include <msgpanel.h>
#include <gerbview_frame.h>
#include <convert_basic_shapes_to_polygon.h>
#include <gerber_draw_item.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <kicad_string.h>

GERBER_DRAW_ITEM::GERBER_DRAW_ITEM( GERBER_FILE_IMAGE* aGerberImageFile ) :
    EDA_ITEM( (EDA_ITEM*)NULL, GERBER_DRAW_ITEM_T )
{
    m_GerberImageFile = aGerberImageFile;
    m_Shape         = GBR_SEGMENT;
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

    if( ( m_netAttributes.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_CMP ) ||
        ( m_netAttributes.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_PAD ) )
        m_GerberImageFile->m_ComponentsList.insert( std::make_pair( m_netAttributes.m_Cmpref, 0 ) );

    if( ( m_netAttributes.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_NET ) )
        m_GerberImageFile->m_NetnamesList.insert( std::make_pair( m_netAttributes.m_Netname, 0 ) );
}


int GERBER_DRAW_ITEM::GetLayer() const
{
    // returns the layer this item is on, or 0 if the m_GerberImageFile is NULL.
    return m_GerberImageFile ? m_GerberImageFile->m_GraphicLayer : 0;
}


bool GERBER_DRAW_ITEM::GetTextD_CodePrms( int& aSize, wxPoint& aPos, double& aOrientation )
{
    // calculate the best size and orientation of the D_Code text

    if( m_DCode <= 0 )
        return false;       // No D_Code for this item

    if( m_Flashed || m_Shape == GBR_ARC )
    {
        aPos = m_Start;
    }
    else    // it is a line:
    {
        aPos = ( m_Start + m_End) / 2;
    }

    aPos = GetABPosition( aPos );

    int size;   // the best size for the text

    if( GetDcodeDescr() )
        size = GetDcodeDescr()->GetShapeDim( this );
    else
        size = std::min( m_Size.x, m_Size.y );

    aOrientation = TEXT_ANGLE_HORIZ;

    if( m_Flashed )
    {
        // A reasonable size for text is min_dim/3 because most of time this text has 3 chars.
        aSize = size / 3;
    }
    else        // this item is a line
    {
        wxPoint delta = m_Start - m_End;

        aOrientation = RAD2DECIDEG( atan2( (double)delta.y, (double)delta.x ) );
        NORMALIZE_ANGLE_90( aOrientation );

        // A reasonable size for text is size/2 because text needs margin below and above it.
        // a margin = size/4 seems good, expecting the line len is large enough to show 3 chars,
        // that is the case most of time.
        aSize = size / 2;
    }

    return true;
}


bool GERBER_DRAW_ITEM::GetTextD_CodePrms( double& aSize, VECTOR2D& aPos, double& aOrientation )
{
    // aOrientation is returned in radians
    int size;
    wxPoint pos;

    if( ! GetTextD_CodePrms( size, pos, aOrientation ) )
        return false;

    aPos = pos;
    aSize = (double) size;
    aOrientation = DECIDEG2RAD( aOrientation );

    return true;
}


wxPoint GERBER_DRAW_ITEM::GetABPosition( const wxPoint& aXYPosition ) const
{
    /* Note: RS274Xrevd_e is obscure about the order of transforms:
     * For instance: Rotation must be made after or before mirroring ?
     * Note: if something is changed here, GetYXPosition must reflect changes
     */
    wxPoint abPos = aXYPosition + m_GerberImageFile->m_ImageJustifyOffset;

    if( m_swapAxis )
        std::swap( abPos.x, abPos.y );

    abPos  += m_layerOffset + m_GerberImageFile->m_ImageOffset;
    abPos.x = KiROUND( abPos.x * m_drawScale.x );
    abPos.y = KiROUND( abPos.y * m_drawScale.y );
    double rotation = m_lyrRotation * 10 + m_GerberImageFile->m_ImageRotation * 10;

    if( rotation )
        RotatePoint( &abPos, -rotation );

    // Negate A axis if mirrored
    if( m_mirrorA )
        abPos.x = -abPos.x;

    // abPos.y must be negated when no mirror, because draw axis is top to bottom
    if( !m_mirrorB )
        abPos.y = -abPos.y;
    return abPos;
}


wxPoint GERBER_DRAW_ITEM::GetXYPosition( const wxPoint& aABPosition ) const
{
    // do the inverse transform made by GetABPosition
    wxPoint xyPos = aABPosition;

    if( m_mirrorA )
        xyPos.x = -xyPos.x;

    if( !m_mirrorB )
        xyPos.y = -xyPos.y;

    double rotation = m_lyrRotation * 10 + m_GerberImageFile->m_ImageRotation * 10;

    if( rotation )
        RotatePoint( &xyPos, rotation );

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
    switch( m_Shape )
    {
    case GBR_SEGMENT:
        return _( "Line" );

    case GBR_ARC:
        return _( "Arc" );

    case GBR_CIRCLE:
        return _( "Circle" );

    case GBR_SPOT_OVAL:
        return wxT( "spot_oval" );

    case GBR_SPOT_CIRCLE:
        return wxT( "spot_circle" );

    case GBR_SPOT_RECT:
        return wxT( "spot_rect" );

    case GBR_SPOT_POLY:
        return wxT( "spot_poly" );

    case GBR_POLYGON:
        return wxT( "polygon" );

    case GBR_SPOT_MACRO:
    {
        wxString name = wxT( "apt_macro" );
        D_CODE* dcode = GetDcodeDescr();

        if( dcode && dcode->GetMacro() )
            name << wxT(" ") << dcode->GetMacro()->name;

        return name;
    }

    default:
        return wxT( "??" );
    }
}


D_CODE* GERBER_DRAW_ITEM::GetDcodeDescr() const
{
    if( (m_DCode < FIRST_DCODE) || (m_DCode > LAST_DCODE) )
        return NULL;

    if( m_GerberImageFile == NULL )
        return NULL;

    return m_GerberImageFile->GetDCODE( m_DCode );
}


const EDA_RECT GERBER_DRAW_ITEM::GetBoundingBox() const
{
    // return a rectangle which is (pos,dim) in nature.  therefore the +1
    EDA_RECT bbox( m_Start, wxSize( 1, 1 ) );
    D_CODE* code = GetDcodeDescr();

    // TODO(JE) GERBER_DRAW_ITEM maybe should actually be a number of subclasses.
    // Until/unless that is changed, we need to do different things depending on
    // what is actually being represented by this GERBER_DRAW_ITEM.

    switch( m_Shape )
    {
    case GBR_POLYGON:
    {
        auto bb = m_Polygon.BBox();
        bbox.Inflate( bb.GetWidth() / 2, bb.GetHeight() / 2 );
        bbox.SetOrigin( bb.GetOrigin().x, bb.GetOrigin().y );
        break;
    }

    case GBR_CIRCLE:
    {
        double radius = GetLineLength( m_Start, m_End );
        bbox.Inflate( radius, radius );
        break;
    }

    case GBR_ARC:
    {
        // Note: using a larger-than-necessary BB to simplify computation
        double radius = GetLineLength( m_Start, m_ArcCentre );
        bbox.Move( m_ArcCentre - m_Start );
        bbox.Inflate( radius + m_Size.x, radius + m_Size.x );
        break;
    }

    case GBR_SPOT_CIRCLE:
    {
        if( code )
        {
            int radius = code->m_Size.x >> 1;
            bbox.Inflate( radius, radius );
        }
        break;
    }

    case GBR_SPOT_RECT:
    {
        if( code )
            bbox.Inflate( code->m_Size.x / 2, code->m_Size.y / 2 );
        break;
    }

    case GBR_SPOT_OVAL:
    {
        if( code )
            bbox.Inflate( code->m_Size.x, code->m_Size.y );
        break;
    }

    case GBR_SPOT_POLY:
    {
        if( code )
        {
            if( code->m_Polygon.OutlineCount() == 0 )
                code->ConvertShapeToPolygon();

            bbox.Inflate( code->m_Polygon.BBox().GetWidth() / 2,
                          code->m_Polygon.BBox().GetHeight() / 2 );
        }
        break;
    }
    case GBR_SPOT_MACRO:
    {
        if( code )
        {
            // Update the shape drawings and the bounding box coordiantes:
            code->GetMacro()->GetApertureMacroShape( this, m_Start );
            // now the bounding box is valid:
            bbox = code->GetMacro()->GetBoundingBox();
        }
        break;
    }

    case GBR_SEGMENT:
    {
        if( code && code->m_Shape == APT_RECT )
        {
            if( m_Polygon.OutlineCount() > 0 )
            {
                auto bb = m_Polygon.BBox();
                bbox.Inflate( bb.GetWidth() / 2, bb.GetHeight() / 2 );
                bbox.SetOrigin( bb.GetOrigin().x, bb.GetOrigin().y );
            }
        }
        else
        {
            int radius = ( m_Size.x + 1 ) / 2;

            int ymax = std::max( m_Start.y, m_End.y ) + radius;
            int xmax = std::max( m_Start.x, m_End.x ) + radius;

            int ymin = std::min( m_Start.y, m_End.y ) - radius;
            int xmin = std::min( m_Start.x, m_End.x ) - radius;

            bbox = EDA_RECT( wxPoint( xmin, ymin ), wxSize( xmax - xmin + 1, ymax - ymin + 1 ) );
        }

        break;
    }
    default:
        wxASSERT_MSG( false, wxT( "GERBER_DRAW_ITEM shape is unknown!" ) );
        break;
    }

    // calculate the corners coordinates in current gerber axis orientations
    wxPoint org = GetABPosition( bbox.GetOrigin() );
    wxPoint end = GetABPosition( bbox.GetEnd() );

    // Set the corners position:
    bbox.SetOrigin( org );
    bbox.SetEnd( end );
    bbox.Normalize();

    return bbox;
}


void GERBER_DRAW_ITEM::MoveAB( const wxPoint& aMoveVector )
{
    wxPoint xymove = GetXYPosition( aMoveVector );

    m_Start     += xymove;
    m_End       += xymove;
    m_ArcCentre += xymove;

    if( m_Polygon.OutlineCount() > 0 )
    {
        for( auto it = m_Polygon.Iterate( 0 ); it; ++it )
            *it += xymove;
    }
}


void GERBER_DRAW_ITEM::MoveXY( const wxPoint& aMoveVector )
{
    m_Start     += aMoveVector;
    m_End       += aMoveVector;
    m_ArcCentre += aMoveVector;

    if( m_Polygon.OutlineCount() > 0 )
    {
        for( auto it = m_Polygon.Iterate( 0 ); it; ++it )
            *it += aMoveVector;
    }
}


bool GERBER_DRAW_ITEM::HasNegativeItems()
{
    bool isClear = m_LayerNegative ^ m_GerberImageFile->m_ImageNegative;

    // if isClear is true, this item has negative shape
    return isClear;
}


void GERBER_DRAW_ITEM::Print( wxDC* aDC, const wxPoint& aOffset, GBR_DISPLAY_OPTIONS* aOptions )
{
    // used when a D_CODE is not found. default D_CODE to draw a flashed item
    static D_CODE dummyD_CODE( 0 );
    bool          isFilled;
    int           radius;
    int           halfPenWidth;
    static bool   show_err;
    D_CODE*       d_codeDescr = GetDcodeDescr();

    if( d_codeDescr == NULL )
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

    switch( m_Shape )
    {
    case GBR_POLYGON:
        isFilled = aOptions->m_DisplayPolygonsFill;

        if( !isDark )
            isFilled = true;

        PrintGerberPoly( aDC, color, aOffset, isFilled );
        break;

    case GBR_CIRCLE:
        radius = KiROUND( GetLineLength( m_Start, m_End ) );

        halfPenWidth = m_Size.x >> 1;

        if( !isFilled )
        {
            // draw the border of the pen's path using two circles, each as narrow as possible
            GRCircle( nullptr, aDC, GetABPosition( m_Start ), radius - halfPenWidth, 0, color );
            GRCircle( nullptr, aDC, GetABPosition( m_Start ), radius + halfPenWidth, 0, color );
        }
        else    // Filled mode
        {
            GRCircle( nullptr, aDC, GetABPosition( m_Start ), radius, m_Size.x, color );
        }
        break;

    case GBR_ARC:
        // Currently, arcs plotted with a rectangular aperture are not supported.
        // a round pen only is expected.
        if( !isFilled )
        {
            GRArc1( nullptr, aDC, GetABPosition( m_Start ), GetABPosition( m_End ),
                    GetABPosition( m_ArcCentre ), 0, color );
        }
        else
        {
            GRArc1( nullptr, aDC, GetABPosition( m_Start ), GetABPosition( m_End ),
                    GetABPosition( m_ArcCentre ), m_Size.x, color );
        }

        break;

    case GBR_SPOT_CIRCLE:
    case GBR_SPOT_RECT:
    case GBR_SPOT_OVAL:
    case GBR_SPOT_POLY:
    case GBR_SPOT_MACRO:
        isFilled = aOptions->m_DisplayFlashedItemsFill;
        d_codeDescr->DrawFlashedShape( this, nullptr, aDC, color, m_Start, isFilled );
        break;

    case GBR_SEGMENT:
        /* Plot a line from m_Start to m_End.
         * Usually, a round pen is used, but some gerber files use a rectangular pen
         * In fact, any aperture can be used to plot a line.
         * currently: only a square pen is handled (I believe using a polygon gives a strange plot).
         */
        if( d_codeDescr->m_Shape == APT_RECT )
        {
            if( m_Polygon.OutlineCount() == 0 )
                ConvertSegmentToPolygon();

            PrintGerberPoly( aDC, color, aOffset, isFilled );
        }
        else
        {
            if( !isFilled )
            {
                GRCSegm( nullptr, aDC, GetABPosition( m_Start ), GetABPosition( m_End ),
                         m_Size.x, color );
            }
            else
            {
                GRFilledSegment( nullptr, aDC, GetABPosition( m_Start ), GetABPosition( m_End ),
                                 m_Size.x, color );
            }
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


void GERBER_DRAW_ITEM::ConvertSegmentToPolygon()
{
    m_Polygon.RemoveAllContours();
    m_Polygon.NewOutline();

    wxPoint start = m_Start;
    wxPoint end = m_End;

    // make calculations more easy if ensure start.x < end.x
    // (only 2 quadrants to consider)
    if( start.x > end.x )
        std::swap( start, end );

    // calculate values relative to start point:
    wxPoint delta = end - start;

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
    wxPoint corner;
    corner.x -= m_Size.x/2;
    corner.y -= m_Size.y/2;
    wxPoint close = corner;
    m_Polygon.Append( VECTOR2I( corner ) );  // Lower left corner, start point (1)
    corner.y += m_Size.y;
    m_Polygon.Append( VECTOR2I( corner ) );  // upper left corner, start point (2)

    if( delta.x || delta.y)
    {
        corner += delta;
        m_Polygon.Append( VECTOR2I( corner ) );  // upper left corner, end point (3)
    }

    corner.x += m_Size.x;
    m_Polygon.Append( VECTOR2I( corner ) );  // upper right corner, end point (4)
    corner.y -= m_Size.y;
    m_Polygon.Append( VECTOR2I( corner ) );  // lower right corner, end point (5)

    if( delta.x || delta.y )
    {
        corner -= delta;
        m_Polygon.Append( VECTOR2I( corner ) );  // lower left corner, start point (6)
    }

    m_Polygon.Append( VECTOR2I( close ) );  // close the shape

    // Create final polygon:
    for( auto it = m_Polygon.Iterate( 0 ); it; ++it )
    {
        if( change )
            ( *it ).y = -( *it ).y;

        *it += start;
    }
}


void GERBER_DRAW_ITEM::PrintGerberPoly( wxDC* aDC, COLOR4D aColor, const wxPoint& aOffset,
                                        bool aFilledShape )
{
    std::vector<wxPoint> points;
    SHAPE_LINE_CHAIN& poly = m_Polygon.Outline( 0 );
    int pointCount = poly.PointCount() - 1;

    points.reserve( pointCount );

    for( int ii = 0; ii < pointCount; ii++ )
    {
        wxPoint p( poly.Point( ii ).x, poly.Point( ii ).y );
        points[ii] = p + aOffset;
        points[ii] = GetABPosition( points[ii] );
    }

    GRClosedPoly( nullptr, aDC, pointCount, &points[0], aFilledShape, aColor, aColor );
}


void GERBER_DRAW_ITEM::GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;
    wxString text;

    msg = ShowGBRShape();
    aList.push_back( MSG_PANEL_ITEM( _( "Type" ), msg, DARKCYAN ) );

    // Display D_Code value with its attributes:
    msg.Printf( _( "D Code %d" ), m_DCode );
    D_CODE* apertDescr = GetDcodeDescr();

    if( !apertDescr || apertDescr->m_AperFunction.IsEmpty() )
        text = _( "No attribute" );
    else
        text = apertDescr->m_AperFunction;

    aList.push_back( MSG_PANEL_ITEM( msg, text, RED ) );

    // Display graphic layer name
    msg = GERBER_FILE_IMAGE_LIST::GetImagesList().GetDisplayName( GetLayer(), true );
    aList.push_back( MSG_PANEL_ITEM( _( "Graphic Layer" ), msg, DARKGREEN ) );

    // Display item rotation
    // The full rotation is Image rotation + m_lyrRotation
    // but m_lyrRotation is specific to this object
    // so we display only this parameter
    msg.Printf( wxT( "%f" ), m_lyrRotation );
    aList.push_back( MSG_PANEL_ITEM( _( "Rotation" ), msg, BLUE ) );

    // Display item polarity (item specific)
    msg = m_LayerNegative ? _("Clear") : _("Dark");
    aList.push_back( MSG_PANEL_ITEM( _( "Polarity" ), msg, BLUE ) );

    // Display mirroring (item specific)
    msg.Printf( wxT( "A:%s B:%s" ),
                m_mirrorA ? _("Yes") : _("No"),
                m_mirrorB ? _("Yes") : _("No"));
    aList.push_back( MSG_PANEL_ITEM( _( "Mirror" ), msg, DARKRED ) );

    // Display AB axis swap (item specific)
    msg = m_swapAxis ? wxT( "A=Y B=X" ) : wxT( "A=X B=Y" );
    aList.push_back( MSG_PANEL_ITEM( _( "AB axis" ), msg, DARKRED ) );

    // Display net info, if exists
    if( m_netAttributes.m_NetAttribType == GBR_NETLIST_METADATA::GBR_NETINFO_UNSPECIFIED )
        return;

    // Build full net info:
    wxString net_msg;
    wxString cmp_pad_msg;

    if( ( m_netAttributes.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_NET ) )
    {
        net_msg = _( "Net:" );
        net_msg << " ";

        if( m_netAttributes.m_Netname.IsEmpty() )
            net_msg << "<no net>";
        else
            net_msg << UnescapeString( m_netAttributes.m_Netname );
    }

    if( ( m_netAttributes.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_PAD ) )
    {
        cmp_pad_msg.Printf( _( "Cmp: %s;  Pad: %s" ),
                                m_netAttributes.m_Cmpref,
                                m_netAttributes.m_Padname );
    }

    else if( ( m_netAttributes.m_NetAttribType & GBR_NETLIST_METADATA::GBR_NETINFO_CMP ) )
    {
        cmp_pad_msg = _( "Cmp:" );
        cmp_pad_msg << " " << m_netAttributes.m_Cmpref;
    }

    aList.push_back( MSG_PANEL_ITEM( net_msg, cmp_pad_msg, DARKCYAN ) );
}


BITMAP_DEF GERBER_DRAW_ITEM::GetMenuImage() const
{
    if( m_Flashed )
        return pad_xpm;

    switch( m_Shape )
    {
    case GBR_SEGMENT:
    case GBR_ARC:
    case GBR_CIRCLE:
        return add_line_xpm;

    case GBR_SPOT_OVAL:
    case GBR_SPOT_CIRCLE:
    case GBR_SPOT_RECT:
    case GBR_SPOT_POLY:
    case GBR_SPOT_MACRO:
        // should be handles by m_Flashed == true
        return pad_xpm;

    case GBR_POLYGON:
        return add_graphical_polygon_xpm;
    }

    return info_xpm;
}


bool GERBER_DRAW_ITEM::HitTest( const wxPoint& aRefPos, int aAccuracy ) const
{
    // In case the item has a very tiny width defined, allow it to be selected
    const int MIN_HIT_TEST_RADIUS = Millimeter2iu( 0.01 );

    // calculate aRefPos in XY gerber axis:
    wxPoint ref_pos = GetXYPosition( aRefPos );

    SHAPE_POLY_SET poly;

    switch( m_Shape )
    {
    case GBR_POLYGON:
        poly = m_Polygon;
        return poly.Contains( VECTOR2I( ref_pos ), 0, aAccuracy );

    case GBR_SPOT_POLY:
        poly = GetDcodeDescr()->m_Polygon;
        poly.Move( m_Start );
        return poly.Contains( VECTOR2I( ref_pos ), 0, aAccuracy );

    case GBR_SPOT_RECT:
        return GetBoundingBox().Contains( aRefPos );

    case GBR_ARC:
        {
            double radius = GetLineLength( m_Start, m_ArcCentre );
            VECTOR2D test_radius = VECTOR2D( ref_pos ) - VECTOR2D( m_ArcCentre );

            int size = ( ( m_Size.x < MIN_HIT_TEST_RADIUS ) ? MIN_HIT_TEST_RADIUS
                                                            : m_Size.x );

            // Are we close enough to the radius?
            bool radius_hit = ( std::fabs( test_radius.EuclideanNorm() - radius) < size );

            if( radius_hit )
            {
                // Now check that we are within the arc angle

                VECTOR2D start = VECTOR2D( m_Start ) - VECTOR2D( m_ArcCentre );
                VECTOR2D end = VECTOR2D( m_End ) - VECTOR2D( m_ArcCentre );

                double start_angle = NormalizeAngleRadiansPos( start.Angle() );
                double end_angle = NormalizeAngleRadiansPos( end.Angle() );

                if( m_Start == m_End )
                {
                    start_angle = 0;
                    end_angle = 2 * M_PI;
                }
                else if( end_angle < start_angle )
                {
                    end_angle += 2 * M_PI;
                }

                double test_angle = NormalizeAngleRadiansPos( test_radius.Angle() );

                return ( test_angle > start_angle && test_angle < end_angle );
            }

            return false;
        }

    case GBR_SPOT_MACRO:
        // Aperture macro polygons are already in absolute coordinates
        auto p = GetDcodeDescr()->GetMacro()->GetApertureMacroShape( this, m_Start );
        return p->Contains( VECTOR2I( aRefPos ), -1, aAccuracy );
    }

    // TODO: a better analyze of the shape (perhaps create a D_CODE::HitTest for flashed items)
    int radius = std::min( m_Size.x, m_Size.y ) >> 1;

    if( radius < MIN_HIT_TEST_RADIUS )
        radius = MIN_HIT_TEST_RADIUS;

    if( m_Flashed )
        return HitTestPoints( m_Start, ref_pos, radius );
    else
        return TestSegmentHit( ref_pos, m_Start, m_End, radius );
}


bool GERBER_DRAW_ITEM::HitTest( const EDA_RECT& aRefArea, bool aContained, int aAccuracy ) const
{
    wxPoint pos = GetABPosition( m_Start );

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

    " shape=\"" << m_Shape << '"' <<
    " addr=\"" << std::hex << this << std::dec << '"' <<
    " layer=\"" << GetLayer() << '"' <<
    " size=\"" << m_Size << '"' <<
    " flags=\"" << m_Flags << '"' <<
    " status=\"" << GetStatus() << '"' <<
    "<start" << m_Start << "/>" <<
    "<end" << m_End << "/>";

    os << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif


void GERBER_DRAW_ITEM::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 2;

    aLayers[0] = GERBER_DRAW_LAYER( GetLayer() );
    aLayers[1] = GERBER_DCODE_LAYER( aLayers[0] );
}


const BOX2I GERBER_DRAW_ITEM::ViewBBox() const
{
    EDA_RECT bbox = GetBoundingBox();
    return BOX2I( VECTOR2I( bbox.GetOrigin() ),
                  VECTOR2I( bbox.GetSize() ) );
}


unsigned int GERBER_DRAW_ITEM::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    // DCodes will be shown only if zoom is appropriate:
    // Returns the level of detail of the item.
    // A level of detail (LOD) is the minimal VIEW scale that
    // is sufficient for an item to be shown on a given layer.
    if( IsDCodeLayer( aLayer ) )
    {
        int size = 0;

        switch( m_Shape )
        {
        case GBR_SPOT_MACRO:
            size = GetDcodeDescr()->GetMacro()->GetBoundingBox().GetWidth();
            break;

        case GBR_ARC:
            size = GetLineLength( m_Start, m_ArcCentre );
            break;

        default:
            size = m_Size.x;
        }

        // the level of details is chosen experimentally, to show
        // only a readable text:
        const int level = Millimeter2iu( 4 );
        return ( level / ( size + 1 ) );
    }

    // Other layers are shown without any conditions
    return 0;
}


SEARCH_RESULT GERBER_DRAW_ITEM::Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] )
{
    KICAD_T stype = *scanTypes;

    // If caller wants to inspect my type
    if( stype == Type() )
    {
        if( SEARCH_QUIT == inspector( this, testData ) )
            return SEARCH_QUIT;
    }

    return SEARCH_CONTINUE;
}


wxString GERBER_DRAW_ITEM::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    wxString layerName;

    layerName = GERBER_FILE_IMAGE_LIST::GetImagesList().GetDisplayName( GetLayer(), true );

    return wxString::Format( _( "%s (D%d) on layer %d: %s" ),
                             ShowGBRShape(),
                             m_DCode,
                             GetLayer() + 1,
                             layerName );
}
