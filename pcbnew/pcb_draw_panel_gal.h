/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef PCB_DRAW_PANEL_GAL_H_
#define PCB_DRAW_PANEL_GAL_H_

#include <class_draw_panel_gal.h>
#include <layer_ids.h>
#include <pcb_view.h>

class DS_PROXY_VIEW_ITEM;
class RATSNEST_VIEW_ITEM;
class PROGRESS_REPORTER;

class PCB_DRAW_PANEL_GAL : public EDA_DRAW_PANEL_GAL
{
public:
    PCB_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId, const wxPoint& aPosition,
                        const wxSize& aSize, KIGFX::GAL_DISPLAY_OPTIONS& aOptions,
                        GAL_TYPE aGalType = GAL_TYPE_OPENGL );

    virtual ~PCB_DRAW_PANEL_GAL();

    /**
     * Add all items from the current board to the VIEW, so they can be displayed by GAL.
     *
     * @param aBoard is the PCB to be loaded.
     */
    void DisplayBoard( BOARD* aBoard, PROGRESS_REPORTER* aReporter = nullptr );

    /**
     * Sets (or updates) drawing-sheet used by the draw panel.
     *
     * @param aDrawingSheet is the drawing-sheet to be used.  The object is then owned by
     *                      #PCB_DRAW_PANEL_GAL.
     */
    void SetDrawingSheet( DS_PROXY_VIEW_ITEM* aDrawingSheet );

    DS_PROXY_VIEW_ITEM* GetDrawingSheet() const { return m_drawingSheet.get(); }

    // TODO(JE) Look at optimizing this out
    /**
     * Update the color settings in the painter and GAL.
     */
    void UpdateColors();

    ///< @copydoc EDA_DRAW_PANEL_GAL::SetHighContrastLayer()
    virtual void SetHighContrastLayer( int aLayer ) override
    {
        SetHighContrastLayer( static_cast< PCB_LAYER_ID >( aLayer ) );
    }

    ///< SetHighContrastLayer(), with some extra smarts for PCB.
    void SetHighContrastLayer( PCB_LAYER_ID aLayer );

    ///< @copydoc EDA_DRAW_PANEL_GAL::SetTopLayer()
    virtual void SetTopLayer( int aLayer ) override
    {
        SetTopLayer( static_cast< PCB_LAYER_ID >( aLayer ) );
    }

    ///< SetTopLayer(), with some extra smarts for PCB.
    void SetTopLayer( PCB_LAYER_ID aLayer );

    /**
     * Update "visibility" property of each layer of a given #BOARD.
     *
     * @param aBoard contains layers visibility settings to be applied.
     */
    void SyncLayersVisibility( const BOARD* aBoard );

    ///< @copydoc EDA_DRAW_PANEL_GAL::GetMsgPanelInfo()
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    ///< @copydoc EDA_DRAW_PANEL_GAL::OnShow()
    void OnShow() override;

    bool SwitchBackend( GAL_TYPE aGalType ) override;

    ///< Force refresh of the ratsnest visual representation.
    void RedrawRatsnest();

    ///< @copydoc EDA_DRAW_PANEL_GAL::GetDefaultViewBBox()
    BOX2I GetDefaultViewBBox() const override;

    virtual KIGFX::PCB_VIEW* GetView() const override;

protected:
    ///< Reassign layer order to the initial settings.
    void setDefaultLayerOrder();

    ///< Set rendering targets & dependencies for layers.
    void setDefaultLayerDeps();

protected:
    std::unique_ptr<DS_PROXY_VIEW_ITEM> m_drawingSheet;  ///< Currently used drawing-sheet.
    std::unique_ptr<RATSNEST_VIEW_ITEM> m_ratsnest;      ///< Ratsnest view item
};

#endif /* PCB_DRAW_PANEL_GAL_H_ */
