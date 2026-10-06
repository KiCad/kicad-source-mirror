/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <io/easypc/easypc_text_metrics.h>

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <memory>
#include <vector>

#include <io/easypc/easypc_classes_root.h>
#include <io/easypc/easypc_units.h>
#include <font/stroke_font.h>
#include <ki_exception.h>
#include <wx/strconv.h>
#include <wx/translation.h>


namespace EASYPC
{

namespace
{

    // Easy-PC capitals fill this fraction of the text height, the scale from KiCad glyph units
    constexpr double CAPITAL_HEIGHT = 0.75;


    /// Stroke advance as a percentage of the height
    int32_t strokeCharWidthPercent( char aChar, bool aProportional )
    {
        if( !aProportional )
            return 100;

        static std::unique_ptr<KIFONT::STROKE_FONT> font( KIFONT::STROKE_FONT::LoadFont( wxEmptyString ) );
        static const wxCSConv                       cp1252( wxFONTENCODING_CP1252 );

        const char bytes[2] = { aChar, '\0' };
        wxString   text( bytes, cp1252 );
        int        index = text.IsEmpty() ? -1 : static_cast<int>( text[0].GetValue() ) - ' ';

        if( index < 0 || index >= static_cast<int>( font->GetGlyphCount() ) )
            index = '?' - ' ';

        return KiROUND( font->GetGlyphBoundingBox( index ).GetEnd().x * CAPITAL_HEIGHT * 100.0 );
    }


    /// Narrow a metric worked in 64 bits, so a crafted style cannot overflow 32-bit arithmetic
    int32_t narrow( int64_t aValue )
    {
        if( aValue < std::numeric_limits<int32_t>::min() || aValue > std::numeric_limits<int32_t>::max() )
            THROW_IO_ERROR( _( "Easy-PC text metrics are outside the supported range." ) );

        return static_cast<int32_t>( aValue );
    }


    /// PointRotate truncates into 32 bits, so the rotated point must stay in range
    void rotate( int32_t& aX, int32_t& aY, int32_t aAngle, const VECTOR2I& aCentre )
    {
        int64_t reach = std::abs( int64_t( aX ) - aCentre.x ) + std::abs( int64_t( aY ) - aCentre.y );

        narrow( std::abs( int64_t( aCentre.x ) ) + reach );
        narrow( std::abs( int64_t( aCentre.y ) ) + reach );
        PointRotate( aX, aY, aAngle, aCentre.x, aCentre.y );
    }


    /// Angles at which upright text is turned, (135, 305] degrees
    bool flipsUpright( int32_t aAngle )
    {
        return aAngle > 135000 && aAngle <= 305000;
    }


    /// A box, empty until the first Include
    struct EXTENT_BOX
    {
        bool    Empty = true;
        int32_t Left = 0;
        int32_t Bottom = 0;
        int32_t Right = 0;
        int32_t Top = 0;

