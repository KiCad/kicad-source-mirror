/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
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

#ifndef __GERBVIEW_PAINTER_H
#define __GERBVIEW_PAINTER_H

#include <layer_ids.h>
#include <gal/painter.h>

#include <dcode.h>
#include <gbr_display_options.h>
#include <geometry/shape_poly_set.h>

#include <memory>


class EDA_ITEM;
class GERBER_DRAW_ITEM;
class GERBER_FILE_IMAGE;


namespace KIGFX
{
class GAL;

/**
 * Store GerbView specific render settings.
 */
class GERBVIEW_RENDER_SETTINGS : public RENDER_SETTINGS
{
public:
    friend class GERBVIEW_PAINTER;

    GERBVIEW_RENDER_SETTINGS();

    void LoadColors( const COLOR_SETTINGS* aSettings ) override;

    /// @copydoc RENDER_SETTINGS::GetColor()
    virtual COLOR4D GetColor( const VIEW_ITEM* aItem, int aLayer ) const override;

    /**
     * Return the color used to draw a layer.
     *
     * @param aLayer is the layer number.
     */
    inline const COLOR4D& GetLayerColor( int aLayer ) const
    {
        auto it = m_layerColors.find( aLayer );
        return it == m_layerColors.end() ? COLOR4D::WHITE : it->second;
    }

    /**
     * Change the color used to draw a layer.
     *
     * @param aLayer is the layer number.
     * @param aColor is the new color.
     */
    inline void SetLayerColor( int aLayer, const COLOR4D& aColor )
    {
        m_layerColors[aLayer] = aColor;

        update();       // recompute other shades of the color
    }

    const COLOR4D& GetBackgroundColor() const override
    {
        auto it = m_layerColors.find( LAYER_GERBVIEW_BACKGROUND );
        return it == m_layerColors.end() ? COLOR4D::BLACK : it->second;
    }

    void SetBackgroundColor( const COLOR4D& aColor ) override
    {
        m_layerColors[ LAYER_GERBVIEW_BACKGROUND ] = aColor;
    }

    const COLOR4D& GetGridColor() override { return m_layerColors[ LAYER_GERBVIEW_GRID ]; }

    const COLOR4D& GetCursorColor() override { return m_layerColors[ LAYER_CURSOR ]; }

    bool GetShowPageLimits() const override;

    /// Clear all highlight selections (dcode, net, component, attribute selection)
    void ClearHighlightSelections();

    /// If set to anything but an empty string, will highlight items with matching component
    wxString m_componentHighlightString;

    /// If set to anything but an empty string, will highlight items with matching net
    wxString m_netHighlightString;

    /// If set to anything but an empty string, will highlight items with matching attribute
    wxString m_attributeHighlightString;

    /// If set to anything but >0 (in fact 10 the min dcode value),
    /// will highlight items with matching dcode
    int m_dcodeHighlightValue;

protected:
    /// Maximum font size for D-Codes and other strings
    static const double MAX_FONT_SIZE;
};


/**
 * Methods for drawing GerbView specific items.
 */
class GERBVIEW_PAINTER : public PAINTER
{
public:
    GERBVIEW_PAINTER( GAL* aGal );

    /// @copydoc PAINTER::GetSettings()
    virtual GERBVIEW_RENDER_SETTINGS* GetSettings() override
    {
        return &m_gerbviewSettings;
    }

    /// @copydoc PAINTER::Draw()
    virtual bool Draw( const VIEW_ITEM* aItem, int aLayer ) override;

protected:
    GERBVIEW_RENDER_SETTINGS m_gerbviewSettings;

    // Drawing functions
    void draw( /*const*/ GERBER_DRAW_ITEM* aVia, int aLayer );

    /**
     * Helper routine to draw a polygon.
     *
     * @param aParent Pointer to the draw item for AB Position calculation.
     * @param aPolygon the polygon to draw.
     * @param aFilled If true, draw the polygon as filled, otherwise only outline.
     * @param aShift If true, draw the polygon relative to the parent item position.
     */
    void drawPolygon( GERBER_DRAW_ITEM* aParent, const SHAPE_POLY_SET& aPolygon,
                      bool aFilled, bool aShift = false );

    /// Helper to draw a flashed shape (aka spot)
    void drawFlashedShape( GERBER_DRAW_ITEM* aItem, bool aFilled );

    /// Helper to draw an aperture macro shape
    void drawApertureMacro( GERBER_DRAW_ITEM* aParent, bool aFilled );

    /**
     * Get the thickness to draw for a line (e.g. 0 thickness lines get a minimum value).
     *
     * @param aActualThickness line own thickness.
     * @return the thickness to draw.
     */
    int getLineThickness( int aActualThickness ) const;
};
} // namespace KIGFX

#endif /* __GERBVIEW_PAINTER_H */
