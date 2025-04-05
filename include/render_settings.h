/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef RENDER_SETTINGS_H
#define RENDER_SETTINGS_H

#include <map>
#include <set>

#include <gal/color4d.h>
#include <layer_ids.h>
#include <lset.h>
#include <memory>

#include <wx/dc.h>

class COLOR_SETTINGS;

namespace KIGFX
{
class VIEW_ITEM;

/**
 * Container for all the knowledge about how graphical objects are drawn on any output
 * surface/device.
 *
 * This includes:
 *  - color/transparency settings
 *  - highlighting and high contrast mode control
 *  - drawing quality control (sketch/outline mode)
 *  - text processing flags
 *
 * The class acts as an interface between the PAINTER object and the GUI (i.e. Layers/Items
 * widget or display options dialog).
 */
class RENDER_SETTINGS
{
public:
    RENDER_SETTINGS();
    virtual ~RENDER_SETTINGS();

    virtual void LoadColors( const COLOR_SETTINGS* aSettings ) { }

    /**
     * Set the specified layer as high-contrast.
     *
     * @param aLayerId is a layer number that should be displayed in a specific mode.
     * @param aEnabled is the new layer state ( true = active or false = not active).
     */
    inline void SetLayerIsHighContrast( int aLayerId, bool aEnabled = true )
    {
        if( aEnabled )
            m_highContrastLayers.insert( aLayerId );
        else
            m_highContrastLayers.erase( aLayerId );
    }

    /**
     * Return information whether the queried layer is marked as high-contrast.
     *
     * @return True if the queried layer is marked as active.
     */
    inline bool GetLayerIsHighContrast( int aLayerId ) const
    {
        return ( m_highContrastLayers.count( aLayerId ) > 0 );
    }

    /**
     * Returns the set of currently high-contrast layers.
     */
    const std::set<int> GetHighContrastLayers() const
    {
        return m_highContrastLayers;
    }

    /**
     * Return the board layer which is in high-contrast mode.
     *
     * There should only be one board layer which is high-contrast at any given time, although
     * there might be many high-contrast synthetic (GAL) layers.
     */
    PCB_LAYER_ID GetPrimaryHighContrastLayer() const
    {
        for( int layer : m_highContrastLayers )
        {
            if( layer >= PCBNEW_LAYER_ID_START && layer < PCB_LAYER_ID_COUNT )
                return (PCB_LAYER_ID) layer;
        }

        return UNDEFINED_LAYER;
    }

    PCB_LAYER_ID GetActiveLayer() const { return m_activeLayer; }
    void SetActiveLayer( PCB_LAYER_ID aLayer ) { m_activeLayer = aLayer; }

    const wxString& GetLayerName() const { return m_layerName; }
    void SetLayerName( const wxString& aLayerName ) { m_layerName = aLayerName; }

    LSET GetPrintLayers() const { return m_printLayers; }
    void SetPrintLayers( const LSET& aLayerSet ) { m_printLayers = aLayerSet; }

    /**
     * Clear the list of active layers.
     */
    inline void ClearHighContrastLayers()
    {
        m_highContrastLayers.clear();
    }

    /**
     * Return current highlight setting.
     *
     * @return True if highlight is enabled, false otherwise.
     */
    inline bool IsHighlightEnabled() const
    {
        return m_highlightEnabled;
    }

    /**
     * Return the netcode of currently highlighted net.
     *
     * @return Netcode of currently highlighted net.
     */
    inline const std::set<int>& GetHighlightNetCodes() const
    {
        return m_highlightNetcodes;
    }

    /**
     * Turns on/off highlighting.
     *
     * It may be done for the active layer or the specified net(s)..
     *
     * @param aEnabled tells if highlighting should be enabled.
     * @param aNetcode is optional and if specified, turns on highlighting only for the net with
     *                 number given as the parameter.
     */
    inline void SetHighlight( bool aEnabled, int aNetcode = -1, bool aMulti = false )
    {
        m_highlightEnabled = aEnabled;

        if( aEnabled )
        {
            if( !aMulti )
                m_highlightNetcodes.clear();

            m_highlightNetcodes.insert( aNetcode );
        }
        else
            m_highlightNetcodes.clear();
    }

    /**
     * Turns on highlighting and highlights multiple nets
     * @param aHighlight is a set of netcodes to highlight
     * @param aEnabled tells if highlighting should be enabled.
     */
    inline void SetHighlight( std::set<int>& aHighlight, bool aEnabled = true )
    {
        m_highlightEnabled  = aEnabled;

        if( aEnabled )
            m_highlightNetcodes = aHighlight;
        else
            m_highlightNetcodes.clear();
    }

    /**
     * Turns on/off high contrast display mode.
     */
    void SetHighContrast( bool aEnabled ) { m_hiContrastEnabled = aEnabled; }
    bool GetHighContrast() const { return m_hiContrastEnabled; }

    void SetDrawBoundingBoxes( bool aEnabled ) { m_drawBoundingBoxes = aEnabled; }
    bool GetDrawBoundingBoxes() const { return m_drawBoundingBoxes; }

    /**
     * Returns the color that should be used to draw the specific VIEW_ITEM on the specific layer
     * using currently used render settings.
     *
     * @param aItem is the VIEW_ITEM.
     * @param aLayer is the layer.
     * @return The color.
     */
    virtual COLOR4D GetColor( const VIEW_ITEM* aItem, int aLayer ) const = 0;

    float GetDrawingSheetLineWidth() const { return m_drawingSheetLineWidth; }

