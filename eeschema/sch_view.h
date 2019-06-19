/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>
#include <view/view.h>
#include <math/box2.h>

#include <view/wx_view_controls.h>
#include <worksheet_viewitem.h>
#include <layers_id_colors_and_visibility.h>

class SCH_SHEET;
class SCH_SCREEN;
class LIB_PART;
class LIB_PIN;
class SCH_BASE_FRAME;

// Eeschema uses mils as the internal units
constexpr double SCH_WORLD_UNIT = 0.001;

static const LAYER_NUM SCH_LAYER_ORDER[] =
    {
        LAYER_GP_OVERLAY, LAYER_SELECT_OVERLAY,
        LAYER_ERC_ERR, LAYER_ERC_WARN,
        LAYER_REFERENCEPART, LAYER_VALUEPART, LAYER_FIELDS,
        LAYER_NOCONNECT,
        LAYER_JUNCTION,
        LAYER_HIERLABEL,
        LAYER_WIRE,
        LAYER_BUS,
        LAYER_NOTES,
        LAYER_DEVICE,
        LAYER_DEVICE_BACKGROUND,
        LAYER_SHEET,
        LAYER_SHEET_BACKGROUND,
        LAYER_SCHEMATIC_BITMAPS,
        LAYER_WORKSHEET
    };


namespace KIGFX
{
    class VIEW_GROUP;
    class WORKSHEET_VIEWITEM;

    namespace PREVIEW
    {
        class SELECTION_AREA;
    };

class SCH_VIEW : public KIGFX::VIEW
{
public:
    // Note: aFrame is used to know the sheet path name when drawing the page layout.
    // It can be null.
    SCH_VIEW( bool aIsDynamic, SCH_BASE_FRAME* aFrame );
    ~SCH_VIEW();

    void DisplaySheet( SCH_SHEET* aSheet );
    void DisplaySheet( SCH_SCREEN* aScreen );
    void DisplayComponent( LIB_PART* aPart );

    // Call it to set new draw area limits (max working and draw area size)
    void ResizeSheetWorkingArea( SCH_SCREEN *aScreen );

    KIGFX::PREVIEW::SELECTION_AREA* GetSelectionArea() const { return m_selectionArea.get(); }

    KIGFX::VIEW_GROUP* GetPreview() const { return m_preview.get(); }

    void ClearPreview();
    void AddToPreview( EDA_ITEM* aItem, bool aTakeOwnership = true );

    void ShowSelectionArea( bool aShow = true );
    void ShowPreview( bool aShow = true );

    /**
     * Clear the hide flag of all items in the view
     */
    void ClearHiddenFlags();

    void HideWorksheet();

    void HighlightItem( EDA_ITEM *aItem, LIB_PIN* aPin = nullptr );

private:
    SCH_BASE_FRAME* m_frame;    // The frame using this view. Can be null. Used mainly
                                // to know the sheet path name when drawing the page layout
    std::unique_ptr<WORKSHEET_VIEWITEM> m_worksheet;
    std::unique_ptr<KIGFX::PREVIEW::SELECTION_AREA> m_selectionArea;
    std::unique_ptr<KIGFX::VIEW_GROUP> m_preview;
    std::vector<EDA_ITEM *> m_ownedItems;
};

}; // namespace

#endif
