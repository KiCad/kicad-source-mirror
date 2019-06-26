/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef __CLASS_PAINTER_H
#define __CLASS_PAINTER_H

#include <map>
#include <set>

#include <gal/color4d.h>
#include <ws_draw_item.h>
#include <layers_id_colors_and_visibility.h>
#include <memory>

class EDA_ITEM;
class COLORS_DESIGN_SETTINGS;

namespace KIGFX
{
class GAL;
class VIEW_ITEM;

/**
 * Class RENDER_SETTINGS
 * Contains all the knowledge about how graphical objects are drawn on
 * any output surface/device. This includes:
 * - color/transparency settings
 * - highlighting and high contrast mode control
 * - drawing quality control (sketch/outline mode)
 * The class acts as an interface between the PAINTER object and the GUI (i.e. Layers/Items
 * widget or display options dialog).
 */
class RENDER_SETTINGS
{
public:
    RENDER_SETTINGS();
    virtual ~RENDER_SETTINGS();

    /**
     * Function ImportLegacyColors
     * Loads a list of color settings for layers.
     * @param aSettings is a list of color settings.
     */
    virtual void ImportLegacyColors( const COLORS_DESIGN_SETTINGS* aSettings ) { }

    /**
     * Function SetActiveLayer
     * Sets the specified layer as active - it means that it can be drawn in a specific mode
     * (eg. highlighted, so it differs from other layers).
     * @param aLayerId is a layer number that should be displayed in a specific mode.
     * @param aEnabled is the new layer state ( true = active or false = not active).
     */
    inline void SetActiveLayer( int aLayerId, bool aEnabled = true )
    {
        if( aEnabled )
            m_activeLayers.insert( aLayerId );
        else
            m_activeLayers.erase( aLayerId );
    }

    /**
     * Function GetActiveLayers()
     * Returns the set of currently active layers.
     * @return The set of currently active layers.
     */
    const std::set<unsigned int> GetActiveLayers()
    {
        return m_activeLayers;
    }

    /**
     * Function ClearActiveLayers
     * Clears the list of active layers.
     */
    inline void ClearActiveLayers()
    {
        m_activeLayers.clear();
    }

    /**
     * Function IsActiveLayer
     * Returns information whether the queried layer is marked as active.
     * @return True if the queried layer is marked as active.
     */
    inline bool IsActiveLayer( int aLayerId ) const
    {
        return ( m_activeLayers.count( aLayerId ) > 0 );
    }

    /**
     * Function IsHighlightEnabled
     * Returns current highlight setting.
     * @return True if highlight is enabled, false otherwise.
     */
    inline bool IsHighlightEnabled() const
    {
        return m_highlightEnabled;
    }

    /**
     * Function GetHighlightNetCode
     * Returns netcode of currently highlighted net.
     * @return Netcode of currently highlighted net.
     */
    inline int GetHighlightNetCode() const
    {
        return m_highlightNetcode;
    }

    /**
     * Function SetHighlight
     * Turns on/off highlighting - it may be done for the active layer, the specified net, or
     * items with their HIGHLIGHTED flags set.
     * @param aEnabled tells if highlighting should be enabled.
     * @param aNetcode is optional and if specified, turns on higlighting only for the net with
     * number given as the parameter.
     */
    inline void SetHighlight( bool aEnabled, int aNetcode = -1, bool aHighlightItems = false )
    {
        m_highlightEnabled = aEnabled;
        m_highlightNetcode = aEnabled ? aNetcode : -1;
        m_highlightItems = aEnabled ? aHighlightItems : false;
    }

    /**
     * Function SetHighContrast
     * Turns on/off high contrast display mode.
     * @param aEnabled determines if high contrast display mode should be enabled or not.
     */
    inline void SetHighContrast( bool aEnabled )
    {
        m_hiContrastEnabled = aEnabled;
    }

    /**
     * Function GetHighContrast
     * Returns information about high contrast display mode.
     * @return True if the high contrast mode is on, false otherwise.
     */
    inline bool GetHighContrast() const
    {
        return m_hiContrastEnabled;
    }

    /**
     * Function GetColor
     * Returns the color that should be used to draw the specific VIEW_ITEM on the specific layer
     * using currently used render settings.
     * @param aItem is the VIEW_ITEM.
     * @param aLayer is the layer.
     * @return The color.
     */
    virtual const COLOR4D& GetColor( const VIEW_ITEM* aItem, int aLayer ) const = 0;

    float GetWorksheetLineWidth() const
    {
        return m_worksheetLineWidth;
    }

    inline bool GetShowPageLimits() const
    {
        return m_showPageLimits;
    }

    inline void SetShowPageLimits( bool aDraw )
    {
        m_showPageLimits = aDraw;
    }

    /**
     * Function GetBackgroundColor
     * Returns current background color settings.
     */
    virtual const COLOR4D& GetBackgroundColor() = 0;

