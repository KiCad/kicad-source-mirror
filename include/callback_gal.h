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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef CALLBACK_GAL_H
#define CALLBACK_GAL_H

#include <gal/gal.h>
#include <gal/graphics_abstraction_layer.h>

class GAL_API CALLBACK_GAL : public KIGFX::GAL
{
public:
    CALLBACK_GAL( KIGFX::GAL_DISPLAY_OPTIONS& aDisplayOptions,
                  std::function<void( const VECTOR2I& aPt1,
                                      const VECTOR2I& aPt2 )> aStrokeCallback,
                  std::function<void( const VECTOR2I& aPt1,
                                      const VECTOR2I& aPt2,
                                      const VECTOR2I& aPt3 )> aTriangleCallback ) :
        GAL( aDisplayOptions )
    {
        m_strokeCallback = std::move( aStrokeCallback );
        m_triangleCallback = std::move( aTriangleCallback );
        m_outlineCallback = []( const SHAPE_LINE_CHAIN& ) {};
        m_stroke = true;
        m_triangulate = true;
    }

    CALLBACK_GAL( KIGFX::GAL_DISPLAY_OPTIONS& aDisplayOptions,
                  std::function<void( const VECTOR2I& aPt1,
                                      const VECTOR2I& aPt2 )> aStrokeCallback,
                  std::function<void( const SHAPE_LINE_CHAIN& aPoly )> aOutlineCallback ) :
        GAL( aDisplayOptions )
    {
        m_strokeCallback = std::move( aStrokeCallback );
        m_triangleCallback = []( const VECTOR2I&, const VECTOR2I&, const VECTOR2I& ) {};
        m_outlineCallback = std::move( aOutlineCallback );
        m_stroke = true;
        m_triangulate = false;
    }

    CALLBACK_GAL( KIGFX::GAL_DISPLAY_OPTIONS& aDisplayOptions,
                  std::function<void( const SHAPE_LINE_CHAIN& aPoly )> aOutlineCallback ) :
        GAL( aDisplayOptions )
    {
        m_strokeCallback = []( const VECTOR2I& aPt1, const VECTOR2I& aPt2 ) {};
        m_triangleCallback = []( const VECTOR2I&, const VECTOR2I&, const VECTOR2I& ) {};
        m_outlineCallback = std::move( aOutlineCallback );
        m_stroke = false;
        m_triangulate = false;
    }

    /**
     * Draw a polygon representing an outline font glyph.
     */
    void DrawGlyph( const KIFONT::GLYPH& aGlyph, int aNth, int aTotal ) override;

private:
    std::function<void( const VECTOR2I& aPt1,
                        const VECTOR2I& aPt2 )> m_strokeCallback;

    std::function<void( const VECTOR2I& aPt1,
                        const VECTOR2I& aPt2,
                        const VECTOR2I& aPt3 )> m_triangleCallback;

    std::function<void( const SHAPE_LINE_CHAIN& aPoly )> m_outlineCallback;

    bool m_stroke;
    bool m_triangulate;
};


#endif // define CALLBACK_GAL_H