    int GetDefaultPenWidth() const { return m_defaultPenWidth; }
    void SetDefaultPenWidth( int aWidth ) { m_defaultPenWidth = aWidth; }

    int GetMinPenWidth() const { return m_minPenWidth; }
    void SetMinPenWidth( int aWidth ) { m_minPenWidth = aWidth; }

    double GetDashLengthRatio() const { return m_dashLengthRatio; }
    void SetDashLengthRatio( double aRatio ) { m_dashLengthRatio = aRatio; }
    double GetDashLength( int aLineWidth ) const;
    double GetDotLength( int aLineWidth ) const;

    double GetGapLengthRatio() const { return m_gapLengthRatio; }
    void SetGapLengthRatio( double aRatio ) { m_gapLengthRatio = aRatio; }
    double GetGapLength( int aLineWidth ) const;

    virtual bool GetShowPageLimits() const { return true; }

    bool IsPrinting() const { return m_isPrinting; }
    void SetIsPrinting( bool isPrinting ) { m_isPrinting = isPrinting; }

    bool IsPrintBlackAndWhite() const { return m_printBlackAndWite; }
    void SetPrintBlackAndWhite( bool aPrintBlackAndWhite )
    {
        m_printBlackAndWite = aPrintBlackAndWhite;
    }

    bool PrintBlackAndWhiteReq() const
    {
        return m_printBlackAndWite && m_isPrinting;
    }

    /**
     * Return current background color settings.
     */
    virtual const COLOR4D& GetBackgroundColor() const = 0;

    /**
     * Set the background color.
     */
    virtual void SetBackgroundColor( const COLOR4D& aColor ) = 0;

    /**
     * Return current grid color settings.
     */
    virtual const COLOR4D& GetGridColor() = 0;

    /**
     * Return current cursor color settings.
     */
    virtual const COLOR4D& GetCursorColor() = 0;

    /**
     * Return the color used to draw a layer.
     *
     * @param aLayer is the layer number.
     */
    inline const COLOR4D& GetLayerColor( int aLayer ) const
    {
        // We don't (yet?) have a separate color for intersheet refs
        if( aLayer == LAYER_INTERSHEET_REFS )
            aLayer = LAYER_GLOBLABEL;

        return m_layerColors.count( aLayer ) ? m_layerColors.at( aLayer ) : COLOR4D::BLACK;
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

    virtual bool IsBackgroundDark() const
    {
        return false;
    }

    /**
     * Set line width used for drawing outlines.
     *
     * @param aWidth is the new width.
     */
    void SetOutlineWidth( float aWidth ) { m_outlineWidth = aWidth; }
    float GetOutlineWidth() const { return m_outlineWidth; }

    void SetHighlightFactor( float aFactor ) { m_highlightFactor = aFactor; }
    void SetSelectFactor( float aFactor ) { m_selectFactor = aFactor; }

    void SetDefaultFont( const wxString& aFont ) { m_defaultFont = aFont; }
    const wxString& GetDefaultFont() const { return m_defaultFont; }

    // TODO: these can go away once the drawing sheet is moved to Cairo-based printing
    wxDC* GetPrintDC() const { return m_printDC; }
    void SetPrintDC( wxDC* aDC ) { m_printDC = aDC; }

protected:
    /**
     * Precalculates extra colors for layers (e.g. highlighted, darkened and any needed version
     * of base colors).
     */
    virtual void update();

    PCB_LAYER_ID           m_activeLayer;        // The active layer (as shown by appearance mgr)
    wxString               m_layerName;
    std::set<int>          m_highContrastLayers; // High-contrast layers (both board layers and
                                                 //   synthetic GAL layers)
    std::map<int, COLOR4D> m_layerColors;        // Layer colors
    std::map<int, COLOR4D> m_layerColorsHi;      // Layer colors for highlighted objects
    std::map<int, COLOR4D> m_layerColorsSel;     // Layer colors for selected objects
    std::map<int, COLOR4D> m_hiContrastColor;    // High-contrast mode layer colors
    std::map<int, COLOR4D> m_layerColorsDark;    // Darkened layer colors (for high-contrast mode)

    COLOR4D m_backgroundColor;                   // The background color

    /// Parameters for display modes
    bool          m_hiContrastEnabled;    // High contrast display mode on/off
    float         m_hiContrastFactor;     // Factor used for computing high contrast color

    bool          m_highlightEnabled;     // Highlight display mode on/off
    std::set<int> m_highlightNetcodes;    // Set of net cods to be highlighted
    float         m_highlightFactor;      // Factor used for computing highlight color

    bool          m_drawBoundingBoxes;    // Visual aid for debugging

    float         m_selectFactor;         // Specifies how color of selected items is changed
    float         m_outlineWidth;         // Line width used when drawing outlines
    float         m_drawingSheetLineWidth;// Line width used for borders and titleblock

    int           m_defaultPenWidth;
    int           m_minPenWidth;          // Some clients (such as PDF) don't like ultra-thin
                                          // lines.  This sets an absolute minimum.
    double        m_dashLengthRatio;
    double        m_gapLengthRatio;

    wxString      m_defaultFont;

    bool          m_isPrinting;           // true when draw to a printer
    bool          m_printBlackAndWite;    // true if black and white printing is requested: some
                                          // backgrounds are not printed to avoid not visible items
    LSET          m_printLayers;

    wxDC*         m_printDC;              // This can go away once the drawing sheet is moved to
                                          // Cairo-based printing.
};

}

#endif /* RENDER_SETTINGS_H */
