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

#ifndef SCH_DIFF_CANVAS_CONTEXT_H
#define SCH_DIFF_CANVAS_CONTEXT_H

#include <diff_merge/diff_scene.h>
#include <gal/color4d.h>
#include <kiid.h>
#include <sch_render_settings.h>

#include <map>
#include <memory>
#include <set>
#include <vector>


class SCHEMATIC;
class SCH_SCREEN;
class WIDGET_DIFF_CANVAS;

namespace KIGFX
{
class GAL;
class PAINTER;
class VIEW_ITEM;
}


namespace KICAD_DIFF
{

std::vector<KIGFX::VIEW_ITEM*> CollectSchematicDiffContextItems( SCHEMATIC& aSchematic, SCH_SCREEN* aScreen = nullptr );

void ConfigureSchDiffContextRenderSettings( SCH_RENDER_SETTINGS& aSettings,
                                            const KIGFX::COLOR4D& aColor );

std::unique_ptr<KIGFX::PAINTER> MakeSchDiffContextPainter( KIGFX::GAL* aGal, SCHEMATIC* aSchematic,
                                                           const KIGFX::COLOR4D&          aColor,
                                                           std::map<KIID, KIGFX::COLOR4D> aOverrides = {} );

void ConfigureSchDiffCanvasContext( WIDGET_DIFF_CANVAS& aCanvas, SCHEMATIC* aReference, SCHEMATIC* aComparison,
                                    const KIGFX::COLOR4D& aColor, const std::map<KIID, KIGFX::COLOR4D>& aOverrides = {},
                                    const std::vector<KIGFX::VIEW_ITEM*>&       aExtraItems = {},
                                    const std::map<KIID, KICAD_DIFF::CATEGORY>& aCategories = {},
                                    SCH_SCREEN* aReferenceScreen = nullptr, SCH_SCREEN* aComparisonScreen = nullptr );

} // namespace KICAD_DIFF

#endif // SCH_DIFF_CANVAS_CONTEXT_H
