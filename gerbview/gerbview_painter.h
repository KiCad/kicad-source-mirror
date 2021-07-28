/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <layers_id_colors_and_visibility.h>
#include <painter.h>

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

    /**
     * Load settings related to display options.
     *
     * @param aOptions are settings that you want to use for displaying items.
     */
    void LoadDisplayOptions( const GBR_DISPLAY_OPTIONS& aOptions );

    /// @copydoc RENDER_SETTINGS::GetColor()
    virtual COLOR4D GetColor( const VIEW_ITEM* aItem, int aLayer ) const override;

    /**
     * Return the color used to draw a layer.
     *
     * @param aLayer is the layer number.
     */
    inline const COLOR4D& GetLayerColor( int aLayer ) const
    {
        return m_layerColors[aLayer];
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

    const COLOR4D& GetBackgroundColor() override
    {
        return m_layerColors[ LAYER_GERBVIEW_BACKGROUND ];
    }

    void SetBackgroundColor( const COLOR4D& aColor ) override
    {
        m_layerColors[ LAYER_GERBVIEW_BACKGROUND ] = aColor;
    }

    const COLOR4D& GetGridColor() override { return m_layerColors[ LAYER_GERBVIEW_GRID ]; }

    const COLOR4D& GetCursorColor() override { return m_layerColors[ LAYER_CURSOR ]; }

    inline bool IsSpotFill() const
    {
        return m_spotFill;
    }

    inline bool IsLineFill() const
    {
        return m_lineFill;
    }

    inline bool IsPolygonFill() const
    {
        return m_polygonFill;
    }

    inline bool IsShowNegativeItems() const
    {
        return m_showNegativeItems;
    }

    inline bool IsShowCodes() const
    {
        return m_showCodes;
    }

    inline bool IsDiffMode() const
    {
        return m_diffMode;
    }

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
    /// Flag determining if spots should be drawn with fill
    bool    m_spotFill;

    /// Flag determining if lines should be drawn with fill
    bool    m_lineFill;

    /// Flag determining if polygons should be drawn with fill
    bool    m_polygonFill;

    /// Flag determining if negative items should be drawn with a "ghost" color
    bool    m_showNegativeItems;

    /// Flag determining if D-Codes should be drawn
    bool    m_showCodes;

    /// Flag determining if layers should be rendered in "diff" mode
    bool    m_diffMode;

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
