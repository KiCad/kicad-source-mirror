/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file design_inspector.cpp
 */

#include <wx/dc.h>
#include <wx/imaglist.h>
#include <wx/wupdlock.h>

#include <eda_item.h>
#include <drawing_sheet/ds_draw_item.h>
#include <drawing_sheet/ds_data_item.h>
#include <drawing_sheet/ds_data_model.h>
#include <tool/tool_manager.h>

#include "dialogs/dialog_design_inspector_base.h"
#include "design_inspector.h"
#include "pl_editor_frame.h"
#include "properties_frame.h"
#include "tools/pl_actions.h"
#include "tools/pl_selection_tool.h"


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


// A helper class to draw these bitmaps into a wxGrid cell:
class BitmapGridCellRenderer : public wxGridCellStringRenderer
{
    const char** m_BitmapXPM;
public:
    BitmapGridCellRenderer( const char** aBitmapXPM )
    {
        m_BitmapXPM = aBitmapXPM;
    }

    void Draw( wxGrid& aGrid, wxGridCellAttr& aAttr,
               wxDC& aDc, const wxRect& aRect,
               int aRow, int aCol, bool aIsSelected) override;
};


// Column ids for m_gridListItems
enum COL_INDEX
{
    COL_BITMAP,
    COL_TYPENAME,
    COL_REPEAT_NUMBER,
    COL_COMMENT,
    COL_TEXTSTRING,
    COL_COUNT       //Sentinel
};


