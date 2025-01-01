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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <cstddef>
#include <map>
#include <span>
#include <vector>

#include <origin_viewitem.h>

#include <math/vector2d.h>
#include <geometry/seg.h> // OPT_VECTOR2I

namespace KIGFX
{

/**
 * View item to draw debug items for anchors.
 *
 * This can be handy when verifying how many/where anchors are being placed.
 */
class ANCHOR_DEBUG : public EDA_ITEM
{
public:
    ANCHOR_DEBUG();

    ANCHOR_DEBUG* Clone() const override;

    std::vector<int> ViewGetLayers() const override;

    const BOX2I ViewBBox() const override;

    void ViewDraw( int aLayer, VIEW* aView ) const override;

    /**
     * Get class name.
     *
     * @return string "ANCHOR_DEBUG"
     */
    wxString GetClass() const override { return wxT( "ANCHOR_DEBUG" ); }

    void ClearAnchors();

    /**
     * Add an anchor at the given position.
     */
    void AddAnchor( const VECTOR2I& aAnchor );

    /**
     * Set the nearest anchor to the given position.
     *
     * Pass an empty optional to clear the nearest anchor.
     */
    void SetNearest( const OPT_VECTOR2I& aNearest );

private:
    // Store the anchors by location and count
    std::map<VECTOR2I, size_t> m_anchors;
    OPT_VECTOR2I               m_nearest;
};

} // namespace KIGFX
