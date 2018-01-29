/**
 * @file design_tree_frame.cpp
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

#include <wx/imaglist.h>
#include <wx/wupdlock.h>
#include <fctsys.h>
#include <worksheet_shape_builder.h>
#include <worksheet_dataitem.h>
#include <pl_editor_id.h>
#include <design_tree_frame.h>

/* XPM
 * This bitmap is used to show item types
 */
static const char*  root_xpm[] =
{
    "12 12 2 1",
    "  c None",
    "x c #008080",
    "   xxxx     ",
    "     xxx    ",
    "      xxx   ",
    "       xxx  ",
    "xxxxxxxxxxx ",
    "xxxxxxxxxxxx",
    "xxxxxxxxxxx ",
    "       xxx  ",
    "      xxx   ",
    "     xxx    ",
    "   xxxx     ",
    "            "
};

static const char*  line_xpm[] =
{
    "12 12 2 1",
    "  c None",
    "x c #008080",
    "xx          ",
    "xx          ",
    "xx          ",
    "xx          ",
    "xx          ",
    "xx          ",
    "xx          ",
    "xx          ",
    "xx          ",
    "xx          ",
    "xxxxxxxxxxxx",
    "xxxxxxxxxxxx"
};

static const char*  rect_xpm[] =
{
    "12 12 2 1",
    "  c None",
    "x c #000080",
    "xxxxxxxxxxxx",
    "xxxxxxxxxxxx",
    "xx        xx",
    "xx        xx",
    "xx        xx",
    "xx        xx",
    "xx        xx",
    "xx        xx",
    "xx        xx",
    "xx        xx",
    "xxxxxxxxxxxx",
    "xxxxxxxxxxxx"
};

static const char*  text_xpm[] =
{
    "12 12 2 1",
    "  c None",
    "x c #800000",
    " xxxxxxxxxx ",
    "xxxxxxxxxxxx",
    "xx   xx   xx",
    "     xx     ",
    "     xx     ",
    "     xx     ",
    "     xx     ",
    "     xx     ",
    "     xx     ",
    "     xx     ",
    "    xxxx    ",
    "   xxxxxx   "
};

static const char*  poly_xpm[] =
{
    "12 12 2 1",
    "  c None",
    "x c #008000",
    "     xx     ",
    "    xxxx    ",
    "   xxxxxx   ",
    "  xxxxxxxx  ",
    " xxxxxxxxxx ",
    "xxxxxxxxxxxx",
    "xxxxxxxxxxxx",
    " xxxxxxxxxx ",
    "  xxxxxxxx  ",
    "   xxxxxx   ",
    "    xxxx    ",
    "     xx     "
};

static const char*  img_xpm[] =
{
    "12 12 2 1",
    "  c None",
    "x c #800000",
    "     xx     ",
    "   xxxxxx   ",
    " xx      xx ",
    "xx        xx",
    "xx        xx",
    " xx      xx ",
    "   xxxxxx   ",
    "     xx     ",
    "     xx     ",
    "     xx     ",
    "     xx     ",
    "     xx     "
};

// Event table:


DESIGN_TREE_FRAME::DESIGN_TREE_FRAME( PL_EDITOR_FRAME* aParent ) :
    wxTreeCtrl( aParent, ID_DESIGN_TREE_FRAME )
{
    // icons size is not know (depending on they are built)
    // so get it:
    wxSize      iconsize;
    wxBitmap    root_bm( root_xpm );

    iconsize.x  = root_bm.GetWidth();
    iconsize.y  = root_bm.GetHeight();

    // Make an image list containing small icons
    m_imageList = new wxImageList( iconsize.x, iconsize.y, true, 6 );

    m_imageList->Add( root_bm );                    // root symbol
    m_imageList->Add( wxBitmap( line_xpm ) );       // line item
    m_imageList->Add( wxBitmap( rect_xpm ) );       // rect item
    m_imageList->Add( wxBitmap( text_xpm ) );       // text item
    m_imageList->Add( wxBitmap( poly_xpm ) );       // poly item
    m_imageList->Add( wxBitmap( img_xpm ) );        // bitmap item

    SetImageList( m_imageList );
}


