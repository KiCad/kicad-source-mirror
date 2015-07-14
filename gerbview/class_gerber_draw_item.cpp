/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 <Jean-Pierre Charras>
 * Copyright (C) 1992-2010 KiCad Developers, see change_log.txt for contributors.
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
 * @file class_gerber_draw_item.cpp
 */

#include <fctsys.h>
#include <polygons_defs.h>
#include <gr_basic.h>
#include <common.h>
#include <trigo.h>
#include <class_drawpanel.h>
#include <macros.h>
#include <msgpanel.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_gerber_draw_item.h>
#include <class_GERBER.h>


GERBER_DRAW_ITEM::GERBER_DRAW_ITEM( GBR_LAYOUT* aParent, GERBER_IMAGE* aGerberparams ) :
    EDA_ITEM( (EDA_ITEM*)aParent, TYPE_GERBER_DRAW_ITEM )
{
    m_imageParams = aGerberparams;
    m_Layer         = 0;
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
    if( m_imageParams )
        SetLayerParameters();
}


// Copy constructor
GERBER_DRAW_ITEM::GERBER_DRAW_ITEM( const GERBER_DRAW_ITEM& aSource ) :
    EDA_ITEM( aSource )
{
    m_imageParams = aSource.m_imageParams;
    m_Shape = aSource.m_Shape;

    m_Flags     = aSource.m_Flags;
    SetTimeStamp( aSource.m_TimeStamp );

    SetStatus( aSource.GetStatus() );
    m_Start         = aSource.m_Start;
    m_End           = aSource.m_End;
    m_Size          = aSource.m_Size;
    m_Layer         = aSource.m_Layer;
    m_Shape         = aSource.m_Shape;
    m_Flashed       = aSource.m_Flashed;
    m_DCode         = aSource.m_DCode;
    m_PolyCorners   = aSource.m_PolyCorners;
    m_UnitsMetric   = aSource.m_UnitsMetric;
    m_LayerNegative = aSource.m_LayerNegative;
    m_swapAxis      = aSource.m_swapAxis;
    m_mirrorA       = aSource.m_mirrorA;
    m_mirrorB       = aSource.m_mirrorB;
    m_layerOffset   = aSource.m_layerOffset;
    m_drawScale     = aSource.m_drawScale;
    m_lyrRotation   = aSource.m_lyrRotation;
}


GERBER_DRAW_ITEM::~GERBER_DRAW_ITEM()
{
}


GERBER_DRAW_ITEM* GERBER_DRAW_ITEM::Copy() const
{
    return new GERBER_DRAW_ITEM( *this );
}


wxPoint GERBER_DRAW_ITEM::GetABPosition( const wxPoint& aXYPosition ) const
{
    /* Note: RS274Xrevd_e is obscure about the order of transforms:
     * For instance: Rotation must be made after or before mirroring ?
     * Note: if something is changed here, GetYXPosition must reflect changes
     */
    wxPoint abPos = aXYPosition + m_imageParams->m_ImageJustifyOffset;

    if( m_swapAxis )
        std::swap( abPos.x, abPos.y );

    abPos  += m_layerOffset + m_imageParams->m_ImageOffset;
    abPos.x = KiROUND( abPos.x * m_drawScale.x );
    abPos.y = KiROUND( abPos.y * m_drawScale.y );
    double rotation = m_lyrRotation * 10 + m_imageParams->m_ImageRotation * 10;

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

    double rotation = m_lyrRotation * 10 + m_imageParams->m_ImageRotation * 10;

    if( rotation )
        RotatePoint( &xyPos, rotation );

    xyPos.x = KiROUND( xyPos.x / m_drawScale.x );
    xyPos.y = KiROUND( xyPos.y / m_drawScale.y );
    xyPos  -= m_layerOffset + m_imageParams->m_ImageOffset;

    if( m_swapAxis )
        std::swap( xyPos.x, xyPos.y );

    return xyPos - m_imageParams->m_ImageJustifyOffset;
}