DIALOG_INSPECTOR::DIALOG_INSPECTOR( PL_EDITOR_FRAME* aParent ) :
    DIALOG_INSPECTOR_BASE( aParent )
{
    m_editorFrame = aParent;
    ReCreateDesignList();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_INSPECTOR::~DIALOG_INSPECTOR()
{
}



void DIALOG_INSPECTOR::ReCreateDesignList()
{
    wxWindowUpdateLocker dummy( this );   // Avoid flicker when rebuilding the tree
    const PAGE_INFO& page_info = m_editorFrame->GetPageLayout().GetPageSettings();

    if( m_gridListItems->GetNumberRows() > 1 )
        m_gridListItems->DeleteRows( 1, m_gridListItems->GetNumberRows() - 1 );

    m_itemsList.clear();

    DS_DATA_MODEL& drawingSheet = DS_DATA_MODEL::GetTheInstance();
    wxFileName     fn( static_cast<PL_EDITOR_FRAME*>( GetParent() )->GetCurrentFileName() );

    if( fn.GetName().IsEmpty() )
        SetTitle( "<default drawing sheet>" );
    else
        SetTitle( fn.GetName() );

    // The first item is the layout: Display info about the page: fmt, size...
    int row = 0;
    GetGridList()->SetCellValue( row, COL_TYPENAME, _( "Layout" ) );

    // Display page format name.
    GetGridList()->SetCellValue( row, COL_COMMENT, page_info.GetTypeAsString() );
    GetGridList()->SetCellValue( row, COL_REPEAT_NUMBER, "-" );
    VECTOR2I page_sizeIU = m_editorFrame->GetPageSizeIU();
    GetGridList()->SetCellValue( row, COL_TEXTSTRING,
                                 wxString::Format( _( "Size: %.1fx%.1fmm" ),
                                                   drawSheetIUScale.IUTomm( page_sizeIU.x ),
                                                   drawSheetIUScale.IUTomm( page_sizeIU.y ) ) );
    GetGridList()->SetCellRenderer (row, COL_BITMAP, new BitmapGridCellRenderer( root_xpm ) );
    GetGridList()->SetReadOnly( row, COL_BITMAP );
    m_itemsList.push_back( nullptr );   // this item is not a DS_DATA_ITEM, just a pseudo item

    // Now adding all current items
    row++;

    for( DS_DATA_ITEM* item : drawingSheet.GetItems() )
    {
        const char** img = nullptr;

        switch( item->GetType() )
        {
        case DS_DATA_ITEM::DS_SEGMENT:
            img = line_xpm;
            break;

        case DS_DATA_ITEM::DS_RECT:
            img = rect_xpm;
            break;

        case DS_DATA_ITEM::DS_TEXT:
            img = text_xpm;
            break;

        case DS_DATA_ITEM::DS_POLYPOLYGON:
            img = poly_xpm;
            break;

        case DS_DATA_ITEM::DS_BITMAP:
            img = img_xpm;
            break;
        }

        GetGridList()->AppendRows( 1 );
        GetGridList()->SetCellRenderer (row, COL_BITMAP, new BitmapGridCellRenderer( img ) );
        GetGridList()->SetReadOnly( row, COL_BITMAP );
        GetGridList()->SetCellValue( row, COL_TYPENAME,item->GetClassName() );
        GetGridList()->SetCellValue( row, COL_REPEAT_NUMBER,
                                     wxString::Format( "%d", item->m_RepeatCount ) );
        GetGridList()->SetCellValue( row, COL_COMMENT, item->m_Info );

        if( item->GetType() == DS_DATA_ITEM::DS_TEXT )
        {
            DS_DATA_ITEM_TEXT* t_item = static_cast<DS_DATA_ITEM_TEXT*>( item );
            GetGridList()->SetCellValue( row, COL_TEXTSTRING, t_item->m_TextBase );
        }

        m_itemsList.push_back( item );
        row++;
    }

    // Now resize the columns:
    int cols_to_resize[] =
    {
        COL_BITMAP, COL_TYPENAME, COL_REPEAT_NUMBER, COL_COMMENT, COL_TEXTSTRING, COL_COUNT
    };

    for( int ii = 0; ; ii++ )
    {
        int col = cols_to_resize[ii];

        if( col == COL_COUNT )
            break;

        if( col == COL_BITMAP )
        {
            #define BITMAP_SIZE 16
            GetGridList()->SetColMinimalWidth( col, BITMAP_SIZE*2 );
            GetGridList()->AutoSizeColumn( col, false );
        }
        else
        {
            GetGridList()->AutoSizeColumn( col );
        }

        GetGridList()->AutoSizeColLabelSize( col );
    }
}


void DIALOG_INSPECTOR::SelectRow( DS_DATA_ITEM* aItem )
{
    // m_itemsList[0] is not a true DS_DATA_ITEM
    for( unsigned row = 1; row < m_itemsList.size(); ++row )
    {
        if( m_itemsList[row] == aItem )
        {
            GetGridList()->GoToCell( row, COL_TYPENAME );
            GetGridList()->SelectRow( row );
            break;
        }
    }
}


DS_DATA_ITEM* DIALOG_INSPECTOR::GetDrawingSheetDataItem( int aRow ) const
{
        return ( aRow >= 0 && aRow < (int)m_itemsList.size() ) ? m_itemsList[aRow]: nullptr;
}


void DIALOG_INSPECTOR::onCellClicked( wxGridEvent& event )
{
    int row = event.GetRow();
    GetGridList()->SelectRow( row );

    DS_DATA_ITEM* item = GetDrawingSheetDataItem( row );

    if( !item )     // only DS_DATA_ITEM are returned.
        return;

    // Select this item in drawing sheet editor, and update the properties panel:
    PL_SELECTION_TOOL*  selectionTool = m_editorFrame->GetToolManager()->GetTool<PL_SELECTION_TOOL>();
    selectionTool->ClearSelection();
    EDA_ITEM* draw_item = item->GetDrawItems()[0];
    selectionTool->AddItemToSel( draw_item );
    m_editorFrame->GetCanvas()->Refresh();
    m_editorFrame->GetPropertiesFrame()->CopyPrmsFromItemToPanel( item );
}


void BitmapGridCellRenderer::Draw( wxGrid& aGrid, wxGridCellAttr& aAttr,
                                   wxDC& aDc, const wxRect& aRect,
                                   int aRow, int aCol, bool aIsSelected )
{
    wxGridCellStringRenderer::Draw( aGrid, aAttr, aDc, aRect, aRow, aCol, aIsSelected );
    wxBitmap bm( m_BitmapXPM );
    aDc.DrawBitmap( bm,aRect.GetX()+5, aRect.GetY()+2, true );
}


void PL_EDITOR_FRAME::ShowDesignInspector()
{
    DIALOG_INSPECTOR dlg( this );

    dlg.ShowModal();
}
