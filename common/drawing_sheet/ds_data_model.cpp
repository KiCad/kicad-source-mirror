/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Jean-Pierre Charras <jp.charras at wanadoo.fr>.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * The WS_DATA_ITEM_* classes define the basic shapes of a drawing sheet (frame references
 * and title block).  The list of these items is stored in a DS_DATA_MODEL instance.
 *
 * These items cannot be drawn or plotted "as is".  They must be converted to WS_DRAW_*
 * types. When building the draw list:
 *   - the DS_DATA_MODEL is used to create a DS_DRAW_ITEM_LIST
 *   - coordinates are converted to draw/plot coordinates.
 *   - texts are expanded if they contain format symbols.
 *   - items with m_RepeatCount > 1 are created m_RepeatCount times.
 *
 * The DS_DATA_MODEL is created only once.
 * The WS_DRAW_ITEM_*s are created and maintained by the PlEditor, but are created each time
 * they're needed for drawing by the clients (Eeschema, Pcbnew, etc.)
 *
 * The DS_DATA_MODEL instance is created from a S expression which describes the drawing sheet
 * (can be the default drawing sheet or a custom file).  This format is also used for undo/redo
 * storage (wrapped in a DS_PROXY_UNDO_ITEM).
 */

#include <kiface_base.h>
#include <title_block.h>
#include <common.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <drawing_sheet/ds_painter.h>


// The layout shape used in the application
// It is accessible by DS_DATA_MODEL::GetTheInstance()
static DS_DATA_MODEL wksTheInstance;
static DS_DATA_MODEL* wksAltInstance = nullptr;

DS_DATA_MODEL::DS_DATA_MODEL() :
        m_WSunits2Iu( 1000.0 ),
        m_DefaultLineWidth( 0.0 ),
        m_DefaultTextSize( TB_DEFAULT_TEXTSIZE, TB_DEFAULT_TEXTSIZE ),
        m_DefaultTextThickness( 0.0 ),
        m_EditMode( false )
{
    m_allowVoidList = false;
    m_fileFormatVersionAtLoad = 0;
    m_leftMargin = 10.0;    // the left page margin in mm
    m_rightMargin = 10.0;   // the right page margin in mm
    m_topMargin = 10.0;     // the top page margin in mm
    m_bottomMargin = 10.0;  // the bottom page margin in mm
}


DS_DATA_MODEL& DS_DATA_MODEL::GetTheInstance()
{
    if( wksAltInstance )
        return *wksAltInstance;
    else
        return wksTheInstance;
}


void DS_DATA_MODEL::SetAltInstance( DS_DATA_MODEL* aLayout )
{
    wksAltInstance = aLayout;
}


void DS_DATA_MODEL::SetupDrawEnvironment( const PAGE_INFO& aPageInfo, double aMilsToIU )
{
#define MILS_TO_MM ( 25.4 / 1000 )

    m_WSunits2Iu = aMilsToIU / MILS_TO_MM;

    // Left top corner position
    VECTOR2D lt_corner;
    lt_corner.x = GetLeftMargin();
    lt_corner.y = GetTopMargin();
    m_LT_Corner = lt_corner;

    // Right bottom corner position
    VECTOR2D rb_corner;
    rb_corner.x = ( aPageInfo.GetSizeMils().x * MILS_TO_MM ) - GetRightMargin();
    rb_corner.y = ( aPageInfo.GetSizeMils().y * MILS_TO_MM ) - GetBottomMargin();
    m_RB_Corner = rb_corner;
}


void DS_DATA_MODEL::ClearList()
{
    for( DS_DATA_ITEM* item : m_list )
        delete item;

    m_list.clear();
}


void DS_DATA_MODEL::Append( DS_DATA_ITEM* aItem )
{
    m_list.push_back( aItem );
}


void DS_DATA_MODEL::Remove( DS_DATA_ITEM* aItem )
{
    auto newEnd = std::remove( m_list.begin(), m_list.end(), aItem );
    m_list.erase( newEnd, m_list.end() );
}


DS_DATA_ITEM* DS_DATA_MODEL::GetItem( unsigned aIdx ) const
{
    if( aIdx < m_list.size() )
        return m_list[aIdx];
    else
        return nullptr;
}

