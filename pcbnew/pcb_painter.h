/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef PCB_PAINTER_H
#define PCB_PAINTER_H

#include <painter.h>
#include <pcb_display_options.h>
#include <math/vector2d.h>
#include <memory>


class EDA_ITEM;
class PCB_DISPLAY_OPTIONS;
class BOARD_ITEM;
class ARC;
class BOARD;
class VIA;
class TRACK;
class PAD;
class PCB_SHAPE;
class PCB_GROUP;
class FOOTPRINT;
class ZONE;
class PCB_TEXT;
class FP_TEXT;
class DIMENSION_BASE;
class PCB_TARGET;
class PCB_MARKER;
class NET_SETTINGS;
class NETINFO_LIST;

namespace KIGFX
{
class GAL;

/**
 * PCB specific render settings.
 */
class PCB_RENDER_SETTINGS : public RENDER_SETTINGS
{
public:
    friend class PCB_PAINTER;

    ///< Flags to control clearance lines visibility
    enum CLEARANCE_MODE
    {
        CL_NONE             = 0x00,

        // Object type
        CL_PADS             = 0x01,
        CL_VIAS             = 0x02,
        CL_TRACKS           = 0x04,

        // Existence
        CL_NEW              = 0x08,
        CL_EDITED           = 0x10,
        CL_EXISTING         = 0x20
    };

    PCB_RENDER_SETTINGS();

    /**
     * Load settings related to display options (high-contrast mode, full or outline modes
     * for vias/pads/tracks and so on).
     *
     * @param aOptions are settings that you want to use for displaying items.
     */
    void LoadDisplayOptions( const PCB_DISPLAY_OPTIONS& aOptions, bool aShowPageLimits );

    virtual void LoadColors( const COLOR_SETTINGS* aSettings ) override;

    /// @copydoc RENDER_SETTINGS::GetColor()
    virtual COLOR4D GetColor( const VIEW_ITEM* aItem, int aLayer ) const override;

    /**
     * Turn on/off sketch mode for given item layer.
     *
     * @param aItemLayer is the item layer that is changed.
     * @param aEnabled decides if it is drawn in sketch mode (true for sketched mode,
     *                 false for filled mode).
     */
    inline void SetSketchMode( int aItemLayer, bool aEnabled )
    {
        m_sketchMode[aItemLayer] = aEnabled;
    }

    /**
     * Return sketch mode setting for a given item layer.
     *
     * @param aItemLayer is the item layer that is changed.
     */
    inline bool GetSketchMode( int aItemLayer ) const
    {
        return m_sketchMode[aItemLayer];
    }

    /**
     * Turn on/off sketch mode for graphic items (DRAWSEGMENTs, texts).
     *
     * @param aEnabled decides if it is drawn in sketch mode (true for sketched mode,
     *                 false for filled mode).
     */
    inline void SetSketchModeGraphicItems( bool aEnabled )
    {
        m_sketchGraphics = aEnabled;
    }

    /**
     * Turn on/off drawing outline and hatched lines for zones.
     */
    void EnableZoneOutlines( bool aEnabled )
    {
        m_zoneOutlines = aEnabled;
    }

    inline bool IsBackgroundDark() const override
    {
        auto luma = m_layerColors[ LAYER_PCB_BACKGROUND ].GetBrightness();

        return luma < 0.5;
    }

    const COLOR4D& GetBackgroundColor() override { return m_layerColors[ LAYER_PCB_BACKGROUND ]; }

    void SetBackgroundColor( const COLOR4D& aColor ) override
    {
        m_layerColors[ LAYER_PCB_BACKGROUND ] = aColor;
    }

    const COLOR4D& GetGridColor() override { return m_layerColors[ LAYER_GRID ]; }

    const COLOR4D& GetCursorColor() override { return m_layerColors[ LAYER_CURSOR ]; }

    /**
     * Switch the contrast mode setting (HIGH_CONTRAST_MODE:NORMAL, DIMMED or HIDDEN )
     * to control how the non active layers are shown
     */
    void SetContrastModeDisplay( HIGH_CONTRAST_MODE aMode ) { m_contrastModeDisplay = aMode; }

    /**
     * @return the contrast mode setting (HIGH_CONTRAST_MODE:NORMAL, DIMMED or HIDDEN ).
     */
    HIGH_CONTRAST_MODE GetContrastModeDisplay() { return m_contrastModeDisplay; }

    inline bool GetCurvedRatsnestLinesEnabled() const { return m_curvedRatsnestlines; }

    inline bool GetGlobalRatsnestLinesEnabled() const { return m_globalRatsnestlines; }

    bool GetDrawIndividualViaLayers() const { return m_drawIndividualViaLayers; }
    void SetDrawIndividualViaLayers( bool aFlag ) { m_drawIndividualViaLayers = aFlag; }

