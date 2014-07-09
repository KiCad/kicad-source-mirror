/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef __CLASS_PCB_PAINTER_H
#define __CLASS_PCB_PAINTER_H

#include <layers_id_colors_and_visibility.h>
#include <boost/shared_ptr.hpp>
#include <painter.h>


class EDA_ITEM;
class COLORS_DESIGN_SETTINGS;
class DISPLAY_OPTIONS;

class BOARD_ITEM;
class BOARD;
class VIA;
class TRACK;
class D_PAD;
class DRAWSEGMENT;
class MODULE;
class SEGZONE;
class ZONE_CONTAINER;
class TEXTE_PCB;
class TEXTE_MODULE;
class DIMENSION;
class PCB_TARGET;
class MARKER_PCB;

namespace KIGFX
{
class GAL;

/**
 * Class PCB_RENDER_SETTINGS
 * Stores PCB specific render settings.
 */
class PCB_RENDER_SETTINGS : public RENDER_SETTINGS
{
public:
    friend class PCB_PAINTER;

    enum ClearanceMode {
        CL_VIAS     = 0x1,
        CL_PADS     = 0x2,
        CL_TRACKS   = 0x4
    };

    ///> Determines how zones should be displayed
    enum DisplayZonesMode {
        DZ_HIDE_FILLED = 0,
        DZ_SHOW_FILLED,
        DZ_SHOW_OUTLINED
    };

    PCB_RENDER_SETTINGS();

    /// @copydoc RENDER_SETTINGS::ImportLegacyColors()
    void ImportLegacyColors( const COLORS_DESIGN_SETTINGS* aSettings );

    /**
     * Function LoadDisplayOptions
     * Loads settings related to display options (high-contrast mode, full or outline modes
     * for vias/pads/tracks and so on).
     * @param aOptions are settings that you want to use for displaying items.
     */
    void LoadDisplayOptions( const DISPLAY_OPTIONS& aOptions );

    /// @copydoc RENDER_SETTINGS::GetColor()
    virtual const COLOR4D& GetColor( const VIEW_ITEM* aItem, int aLayer ) const;

    /**
     * Function GetLayerColor
     * Returns the color used to draw a layer.
     * @param aLayer is the layer number.
     */
    inline const COLOR4D& GetLayerColor( int aLayer ) const
    {
        return m_layerColors[aLayer];
    }

    /**
     * Function SetLayerColor
     * Changes the color used to draw a layer.
     * @param aLayer is the layer number.
     * @param aColor is the new color.
     */
    inline void SetLayerColor( int aLayer, const COLOR4D& aColor )
    {
        m_layerColors[aLayer] = aColor;

        update();       // recompute other shades of the color
    }

    /**
     * Function SetSketchMode
     * Turns on/off sketch mode for given item layer.
     * @param aItemLayer is the item layer that is changed.
     * @param aEnabled decides if it is drawn in sketch mode (true for sketched mode,
     * false for filled mode).
     */
    inline void SetSketchMode( int aItemLayer, bool aEnabled )
    {
        // It is supposed to work only with item layers
        assert( aItemLayer >= ITEM_GAL_LAYER( 0 ) );

        m_sketchMode[aItemLayer] = aEnabled;
    }

    /**
     * Function GetSketchMode
     * Returns sketch mode setting for a given item layer.
     * @param aItemLayer is the item layer that is changed.
     */
    inline bool GetSketchMode( int aItemLayer ) const
    {
        // It is supposed to work only with item layers
        assert( aItemLayer >= ITEM_GAL_LAYER( 0 ) );

        return m_sketchMode[aItemLayer];
    }

protected:
    ///> @copydoc RENDER_SETTINGS::Update()
    void update();

    ///> Colors for all layers (normal)
    COLOR4D m_layerColors[TOTAL_LAYER_COUNT];

    ///> Colors for all layers (highlighted)
    COLOR4D m_layerColorsHi[TOTAL_LAYER_COUNT];

    ///> Colors for all layers (selected)
    COLOR4D m_layerColorsSel[TOTAL_LAYER_COUNT];

    ///> Colors for all layers (darkened)
    COLOR4D m_layerColorsDark[TOTAL_LAYER_COUNT];

    ///> Flag determining if items on a given layer should be drawn as an outline or a filled item
    bool    m_sketchMode[TOTAL_LAYER_COUNT];

    ///> Flag determining if pad numbers should be visible
    bool    m_padNumbers;

    ///> Flag determining if net names should be visible for pads
    bool    m_netNamesOnPads;

    ///> Flag determining if net names should be visible for tracks
    bool    m_netNamesOnTracks;

    ///> Maximum font size for netnames (and other dynamically shown strings)
    static const double MAX_FONT_SIZE;

    ///> Option for different display modes for zones
    DisplayZonesMode m_displayZoneMode;
};


/**
 * Class PCB_PAINTER
 * Contains methods for drawing PCB-specific items.
 */
class PCB_PAINTER : public PAINTER
{
public:
    PCB_PAINTER( GAL* aGal );

    /// @copydoc PAINTER::ApplySettings()
    virtual void ApplySettings( const RENDER_SETTINGS* aSettings )
    {
        m_pcbSettings = *static_cast<const PCB_RENDER_SETTINGS*>( aSettings );
    }

    /// @copydoc PAINTER::GetSettings()
    virtual RENDER_SETTINGS* GetSettings()
    {
        return &m_pcbSettings;
    }

    /// @copydoc PAINTER::Draw()
    virtual bool Draw( const VIEW_ITEM* aItem, int aLayer );

protected:
    PCB_RENDER_SETTINGS m_pcbSettings;

    // Drawing functions for various types of PCB-specific items
    void draw( const TRACK* aTrack, int aLayer );
    void draw( const VIA* aVia, int aLayer );
    void draw( const D_PAD* aPad, int aLayer );
    void draw( const DRAWSEGMENT* aSegment );
    void draw( const TEXTE_PCB* aText, int aLayer );
    void draw( const TEXTE_MODULE* aText, int aLayer );
    void draw( const MODULE* aModule );
    void draw( const ZONE_CONTAINER* aZone );
    void draw( const DIMENSION* aDimension, int aLayer );
    void draw( const PCB_TARGET* aTarget );
    void draw( const MARKER_PCB* aMarker );
};
} // namespace KIGFX

#endif /* __CLASS_PAINTER_H */