void GERBER_DRAW_ITEM::SetLayerParameters()
{
    m_UnitsMetric = m_imageParams->m_GerbMetric;
    m_swapAxis    = m_imageParams->m_SwapAxis;     // false if A = X, B = Y;

    // true if A =Y, B = Y
    m_mirrorA     = m_imageParams->m_MirrorA;      // true: mirror / axe A
    m_mirrorB     = m_imageParams->m_MirrorB;      // true: mirror / axe B
    m_drawScale   = m_imageParams->m_Scale;        // A and B scaling factor
    m_layerOffset = m_imageParams->m_Offset;       // Offset from OF command

    // Rotation from RO command:
    m_lyrRotation = m_imageParams->m_LocalRotation;
    m_LayerNegative = m_imageParams->GetLayerParams().m_LayerNegative;
}


wxString GERBER_DRAW_ITEM::ShowGBRShape()
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


D_CODE* GERBER_DRAW_ITEM::GetDcodeDescr()
{
    if( (m_DCode < FIRST_DCODE) || (m_DCode > LAST_DCODE) )
        return NULL;

    GERBER_IMAGE* gerber = g_GERBER_List.GetGbrImage( m_Layer );

    if( gerber == NULL )
        return NULL;

    D_CODE* d_code = gerber->GetDCODE( m_DCode, false );

    return d_code;
}


const EDA_RECT GERBER_DRAW_ITEM::GetBoundingBox() const
{
    // return a rectangle which is (pos,dim) in nature.  therefore the +1
    EDA_RECT bbox( m_Start, wxSize( 1, 1 ) );

    bbox.Inflate( m_Size.x / 2, m_Size.y / 2 );

    bbox.SetOrigin( GetABPosition( bbox.GetOrigin() ) );
    bbox.SetEnd( GetABPosition( bbox.GetEnd() ) );
    return bbox;
}


void GERBER_DRAW_ITEM::MoveAB( const wxPoint& aMoveVector )
{
    wxPoint xymove = GetXYPosition( aMoveVector );

    m_Start     += xymove;
    m_End       += xymove;
    m_ArcCentre += xymove;

    for( unsigned ii = 0; ii < m_PolyCorners.size(); ii++ )
        m_PolyCorners[ii] += xymove;
}


void GERBER_DRAW_ITEM::MoveXY( const wxPoint& aMoveVector )
{
    m_Start     += aMoveVector;
    m_End       += aMoveVector;
    m_ArcCentre += aMoveVector;

    for( unsigned ii = 0; ii < m_PolyCorners.size(); ii++ )
        m_PolyCorners[ii] += aMoveVector;
}


bool GERBER_DRAW_ITEM::Save( FILE* aFile ) const
{
    return true;
}

bool GERBER_DRAW_ITEM::HasNegativeItems()
{
    bool isClear = m_LayerNegative ^ m_imageParams->m_ImageNegative;

    // if isClear is true, this item has negative shape
    // but if isClear is true, and if this item use an aperture macro definition,
    // we must see if this aperture macro uses a negative shape.
    if( isClear )
        return true;

    // see for a macro def
    D_CODE* dcodeDescr = GetDcodeDescr();

    if( dcodeDescr == NULL )
        return false;

    if( m_Shape ==  GBR_SPOT_MACRO )
    {
        APERTURE_MACRO* macro = dcodeDescr->GetMacro();

        if( macro )     // macro == NULL should not occurs
            return macro->HasNegativeItems( this );
    }

    return false;
}


