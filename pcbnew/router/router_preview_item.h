/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ROUTER_PREVIEW_ITEM_H
#define __ROUTER_PREVIEW_ITEM_H

#include <cstdio>

#include <view/view.h>
#include <view/view_item.h>
#include <view/view_group.h>

#include <math/vector2d.h>
#include <math/box2.h>

#include <geometry/shape_line_chain.h>
#include <geometry/shape_circle.h>

#include <gal/color4d.h>
#include <gal/graphics_abstraction_layer.h>

#include <layers_id_colors_and_visibility.h>

class PNS_ITEM;
class PNS_ROUTER;

class ROUTER_PREVIEW_ITEM : public EDA_ITEM
{
public:
    enum ITEM_TYPE
    {
        PR_STUCK_MARKER = 0,
        PR_POINT,
        PR_SHAPE
    };

    ROUTER_PREVIEW_ITEM( const PNS_ITEM* aItem = NULL, KIGFX::VIEW_GROUP* aParent = NULL );
    ~ROUTER_PREVIEW_ITEM();

    void Update( const PNS_ITEM* aItem );

    void StuckMarker( VECTOR2I& aPosition );

    void Line( const SHAPE_LINE_CHAIN& aLine, int aWidth = 0, int aStyle = 0 );
    void Box( const BOX2I& aBox, int aStyle = 0 );
    void Point ( const VECTOR2I& aPos, int aStyle = 0);

    void SetColor( const KIGFX::COLOR4D& aColor )
    {
        m_color = aColor;
    }

    void SetClearance( int aClearance )
    {
        m_clearance = aClearance;
    }

#if defined(DEBUG)
    void Show( int aA, std::ostream& aB ) const {};
#endif

    /** Get class name
     * @return  string "ROUTER_PREVIEW_ITEM"
     */
    virtual wxString GetClass() const
    {
        return wxT( "ROUTER_PREVIEW_ITEM" );
    }

    const BOX2I ViewBBox() const;

    virtual void ViewDraw( int aLayer, KIGFX::GAL* aGal ) const;

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const
    {
        aLayers[0] = m_layer;
        aCount = 1;
    }

    void drawLineChain( const SHAPE_LINE_CHAIN& aL, KIGFX::GAL* aGal ) const;

private:
    const KIGFX::COLOR4D assignColor( int aStyle ) const;
    const KIGFX::COLOR4D getLayerColor( int aLayer ) const;

    KIGFX::VIEW_GROUP* m_parent;

    PNS_ROUTER* m_router;
    SHAPE* m_shape;

    ITEM_TYPE m_type;

    int m_style;
    int m_width;
    int m_layer;
    int m_originLayer;
    int m_clearance;

    // fixme: shouldn't this go to VIEW?
    static const int ClearanceOverlayDepth;
    static const int BaseOverlayDepth;
    static const int ViaOverlayDepth;

    double m_depth;

    KIGFX::COLOR4D m_color;
    VECTOR2I m_pos;
};

#endif
