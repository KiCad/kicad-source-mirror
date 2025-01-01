/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef CLASS_PL_EDITOR_LAYOUT_H
#define CLASS_PL_EDITOR_LAYOUT_H

#include <page_info.h>
#include <title_block.h>
#include <drawing_sheet/ds_draw_item.h>


class PL_EDITOR_LAYOUT
{
public:
    PL_EDITOR_LAYOUT();
    ~PL_EDITOR_LAYOUT();

    PAGE_INFO& GetPageSettings() { return m_paper; }
    const PAGE_INFO& GetPageSettings() const { return m_paper; }
    void SetPageSettings( const PAGE_INFO& aPageSettings ) { m_paper = aPageSettings; }

    const wxPoint& GetAuxOrigin() const
    {
        static wxPoint zero( 0, 0 );
        return zero;
    }

    TITLE_BLOCK& GetTitleBlock() { return m_titles; }
    const TITLE_BLOCK& GetTitleBlock() const { return m_titles; }
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock ) { m_titles = aTitleBlock; }

    DS_DRAW_ITEM_LIST& GetDrawItems()
    {
        return m_drawItemList;
    }

    /**
     * Calculate the bounding box containing all Gerber items.
     *
     * @return the full item list bounding box.
     */
    BOX2I ComputeBoundingBox();

    /**
     * Called soon after ComputeBoundingBox() to return the same BOX2I, as long as the
     * CLASS_PL_EDITOR_LAYOUT has not changed.
     */
    const BOX2I GetBoundingBox() const { return m_boundingBox; }

    void SetBoundingBox( const BOX2I& aBox ) { m_boundingBox = aBox; }

#if defined(DEBUG)
    void    Show( int nestLevel, std::ostream& os ) const;
#endif

private:
    BOX2I                m_boundingBox;
    PAGE_INFO            m_paper;
    TITLE_BLOCK          m_titles;

    DS_DRAW_ITEM_LIST    m_drawItemList;
};

#endif      // #ifndef CLASS_PL_EDITOR_LAYOUT_H
