/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GERBVIEW_DRAW_PANEL_GAL_H_
#define GERBVIEW_DRAW_PANEL_GAL_H_

#include <class_draw_panel_gal.h>

class DS_PROXY_VIEW_ITEM;


class GERBVIEW_DRAW_PANEL_GAL : public EDA_DRAW_PANEL_GAL
{
public:
    GERBVIEW_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                             const wxPoint& aPosition, const wxSize& aSize,
                             KIGFX::GAL_DISPLAY_OPTIONS& aOptions,
                             GAL_TYPE aGalType = GAL_TYPE_OPENGL );

    virtual ~GERBVIEW_DRAW_PANEL_GAL();

    ///< @copydoc EDA_DRAW_PANEL_GAL::SetHighContrastLayer()
    virtual void SetHighContrastLayer( int aLayer ) override;

    ///< @copydoc EDA_DRAW_PANEL_GAL::GetMsgPanelInfo()
    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    ///< @copydoc EDA_DRAW_PANEL_GAL::OnShow()
    void OnShow() override;

    bool SwitchBackend( GAL_TYPE aGalType ) override;

    ///< @copydoc EDA_DRAW_PANEL_GAL::SetTopLayer
    virtual void SetTopLayer( int aLayer ) override;

    ///< @copydoc EDA_DRAW_PANEL_GAL::GetDefaultViewBBox()
    BOX2I GetDefaultViewBBox() const override;

    /**
     * Set or update the drawing-sheet (borders and title block) used by the draw panel.
     *
     * @param aDrawingSheet is the drawing-sheet to be used.
     *        The object is then owned by GERBVIEW_DRAW_PANEL_GAL.
     */
    void SetDrawingSheet( DS_PROXY_VIEW_ITEM* aDrawingSheet );

    /**
     * @return the current drawing-sheet
     */
    DS_PROXY_VIEW_ITEM* GetDrawingSheet() const { return m_drawingSheet.get(); }

protected:
    ///< Set rendering targets & dependencies for layers.
    void setDefaultLayerDeps();

    ///< Currently used drawing-sheet (borders and title block)
    std::unique_ptr<DS_PROXY_VIEW_ITEM> m_drawingSheet;
};


#endif /* GERBVIEW_DRAW_PANEL_GAL_H_ */
