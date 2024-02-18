/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2012 Jean-Pierre Charras
 * Copyright (C) 2004-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <git/kicad_git_common.h>
#include <wx/settings.h>

#include "project_tree_item.h"
#include "project_tree_pane.h"
#include "project_tree.h"
#include "kicad_id.h"


IMPLEMENT_ABSTRACT_CLASS( PROJECT_TREE, wxTreeCtrl )


#ifdef __WXMSW__
#define PLATFORM_STYLE wxTR_LINES_AT_ROOT
#else
#define PLATFORM_STYLE wxTR_NO_LINES
#endif

PROJECT_TREE::PROJECT_TREE( PROJECT_TREE_PANE* parent ) :
        wxTreeCtrl( parent, ID_PROJECT_TREE, wxDefaultPosition, wxDefaultSize,
                    PLATFORM_STYLE | wxTR_HAS_BUTTONS | wxTR_MULTIPLE, wxDefaultValidator,
                    wxT( "EDATreeCtrl" ) ),
        m_imageList( nullptr ),
        m_statusImageList( nullptr )
{
    m_projectTreePane = parent;
    m_gitCommon = std::make_unique<KIGIT_COMMON>( nullptr );

    // Make sure the GUI font scales properly on GTK
    SetFont( KIUI::GetControlFont( this ) );

    LoadIcons();
}


PROJECT_TREE::~PROJECT_TREE()
{
    delete m_imageList;
    delete m_statusImageList;
}


void PROJECT_TREE::LoadIcons()
{
    // KiCad for macOS currently has backported high-DPI support for this control
    // that is not in a released version of wxWidgets 3.2 yet.  This can become the
    // main codepath once wxWidgets 3.4 is released.
#ifdef __WXMAC__
    wxVector<wxBitmapBundle> images;
    images.push_back( KiBitmapBundle( BITMAPS::project ) );                // TREE_LEGACY_PROJECT
    images.push_back( KiBitmapBundle( BITMAPS::project_kicad ) );          // TREE_JSON_PROJECT
    images.push_back( KiBitmapBundle( BITMAPS::icon_eeschema_24 ) );       // TREE_LEGACY_SCHEMATIC
    images.push_back( KiBitmapBundle( BITMAPS::icon_eeschema_24 ) );       // TREE_SEXPR_SCHEMATIC
    images.push_back( KiBitmapBundle( BITMAPS::icon_pcbnew_24 ) );         // TREE_LEGACY_PCB
    images.push_back( KiBitmapBundle( BITMAPS::icon_pcbnew_24 ) );         // TREE_SEXPR_PCB
    images.push_back( KiBitmapBundle( BITMAPS::icon_gerbview_24 ) );       // TREE_GERBER
    images.push_back( KiBitmapBundle( BITMAPS::file_gerber_job ) );        // TREE_GERBER_JOB_FILE (.gbrjob)
    images.push_back( KiBitmapBundle( BITMAPS::file_html ) );              // TREE_HTML
    images.push_back( KiBitmapBundle( BITMAPS::file_pdf ) );               // TREE_PDF
    images.push_back( KiBitmapBundle( BITMAPS::editor ) );                 // TREE_TXT
    images.push_back( KiBitmapBundle( BITMAPS::editor ) );                 // TREE_MD
    images.push_back( KiBitmapBundle( BITMAPS::netlist ) );                // TREE_NET
    images.push_back( KiBitmapBundle( BITMAPS::file_cir ) );               // TREE_NET_SPICE
    images.push_back( KiBitmapBundle( BITMAPS::unknown ) );                // TREE_UNKNOWN
    images.push_back( KiBitmapBundle( BITMAPS::directory ) );              // TREE_DIRECTORY
    images.push_back( KiBitmapBundle( BITMAPS::icon_cvpcb_24 ) );          // TREE_CMP_LINK
    images.push_back( KiBitmapBundle( BITMAPS::tools ) );                  // TREE_REPORT
    images.push_back( KiBitmapBundle( BITMAPS::file_pos ) );               // TREE_POS
    images.push_back( KiBitmapBundle( BITMAPS::file_drl ) );               // TREE_DRILL
    images.push_back( KiBitmapBundle( BITMAPS::file_drl ) );               // TREE_DRILL_NC (similar TREE_DRILL)
    images.push_back( KiBitmapBundle( BITMAPS::file_drl ) );               // TREE_DRILL_XNC (similar TREE_DRILL)
    images.push_back( KiBitmapBundle( BITMAPS::file_svg ) );               // TREE_SVG
    images.push_back( KiBitmapBundle( BITMAPS::icon_pagelayout_editor_24 ) ); // TREE_PAGE_LAYOUT_DESCR
    images.push_back( KiBitmapBundle( BITMAPS::module ) );                 // TREE_FOOTPRINT_FILE
    images.push_back( KiBitmapBundle( BITMAPS::library ) );                // TREE_SCHEMATIC_LIBFILE
    images.push_back( KiBitmapBundle( BITMAPS::library ) );                // TREE_SEXPR_SYMBOL_LIB_FILE
    images.push_back( KiBitmapBundle( BITMAPS::editor ) );                 // DESIGN_RULES
    images.push_back( KiBitmapBundle( BITMAPS::zip ) );                    // ZIP_ARCHIVE
    SetImages( images );

    wxVector<wxBitmapBundle> stateImages;
    stateImages.push_back( wxBitmapBundle() );                          // GIT_STATUS_UNTRACKED
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_good_check ) ); // GIT_STATUS_CURRENT
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_modified ) ); // GIT_STATUS_MODIFIED
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_add ) );    // GIT_STATUS_ADDED
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_delete ) ); // GIT_STATUS_DELETED
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_out_of_date ) );   // GIT_STATUS_BEHIND
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_changed_ahead ) ); // GIT_STATUS_AHEAD
    stateImages.push_back( KiBitmapBundle( BITMAPS::git_conflict ) ); // GIT_STATUS_CONFLICTED

    SetStateImages( stateImages );