    /**
     * Sets the background color.
     */
    virtual void SetBackgroundColor( const COLOR4D& aColor ) = 0;

    /**
     * Function GetGridColor
     * Returns current grid color settings.
     */
    virtual const COLOR4D& GetGridColor() = 0;

    /**
     * Function GetCursorColor
     * Returns current cursor color settings.
     */
    virtual const COLOR4D& GetCursorColor() = 0;

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

    virtual bool IsBackgroundDark() const
    {
        return false;
    }

    /**
     * Set line width used for drawing outlines.
     *
     * @param aWidth is the new width.
     */
    void SetOutlineWidth( float aWidth )
    {
        m_outlineWidth = aWidth;
    }

protected:
    /**
     * Function update
     * Precalculates extra colors for layers (e.g. highlighted, darkened and any needed version
     * of base colors).
     */
    virtual void update();

    std::set<unsigned int> m_activeLayers; ///< Stores active layers number

    ///> Colors for all layers (normal)
    COLOR4D m_layerColors[LAYER_ID_COUNT];

    ///> Colors for all layers (highlighted)
    COLOR4D m_layerColorsHi[LAYER_ID_COUNT];

    ///> Colors for all layers (selected)
    COLOR4D m_layerColorsSel[LAYER_ID_COUNT];

    ///> Colors for all layers (darkened)
    COLOR4D m_layerColorsDark[LAYER_ID_COUNT];

    ///< Colora used for high contrast display mode
    COLOR4D m_hiContrastColor[LAYER_ID_COUNT];

    /// Parameters for display modes
    bool    m_hiContrastEnabled;    ///< High contrast display mode on/off
    float   m_hiContrastFactor;     ///< Factor used for computing high contrast color

    bool    m_highlightEnabled;     ///< Highlight display mode on/off
    int     m_highlightNetcode;     ///< Net number that is displayed in highlight
                                    ///< -1 means that there is no specific net, and whole active
                                    ///< layer is highlighted
    bool    m_highlightItems;       ///< Highlight items with their HIGHLIGHT flags set
    float   m_highlightFactor;      ///< Factor used for computing hightlight color

    float   m_selectFactor;         ///< Specifies how color of selected items is changed
    float   m_layerOpacity;         ///< Determines opacity of all layers
    float   m_outlineWidth;         ///< Line width used when drawing outlines
    float   m_worksheetLineWidth;   ///< Line width used when drawing worksheet

    bool    m_showPageLimits;

    COLOR4D m_backgroundColor;      ///< The background color
};


/**
 * Class PAINTER
 * contains all the knowledge about how to draw graphical object onto
 * any particular output device.
 * This knowledge is held outside the individual graphical objects so that
 * alternative output devices may be used, and so that the graphical objects
 * themselves to not contain drawing routines.  Drawing routines in the objects
 * cause problems with usages of the objects as simple container objects in
 * DLL/DSOs.
 * PAINTER is an abstract layer, because every module (pcbnew, eeschema, etc.)
 * has to draw different kinds of objects.
 */
class PAINTER
{
public:
    /**
     * Constructor PAINTER( GAL* )
     * initializes this object for painting on any of the polymorphic
     * GRAPHICS_ABSTRACTION_LAYER* derivatives.
     *
     * @param aGal is a pointer to a polymorphic GAL device on which
     *  to draw (i.e. Cairo, OpenGL, wxDC)
     *  No ownership is given to this PAINTER of aGal.
     */
    PAINTER( GAL* aGal );
    virtual ~PAINTER();

    /**
     * Function SetGAL
     * Changes Graphics Abstraction Layer used for drawing items for a new one.
     * @param aGal is the new GAL instance.
     */
    void SetGAL( GAL* aGal )
    {
        m_gal = aGal;
    }

    /**
     * Function ApplySettings
     * Loads colors and display modes settings that are going to be used when drawing items.
     * @param aSettings are settings to be applied.
     */
    virtual void ApplySettings( const RENDER_SETTINGS* aSettings ) = 0;

    /**
     * Function GetSettings
     * Returns pointer to current settings that are going to be used when drawing items.
     * @return Current rendering settings.
     */
    virtual RENDER_SETTINGS* GetSettings() = 0;

    /**
     * Function Draw
     * Takes an instance of VIEW_ITEM and passes it to a function that know how to draw the item.
     * @param aItem is an item to be drawn.
     * @param aLayer tells which layer is currently rendered so that draw functions
     * may know what to draw (eg. for pads there are separate layers for holes, because they
     * have other dimensions then the pad itself.
     */
    virtual bool Draw( const VIEW_ITEM* aItem, int aLayer ) = 0;

protected:
    /// Instance of graphic abstraction layer that gives an interface to call
    /// commands used to draw (eg. DrawLine, DrawCircle, etc.)
    GAL* m_gal;

    /// Color of brightened item frame
    COLOR4D m_brightenedColor;
};
} // namespace KIGFX

#endif /* __CLASS_PAINTER_H */
