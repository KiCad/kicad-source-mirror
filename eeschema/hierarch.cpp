/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <fctsys.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <id.h>
#include <bitmaps.h>
#include <dialog_shim.h>
#include <sch_edit_frame.h>
#include <general.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <wx/imaglist.h>
#include <wx/treectrl.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/sch_editor_control.h>
//#include <netlist_object.h>
#include <sch_sheet_path.h>

#include <hierarch.h>
#include <view/view.h>

class HIERARCHY_NAVIG_DLG;


/**
 * Store an SCH_SHEET_PATH of each sheet in hierarchy.
 */
class TreeItemData : public wxTreeItemData
{
public:
    SCH_SHEET_PATH m_SheetPath;

    TreeItemData( SCH_SHEET_PATH& sheet ) : wxTreeItemData()
    {
        m_SheetPath = sheet;
    }
};

HIERARCHY_TREE::HIERARCHY_TREE( HIERARCHY_NAVIG_DLG* parent ) :
    wxTreeCtrl( (wxWindow*) parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTR_HAS_BUTTONS, wxDefaultValidator, wxT( "HierachyTreeCtrl" ) )
{
    m_parent = parent;

    // Make an image list containing small icons
    // All icons are expected having the same size.
    wxBitmap tree_nosel_bm( KiBitmap( tree_nosel_xpm ) );
    imageList = new wxImageList( tree_nosel_bm.GetWidth(), tree_nosel_bm.GetHeight(), true, 2 );

    imageList->Add( tree_nosel_bm );
    imageList->Add( KiBitmap( tree_sel_xpm ) );

    AssignImageList( imageList );
}

HIERARCHY_NAVIG_DLG::HIERARCHY_NAVIG_DLG( SCH_EDIT_FRAME* aParent ) :
    DIALOG_SHIM( aParent, wxID_ANY, _( "Navigator" ), wxDefaultPosition, wxDefaultSize,
                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER, HIERARCHY_NAVIG_DLG_WNAME )
{
    wxASSERT( dynamic_cast< SCH_EDIT_FRAME* >( aParent ) );

    m_SchFrameEditor = aParent;
    m_currSheet = aParent->GetCurrentSheet();
    m_Tree = new HIERARCHY_TREE( this );
    m_nbsheets = 1;

    // root is the link to the main sheet.
    wxTreeItemId root = m_Tree->AddRoot( _( "Root" ), 0, 1 );
    m_Tree->SetItemBold( root, true );

    m_list.push_back( g_RootSheet );
    m_Tree->SetItemData( root, new TreeItemData( m_list ) );

    if( m_SchFrameEditor->GetCurrentSheet().Last() == g_RootSheet )
        m_Tree->SelectItem( root );

    buildHierarchyTree( &m_list, &root );

    m_Tree->ExpandAll();

    // This bloc gives a good size to the dialog, better than the default "best" size,
    // the first time the dialog is opened, during a session
    wxRect itemrect;
    wxSize tree_size;

    m_Tree->GetBoundingRect( root, itemrect );

    // Set dialog window size to be large enough
    tree_size.x = itemrect.GetWidth() + 20;
    tree_size.x = std::max( tree_size.x, 250 );

    // Readjust the size of the frame to an optimal value.
    tree_size.y = m_nbsheets * itemrect.GetHeight();

    if( m_nbsheets < 2 )
        tree_size.y += 10;  // gives a better look for small trees

    SetClientSize( tree_size );

    // manage the ESC key to close the dialog, because thre is no Cancel button
    // in dialog
    m_Tree->Connect( wxEVT_CHAR, wxKeyEventHandler( HIERARCHY_TREE::onChar ) );

    // Manage double click on a selection, or the enter key:
    Bind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_NAVIG_DLG::onSelectSheetPath, this );
    // Manage a simple click on a selection, if the selection changes
    Bind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_NAVIG_DLG::onSelectSheetPath, this );

	// Connect close event for the dialog:
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( HIERARCHY_NAVIG_DLG::OnCloseNav ) );
}


