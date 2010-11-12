/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2004-2010 Jean-Pierre Charras, jean-pierre.charras@gpisa-lab.inpg.fr
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 Kicad Developers, see change_log.txt for contributors.
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

#include "layer_widget.h"

/**
 * Class GERBER_LAYER_WIDGET
 * is here to implement the abtract functions of LAYER_WIDGET so they
 * may be tied into the WinEDA_GerberFrame's data and so we can add a popup
 * menu which is specific to PCBNEW's needs.
 */
class GERBER_LAYER_WIDGET : public LAYER_WIDGET
{
    WinEDA_GerberFrame*    myframe;

    // popup menu ids.
#define ID_SHOW_ALL_COPPERS     wxID_HIGHEST
#define ID_SHOW_NO_COPPERS      (wxID_HIGHEST+1)

    /**
     * Function OnRightDownLayers
     * puts up a popup menu for the layer panel.
     */
    void onRightDownLayers( wxMouseEvent& event );

    void onPopupSelection( wxCommandEvent& event );

    /// this is for the popup menu, the right click handler has to be installed
    /// on every child control within the layer panel.
    void installRightLayerClickHandler();

public:

    /**
     * Constructor
     * @param aPointSize is the font point size to use within the widget.  This
     *  effectively sets the overal size of the widget via the row height and bitmap
     *  button sizes.
     */
    GERBER_LAYER_WIDGET( WinEDA_GerberFrame* aParent, wxWindow* aFocusOwner, int aPointSize = 10 );

    void ReFill();

    //-----<implement LAYER_WIDGET abstract callback functions>-----------
    void OnLayerColorChange( int aLayer, int aColor );
    bool OnLayerSelect( int aLayer );
    void OnLayerVisible( int aLayer, bool isVisible, bool isFinal );
    void OnRenderColorChange( int aId, int aColor );
    void OnRenderEnable( int aId, bool isEnabled );
    /**
     * Function SetLayersManagerTabsText
     * Update the layer manager tabs labels
     * Useful when changing Language or to set labels to a non default value
     */
    void SetLayersManagerTabsText( );
    //-----</implement LAYER_WIDGET abstract callback functions>----------
};

#endif  // _CLASS_GERBER_LAYER_WIDGET_H_
