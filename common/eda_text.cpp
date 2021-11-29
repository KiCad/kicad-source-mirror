/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
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
 * @file eda_text.cpp
 * @brief Implementation of base KiCad text object.
 */

#include <algorithm>          // for max
#include <stddef.h>           // for NULL
#include <type_traits>        // for swap
#include <vector>             // for vector

#include <eda_item.h>         // for EDA_ITEM
#include <base_units.h>
#include <basic_gal.h>        // for BASIC_GAL, basic_gal
#include <convert_to_biu.h>   // for Mils2iu
#include <convert_basic_shapes_to_polygon.h>
#include <eda_rect.h>         // for EDA_RECT
#include <eda_text.h>         // for EDA_TEXT, TEXT_EFFECTS, GR_TEXT_VJUSTIF...
#include <gal/color4d.h>      // for COLOR4D, COLOR4D::BLACK
#include <gal/stroke_font.h>  // for STROKE_FONT
#include <gr_text.h>          // for GRText
#include <string_utils.h>     // for UnescapeString
#include <math/util.h>        // for KiROUND
#include <math/vector2d.h>    // for VECTOR2D
#include <richio.h>
#include <render_settings.h>
#include <trigo.h>            // for RotatePoint
#include <i18n_utility.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_poly_set.h>


#include <wx/debug.h>           // for wxASSERT
#include <wx/string.h>          // wxString, wxArrayString
#include <wx/gdicmn.h>          // for wxPoint,wxSize

class OUTPUTFORMATTER;
class wxFindReplaceData;


void addTextSegmToPoly( int x0, int y0, int xf, int yf, void* aData )
{
    TSEGM_2_POLY_PRMS* prm = static_cast<TSEGM_2_POLY_PRMS*>( aData );
    TransformOvalToPolygon( *prm->m_cornerBuffer, wxPoint( x0, y0 ), wxPoint( xf, yf ),
                            prm->m_textWidth, prm->m_error, ERROR_INSIDE );
}


EDA_TEXT_HJUSTIFY_T EDA_TEXT::MapHorizJustify( int aHorizJustify )
{
    wxASSERT( aHorizJustify >= GR_TEXT_HJUSTIFY_LEFT && aHorizJustify <= GR_TEXT_HJUSTIFY_RIGHT );

    if( aHorizJustify > GR_TEXT_HJUSTIFY_RIGHT )
        return GR_TEXT_HJUSTIFY_RIGHT;

    if( aHorizJustify < GR_TEXT_HJUSTIFY_LEFT )
        return GR_TEXT_HJUSTIFY_LEFT;

    return static_cast<EDA_TEXT_HJUSTIFY_T>( aHorizJustify );
}


EDA_TEXT_VJUSTIFY_T EDA_TEXT::MapVertJustify( int aVertJustify )
{
    wxASSERT( aVertJustify >= GR_TEXT_VJUSTIFY_TOP && aVertJustify <= GR_TEXT_VJUSTIFY_BOTTOM );

    if( aVertJustify > GR_TEXT_VJUSTIFY_BOTTOM )
        return GR_TEXT_VJUSTIFY_BOTTOM;

    if( aVertJustify < GR_TEXT_VJUSTIFY_TOP )
        return GR_TEXT_VJUSTIFY_TOP;

    return static_cast<EDA_TEXT_VJUSTIFY_T>( aVertJustify );
}


EDA_TEXT::EDA_TEXT( const wxString& text ) :
        m_text( text ),
        m_e( 1 << TE_VISIBLE )
{
    int sz = Mils2iu( DEFAULT_SIZE_TEXT );
    SetTextSize( wxSize( sz, sz ) );
    cacheShownText();
}


EDA_TEXT::EDA_TEXT( const EDA_TEXT& aText ) :
        m_text( aText.m_text ),
        m_e( aText.m_e )
{
    cacheShownText();
}


EDA_TEXT::~EDA_TEXT()
{
}


void EDA_TEXT::SetText( const wxString& aText )
{
    m_text = aText;
    cacheShownText();
}


void EDA_TEXT::CopyText( const EDA_TEXT& aSrc )
{
    m_text = aSrc.m_text;
    m_shown_text = aSrc.m_shown_text;
    m_shown_text_has_text_var_refs = aSrc.m_shown_text_has_text_var_refs;
}


