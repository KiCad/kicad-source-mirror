/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KICAD_DIFF_RENDERER_GAL_H
#define KICAD_DIFF_RENDERER_GAL_H

#include <diff_merge/diff_scene.h>

#include <array>
#include <optional>
#include <set>


namespace KIGFX
{
class VIEW_OVERLAY;
}


namespace KICAD_DIFF
{

/**
 * Default-visible category filter — every CATEGORY shows. Use as the default
 * argument to RenderSceneToOverlay.
 */
inline constexpr std::array<bool, CATEGORY_COUNT> ALL_CATEGORIES_VISIBLE{ {
        true, true, true, true } };


/**
 * Push a DIFF_SCENE's shapes onto a VIEW_OVERLAY as filled, semi-transparent
 * rectangles, drawn in the canonical PAINT_ORDER so conflicts visually win
 * over earlier categories. The overlay is cleared first; existing commands
 * are discarded.
 *
 * Caller is responsible for adding the overlay to its KIGFX::VIEW and for
 * invalidating the view after rendering. The function does not interact
 * with the VIEW; it only mutates the overlay.
 *
 * When aHighlight is set, a second pass strokes the matching shapes' bbox
 * with a one-pixel outline on top of the fills so the selection stays
 * visible even when its category is painted under another.
 *
 * Shapes whose changeId is in aHidden render muted grey instead of their
 * category color.
 */
void RenderSceneToOverlay( KIGFX::VIEW_OVERLAY& aOverlay, const DIFF_SCENE& aScene,
                           const std::array<bool, CATEGORY_COUNT>& aVisible = ALL_CATEGORIES_VISIBLE,
                           const std::optional<KIID_PATH>&         aHighlight = std::nullopt,
                           const std::set<KIID_PATH>&              aHidden = {} );

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_RENDERER_GAL_H
