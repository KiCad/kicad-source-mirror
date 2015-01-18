/**
 * @file class_pl_editor_layout.h
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#include <base_struct.h>
#include <class_page_info.h>
#include <class_title_block.h>

class EDA_DRAW_PANEL;


/**
 * Class PL_EDITOR_LAYOUT
 * holds list of GERBER_DRAW_ITEM currently loaded.
 */
class PL_EDITOR_LAYOUT
{
private:
    EDA_RECT                m_BoundingBox;
    PAGE_INFO               m_paper;
    TITLE_BLOCK             m_titles;

public:
    PL_EDITOR_LAYOUT();
    ~PL_EDITOR_LAYOUT();

    const PAGE_INFO&    GetPageSettings() const { return m_paper; }
    void SetPageSettings( const PAGE_INFO& aPageSettings )
    {
        m_paper = aPageSettings;
    }

    const wxPoint&      GetAuxOrigin() const
    {
        static wxPoint zero( 0, 0 );
        return zero;
    }

    const TITLE_BLOCK& GetTitleBlock() const
    {
        return m_titles;
    }

    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock )
    {
        m_titles = aTitleBlock;
    }

    /**
     * Function ComputeBoundingBox
     * calculates the bounding box containing all Gerber items.
     * @return EDA_RECT - the full item list bounding box
     */
    EDA_RECT ComputeBoundingBox();

    /**
     * Function GetBoundingBox
     * may be called soon after ComputeBoundingBox() to return the same EDA_RECT,
     * as long as the CLASS_PL_EDITOR_LAYOUT has not changed.
     */
    const EDA_RECT GetBoundingBox() const { return m_BoundingBox; }    // override

    void SetBoundingBox( const EDA_RECT& aBox ) { m_BoundingBox = aBox; }

#if defined(DEBUG)
    void    Show( int nestLevel, std::ostream& os ) const;  // overload

#endif
};

#endif      // #ifndef CLASS_PL_EDITOR_LAYOUT_H
