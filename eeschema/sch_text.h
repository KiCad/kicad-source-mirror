/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef SCH_TEXT_H
#define SCH_TEXT_H


#include <eda_text.h>
#include <sch_item.h>
#include <sch_connection.h>   // for CONNECTION_TYPE
#include <schematic.h>


class HTML_MESSAGE_BOX;

class SCH_TEXT : public SCH_ITEM, public EDA_TEXT
{
public:
    SCH_TEXT( const VECTOR2I& aPos = { 0, 0 }, const wxString& aText = wxEmptyString,
              SCH_LAYER_ID aLayer = LAYER_NOTES, KICAD_T aType = SCH_TEXT_T );

    SCH_TEXT( const SCH_TEXT& aText );

    ~SCH_TEXT() override { }

    static bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_TEXT_T == aItem->Type();
    }

    wxString GetClass() const override
    {
        return wxT( "SCH_TEXT" );
    }

    wxString GetFriendlyName() const override
    {
        return _( "Text" );
    }

    KIFONT::FONT* GetDrawFont( const RENDER_SETTINGS* aSettings ) const override;

    virtual wxString GetShownText( const SCH_SHEET_PATH* aPath, bool aAllowExtraText,
                                   int aDepth = 0 ) const;

    wxString GetShownText( bool aAllowExtraText, int aDepth = 0 ) const override
    {
        SCHEMATIC* schematic = Schematic();

        if( schematic )
            return GetShownText( &schematic->CurrentSheet(), aAllowExtraText, aDepth );
        else
            return GetText();
    }

    int GetSchTextSize() const { return GetTextWidth(); }
    void SetSchTextSize( int aSize ) { SetTextSize( VECTOR2I( aSize, aSize ) ); }

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

    /**
     * This offset depends on the orientation, the type of text, and the area required to
     * draw the associated graphic symbol or to put the text above a wire.
     *
     * @return the offset between the SCH_TEXT position and the text itself position
     */
    virtual VECTOR2I GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const;

    const BOX2I GetBoundingBox() const override;

    bool operator<( const SCH_ITEM& aItem ) const override;

    int GetTextOffset( const RENDER_SETTINGS* aSettings = nullptr ) const;

    int GetPenWidth() const override;

    void Move( const VECTOR2I& aMoveVector ) override
    {
        EDA_TEXT::Offset( aMoveVector );
    }

    void NormalizeJustification( bool inverse );

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW ) override;

    virtual void Rotate90( bool aClockwise );
    virtual void MirrorSpinStyle( bool aLeftRight );

    void BeginEdit( const VECTOR2I& aStartPoint ) override;
    void CalcEdit( const VECTOR2I& aPosition ) override;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override
    {
        return SCH_ITEM::Matches( GetText(), aSearchData );
    }

    bool Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) override
    {
        return EDA_TEXT::Replace( aSearchData );
    }

    bool IsReplaceable() const override { return true; }

    std::vector<int> ViewGetLayers() const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    BITMAPS GetMenuImage() const override;

    VECTOR2I GetPosition() const override { return EDA_TEXT::GetTextPos(); }
    void     SetPosition( const VECTOR2I& aPosition ) override
    {
        EDA_TEXT::SetTextPos( aPosition );
    }

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;
    bool HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const override;

    void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
               int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed ) override;

    EDA_ITEM* Clone() const override
    {
        return new SCH_TEXT( *this );
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    double Similarity( const SCH_ITEM& aItem ) const override;

    bool operator==( const SCH_ITEM& aItem ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    static HTML_MESSAGE_BOX* ShowSyntaxHelp( wxWindow* aParentWindow );

protected:
    void swapData( SCH_ITEM* aItem ) override;

    const KIFONT::METRICS& getFontMetrics() const override { return GetFontMetrics(); }

    /**
     * @copydoc SCH_ITEM::compare()
     *
     * The text specific sort order is as follows:
     *      - Text string, case insensitive compare.
     *      - Text horizontal (X) position.
     *      - Text vertical (Y) position.
     *      - Text width.
     *      - Text height.
     */
    int compare( const SCH_ITEM& aOther, int aCompareFlags = 0 ) const override;

protected:
    bool            m_excludedFromSim;
};


#endif /* SCH_TEXT_H */
