/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <layer_ids.h>
#include <view/view.h>
#include <board_item.h>

class PCB_DISPLAY_OPTIONS;

namespace KIGFX {

class PCB_VIEW : public VIEW
{
public:
    PCB_VIEW();
    virtual ~PCB_VIEW();

    /// @copydoc VIEW::Add()
    virtual void Add( VIEW_ITEM* aItem, int aDrawPriority = -1 ) override;

    /// @copydoc VIEW::Remove()
    virtual void Remove( VIEW_ITEM* aItem ) override;

    /// @copydoc VIEW::Update()
    virtual void Update( const VIEW_ITEM* aItem, int aUpdateFlags ) const override;

    /// @copydoc VIEW::Update()
    virtual void Update( const VIEW_ITEM* aItem ) const override;

    /**
     * Sets the KIGFX::REPAINT on all items matching \a aTypes which intersect \a aStaleAreas.
     */
    void UpdateCollidingItems( const std::vector<BOX2I>& aStaleAreas,
                               std::initializer_list<KICAD_T> aTypes );

    void UpdateDisplayOptions( const PCB_DISPLAY_OPTIONS& aOptions );
};

}
