/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 CERN
 * Copyright (C) 2020-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_render_settings.h>
#include <sch_symbol.h>

#include <gal/painter.h>


class SCH_PIN;
class LIB_SYMBOL;
class SCH_SYMBOL;
class SCH_FIELD;
class SCH_JUNCTION;
class SCH_LABEL;
class SCH_TEXT;
class SCH_TEXTBOX;
class SCH_TABLE;
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
    void drawItemBoundingBox( const EDA_ITEM* aItem );
    void draw( const EDA_ITEM*, int, bool aDimmed );
    void draw( const SCH_PIN* aPin, int aLayer, bool aDimmed );
    void draw( const LIB_SYMBOL* aSymbol, int, bool aDrawFields = true, int aUnit = 0,
               int aBodyStyle = 0, bool aDimmed = false );
    void draw( const SCH_SYMBOL* aSymbol, int aLayer );
    void draw( const SCH_SHAPE* aShape, int aLayer, bool aDimmed );
    void draw( const SCH_JUNCTION* aJct, int aLayer );
    void draw( const SCH_FIELD* aField, int aLayer, bool aDimmed );
    void draw( const SCH_TEXTBOX* aTextBox, int aLayer, bool aDimmed );
    void draw( const SCH_TEXT* aText, int aLayer, bool aDimmed );
    void draw( const SCH_TABLE* aTable, int aLayer, bool aDimmed );
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

    void drawPinDanglingIndicator( const VECTOR2I& aPos, const COLOR4D& aColor,
                                   bool aDrawingShadows, bool aBrightened );
    void drawDanglingIndicator( const VECTOR2I& aPos, const COLOR4D& aColor, int aWidth,
                                bool aDangling, bool aDrawingShadows, bool aBrightened );

    int internalPinDecoSize( const SCH_PIN &aPin );
    int externalPinDecoSize( const SCH_PIN &aPin );

    // Indicates the item is drawn on a non-cached layer in OpenGL
    bool nonCached( const EDA_ITEM* aItem );

    bool isUnitAndConversionShown( const SCH_ITEM* aItem ) const;

    float getShadowWidth( bool aForHighlight ) const;
    COLOR4D getRenderColor( const SCH_ITEM* aItem, int aLayer, bool aDrawingShadows,
                            bool aDimmed = false ) const;
    KIFONT::FONT* getFont( const EDA_TEXT* aText ) const;
    float getLineWidth( const SCH_ITEM* aItem, bool aDrawingShadows ) const;
    float getTextThickness( const SCH_ITEM* aItem ) const;

    int getOperatingPointTextSize() const;

    bool setDeviceColors( const SCH_ITEM* aItem, int aLayer, bool aDimmed );

    void triLine( const VECTOR2D &a, const VECTOR2D &b, const VECTOR2D &c );
    void strokeText( const wxString& aText, const VECTOR2D& aPosition,
                     const TEXT_ATTRIBUTES& aAttributes, const KIFONT::METRICS& aFontMetrics );
    void bitmapText( const wxString& aText, const VECTOR2D& aPosition,
                     const TEXT_ATTRIBUTES& aAttributes );
    void knockoutText( const wxString& aText, const VECTOR2D& aPosition,
                       const TEXT_ATTRIBUTES& aAttrs, const KIFONT::METRICS& aFontMetrics );
    void boxText( const wxString& aText, const VECTOR2D& aPosition,
                  const TEXT_ATTRIBUTES& aAttrs, const KIFONT::METRICS& aFontMetrics,
                  bool aInvertY );

    wxString expandLibItemTextVars( const wxString& aSourceText, const SCH_SYMBOL* aSymbolContext );

public:
    static std::vector<KICAD_T> g_ScaledSelectionTypes;

private:
    SCH_RENDER_SETTINGS m_schSettings;
    SCHEMATIC*          m_schematic;
};

}; // namespace KIGFX


#endif // __SCH_PAINTER_H
