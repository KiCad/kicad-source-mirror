/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_field.h>
#include <sch_connection.h>   // for CONNECTION_TYPE
#include <schematic.h>


class HTML_MESSAGE_BOX;

class SCH_TEXT : public SCH_ITEM, public EDA_TEXT
{
public:
    SCH_TEXT( const VECTOR2I& aPos = { 0, 0 }, const wxString& aText = wxEmptyString,
              KICAD_T aType = SCH_TEXT_T );

    SCH_TEXT( const SCH_TEXT& aText );

    ~SCH_TEXT() { }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_TEXT_T == aItem->Type();
    }

    virtual wxString GetClass() const override
    {
        return wxT( "SCH_TEXT" );
    }

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

    bool IsHypertext() const override
    {
        return HasHyperlink();
    }

    void DoHypertextAction( EDA_DRAW_FRAME* aFrame ) const override;

    void SetExcludedFromSim( bool aExclude ) override { m_excludedFromSim = aExclude; }
    bool GetExcludedFromSim() const override { return m_excludedFromSim; }

    /**
     * This offset depends on the orientation, the type of text, and the area required to
     * draw the associated graphic symbol or to put the text above a wire.
     *
     * @return the offset between the SCH_TEXT position and the text itself position
     */
    virtual VECTOR2I GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const;

    void Print( const SCH_RENDER_SETTINGS* aSettings, const VECTOR2I& offset ) override;

    void SwapData( SCH_ITEM* aItem ) override;

    const BOX2I GetBoundingBox() const override;

    bool operator<( const SCH_ITEM& aItem ) const override;

    int GetTextOffset( const RENDER_SETTINGS* aSettings = nullptr ) const;

    int GetPenWidth() const override;

    void Move( const VECTOR2I& aMoveVector ) override
    {
        EDA_TEXT::Offset( aMoveVector );
    }

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW ) override;

    virtual void Rotate90( bool aClockwise );
    virtual void MirrorSpinStyle( bool aLeftRight );

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override
    {
        return SCH_ITEM::Matches( GetText(), aSearchData );
    }

    bool Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) override
    {
        return EDA_TEXT::Replace( aSearchData );
    }

    virtual bool IsReplaceable() const override { return true; }

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override;

    BITMAPS GetMenuImage() const override;

    VECTOR2I GetPosition() const override { return EDA_TEXT::GetTextPos(); }
    void     SetPosition( const VECTOR2I& aPosition ) override { EDA_TEXT::SetTextPos( aPosition ); }

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;
    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    void Plot( PLOTTER* aPlotter, bool aBackground,
               const SCH_PLOT_SETTINGS& aPlotSettings ) const override;

    EDA_ITEM* Clone() const override
    {
        return new SCH_TEXT( *this );
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    virtual double Similarity( const SCH_ITEM& aItem ) const override;

    virtual bool operator==( const SCH_ITEM& aItem ) const override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    static HTML_MESSAGE_BOX* ShowSyntaxHelp( wxWindow* aParentWindow );

protected:
    KIFONT::FONT* getDrawFont() const override;

    const KIFONT::METRICS& getFontMetrics() const override { return GetFontMetrics(); }

protected:
    bool            m_excludedFromSim;
};


#endif /* SCH_TEXT_H */
