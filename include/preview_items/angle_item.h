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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PREVIEW_ANGLE_ITEM_H
#define PREVIEW_ANGLE_ITEM_H

#include <memory>

#include <preview_items/simple_overlay_item.h>

class EDIT_POINTS;

namespace KIGFX
{
namespace PREVIEW
{

class ANGLE_ITEM : public SIMPLE_OVERLAY_ITEM
{
public:
    ANGLE_ITEM( const std::shared_ptr<EDIT_POINTS>& aPoints );

    const BOX2I ViewBBox() const override;

    void SetEditPoints( const std::shared_ptr<EDIT_POINTS>& aPoints )
    {
        m_points = aPoints;
    }

private:
    void drawPreviewShape( KIGFX::VIEW* aView ) const override;

    std::weak_ptr<EDIT_POINTS> m_points;
};

} // PREVIEW
} // KIGFX

#endif
