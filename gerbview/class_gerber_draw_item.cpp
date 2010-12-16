/*************************************
*  file class_gerber_draw_item.cpp
*************************************/

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 <Jean-Pierre Charras>
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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

#include "fctsys.h"
#include "polygons_defs.h"
#include "gr_basic.h"
#include "common.h"
#include "trigo.h"
#include "class_drawpanel.h"
#include "drawtxt.h"

#include "gerbview.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"
#include "class_gerber_draw_item.h"
#include "class_GERBER.h"


/**********************************************************/
GERBER_DRAW_ITEM::GERBER_DRAW_ITEM( BOARD_ITEM* aParent, GERBER_IMAGE* aGerberparams ) :
    BOARD_ITEM( aParent, TYPE_GERBER_DRAW_ITEM )
/**********************************************************/
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
    BOARD_ITEM( aSource )
{
    m_imageParams = aSource.m_imageParams;
    m_Shape = aSource.m_Shape;

    m_Flags     = aSource.m_Flags;
    m_TimeStamp = aSource.m_TimeStamp;

    SetStatus( aSource.ReturnStatus() );
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


/**
 * Function GetABPosition
 * returns the image position of aPosition for this object.
 * Image position is the value of aPosition, modified by image parameters:
 * offsets, axis selection, scale, rotation
 * @param aXYPosition = position in Y,X gerber axis
 * @return const wxPoint& - The position in A,B axis.
 * Because draw axis is top to bottom, the final y coordinates is negated
 */
wxPoint GERBER_DRAW_ITEM::GetABPosition( const wxPoint& aXYPosition ) const
{
    /* Note: RS274Xrevd_e is obscure about the order of transforms:
     * For instance: Rotation must be made after or before mirroring ?
     * Note: if something is changed here, GetYXPosition must reflect changes
     */
    wxPoint abPos = aXYPosition + m_imageParams->m_ImageJustifyOffset;

    if( m_swapAxis )
        EXCHG( abPos.x, abPos.y );
    abPos  += m_layerOffset + m_imageParams->m_ImageOffset;
    abPos.x = wxRound( abPos.x * m_drawScale.x );
    abPos.y = wxRound( abPos.y * m_drawScale.y );
    int rotation = wxRound(m_lyrRotation*10) + (m_imageParams->m_ImageRotation*10);
    if( rotation )
        RotatePoint( &abPos, -rotation );

    // Negate A axis if mirrored
    if( m_mirrorA )
        NEGATE( abPos.x );

    // abPos.y must be negated when no mirror, because draw axis is top to bottom
    if( !m_mirrorB )
        NEGATE( abPos.y );
    return abPos;
}


/**
 * Function GetXYPosition
 * returns the image position of aPosition for this object.
 * Image position is the value of aPosition, modified by image parameters:
 * offsets, axis selection, scale, rotation
 * @param aABPosition = position in A,B plotter axis
 * @return const wxPoint - The given position in X,Y axis.
 */
wxPoint GERBER_DRAW_ITEM::GetXYPosition( const wxPoint& aABPosition )
{
    // do the inverse tranform made by GetABPosition
    wxPoint xyPos = aABPosition;

    if( m_mirrorA )
        NEGATE( xyPos.x );
    if( !m_mirrorB )
        NEGATE( xyPos.y );
    int rotation = wxRound(m_lyrRotation*10) + (m_imageParams->m_ImageRotation*10);
    if( rotation )
        RotatePoint( &xyPos, rotation );
    xyPos.x = wxRound( xyPos.x / m_drawScale.x );
    xyPos.y = wxRound( xyPos.y / m_drawScale.y );
    xyPos  -= m_layerOffset + m_imageParams->m_ImageOffset;
    if( m_swapAxis )
        EXCHG( xyPos.x, xyPos.y );
    return xyPos - m_imageParams->m_ImageJustifyOffset;
}


/**
 * Function SetLayerParameters
 * Initialize draw parameters from Image and Layer parameters
 * found in the gerber file:
 *   m_UnitsMetric,
 *   m_MirrorA, m_MirrorB,
 *   m_DrawScale, m_DrawOffset
 */
void GERBER_DRAW_ITEM::SetLayerParameters()
{
    m_UnitsMetric = m_imageParams->m_GerbMetric;
    m_swapAxis    = m_imageParams->m_SwapAxis;     // false if A = X, B = Y;
    // true if A =Y, B = Y
    m_mirrorA     = m_imageParams->m_MirrorA;      // true: mirror / axe A
    m_mirrorB     = m_imageParams->m_MirrorB;      // true: mirror / axe B
    m_drawScale   = m_imageParams->m_Scale;         // A and B scaling factor
    m_layerOffset = m_imageParams->m_Offset;        // Offset from OF command
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


/**
 * Function GetDcodeDescr
 * returns the GetDcodeDescr of this object, or NULL.
 * @return D_CODE* - a pointer to the DCode description (for flashed items).
 */
D_CODE* GERBER_DRAW_ITEM::GetDcodeDescr()
{
    if( (m_DCode < FIRST_DCODE) || (m_DCode > LAST_DCODE) )
        return NULL;
    GERBER_IMAGE* gerber = g_GERBER_List[m_Layer];
    if( gerber == NULL )
        return NULL;

    D_CODE* d_code = gerber->GetDCODE( m_DCode, false );

    return d_code;
}


EDA_Rect GERBER_DRAW_ITEM::GetBoundingBox() const
{
    // return a rectangle which is (pos,dim) in nature.  therefore the +1
    EDA_Rect bbox( m_Start, wxSize( 1, 1 ) );

    bbox.Inflate( m_Size.x / 2, m_Size.y / 2 );

    bbox.SetOrigin( GetABPosition( bbox.GetOrigin() ) );
    bbox.SetEnd( GetABPosition( bbox.GetEnd() ) );
    return bbox;
}


/**
 * Function MoveAB
 * move this object.
 * @param const wxPoint& aMoveVector - the move vector for this object, in AB plotter axis.
 */
void GERBER_DRAW_ITEM::MoveAB( const wxPoint& aMoveVector )
{
    wxPoint xymove = GetXYPosition( aMoveVector );

    m_Start     += xymove;
    m_End       += xymove;
    m_ArcCentre += xymove;
    for( unsigned ii = 0; ii < m_PolyCorners.size(); ii++ )
        m_PolyCorners[ii] += xymove;
}


/**
 * Function MoveXY
 * move this object.
 * @param const wxPoint& aMoveVector - the move vector for this object, in XY gerber axis.
 */
void GERBER_DRAW_ITEM::MoveXY( const wxPoint& aMoveVector )
{
    m_Start     += aMoveVector;
    m_End       += aMoveVector;
    m_ArcCentre += aMoveVector;
    for( unsigned ii = 0; ii < m_PolyCorners.size(); ii++ )
        m_PolyCorners[ii] += aMoveVector;
}


/**
 * Function Save.
 * currently: no nothing, but must be defined to meet requirements
 * of the basic class
 */
bool GERBER_DRAW_ITEM::Save( FILE* aFile ) const
{
    return true;
}

/* Function HasNegativeItems
 * return true if this item or at least one shape (when using aperture macros)
 *    must be drawn in background color
 * useful to optimize screen refresh
 */
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


/*********************************************************************/
void GERBER_DRAW_ITEM::Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, int aDrawMode,
                             const wxPoint& aOffset )
/*********************************************************************/
{
    static D_CODE dummyD_CODE( 0 );      // used when a D_CODE is not found. default D_CODE to draw a flashed item
    int           color, alt_color;
    bool          isFilled;
    int           radius;
    int           halfPenWidth;
    static bool   show_err;
    BOARD*        brd = GetBoard();
    D_CODE*       d_codeDescr = GetDcodeDescr();

    if( d_codeDescr == NULL )
        d_codeDescr = &dummyD_CODE;

    if( brd->IsLayerVisible( GetLayer() ) == false )
        return;

    color = brd->GetLayerColor( GetLayer() );

    if( aDrawMode & GR_SURBRILL )
    {
        if( aDrawMode & GR_AND )
            color &= ~HIGHLIGHT_FLAG;
        else
            color |= HIGHLIGHT_FLAG;
    }
    if( color & HIGHLIGHT_FLAG )
        color = ColorRefs[color & MASKCOLOR].m_LightColor;

    alt_color = g_DrawBgColor;

    /* isDark is true if flash is positive and should use a drawing
     *   color other than the background color, else use the background color
     *   when drawing so that an erasure happens.
     */
    bool isDark = !(m_LayerNegative ^ m_imageParams->m_ImageNegative);
    if( !isDark )
    {
        // draw in background color ("negative" color)
        EXCHG( color, alt_color );
    }

    GRSetDrawMode( aDC, aDrawMode );

    isFilled = DisplayOpt.DisplayPcbTrackFill ? true : false;

    switch( m_Shape )
    {
    case GBR_POLYGON:
        isFilled = (g_DisplayPolygonsModeSketch == false);
        if( !isDark )
            isFilled = true;
        DrawGbrPoly( &aPanel->m_ClipBox, aDC, color, aOffset, isFilled );
        break;

    case GBR_CIRCLE:
        radius = wxRound(hypot( (double) ( m_End.x - m_Start.x ),
                             (double) ( m_End.y - m_Start.y ) ));

        halfPenWidth = m_Size.x >> 1;

        if( !isFilled )
        {
            // draw the border of the pen's path using two circles, each as narrow as possible
            GRCircle( &aPanel->m_ClipBox, aDC, GetABPosition( m_Start ),
                      radius - halfPenWidth, 0, color );
            GRCircle( &aPanel->m_ClipBox, aDC, GetABPosition( m_Start ),
                      radius + halfPenWidth, 0, color );
        }
        else    // Filled mode
        {
            GRCircle( &aPanel->m_ClipBox, aDC, GetABPosition( m_Start ),
                      radius, m_Size.x, color );
        }
        break;

    case GBR_ARC:
        // Currently, arcs plotted witha rectangular aperture are not supported.
        // a round pen only is expected.
#if 0     // for arc debug only
        GRLine( &aPanel->m_ClipBox, aDC, GetABPosition( m_Start ),
                GetABPosition( m_ArcCentre ), 0, color );
        GRLine( &aPanel->m_ClipBox, aDC, GetABPosition( m_End ),
                GetABPosition( m_ArcCentre ), 0, color );
#endif
        if( !isFilled )
        {
            GRArc1( &aPanel->m_ClipBox, aDC, GetABPosition( m_Start ),
                    GetABPosition( m_End ), GetABPosition( m_ArcCentre ),
                    0, color );
        }
        else
        {
            GRArc1( &aPanel->m_ClipBox, aDC, GetABPosition( m_Start ),
                    GetABPosition( m_End ), GetABPosition( m_ArcCentre ),
                    m_Size.x, color );
        }
        break;

    case GBR_SPOT_CIRCLE:
    case GBR_SPOT_RECT:
    case GBR_SPOT_OVAL:
    case GBR_SPOT_POLY:
    case GBR_SPOT_MACRO:
        isFilled = DisplayOpt.DisplayPadFill ? true : false;
        d_codeDescr->DrawFlashedShape( this, &aPanel->m_ClipBox, aDC, color, alt_color,
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
            DrawGbrPoly( &aPanel->m_ClipBox, aDC, color, aOffset, isFilled );
        }
        else
        {
            if( !isFilled )
            {
                    GRCSegm( &aPanel->m_ClipBox, aDC, GetABPosition( m_Start ),
                             GetABPosition( m_End ), m_Size.x, color );
            }
            else
            {
                GRFilledSegment( &aPanel->m_ClipBox, aDC, GetABPosition( m_Start ),
                                 GetABPosition( m_End ), m_Size.x, color );
            }
        }
        break;

    default:
        if( !show_err )
        {
            wxMessageBox( wxT( "Trace_Segment() type error" ) );
            show_err = TRUE;
        }
        break;
    }
}

/**
 * Function ConvertSegmentToPolygon
 * convert a line to an equivalent polygon.
 * Useful when a line is plotted using a rectangular pen.
 * In this case, the usual segment plot cannot be used
 * The equivalent polygon is the area paint by the rectancular pen
 * from m_Start to m_End.
 */
void GERBER_DRAW_ITEM::ConvertSegmentToPolygon( )
{
    m_PolyCorners.clear();
    m_PolyCorners.reserve(6);

    wxPoint start = m_Start;
    wxPoint end = m_End;
    // make calculations more easy if ensure start.x < end.x
    // (only 2 quadrants to consider)
    if( start.x > end.x )
        EXCHG( start, end );

    // calculate values relative to start point:
    wxPoint delta = end - start;
    // calculate corners for the first quadrant only (delta.x and delta.y > 0 )
    // currently, delta.x already is > 0.
    // make delta.y > 0
    bool change = delta.y < 0;
    if( change )
        NEGATE( delta.y);
    // Now create the full polygon.
    // Due to previous chnages, the shape is always something like
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
            NEGATE( m_PolyCorners[ii].y);
         m_PolyCorners[ii] += start;
    }
}


