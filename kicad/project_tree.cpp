/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2012 Jean-Pierre Charras
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <bitmaps.h>

#include "project_tree_item.h"
#include "project_tree_pane.h"
#include "project_tree.h"
#include "kicad_id.h"


IMPLEMENT_ABSTRACT_CLASS( PROJECT_TREE, wxTreeCtrl )


PROJECT_TREE::PROJECT_TREE( PROJECT_TREE_PANE* parent ) :
        wxTreeCtrl( parent, ID_PROJECT_TREE, wxDefaultPosition, wxDefaultSize,
                    wxTR_HAS_BUTTONS | wxTR_MULTIPLE, wxDefaultValidator,
                    wxT( "EDATreeCtrl" ) )
{
    m_projectTreePane = parent;

    // icons size is not know (depending on they are built)
    // so get it:
    wxSize iconsize;
    wxBitmap dummy = KiBitmap( icon_eeschema_24_xpm );
    iconsize.x = dummy.GetWidth();
    iconsize.y = dummy.GetHeight();

    // Make an image list containing small icons
    m_imageList = new wxImageList( iconsize.x, iconsize.y, true,
                                   static_cast<int>( TREE_FILE_TYPE::MAX ) );

    m_imageList->Add( KiBitmap( project_xpm ) );                // TREE_LEGACY_PROJECT
    m_imageList->Add( KiBitmap( project_kicad_xpm ) );          // TREE_JSON_PROJECT
    m_imageList->Add( KiBitmap( icon_eeschema_24_xpm ) );       // TREE_LEGACY_SCHEMATIC
    m_imageList->Add( KiBitmap( icon_eeschema_24_xpm ) );       // TREE_SEXPR_SCHEMATIC
    m_imageList->Add( KiBitmap( icon_pcbnew_24_xpm ) );         // TREE_LEGACY_PCB
    m_imageList->Add( KiBitmap( icon_pcbnew_24_xpm ) );         // TREE_SEXPR_PCB
    m_imageList->Add( KiBitmap( icon_gerbview_24_xpm ) );       // TREE_GERBER
    m_imageList->Add( KiBitmap( file_gerber_job_xpm ) );        // TREE_GERBER_JOB_FILE (.gbrjob)
    m_imageList->Add( KiBitmap( file_html_xpm ) );              // TREE_HTML
    m_imageList->Add( KiBitmap( file_pdf_xpm ) );               // TREE_PDF
    m_imageList->Add( KiBitmap( editor_xpm ) );                 // TREE_TXT
    m_imageList->Add( KiBitmap( netlist_xpm ) );                // TREE_NET
    m_imageList->Add( KiBitmap( unknown_xpm ) );                // TREE_UNKNOWN
    m_imageList->Add( KiBitmap( directory_xpm ) );              // TREE_DIRECTORY
    m_imageList->Add( KiBitmap( icon_cvpcb_24_xpm ) );          // TREE_CMP_LINK
    m_imageList->Add( KiBitmap( tools_xpm ) );                  // TREE_REPORT
    m_imageList->Add( KiBitmap( file_pos_xpm ) );               // TREE_POS
    m_imageList->Add( KiBitmap( file_drl_xpm ) );               // TREE_DRILL
    m_imageList->Add( KiBitmap( file_drl_xpm ) );               // TREE_DRILL_NC (similar TREE_DRILL)
    m_imageList->Add( KiBitmap( file_drl_xpm ) );               // TREE_DRILL_XNC (similar TREE_DRILL)
    m_imageList->Add( KiBitmap( file_svg_xpm ) );               // TREE_SVG
    m_imageList->Add( KiBitmap( icon_pagelayout_editor_24_xpm ) );             // TREE_PAGE_LAYOUT_DESCR
    m_imageList->Add( KiBitmap( module_xpm ) );                 // TREE_FOOTPRINT_FILE
    m_imageList->Add( KiBitmap( library_xpm ) );                // TREE_SCHEMATIC_LIBFILE
    m_imageList->Add( KiBitmap( library_xpm ) );                // TREE_SEXPR_SYMBOL_LIB_FILE

    SetImageList( m_imageList );
}


PROJECT_TREE::~PROJECT_TREE()
{
    delete m_imageList;
}


int PROJECT_TREE::OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 )
{
    PROJECT_TREE_ITEM* myitem1 = (PROJECT_TREE_ITEM*) GetItemData( item1 );
    PROJECT_TREE_ITEM* myitem2 = (PROJECT_TREE_ITEM*) GetItemData( item2 );

    if( !myitem1 || !myitem2 )
        return 0;

    if( myitem1->GetType() == TREE_FILE_TYPE::DIRECTORY
            && myitem2->GetType() != TREE_FILE_TYPE::DIRECTORY )
        return -1;

    if( myitem2->GetType() == TREE_FILE_TYPE::DIRECTORY
            && myitem1->GetType() != TREE_FILE_TYPE::DIRECTORY )
        return 1;

    if( myitem1->IsRootFile() && !myitem2->IsRootFile() )
        return -1;

    if( myitem2->IsRootFile() && !myitem1->IsRootFile() )
        return 1;

    return myitem1->GetFileName().CmpNoCase( myitem2->GetFileName() );
}
