/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 Kicad Developers, see AUTHORS.txt for contributors.
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

#ifndef WS_PROXY_UNDO_ITEM_H
#define WS_PROXY_UNDO_ITEM_H

#include <base_struct.h>
#include <title_block.h>
#include <page_info.h>


class WS_PROXY_UNDO_ITEM : public EDA_ITEM
{
public:
    WS_PROXY_UNDO_ITEM( const EDA_DRAW_FRAME* aFrame );

    /*
     * Restores the saved worksheet layout to the global worksheet record, and the saved
     * page info and title blocks to the given frame.  The WS_DRAW_ITEMs are rehydrated
     * and installed in aView if it is not null (ie: if we're in the PageLayout Editor).
     */
    void Restore( EDA_DRAW_FRAME* aFrame, KIGFX::VIEW* aView = nullptr );

#if defined(DEBUG)
    /// @copydoc EDA_ITEM::Show()
    void Show( int x, std::ostream& st ) const override { }
#endif

    wxString GetClass() const override
    {
        return wxT( "WS_PROXY_UNDO_ITEM" );
    }

protected:
    TITLE_BLOCK m_titleBlock;
    PAGE_INFO   m_pageInfo;
    wxString    m_layoutSerialization;
    int         m_selectedDataItem;
    int         m_selectedDrawItem;
};

#endif /* WS_PROXY_UNDO_ITEM_H */