DESIGN_TREE_FRAME::~DESIGN_TREE_FRAME()
{
    delete m_imageList;
}


wxSize DESIGN_TREE_FRAME::GetMinSize() const
{
    return wxSize( 100, -1 );
}


void DESIGN_TREE_FRAME::ReCreateDesignTree()
{
    wxWindowUpdateLocker dummy(this);   // Avoid flicker when rebuilding the tree

    DeleteAllItems();

    const WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();

    // root tree:
    wxFileName      fn( ((PL_EDITOR_FRAME*) GetParent())->GetCurrFileName() );
    wxTreeItemId    rootitem;

    if( fn.GetName().IsEmpty() )
        rootitem = AddRoot( wxT( "<default>" ), 0, 0 );
    else
        rootitem = AddRoot( fn.GetName(), 0, 0 );

    SetItemBold( rootitem, true );

    // Now adding all current items
    for( unsigned ii = 0; ii < pglayout.GetCount(); ii++ )
    {
        WORKSHEET_DATAITEM* item = pglayout.GetItem( ii );
        int img = 0;
        switch( item->GetType() )
        {
            case WORKSHEET_DATAITEM::WS_SEGMENT: img = 1; break;
            case WORKSHEET_DATAITEM::WS_RECT: img = 2; break;
            case WORKSHEET_DATAITEM::WS_TEXT: img = 3; break;
            case WORKSHEET_DATAITEM::WS_POLYPOLYGON: img = 4; break;
            case WORKSHEET_DATAITEM::WS_BITMAP: img = 5; break;
        }
        wxTreeItemId cell= AppendItem( rootitem, item->m_Name, img, img );
        DESIGN_TREE_ITEM_DATA* data = new DESIGN_TREE_ITEM_DATA( item );
        SetItemData( cell, data );
    }

    Expand( rootitem );
}


// Select the tree item corresponding to the WORKSHEET_DATAITEM aItem
void DESIGN_TREE_FRAME::SelectCell( WORKSHEET_DATAITEM* aItem )
{
    wxTreeItemId        rootcell = GetRootItem();
    wxTreeItemIdValue   cookie;

    wxTreeItemId        cell = GetFirstChild( rootcell, cookie );

    while( cell.IsOk() )
    {
        DESIGN_TREE_ITEM_DATA* data = (DESIGN_TREE_ITEM_DATA*) GetItemData( cell );

        if( data->GetItem() == aItem )
        {
            SelectItem( cell );
            return;
        }

        cell = GetNextChild( rootcell, cookie );
    }
}

//return the page layout item managed by the cell
WORKSHEET_DATAITEM* DESIGN_TREE_FRAME::GetPageLayoutItem( wxTreeItemId aCell ) const
{
    DESIGN_TREE_ITEM_DATA* data = (DESIGN_TREE_ITEM_DATA*) GetItemData( aCell );
    if( data )
        return data->GetItem();
    else
        return NULL;
}

/* return the page layout item managed by the selected cell (or NULL)
 */
WORKSHEET_DATAITEM* DESIGN_TREE_FRAME::GetPageLayoutSelectedItem() const
{
    wxTreeItemId cell;
    cell = GetSelection();

    if( cell.IsOk() )
        return GetPageLayoutItem( cell );

    return NULL;
}

/* return the page layout item index managed by the selected cell (or -1)
 */
int DESIGN_TREE_FRAME::GetSelectedItemIndex()
{
    WORKSHEET_DATAITEM*item = GetPageLayoutSelectedItem();

    if( item == NULL )
        return -1;

    WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
    return pglayout.GetItemIndex( item );
}
