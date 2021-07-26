/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef LAYER_BOX_SELECTOR_H
#define LAYER_BOX_SELECTOR_H

#include <wx/bmpcbox.h>
#include <gal/color4d.h>
#include <layers_id_colors_and_visibility.h>

using KIGFX::COLOR4D;

/**
 * Base class to build a layer list.
 */
class LAYER_SELECTOR
{
public:
    LAYER_SELECTOR();

    virtual ~LAYER_SELECTOR()
    {
    }

    bool SetLayersHotkeys( bool value );

    // Fill the layer bitmap aLayerbmp with the layer color
    static void DrawColorSwatch( wxBitmap& aLayerbmp, const COLOR4D& aBackground,
                                 const COLOR4D& aColor );

protected:
    // Return a color index from the layer id
    virtual COLOR4D getLayerColor( LAYER_NUM aLayer ) const = 0;

    // Return the name of the layer id
    virtual wxString getLayerName( LAYER_NUM aLayer ) const = 0;

    // Return true if the layer id is enabled (i.e. is it should be displayed)
    virtual bool isLayerEnabled( LAYER_NUM aLayer ) const = 0;

    bool m_layerhotkeys;
};


/*
 * Display a layer list in a wxBitmapComboBox.
 */
class LAYER_BOX_SELECTOR : public wxBitmapComboBox, public LAYER_SELECTOR
{
public:
    LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        int n = 0, const wxString choices[] = nullptr );

    LAYER_BOX_SELECTOR( wxWindow* parent, wxWindowID id,
                        const wxPoint& pos, const wxSize& size,
                        const wxArrayString& choices );

    ~LAYER_BOX_SELECTOR() override;

    // Get Current Item #
    int GetChoice();

    // Get Current Layer
    LAYER_NUM GetLayerSelection() const;

    // Set Layer #
    int SetLayerSelection( LAYER_NUM layer );

    // Reload the Layers
    // Virtual pure function because GerbView uses its own functions in a derived class
    virtual void Resync() = 0;

    // Reload the Layers bitmaps colors
    void ResyncBitmapOnly();

private:
    void onKeyDown( wxKeyEvent& aEvent );
};

#endif // LAYER_BOX_SELECTOR_H
