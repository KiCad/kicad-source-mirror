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


class HTML_MESSAGE_BOX;

/*
 * Spin style for text items of all kinds on schematics
 * Basically a higher level abstraction of rotation and justification of text
 */
class TEXT_SPIN_STYLE
{
public:
    enum SPIN : int
    {
        LEFT   = 0,
        UP     = 1,
        RIGHT  = 2,
        BOTTOM = 3
    };


    TEXT_SPIN_STYLE() = default;
    constexpr TEXT_SPIN_STYLE( SPIN aSpin ) : m_spin( aSpin )
    {
    }

    constexpr bool operator==( SPIN a ) const
    {
        return m_spin == a;
    }

    constexpr bool operator!=( SPIN a ) const
    {
        return m_spin != a;
    }

    operator int() const
    {
        return static_cast<int>( m_spin );
    }

    TEXT_SPIN_STYLE RotateCW();

    TEXT_SPIN_STYLE RotateCCW();

    /**
     * Mirror the label spin style across the X axis or simply swaps up and bottom.
     */
    TEXT_SPIN_STYLE MirrorX();

    /**
     * Mirror the label spin style across the Y axis or simply swaps left and right.
     */
    TEXT_SPIN_STYLE MirrorY();

private:
    SPIN m_spin;
};


/*
 * Label and flag shapes used with text objects.
 */
enum LABEL_FLAG_SHAPE
{
    L_INPUT,
    L_OUTPUT,
    L_BIDI,
    L_TRISTATE,
    L_UNSPECIFIED,

    F_FIRST,
    F_DOT = F_FIRST,
    F_ROUND,
    F_DIAMOND,
    F_RECTANGLE
};


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
        return GetShownText( nullptr, aAllowExtraText, aDepth );
    }

    bool IsHypertext() const override
    {
        return HasHyperlink();
    }

    void DoHypertextAction( EDA_DRAW_FRAME* aFrame ) const override;

    void SetExcludeFromSim( bool aExclude ) override { m_excludeFromSim = aExclude; }
    bool GetExcludeFromSim() const override { return m_excludeFromSim; }

    /**
     * Set a spin or rotation angle, along with specific horizontal and vertical justification
     * styles with each angle.
     *
     * @param aSpinStyle Spin style as per #TEXT_SPIN_STYLE storage class, may be the enum
     *                   values or int value
     */
    virtual void    SetTextSpinStyle( TEXT_SPIN_STYLE aSpinStyle );
    TEXT_SPIN_STYLE GetTextSpinStyle() const         { return m_spin_style; }

    virtual LABEL_FLAG_SHAPE GetShape() const        { return L_UNSPECIFIED; }
    virtual void SetShape( LABEL_FLAG_SHAPE aShape ) { }

    /**
     * This offset depends on the orientation, the type of text, and the area required to
     * draw the associated graphic symbol or to put the text above a wire.
     *
     * @return the offset between the SCH_TEXT position and the text itself position
     */
    virtual VECTOR2I GetSchematicTextOffset( const RENDER_SETTINGS* aSettings ) const;

    void Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& offset ) override;

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
    void Rotate( const VECTOR2I& aCenter ) override;

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

    void Plot( PLOTTER* aPlotter, bool aBackground ) const override;

    EDA_ITEM* Clone() const override
    {
        return new SCH_TEXT( *this );
    }

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override;
#endif

    static HTML_MESSAGE_BOX* ShowSyntaxHelp( wxWindow* aParentWindow );

protected:
    KIFONT::FONT* getDrawFont() const override;

protected:
    /**
     * The orientation of text and any associated drawing elements of derived objects.
     *  - 0 is the horizontal and left justified.
     *  - 1 is vertical and top justified.
     *  - 2 is horizontal and right justified.  It is the equivalent of the mirrored 0 orientation.
     *  - 3 is vertical and bottom justified. It is the equivalent of the mirrored 1 orientation.
     *
     * This is a duplication of m_Orient, m_HJustified, and m_VJustified in #EDA_TEXT but is
     * easier to handle than 3 parameters when editing and reading and saving files.
     */
    TEXT_SPIN_STYLE m_spin_style;

    bool            m_excludeFromSim;
};


#endif /* SCH_TEXT_H */
