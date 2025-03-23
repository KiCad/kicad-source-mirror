/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2015 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
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

#ifndef PCB_LAYER_BOX_SELECTOR_H
#define PCB_LAYER_BOX_SELECTOR_H

#include <memory>

#include <lset.h>
#include <widgets/layer_box_selector.h>

class PCB_BASE_FRAME;
class PCB_LAYER_PRESENTATION;


/**
 * Class to display a pcb layer list in a wxBitmapComboBox.
 */
class PCB_LAYER_BOX_SELECTOR : public LAYER_BOX_SELECTOR
{
public:
    // If you are thinking the constructor is a bit curious, just remember it is automatically
    // generated when used in wxFormBuilder files, and so must have the same signature as the
    // wxBitmapComboBox constructor.  In particular, value and style are not used by this class.
    PCB_LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id, const wxString& value = wxEmptyString,
                            const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize, int n = 0,
                            const wxString choices[] = nullptr, int style = 0 );

    // SetBoardFrame should be called after creating a PCB_LAYER_BOX_SELECTOR.  It is not passed
    // through the constructor because it must have the same signature as wxBitmapComboBox for
    // use with wxFormBuilder.
    void SetBoardFrame( PCB_BASE_FRAME* aFrame );

    // SetLayerSet allows disabling some layers, which are not shown in list
    void SetNotAllowedLayerSet( const LSET& aMask ) { m_layerMaskDisable = aMask; }

    // If the UNDEFINED_LAYER should be selectable, give it a name here.  Usually either
    // INDETERMINATE_STATE or INDETERMINATE_ACTION.
    void SetUndefinedLayerName( const wxString& aName ) { m_undefinedLayerName = aName; }

    // Reload the Layers names and bitmaps
    void Resync() override;

    // Allow (or not) the layers not activated for the current board to be shown in layer
    // selector. Not activated layers have their names appended with "(not activated)".
    void ShowNonActivatedLayers( bool aShow ) { m_showNotEnabledBrdlayers = aShow; }

private:
    // Returns true if the layer id is enabled (i.e. if it should be displayed)
    bool isLayerEnabled( int aLayer ) const override;

    LSET getEnabledLayers() const;

    PCB_BASE_FRAME* m_boardFrame;

    LSET     m_layerMaskDisable;        // A mask to remove some (not allowed) layers
                                        // from layer list
    bool     m_showNotEnabledBrdlayers; // true to list all allowed layers
                                        // (with not activated layers flagged)
    wxString m_undefinedLayerName;      // if not empty add an item with this name which sets
                                        // the layer to UNDEFINED_LAYER

    std::unique_ptr<PCB_LAYER_PRESENTATION> m_layerPresentation;
};

#endif // PCB_LAYER_BOX_SELECTOR_H