void EDA_TEXT::SetEffects( const EDA_TEXT& aSrc )
{
    m_e = aSrc.m_e;
}


void EDA_TEXT::SwapText( EDA_TEXT& aTradingPartner )
{
    std::swap( m_text, aTradingPartner.m_text );
    std::swap( m_shown_text, aTradingPartner.m_shown_text );
    std::swap( m_shown_text_has_text_var_refs, aTradingPartner.m_shown_text_has_text_var_refs );
}


void EDA_TEXT::SwapEffects( EDA_TEXT& aTradingPartner )
{
    std::swap( m_e, aTradingPartner.m_e );
}


int EDA_TEXT::GetEffectiveTextPenWidth( int aDefaultWidth ) const
{
    int width = GetTextThickness();

    if( width <= 1 )
    {
        width = aDefaultWidth;

        if( IsBold() )
            width = GetPenSizeForBold( GetTextWidth() );
        else if( width <= 1 )
            width = GetPenSizeForNormal( GetTextWidth() );
    }

    // Clip pen size for small texts:
    width = Clamp_Text_PenSize( width, GetTextSize(), ALLOW_BOLD_THICKNESS );

    return width;
}


bool EDA_TEXT::Replace( const wxFindReplaceData& aSearchData )
{
    bool retval = EDA_ITEM::Replace( aSearchData, m_text );
    cacheShownText();

    return retval;
}


void EDA_TEXT::cacheShownText()
{
    if( m_text.IsEmpty() || m_text == wxT( "~" ) )     // ~ is legacy empty-string token
    {
        m_shown_text = wxEmptyString;
        m_shown_text_has_text_var_refs = false;
    }
    else
    {
        m_shown_text = UnescapeString( m_text );
        m_shown_text_has_text_var_refs = m_shown_text.Contains( wxT( "${" ) );
    }
}


int EDA_TEXT::LenSize( const wxString& aLine, int aThickness ) const
{
    basic_gal.SetFontItalic( IsItalic() );
    basic_gal.SetFontBold( IsBold() );
    basic_gal.SetFontUnderlined( false );
    basic_gal.SetLineWidth( (float) aThickness );
    basic_gal.SetGlyphSize( VECTOR2D( GetTextSize() ) );

    VECTOR2D tsize = basic_gal.GetTextLineSize( aLine );

    return KiROUND( tsize.x );
}


wxString EDA_TEXT::ShortenedShownText() const
{
    wxString tmp = GetShownText();

    tmp.Replace( wxT( "\n" ), wxT( " " ) );
    tmp.Replace( wxT( "\r" ), wxT( " " ) );
    tmp.Replace( wxT( "\t" ), wxT( " " ) );

    if( tmp.Length() > 36 )
        tmp = tmp.Left( 34 ) + wxT( "..." );

    return tmp;
}


int EDA_TEXT::GetInterline() const
{
    return KiROUND( KIGFX::STROKE_FONT::GetInterline( GetTextHeight() ) );
}


