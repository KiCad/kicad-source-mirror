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

#include <vector>
#include <map>

#include <gal/color4d.h>
#include <colors.h>


class EDA_ITEM;
class COLORS_DESIGN_SETTINGS;

namespace KiGfx
{
class GAL;
class STROKE_FONT;

/**
 * Class RENDER_SETTINGS
 * Contains all the knowledge about how graphical objects are drawn on
 * any output surface/device. This includes:
 * - color/transparency settings
 * - highlighting and high contrast mode control
 * - drawing quality control (sketch/outline mode)
 * The class acts as an interface between the PAINTER object and the GUI (i.e. Layers/Items
 * widget or display options dialog).
 *
 * Todo: properties/introspection
 */
class RENDER_SETTINGS
{
public:

    RENDER_SETTINGS();
    virtual ~RENDER_SETTINGS();

    /**
     * Function Update
     * Precalculates extra colors for layers (eg. highlighted, darkened and any needed version
     * of base colors).
     */
    virtual void Update();

    /**
     * Function ImportLegacyColors
     * Loads a list of color settings for layers.
     * @param aSettings is a list of color settings.
     */
    virtual void ImportLegacyColors( COLORS_DESIGN_SETTINGS* aSettings ) = 0;

    /**
     * Function SetActiveLayer
     * Sets the specified layer as active - it means that it can be drawn in a specific mode
     * (eg. highlighted, so it differs from other layers).
     * @param aLayerId is a layer number that should be displayed in a specific mode.
     */
    void SetActiveLayer( int aLayerId )
    {
        m_activeLayer = aLayerId;
    }

    /**
     * Function SetHighlight
     * Turns on/off highlighting - it may be done for the active layer or the specified net.
     * @param aEnabled tells if highlighting should be enabled.
     * @param aNetCode is optional and if specified, turns on higlighting only for the net with
     * number given as the parameter.
     */
    void SetHighlight( bool aEnabled, int aNetcode = -1 )
    {
        m_highlightEnabled = aEnabled;

        if( aNetcode > 0 )
            m_highlightNetcode = aNetcode;
    }

    /**
     * Function SetHighContrast
     * Turns on/off high contrast display mode.
     * @param aEnabled determines if high contrast display mode should be enabled or not.
     */
    void SetHighContrast( bool aEnabled )
    {
        m_hiContrastEnabled = aEnabled;
    }

protected:

    int     m_activeLayer;          /// Stores active layer number

    /// Parameters for display modes
    bool    m_hiContrastEnabled;    /// High contrast display mode on/off
    COLOR4D m_hiContrastColor;      /// Color used for high contrast display mode
    float   m_hiContrastFactor;     /// Factor used for computing high contrast color

    bool    m_highlightEnabled;     /// Highlight display mode on/off
    int     m_highlightNetcode;     /// Net number that is displayed in highlight
                                    /// -1 means that there is no specific net, and whole active
                                    /// layer is highlighted
    float   m_highlightFactor;      /// Factor used for computing hightlight color

    COLOR4D m_selectionBorderColor; /// Color of selection box border
    COLOR4D m_netLabelColor;        /// Color of net labels

    float   m_selectFactor;         /// Specifies how color of selected items is changed
    float   m_layerOpacity;         /// Determines opacity of all layers, so every can be seen
                                    /// at the same time
    float   m_outlineWidth;         /// Line width used when drawing outlines

    /// Map of colors that were usually used for display
    std::map<EDA_COLOR_T, COLOR4D> m_legacyColorMap;
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

    /*
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
     * Function ApplySettings
     * Loads colors and display modes settings that are going to be used when drawing items.
     * @param aSettings are settings to be applied.
     */
    virtual void ApplySettings( RENDER_SETTINGS* aSettings )
    {
        if( m_settings )
            delete m_settings;

        m_settings = aSettings;
    }

    /**
     * Function SetGAL
     * Changes Graphics Abstraction Layer used for drawing items for a new one.
     * @param aGal is the new GAL instance.
     */
    void SetGAL( GAL* aGal );

    /**
     * Function GetSettings
     * Returns pointer to current settings that are going to be used when drawing items.
     * @return Current rendering settings.
     */
    virtual RENDER_SETTINGS* GetSettings()
    {
        return m_settings;
    }

    /**
     * Function Draw
     * Takes an instance of EDA_ITEM and passes it to a function that know how to draw the item.
     * @param aItem is an item to be drawn.
     * @param aLayer tells which layer is currently rendered so that draw functions
     * may know what to draw (eg. for pads there are separate layers for holes, because they
     * have other dimensions then the pad itself.
     */
    virtual bool Draw( const EDA_ITEM* aItem, int aLayer ) = 0;

protected:

    /**
     * Function getLayerColor
     * is used for obtaining color that should be used for specific layer/net
     * combination using stored color settings.
     * @param aLayer is the layer number that is being drawn.
     * @param aNetCode is a number of the net that is being drawn.
     */
    virtual const COLOR4D& getLayerColor( int aLayer, int aNetCode ) const = 0;

    /// Instance of graphic abstraction layer that gives an interface to call
    /// commands used to draw (eg. DrawLine, DrawCircle, etc.)
    GAL*                m_gal;

    /// Instance of object that stores information about how to draw texts (including different
    /// font display modes [bold/italic/mirror]).
    STROKE_FONT*        m_stroke_font;

    /// Colors and display modes settings that are going to be used when drawing items.
    RENDER_SETTINGS*    m_settings;
};
} // namespace KiGfx

#endif /* __CLASS_PAINTER_H */
