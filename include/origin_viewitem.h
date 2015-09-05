/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#ifndef __ORIGIN_VIEWITEM_H
#define __ORIGIN_VIEWITEM_H

#include <math/box2.h>
#include <view/view.h>
#include <class_board_item.h>
#include <layers_id_colors_and_visibility.h>
#include <gal/color4d.h>

/**
 * Class ORIGIN_VIEWITEM
 *
 * View item to draw an origin marker.
 */
namespace KIGFX {

class ORIGIN_VIEWITEM : public EDA_ITEM
{
public:
    ///> Marker symbol styles
    enum MARKER_STYLE { NONE, CROSS, X, DOT, CIRCLE_CROSS, CIRCLE_X, CIRCLE_DOT };

    ORIGIN_VIEWITEM( const COLOR4D& aColor = COLOR4D( 1.0, 1.0, 1.0, 1.0 ),
                     MARKER_STYLE aStyle = CIRCLE_X, int aSize = 16,
                     const VECTOR2D& aPosition = VECTOR2D( 0, 0 ) );

    const BOX2I ViewBBox() const;

    void ViewDraw( int aLayer, KIGFX::GAL* aGal ) const;

    void ViewGetLayers( int aLayers[], int& aCount ) const
    {
        aLayers[0] = ITEM_GAL_LAYER( GP_OVERLAY );
        aCount = 1;
    }

#if defined(DEBUG)
    void Show( int x, std::ostream& st ) const
    {
    }
#endif

    /** Get class name
     * @return  string "ORIGIN_VIEWITEM"
     */
    wxString GetClass() const
    {
        return wxT( "ORIGIN_VIEWITEM" );
    }

    /**
     * Function SetDrawAtZero()
     * Set the draw at zero flag. When set the marker will be drawn when it's position is 0,0.
     * Otherwise it will not be drawn when its position is 0,0
     * @param aDrawFlag The value to set the draw at zero flag
     */
    inline void SetDrawAtZero( bool aDrawFlag )
    {
        m_drawAtZero = aDrawFlag;
    }

    inline void SetPosition( const VECTOR2D& aPosition )
    {
        m_position = aPosition;
    }

    inline const VECTOR2D& GetPosition() const
    {
        return m_position;
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

    inline const KIGFX::COLOR4D& GetColor() const
    {
        return m_color;
    }

    inline void SetStyle( MARKER_STYLE aStyle )
    {
        m_style = aStyle;
    }

    inline MARKER_STYLE GetStyle() const
    {
        return m_style;
    }

protected:
    ///> Marker coordinates.
    VECTOR2D        m_position;

    ///> Marker size (in pixels).
    int             m_size;

    ///> Marker color.
    COLOR4D         m_color;

    ///> Marker symbol.
    MARKER_STYLE    m_style;

    ///> If set, the marker will be drawn even if its position is 0,0
    bool            m_drawAtZero;
};

} // namespace KIGFX

#endif