HIERARCHY_NAVIG_DLG::~HIERARCHY_NAVIG_DLG()
{
    Unbind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_NAVIG_DLG::onSelectSheetPath, this );
    Unbind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_NAVIG_DLG::onSelectSheetPath, this );
    m_Tree->Disconnect( wxEVT_CHAR, wxKeyEventHandler( HIERARCHY_TREE::onChar ) );
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( HIERARCHY_NAVIG_DLG::OnCloseNav ) );
}


void HIERARCHY_TREE::onChar( wxKeyEvent& event )
{
    if( event.GetKeyCode() == WXK_ESCAPE )
        m_parent->Close( true );
    else
        event.Skip();
}


void HIERARCHY_NAVIG_DLG::buildHierarchyTree( SCH_SHEET_PATH* aList, wxTreeItemId* aPreviousmenu )
{
    wxCHECK_RET( m_nbsheets < NB_MAX_SHEET, "Maximum number of sheets exceeded." );

    for( auto aItem : aList->LastScreen()->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( aItem );
        m_nbsheets++;
        wxTreeItemId menu;
        menu = m_Tree->AppendItem( *aPreviousmenu, sheet->GetName(), 0, 1 );
        aList->push_back( sheet );
        m_Tree->SetItemData( menu, new TreeItemData( *aList ) );

        if( *aList == m_currSheet )
        {
            m_Tree->EnsureVisible( menu );
            m_Tree->SelectItem( menu );
        }

        buildHierarchyTree( aList, &menu );
        aList->pop_back();

        if( m_nbsheets >= NB_MAX_SHEET )
            break;
    }
}

void HIERARCHY_NAVIG_DLG::UpdateHierarchyTree()
{
    Freeze();

    m_currSheet       = m_SchFrameEditor->GetCurrentSheet();
    wxTreeItemId root = m_Tree->GetRootItem();
    m_Tree->DeleteChildren( root );
    m_list.clear();
    m_list.push_back( g_RootSheet );
    buildHierarchyTree( &m_list, &root );

    Thaw();
}

void HIERARCHY_NAVIG_DLG::onSelectSheetPath( wxTreeEvent& event )
{
    m_SchFrameEditor->GetToolManager()->RunAction( ACTIONS::cancelInteractive, true );
    m_SchFrameEditor->GetToolManager()->RunAction( EE_ACTIONS::clearSelection, true );

    wxTreeItemId ItemSel = m_Tree->GetSelection();
    m_SchFrameEditor->SetCurrentSheet(( (TreeItemData*) m_Tree->GetItemData( ItemSel ) )->m_SheetPath );
    m_SchFrameEditor->DisplayCurrentSheet();

    if( m_SchFrameEditor->GetNavigatorStaysOpen() == false )
        Close( true );
}


void HIERARCHY_NAVIG_DLG::OnCloseNav( wxCloseEvent& event )
{
    Destroy();
}


void SCH_EDIT_FRAME::DisplayCurrentSheet()
{
    m_toolManager->RunAction( ACTIONS::cancelInteractive, true );
    m_toolManager->RunAction( EE_ACTIONS::clearSelection, true );

    SCH_SCREEN* screen = g_CurrentSheet->LastScreen();

    wxASSERT( screen );

    // Switch to current sheet,
    // and update the grid size, because it can be modified in latest screen
    SetScreen( screen );
    GetScreen()->SetGrid( m_LastGridSizeId + ID_POPUP_GRID_LEVEL_1000 );

    // update the References
    g_CurrentSheet->UpdateAllScreenReferences();
    SetSheetNumberAndCount();

    if( !screen->m_Initialized )
    {
        m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
        screen->m_Initialized = true;
        screen->ClearUndoORRedoList( screen->m_UndoList, 1 );
    }
    else
    {
        // RedrawScreen() will set zoom to last used
        RedrawScreen( (wxPoint) GetScreen()->m_ScrollCenter, false );
    }

    UpdateTitle();

    SCH_EDITOR_CONTROL* editTool = m_toolManager->GetTool<SCH_EDITOR_CONTROL>();
    TOOL_EVENT dummy;
    editTool->UpdateNetHighlighting( dummy );

    HardRedraw();   // Ensure any item has its view updated, especially the worksheet items
}
