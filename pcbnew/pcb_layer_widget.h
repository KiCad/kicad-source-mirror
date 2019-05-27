/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010-2018 KiCad Developers, see cAUTHORS.txt for contributors.
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


/************************************************************/
/* pcb_layer_widget.h : header for the layers manager */
/************************************************************/

#ifndef PCB_LAYER_WIDGET_H
#define PCB_LAYER_WIDGET_H

#include <layer_widget.h>

/**
 * Class PCB_LAYER_WIDGET
 * is here to implement the abstract functions of LAYER_WIDGET so they
 * may be tied into the PCB_EDIT_FRAME's data and so we can add a popup
 * menu which is specific to PCBNEW's needs.
 */
class PCB_LAYER_WIDGET : public LAYER_WIDGET
{
public:

    /**
     * Constructor
     * @param aParent is the parent window
     * @param aFocusOwner is the window that should be sent the focus after
     * @param aFpEditorMode false for the board editor (default), true for fp editor
     *  when true, some options or layers which cannot be used in editor mode are not
     * displayed
     */
    PCB_LAYER_WIDGET( PCB_BASE_FRAME* aParent, wxWindow* aFocusOwner, bool aFpEditorMode = false );

    void ReFill();

    /**
     * Function ReFillRender
     * rebuilds Render for instance after the config is read
     */
    void ReFillRender();

    /**
     * Function SyncLayerVisibilities
     * updates each "Layer" checkbox in this layer widget according
     * to each layer's current visibility determined by IsLayerVisible(), and is
     * helpful immediately after loading a BOARD which may have state information in it.
     */
    void SyncLayerVisibilities();

    /**
     * Function SyncLayerAlphaIndicators
     * updates each "Layer"s alpha indicator to show if the board is currently being
     * rendered with more transparency or less.
     */
    void SyncLayerAlphaIndicators();

    /**
     * Function SetLayersManagerTabsText
     * Update the layer manager tabs labels
     * Useful when changing Language or to set labels to a non default value
     */
    void SetLayersManagerTabsText();

    //-----<implement LAYER_WIDGET abstract callback functions>-----------
    void OnLayerColorChange( int aLayer, COLOR4D aColor ) override;
    bool OnLayerSelect( int aLayer ) override;
    void OnLayerVisible( int aLayer, bool isVisible, bool isFinal ) override;
    void OnLayerRightClick( wxMenu& aMenu ) override;
    void OnRenderColorChange( int aId, COLOR4D aColor ) override;
    void OnRenderEnable( int aId, bool isEnabled ) override;
    //-----</implement LAYER_WIDGET abstract callback functions>----------

    /**
     * Function OnLayerSelected
     * ensure the active layer is visible, and other layers not visible
     * when m_alwaysShowActiveLayer is true
     * Otherwise do nothing.
     * @return true m_alwaysShowActiveLayer is true and the canvas is refreshed,
     * and false if do nothing
     */
    bool OnLayerSelected();     // postprocess after an active layer selection
                                // ensure active layer visible if
                                // m_alwaysShowActiveCopperLayer is true;

    /**
     * Function addRightClickMenuItems
     * add menu items to a menu that should be shown when right-clicking
     * the PCB layer widget.
     */
    void AddRightClickMenuItems( wxMenu& menu );


protected:

    static const LAYER_WIDGET::ROW  s_render_rows[];
    bool m_alwaysShowActiveCopperLayer;         // If true: Only shows the current active layer
                                                // even if it is changed
    bool m_fp_editor_mode;

    PCB_BASE_FRAME* myframe;

    // popup menu ids.
    enum POPUP_ID
    {
        ID_SHOW_ALL_COPPER_LAYERS                    = LAYER_WIDGET::ID_LAST_VALUE,
        ID_SHOW_NO_COPPER_LAYERS,
        ID_SHOW_NO_COPPER_LAYERS_BUT_ACTIVE,
        ID_ALWAYS_SHOW_NO_COPPER_LAYERS_BUT_ACTIVE,
        ID_SHOW_NO_LAYERS,
        ID_SHOW_ALL_LAYERS,
        ID_SHOW_ALL_FRONT,
        ID_SHOW_ALL_BACK,
        ID_HIDE_ALL_NON_COPPER,
        ID_SHOW_ALL_NON_COPPER,
        ID_LAST_VALUE
    };

    virtual COLOR4D getBackgroundLayerColor() override;

    /**
     * Function isAllowedInFpMode
     * @return true if item aId has meaning in footprint editor mode.
     * and therefore is shown in render panel
     */
    bool isAllowedInFpMode( int aId );

    /**
     * Function isLayerAllowedInFpMode
     *
     * User layers, which are not paired, are not shown in layers manager.  However a not
     * listed layer can be reachable in the graphic item properties dialog.
     *
     * @param aLayer is the layer id to test
     * @return true if PCB_LAYER_ID aLayer has meaning in footprint editor mode.
     * and therefore is shown in render panel
     */
    bool isLayerAllowedInFpMode( PCB_LAYER_ID aLayer );

    /**
     * Function OnRightDownLayers
     * puts up a popup menu for the layer panel.
     */
    void onRightDownLayers( wxMouseEvent& event );

    void onPopupSelection( wxCommandEvent& event );

    /// this is for the popup menu, the right click handler has to be installed
    /// on every child control within the layer panel.
    void installRightLayerClickHandler();
};

#endif  // PCB_LAYER_WIDGET_H
