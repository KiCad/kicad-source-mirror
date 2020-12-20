/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __SCH_PAINTER_H
#define __SCH_PAINTER_H

#include <sch_component.h>

#include <painter.h>


class LIB_RECTANGLE;
class LIB_PIN;
class LIB_CIRCLE;
class LIB_ITEM;
class LIB_PART;
class LIB_POLYLINE;
class LIB_ARC;
class LIB_FIELD;
class LIB_TEXT;
class LIB_BEZIER;
class SCH_COMPONENT;
class SCH_FIELD;
class SCH_JUNCTION;
class SCH_LABEL;
class SCH_TEXT;
class SCH_HIERLABEL;
class SCH_GLOBALLABEL;
class SCH_SHEET;
class SCH_SHEET_PIN;
class SCH_MARKER;
class SCH_NO_CONNECT;
class SCH_LINE;
class SCH_BUS_ENTRY_BASE;
class SCH_BITMAP;
class SCHEMATIC;

namespace KIGFX
{
class GAL;
class SCH_PAINTER;


/**
 * SCH_RENDER_SETTINGS
 * Stores schematic-specific render settings.
 */

class SCH_RENDER_SETTINGS : public RENDER_SETTINGS
{
public:
    friend class SCH_PAINTER;

    SCH_RENDER_SETTINGS();

    void LoadColors( const COLOR_SETTINGS* aSettings ) override;

    /// @copydoc RENDER_SETTINGS::GetColor()
    virtual COLOR4D GetColor( const VIEW_ITEM* aItem, int aLayer ) const override;

    bool IsBackgroundDark() const override
    {
        auto luma = m_layerColors[ LAYER_SCHEMATIC_BACKGROUND ].GetBrightness();

        return luma < 0.5;
    }

    const COLOR4D& GetBackgroundColor() override
    {
        return m_layerColors[ LAYER_SCHEMATIC_BACKGROUND ];
    }

    void SetBackgroundColor( const COLOR4D& aColor ) override
    {
        m_layerColors[ LAYER_SCHEMATIC_BACKGROUND ] = aColor;
    }

    float GetDanglineSymbolThickness() const
    {
        return (float) m_defaultPenWidth / 3.0F;
    }

    const COLOR4D& GetGridColor() override { return m_layerColors[ LAYER_SCHEMATIC_GRID ]; }

    const COLOR4D& GetCursorColor() override { return m_layerColors[ LAYER_SCHEMATIC_CURSOR ]; }

    int    m_ShowUnit;                // Show all units if 0
    int    m_ShowConvert;             // Show all conversions if 0

    bool   m_ShowHiddenText;
    bool   m_ShowHiddenPins;
    bool   m_ShowPinsElectricalType;
    bool   m_ShowDisabled;
    bool   m_ShowUmbilicals;

    bool   m_OverrideItemColors;

    double m_TextOffsetRatio;        // Proportion of font size to offset text above/below
                                     // wires, buses, etc.

    int    m_DefaultWireThickness;
    int    m_DefaultBusThickness;
    int    m_PinSymbolSize;
    int    m_JunctionSize;
};


/**
 * SCH_PAINTER
 * Contains methods for drawing schematic-specific items.
 */
class SCH_PAINTER : public PAINTER
{
public:
    SCH_PAINTER( GAL* aGal );

    /// @copydoc PAINTER::Draw()
    virtual bool Draw( const VIEW_ITEM*, int ) override;

    /// @copydoc PAINTER::ApplySettings()
    virtual void ApplySettings( const RENDER_SETTINGS* aSettings ) override
    {
        m_schSettings = *static_cast<const SCH_RENDER_SETTINGS*>( aSettings );
    }

    /// @copydoc PAINTER::GetAdapter()
    virtual SCH_RENDER_SETTINGS* GetSettings() override
    {
        return &m_schSettings;
    }

    void SetSchematic( SCHEMATIC* aSchematic )
    {
        m_schematic = aSchematic;
    }

private:
    void draw( const LIB_RECTANGLE* aRect, int aLayer );
    void draw( LIB_PIN* aPin, int aLayer );
    void draw( const LIB_CIRCLE* aCircle, int aLayer );
    void draw( const LIB_PART* aPart, int, bool aDrawFields = true, int aUnit = 0, int aConvert = 0 );
    void draw( const LIB_ARC* aArc, int aLayer );
    void draw( const LIB_POLYLINE* aLine, int aLayer );
    void draw( const LIB_FIELD* aField, int aLayer );
    void draw( const LIB_TEXT* aText, int aLayer );
    void draw( const LIB_BEZIER* aCurve, int aLayer );
    void draw( SCH_COMPONENT* aSymbol, int aLayer );
    void draw( const SCH_JUNCTION* aJct, int aLayer );
    void draw( const SCH_FIELD* aField, int aLayer );
    void draw( const SCH_TEXT* aText, int aLayer );
    void draw( SCH_HIERLABEL* aLabel, int aLayer );
    void draw( SCH_GLOBALLABEL* aLabel, int aLayer );
    void draw( const SCH_SHEET* aSheet, int aLayer );
    void draw( const SCH_NO_CONNECT* aNC, int aLayer );
    void draw( const SCH_MARKER* aMarker, int aLayer );
    void draw( const SCH_BITMAP* aBitmap, int aLayer );
    void draw( const SCH_LINE* aLine, int aLayer );
    void draw( const SCH_BUS_ENTRY_BASE* aEntry, int aLayer );

    void drawPinDanglingSymbol( const VECTOR2I& aPos, bool aDrawingShadows );
    void drawDanglingSymbol( const wxPoint& aPos, int aWidth, bool aDrawingShadows );

    int internalPinDecoSize( const LIB_PIN &aPin );
    int externalPinDecoSize( const LIB_PIN &aPin );

    bool isUnitAndConversionShown( const LIB_ITEM* aItem ) const;

    float getShadowWidth() const;
    COLOR4D getRenderColor( const EDA_ITEM* aItem, int aLayer, bool aDrawingShadows ) const;
    float getLineWidth( const LIB_ITEM* aItem, bool aDrawingShadows ) const;
    float getLineWidth( const SCH_ITEM* aItem, bool aDrawingShadows ) const;
    float getTextThickness( const SCH_TEXT* aItem, bool aDrawingShadows ) const;
    float getTextThickness( const SCH_FIELD* aItem, bool aDrawingShadows ) const;
    float getTextThickness( const LIB_FIELD* aItem, bool aDrawingShadows ) const;
    float getTextThickness( const LIB_TEXT* aItem, bool aDrawingShadows ) const;

    bool setDeviceColors( const LIB_ITEM* aItem, int aLayer );
    void fillIfSelection( int aLayer );

    void triLine ( const VECTOR2D &a, const VECTOR2D &b, const VECTOR2D &c );
    void strokeText( const wxString& aText, const VECTOR2D& aPosition, double aRotationAngle );

    SCH_RENDER_SETTINGS m_schSettings;

    SCHEMATIC* m_schematic;
};

}; // namespace KIGFX


#endif
