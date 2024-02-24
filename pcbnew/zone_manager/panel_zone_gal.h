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


#ifndef PANEL_ZONE_GAL_H
#define PANEL_ZONE_GAL_H

#include <board_item.h>
#include <wx/gdicmn.h>
#include <zone.h>
#include <memory>
#include <wx/string.h>
#include "board_edges_bounding_item.h"
#include "pcb_draw_panel_gal.h"
#include "zone_manager/zone_selection_change_notifier.h"

class PANEL_ZONE_GAL : public PCB_DRAW_PANEL_GAL, public ZONE_SELECTION_CHANGE_NOTIFIER
{
public:
    PANEL_ZONE_GAL( BOARD* aPcb, wxWindow* aParentWindow, KIGFX::GAL_DISPLAY_OPTIONS& aOptions,
                    wxWindowID aWindowId = 0, const wxPoint& aPosition = wxDefaultPosition,
                    const wxSize& aSize = wxDefaultSize, GAL_TYPE aGalType = GAL_TYPE_OPENGL );
    ~PANEL_ZONE_GAL() override;


    const wxSize GetPageSizeIU() const;

    void  ZoomFitScreen();
    BOX2I GetBoardBoundingBox( bool aBoardEdgesOnly ) const;

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const;

    /**
     * @brief Show the zone painting on layer
     *
     * @param aLayer The expected layer
     * @return return true if the GAL updated the display else falue
     */
    bool OnLayerSelected( int aLayer );

    int GetLayer() const { return m_layer; }

    void ActivateSelectedZone( ZONE* aZone ) override;

    ZONE* GetZone() const { return m_zone; }

private:
    BOARD*                                     m_pcb;
    int                                        m_layer;
    std::unique_ptr<BOARD_EDGES_BOUNDING_ITEM> m_pcb_bounding_box;
    ZONE*                                      m_zone;
};

#endif