    NET_COLOR_MODE GetNetColorMode() const { return m_netColorMode; }
    void SetNetColorMode( NET_COLOR_MODE aMode ) { m_netColorMode = aMode; }

    RATSNEST_MODE GetRatsnestDisplayMode() const { return m_ratsnestDisplayMode; }
    void SetRatsnestDisplayMode( RATSNEST_MODE aMode ) { m_ratsnestDisplayMode = aMode; }

    std::map<wxString, KIGFX::COLOR4D>& GetNetclassColorMap() { return m_netclassColors; }

    std::map<int, KIGFX::COLOR4D>& GetNetColorMap() { return m_netColors; }

    std::set<int>& GetHiddenNets() { return m_hiddenNets; }
    const std::set<int>& GetHiddenNets() const { return m_hiddenNets; }

    void SetZoneDisplayMode( ZONE_DISPLAY_MODE mode ) { m_zoneDisplayMode = mode; }

protected:
    ///< Maximum font size for netnames (and other dynamically shown strings)
    static const double MAX_FONT_SIZE;

    bool               m_sketchMode[GAL_LAYER_ID_END];
    bool               m_sketchGraphics;
    bool               m_sketchText;

    bool               m_padNumbers;
    bool               m_netNamesOnPads;
    bool               m_netNamesOnTracks;
    bool               m_netNamesOnVias;

    bool               m_zoneOutlines;

    bool               m_curvedRatsnestlines = true;
    bool               m_globalRatsnestlines = true;

    bool               m_drawIndividualViaLayers = false;

    ZONE_DISPLAY_MODE  m_zoneDisplayMode;
    HIGH_CONTRAST_MODE m_contrastModeDisplay;
    RATSNEST_MODE      m_ratsnestDisplayMode;

    int                m_clearanceDisplayFlags;

    ///< How to display nets and netclasses with color overrides
    NET_COLOR_MODE m_netColorMode;

    ///< Overrides for specific netclass colors
    std::map<wxString, KIGFX::COLOR4D> m_netclassColors;

    ///< Overrides for specific net colors, stored as netcodes for the ratsnest to access easily
    std::map<int, KIGFX::COLOR4D> m_netColors;

    ///< Set of net codes that should not have their ratsnest displayed
    std::set<int> m_hiddenNets;

    // These opacity overrides multiply with any opacity in the base layer color
    double m_trackOpacity;     ///< Opacity override for all tracks
    double m_viaOpacity;       ///< Opacity override for all types of via
    double m_padOpacity;       ///< Opacity override for SMD pads and PTHs
    double m_zoneOpacity;      ///< Opacity override for filled zones
};


/**
 * Contains methods for drawing PCB-specific items.
 */
class PCB_PAINTER : public PAINTER
{
public:
    PCB_PAINTER( GAL* aGal );

    /// @copydoc PAINTER::GetSettings()
    virtual PCB_RENDER_SETTINGS* GetSettings() override
    {
        return &m_pcbSettings;
    }

    /// @copydoc PAINTER::Draw()
    virtual bool Draw( const VIEW_ITEM* aItem, int aLayer ) override;

protected:
    // Drawing functions for various types of PCB-specific items
    void draw( const TRACK* aTrack, int aLayer );
    void draw( const ARC* aArc, int aLayer );
    void draw( const VIA* aVia, int aLayer );
    void draw( const PAD* aPad, int aLayer );
    void draw( const PCB_SHAPE* aSegment, int aLayer );
    void draw( const PCB_TEXT* aText, int aLayer );
    void draw( const FP_TEXT* aText, int aLayer );
    void draw( const FOOTPRINT* aFootprint, int aLayer );
    void draw( const PCB_GROUP* aGroup, int aLayer );
    void draw( const ZONE* aZone, int aLayer );
    void draw( const DIMENSION_BASE* aDimension, int aLayer );
    void draw( const PCB_TARGET* aTarget );
    void draw( const PCB_MARKER* aMarker, int aLayer );

    /**
     * Get the thickness to draw for a line (e.g. 0 thickness lines get a minimum value).
     *
     * @param aActualThickness line own thickness
     * @return the thickness to draw
     */
    int getLineThickness( int aActualThickness ) const;

    /**
     * Return drill shape of a pad.
     */
    virtual int getDrillShape( const PAD* aPad ) const;

    /**
     * Return drill size for a pad (internal units).
     */
    virtual VECTOR2D getDrillSize( const PAD* aPad ) const;

    /**
     * Return drill diameter for a via (internal units).
     */
    virtual int getDrillSize( const VIA* aVia ) const;

protected:
        PCB_RENDER_SETTINGS m_pcbSettings;
};
} // namespace KIGFX

#endif /* PCB_PAINTER_H */
