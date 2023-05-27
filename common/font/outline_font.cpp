/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski <gitlab@rinta-koski.net>
 * Copyright (C) 2021-2023 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * Outline font class
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

#include <limits>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <bezier_curves.h>
#include <geometry/shape_poly_set.h>
#include <eda_text.h>
#include <font/fontconfig.h>
#include <font/outline_font.h>
#include FT_GLYPH_H
#include FT_BBOX_H
#include <trigo.h>
#include <font/fontconfig.h>
#include <convert_basic_shapes_to_polygon.h>

using namespace KIFONT;


FT_Library OUTLINE_FONT::m_freeType = nullptr;
std::mutex OUTLINE_FONT::m_freeTypeMutex;

OUTLINE_FONT::OUTLINE_FONT() :
        m_face(NULL),
        m_faceSize( 16 ),
        m_fakeBold( false ),
        m_fakeItal( false )
{
    std::lock_guard<std::mutex> guard( m_freeTypeMutex );

    if( !m_freeType )
        FT_Init_FreeType( &m_freeType );
}


wxString OUTLINE_FONT::FreeTypeVersion()
{
    std::lock_guard<std::mutex> guard( m_freeTypeMutex );

    if( !m_freeType )
        FT_Init_FreeType( &m_freeType );

    FT_Int major = 0;
    FT_Int minor = 0;
    FT_Int patch = 0;
    FT_Library_Version( m_freeType, &major, &minor, &patch );

    return wxString::Format( "%d.%d.%d", major, minor, patch );
}


wxString OUTLINE_FONT::HarfBuzzVersion()
{
    return wxString::FromUTF8( HB_VERSION_STRING );
}


wxString OUTLINE_FONT::FontConfigVersion()
{
    return fontconfig::FONTCONFIG::Version();
}


wxString OUTLINE_FONT::FontLibraryVersion()
{
    return wxString::Format( "FreeType %s HarfBuzz %s", FreeTypeVersion(), HarfBuzzVersion() );
}


OUTLINE_FONT* OUTLINE_FONT::LoadFont( const wxString& aFontName, bool aBold, bool aItalic )
{
    std::unique_ptr<OUTLINE_FONT> font = std::make_unique<OUTLINE_FONT>();

    wxString fontFile;
    int      faceIndex;
    using fc = fontconfig::FONTCONFIG;

    fc::FF_RESULT retval = Fontconfig()->FindFont( aFontName, fontFile, faceIndex, aBold, aItalic );

    if( retval == fc::FF_RESULT::FF_ERROR )
        return nullptr;

    if( retval == fc::FF_RESULT::FF_MISSING_BOLD || retval == fc::FF_RESULT::FF_MISSING_BOLD_ITAL )
        font->SetFakeBold();

    if( retval == fc::FF_RESULT::FF_MISSING_ITAL || retval == fc::FF_RESULT::FF_MISSING_BOLD_ITAL )
        font->SetFakeItal();

    if( font->loadFace( fontFile, faceIndex ) != 0 )
        return nullptr;

    font->m_fontName = aFontName;       // Keep asked-for name, even if we substituted.
    font->m_fontFileName = fontFile;

    return font.release();
}


FT_Error OUTLINE_FONT::loadFace( const wxString& aFontFileName, int aFaceIndex )
{
    std::lock_guard<std::mutex> guard( m_freeTypeMutex );

    // TODO: check that going from wxString to char* with UTF-8
    // conversion for filename makes sense on any/all platforms
    FT_Error e = FT_New_Face( m_freeType, aFontFileName.mb_str( wxConvUTF8 ), aFaceIndex, &m_face );

    if( !e )
    {
        FT_Select_Charmap( m_face, FT_Encoding::FT_ENCODING_UNICODE );
        // params:
        // m_face = handle to face object
        // 0 = char width in 1/64th of points ( 0 = same as char height )
        // faceSize() = char height in 1/64th of points
        // GLYPH_RESOLUTION = horizontal device resolution (288dpi, 4x default)
        // 0 = vertical device resolution ( 0 = same as horizontal )
        FT_Set_Char_Size( m_face, 0, faceSize(), GLYPH_RESOLUTION, 0 );
    }

    return e;
}


/**
 * Compute the vertical position of an overbar.  This is the distance between the text
 * baseline and the overbar.
 */
double OUTLINE_FONT::ComputeOverbarVerticalPosition( double aGlyphHeight ) const
{
    // The overbar on actual text is positioned above the bounding box of the glyphs.  However,
    // that's expensive to calculate so we use an estimation here (as this is only used for
    // calculating bounding boxes).
    return aGlyphHeight * m_outlineFontSizeCompensation;
}


