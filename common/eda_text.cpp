/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2017 KiCad Developers, see change_log.txt for contributors.
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
#include <eda_rect.h>         // for EDA_RECT
#include <eda_text.h>         // for EDA_TEXT, TEXT_EFFECTS, GR_TEXT_VJUSTIF...
#include <gal/color4d.h>      // for COLOR4D, COLOR4D::BLACK
#include <gal/stroke_font.h>  // for STROKE_FONT
#include <gr_text.h>          // for GRText
#include <kicad_string.h>     // for UnescapeString
#include <math/util.h>          // for KiROUND
#include <math/vector2d.h>    // for VECTOR2D
#include <richio.h>
#include <render_settings.h>
#include <trigo.h>            // for RotatePoint
#include <i18n_utility.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_compound.h>


#include <wx/debug.h>           // for wxASSERT
#include <wx/string.h>          // wxString, wxArrayString
#include <wx/gdicmn.h>          // for wxPoint,wxSize

class OUTPUTFORMATTER;
class wxFindReplaceData;


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
    m_shown_text_has_text_var_refs = false;

    if( !text.IsEmpty() )
    {
        m_shown_text = UnescapeString( text );
        m_shown_text_has_text_var_refs = m_shown_text.Contains( wxT( "${" ) );
    }
}


EDA_TEXT::EDA_TEXT( const EDA_TEXT& aText ) :
        m_text( aText.m_text ),
        m_e( aText.m_e )
{
    m_shown_text = UnescapeString( m_text );
    m_shown_text_has_text_var_refs = m_shown_text.Contains( wxT( "${" ) );
}


EDA_TEXT::~EDA_TEXT()
{
}


void EDA_TEXT::SetText( const wxString& aText )
{
    m_text = aText;
    m_shown_text = UnescapeString( aText );
    m_shown_text_has_text_var_refs = m_shown_text.Contains( wxT( "${" ) );
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
    m_shown_text = UnescapeString( m_text );
    m_shown_text_has_text_var_refs = m_shown_text.Contains( wxT( "${" ) );

    return retval;
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
        if( text[ii-1] == '~' && text[ii] != '~' )
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
                      COLOR4D aColor, OUTLINE_MODE aFillMode )
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
                                   COLOR4D aColor, OUTLINE_MODE aFillMode,
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
        aFormatter->Print( 0, " (thickness %s)", FormatInternalUnits( GetTextThickness() ).c_str() );

    if( IsBold() )
        aFormatter->Print( 0, " bold" );

    if( IsItalic() )
        aFormatter->Print( 0, " italic" );

    aFormatter->Print( 0, ")"); // (font

    if( IsMirrored() ||
        GetHorizJustify() != GR_TEXT_HJUSTIFY_CENTER ||
        GetVertJustify() != GR_TEXT_VJUSTIFY_CENTER )
    {
        aFormatter->Print( 0, " (justify");

        if( GetHorizJustify() != GR_TEXT_HJUSTIFY_CENTER )
            aFormatter->Print( 0, (GetHorizJustify() == GR_TEXT_HJUSTIFY_LEFT) ? " left" : " right" );

        if( GetVertJustify() != GR_TEXT_VJUSTIFY_CENTER )
            aFormatter->Print( 0, (GetVertJustify() == GR_TEXT_VJUSTIFY_TOP) ? " top" : " bottom" );

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
            GRText( NULL, positions[ii], color, txt, GetDrawRotation(), size, GetHorizJustify(),
                    GetVertJustify(), penWidth, IsItalic(), forceBold, addTextSegmToBuffer,
                    &cornerBuffer );
        }
    }
    else
    {
        GRText( NULL, GetTextPos(), color, GetShownText(), GetDrawRotation(), size,
                GetHorizJustify(), GetVertJustify(), penWidth, IsItalic(), forceBold,
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


double EDA_TEXT::GetDrawRotation() const
{
    return GetTextAngle();
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
                    &EDA_TEXT::SetTextThickness, &EDA_TEXT::GetTextThickness, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, bool>( _HKI( "Italic" ),
                    &EDA_TEXT::SetItalic, &EDA_TEXT::IsItalic ) );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, bool>( _HKI( "Bold" ),
                    &EDA_TEXT::SetBold, &EDA_TEXT::IsBold ) );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, bool>( _HKI( "Mirrored" ),
                    &EDA_TEXT::SetMirrored, &EDA_TEXT::IsMirrored ) );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, bool>( _HKI( "Visible" ),
                    &EDA_TEXT::SetVisible, &EDA_TEXT::IsVisible ) );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, int>( _HKI( "Width" ),
                    &EDA_TEXT::SetTextWidth, &EDA_TEXT::GetTextWidth, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<EDA_TEXT, int>( _HKI( "Height" ),
                    &EDA_TEXT::SetTextHeight, &EDA_TEXT::GetTextHeight, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY_ENUM<EDA_TEXT, EDA_TEXT_HJUSTIFY_T>( _HKI( "Horizontal Justification" ),
                    &EDA_TEXT::SetHorizJustify, &EDA_TEXT::GetHorizJustify ) );
        propMgr.AddProperty( new PROPERTY_ENUM<EDA_TEXT, EDA_TEXT_VJUSTIFY_T>( _HKI( "Vertical Justification" ),
                    &EDA_TEXT::SetVertJustify, &EDA_TEXT::GetVertJustify ) );
    }
} _EDA_TEXT_DESC;

ENUM_TO_WXANY( EDA_TEXT_HJUSTIFY_T )
ENUM_TO_WXANY( EDA_TEXT_VJUSTIFY_T )
