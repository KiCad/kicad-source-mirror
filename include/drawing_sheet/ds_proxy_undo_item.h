/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DS_PROXY_UNDO_ITEM_H
#define DS_PROXY_UNDO_ITEM_H

#include <eda_item.h>
#include <title_block.h>
#include <page_info.h>


class DS_PROXY_UNDO_ITEM : public EDA_ITEM
{
public:
    DS_PROXY_UNDO_ITEM( const EDA_DRAW_FRAME* aFrame );

    /*
     * Restores the saved drawing sheet layout to the global drawing sheet record, and the saved
     * page info and title blocks to the given frame.  The WS_DRAW_ITEMs are rehydrated and
     * installed in aView if it is not null (ie: if we're in the PageLayout Editor).
     */
    void Restore( EDA_DRAW_FRAME* aFrame, KIGFX::VIEW* aView = nullptr );

#if defined(DEBUG)
    /// @copydoc EDA_ITEM::Show()
    void Show( int x, std::ostream& st ) const override { }
#endif

    wxString GetClass() const override
    {
        return wxT( "DS_PROXY_UNDO_ITEM" );
    }

protected:
    TITLE_BLOCK m_titleBlock;
    PAGE_INFO   m_pageInfo;
    wxString    m_layoutSerialization;
    int         m_selectedDataItem;
    int         m_selectedDrawItem;
};

#endif /* DS_PROXY_UNDO_ITEM_H */
