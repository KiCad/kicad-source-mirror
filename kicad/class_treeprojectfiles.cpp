/*
 * file class_treeprojectfiles.cpp
 * this is the wxTreeCtrl that shows a Kicad tree project files
 */

#ifdef KICAD_PYTHON
#include <pyhandler.h>
#endif

#include "fctsys.h"
#include "common.h"
#include "bitmaps.h"

#include "kicad.h"
#include "tree_project_frame.h"
#include "class_treeprojectfiles.h"
#include "class_treeproject_item.h"

#include "wx/regex.h"


IMPLEMENT_ABSTRACT_CLASS( TREEPROJECTFILES, wxTreeCtrl )


TREEPROJECTFILES::TREEPROJECTFILES( TREE_PROJECT_FRAME* parent ) :
    wxTreeCtrl( parent, ID_PROJECT_TREE,
                wxDefaultPosition, wxDefaultSize,
                wxTR_HAS_BUTTONS | wxTR_EDIT_LABELS, wxDefaultValidator,
                wxT( "EDATreeCtrl" ) )
{
    m_Parent = parent;

    // Make an image list containing small icons
    m_ImageList = new wxImageList( 16, 16, TRUE, TREE_MAX );

    m_ImageList->Add( wxBitmap( kicad_icon_small_xpm ) );       // TREE_PROJECT
    m_ImageList->Add( wxBitmap( eeschema_xpm ) );               // TREE_SCHEMA
    m_ImageList->Add( wxBitmap( pcbnew_xpm ) );                 // TREE_PCB
    m_ImageList->Add( wxBitmap( icon_python_small_xpm ) );      // TREE_PY
    m_ImageList->Add( wxBitmap( icon_gerbview_small_xpm ) );    // TREE_GERBER
    m_ImageList->Add( wxBitmap( datasheet_xpm ) );              // TREE_PDF
    m_ImageList->Add( wxBitmap( icon_txt_xpm ) );               // TREE_TXT
    m_ImageList->Add( wxBitmap( icon_cvpcb_small_xpm ) );       // TREE_NET
    m_ImageList->Add( wxBitmap( unknown_xpm ) );                // TREE_UNKNOWN
    m_ImageList->Add( wxBitmap( directory_xpm ) );              // TREE_DIRECTORY

    SetImageList( m_ImageList );
}


TREEPROJECTFILES::~TREEPROJECTFILES()
{
    if( m_ImageList )
        delete m_ImageList;
}


int TREEPROJECTFILES::OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 )
{
    TREEPROJECT_ITEM* myitem1 = (TREEPROJECT_ITEM*) GetItemData( item1 );
    TREEPROJECT_ITEM* myitem2 = (TREEPROJECT_ITEM*) GetItemData( item2 );

    if( (myitem1->m_Type == TREE_DIRECTORY) && ( myitem2->m_Type != TREE_DIRECTORY ) )
        return -1;
    if( (myitem2->m_Type == TREE_DIRECTORY) && ( myitem1->m_Type != TREE_DIRECTORY ) )
        return 1;

    if( myitem1->m_IsRootFile  && !myitem2->m_IsRootFile )
        return -1;
    if( myitem2->m_IsRootFile && !myitem1->m_IsRootFile )
        return 1;

    return myitem1->m_FileName.CmpNoCase( myitem2->m_FileName );
}

