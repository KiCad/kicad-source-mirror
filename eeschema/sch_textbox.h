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

#ifndef SCH_TEXTBOX_H
#define SCH_TEXTBOX_H


#include <eda_text.h>
#include <sch_shape.h>
#include <schematic.h>


class HTML_MESSAGE_BOX;

class SCH_TEXTBOX : public SCH_SHAPE, public EDA_TEXT
{
public:
    SCH_TEXTBOX( SCH_LAYER_ID aLayer = LAYER_NOTES, int aLineWidth = 0,
                 FILL_T aFillType = FILL_T::NO_FILL, const wxString& aText = wxEmptyString,
                 KICAD_T aType = SCH_TEXTBOX_T );

    SCH_TEXTBOX( const SCH_TEXTBOX& aText );

    ~SCH_TEXTBOX() { }

    static bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_TEXTBOX_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_TEXTBOX" );
    }

    int GetLegacyTextMargin() const;

    void SetMarginLeft( int aLeft )     { m_marginLeft = aLeft; }
    void SetMarginTop( int aTop )       { m_marginTop = aTop; }
    void SetMarginRight( int aRight )   { m_marginRight = aRight; }
    void SetMarginBottom( int aBottom ) { m_marginBottom = aBottom; }

    int GetMarginLeft() const           { return m_marginLeft; }
    int GetMarginTop() const            { return m_marginTop; }
    int GetMarginRight() const          { return m_marginRight; }
    int GetMarginBottom() const         { return m_marginBottom; }

    int GetSchTextSize() const { return GetTextWidth(); }
    void SetSchTextSize( int aSize ) { SetTextSize( VECTOR2I( aSize, aSize ) ); }

    VECTOR2I GetDrawPos() const override;

    KIFONT::FONT* GetDrawFont( const RENDER_SETTINGS* aSettings ) const override;

    virtual wxString GetShownText( const RENDER_SETTINGS* aSettings, const SCH_SHEET_PATH* aPath,
                                   bool aAllowExtraText, int aDepth = 0 ) const;

    wxString GetShownText( bool aAllowExtraText, int aDepth = 0 ) const override
    {
        SCH_SHEET_PATH* sheetPath = nullptr;

        if( SCHEMATIC* schematic = Schematic() )
            sheetPath = &schematic->CurrentSheet();

        return GetShownText( nullptr, sheetPath, aAllowExtraText, aDepth );
    }

    bool HasHypertext() const override;
    bool HasHoveredHypertext() const override;
    void DoHypertextAction( EDA_DRAW_FRAME* aFrame, const VECTOR2I& aMousePos ) const override;

    void SetExcludedFromSim( bool aExclude, const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString& aVariantName = wxEmptyString ) override
    {
        m_excludedFromSim = aExclude;
    }

    bool GetExcludedFromSim( const SCH_SHEET_PATH* aInstance = nullptr,
                             const wxString& aVariantName = wxEmptyString ) const override
    {
        return m_excludedFromSim;
    }

    bool operator<( const SCH_ITEM& aItem ) const override;

    void Move( const VECTOR2I& aMoveVector ) override
    {
        EDA_SHAPE::move( aMoveVector );
        EDA_TEXT::Offset( aMoveVector );
    }

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW ) override;

    virtual void Rotate90( bool aClockwise );

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;

    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;
    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override
    {
        return SCH_ITEM::Matches( GetText(), aSearchData );
    }

    bool Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) override
    {
        return EDA_TEXT::Replace( aSearchData );
    }

    virtual bool IsReplaceable() const override { return true; }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    BITMAPS GetMenuImage() const override;

    void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
               int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    EDA_ITEM* Clone() const override
    {
        return new SCH_TEXTBOX( *this );
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    double Similarity( const SCH_ITEM& aOther ) const override;

    bool operator==( const SCH_ITEM& aOther ) const override;

protected:
    void swapData( SCH_ITEM* aItem ) override;

    const KIFONT::METRICS& getFontMetrics() const override { return GetFontMetrics(); }

    int compare( const SCH_ITEM& aOther, int aCompareFlags = 0 ) const override;

protected:
    bool m_excludedFromSim;
    int  m_marginLeft;
    int  m_marginTop;
    int  m_marginRight;
    int  m_marginBottom;
};


#endif /* SCH_TEXTBOX_H */
