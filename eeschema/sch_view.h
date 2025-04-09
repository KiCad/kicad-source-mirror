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

#ifndef SCH_VIEW_H_
#define SCH_VIEW_H_

#include <layer_ids.h>
#include <math/vector2d.h>
#include <view/view.h>

#include <memory>
#include <vector>

class SCH_SHEET;
class SCH_SCREEN;
class LIB_SYMBOL;
class SCH_PIN;
class SCH_BASE_FRAME;
class DS_PROXY_VIEW_ITEM;


// Eeschema 100nm as the internal units
constexpr double SCH_WORLD_UNIT ( 1e-7 / 0.0254 );

static const int SCH_LAYER_ORDER[] = { LAYER_GP_OVERLAY,
                                       LAYER_SELECT_OVERLAY,
                                       LAYER_ERC_ERR,
                                       LAYER_ERC_WARN,
                                       LAYER_ERC_EXCLUSION,
                                       LAYER_DANGLING,
                                       LAYER_OP_VOLTAGES,
                                       LAYER_OP_CURRENTS,
                                       LAYER_REFERENCEPART,
                                       LAYER_VALUEPART,
                                       LAYER_FIELDS,
                                       LAYER_PINNUM,
                                       LAYER_PINNAM,
                                       LAYER_INTERSHEET_REFS,
                                       LAYER_NETCLASS_REFS,
                                       LAYER_RULE_AREAS,
                                       LAYER_BUS_JUNCTION,
                                       LAYER_JUNCTION,
                                       LAYER_NOCONNECT,
                                       LAYER_HIERLABEL,
                                       LAYER_GLOBLABEL,
                                       LAYER_LOCLABEL,
                                       LAYER_SHEETFILENAME,
                                       LAYER_SHEETNAME,
                                       LAYER_SHEETLABEL,
                                       LAYER_SHEETFIELDS,
                                       LAYER_NOTES,
                                       LAYER_PRIVATE_NOTES,
                                       LAYER_WIRE,
                                       LAYER_BUS,
                                       LAYER_DEVICE,
                                       LAYER_SHEET,
                                       LAYER_SELECTION_SHADOWS,
                                       LAYER_DRAW_BITMAPS,
                                       LAYER_SHAPES_BACKGROUND,
                                       LAYER_DEVICE_BACKGROUND,
                                       LAYER_SHEET_BACKGROUND,
                                       LAYER_NOTES_BACKGROUND,
                                       LAYER_DRAWINGSHEET };


namespace KIGFX
{
    class VIEW_GROUP;

    namespace PREVIEW
    {
        class SELECTION_AREA;
    };

class SCH_VIEW : public KIGFX::VIEW
{
public:
    // Note: aFrame is used to know the sheet path name when drawing the drawing sheet.
    // It can be null.
    SCH_VIEW( SCH_BASE_FRAME* aFrame );
    ~SCH_VIEW();

    void Update( const KIGFX::VIEW_ITEM* aItem, int aUpdateFlags ) const override;
    void Update( const KIGFX::VIEW_ITEM* aItem ) const override;

    void Cleanup();

    void DisplaySheet( const SCH_SCREEN* aScreen );
    void DisplaySymbol( LIB_SYMBOL* aSymbol );

    void SetScale( double aScale, VECTOR2D aAnchor = { 0, 0 } ) override;

    /**
     * Clear the hide flag of all items in the view
     */
    void ClearHiddenFlags();

    void HideDrawingSheet();

    DS_PROXY_VIEW_ITEM* GetDrawingSheet() const { return m_drawingSheet.get(); }

private:
    SCH_BASE_FRAME* m_frame;    // The frame using this view. Can be null. Used mainly
                                // to know the sheet path name when drawing the drawing sheet

    std::unique_ptr<DS_PROXY_VIEW_ITEM> m_drawingSheet;
};

}; // namespace

#endif
