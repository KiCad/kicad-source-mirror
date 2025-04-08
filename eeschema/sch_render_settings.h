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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SCH_RENDER_SETTINGS_H
#define SCH_RENDER_SETTINGS_H

#include <gal/color4d.h>
#include <render_settings.h>
#include <transform.h>


using KIGFX::COLOR4D;


class SCH_RENDER_SETTINGS : public KIGFX::RENDER_SETTINGS
{
public:
    SCH_RENDER_SETTINGS();

    void LoadColors( const COLOR_SETTINGS* aSettings ) override;

    virtual COLOR4D GetColor( const KIGFX::VIEW_ITEM* aItem, int aLayer ) const override
    {
        auto it = m_layerColors.find( aLayer );
        return it == m_layerColors.end() ? COLOR4D::WHITE : it->second;
    }

    bool IsBackgroundDark() const override
    {
        auto it = m_layerColors.find( LAYER_SCHEMATIC_BACKGROUND );
        return it != m_layerColors.end() && it->second.GetBrightness() < 0.5;
    }

    const KIGFX::COLOR4D& GetBackgroundColor() const override
    {
        auto it = m_layerColors.find( LAYER_SCHEMATIC_BACKGROUND );
        return it == m_layerColors.end() ? COLOR4D::BLACK : it->second;
    }

    void SetBackgroundColor( const COLOR4D& aColor ) override
    {
        m_layerColors[ LAYER_SCHEMATIC_BACKGROUND ] = aColor;
    }

    float GetDanglingIndicatorThickness() const
    {
        return (float) m_defaultPenWidth / 3.0F;
    }

    const COLOR4D& GetGridColor() override { return m_layerColors[ LAYER_SCHEMATIC_GRID ]; }
    const COLOR4D& GetCursorColor() override { return m_layerColors[ LAYER_SCHEMATIC_CURSOR ]; }

    bool GetShowPageLimits() const override;

    VECTOR2I TransformCoordinate( const VECTOR2I& aPoint ) const
    {
        return m_Transform.TransformCoordinate( aPoint );
    }

public:
    bool   m_IsSymbolEditor;

    int    m_ShowUnit;               // Show all units if 0
    int    m_ShowBodyStyle;          // Show all body styles if 0

    bool   m_ShowPinsElectricalType;
    bool   m_ShowHiddenPins;
    bool   m_ShowHiddenFields;
    bool   m_ShowVisibleFields;
    bool   m_ShowPinNumbers;         // Force showing of pin numbers (normally symbol-specific)
    bool   m_ShowPinNames;           // Force showing of pin names (normally symbol-specific)
    bool   m_ShowPinAltIcons;
    bool   m_ShowDisabled;
    bool   m_ShowGraphicsDisabled;
    bool   m_ShowConnectionPoints;

    bool   m_OverrideItemColors;

    double m_LabelSizeRatio;         // Proportion of font size to label box
    double m_TextOffsetRatio;        // Proportion of font size to offset text above/below
                                     // wires, buses, etc.
    int    m_PinSymbolSize;
    int    m_SymbolLineWidth;        ///< Override line widths for symbol drawing objects set to default line width.

    TRANSFORM m_Transform;
};


#endif /* SCH_RENDER_SETTINGS_H */
