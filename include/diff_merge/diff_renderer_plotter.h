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

#ifndef KICAD_DIFF_RENDERER_PLOTTER_H
#define KICAD_DIFF_RENDERER_PLOTTER_H

#include <kicommon.h>

#include <diff_merge/diff_scene.h>

#include <wx/string.h>


namespace KICAD_DIFF
{

/**
 * Options controlling the headless PNG/SVG renderer.
 */
struct PLOTTER_RENDER_OPTIONS
{
    int  dpi        = 300;
    int  pixelWidth = 0;   // 0 = auto from bbox + dpi
    int  pixelHeight = 0;
    bool antialias  = true;
    DIFF_COLOR_THEME theme;
};


/**
 * Render a DIFF_SCENE to a PNG file. Returns true on success.
 *
 * Each change is drawn as a filled colored rectangle around its bbox.
 * Categories render bottom-up so the most important (conflicts) end up
 * visually on top: modified -> added -> removed -> conflict.
 *
 * Future revisions can layer the actual board/schematic geometry beneath
 * the change overlay for a fuller render (matches the GAL renderer's
 * widget behavior); the API stays stable.
 */
bool RenderSceneToPng( const DIFF_SCENE&             aScene,
                                    const wxString&               aOutputPath,
                                    const PLOTTER_RENDER_OPTIONS& aOptions );


/**
 * Render a DIFF_SCENE to an SVG file. Returns true on success.
 */
bool RenderSceneToSvg( const DIFF_SCENE&             aScene,
                                    const wxString&               aOutputPath,
                                    const PLOTTER_RENDER_OPTIONS& aOptions );


/**
 * Minimum stroke width the headless plotter will use when a primitive
 * declares a non-positive line width.
 *
 * SVG treats stroke-width=0 as "no stroke" and the PNG path silently drops
 * sub-pixel widths, so for diff-overlay shapes we substitute a small
 * absolute internal-unit width. Pinned here so tests and any new plotter
 * surfaces share the same hairline definition.
 */
constexpr int PLOT_HAIRLINE_IU = 10;


/**
 * Return aWidth if positive, otherwise PLOT_HAIRLINE_IU. Pure helper used
 * by the plotter's polygon/segment/circle paths to keep them in lock-step
 * on the "what does width<=0 mean" rule.
 */
constexpr int EffectivePlotWidth( int aWidth )
{
    return aWidth > 0 ? aWidth : PLOT_HAIRLINE_IU;
}

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_RENDERER_PLOTTER_H