/**
 * Compute the vertical position of an underline.  This is the distance between the text
 * baseline and the underline.
 */
double OUTLINE_FONT::ComputeUnderlineVerticalPosition( double aGlyphHeight ) const
{
    return aGlyphHeight * m_underlineOffsetScaler;
}


/**
 * Compute the distance (interline) between 2 lines of text (for multiline texts).  This is
 * the distance between baselines, not the space between line bounding boxes.
 */
double OUTLINE_FONT::GetInterline( double aGlyphHeight, double aLineSpacing ) const
{
    double pitch = INTERLINE_PITCH_RATIO;

    if( GetFace()->units_per_EM )
        pitch = GetFace()->height / GetFace()->units_per_EM;

    double interline = aLineSpacing * aGlyphHeight * pitch * m_outlineFontSizeCompensation;

    // FONT TODO this hack is an attempt to fix interline spacing by eyeballing it
    static constexpr double interlineHackMultiplier = 1.2;
    interline *= interlineHackMultiplier;

    return interline;
}


static bool contourIsFilled( const CONTOUR& c )
{
    switch( c.m_Orientation )
    {
    case FT_ORIENTATION_TRUETYPE:   return c.m_Winding == 1;
    case FT_ORIENTATION_POSTSCRIPT: return c.m_Winding == -1;
    default:                        return false;
    }
}


static bool contourIsHole( const CONTOUR& c )
{
    return !contourIsFilled( c );
}


BOX2I OUTLINE_FONT::getBoundingBox( const std::vector<std::unique_ptr<GLYPH>>& aGlyphs ) const
{
    int minX = INT_MAX;
    int minY = INT_MAX;
    int maxX = INT_MIN;
    int maxY = INT_MIN;

    for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aGlyphs )
    {
        BOX2D bbox = glyph->BoundingBox();
        bbox.Normalize();

        if( minX > bbox.GetX() )
            minX = bbox.GetX();

        if( minY > bbox.GetY() )
            minY = bbox.GetY();

        if( maxX < bbox.GetRight() )
            maxX = bbox.GetRight();

        if( maxY < bbox.GetBottom() )
            maxY = bbox.GetBottom();
    }

    BOX2I ret;
    ret.SetOrigin( minX, minY );
    ret.SetEnd( maxX, maxY );
    return ret;
}


void OUTLINE_FONT::GetLinesAsGlyphs( std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                                     const wxString& aText, const VECTOR2I& aPosition,
                                     const TEXT_ATTRIBUTES& aAttrs ) const
{
    wxArrayString         strings;
    std::vector<VECTOR2I> positions;
    std::vector<VECTOR2I> extents;
    TEXT_STYLE_FLAGS      textStyle = 0;

    if( aAttrs.m_Italic )
        textStyle |= TEXT_STYLE::ITALIC;

    getLinePositions( aText, aPosition, strings, positions, extents, aAttrs );

    for( size_t i = 0; i < strings.GetCount(); i++ )
    {
        (void) drawMarkup( nullptr, aGlyphs, strings.Item( i ), positions[i], aAttrs.m_Size,
                           aAttrs.m_Angle, aAttrs.m_Mirrored, aPosition, textStyle );
    }
}


VECTOR2I OUTLINE_FONT::GetTextAsGlyphs( BOX2I* aBBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                                        const wxString& aText, const VECTOR2I& aSize,
                                        const VECTOR2I& aPosition, const EDA_ANGLE& aAngle,
                                        bool aMirror, const VECTOR2I& aOrigin,
                                        TEXT_STYLE_FLAGS aTextStyle ) const
{
    // HarfBuzz needs further processing to split tab-delimited text into text runs.

    constexpr double TAB_WIDTH = 4 * 0.6;

    VECTOR2I position = aPosition;
    wxString textRun;

    if( aBBox )
    {
        aBBox->SetOrigin( aPosition );
        aBBox->SetEnd( aPosition );
    }

    for( wxUniChar c : aText )
    {
        // Handle tabs as locked to the nearest 4th column (in space-widths).
        if( c == '\t' )
        {
            if( !textRun.IsEmpty() )
            {
                position = getTextAsGlyphs( aBBox, aGlyphs, textRun, aSize, position, aAngle,
                                            aMirror, aOrigin, aTextStyle );
                textRun.clear();
            }

            int tabWidth = KiROUND( aSize.x * TAB_WIDTH );
            int currentIntrusion = ( position.x - aOrigin.x ) % tabWidth;

            position.x += tabWidth - currentIntrusion;
        }
        else
        {
            textRun += c;
        }
    }

    if( !textRun.IsEmpty() )
    {
        position = getTextAsGlyphs( aBBox, aGlyphs, textRun, aSize, position, aAngle, aMirror,
                                    aOrigin, aTextStyle );
    }

    return position;
}


