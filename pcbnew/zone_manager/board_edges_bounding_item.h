/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
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

#ifndef BOARD_EDGES_BOUNDING_ITEM_H
#define BOARD_EDGES_BOUNDING_ITEM_H

#include <view/view_item.h>
class BOARD_EDGES_BOUNDING_ITEM : public KIGFX::VIEW_ITEM
{
public:
    BOARD_EDGES_BOUNDING_ITEM( BOX2I aBox );
    ~BOARD_EDGES_BOUNDING_ITEM() override {};

    wxString GetClass() const override;

    const BOX2I ViewBBox() const override;

    std::vector<int> ViewGetLayers() const override;

private:
    BOX2I m_box;
};

#endif