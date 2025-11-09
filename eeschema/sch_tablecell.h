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

#ifndef SCH_TABLECELL_H
#define SCH_TABLECELL_H


#include <sch_textbox.h>


class SCH_TABLECELL : public SCH_TEXTBOX
{
public:
    SCH_TABLECELL( int aLineWidth = 0, FILL_T aFillType = FILL_T::NO_FILL );

    static inline bool ClassOf( const EDA_ITEM* aItem ) { return aItem && SCH_TABLECELL_T == aItem->Type(); }

    virtual wxString GetClass() const override { return wxT( "SCH_TABLECELL" ); }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    EDA_ITEM* Clone() const override { return new SCH_TABLECELL( *this ); }

    EDA_GROUP* GetParentGroup() const override { return GetParent()->GetParentGroup(); }

    int GetRow() const;
    int GetColumn() const;

    /// @return the spreadsheet nomenclature for the cell (ie: B3 for 2nd column, 3rd row)
    wxString GetAddr() const;

    wxString GetShownText( const RENDER_SETTINGS* aSettings, const SCH_SHEET_PATH* aPath, bool aAllowExtraText,
                           int aDepth = 0 ) const override;

    wxString GetShownText( bool aAllowExtraText, int aDepth = 0 ) const override
    {
        SCH_SHEET_PATH* sheetPath = nullptr;

        if( SCHEMATIC* schematic = Schematic() )
            sheetPath = &schematic->CurrentSheet();

        return GetShownText( nullptr, sheetPath, aAllowExtraText, aDepth );
    }

    int  GetColSpan() const { return m_colSpan; }
    void SetColSpan( int aSpan ) { m_colSpan = aSpan; }

    int  GetRowSpan() const { return m_rowSpan; }
    void SetRowSpan( int aSpan ) { m_rowSpan = aSpan; }

    int  GetRowHeight() const;
    void SetRowHeight( int aHeight );

    int  GetColumnWidth() const;
    void SetColumnWidth( int aWidth );

    bool IsFilledForHitTesting() const override { return true; }

    void Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts, int aUnit, int aBodyStyle,
               const VECTOR2I& aOffset, bool aDimmed ) override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    double Similarity( const SCH_ITEM& aOther ) const override;

    bool operator==( const SCH_TABLECELL& aOther ) const;
    bool operator==( const SCH_ITEM& aOther ) const override;

protected:
    void swapData( SCH_ITEM* aItem ) override;

    int m_colSpan;
    int m_rowSpan;
};


#endif /* SCH_TABLECELL_H */
