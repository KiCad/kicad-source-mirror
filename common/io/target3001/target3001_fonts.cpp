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

#include "target3001_fonts.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <eda_text.h>
#include <font/glyph_types.h>
#include <font/outline_font.h>
#include <geometry/eda_angle.h>
#include <geometry/shape_compound.h>
#include <math/util.h>
#include <trigo.h>

#include "target3001_parser.h"

namespace TARGET3001
{

namespace
{

constexpr double MASTER_EM = 300.0;
constexpr double HEIGHT_DIVISOR = 222.0;
constexpr double CALIBRI_HEIGHT_DIVISOR = 241.0;

// Text weight times this is the Windows font weight; 16 and above select a bold face
constexpr int FONT_WEIGHT_PER_WEIGHT = 35;
constexpr int BOLD_FONT_WEIGHT = 551;

// Measuring at a large size keeps integer advances precise
constexpr int PROBE_SIZE = 1000000;

// Built-in fonts stroke at this share of their size per weight unit
constexpr double STROKE_PER_WEIGHT = 0.01;

// A zero weight marks hidden text, which keeps the default weight
constexpr int DEFAULT_WEIGHT = 10;

constexpr double DIN_CAP_HEIGHT = 0.9;

// DIN-ISO-ANSI width for text too short to fit, the average advance in KiCad's stroke font
constexpr double DIN_WIDTH = 0.68;

// DIN-ISO-ANSI glyphs are this share of the size plus one stroke apart, and a space widens the gap by DIN_SPACE
constexpr double DIN_GAP = 0.1;
constexpr double DIN_SPACE = 0.43;

// DIN-ISO-ANSI glyph widths between stroke centres in DIN_GRID units of the size; others take DIN_DEFAULT_UNITS
constexpr double DIN_GRID = 0.045;
constexpr int    DIN_DEFAULT_UNITS = 8;

constexpr std::pair<wchar_t, int> DIN_UNITS[] = {
    { '!', 0 },  { '(', 4 },  { ')', 4 },  { '+', 10 }, { ',', 2 },  { '-', 9 },  { '.', 0 },  { '/', 9 },
    { '1', 4 },  { ':', 2 },  { ';', 2 },  { 'A', 14 }, { 'B', 10 }, { 'C', 10 }, { 'D', 10 }, { 'E', 10 },
    { 'F', 10 }, { 'G', 10 }, { 'H', 10 }, { 'I', 0 },  { 'K', 10 }, { 'L', 9 },  { 'M', 14 }, { 'N', 10 },
    { 'O', 10 }, { 'P', 10 }, { 'Q', 10 }, { 'R', 10 }, { 'S', 10 }, { 'T', 10 }, { 'U', 10 }, { 'V', 10 },
    { 'W', 20 }, { 'X', 10 }, { 'Z', 10 }, { '_', 19 }, { 'a', 10 }, { 'f', 6 },  { 'i', 0 },  { 'k', 9 },
    { 'l', 3 },  { 'm', 16 }, { 'r', 5 },  { 't', 6 },  { 'v', 10 }, { 'w', 16 }, { 'y', 10 }, { 'z', 9 },
    { L'\u00B5', 10 }, { L'\u00E4', 10 }
};

// TrueType OS/2 version 1 weights for the average character width, per 1000 characters
constexpr std::pair<char, int> AVERAGE_WIDTH_WEIGHTS[] = {
    { 'a', 64 }, { 'b', 14 }, { 'c', 27 }, { 'd', 35 }, { 'e', 100 }, { 'f', 20 }, { 'g', 14 },
    { 'h', 42 }, { 'i', 63 }, { 'j', 3 },  { 'k', 6 },  { 'l', 35 },  { 'm', 20 }, { 'n', 56 },
    { 'o', 56 }, { 'p', 17 }, { 'q', 4 },  { 'r', 49 }, { 's', 56 },  { 't', 71 }, { 'u', 31 },
    { 'v', 10 }, { 'w', 18 }, { 'x', 3 },  { 'y', 18 }, { 'z', 2 },   { ' ', 166 }
};


/// Stroke centre of each DIN-ISO-ANSI glyph from the pen, spaces excluded
std::vector<double> dinCentres( const wxString& aText, double aSize, double aPen )
{
    std::vector<double> centres;
    double              cursor = 0.0;

    for( wxUniChar c : aText )
    {
        if( c == ' ' )
        {
            cursor += DIN_SPACE * aSize;
            continue;
        }

        int units = DIN_DEFAULT_UNITS;

        for( const auto& [ch, chUnits] : DIN_UNITS )
        {
            if( c.GetValue() == wxUniChar::value_type( ch ) )
            {
                units = chUnits;
                break;
            }
        }

        double glyphWidth = units * DIN_GRID * aSize;

        centres.push_back( cursor + glyphWidth / 2.0 );
        cursor += glyphWidth + DIN_GAP * aSize + aPen;
    }

    return centres;
}


/// Leftmost and rightmost stroke of each glyph of @a aText in KiCad's stroke font at the probe size
std::vector<std::pair<double, double>> strokeSpans( const wxString& aText )
{
    std::vector<std::unique_ptr<KIFONT::GLYPH>> glyphs;
    std::vector<std::pair<double, double>>      spans;

    KIFONT::FONT::GetFont()->GetTextAsGlyphs( nullptr, &glyphs, aText, VECTOR2I( PROBE_SIZE, PROBE_SIZE ),
                                              VECTOR2I( 0, 0 ), ANGLE_0, false, VECTOR2I( 0, 0 ), 0 );

    for( const std::unique_ptr<KIFONT::GLYPH>& glyph : glyphs )
    {
        std::pair<double, double> span( std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest() );

        for( const std::vector<VECTOR2D>& stroke : *static_cast<KIFONT::STROKE_GLYPH*>( glyph.get() ) )
        {
            for( const VECTOR2D& pt : stroke )
            {
                span.first = std::min( span.first, pt.x );
                span.second = std::max( span.second, pt.x );
            }
        }

        if( span.first <= span.second )
            spans.push_back( span );
    }

    return spans;
}

} // namespace


void FONTS::Apply( EDA_TEXT* aText, const ELEMENT& aElem, int aUnit )
{
    if( applyTrueType( aText, aElem, aUnit ) )
        return;

    const TEXT& t = aElem.text;
    int         weight = t.weight ? t.weight : DEFAULT_WEIGHT;
    double      height = double( t.height ) / aUnit;
    double      width = double( t.width ) / aUnit;
    VECTOR2D    size( width, height );
    double      strokeBase = width;

    if( ToWx( t.font ).Trim().Trim( false ).IsSameAs( wxS( "DIN-ISO-ANSI" ), false ) )
    {
        double                                 scale = std::max( height, width );
        double                                 pen = scale * weight * STROKE_PER_WEIGHT;
        std::vector<double>                    din = dinCentres( aText->GetText(), scale, pen );
        std::vector<std::pair<double, double>> spans = strokeSpans( aText->GetText() );
        double                                 stem = strokeSpans( wxS( "H" ) ).front().first;
        double                                 cross = 0.0;
        double                                 norm = 0.0;

        size = VECTOR2D( DIN_WIDTH * scale, DIN_CAP_HEIGHT * scale );
        strokeBase = scale;

        // Least squares fit of the glyph centres, measured from the stem of H where both fonts start
        if( din.size() > 1 && din.size() == spans.size() )
        {
            for( size_t ii = 0; ii < din.size(); ++ii )
            {
                double centre = ( spans[ii].first + spans[ii].second ) / 2.0 - stem;

                cross += centre * din[ii];
                norm += centre * centre;
            }
        }

        if( cross > 0.0 && norm > 0.0 )
            size.x = cross / norm * PROBE_SIZE;
    }
    else
    {
        // Fixed pitch, so the string length is known exactly and sets the width
        wxString text = aText->GetText();
        double   pitch = width * ( 1.0 + 2.0 * STROKE_PER_WEIGHT * weight );
        VECTOR2I end = KIFONT::FONT::GetFont()->GetTextAsGlyphs( nullptr, nullptr, text,
                                                                 VECTOR2I( PROBE_SIZE, PROBE_SIZE ),
                                                                 VECTOR2I( 0, 0 ), ANGLE_0, false,
                                                                 VECTOR2I( 0, 0 ), 0 );

        if( end.x > 0 )
            size.x = pitch * text.length() * PROBE_SIZE / end.x;
    }

    aText->SetTextSize( VECTOR2I( std::max( 1, KiROUND( size.x ) ), std::max( 1, KiROUND( size.y ) ) ) );
    aText->SetTextThickness( std::max( 1, KiROUND( strokeBase * weight * STROKE_PER_WEIGHT ) ) );
}


VECTOR2I FONTS::BaselineShift( const EDA_TEXT& aText )
{
    EDA_TEXT probe( aText );
    bool     stroke = !aText.GetFont() || aText.GetFont()->IsStroke();
    int      halfPen = stroke ? aText.GetEffectiveTextPenWidth() / 2 : 0;

    // H stands on the baseline with its stem at the pen in both built-in fonts
    probe.SetText( wxS( "H" ) );
    probe.SetTextPos( VECTOR2I( 0, 0 ) );
    probe.SetTextAngle( ANGLE_0 );
    probe.SetMirrored( false );
    probe.SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    probe.SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );

