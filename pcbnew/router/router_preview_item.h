/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
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
    enum ItemType
    {
        PR_VIA,
        PR_LINE,
        PR_STUCK_MARKER
    };

    enum ItemFlags
    {
        PR_SUGGESTION = 1
    };

    ROUTER_PREVIEW_ITEM( const PNS_ITEM* aItem = NULL, KIGFX::VIEW_GROUP* aParent = NULL );
    ~ROUTER_PREVIEW_ITEM();

    void Update( const PNS_ITEM* aItem );

    void StuckMarker( VECTOR2I& aPosition );
    void DebugLine( const SHAPE_LINE_CHAIN& aLine, int aWidth = 0, int aStyle = 0 );
    void DebugBox( const BOX2I& aBox, int aStyle = 0 );

    void Show( int a, std::ostream& b ) const {};

    const BOX2I ViewBBox() const;

    virtual void ViewDraw( int aLayer, KIGFX::GAL* aGal ) const;

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const
    {
        aLayers[0] = m_layer;
        aCount = 1;
    }

    void MarkAsHead();

private:
    const KIGFX::COLOR4D assignColor( int aStyle ) const;
    const KIGFX::COLOR4D getLayerColor( int aLayer ) const;

    KIGFX::VIEW_GROUP* m_parent;

    PNS_ROUTER* m_router;
    SHAPE_LINE_CHAIN m_line;

    ItemType m_type;
    int m_style;
    int m_width;
    int m_layer;

    KIGFX::COLOR4D m_color;

    VECTOR2I m_stuckPosition;
    VECTOR2I m_viaCenter;
};

#endif
