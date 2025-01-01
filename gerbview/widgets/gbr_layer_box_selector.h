/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#ifndef GBR_LAYER_BOX_SELECTOR_H
#define GBR_LAYER_BOX_SELECTOR_H

#include <memory>

#include <widgets/layer_box_selector.h>

class LAYER_PRESENTATION;

// class to display a layer list in GerbView.
class GBR_LAYER_BOX_SELECTOR : public LAYER_BOX_SELECTOR
{
public:
    GBR_LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize, int n = 0,
                            const wxString choices[] = nullptr );

    // Reload the Layers names and bitmaps
    void Resync() override;

    // Return true if the layer id is enabled (i.e. is it should be displayed)
    bool isLayerEnabled( int aLayer ) const override { return true; }

private:
    std::unique_ptr<LAYER_PRESENTATION> m_layerPresentation;
};

#endif //GBR_LAYER_BOX_SELECTOR_H