EDA_RECT EDA_TEXT::GetTextBox( int aLine, bool aInvertY ) const
{
    EDA_RECT       rect;
    wxArrayString  strings;
    wxString       text = GetShownText();
    int            thickness = GetEffectiveTextPenWidth();
    int            linecount = 1;
    bool           hasOverBar = false;     // true if the first line of text as an overbar

    if( IsMultilineAllowed() )
    {
        wxStringSplit( text, strings, '\n' );

        if( strings.GetCount() )     // GetCount() == 0 for void strings
        {
            if( aLine >= 0 && ( aLine < static_cast<int>( strings.GetCount() ) ) )
                text = strings.Item( aLine );
            else
                text = strings.Item( 0 );

            linecount = strings.GetCount();
        }
    }

    // Search for overbar symbol. Only text is scanned,
    // because only this line can change the bounding box
    for( unsigned ii = 1; ii < text.size(); ii++ )
    {
        if( text[ii-1] == '~' && text[ii] == '{' )
        {
            hasOverBar = true;
            break;
        }
    }

    // calculate the H and V size
    const auto& font = basic_gal.GetStrokeFont();
    VECTOR2D    fontSize( GetTextSize() );
    double      penWidth( thickness );
    int         dx = KiROUND( font.ComputeStringBoundaryLimits( text, fontSize, penWidth ).x );
    int         dy = GetInterline();

    // Creates bounding box (rectangle) for horizontal, left and top justified text. The
    // bounding box will be moved later according to the actual text options
    wxSize textsize = wxSize( dx, dy );
    wxPoint pos = GetTextPos();

    if( IsMultilineAllowed() && aLine > 0 && ( aLine < static_cast<int>( strings.GetCount() ) ) )
    {
        pos.y -= aLine * GetInterline();
    }

    if( aInvertY )
        pos.y = -pos.y;

    rect.SetOrigin( pos );

    if( hasOverBar )
    {   // A overbar adds an extra size to the text
        // Height from the base line text of chars like [ or {
        double curr_height = GetTextHeight() * 1.15;
        double overbarPosition = font.ComputeOverbarVerticalPosition( fontSize.y );
        int    extra_height = KiROUND( overbarPosition - curr_height );

        extra_height += thickness / 2;
        textsize.y += extra_height;
        rect.Move( wxPoint( 0, -extra_height ) );
    }

    // for multiline texts and aLine < 0, merge all rectangles
    // ( if aLine < 0, we want the full text bounding box )
    if( IsMultilineAllowed() && aLine < 0 )
    {
        for( unsigned ii = 1; ii < strings.GetCount(); ii++ )
        {
            text = strings.Item( ii );
            dx = KiROUND( font.ComputeStringBoundaryLimits( text, fontSize, penWidth ).x );
            textsize.x = std::max( textsize.x, dx );
            textsize.y += dy;
        }
    }

    rect.SetSize( textsize );

    /* Now, calculate the rect origin, according to text justification
     * At this point the rectangle origin is the text origin (m_Pos).
     * This is true only for left and top text justified texts (using top to bottom Y axis
     * orientation). and must be recalculated for others justifications
     * also, note the V justification is relative to the first line
     */
    switch( GetHorizJustify() )
    {
    case GR_TEXT_HJUSTIFY_LEFT:
        if( IsMirrored() )
            rect.SetX( rect.GetX() - rect.GetWidth() );
        break;

    case GR_TEXT_HJUSTIFY_CENTER:
        rect.SetX( rect.GetX() - (rect.GetWidth() / 2) );
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        if( !IsMirrored() )
            rect.SetX( rect.GetX() - rect.GetWidth() );
        break;
    }

    switch( GetVertJustify() )
    {
    case GR_TEXT_VJUSTIFY_TOP:
        break;

    case GR_TEXT_VJUSTIFY_CENTER:
        rect.SetY( rect.GetY() - ( dy / 2) );
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        rect.SetY( rect.GetY() - dy );
        break;
    }

    if( linecount > 1 )
    {
        int yoffset;
        linecount -= 1;

        switch( GetVertJustify() )
        {
        case GR_TEXT_VJUSTIFY_TOP:
            break;

        case GR_TEXT_VJUSTIFY_CENTER:
            yoffset = linecount * GetInterline() / 2;
            rect.SetY( rect.GetY() - yoffset );
            break;

        case GR_TEXT_VJUSTIFY_BOTTOM:
            yoffset = linecount * GetInterline();
            rect.SetY( rect.GetY() - yoffset );
            break;
        }
    }

    // Many fonts draw diacriticals, descenders, etc. outside the X-height of the font.  This
    // will cacth most (but probably not all) of them.
    rect.Inflate( 0, thickness * 1.5 );

    rect.Normalize();       // Make h and v sizes always >= 0

    return rect;
}


bool EDA_TEXT::TextHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    EDA_RECT rect = GetTextBox();
    wxPoint location = aPoint;

    rect.Inflate( aAccuracy );
    RotatePoint( &location, GetTextPos(), -GetTextAngle() );

    return rect.Contains( location );
}


bool EDA_TEXT::TextHitTest( const EDA_RECT& aRect, bool aContains, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContains )
        return rect.Contains( GetTextBox() );

    return rect.Intersects( GetTextBox(), GetTextAngle() );
}


void EDA_TEXT::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                      const COLOR4D& aColor, OUTLINE_MODE aFillMode )
{
    if( IsMultilineAllowed() )
    {
        std::vector<wxPoint> positions;
        wxArrayString  strings;
        wxStringSplit( GetShownText(), strings, '\n' );

        positions.reserve( strings.Count() );

        GetLinePositions( positions, strings.Count() );

        for( unsigned ii = 0; ii < strings.Count(); ii++ )
            printOneLineOfText( aSettings, aOffset, aColor, aFillMode, strings[ii], positions[ii] );
    }
    else
    {
        printOneLineOfText( aSettings, aOffset, aColor, aFillMode, GetShownText(), GetTextPos() );
    }
}