VECTOR2I OUTLINE_FONT::getTextAsGlyphs( BOX2I* aBBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                                        const wxString& aText, const VECTOR2I& aSize,
                                        const VECTOR2I& aPosition, const EDA_ANGLE& aAngle,
                                        bool aMirror, const VECTOR2I& aOrigin,
                                        TEXT_STYLE_FLAGS aTextStyle ) const
{
    std::lock_guard<std::mutex> guard( m_freeTypeMutex );

    return getTextAsGlyphsUnlocked( aBBox, aGlyphs, aText, aSize, aPosition, aAngle, aMirror,
                                    aOrigin, aTextStyle );
}

VECTOR2I OUTLINE_FONT::getTextAsGlyphsUnlocked( BOX2I* aBBox,
                                                std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                                                const wxString& aText, const VECTOR2I& aSize,
                                                const VECTOR2I& aPosition, const EDA_ANGLE& aAngle,
                                                bool aMirror, const VECTOR2I& aOrigin,
                                                TEXT_STYLE_FLAGS aTextStyle ) const
{
    VECTOR2D glyphSize = aSize;
    FT_Face  face = m_face;
    double   scaler = faceSize();

    if( IsSubscript( aTextStyle ) || IsSuperscript( aTextStyle ) )
    {
        scaler = subscriptSize();
    }

    // set glyph resolution so that FT_Load_Glyph() results are good enough for decomposing
    FT_Set_Char_Size( face, 0, scaler, GLYPH_RESOLUTION, 0 );

    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8( buf, UTF8( aText ).c_str(), -1, 0, -1 );
    hb_buffer_guess_segment_properties( buf );  // guess direction, script, and language based on
                                                // contents

    unsigned int         glyphCount;
    hb_glyph_info_t*     glyphInfo = hb_buffer_get_glyph_infos( buf, &glyphCount );
    hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions( buf, &glyphCount );

    hb_font_t* referencedFont = hb_ft_font_create_referenced( face );
    hb_ft_font_set_funcs( referencedFont );
    hb_shape( referencedFont, buf, nullptr, 0 );

    VECTOR2D scaleFactor( glyphSize.x / faceSize(), -glyphSize.y / faceSize() );
    scaleFactor = scaleFactor * m_outlineFontSizeCompensation;

    VECTOR2I cursor( 0, 0 );

    if( aGlyphs )
        aGlyphs->reserve( glyphCount );

    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        // Don't process glyphs that were already included in a previous cluster
        if( i > 0 && glyphInfo[i].cluster == glyphInfo[i-1].cluster )
            continue;

        if( aGlyphs )
        {
            if( m_fakeItal )
            {
                FT_Matrix matrix;
                // Create a 12 degree slant
                const float angle = (float)( -M_PI * 12.0f ) / 180.0f;
                matrix.xx = (FT_Fixed) ( cos( angle ) * 0x10000L );
                matrix.xy = (FT_Fixed) ( -sin( angle ) * 0x10000L );
                matrix.yx = (FT_Fixed) ( 0 * 0x10000L );  // Don't rotate in the y direction
                matrix.yy = (FT_Fixed) ( 1 * 0x10000L );

                FT_Set_Transform( face, &matrix, 0 );
            }

            FT_Load_Glyph( face, glyphInfo[i].codepoint, FT_LOAD_NO_BITMAP );

            if( m_fakeBold )
                FT_Outline_Embolden( &face->glyph->outline, 1 << 6 );

            // contours is a collection of all outlines in the glyph; for example the 'o' glyph
            // generally contains 2 contours, one for the glyph outline and one for the hole
            CONTOURS contours;

            OUTLINE_DECOMPOSER decomposer( face->glyph->outline );
            decomposer.OutlineToSegments( &contours );

            std::unique_ptr<OUTLINE_GLYPH> glyph = std::make_unique<OUTLINE_GLYPH>();
            std::vector<SHAPE_LINE_CHAIN>  holes;

            for( CONTOUR& c : contours )
            {
                GLYPH_POINTS     points = c.m_Points;
                SHAPE_LINE_CHAIN shape;

                shape.ReservePoints( points.size() );

                for( const VECTOR2D& v : points )
                {
                    VECTOR2D pt( v + cursor );

                    if( IsSubscript( aTextStyle ) )
                        pt.y += m_subscriptVerticalOffset * scaler;
                    else if( IsSuperscript( aTextStyle ) )
                        pt.y += m_superscriptVerticalOffset * scaler;

                    pt *= scaleFactor;
                    pt += aPosition;

                    if( aMirror )
                        pt.x = aOrigin.x - ( pt.x - aOrigin.x );

                    if( !aAngle.IsZero() )
                        RotatePoint( pt, aOrigin, aAngle );

                    shape.Append( pt.x, pt.y );
                }

                shape.SetClosed( true );

                if( contourIsHole( c ) )
                    holes.push_back( std::move( shape ) );
                else
                    glyph->AddOutline( std::move( shape ) );
            }

            for( SHAPE_LINE_CHAIN& hole : holes )
            {
                if( hole.PointCount() )
                {
                    for( int ii = 0; ii < glyph->OutlineCount(); ++ii )
                    {
                        if( glyph->Outline( ii ).PointInside( hole.GetPoint( 0 ) ) )
                        {
                            glyph->AddHole( std::move( hole ), ii );
                            break;
                        }
                    }
                }
            }

            aGlyphs->push_back( std::move( glyph ) );
        }

        hb_glyph_position_t& pos = glyphPos[i];
        cursor.x += ( pos.x_advance * GLYPH_SIZE_SCALER );
        cursor.y += ( pos.y_advance * GLYPH_SIZE_SCALER );
    }

    int      ascender = abs( face->size->metrics.ascender * GLYPH_SIZE_SCALER );
    int      descender = abs( face->size->metrics.descender * GLYPH_SIZE_SCALER );
    VECTOR2I extents( cursor.x * scaleFactor.x, ( ascender + descender ) * abs( scaleFactor.y ) );

    hb_buffer_destroy( buf );
    hb_font_destroy( referencedFont );

    VECTOR2I cursorDisplacement( cursor.x * scaleFactor.x, -cursor.y * scaleFactor.y );

    if( aBBox )
        aBBox->Merge( aPosition + extents );

    return VECTOR2I( aPosition.x + cursorDisplacement.x, aPosition.y + cursorDisplacement.y );
}


