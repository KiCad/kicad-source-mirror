/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski <gitlab@rinta-koski.net>
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

#include <limits>
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <bezier_curves.h>
#include <geometry/shape_poly_set.h>
#include <font/fontconfig.h>
#include <font/outline_font.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_TABLES_H
#include FT_GLYPH_H
#include FT_BBOX_H
#include <trigo.h>
#include <core/utf8.h>

using namespace KIFONT;


FT_Library OUTLINE_FONT::m_freeType = nullptr;
std::mutex OUTLINE_FONT::m_freeTypeMutex;

OUTLINE_FONT::OUTLINE_FONT() :
        m_face(NULL),
        m_faceSize( 16 ),
        m_fakeBold( false ),
        m_fakeItal( false ),
        m_forDrawingSheet( false )
{
    std::lock_guard<std::mutex> guard( m_freeTypeMutex );

    if( !m_freeType )
        FT_Init_FreeType( &m_freeType );
}


OUTLINE_FONT::EMBEDDING_PERMISSION OUTLINE_FONT::GetEmbeddingPermission() const
{
    TT_OS2* os2 = reinterpret_cast<TT_OS2*>( FT_Get_Sfnt_Table( m_face, FT_SFNT_OS2 ) );

    // If this table isn't present, we can't assume anything
    if( !os2 )
        return EMBEDDING_PERMISSION::RESTRICTED;

    // We don't support bitmap fonts, so this disables embedding
    if( os2->fsType & FT_FSTYPE_BITMAP_EMBEDDING_ONLY )
        return EMBEDDING_PERMISSION::RESTRICTED;

    // Per the OpenType spec, only bits 0-3 of fsType define the embedding license.
    // Bits 8-9 (no-subsetting, bitmap-only) are independent modifiers and must be
    // masked off before checking the embedding permission level.
    // See: http://freetype.org/freetype2/docs/reference/ft2-information_retrieval.html
    FT_UShort embeddingBits = os2->fsType & 0x000F;

    // This allows the font to be exported from KiCad
    if( embeddingBits == FT_FSTYPE_INSTALLABLE_EMBEDDING )
        return EMBEDDING_PERMISSION::INSTALLABLE;

    // This allows us to use the font in KiCad but not export
    if( embeddingBits & FT_FSTYPE_EDITABLE_EMBEDDING )
        return EMBEDDING_PERMISSION::EDITABLE;

    // This is not actually supported by KiCad ATM(2024)
    if( embeddingBits & FT_FSTYPE_PREVIEW_AND_PRINT_EMBEDDING )
        return EMBEDDING_PERMISSION::PRINT_PREVIEW_ONLY;

    // Anything else that is not explicitly enabled we treat as restricted.
    return EMBEDDING_PERMISSION::RESTRICTED;
}


OUTLINE_FONT* OUTLINE_FONT::LoadFont( const wxString& aFontName, bool aBold, bool aItalic,
                                      const std::vector<wxString>* aEmbeddedFiles,
                                      bool aForDrawingSheet )
{
    std::unique_ptr<OUTLINE_FONT> font = std::make_unique<OUTLINE_FONT>();

    wxString fontFile;
    int      faceIndex;
    using fc = fontconfig::FONTCONFIG;


    fc::FF_RESULT retval = Fontconfig()->FindFont( aFontName, fontFile, faceIndex, aBold, aItalic,
                                                   aEmbeddedFiles );

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
    font->m_forDrawingSheet = aForDrawingSheet;

    return font.release();
}


FT_Error OUTLINE_FONT::loadFace( const wxString& aFontFileName, int aFaceIndex )
{
    std::lock_guard<std::mutex> guard( m_freeTypeMutex );

    FT_Error e = FT_New_Face( m_freeType, aFontFileName.mb_str( wxConvUTF8 ), aFaceIndex, &m_face );

    if( !e )
    {
        SelectCharmap( m_face );
        // params:
        // m_face = handle to face object
        // 0 = char width in 1/64th of points ( 0 = same as char height )
        // faceSize() = char height in 1/64th of points
        // GLYPH_RESOLUTION = horizontal device resolution (1152dpi, 16x default)
        // 0 = vertical device resolution ( 0 = same as horizontal )
        FT_Set_Char_Size( m_face, 0, faceSize(), GLYPH_RESOLUTION, 0 );
    }

    return e;
}