void EDA_TEXT::GetLinePositions( std::vector<wxPoint>& aPositions, int aLineCount ) const
{
    wxPoint        pos  = GetTextPos();     // Position of first line of the
                                            // multiline text according to
                                            // the center of the multiline text block

    wxPoint        offset;                  // Offset to next line.

    offset.y = GetInterline();

    if( aLineCount > 1 )
    {
        switch( GetVertJustify() )
        {
        case GR_TEXT_VJUSTIFY_TOP:
            break;

        case GR_TEXT_VJUSTIFY_CENTER:
            pos.y -= ( aLineCount - 1 ) * offset.y / 2;
            break;

        case GR_TEXT_VJUSTIFY_BOTTOM:
            pos.y -= ( aLineCount - 1 ) * offset.y;
            break;
        }
    }

    // Rotate the position of the first line
    // around the center of the multiline text block
    RotatePoint( &pos, GetTextPos(), GetTextAngle() );

    // Rotate the offset lines to increase happened in the right direction
    RotatePoint( &offset, GetTextAngle() );

    for( int ii = 0; ii < aLineCount; ii++ )
    {
        aPositions.push_back( pos );
        pos += offset;
    }
}

void EDA_TEXT::printOneLineOfText( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset,
                                   const COLOR4D& aColor, OUTLINE_MODE aFillMode,
                                   const wxString& aText, const wxPoint &aPos )
{
    wxDC* DC = aSettings->GetPrintDC();
    int   penWidth = std::max( GetEffectiveTextPenWidth(), aSettings->GetDefaultPenWidth() );

    if( aFillMode == SKETCH )
        penWidth = -penWidth;

    wxSize size = GetTextSize();

    if( IsMirrored() )
        size.x = -size.x;

    GRText( DC, aOffset + aPos, aColor, aText, GetTextAngle(), size, GetHorizJustify(),
            GetVertJustify(), penWidth, IsItalic(), IsBold() );
}


wxString EDA_TEXT::GetTextStyleName() const
{
    int style = 0;

    if( IsItalic() )
        style = 1;

    if( IsBold() )
        style += 2;

    wxString stylemsg[4] = {
        _("Normal"),
        _("Italic"),
        _("Bold"),
        _("Bold+Italic")
    };

    return stylemsg[style];
}


bool EDA_TEXT::IsDefaultFormatting() const
{
    return ( IsVisible()
             && !IsMirrored()
             && GetHorizJustify() == GR_TEXT_HJUSTIFY_CENTER
             && GetVertJustify() == GR_TEXT_VJUSTIFY_CENTER
             && GetTextThickness() == 0
             && !IsItalic()
             && !IsBold()
             && !IsMultilineAllowed()
           );
}


void EDA_TEXT::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
{
#ifndef GERBVIEW        // Gerbview does not use EDA_TEXT::Format
                        // and does not define FormatInternalUnits, used here
                        // however this function should exist

    aFormatter->Print( aNestLevel + 1, "(effects" );

    // Text size
    aFormatter->Print( 0, " (font" );

    aFormatter->Print( 0, " (size %s %s)",
                       FormatInternalUnits( GetTextHeight() ).c_str(),
                       FormatInternalUnits( GetTextWidth() ).c_str() );

    if( GetTextThickness() )
    {
        aFormatter->Print( 0, " (thickness %s)",
                           FormatInternalUnits( GetTextThickness() ).c_str() );
    }

    if( IsBold() )
        aFormatter->Print( 0, " bold" );

    if( IsItalic() )
        aFormatter->Print( 0, " italic" );

    aFormatter->Print( 0, ")"); // (font

    if( IsMirrored() || GetHorizJustify() != GR_TEXT_HJUSTIFY_CENTER
                     || GetVertJustify() != GR_TEXT_VJUSTIFY_CENTER )
    {
        aFormatter->Print( 0, " (justify");

        if( GetHorizJustify() != GR_TEXT_HJUSTIFY_CENTER )
            aFormatter->Print( 0, GetHorizJustify() == GR_TEXT_HJUSTIFY_LEFT ? " left" : " right" );

        if( GetVertJustify() != GR_TEXT_VJUSTIFY_CENTER )
            aFormatter->Print( 0, GetVertJustify() == GR_TEXT_VJUSTIFY_TOP ? " top" : " bottom" );

        if( IsMirrored() )
            aFormatter->Print( 0, " mirror" );

        aFormatter->Print( 0, ")" ); // (justify
    }

    if( !(aControlBits & CTL_OMIT_HIDE) && !IsVisible() )
        aFormatter->Print( 0, " hide" );

    aFormatter->Print( 0, ")\n" ); // (justify

#endif
}