        void Include( int32_t aX, int32_t aY )
        {
            Left = Empty ? aX : std::min( Left, aX );
            Right = Empty ? aX : std::max( Right, aX );
            Bottom = Empty ? aY : std::min( Bottom, aY );
            Top = Empty ? aY : std::max( Top, aY );
            Empty = false;
        }
    };

} // namespace


TEXT_METRICS::TEXT_METRICS( const SOURCE_TEXT_STYLE& aStyle ) :
        m_height( aStyle.Height ),
        m_interlinePercent( aStyle.InterlineHeightPercent ),
        m_charWidthPercent( aStyle.Font ? 100 : aStyle.CharWidthPercent ),
        m_proportional( !aStyle.Font && aStyle.Proportional ),
        m_typeFont( dynamic_cast<const SOURCE_TYPE_FONT*>( aStyle.Font ) != nullptr )
{
    // TrueType text has no pen width
    m_penWidth = m_typeFont ? 0 : aStyle.LineWidth;
}


void TEXT_METRICS::checkMeasurable() const
{
    if( m_typeFont )
        THROW_IO_ERROR( _( "Easy-PC TrueType text cannot be measured." ) );
}


int32_t TEXT_METRICS::linePitch() const
{
    return narrow( int64_t( m_height ) * m_interlinePercent / 100 );
}


int32_t TEXT_METRICS::charWidth( char aChar ) const
{
    int64_t w = int64_t( m_height ) * strokeCharWidthPercent( aChar, m_proportional ) / 100;

    if( m_charWidthPercent != 100 )
        w = m_charWidthPercent * w / 100;

    return narrow( w );
}


int32_t TEXT_METRICS::RunWidth( const std::string& aText, size_t aBegin, size_t aEnd ) const
{
    checkMeasurable();

    int64_t w = 0;

    for( size_t i = aBegin; i < aEnd; ++i )
        w += charWidth( aText[i] );

    return w != 0 ? narrow( w ) : m_height;
}


BOX2I TEXT_METRICS::TextBox( const wxString& aText, const VECTOR2I& aAnchor, int32_t aAngle, bool aMirrored,
                             int aHAlign, int aVAlign ) const
{
    const std::string text = Encode( aText );
    const size_t      n = text.size();

    if( n == 0 )
        return BOX2I();

    // A doubled underscore toggles the overbar and starts a run, a CR, LF,
    // CRLF or LFCR starts a line, and each run is measured on its own
    auto bar = [&]( size_t aPos )
    {
        return aPos + 1 < n && text[aPos] == '_' && text[aPos + 1] == '_';
    };

    std::vector<int64_t> widths;
    size_t               end = 0;

    for( bool first = true;; first = false )
    {
        if( bar( end ) )
            end += 2;

        if( end >= n )
            break;

        size_t begin = end;
        bool   newLine = text[begin] == '\n' || text[begin] == '\r';

        if( newLine )
        {
            ++begin;

            if( begin < n && text[begin] != text[begin - 1] && ( text[begin] == '\n' || text[begin] == '\r' ) )
                ++begin;
        }

        if( bar( begin ) )
            begin += 2;

        for( end = begin; end < n && text[end] != '\n' && text[end] != '\r' && !bar( end ); ++end )
        {
        }

        // The first run's newline flag is dropped
        if( first || newLine )
            widths.push_back( 0 );

        widths.back() = narrow( widths.back() + RunWidth( text, begin, end ) );
    }

    int64_t baseline = aAnchor.y;

    if( aVAlign == TEXT_V_TOP )
        baseline -= m_height;
    else if( aVAlign == TEXT_V_MIDDLE )
        baseline -= m_height >> 1;

    EXTENT_BOX box;

    for( size_t i = 0; i < widths.size(); ++i )
    {
        if( i > 0 )
            baseline -= linePitch();

        int64_t x = aAnchor.x;

        if( aHAlign == TEXT_H_RIGHT )
            x -= widths[i];
        else if( aHAlign == TEXT_H_CENTRE )
            x -= widths[i] >> 1;

        box.Include( narrow( x ), narrow( baseline ) );
        box.Include( narrow( x + widths[i] ), narrow( baseline + linePitch() ) );
    }

    int64_t grow = ( int64_t( m_penWidth ) + 1 ) >> 1;
    box.Left = narrow( box.Left - grow );
    box.Bottom = narrow( box.Bottom - grow );
    box.Right = narrow( box.Right + grow );
    box.Top = narrow( box.Top + grow );

    if( aMirrored )
    {
        int32_t left = narrow( 2 * int64_t( aAnchor.x ) - box.Right );
        box.Right = narrow( 2 * int64_t( aAnchor.x ) - box.Left );
        box.Left = left;
    }

    if( aAngle != 0 )
    {
        // The rotated box bounds the four rotated corners
        const VECTOR2I corners[4] = {
            { box.Right, box.Top }, { box.Left, box.Bottom }, { box.Right, box.Bottom }, { box.Left, box.Top }
        };
        EXTENT_BOX rotated;

        for( VECTOR2I c : corners )
        {
            rotate( c.x, c.y, aAngle, aAnchor );
            rotated.Include( c.x, c.y );
        }

        box = rotated;
    }

    BOX2I out( VECTOR2I( box.Left, box.Bottom ) );
    out.SetEnd( VECTOR2I( box.Right, box.Top ) );
    return out;
}


VECTOR2I TEXT_METRICS::UprightAnchor( const wxString& aText, const VECTOR2I& aAnchor, int32_t aAngle, bool aMirrored,
                                      int aHAlign, bool aKeepUpright ) const
{
    if( !aKeepUpright || aText.IsEmpty() || !flipsUpright( aAngle ) )
        return aAnchor;

    BOX2I box = TextBox( aText, aAnchor, 0, false, aHAlign );

    // Shrunk by half the pen width, after TextBox grew it by half rounded up
    int32_t shrink = m_penWidth / 2;
    int32_t x = narrow( aHAlign == TEXT_H_LEFT ? int64_t( box.GetEnd().x ) - shrink
                                               : int64_t( box.GetOrigin().x ) + shrink );
    int32_t y = narrow( int64_t( m_height ) + box.GetOrigin().y + shrink );

    if( aMirrored )
        x = narrow( 2 * int64_t( aAnchor.x ) - x );

    rotate( x, y, aAngle, aAnchor );
    return VECTOR2I( x, y );
}


std::string TEXT_METRICS::Encode( const wxString& aText )
{
    static const wxCSConv cp1252( wxFONTENCODING_CP1252 );
    std::string           bytes;

    bytes.reserve( aText.length() );

    for( wxUniChar ch : aText )
    {
        if( ch.IsAscii() )
        {
            bytes.push_back( static_cast<char>( ch.GetValue() ) );
            continue;
        }

        wxCharBuffer one = wxString( ch ).mb_str( cp1252 );
        bytes.push_back( one.length() == 1 ? one.data()[0] : '\0' );
    }

    return bytes;
}


bool TextStaysUpright( const SOURCE_DESIGN& aDesign, bool aSchematic )
{
    if( aDesign.AdjustTextRotation )
        return true;

    if( !aSchematic )
        return false;

    // Schematics before 3000 store no flag and are always upright
    return aDesign.Version <= 2999 || aDesign.Product == PRODUCT_DESIGNSPARK_CREATOR
           || aDesign.Product == PRODUCT_DESIGNSPARK_PRO || aDesign.Product == PRODUCT_PROTOPCB;
}


int32_t UprightRotation( int32_t aAngle, bool aKeepUpright )
{
    if( !aKeepUpright || !flipsUpright( aAngle ) )
        return aAngle;

    return ( aAngle + 180000 ) % ANGLE_FULL_TURN;
}


wxString ToKiCadMarkup( const wxString& aText )
{
    wxString out;
    bool     overbar = false;

    for( size_t i = 0; i < aText.length(); ++i )
    {
        wxUniChar c = aText[i];
        wxUniChar next = i + 1 < aText.length() ? aText[i + 1] : wxUniChar( 0 );

        if( c == '_' && next == '_' )
        {
            out += overbar ? wxS( "}" ) : wxS( "~{" );
            overbar = !overbar;
            ++i;
        }
        else if( c == '\n' || c == '\r' )
        {
            if( overbar )
                out += wxS( "}" );

            overbar = false;
            out += c;
        }
        else if( c == '~' && next == '{' )
        {
            // An empty overbar between them keeps KiCad from reading the pair as markup
            out += wxS( "~~{}{" );
            ++i;
        }
        else if( c == '}' && overbar )
        {
            // A brace would close the overbar, so it is drawn between two overbarred runs
            out += wxS( "}}~{" );
        }
        else
        {
            out += c;
        }
    }

    if( overbar )
        out += wxS( "}" );

    return out;
}

} // namespace EASYPC