void OUTLINE_FONT::SelectCharmap( FT_Face aFace )
{
    // A normal text font carries a Unicode charmap that maps the Basic Latin block directly.
    // Keep it when present.
    if( FT_Select_Charmap( aFace, FT_ENCODING_UNICODE ) == 0 )
    {
        // Some legacy "symbol" fonts expose a Unicode charmap that only mirrors their private-use
        // (U+F000..U+F0FF) glyph layout, leaving ASCII unmapped. Probe a few common characters to
        // tell a usable Unicode charmap apart from such a font.
        static const FT_ULong probes[] = { 'A', 'a', '0', ' ' };

        for( FT_ULong codepoint : probes )
        {
            if( FT_Get_Char_Index( aFace, codepoint ) != 0 )
                return;
        }
    }

    // No Unicode charmap can resolve ASCII. If the font carries a Microsoft Symbol charmap,
    // select it so HarfBuzz applies its U+F000 offset remapping and the glyphs become reachable.
    if( FT_Select_Charmap( aFace, FT_ENCODING_MS_SYMBOL ) == 0 )
        return;

    // Otherwise fall back to the Unicode charmap (e.g. a CJK-only font with no Basic Latin).
    FT_Select_Charmap( aFace, FT_ENCODING_UNICODE );
}


double OUTLINE_FONT::GetInterline( double aGlyphHeight, const METRICS& aFontMetrics ) const
{
    double glyphToFontHeight = 1.0;

    // Both FreeType metrics are integers, so the ratio must be computed in floating point.
    // Truncated integer division collapsed the spacing to zero for fonts with height < units_per_EM.
    if( GetFace() && GetFace()->units_per_EM > 0 && GetFace()->height > 0 )
        glyphToFontHeight = static_cast<double>( GetFace()->height )
                            / static_cast<double>( GetFace()->units_per_EM );

    return aFontMetrics.GetInterline( aGlyphHeight * glyphToFontHeight );
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
                                     const TEXT_ATTRIBUTES& aAttrs,
                                     const METRICS& aFontMetrics ) const
{
    wxArrayString         strings;
    std::vector<VECTOR2I> positions;
    std::vector<VECTOR2I> extents;
    TEXT_STYLE_FLAGS      textStyle = 0;

    if( aAttrs.m_Italic )
        textStyle |= TEXT_STYLE::ITALIC;

    getLinePositions( aText, aPosition, strings, positions, extents, aAttrs, aFontMetrics );

    for( size_t i = 0; i < strings.GetCount(); i++ )
    {
        (void) drawMarkup( nullptr, aGlyphs, strings.Item( i ), positions[i], aAttrs.m_Size,
                           aAttrs.m_Angle, aAttrs.m_Mirrored, aPosition, textStyle, aFontMetrics );
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


struct HARFBUZZ_CACHE_KEY {
    FT_Face     face;
    std::string text;
    int         scaler;

    bool operator==(const HARFBUZZ_CACHE_KEY& rhs ) const
    {
        return face == rhs.face
                && scaler == rhs.scaler
                && text == rhs.text;
    }
};


struct HARFBUZZ_CACHE_ENTRY
{
    std::vector<hb_glyph_info_t>     m_GlyphInfo;
    std::vector<hb_glyph_position_t> m_GlyphPositions;
    bool                             m_Initialized = false;
};


namespace std
{
    template <>
    struct hash<HARFBUZZ_CACHE_KEY>
    {
        std::size_t operator()( const HARFBUZZ_CACHE_KEY& k ) const
        {
            return hash_val( k.face, k.scaler, k.text );
        }
    };
}


static const HARFBUZZ_CACHE_ENTRY& getHarfbuzzShape( FT_Face aFace, const wxString& aText,
                                                     int aScaler )
{
    static std::unordered_map<HARFBUZZ_CACHE_KEY, HARFBUZZ_CACHE_ENTRY> s_harfbuzzCache;

    std::string textUtf8 = UTF8( aText );
    HARFBUZZ_CACHE_KEY key = { aFace, textUtf8, aScaler };

    HARFBUZZ_CACHE_ENTRY& entry = s_harfbuzzCache[key];

    if( !entry.m_Initialized )
    {
        hb_buffer_t* buf = hb_buffer_create();
        hb_buffer_add_utf8( buf, textUtf8.c_str(), -1, 0, -1 );
        hb_buffer_guess_segment_properties( buf );  // guess direction, script, and language based on
                                                    // contents

        hb_font_t* referencedFont = hb_ft_font_create_referenced( aFace );

        hb_shape( referencedFont, buf, nullptr, 0 );

        unsigned int         glyphCount;
        hb_glyph_info_t*     glyphInfo = hb_buffer_get_glyph_infos( buf, &glyphCount );
        hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions( buf, &glyphCount );

        entry.m_GlyphInfo.assign( glyphInfo, glyphInfo + glyphCount );
        entry.m_GlyphPositions.assign( glyphPos, glyphPos + glyphCount );
        entry.m_Initialized = true;

        hb_buffer_destroy( buf );
        hb_font_destroy( referencedFont );
    }

    return entry;
}


struct GLYPH_CACHE_KEY {
    FT_Face        face;
    hb_codepoint_t codepoint;
    VECTOR2D       scale;
    bool           forDrawingSheet;
    bool           fakeItalic;
    bool           fakeBold;
    bool           mirror;
    bool           supersub;
    EDA_ANGLE      angle;

    bool operator==(const GLYPH_CACHE_KEY& rhs ) const
    {
        return face == rhs.face
                && codepoint == rhs.codepoint
                && scale == rhs.scale
                && forDrawingSheet == rhs.forDrawingSheet
                && fakeItalic == rhs.fakeItalic
                && fakeBold == rhs.fakeBold
                && mirror == rhs.mirror
                && supersub == rhs.supersub
                && angle == rhs.angle;
    }
};


namespace std
{
    template <>
    struct hash<GLYPH_CACHE_KEY>
    {
        std::size_t operator()( const GLYPH_CACHE_KEY& k ) const
        {
            return hash_val( k.face, k.codepoint, k.scale.x, k.scale.y, k.forDrawingSheet,
                             k.fakeItalic, k.fakeBold, k.mirror, k.supersub, k.angle.AsDegrees() );
        }
    };
}


bool OUTLINE_FONT::LoadGlyphContours( unsigned int aGlyphIndex,
                                      std::vector<CONTOUR>& aContours ) const
{
    aContours.clear();

    if( m_fakeItal )
    {
        FT_Matrix matrix;
        // Create a 12 degree slant
        const float angle = (float) ( -M_PI * 12.0f ) / 180.0f;
        matrix.xx = (FT_Fixed) ( cos( angle ) * 0x10000L );
        matrix.xy = (FT_Fixed) ( -sin( angle ) * 0x10000L );
        matrix.yx = 0;  // Don't rotate in the y direction
        matrix.yy = 0x10000L;

        FT_Set_Transform( m_face, &matrix, nullptr );
    }

    // FreeType clears the shared glyph slot before the font driver runs, so a failed load leaves
    // an empty or partially parsed outline. Decomposing it would silently render nothing, so bail
    // and let the caller draw a placeholder box instead.
    if( FT_Load_Glyph( m_face, aGlyphIndex, FT_LOAD_NO_BITMAP ) != 0 )
        return false;

    // Bitmap-only fonts ignore FT_LOAD_NO_BITMAP and load without error, leaving no outline.
    if( m_face->glyph->format != FT_GLYPH_FORMAT_OUTLINE )
        return false;

    if( m_fakeBold )
        FT_Outline_Embolden( &m_face->glyph->outline, 1 << 6 );

    OUTLINE_DECOMPOSER decomposer( m_face->glyph->outline );

    if( !decomposer.OutlineToSegments( &aContours ) )
    {
        aContours.clear();
        return false;
    }

    return true;
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
    bool     supersub = IsSuperscript( aTextStyle ) || IsSubscript( aTextStyle );

    if( supersub )
        scaler = subscriptSize();

    // set glyph resolution so that FT_Load_Glyph() results are good enough for decomposing
    FT_Set_Char_Size( face, 0, scaler, GLYPH_RESOLUTION, 0 );

    const HARFBUZZ_CACHE_ENTRY& hbShape = getHarfbuzzShape( face, aText, scaler );

    unsigned int glyphCount = static_cast<unsigned int>( hbShape.m_GlyphInfo.size() );
    const hb_glyph_info_t* glyphInfo = hbShape.m_GlyphInfo.data();
    const hb_glyph_position_t* glyphPos = hbShape.m_GlyphPositions.data();

    VECTOR2D scaleFactor( glyphSize.x / faceSize(), -glyphSize.y / faceSize() );
    scaleFactor = scaleFactor * m_outlineFontSizeCompensation;

    VECTOR2I cursor( 0, 0 );

    if( aGlyphs )
        aGlyphs->reserve( glyphCount );

    // GLYPH_DATA is a collection of all outlines in the glyph; for example the 'o' glyph
    // generally contains 2 contours, one for the glyph outline and one for the hole
    static std::unordered_map<GLYPH_CACHE_KEY, GLYPH_DATA> s_glyphCache;

    for( unsigned int i = 0; i < glyphCount; i++ )
    {
        if( aGlyphs )
        {
            GLYPH_CACHE_KEY key = { face, glyphInfo[i].codepoint, scaleFactor, m_forDrawingSheet,
                                    m_fakeItal, m_fakeBold, aMirror, supersub, aAngle };
            GLYPH_DATA&     glyphData = s_glyphCache[ key ];

            if( !glyphData.m_Loaded )
            {
                glyphData.m_Loaded = true;

                if( !LoadGlyphContours( glyphInfo[i].codepoint, glyphData.m_Contours ) )
                {
                    double  hb_advance = glyphPos[i].x_advance * GLYPH_SIZE_SCALER;
                    BOX2D   tofuBox( { scaler * 0.03, 0.0 },
                                     { hb_advance - scaler * 0.02, scaler * 0.72 } );

                    CONTOUR outline;
                    outline.m_Winding = 1;
                    outline.m_Orientation = FT_ORIENTATION_TRUETYPE;
                    outline.m_Points.push_back( tofuBox.GetPosition() );
                    outline.m_Points.push_back( { tofuBox.GetSize().x, tofuBox.GetPosition().y } );
                    outline.m_Points.push_back( tofuBox.GetSize() );
                    outline.m_Points.push_back( { tofuBox.GetPosition().x, tofuBox.GetSize().y } );
                    glyphData.m_Contours.push_back( std::move( outline ) );

                    CONTOUR hole;
                    tofuBox.Move( { scaler * 0.06, scaler * 0.06 } );
                    tofuBox.SetSize( { tofuBox.GetWidth() - scaler * 0.06,
                                       tofuBox.GetHeight() - scaler * 0.06 } );
                    hole.m_Winding = 1;
                    hole.m_Orientation = FT_ORIENTATION_NONE;
                    hole.m_Points.push_back( tofuBox.GetPosition() );
                    hole.m_Points.push_back( { tofuBox.GetSize().x, tofuBox.GetPosition().y } );
                    hole.m_Points.push_back( tofuBox.GetSize() );
                    hole.m_Points.push_back( { tofuBox.GetPosition().x, tofuBox.GetSize().y } );
                    glyphData.m_Contours.push_back( std::move( hole ) );
                }
            }

            std::unique_ptr<OUTLINE_GLYPH> glyph = std::make_unique<OUTLINE_GLYPH>();
            std::vector<SHAPE_LINE_CHAIN>  holes;

            for( const CONTOUR& c : glyphData.m_Contours )
            {
                const std::vector<VECTOR2D>& points = c.m_Points;
                SHAPE_LINE_CHAIN             shape;

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
                bool added_hole = false;

                if( hole.PointCount() )
                {
                    for( int ii = 0; ii < glyph->OutlineCount(); ++ii )
                    {
                        if( glyph->Outline( ii ).PointInside( hole.GetPoint( 0 ) ) )
                        {
                            glyph->AddHole( std::move( hole ), ii );
                            added_hole = true;
                            break;
                        }
                    }

                    // Some lovely TTF fonts decided that winding didn't matter for outlines that
                    // don't have holes, so holes that don't fit in any outline are added as
                    // outlines.
                    if( !added_hole )
                        glyph->AddOutline( std::move( hole ) );
                }
            }

            if( glyphData.m_TriangulationData.empty() )
            {
                glyph->CacheTriangulation();
                glyphData.m_TriangulationData = glyph->GetTriangulationData();
            }
            else
            {
                glyph->CacheTriangulation( glyphData.m_TriangulationData );
            }

            aGlyphs->push_back( std::move( glyph ) );
        }

        const hb_glyph_position_t& pos = glyphPos[i];
        cursor.x += ( pos.x_advance * GLYPH_SIZE_SCALER );
        cursor.y += ( pos.y_advance * GLYPH_SIZE_SCALER );
    }

    int ascender = abs( face->size->metrics.ascender * GLYPH_SIZE_SCALER );
    int descender = abs( face->size->metrics.descender * GLYPH_SIZE_SCALER );

    if( aBBox )
    {
        aBBox->Merge( aPosition - VECTOR2I( 0, ascender * abs( scaleFactor.y ) ) );
        aBBox->Merge( aPosition + VECTOR2I( cursor.x * scaleFactor.x, descender * abs( scaleFactor.y ) ) );
    }

    return VECTOR2I( aPosition.x + cursor.x * scaleFactor.x, aPosition.y - cursor.y * scaleFactor.y );
}
