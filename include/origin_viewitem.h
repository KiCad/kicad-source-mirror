/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#pragma once

#include <math/box2.h>
#include <view/view.h>
#include <layer_ids.h>
#include <gal/color4d.h>
#include <eda_item.h>

/**
 * View item to draw an origin marker.
 */
namespace KIGFX {

class ORIGIN_VIEWITEM : public EDA_ITEM
{
public:
    /// Marker symbol styles.
    enum MARKER_STYLE
    {
        NO_GRAPHIC, CROSS, CIRCLE_CROSS, CIRCLE_X
    };

    ORIGIN_VIEWITEM( const COLOR4D& aColor = COLOR4D( 1.0, 1.0, 1.0, 1.0 ),
                     MARKER_STYLE aStyle = CIRCLE_X, int aSize = 16,
                     const VECTOR2D& aPosition = VECTOR2D( 0, 0 ) );

    ORIGIN_VIEWITEM( const VECTOR2D& aPosition, EDA_ITEM_FLAGS flags );

    ORIGIN_VIEWITEM* Clone() const override;

    const BOX2I ViewBBox() const override;

    void ViewDraw( int aLayer, VIEW* aView ) const override;

    std::vector<int> ViewGetLayers() const override
    {
        return { LAYER_GP_OVERLAY };
    }

#if defined(DEBUG)
    void Show( int x, std::ostream& st ) const override
    {
    }
#endif

    /**
     * Get class name.
     *
     * @return string "ORIGIN_VIEWITEM"
     */
    wxString GetClass() const override
    {
        return wxT( "ORIGIN_VIEWITEM" );
    }

    /**
     * Set the draw at zero flag.
     *
     * When set the marker will be drawn when its position is 0,0.  Otherwise it will not
     * be drawn when its position is 0,0.
     *
     * @param aDrawFlag The value to set the draw at zero flag.
     */
    inline void SetDrawAtZero( bool aDrawFlag )
    {
        m_drawAtZero = aDrawFlag;
    }

    void SetPosition( const VECTOR2I& aPosition ) override
    {
        m_position = VECTOR2D( aPosition );
    }

    VECTOR2I GetPosition() const override
    {
        return VECTOR2I( m_position.x, m_position.y );
    }

    inline void SetSize( int aSize )
    {
        m_size = aSize;
    }

    inline int GetSize() const
    {
        return m_size;
    }

    inline void SetColor( const KIGFX::COLOR4D& aColor )
    {
        m_color = aColor;
    }

    inline void SetStyle( MARKER_STYLE aStyle )
    {
        m_style = aStyle;
    }

protected:
    VECTOR2D        m_position;
    int             m_size;           /// (in pixels)
    COLOR4D         m_color;
    MARKER_STYLE    m_style;
    bool            m_drawAtZero;     /// If set, the marker will be drawn even if its position is 0,0.
};

} // namespace KIGFX
