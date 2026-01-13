/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2012 Jean-Pierre Charras
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


#include <bitmaps.h>
#include <git/kicad_git_common.h>
#include <wx/settings.h>
#include <wx/dcmemory.h>

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
    // We pass ownership of these to wxWidgets in SetImageList() and SetStateImageList()
    //delete m_statusImageList;
}


void PROJECT_TREE::LoadIcons()
{
#ifdef __WXMAC__
    const int c_fileDefSize = 18;
    const int c_gitDefSize = 16;
#else
    const int c_fileDefSize = 22;
    const int c_gitDefSize = 16;
#endif

    auto getBundle = [&]( BITMAPS aBmp, int aDefSize )
    {
#ifdef __WXMSW__
        // Add padding to bitmaps because MSW is the only platform that doesn't get it automatically
        const int          c_padding = 1;
        wxVector<wxBitmap> bmps;

        for( double scale : { 1.0, 1.25, 1.5, 1.75, 2.0, 3.0 } )
        {
            int            size = aDefSize * scale;
            int            paddedSize = size + c_padding * 2 * scale;
            wxBitmapBundle scaled = KiBitmapBundleDef( aBmp, size );
            wxBitmap       bmp = scaled.GetBitmap( wxDefaultSize );
            wxBitmap       padded( paddedSize, paddedSize, 32 );

            {
                padded.UseAlpha();
                wxMemoryDC dc( padded );
                dc.DrawBitmap( bmp, c_padding * scale, c_padding * scale );
            }

            bmps.push_back( padded );
        }

        return wxBitmapBundle::FromBitmaps( bmps );
#else
        return KiBitmapBundleDef( aBmp, aDefSize );
#endif
    };

    wxVector<wxBitmapBundle> images;
    images.push_back( getBundle( BITMAPS::project, c_fileDefSize ) );                // TREE_LEGACY_PROJECT
    images.push_back( getBundle( BITMAPS::project_kicad, c_fileDefSize ) );          // TREE_JSON_PROJECT
    images.push_back( getBundle( BITMAPS::icon_eeschema_24, c_fileDefSize ) );       // TREE_LEGACY_SCHEMATIC
    images.push_back( getBundle( BITMAPS::icon_eeschema_24, c_fileDefSize ) );       // TREE_SEXPR_SCHEMATIC
    images.push_back( getBundle( BITMAPS::icon_pcbnew_24, c_fileDefSize ) );         // TREE_LEGACY_PCB
    images.push_back( getBundle( BITMAPS::icon_pcbnew_24, c_fileDefSize ) );         // TREE_SEXPR_PCB
    images.push_back( getBundle( BITMAPS::icon_gerbview_24, c_fileDefSize ) );       // TREE_GERBER
    images.push_back( getBundle( BITMAPS::file_gerber_job, c_fileDefSize ) );        // TREE_GERBER_JOB_FILE (.gbrjob)
    images.push_back( getBundle( BITMAPS::file_html, c_fileDefSize ) );              // TREE_HTML
    images.push_back( getBundle( BITMAPS::file_pdf, c_fileDefSize ) );               // TREE_PDF
    images.push_back( getBundle( BITMAPS::editor, c_fileDefSize ) );                 // TREE_TXT
    images.push_back( getBundle( BITMAPS::editor, c_fileDefSize ) );                 // TREE_MD
    images.push_back( getBundle( BITMAPS::netlist, c_fileDefSize ) );                // TREE_NET
    images.push_back( getBundle( BITMAPS::file_cir, c_fileDefSize ) );               // TREE_NET_SPICE
    images.push_back( getBundle( BITMAPS::unknown, c_fileDefSize ) );                // TREE_UNKNOWN
    images.push_back( getBundle( BITMAPS::directory, c_fileDefSize ) );              // TREE_DIRECTORY
    images.push_back( getBundle( BITMAPS::icon_cvpcb_24, c_fileDefSize ) );          // TREE_CMP_LINK
    images.push_back( getBundle( BITMAPS::tools, c_fileDefSize ) );                  // TREE_REPORT
    images.push_back( getBundle( BITMAPS::file_pos, c_fileDefSize ) );               // TREE_POS
    images.push_back( getBundle( BITMAPS::file_drl, c_fileDefSize ) );               // TREE_DRILL
    images.push_back( getBundle( BITMAPS::file_drl, c_fileDefSize ) );               // TREE_DRILL_NC (similar TREE_DRILL)
    images.push_back( getBundle( BITMAPS::file_drl, c_fileDefSize ) );               // TREE_DRILL_XNC (similar TREE_DRILL)
    images.push_back( getBundle( BITMAPS::file_svg, c_fileDefSize ) );               // TREE_SVG
    images.push_back( getBundle( BITMAPS::icon_pagelayout_editor_24, c_fileDefSize ) ); // TREE_PAGE_LAYOUT_DESCR
    images.push_back( getBundle( BITMAPS::module, c_fileDefSize ) );                 // TREE_FOOTPRINT_FILE
    images.push_back( getBundle( BITMAPS::library, c_fileDefSize ) );                // TREE_SCHEMATIC_LIBFILE
    images.push_back( getBundle( BITMAPS::library, c_fileDefSize ) );                // TREE_SEXPR_SYMBOL_LIB_FILE
    images.push_back( getBundle( BITMAPS::editor, c_fileDefSize ) );                 // DESIGN_RULES
    images.push_back( getBundle( BITMAPS::zip, c_fileDefSize ) );                    // ZIP_ARCHIVE
    images.push_back( getBundle( BITMAPS::editor, c_fileDefSize ) );                 // JOBSET_FILE

    SetImages( images );

    // KiCad for macOS currently has backported SetStateImages for this control
    // that is otherwise available since wxWidgets 3.3 on other platforms.
#if wxCHECK_VERSION( 3, 3, 0 ) || defined( __WXMAC__ )
    wxVector<wxBitmapBundle> stateImages;
    stateImages.push_back( wxBitmapBundle( wxBitmap( c_gitDefSize, c_gitDefSize ) ) );      // GIT_STATUS_UNTRACKED
    stateImages.push_back( KiBitmapBundleDef( BITMAPS::git_good_check, c_gitDefSize ) );    // GIT_STATUS_CURRENT
    stateImages.push_back( KiBitmapBundleDef( BITMAPS::git_modified, c_gitDefSize ) );      // GIT_STATUS_MODIFIED
    stateImages.push_back( KiBitmapBundleDef( BITMAPS::git_add, c_gitDefSize ) );           // GIT_STATUS_ADDED
    stateImages.push_back( KiBitmapBundleDef( BITMAPS::git_delete, c_gitDefSize ) );        // GIT_STATUS_DELETED
    stateImages.push_back( KiBitmapBundleDef( BITMAPS::git_out_of_date, c_gitDefSize ) );   // GIT_STATUS_BEHIND
    stateImages.push_back( KiBitmapBundleDef( BITMAPS::git_changed_ahead, c_gitDefSize ) ); // GIT_STATUS_AHEAD
    stateImages.push_back( KiBitmapBundleDef( BITMAPS::git_conflict, c_gitDefSize ) );      // GIT_STATUS_CONFLICTED
    stateImages.push_back( wxBitmapBundle( wxBitmap( c_gitDefSize, c_gitDefSize ) ) );      // GIT_STATUS_IGNORED

    SetStateImages( stateImages );
#else
    // Make an image list containing small icons
    wxBitmap blank_bitmap( c_gitDefSize, c_gitDefSize );

    delete m_statusImageList;
    m_statusImageList = new wxImageList( c_gitDefSize, c_gitDefSize, true,
                                         static_cast<int>( KIGIT_COMMON::GIT_STATUS::GIT_STATUS_LAST ) );

    m_statusImageList->Add( blank_bitmap );                                         // GIT_STATUS_UNTRACKED
    m_statusImageList->Add( KiBitmap( BITMAPS::git_good_check, c_gitDefSize ) );    // GIT_STATUS_CURRENT
    m_statusImageList->Add( KiBitmap( BITMAPS::git_modified, c_gitDefSize ) );      // GIT_STATUS_MODIFIED
    m_statusImageList->Add( KiBitmap( BITMAPS::git_add, c_gitDefSize ) );           // GIT_STATUS_ADDED
    m_statusImageList->Add( KiBitmap( BITMAPS::git_delete, c_gitDefSize ) );        // GIT_STATUS_DELETED
    m_statusImageList->Add( KiBitmap( BITMAPS::git_out_of_date, c_gitDefSize ) );   // GIT_STATUS_BEHIND
    m_statusImageList->Add( KiBitmap( BITMAPS::git_changed_ahead, c_gitDefSize ) ); // GIT_STATUS_AHEAD
    m_statusImageList->Add( KiBitmap( BITMAPS::git_conflict, c_gitDefSize ) );      // GIT_STATUS_CONFLICTED
    m_statusImageList->Add( blank_bitmap );                                         // GIT_STATUS_IGNORED

    SetStateImageList( m_statusImageList );
#endif

}


void PROJECT_TREE::GetItemsRecursively( const wxTreeItemId& aParentId, std::vector<wxTreeItemId>& aItems )
{
    wxTreeItemIdValue cookie;
    wxTreeItemId      child = GetFirstChild( aParentId, cookie );

    while( child.IsOk() )
    {
        aItems.push_back( child );
        GetItemsRecursively( child, aItems );
        child = GetNextChild( aParentId, cookie );
    }
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
