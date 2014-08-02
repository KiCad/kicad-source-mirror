/**
 * @file class_worksheet_layout.cpp
 * @brief description of graphic items and texts to build a title block
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
 * Copyright (C) 1992-2013 KiCad Developers, see change_log.txt for contributors.
 *
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


/*
 * the class WORKSHEET_DATAITEM (and derived ) defines
 * a basic shape of a page layout ( frame references and title block )
 * The list of these items is stored in a WORKSHEET_LAYOUT instance.
 *
 *
 * These items cannot be drawn or plot "as this". they should be converted
 * to a "draw list". When building the draw list:
 * the WORKSHEET_LAYOUT is used to create a WS_DRAW_ITEM_LIST
 *  coordinates are converted to draw/plot coordinates.
 *  texts are expanded if they contain format symbols.
 *  Items with m_RepeatCount > 1 are created m_RepeatCount times
 *
 * the WORKSHEET_LAYOUT is created only once.
 * the WS_DRAW_ITEM_LIST is created each time the page layout is plot/drawn
 *
 * the WORKSHEET_LAYOUT instance is created from a S expression which
 * describes the page layout (can be the default page layout or a custom file).
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <drawtxt.h>
#include <worksheet.h>
#include <class_title_block.h>
#include <worksheet_shape_builder.h>
#include <class_worksheet_dataitem.h>


// The layout shape used in the application
// It is accessible by WORKSHEET_LAYOUT::GetTheInstance()
WORKSHEET_LAYOUT wksTheInstance;

WORKSHEET_LAYOUT::WORKSHEET_LAYOUT()
{
    m_allowVoidList = false;
    m_leftMargin = 10.0;    // the left page margin in mm
    m_rightMargin = 10.0;   // the right page margin in mm
    m_topMargin = 10.0;     // the top page margin in mm
    m_bottomMargin = 10.0;  // the bottom page margin in mm
}


void WORKSHEET_LAYOUT::SetLeftMargin( double aMargin )
{
    m_leftMargin = aMargin;    // the left page margin in mm
}


void WORKSHEET_LAYOUT::SetRightMargin( double aMargin )
{
    m_rightMargin = aMargin;   // the right page margin in mm
}


void WORKSHEET_LAYOUT::SetTopMargin( double aMargin )
{
    m_topMargin = aMargin;     // the top page margin in mm
}


void WORKSHEET_LAYOUT::SetBottomMargin( double aMargin )
{
    m_bottomMargin = aMargin;  // the bottom page margin in mm
}


void WORKSHEET_LAYOUT::ClearList()
{
    for( unsigned ii = 0; ii < m_list.size(); ii++ )
        delete m_list[ii];
    m_list.clear();
}


void WORKSHEET_LAYOUT::Insert( WORKSHEET_DATAITEM* aItem, unsigned aIdx )
{
    if ( aIdx >= GetCount() )
        Append( aItem );
    else
        m_list.insert(  m_list.begin() + aIdx, aItem );
}


bool WORKSHEET_LAYOUT::Remove( unsigned aIdx )
{
    if ( aIdx >= GetCount() )
        return false;
    m_list.erase( m_list.begin() + aIdx );
    return true;
}


bool WORKSHEET_LAYOUT::Remove( WORKSHEET_DATAITEM* aItem )
{
    unsigned idx = 0;

    while( idx < m_list.size() )
    {
        if( m_list[idx] == aItem )
            break;

        idx++;
    }

    return Remove( idx );
}


int WORKSHEET_LAYOUT::GetItemIndex( WORKSHEET_DATAITEM* aItem ) const
{
    unsigned idx = 0;
    while( idx < m_list.size() )
    {
        if( m_list[idx] == aItem )
            return (int) idx;

        idx++;
    }

    return -1;
}

/* return the item from its index aIdx, or NULL if does not exist
 */
WORKSHEET_DATAITEM* WORKSHEET_LAYOUT::GetItem( unsigned aIdx ) const
{
    if( aIdx < m_list.size() )
        return m_list[aIdx];
    else
        return NULL;
}


const wxString WORKSHEET_LAYOUT::MakeShortFileName( const wxString& aFullFileName )
{
    wxFileName  fn = aFullFileName;
    wxString    shortFileName = aFullFileName;
    wxString    fileName = Kiface().KifaceSearch().FindValidPath( fn.GetFullName() );

    if( !fileName.IsEmpty() )
    {
        fn = fileName;
        shortFileName = fn.GetFullName();
        return shortFileName;
    }

    return shortFileName;
}


const wxString WORKSHEET_LAYOUT::MakeFullFileName( const wxString& aShortFileName )
{
    wxFileName  fn = aShortFileName;
    wxString    fullFileName = aShortFileName;

    if( fn.GetPath().IsEmpty() && !fn.GetFullName().IsEmpty() )
    {
        wxString name = Kiface().KifaceSearch().FindValidPath( fn.GetFullName() );
        if( !name.IsEmpty() )
            fullFileName = name;
    }

    return fullFileName;
}
