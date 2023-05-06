/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 CERN
 * Copyright (C) 2020-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#include <sch_symbol.h>

#include <painter.h>


class LIB_PIN;
class LIB_SHAPE;
class LIB_ITEM;
class LIB_SYMBOL;
class LIB_FIELD;
class LIB_TEXT;
class LIB_TEXTBOX;
class SCH_SYMBOL;
class SCH_FIELD;
class SCH_JUNCTION;
class SCH_LABEL;
class SCH_TEXT;
class SCH_TEXTBOX;
class SCH_HIERLABEL;
class SCH_DIRECTIVE_LABEL;
class SCH_GLOBALLABEL;
class SCH_SHEET;
class SCH_SHEET_PIN;
class SCH_SHAPE;
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
 * Store schematic specific render settings.
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

    const COLOR4D& GetBackgroundColor() const override
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

    bool GetShowPageLimits() const override;

public:
    bool   m_IsSymbolEditor;

    int    m_ShowUnit;               // Show all units if 0
    int    m_ShowConvert;            // Show all conversions if 0

    bool   m_ShowPinsElectricalType;
    bool   m_ShowPinNumbers;         // Force showing of pin numbers (normally symbol-specific)
    bool   m_ShowDisabled;
    bool   m_ShowGraphicsDisabled;

    bool   m_OverrideItemColors;

    double m_LabelSizeRatio;         // Proportion of font size to label box
    double m_TextOffsetRatio;        // Proportion of font size to offset text above/below
                                     // wires, buses, etc.
    int    m_PinSymbolSize;
};


/**
 * Contains methods for drawing schematic-specific items.
 */
class SCH_PAINTER : public PAINTER
{
public:
    SCH_PAINTER( GAL* aGal );

    /// @copydoc PAINTER::Draw()
    virtual bool Draw( const VIEW_ITEM*, int ) override;

    /// @copydoc PAINTER::GetSettings()
    virtual SCH_RENDER_SETTINGS* GetSettings() override { return &m_schSettings; }

    void SetSchematic( SCHEMATIC* aSchematic ) { m_schematic = aSchematic; }

private:
    void draw( const EDA_ITEM*, int, bool aDimmed );
    void draw( const LIB_PIN* aPin, int aLayer, bool aDimmed );
    void draw( const LIB_SHAPE* aCircle, int aLayer, bool aDimmed );
    void draw( const LIB_SYMBOL* aSymbol, int, bool aDrawFields = true, int aUnit = 0,
               int aConvert = 0, bool aDimmed = false );
    void draw( const LIB_FIELD* aField, int aLayer, bool aDimmed );
    void draw( const LIB_TEXT* aText, int aLayer, bool aDimmed );
    void draw( const LIB_TEXTBOX* aTextBox, int aLayer, bool aDimmed );
    void draw( const SCH_SYMBOL* aSymbol, int aLayer );
    void draw( const SCH_JUNCTION* aJct, int aLayer );
    void draw( const SCH_FIELD* aField, int aLayer, bool aDimmed );
    void draw( const SCH_SHAPE* aShape, int aLayer );
    void draw( const SCH_TEXTBOX* aTextBox, int aLayer );
    void draw( const SCH_TEXT* aText, int aLayer );
    void draw( const SCH_LABEL* aText, int aLayer );
    void draw( const SCH_DIRECTIVE_LABEL* aLabel, int aLayer );
    void draw( const SCH_HIERLABEL* aLabel, int aLayer );
    void draw( const SCH_GLOBALLABEL* aLabel, int aLayer );
    void draw( const SCH_SHEET* aSheet, int aLayer );
    void draw( const SCH_NO_CONNECT* aNC, int aLayer );
    void draw( const SCH_MARKER* aMarker, int aLayer );
    void draw( const SCH_BITMAP* aBitmap, int aLayer );
    void draw( const SCH_LINE* aLine, int aLayer );
    void draw( const SCH_BUS_ENTRY_BASE* aEntry, int aLayer );

    void drawPinDanglingSymbol( const VECTOR2I& aPos, const COLOR4D& aColor,
                                bool aDrawingShadows, bool aBrightened );
    void drawDanglingSymbol( const VECTOR2I& aPos, const COLOR4D& aColor, int aWidth,
                             bool aDangling, bool aDrawingShadows, bool aBrightened );

    int internalPinDecoSize( const LIB_PIN &aPin );
    int externalPinDecoSize( const LIB_PIN &aPin );

    // Indicates the item is drawn on a non-cached layer in OpenGL
    bool nonCached( const EDA_ITEM* aItem );

    bool isUnitAndConversionShown( const LIB_ITEM* aItem ) const;

    float getShadowWidth( bool aForHighlight ) const;
    COLOR4D getRenderColor( const EDA_ITEM* aItem, int aLayer, bool aDrawingShadows,
                            bool aDimmed = false ) const;
    KIFONT::FONT* getFont( const EDA_TEXT* aText ) const;
    float getLineWidth( const EDA_ITEM* aItem, bool aDrawingShadows ) const;
    float getTextThickness( const EDA_ITEM* aItem ) const;

    int getOperatingPointTextSize() const;

    bool setDeviceColors( const LIB_ITEM* aItem, int aLayer, bool aDimmed );

    void triLine( const VECTOR2D &a, const VECTOR2D &b, const VECTOR2D &c );
    void strokeText( const wxString& aText, const VECTOR2D& aPosition,
                     const TEXT_ATTRIBUTES& aAttributes );
    void bitmapText( const wxString& aText, const VECTOR2D& aPosition,
                     const TEXT_ATTRIBUTES& aAttributes );
    void knockoutText( const wxString& aText, const VECTOR2D& aPosition,
                       const TEXT_ATTRIBUTES& aAttrs );
    void boxText( const wxString& aText, const VECTOR2D& aPosition,
                  const TEXT_ATTRIBUTES& aAttrs );

    wxString expandLibItemTextVars( const wxString& aSourceText, const SCH_SYMBOL* aSymbolContext );

public:
    static std::vector<KICAD_T> g_ScaledSelectionTypes;

private:
    SCH_RENDER_SETTINGS m_schSettings;
    SCHEMATIC*          m_schematic;
};

}; // namespace KIGFX


#endif // __SCH_PAINTER_H