#else
    delete m_imageList;

    // Make an image list containing small icons
    int size = 24;

    m_imageList = new wxImageList( size, size, true,
                                   static_cast<int>( TREE_FILE_TYPE::MAX ) );

    m_imageList->Add( KiBitmap( BITMAPS::project, size ) );                // TREE_LEGACY_PROJECT
    m_imageList->Add( KiBitmap( BITMAPS::project_kicad, size ) );          // TREE_JSON_PROJECT
    m_imageList->Add( KiBitmap( BITMAPS::icon_eeschema_24, size ) );       // TREE_LEGACY_SCHEMATIC
    m_imageList->Add( KiBitmap( BITMAPS::icon_eeschema_24, size ) );       // TREE_SEXPR_SCHEMATIC
    m_imageList->Add( KiBitmap( BITMAPS::icon_pcbnew_24, size ) );         // TREE_LEGACY_PCB
    m_imageList->Add( KiBitmap( BITMAPS::icon_pcbnew_24, size ) );         // TREE_SEXPR_PCB
    m_imageList->Add( KiBitmap( BITMAPS::icon_gerbview_24, size ) );       // TREE_GERBER
    m_imageList->Add( KiBitmap( BITMAPS::file_gerber_job, size ) );        // TREE_GERBER_JOB_FILE (.gbrjob)
    m_imageList->Add( KiBitmap( BITMAPS::file_html, size ) );              // TREE_HTML
    m_imageList->Add( KiBitmap( BITMAPS::file_pdf, size ) );               // TREE_PDF
    m_imageList->Add( KiBitmap( BITMAPS::editor, size ) );                 // TREE_TXT
    m_imageList->Add( KiBitmap( BITMAPS::editor, size ) );                 // TREE_MD
    m_imageList->Add( KiBitmap( BITMAPS::netlist, size ) );                // TREE_NET
    m_imageList->Add( KiBitmap( BITMAPS::file_cir, size ) );               // TREE_NET_SPICE
    m_imageList->Add( KiBitmap( BITMAPS::unknown, size ) );                // TREE_UNKNOWN
    m_imageList->Add( KiBitmap( BITMAPS::directory, size ) );              // TREE_DIRECTORY
    m_imageList->Add( KiBitmap( BITMAPS::icon_cvpcb_24, size ) );          // TREE_CMP_LINK
    m_imageList->Add( KiBitmap( BITMAPS::tools, size ) );                  // TREE_REPORT
    m_imageList->Add( KiBitmap( BITMAPS::file_pos, size ) );               // TREE_POS
    m_imageList->Add( KiBitmap( BITMAPS::file_drl, size ) );               // TREE_DRILL
    m_imageList->Add( KiBitmap( BITMAPS::file_drl, size ) );               // TREE_DRILL_NC (similar TREE_DRILL)
    m_imageList->Add( KiBitmap( BITMAPS::file_drl, size ) );               // TREE_DRILL_XNC (similar TREE_DRILL)
    m_imageList->Add( KiBitmap( BITMAPS::file_svg, size ) );               // TREE_SVG
    m_imageList->Add( KiBitmap( BITMAPS::icon_pagelayout_editor_24, size ) ); // TREE_PAGE_LAYOUT_DESCR
    m_imageList->Add( KiBitmap( BITMAPS::module, size ) );                 // TREE_FOOTPRINT_FILE
    m_imageList->Add( KiBitmap( BITMAPS::library, size ) );                // TREE_SCHEMATIC_LIBFILE
    m_imageList->Add( KiBitmap( BITMAPS::library, size ) );                // TREE_SEXPR_SYMBOL_LIB_FILE
    m_imageList->Add( KiBitmap( BITMAPS::editor, size ) );                 // DESIGN_RULES
    m_imageList->Add( KiBitmap( BITMAPS::zip, size ) );                    // ZIP_ARCHIVE

    SetImageList( m_imageList );

    // Make an image list containing small icons
    size = 16;

    wxBitmap blank_bitmap( size, size );

    delete m_statusImageList;
    m_statusImageList = new wxImageList( size, size, true,
                                         static_cast<int>( KIGIT_COMMON::GIT_STATUS::GIT_STATUS_LAST ) );

    m_statusImageList->Add( blank_bitmap );                          // GIT_STATUS_UNTRACKED
    m_statusImageList->Add( KiBitmap( BITMAPS::git_good_check, size ) ); // GIT_STATUS_CURRENT
    m_statusImageList->Add( KiBitmap( BITMAPS::git_modified, size ) ); // GIT_STATUS_MODIFIED
    m_statusImageList->Add( KiBitmap( BITMAPS::git_add, size ) );    // GIT_STATUS_ADDED
    m_statusImageList->Add( KiBitmap( BITMAPS::git_delete, size ) ); // GIT_STATUS_DELETED
    m_statusImageList->Add( KiBitmap( BITMAPS::git_out_of_date, size ) );   // GIT_STATUS_BEHIND
    m_statusImageList->Add( KiBitmap( BITMAPS::git_changed_ahead, size ) ); // GIT_STATUS_AHEAD
    m_statusImageList->Add( KiBitmap( BITMAPS::git_conflict, size ) ); // GIT_STATUS_CONFLICTED

    SetStateImageList( m_statusImageList );
#endif

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
