/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <base_screen.h>
#include <eda_item.h>
#include <trace_helpers.h>


wxString BASE_SCREEN::m_DrawingSheetFileName;   // the name of the drawing sheet file.


BASE_SCREEN::BASE_SCREEN( EDA_ITEM* aParent, KICAD_T aType ) :
    EDA_ITEM( aParent, aType )
{
    m_virtualPageNumber = 1;
    m_pageCount         = 1;         // Hierarchy: Root: ScreenNumber = 1
    m_Center            = true;

    m_flagModified      = false;     // Set when any change is made on board.
}


void BASE_SCREEN::InitDataPoints( const VECTOR2I& aPageSizeIU )
{
    if( m_Center )
    {
        m_DrawOrg.x = -aPageSizeIU.x / 2;
        m_DrawOrg.y = -aPageSizeIU.y / 2;
    }
    else
    {
        m_DrawOrg.x = 0;
        m_DrawOrg.y = 0;
    }

    m_LocalOrigin = { 0, 0 };
}


void BASE_SCREEN::SetPageCount( int aPageCount )
{
    if( aPageCount > 0 )
        m_pageCount = aPageCount;
}


const wxString& BASE_SCREEN::GetPageNumber() const
{
    static wxString pageNumber;

    if( m_pageNumber.IsEmpty() )
        pageNumber.Printf( wxS( "%d" ), m_virtualPageNumber );
    else
        pageNumber = m_pageNumber;

    return pageNumber;
}


#if defined(DEBUG)

void BASE_SCREEN::Show( int nestLevel, std::ostream& os ) const
{
    // for now, make it look like XML, expand on this later.
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << ">\n";

    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}

#endif