// Convert the text shape to a list of segment
// each segment is stored as 2 wxPoints: its starting point and its ending point
// we are using GRText to create the segments and therefore a call-back function is needed

// This is a call back function, used by GRText to put each segment in buffer
static void addTextSegmToBuffer( int x0, int y0, int xf, int yf, void* aData )
{
    std::vector<wxPoint>* cornerBuffer = static_cast<std::vector<wxPoint>*>( aData );
    cornerBuffer->push_back( wxPoint( x0, y0 ) );
    cornerBuffer->push_back( wxPoint( xf, yf ) );
}


std::vector<wxPoint> EDA_TEXT::TransformToSegmentList() const
{
    std::vector<wxPoint> cornerBuffer;
    wxSize size = GetTextSize();

    if( IsMirrored() )
        size.x = -size.x;

    bool forceBold = true;
    int  penWidth = 0;      // use max-width for bold text

    COLOR4D color = COLOR4D::BLACK;  // not actually used, but needed by GRText

    if( IsMultilineAllowed() )
    {
        wxArrayString strings_list;
        wxStringSplit( GetShownText(), strings_list, wxChar('\n') );
        std::vector<wxPoint> positions;
        positions.reserve( strings_list.Count() );
        GetLinePositions( positions, strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
        {
            wxString txt = strings_list.Item( ii );
            GRText( nullptr, positions[ii], color, txt, GetDrawRotation(), size,
                    GetDrawHorizJustify(), GetDrawVertJustify(), penWidth, IsItalic(), forceBold,
                    addTextSegmToBuffer, &cornerBuffer );
        }
    }
    else
    {
        GRText( nullptr, GetDrawPos(), color, GetShownText(), GetDrawRotation(), size,
                GetDrawHorizJustify(), GetDrawVertJustify(), penWidth, IsItalic(), forceBold,
                addTextSegmToBuffer, &cornerBuffer );
    }

    return cornerBuffer;
}


std::shared_ptr<SHAPE_COMPOUND> EDA_TEXT::GetEffectiveTextShape( ) const
{
    std::shared_ptr<SHAPE_COMPOUND> shape = std::make_shared<SHAPE_COMPOUND>();
    int penWidth = GetEffectiveTextPenWidth();
    std::vector<wxPoint> pts = TransformToSegmentList();

    for( unsigned jj = 0; jj < pts.size(); jj += 2 )
        shape->AddShape( new SHAPE_SEGMENT( pts[jj], pts[jj+1], penWidth ) );

    return shape;
}


int EDA_TEXT::Compare( const EDA_TEXT* aOther ) const
{
#define EPSILON 2       // Should be enough for rounding errors on calculated items

#define TEST( a, b ) { if( a != b ) return a - b; }
#define TEST_E( a, b ) { if( abs( a - b ) > EPSILON ) return a - b; }
#define TEST_PT( a, b ) { TEST_E( a.x, b.x ); TEST_E( a.y, b.y ); }

    TEST_PT( m_e.pos, aOther->m_e.pos );

    TEST_PT( m_e.size, aOther->m_e.size );
    TEST_E( m_e.penwidth, aOther->m_e.penwidth );
    TEST( m_e.angle, aOther->m_e.angle );

    TEST( m_e.hjustify, aOther->m_e.hjustify );
    TEST( m_e.vjustify, aOther->m_e.vjustify );
    TEST( m_e.bits, aOther->m_e.bits );

    return m_text.Cmp( aOther->m_text );
}


void EDA_TEXT::TransformBoundingBoxWithClearanceToPolygon( SHAPE_POLY_SET* aCornerBuffer,
                                                           int aClearanceValue ) const
{
    if( GetText().Length() == 0 )
        return;

    wxPoint  corners[4];    // Buffer of polygon corners

    EDA_RECT rect = GetTextBox();

    // This ugly hack is because this code used to be defined in the board polygon code
    // file rather than in the EDA_TEXT source file where it belonged.  Using the board
    // default text width was dubious so this recreates the same code with the exception
    // if for some reason a different default text width is require for some other object.
#if !defined( DEFAULT_TEXT_WIDTH )
#define LOCAL_DEFAULT_TEXT_WIDTH
#define DEFAULT_TEXT_WIDTH            0.15
#endif

    rect.Inflate( aClearanceValue + Millimeter2iu( DEFAULT_TEXT_WIDTH ) );

#if defined( LOCAL_DEFAULT_TEXT_WIDTH )
#undef DEFAULT_TEXT_WIDTH
#undef LOCAL_DEFAULT_TEXT_WIDTH
#endif

    corners[0].x = rect.GetOrigin().x;
    corners[0].y = rect.GetOrigin().y;
    corners[1].y = corners[0].y;
    corners[1].x = rect.GetRight();
    corners[2].x = corners[1].x;
    corners[2].y = rect.GetBottom();
    corners[3].y = corners[2].y;
    corners[3].x = corners[0].x;

    aCornerBuffer->NewOutline();

    for( wxPoint& corner : corners )
    {
        // Rotate polygon
        RotatePoint( &corner.x, &corner.y, GetTextPos().x, GetTextPos().y, GetTextAngle() );
        aCornerBuffer->Append( corner.x, corner.y );
    }
}


static struct EDA_TEXT_DESC
{
    EDA_TEXT_DESC()
    {
        ENUM_MAP<EDA_TEXT_HJUSTIFY_T>::Instance()
                .Map( GR_TEXT_HJUSTIFY_LEFT,   _HKI( "Left" ) )
                .Map( GR_TEXT_HJUSTIFY_CENTER, _HKI( "Center" ) )
                .Map( GR_TEXT_HJUSTIFY_RIGHT,  _HKI( "Right" ) );
        ENUM_MAP<EDA_TEXT_VJUSTIFY_T>::Instance()
                .Map( GR_TEXT_VJUSTIFY_TOP,    _HKI( "Top" ) )
                .Map( GR_TEXT_VJUSTIFY_CENTER, _HKI( "Center" ) )
                .Map( GR_TEXT_VJUSTIFY_BOTTOM, _HKI( "Bottom" ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( EDA_TEXT );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, wxString>( _HKI( "Text" ),
                    &EDA_TEXT::SetText, &EDA_TEXT::GetText ) );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, int>( _HKI( "Thickness" ),
                                                          &EDA_TEXT::SetTextThickness,
                                                          &EDA_TEXT::GetTextThickness,
                                                          PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, bool>( _HKI( "Italic" ),
                                                         &EDA_TEXT::SetItalic,
                                                         &EDA_TEXT::IsItalic ) );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, bool>( _HKI( "Bold" ),
                                                         &EDA_TEXT::SetBold, &EDA_TEXT::IsBold ) );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, bool>( _HKI( "Mirrored" ),
                                                         &EDA_TEXT::SetMirrored,
                                                         &EDA_TEXT::IsMirrored ) );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, bool>( _HKI( "Visible" ),
                                                         &EDA_TEXT::SetVisible,
                                                         &EDA_TEXT::IsVisible ) );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, int>( _HKI( "Width" ),
                                                          &EDA_TEXT::SetTextWidth,
                                                          &EDA_TEXT::GetTextWidth,
                                                          PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, int>( _HKI( "Height" ),
                                                          &EDA_TEXT::SetTextHeight,
                                                          &EDA_TEXT::GetTextHeight,
                                                          PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY_ENUM<EDA_TEXT,
                             EDA_TEXT_HJUSTIFY_T>( _HKI( "Horizontal Justification" ),
                                                   &EDA_TEXT::SetHorizJustify,
                                                   &EDA_TEXT::GetHorizJustify ) );
        propMgr.AddProperty( new PROPERTY_ENUM<EDA_TEXT,
                             EDA_TEXT_VJUSTIFY_T>( _HKI( "Vertical Justification" ),
                                                   &EDA_TEXT::SetVertJustify,
                                                   &EDA_TEXT::GetVertJustify ) );
    }
} _EDA_TEXT_DESC;

ENUM_TO_WXANY( EDA_TEXT_HJUSTIFY_T )
ENUM_TO_WXANY( EDA_TEXT_VJUSTIFY_T )
