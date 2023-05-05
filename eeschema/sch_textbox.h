/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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


class HTML_MESSAGE_BOX;

class SCH_TEXTBOX : public SCH_SHAPE, public EDA_TEXT
{
public:
    SCH_TEXTBOX( int aLineWidth = 0, FILL_T aFillType = FILL_T::NO_FILL,
                 const wxString& aText = wxEmptyString );

    SCH_TEXTBOX( const SCH_TEXTBOX& aText );

    ~SCH_TEXTBOX() { }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && SCH_TEXTBOX_T == aItem->Type();
    }

    virtual wxString GetClass() const override
    {
        return wxT( "SCH_TEXTBOX" );
    }

    int GetTextMargin() const;

    VECTOR2I GetDrawPos() const override;

    virtual wxString GetShownText( const SCH_SHEET_PATH* aPath, bool aAllowExtraText,
                                   int aDepth = 0 ) const;

    wxString GetShownText( bool aAllowExtraText, int aDepth = 0 ) const override
    {
        return GetShownText( nullptr, aAllowExtraText, aDepth );
    }

    bool IsHypertext() const override
    {
        return HasHyperlink();
    }

    void DoHypertextAction( EDA_DRAW_FRAME* aFrame ) const override;

    void SetExcludeFromSim( bool aExclude ) override { m_excludeFromSim = aExclude; }
    bool GetExcludeFromSim() const override { return m_excludeFromSim; }

    void Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& offset ) override;

    void SwapData( SCH_ITEM* aItem ) override;

    bool operator<( const SCH_ITEM& aItem ) const override;

    void Move( const VECTOR2I& aMoveVector ) override
    {
        EDA_SHAPE::move( aMoveVector );
        EDA_TEXT::Offset( aMoveVector );
    }

    void MirrorHorizontally( int aCenter ) override;
    void MirrorVertically( int aCenter ) override;
    void Rotate( const VECTOR2I& aCenter ) override;

    virtual void Rotate90( bool aClockwise );

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;

    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override
    {
        return SCH_ITEM::Matches( GetText(), aSearchData );
    }

    bool Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) override
    {
        return EDA_TEXT::Replace( aSearchData );
    }

    virtual bool IsReplaceable() const override { return true; }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override;

    BITMAPS GetMenuImage() const override;

    void Plot( PLOTTER* aPlotter, bool aBackground ) const override;

    EDA_ITEM* Clone() const override
    {
        return new SCH_TEXTBOX( *this );
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

protected:
    KIFONT::FONT* getDrawFont() const override;

protected:
    bool m_excludeFromSim;
};


#endif /* SCH_TEXTBOX_H */