/**
 * Function DrawGbrPoly
 * a helper function used id ::Draw to draw the polygon stored in m_PolyCorners
 * Draw filled polygons
 */
void GERBER_DRAW_ITEM::DrawGbrPoly( EDA_Rect*      aClipBox,
                                    wxDC*          aDC,
                                    int            aColor,
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


/**
 * Function DisplayInfo
 * has knowledge about the frame and how and where to put status information
 * about this object into the frame's message panel.
 * Display info about this GERBER item
 * @param frame A WinEDA_DrawFrame in which to print status information.
 */
void GERBER_DRAW_ITEM::DisplayInfo( WinEDA_DrawFrame* frame )
{
    wxString msg;

    frame->ClearMsgPanel();
    msg = ShowGBRShape();
    frame->AppendMsgPanel( _( "Type" ), msg, DARKCYAN );

    // Display D_Code value:
    msg.Printf( wxT( "%d" ), m_DCode );
    frame->AppendMsgPanel( _( "D Code" ), msg, RED );

    // Display graphic layer number
    msg.Printf( wxT( "%d" ), GetLayer() + 1 );
    frame->AppendMsgPanel( _( "Graphic layer" ), msg, BROWN );

    // Display item rotation
    // The full rotation is Image rotation + m_lyrRotation
    // but m_lyrRotation is specific to this object
    // so we display only this parameter
    msg.Printf( wxT( "%f" ), m_lyrRotation );
    frame->AppendMsgPanel( _( "Rotation" ), msg, BLUE );

    // Display item polarity (item specific)
    msg = m_LayerNegative ? _("Clear") : _("Dark");
    frame->AppendMsgPanel( _( "Polarity" ), msg, BLUE );

    // Display mirroring (item specific)
    msg.Printf( wxT( "A:%s B:%s" ),
                m_mirrorA ? _("Yes") : _("No"),
                m_mirrorB ? _("Yes") : _("No"));
    frame->AppendMsgPanel( _( "Mirror" ), msg, DARKRED );

    // Display AB axis swap (item specific)
    msg = m_swapAxis ? wxT( "A=Y B=X" ) : wxT( "A=X B=Y" );
    frame->AppendMsgPanel( _( "AB axis" ), msg, DARKRED );
}


/**
 * Function HitTest
 * tests if the given wxPoint is within the bounds of this object.
 * @param aRefPos A wxPoint to test in AB axis
 * @return bool - true if a hit, else false
 */
bool GERBER_DRAW_ITEM::HitTest( const wxPoint& aRefPos )
{
    // calculate aRefPos in XY gerber axis:
    wxPoint ref_pos = GetXYPosition( aRefPos );

    // TODO: a better analyse of the shape (perhaps create a D_CODE::HitTest for flashed items)
    int     radius = MIN( m_Size.x, m_Size.y ) >> 1;

    // delta is a vector from m_Start to m_End (an origin of m_Start)
    wxPoint delta = m_End - m_Start;

    // dist is a vector from m_Start to ref_pos (an origin of m_Start)
    wxPoint dist = ref_pos - m_Start;

    if( m_Flashed )
    {
        return (double) dist.x * dist.x + (double) dist.y * dist.y <=
               (double) radius * radius;
    }
    else
    {
        if( DistanceTest( radius, delta.x, delta.y, dist.x, dist.y ) )
            return true;
    }

    return false;
}


/**
 * Function HitTest (overlayed)
 * tests if the given EDA_Rect intersect this object.
 * For now, an ending point must be inside this rect.
 * @param refArea : the given EDA_Rect in AB plotter axis
 * @return bool - true if a hit, else false
 */
bool GERBER_DRAW_ITEM::HitTest( EDA_Rect& refArea )
{
    wxPoint pos = GetABPosition( m_Start );

    if( refArea.Inside( pos ) )
        return true;
    pos = GetABPosition( m_End );
    if( refArea.Inside( pos ) )
        return true;
    return false;
}


#if defined(DEBUG)

/**
 * Function Show
 * is used to output the object tree, currently for debugging only.
 * @param nestLevel An aid to prettier tree indenting, and is the level
 *          of nesting of this object within the overall tree.
 * @param os The ostream& to output to.
 */
void GERBER_DRAW_ITEM::Show( int nestLevel, std::ostream& os )
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() <<

    " shape=\"" << m_Shape << '"' <<
    " addr=\"" << std::hex << this << std::dec << '"' <<
    " layer=\"" << m_Layer << '"' <<
    " size=\"" << m_Size << '"' <<
    " flags=\"" << m_Flags << '"' <<
    " status=\"" << GetState( -1 ) << '"' <<
    "<start" << m_Start << "/>" <<
    "<end" << m_End << "/>";

    os << "</" << GetClass().Lower().mb_str() << ">\n";
}


#endif
