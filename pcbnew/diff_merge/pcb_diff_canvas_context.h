/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 */

#ifndef PCB_DIFF_CANVAS_CONTEXT_H
#define PCB_DIFF_CANVAS_CONTEXT_H

#include <diff_merge/diff_scene.h>
#include <gal/color4d.h>
#include <kiid.h>
#include <pcb_painter.h>

#include <map>
#include <memory>
#include <vector>


class BOARD;
class FOOTPRINT;
class WIDGET_DIFF_CANVAS;

namespace KIGFX
{
class GAL;
class PAINTER;
class VIEW_ITEM;
}


namespace KICAD_DIFF
{

std::vector<KIGFX::VIEW_ITEM*> CollectBoardDiffContextItems( BOARD& aBoard );
std::vector<KIGFX::VIEW_ITEM*> CollectFootprintDiffContextItems( FOOTPRINT& aFootprint );

void ConfigurePcbDiffContextRenderSettings( KIGFX::PCB_RENDER_SETTINGS& aSettings,
                                            const KIGFX::COLOR4D& aColor );

std::unique_ptr<KIGFX::PAINTER> MakePcbDiffContextPainter( KIGFX::GAL* aGal, const KIGFX::COLOR4D& aColor,
                                                           std::map<KIID, KIGFX::COLOR4D> aOverrides = {} );

void ConfigurePcbDiffCanvasContext( WIDGET_DIFF_CANVAS& aCanvas, BOARD* aReference, BOARD* aComparison,
                                    const KIGFX::COLOR4D& aColor, const std::map<KIID, KIGFX::COLOR4D>& aOverrides = {},
                                    const std::vector<KIGFX::VIEW_ITEM*>&       aExtraItems = {},
                                    const std::map<KIID, KICAD_DIFF::CATEGORY>& aCategories = {} );

} // namespace KICAD_DIFF

#endif // PCB_DIFF_CANVAS_CONTEXT_H
