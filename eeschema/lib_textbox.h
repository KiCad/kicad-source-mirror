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

#ifndef LIB_TEXTBOX_H
#define LIB_TEXTBOX_H


#include <eda_text.h>
#include <lib_shape.h>


class HTML_MESSAGE_BOX;

class LIB_TEXTBOX : public LIB_SHAPE, public EDA_TEXT
{
public:
    LIB_TEXTBOX( LIB_SYMBOL* aParent, int aLineWidth = 0, FILL_T aFillType = FILL_T::NO_FILL,
                 const wxString& aText = wxEmptyString );

    LIB_TEXTBOX( const LIB_TEXTBOX& aText );

    ~LIB_TEXTBOX() { }

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && aItem->Type() == LIB_TEXTBOX_T;
    }

    virtual wxString GetClass() const override
    {
        return wxT( "LIB_TEXTBOX" );
    }

    int GetTextMargin() const;

    VECTOR2I GetDrawPos() const override;

    wxString GetShownText( bool aAllowExtraText, int aDepth = 0 ) const override;

    void MirrorHorizontally( const VECTOR2I& center );
    void MirrorVertically( const VECTOR2I& center );
    void Rotate( const VECTOR2I& aCenter, bool aRotateCCW ) override;

    bool HitTest( const VECTOR2I& aPosition, int aAccuracy = 0 ) const override;

    bool HitTest( const BOX2I& aRect, bool aContained, int aAccuracy = 0 ) const override;

    bool Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const override
    {
        return LIB_ITEM::Matches( GetText(), aSearchData );
    }

    bool Replace( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) override
    {
        return EDA_TEXT::Replace( aSearchData );
    }

    virtual bool IsReplaceable() const override { return true; }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const override;

    BITMAPS GetMenuImage() const override;

    void Plot( PLOTTER* aPlotter, bool aBackground, const VECTOR2I& offset,
               const TRANSFORM& aTransform, bool aDimmed ) const override;

    EDA_ITEM* Clone() const override
    {
        return new LIB_TEXTBOX( *this );
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    void ViewGetLayers( int aLayers[], int& aCount ) const override;

protected:
        KIFONT::FONT* getDrawFont() const override;

private:
    int compare( const LIB_ITEM& aOther, int aCompareFlags = 0 ) const override;

    void print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset, void* aData,
                const TRANSFORM& aTransform, bool aDimmed ) override;
};


#endif /* LIB_TEXTBOX_H */
