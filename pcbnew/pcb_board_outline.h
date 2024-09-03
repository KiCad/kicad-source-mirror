/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCB_BOARD_OUTLINE_H
#define PCB_BOARD_OUTLINE_H

#include <geometry/shape_poly_set.h>
#include <eda_item.h>
#include <board_item.h>
#include <memory>


class PCB_BOARD_OUTLINE : public BOARD_ITEM
{
public:
    PCB_BOARD_OUTLINE( BOARD_ITEM* aParent );

    PCB_BOARD_OUTLINE( const PCB_BOARD_OUTLINE& aOther );

    PCB_BOARD_OUTLINE& operator=( const PCB_BOARD_OUTLINE& aOther );

    ~PCB_BOARD_OUTLINE() override;

    const SHAPE_POLY_SET& GetOutline() const { return m_outlines; }

    SHAPE_POLY_SET& GetOutline() { return m_outlines; }

    std::vector<int> ViewGetLayers() const override;

    const BOX2I GetBoundingBox() const override;

    PCB_LAYER_ID GetLayer() const override;

    LSET GetLayerSet() const override;

    bool IsOnLayer( PCB_LAYER_ID aLayer ) const override;

    double Similarity( const BOARD_ITEM& aItem ) const override;

    bool operator==( const BOARD_ITEM& aItem ) const override;

    EDA_ITEM* Clone() const override;

    bool HasOutline() const { return !m_outlines.IsEmpty(); }

    static void CopyPros( const PCB_BOARD_OUTLINE& aCopyFrom, PCB_BOARD_OUTLINE& aCopyTo );

#if defined( DEBUG )

    void Show( int nestLevel, std::ostream& os ) const override;

#endif

    wxString GetClass() const override;

private:
    SHAPE_POLY_SET m_outlines;
};

#endif
