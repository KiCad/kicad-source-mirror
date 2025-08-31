/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2024 The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PREVIEW_ANGLE_ITEM_H
#define PREVIEW_ANGLE_ITEM_H

#include <preview_items/simple_overlay_item.h>

class EDIT_POINTS;

namespace KIGFX
{
namespace PREVIEW
{

class ANGLE_ITEM : public SIMPLE_OVERLAY_ITEM
{
public:
    ANGLE_ITEM( EDIT_POINTS* aPoints );

    const BOX2I ViewBBox() const override;

    void SetEditPoints( EDIT_POINTS* aPoints )
    {
        m_points = aPoints;
    }

private:
    void drawPreviewShape( KIGFX::VIEW* aView ) const override;

    EDIT_POINTS* m_points;
};

} // PREVIEW
} // KIGFX

#endif