    // Stroke ends are round, so the stroke centre lies half a pen inside the shape's bounds
    BOX2I    ink = probe.GetEffectiveTextShape( false )->BBox();
    VECTOR2I shift( -( ink.GetLeft() + halfPen ), -( ink.GetBottom() - halfPen ) );

    if( !stroke || aText.GetHorizJustify() == GR_TEXT_H_ALIGN_RIGHT )
        shift.x = 0;

    if( aText.IsMirrored() )
        shift.x = -shift.x;

    RotatePoint( shift, aText.GetDrawRotation() );
    return shift;
}


bool FONTS::applyTrueType( EDA_TEXT* aText, const ELEMENT& aElem, int aUnit )
{
    const TEXT& t = aElem.text;
    wxString    name = ToWx( t.font ).Trim().Trim( false );
    wxString    key = name.Upper();

    if( key.IsEmpty() || key == wxS( "TARGET" ) || key == wxS( "DIN-ISO-ANSI" ) )
        return false;

    // Hidden text keeps its stored weight of zero, which renders regular
    bool bold = t.weight * FONT_WEIGHT_PER_WEIGHT >= BOLD_FONT_WEIGHT;
    bool italic = aElem.base.FlagProp( "ITALIC" );
    auto fontKey = std::make_tuple( key, bold, italic );
    auto fontIt = m_fonts.find( fontKey );

    if( fontIt == m_fonts.end() )
    {
        KIFONT::FONT*         font = KIFONT::FONT::GetFont( name, bold, italic );
        KIFONT::OUTLINE_FONT* outline = font && font->IsOutline() ? static_cast<KIFONT::OUTLINE_FONT*>( font )
                                                                  : nullptr;

        fontIt = m_fonts.emplace( fontKey, outline ).first;
    }

    KIFONT::OUTLINE_FONT* outline = fontIt->second;

    if( !outline )
        return false;

    auto widthIt = m_averageWidth.find( outline );

    if( widthIt == m_averageWidth.end() )
    {
        double total = 0.0;

        for( const auto& [ch, share] : AVERAGE_WIDTH_WEIGHTS )
        {
            VECTOR2I end = outline->GetTextAsGlyphs( nullptr, nullptr, wxString( ch ),
                                                     VECTOR2I( PROBE_SIZE, PROBE_SIZE ), VECTOR2I( 0, 0 ), ANGLE_0,
                                                     false, VECTOR2I( 0, 0 ), 0 );
            total += double( end.x ) * share;
        }

        widthIt = m_averageWidth.emplace( outline, total / 1000.0 / PROBE_SIZE ).first;
    }

    double divisor = key == wxS( "CALIBRI" ) ? CALIBRI_HEIGHT_DIVISOR : HEIGHT_DIVISOR;
    double em = MASTER_EM / divisor * t.height / aUnit;
    double width = widthIt->second > 0.0 ? double( t.width ) / aUnit / widthIt->second : double( t.width ) / aUnit;

    aText->SetFont( outline );
    aText->SetBoldFlag( bold );
    aText->SetItalicFlag( italic );
    aText->SetTextSize( VECTOR2I( std::max( 1, KiROUND( width ) ),
                                  std::max( 1, KiROUND( em / outline->GetEmSize( 1.0 ) ) ) ) );
    return true;
}

} // namespace TARGET3001