void GERBER_DRAW_ITEM::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode,
                             const wxPoint& aOffset )
{
    // used when a D_CODE is not found. default D_CODE to draw a flashed item
    static D_CODE dummyD_CODE( 0 );
    EDA_COLOR_T   color, alt_color;
    bool          isFilled;
    int           radius;
    int           halfPenWidth;
    static bool   show_err;
    D_CODE*       d_codeDescr = GetDcodeDescr();
    GERBVIEW_FRAME* gerbFrame = (GERBVIEW_FRAME*) aPanel->GetParent();

    if( d_codeDescr == NULL )
        d_codeDescr = &dummyD_CODE;

    if( gerbFrame->IsLayerVisible( GetLayer() ) == false )
        return;

    color = gerbFrame->GetLayerColor( GetLayer() );

    if( aDrawMode & GR_HIGHLIGHT )
        ColorChangeHighlightFlag( &color, !(aDrawMode & GR_AND) );

    ColorApplyHighlightFlag( &color );

    alt_color = gerbFrame->GetNegativeItemsColor();

    /* isDark is true if flash is positive and should use a drawing
     *   color other than the background color, else use the background color
     *   when drawing so that an erasure happens.
     */
    bool isDark = !(m_LayerNegative ^ m_imageParams->m_ImageNegative);

    if( !isDark )
    {
        // draw in background color ("negative" color)
        std::swap( color, alt_color );
    }

    GRSetDrawMode( aDC, aDrawMode );

    isFilled = gerbFrame->DisplayLinesSolidMode();

    switch( m_Shape )
    {
    case GBR_POLYGON:
        isFilled = gerbFrame->DisplayPolygonsSolidMode();

        if( !isDark )
            isFilled = true;

        DrawGbrPoly( aPanel->GetClipBox(), aDC, color, aOffset, isFilled );
        break;

    case GBR_CIRCLE:
        radius = KiROUND( GetLineLength( m_Start, m_End ) );

        halfPenWidth = m_Size.x >> 1;

        if( !isFilled )
        {
            // draw the border of the pen's path using two circles, each as narrow as possible
            GRCircle( aPanel->GetClipBox(), aDC, GetABPosition( m_Start ),
                      radius - halfPenWidth, 0, color );
            GRCircle( aPanel->GetClipBox(), aDC, GetABPosition( m_Start ),
                      radius + halfPenWidth, 0, color );
        }
        else    // Filled mode
        {
            GRCircle( aPanel->GetClipBox(), aDC, GetABPosition( m_Start ),
                      radius, m_Size.x, color );
        }
        break;

    case GBR_ARC:
        // Currently, arcs plotted with a rectangular aperture are not supported.
        // a round pen only is expected.

#if 0   // for arc debug only
        GRLine( aPanel->GetClipBox(), aDC, GetABPosition( m_Start ),
                GetABPosition( m_ArcCentre ), 0, color );
        GRLine( aPanel->GetClipBox(), aDC, GetABPosition( m_End ),
                GetABPosition( m_ArcCentre ), 0, color );
#endif

        if( !isFilled )
        {
            GRArc1( aPanel->GetClipBox(), aDC, GetABPosition( m_Start ),
                    GetABPosition( m_End ), GetABPosition( m_ArcCentre ),
                    0, color );
        }
        else
        {
            GRArc1( aPanel->GetClipBox(), aDC, GetABPosition( m_Start ),
                    GetABPosition( m_End ), GetABPosition( m_ArcCentre ),
                    m_Size.x, color );
        }

        break;

    case GBR_SPOT_CIRCLE:
    case GBR_SPOT_RECT:
    case GBR_SPOT_OVAL:
    case GBR_SPOT_POLY:
    case GBR_SPOT_MACRO:
        isFilled = gerbFrame->DisplayFlashedItemsSolidMode();
        d_codeDescr->DrawFlashedShape( this, aPanel->GetClipBox(), aDC, color, alt_color,
                                       m_Start, isFilled );
        break;

    case GBR_SEGMENT:
        /* Plot a line from m_Start to m_End.
         * Usually, a round pen is used, but some gerber files use a rectangular pen
         * In fact, any aperture can be used to plot a line.
         * currently: only a square pen is handled (I believe using a polygon gives a strange plot).
         */
        if( d_codeDescr->m_Shape == APT_RECT )
        {
            if( m_PolyCorners.size() == 0 )
                ConvertSegmentToPolygon( );

            DrawGbrPoly( aPanel->GetClipBox(), aDC, color, aOffset, isFilled );
        }
        else
        {
            if( !isFilled )
            {
                    GRCSegm( aPanel->GetClipBox(), aDC, GetABPosition( m_Start ),
                             GetABPosition( m_End ), m_Size.x, color );
            }
            else
            {
                GRFilledSegment( aPanel->GetClipBox(), aDC, GetABPosition( m_Start ),
                                 GetABPosition( m_End ), m_Size.x, color );
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


void GERBER_DRAW_ITEM::ConvertSegmentToPolygon( )
{
    m_PolyCorners.clear();
    m_PolyCorners.reserve(6);

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
    //          3 4
    // 2          5
    // 1 6
    wxPoint corner;
    corner.x -= m_Size.x/2;
    corner.y -= m_Size.y/2;
    m_PolyCorners.push_back( corner );  // Lower left corner, start point (1)
    corner.y += m_Size.y;
    m_PolyCorners.push_back( corner );  // upper left corner, start point (2)

    if( delta.x || delta.y)
    {
        corner += delta;
        m_PolyCorners.push_back( corner );  // upper left corner, end point (3)
    }

    corner.x += m_Size.x;
    m_PolyCorners.push_back( corner );  // upper right corner, end point (4)
    corner.y -= m_Size.y;
    m_PolyCorners.push_back( corner );  // lower right corner, end point (5)

    if( delta.x || delta.y )
    {
        corner -= delta;
        m_PolyCorners.push_back( corner );  // lower left corner, start point (6)
    }

    // Create final polygon:
    for( unsigned ii = 0; ii < m_PolyCorners.size(); ii++ )
    {
        if( change )
            m_PolyCorners[ii].y = -m_PolyCorners[ii].y;

         m_PolyCorners[ii] += start;
    }
}


void GERBER_DRAW_ITEM::DrawGbrPoly( EDA_RECT*      aClipBox,
                                    wxDC*          aDC,
                                    EDA_COLOR_T    aColor,
                                    const wxPoint& aOffset,
                                    bool           aFilledShape )
{
    std::vector<wxPoint> points;

    points = m_PolyCorners;
    for( unsigned ii = 0; ii < points.size(); ii++ )
    {
        points[ii] += aOffset;
        points[ii]  = GetABPosition( points[ii] );
    }

    GRClosedPoly( aClipBox, aDC, points.size(), &points[0], aFilledShape, aColor, aColor );
}


void GERBER_DRAW_ITEM::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString msg;

    msg = ShowGBRShape();
    aList.push_back( MSG_PANEL_ITEM( _( "Type" ), msg, DARKCYAN ) );

    // Display D_Code value:
    msg.Printf( wxT( "%d" ), m_DCode );
    aList.push_back( MSG_PANEL_ITEM( _( "D Code" ), msg, RED ) );

    // Display graphic layer number
    msg.Printf( wxT( "%d" ), GetLayer() + 1 );
    aList.push_back( MSG_PANEL_ITEM( _( "Graphic Layer" ), msg, BROWN ) );

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
}


bool GERBER_DRAW_ITEM::HitTest( const wxPoint& aRefPos ) const
{
    // calculate aRefPos in XY gerber axis:
    wxPoint ref_pos = GetXYPosition( aRefPos );

    // TODO: a better analyze of the shape (perhaps create a D_CODE::HitTest for flashed items)
    int     radius = std::min( m_Size.x, m_Size.y ) >> 1;

    if( m_Flashed )
        return HitTestPoints( m_Start, ref_pos, radius );
    else
        return TestSegmentHit( ref_pos, m_Start, m_End, radius );
}


bool GERBER_DRAW_ITEM::HitTest( const EDA_RECT& aRefArea ) const
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
    " layer=\"" << m_Layer << '"' <<
    " size=\"" << m_Size << '"' <<
    " flags=\"" << m_Flags << '"' <<
    " status=\"" << GetStatus() << '"' <<
    "<start" << m_Start << "/>" <<
    "<end" << m_End << "/>";

    os << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif
