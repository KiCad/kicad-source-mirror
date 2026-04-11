/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LAYER_PRESENTATION_H
#define LAYER_PRESENTATION_H

#include <gal/color4d.h>
#include <layer_ids.h>

class wxBitmap;
class wxBitmapBundle;

using KIGFX::COLOR4D;

/**
 * Base class for an object that can provide information about
 * presenting layers (colours, etc).
 */
class LAYER_PRESENTATION
{
public:
    virtual ~LAYER_PRESENTATION() {}

    // Return a color index from the layer id
    virtual COLOR4D getLayerColor( int aLayer ) const = 0;

    // Return the name of the layer id
    virtual wxString getLayerName( int aLayer ) const = 0;

    // Fill the layer bitmap aLayerbmp with the layer color
    static void DrawColorSwatch( wxBitmap& aLayerbmp, const COLOR4D& aBackground,
                                 const COLOR4D& aColor );

    /**
     * Fill the layer bitmap aLayerbmp with the layer color
     * for the layer ID.
     */
    void DrawColorSwatch( wxBitmap& aLayerbmp, int aLayer ) const;

    /**
     * Create a layer pair "side-by-side swatch" icon
     */
    static wxBitmapBundle CreateLayerPairIcon( const COLOR4D& aTopColor, const COLOR4D& aBottomColor,
                                               int aDefSize = 24 );

    /**
     * Create a layer pair "side-by-side swatch" icon for the given
     * layer pair with the style of this presentation.
     */
    wxBitmapBundle CreateLayerPairIcon( int aLeftLayer, int aRightLayer, int aDefSize = 24 ) const;
};

#endif // LAYER_PRESENTATION_H
