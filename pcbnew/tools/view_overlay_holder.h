/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#pragma once

#include <memory>

namespace KIGFX
{
class VIEW;
class VIEW_OVERLAY;
}


/**
 * RAII owner of a single VIEW_OVERLAY.
 *
 * VIEW::MakeOverlay() already registers the overlay with the view, so it must be removed exactly
 * once and never Add()ed again.  Getting that wrong leaves a dangling R-tree entry that the next
 * repaint dereferences.  Centralising the make/remove pair here keeps the invariant in one place.
 * Derive from this to own a diagnostics or marker overlay.
 */
class VIEW_OVERLAY_HOLDER
{
public:
    VIEW_OVERLAY_HOLDER( KIGFX::VIEW* aView );
    virtual ~VIEW_OVERLAY_HOLDER();

    VIEW_OVERLAY_HOLDER( const VIEW_OVERLAY_HOLDER& ) = delete;
    VIEW_OVERLAY_HOLDER& operator=( const VIEW_OVERLAY_HOLDER& ) = delete;

protected:
    KIGFX::VIEW*                         m_view;
    std::shared_ptr<KIGFX::VIEW_OVERLAY> m_overlay;
};
