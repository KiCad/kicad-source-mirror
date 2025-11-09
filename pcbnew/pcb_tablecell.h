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

#ifndef PCB_TABLECELL_H
#define PCB_TABLECELL_H


#include <pcb_textbox.h>
#include <board_item_container.h>


class PCB_TABLECELL : public PCB_TEXTBOX
{
public:
    PCB_TABLECELL( BOARD_ITEM* parent );

    static inline bool ClassOf( const EDA_ITEM* aItem ) { return aItem && PCB_TABLECELL_T == aItem->Type(); }

    wxString GetClass() const override { return wxT( "PCB_TABLECELL" ); }

    virtual wxString GetFriendlyName() const override { return _( "Table Cell" ); }

    EDA_ITEM* Clone() const override { return new PCB_TABLECELL( *this ); }

    EDA_GROUP* GetParentGroup() const override { return GetParent()->GetParentGroup(); }

    int GetRow() const;
    int GetColumn() const;

    // @return the spreadsheet nomenclature for the cell (ie: B3 for 2nd column, 3rd row)
    wxString GetAddr() const;

    wxString GetShownText( bool aAllowExtraText, int aDepth = 0 ) const override;

    int  GetColSpan() const { return m_colSpan; }
    void SetColSpan( int aSpan ) { m_colSpan = aSpan; }

    int  GetRowSpan() const { return m_rowSpan; }
    void SetRowSpan( int aSpan ) { m_rowSpan = aSpan; }

    int  GetRowHeight() const;
    void SetRowHeight( int aHeight );

    int  GetColumnWidth() const;
    void SetColumnWidth( int aWidth );

    bool IsFilledForHitTesting() const override { return true; }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    double Similarity( const BOARD_ITEM& aBoardItem ) const override;

    bool operator==( const PCB_TABLECELL& aBoardItem ) const;
    bool operator==( const BOARD_ITEM& aBoardItem ) const override;

protected:
    virtual void swapData( BOARD_ITEM* aImage ) override;

protected:
    int m_colSpan;
    int m_rowSpan;
};


#endif /* PCB_TABLECELL_H */
