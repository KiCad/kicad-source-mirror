/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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


#ifndef GERBER_LAYER_WIDGET_H
#define GERBER_LAYER_WIDGET_H

#include "layer_widget.h"

/**
 * Abstract functions of LAYER_WIDGET so they may be tied into the GERBVIEW_FRAME's data and
 * we can add a popup menu which is specific to Pcbnew's needs.
 */
class GERBER_LAYER_WIDGET : public LAYER_WIDGET
{
public:
    /**
     * @param aParent is the parent frame.
     * @param aFocusOwner is the window that has the keyboard focus.
     */
    GERBER_LAYER_WIDGET( GERBVIEW_FRAME* aParent, wxWindow* aFocusOwner );

    /**
     * Collect the current color settings and put it in aColorSettings
     */
    void CollectCurrentColorSettings(  COLOR_SETTINGS* aColorSettings );

    /**
     * Rebuild Render for instance after the config is read.
     */
    void ReFill();

    /**
     * Rebuild Render for instance after the config is read.
     */
    void ReFillRender();

    //-----<implement LAYER_WIDGET abstract callback functions>-----------
    void OnLayerRightClick( wxMenu& aMenu ) override;
    void OnLayerColorChange( int aLayer, const COLOR4D& aColor ) override;
    bool OnLayerSelect( int aLayer ) override;
    void OnLayerVisible( int aLayer, bool isVisible, bool isFinal ) override;
    void OnRenderColorChange( int aId, const COLOR4D& aColor ) override;
    void OnRenderEnable( int aId, bool isEnabled ) override;

    /**
     * Update the layer manager tabs labels.
     *
     * Useful when changing Language or to set labels to a non default value.
     */
    void SetLayersManagerTabsText( );
    //-----</implement LAYER_WIDGET abstract callback functions>----------

    /**
     * Ensure the active layer is visible, and other layers not visible when
     * m_alwaysShowActiveLayer is true.
     *
     * @return true m_alwaysShowActiveLayer is true and the canvas is refreshed else false
     *         if do nothing.
     */
    bool OnLayerSelected();     // postprocess after an active layer selection
                                // ensure active layer visible if
                                // m_alwaysShowActiveCopperLayer is true;

    /**
     * Add menu items to a menu that should be shown when right-clicking the Gerber layer widget.
     *
     * @param aMenu is the menu to modify: menuitems will be added to aMenu.
     */
    void AddRightClickMenuItems( wxMenu* aMenu );

protected:
    // popup menu ids. in layer manager
    enum LAYER_MANAGER
    {
        ID_LAYER_MANAGER_START = LAYER_WIDGET::ID_LAST_VALUE,
        ID_SHOW_ALL_LAYERS = ID_LAYER_MANAGER_START,
        ID_SHOW_NO_LAYERS,
        ID_SHOW_NO_LAYERS_BUT_ACTIVE,
        ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE,
        ID_SORT_GBR_LAYERS_X2,
        ID_SORT_GBR_LAYERS_FILE_EXT,
        ID_SET_GBR_LAYERS_DRAW_PRMS,
        ID_LAYER_MOVE_UP,
        ID_LAYER_MOVE_DOWN,
        ID_LAYER_DELETE,
        ID_LAYER_MANAGER_END = ID_LAYER_DELETE,
    };

private:
    /**
     * Put up a popup menu for the layer panel.
     */
    void onRightDownLayers( wxMouseEvent& event );

    void onPopupSelection( wxCommandEvent& event );

    /// this is for the popup menu, the right click handler has to be installed
    /// on every child control within the layer panel.
    void installRightLayerClickHandler();

    GERBER_FILE_IMAGE_LIST* GetImagesList();

    GERBVIEW_FRAME* m_frame;

    bool m_alwaysShowActiveLayer;   // If true: Only shows the current active layer
                                    // even if it is changed
};

#endif  // GERBER_LAYER_WIDGET_H
