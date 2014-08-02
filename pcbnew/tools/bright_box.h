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

#ifndef __BRIGHT_BOX_H
#define __BRIGHT_BOX_H

#include <math/box2.h>
#include <view/view.h>
#include <class_board_item.h>
#include <layers_id_colors_and_visibility.h>
#include <gal/color4d.h>

/**
 * Class BRIGHT_BOX
 *
 * Draws a decoration to indicate a brightened item.
 */
class BRIGHT_BOX : public EDA_ITEM
{
public:
    BRIGHT_BOX( BOARD_ITEM* aItem );
    ~BRIGHT_BOX() {};

    virtual const BOX2I ViewBBox() const
    {
        return m_item->ViewBBox();
    }

    void ViewDraw( int aLayer, KIGFX::GAL* aGal ) const;

    void ViewGetLayers( int aLayers[], int& aCount ) const
    {
        aLayers[0] = ITEM_GAL_LAYER( GP_OVERLAY );
        aCount = 1;
    }

    void Show( int x, std::ostream& st ) const
    {
    }

private:
    static const KIGFX::COLOR4D BOX_COLOR;
    static const double LINE_WIDTH;

    BOARD_ITEM* m_item;
};

#endif
