/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_draw_panel.h>
#include <confirm.h>
#include <id.h>
#include <bitmaps.h>
#include <dialog_shim.h>
#include <sch_edit_frame.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/sch_editor_control.h>
#include <sch_sheet_path.h>

#include <hierarch.h>
#include <view/view.h>
#include <kiface_i.h>
#include "eeschema_settings.h"

#include <wx/object.h>

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

// Need to use wxRTTI macros in order for OnCompareItems to work properly
// See: https://docs.wxwidgets.org/3.1/classwx_tree_ctrl.html#ab90a465793c291ca7aa827a576b7d146
wxIMPLEMENT_ABSTRACT_CLASS( HIERARCHY_TREE, wxTreeCtrl );


HIERARCHY_TREE::HIERARCHY_TREE( HIERARCHY_NAVIG_DLG* parent ) :
    wxTreeCtrl( (wxWindow*) parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxTR_HAS_BUTTONS, wxDefaultValidator, wxT( "HierachyTreeCtrl" ) )
{
    m_parent = parent;

    // Make an image list containing small icons
    // All icons are expected having the same size.
    wxBitmap tree_nosel_bm( KiBitmap( BITMAPS::tree_nosel ) );
    imageList = new wxImageList( tree_nosel_bm.GetWidth(), tree_nosel_bm.GetHeight(), true, 2 );

    imageList->Add( tree_nosel_bm );
    imageList->Add( KiBitmap( BITMAPS::tree_sel ) );

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
    wxTreeItemId root = m_Tree->AddRoot( getRootString(), 0, 1 );
    m_Tree->SetItemBold( root, true );

    m_list.push_back( &m_SchFrameEditor->Schematic().Root() );
    m_Tree->SetItemData( root, new TreeItemData( m_list ) );

    if( m_SchFrameEditor->GetCurrentSheet().Last() == &m_SchFrameEditor->Schematic().Root() )
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

    // manage the ESC key to close the dialog, because there is no Cancel button
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
    Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( HIERARCHY_NAVIG_DLG::OnCloseNav ) );
}


void HIERARCHY_TREE::onChar( wxKeyEvent& event )
{
    if( event.GetKeyCode() == WXK_ESCAPE )
        m_parent->Close( true );
    else
        event.Skip();
}


int HIERARCHY_TREE::OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 )
{
    SCH_SHEET_PATH* item1Path = &static_cast<TreeItemData*>( GetItemData( item1 ) )->m_SheetPath;
    SCH_SHEET_PATH* item2Path = &static_cast<TreeItemData*>( GetItemData( item2 ) )->m_SheetPath;

    return item1Path->ComparePageNumAndName( *item2Path );
}


void HIERARCHY_NAVIG_DLG::buildHierarchyTree( SCH_SHEET_PATH* aList, wxTreeItemId* aPreviousmenu )
{
    wxCHECK_RET( m_nbsheets < NB_MAX_SHEET, "Maximum number of sheets exceeded." );

    std::vector<SCH_ITEM*> sheetChildren;
    aList->LastScreen()->GetSheets( &sheetChildren );

    for( SCH_ITEM* aItem : sheetChildren )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( aItem );
        aList->push_back( sheet );

        wxString sheetName = formatPageString( sheet->GetFields()[SHEETNAME].GetShownText(),
                                               sheet->GetPageNumber( *aList ) );
        m_nbsheets++;
        wxTreeItemId menu;
        menu = m_Tree->AppendItem( *aPreviousmenu, sheetName, 0, 1 );
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

    m_Tree->SortChildren( *aPreviousmenu );
}


void HIERARCHY_NAVIG_DLG::UpdateHierarchyTree()
{
    Freeze();

    // Disable selection events
    Unbind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_NAVIG_DLG::onSelectSheetPath, this );
    Unbind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_NAVIG_DLG::onSelectSheetPath, this );

    m_currSheet = m_SchFrameEditor->GetCurrentSheet();
    m_Tree->DeleteAllItems();
    m_nbsheets = 1;

    wxTreeItemId root = m_Tree->AddRoot( getRootString(), 0, 1 );
    m_Tree->SetItemBold( root, true );

    m_list.clear();
    m_list.push_back( &m_SchFrameEditor->Schematic().Root() );
    m_Tree->SetItemData( root, new TreeItemData( m_list ) );

    if( m_SchFrameEditor->GetCurrentSheet().Last() == &m_SchFrameEditor->Schematic().Root() )
        m_Tree->SelectItem( root );

    buildHierarchyTree( &m_list, &root );
    m_Tree->ExpandAll();

    // Enable selection events
    Bind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_NAVIG_DLG::onSelectSheetPath, this );
    Bind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_NAVIG_DLG::onSelectSheetPath, this );

    Thaw();
}


void HIERARCHY_NAVIG_DLG::onSelectSheetPath( wxTreeEvent& event )
{
    m_SchFrameEditor->GetToolManager()->RunAction( ACTIONS::cancelInteractive, true );
    m_SchFrameEditor->GetToolManager()->RunAction( EE_ACTIONS::clearSelection, true );

    wxTreeItemId  itemSel = m_Tree->GetSelection();
    TreeItemData* itemData = static_cast<TreeItemData*>( m_Tree->GetItemData( itemSel ) );

    // Store the current zoom level into the current screen before switching
    m_SchFrameEditor->GetScreen()->m_LastZoomLevel =
                m_SchFrameEditor->GetCanvas()->GetView()->GetScale();

    m_SchFrameEditor->SetCurrentSheet( itemData->m_SheetPath );
    m_SchFrameEditor->DisplayCurrentSheet();

    EESCHEMA_SETTINGS* appSettings = static_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    if( !appSettings->m_Appearance.navigator_stays_open )
        Close( true );
}


wxString HIERARCHY_NAVIG_DLG::getRootString()
{
    SCH_SHEET*   rootSheet = &m_SchFrameEditor->Schematic().Root();
    SCH_SHEET_PATH rootPath;
    rootPath.push_back( rootSheet );

    return formatPageString ( _( "Root" ), rootSheet->GetPageNumber( rootPath ) );
}


wxString HIERARCHY_NAVIG_DLG::formatPageString( wxString aName, wxString aPage )
{
    return aName + wxT( " " ) + wxString::Format( _( "(page %s)" ), aPage );
}


void HIERARCHY_NAVIG_DLG::OnCloseNav( wxCloseEvent& event )
{
    Destroy();
}


void SCH_EDIT_FRAME::DisplayCurrentSheet()
{
    m_toolManager->RunAction( ACTIONS::cancelInteractive, true );
    m_toolManager->RunAction( EE_ACTIONS::clearSelection, true );
    SCH_SCREEN* screen = GetCurrentSheet().LastScreen();

    wxASSERT( screen );

    SetScreen( screen );

    // update the References
    GetCurrentSheet().UpdateAllScreenReferences();
    SetSheetNumberAndCount();

    if( !screen->m_zoomInitialized )
    {
        initScreenZoom();
    }
    else
    {
        // Set zoom to last used in this screen
        GetCanvas()->GetView()->SetScale( GetScreen()->m_LastZoomLevel );
        RedrawScreen( (wxPoint) GetScreen()->m_ScrollCenter, false );
    }

    UpdateTitle();

    SCH_EDITOR_CONTROL* editTool = m_toolManager->GetTool<SCH_EDITOR_CONTROL>();
    TOOL_EVENT dummy;
    editTool->UpdateNetHighlighting( dummy );

    HardRedraw();   // Ensure all items are redrawn (especially the drawing-sheet items)
}
