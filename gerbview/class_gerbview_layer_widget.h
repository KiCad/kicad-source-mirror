/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2010 Jean-Pierre Charras, jean-pierre.charras@gpisa-lab.inpg.fr
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 KiCad Developers, see change_log.txt for contributors.
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
/* class_gerber_layer_widget.h : header for the layers manager */
/************************************************************/

#ifndef _CLASS_GERBER_LAYER_WIDGET_H_
#define _CLASS_GERBER_LAYER_WIDGET_H_

#include <layer_widget.h>

// popup menu ids. in layer manager
enum LAYER_MANAGER
{
    ID_LAYER_MANAGER_START = wxID_HIGHEST+1,
    ID_SHOW_ALL_LAYERS = ID_LAYER_MANAGER_START,
    ID_SHOW_NO_LAYERS,
    ID_SHOW_NO_LAYERS_BUT_ACTIVE,
    ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE,
    ID_SORT_GBR_LAYERS,
    ID_LAYER_MANAGER_END = ID_SORT_GBR_LAYERS,
};

/**
 * Class GERBER_LAYER_WIDGET
 * is here to implement the abtract functions of LAYER_WIDGET so they
 * may be tied into the GERBVIEW_FRAME's data and so we can add a popup
 * menu which is specific to Pcbnew's needs.
 */
class GERBER_LAYER_WIDGET : public LAYER_WIDGET
{
    GERBVIEW_FRAME*    myframe;
    bool m_alwaysShowActiveLayer;   // If true: Only shows the current active layer
                                    // even if it is changed


    /**
     * Function OnRightDownLayers
     * puts up a popup menu for the layer panel.
     */
    void onRightDownLayers( wxMouseEvent& event );

    void onPopupSelection( wxCommandEvent& event );

    /// this is for the popup menu, the right click handler has to be installed
    /// on every child control within the layer panel.
    void installRightLayerClickHandler();

    /**
     * Virtual Function useAlternateBitmap
     * @return true if bitmaps shown in Render layer list
     * are alternate bitmaps, or false if they are "normal" bitmaps
     */
    virtual bool useAlternateBitmap(int aRow);

public:

    /**
     * Constructor
     * @param aParent : the parent frame
     * @param aFocusOwner : the window that has the keyboard focus.
     * @param aPointSize is the font point size to use within the widget.  This
     *  effectively sets the overal size of the widget via the row height and bitmap
     *  button sizes.
     */
    GERBER_LAYER_WIDGET( GERBVIEW_FRAME* aParent, wxWindow* aFocusOwner, int aPointSize = 10 );

    void ReFill();

    /**
     * Function ReFillRender
     * Rebuild Render for instance after the config is read
     */
    void ReFillRender();

    //-----<implement LAYER_WIDGET abstract callback functions>-----------
    void OnLayerColorChange( int aLayer, EDA_COLOR_T aColor );
    bool OnLayerSelect( int aLayer );
    void OnLayerVisible( int aLayer, bool isVisible, bool isFinal );
    void OnRenderColorChange( int aId, EDA_COLOR_T aColor );
    void OnRenderEnable( int aId, bool isEnabled );
    /**
     * Function SetLayersManagerTabsText
     * Update the layer manager tabs labels
     * Useful when changing Language or to set labels to a non default value
     */
    void SetLayersManagerTabsText( );
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
     * Function UpdateLayerIcons
     * Update the layer manager icons (layers only)
     * Useful when loading a file or clearing a layer because they change
     */
    void UpdateLayerIcons();
};

#endif  // _CLASS_GERBER_LAYER_WIDGET_H_
