/**
 * @file class_treeprojectfiles.cpp
 * this is the wxTreeCtrl that shows a KiCad tree project files
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2012 Jean-Pierre Charras
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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


#include <fctsys.h>

#include <kicad.h>
#include <tree_project_frame.h>
#include <class_treeprojectfiles.h>
#include <class_treeproject_item.h>

#include <wx/regex.h>
#include <wx/imaglist.h>
#include <menus_helpers.h>


IMPLEMENT_ABSTRACT_CLASS( TREEPROJECTFILES, wxTreeCtrl )


TREEPROJECTFILES::TREEPROJECTFILES( TREE_PROJECT_FRAME* parent ) :
    wxTreeCtrl( parent, ID_PROJECT_TREE,
                wxDefaultPosition, wxDefaultSize,
                wxTR_HAS_BUTTONS, wxDefaultValidator,
                wxT( "EDATreeCtrl" ) )
{
    m_Parent = parent;

    // icons size is not know (depending on they are built)
    // so get it:
    wxSize iconsize;
    wxBitmap dummy = KiBitmap( eeschema_xpm );
    iconsize.x = dummy.GetWidth();
    iconsize.y = dummy.GetHeight();

    // Make an image list containing small icons
    m_ImageList = new wxImageList( iconsize.x, iconsize.y, true, TREE_MAX );

    m_ImageList->Add( KiBitmap( kicad_icon_small_xpm ) );       // TREE_PROJECT
    m_ImageList->Add( KiBitmap( eeschema_xpm ) );               // TREE_SCHEMA
    m_ImageList->Add( KiBitmap( pcbnew_xpm ) );                 // TREE_LEGACY_PCB
    m_ImageList->Add( KiBitmap( pcbnew_xpm ) );                 // TREE_SFMT_PCB
    m_ImageList->Add( KiBitmap( icon_gerbview_small_xpm ) );    // TREE_GERBER
    m_ImageList->Add( KiBitmap( datasheet_xpm ) );              // TREE_PDF
    m_ImageList->Add( KiBitmap( icon_txt_xpm ) );               // TREE_TXT
    m_ImageList->Add( KiBitmap( netlist_xpm ) );                // TREE_NET
    m_ImageList->Add( KiBitmap( unknown_xpm ) );                // TREE_UNKNOWN
    m_ImageList->Add( KiBitmap( directory_xpm ) );              // TREE_DIRECTORY
    m_ImageList->Add( KiBitmap( icon_cvpcb_small_xpm ) );       // TREE_CMP_LINK
    m_ImageList->Add( KiBitmap( tools_xpm ) );                  // TREE_REPORT
    m_ImageList->Add( KiBitmap( post_compo_xpm ) );             // TREE_POS
    m_ImageList->Add( KiBitmap( post_drill_xpm ) );             // TREE_DRILL
    m_ImageList->Add( KiBitmap( svg_file_xpm ) );               // TREE_SVG
    m_ImageList->Add( KiBitmap( pagelayout_load_default_xpm ) );// TREE_PAGE_LAYOUT_DESCR

    SetImageList( m_ImageList );
}


TREEPROJECTFILES::~TREEPROJECTFILES()
{
    delete m_ImageList;
}


int TREEPROJECTFILES::OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 )
{
    TREEPROJECT_ITEM* myitem1 = (TREEPROJECT_ITEM*) GetItemData( item1 );
    TREEPROJECT_ITEM* myitem2 = (TREEPROJECT_ITEM*) GetItemData( item2 );

    if( myitem1->GetType() == TREE_DIRECTORY && myitem2->GetType() != TREE_DIRECTORY )
        return -1;

    if( myitem2->GetType() == TREE_DIRECTORY && myitem1->GetType() != TREE_DIRECTORY )
        return 1;

    if( myitem1->IsRootFile() && !myitem2->IsRootFile() )
        return -1;

    if( myitem2->IsRootFile() && !myitem1->IsRootFile() )
        return 1;

    return myitem1->GetFileName().CmpNoCase( myitem2->GetFileName() );
}