#undef OUTLINEFONT_RENDER_AS_PIXELS
#ifdef OUTLINEFONT_RENDER_AS_PIXELS
/*
 * WIP: eeschema (and PDF output?) should use pixel rendering instead of linear segmentation
 */
void OUTLINE_FONT::RenderToOpenGLCanvas( KIGFX::OPENGL_GAL& aGal, const wxString& aString,
                                         const VECTOR2D& aGlyphSize, const VECTOR2I& aPosition,
                                         const EDA_ANGLE& aOrientation, bool aIsMirrored ) const
{
    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8( buf, UTF8( aString ).c_str(), -1, 0, -1 );
    hb_buffer_guess_segment_properties( buf ); // guess direction, script, and language based on contents

    unsigned int         glyphCount;
    hb_glyph_info_t*     glyphInfo = hb_buffer_get_glyph_infos( buf, &glyphCount );
    hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions( buf, &glyphCount );

    std::lock_guard<std::mutex> guard( m_freeTypeMutex );

    hb_font_t*           referencedFont = hb_ft_font_create_referenced( m_face );

    hb_ft_font_set_funcs( referencedFont );
    hb_shape( referencedFont, buf, nullptr, 0 );

    const double mirror_factor = ( aIsMirrored ? 1 : -1 );
    const double x_scaleFactor = mirror_factor * aGlyphSize.x / mScaler;
    const double y_scaleFactor = aGlyphSize.y / mScaler;

    hb_position_t cursor_x = 0;
    hb_position_t cursor_y = 0;

    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        hb_glyph_position_t& pos = glyphPos[i];
        int                  codepoint = glyphInfo[i].codepoint;

        FT_Error e = FT_Load_Glyph( m_face, codepoint, FT_LOAD_DEFAULT );
        // TODO handle FT_Load_Glyph error

        FT_Glyph glyph;
        e = FT_Get_Glyph( m_face->glyph, &glyph );
        // TODO handle FT_Get_Glyph error

        wxPoint pt( aPosition );
        pt.x += ( cursor_x >> 6 ) * x_scaleFactor;
        pt.y += ( cursor_y >> 6 ) * y_scaleFactor;

        cursor_x += pos.x_advance;
        cursor_y += pos.y_advance;
    }

    hb_buffer_destroy( buf );
}
#endif //OUTLINEFONT_RENDER_AS_PIXELS
