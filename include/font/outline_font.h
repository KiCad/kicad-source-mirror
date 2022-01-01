/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021-2022 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef OUTLINE_FONT_H_
#define OUTLINE_FONT_H_

#include <gal/graphics_abstraction_layer.h>
#include <geometry/shape_poly_set.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
//#include <gal/opengl/opengl_freetype.h>
#include <harfbuzz/hb.h>
#include <font/font.h>
#include <font/glyph.h>
#include <font/outline_decomposer.h>

namespace KIFONT
{
/**
 * Class OUTLINE_FONT implements outline font drawing.
 */
class OUTLINE_FONT : public FONT
{
public:
    OUTLINE_FONT();

    bool IsOutline() const override { return true; }

    bool IsBold() const override
    {
        return m_face && ( m_face->style_flags & FT_STYLE_FLAG_BOLD );
    }

    bool IsItalic() const override
    {
        return m_face && ( m_face->style_flags & FT_STYLE_FLAG_ITALIC );
    }

    /**
     * Load an outline font. TrueType (.ttf) and OpenType (.otf) are supported.
     * @param aFontFileName is the (platform-specific) fully qualified name of the font file
     */
    static OUTLINE_FONT* LoadFont( const wxString& aFontFileName, bool aBold, bool aItalic );

#if 0
    /**
     * Draw a string.
     *
     * @param aGal
     * @param aText is the text to be drawn.
     * @param aPosition is the text position in world coordinates.
     * @param aOrigin is the item origin
     * @param aAttributes contains text attributes (angle, line spacing, ...)
     * @return bounding box width/height
     */
    VECTOR2D Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                   const VECTOR2D& aOrigin, const TEXT_ATTRIBUTES& aAttributes ) const override;
#endif

    /**
     * Compute the boundary limits of aText (the bounding box of all shapes).
     *
     * The overbar and alignment are not taken in account, '~' characters are skipped.
     *
     * @return a VECTOR2D giving the width and height of text.
     */
    VECTOR2D StringBoundaryLimits( const KIGFX::GAL* aGal, const UTF8& aText,
                                   const VECTOR2D& aGlyphSize,
                                   double          aGlyphThickness ) const override;

    /**
     * Compute the vertical position of an overbar.  This is the distance between the text
     * baseline and the overbar.
     */
    double ComputeOverbarVerticalPosition( double aGlyphHeight ) const override;

    /**
     * Compute the distance (interline) between 2 lines of text (for multiline texts).  This is
     * the distance between baselines, not the space between line bounding boxes.
     */
    double GetInterline( double aGlyphHeight = 0.0, double aLineSpacing = 1.0 ) const override;

    /**
     * Compute the X and Y size of a given text. The text is expected to be a single line.
     */
    VECTOR2D ComputeTextLineSize( const KIGFX::GAL* aGal, const UTF8& aText ) const override;


    VECTOR2I GetTextAsGlyphs( BOX2I* aBoundingBox, std::vector<std::unique_ptr<GLYPH>>& aGlyphs,
                              const UTF8& aText, const VECTOR2D& aGlyphSize,
                              const wxPoint& aPosition, const EDA_ANGLE& aAngle,
                              TEXT_STYLE_FLAGS aTextStyle ) const override;

    /**
     * Like GetTextAsGlyphs, but handles multiple lines.
     * TODO: Combine with GetTextAsGlyphs, maybe with a boolean parameter,
     * but it's possible a non-line-breaking version isn't even needed
     *
     * @param aGlyphs returns text glyphs
     * @param aText the text item
     */
    VECTOR2I GetLinesAsGlyphs( std::vector<std::unique_ptr<GLYPH>>& aGlyphs,
                               const EDA_TEXT* aText ) const;

    const FT_Face& GetFace() const { return m_face; }

#if 0
    void RenderToOpenGLCanvas( KIGFX::OPENGL_FREETYPE& aTarget, const UTF8& aString,
                               const VECTOR2D& aGlyphSize, const wxPoint& aPosition,
                               double aOrientation, bool aIsMirrored ) const;
#endif

protected:
    VECTOR2D getBoundingBox( const UTF8& aString, const VECTOR2D& aGlyphSize,
                             TEXT_STYLE_FLAGS aTextStyle ) const override;


    FT_Error loadFace( const wxString& aFontFileName );

    bool loadFontSimple( const wxString& aFontFileName );

    BOX2I getBoundingBox( const std::vector<std::unique_ptr<GLYPH>>& aGlyphs ) const;

private:
    // FreeType variables
    static FT_Library m_freeType;
    FT_Face           m_face;
    const int         m_faceSize;
    FT_Face           m_subscriptFace;
    const int         m_subscriptSize;

    int m_faceScaler;
    int m_subscriptFaceScaler;

    // cache for glyphs converted to straight segments
    // key is glyph index (FT_GlyphSlot field glyph_index)
    std::map<unsigned int, GLYPH_POINTS_LIST> m_contourCache;
};

} //namespace KIFONT

#endif // OUTLINE_FONT_H_
