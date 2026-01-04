/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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
#include <sch_edit_frame.h>
#include <sch_commit.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <hierarchy_pane.h>
#include <project/project_local_settings.h>
#include <kiface_base.h>
#include <wx/object.h>
#include <wx/generic/textdlgg.h>
#include <wx/menu.h>
#include <wx/wupdlock.h>
#include <wx/msgdlg.h>

/**
 * Store an SCH_SHEET_PATH of each sheet in hierarchy.
 */
class TREE_ITEM_DATA : public wxTreeItemData
{
public:
    SCH_SHEET_PATH m_SheetPath;

    TREE_ITEM_DATA( SCH_SHEET_PATH& sheet ) : wxTreeItemData(), m_SheetPath( sheet ) {}
};


// Need to use wxRTTI macros in order for OnCompareItems to work properly
// See: https://docs.wxwidgets.org/3.1/classwx_tree_ctrl.html#ab90a465793c291ca7aa827a576b7d146
wxIMPLEMENT_ABSTRACT_CLASS( HIERARCHY_TREE, wxTreeCtrl );


int HIERARCHY_TREE::OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 )
{
    SCH_SHEET_PATH* item1Path = &static_cast<TREE_ITEM_DATA*>( GetItemData( item1 ) )->m_SheetPath;
    SCH_SHEET_PATH* item2Path = &static_cast<TREE_ITEM_DATA*>( GetItemData( item2 ) )->m_SheetPath;

    return item1Path->ComparePageNum( *item2Path );
}


HIERARCHY_PANE::HIERARCHY_PANE( SCH_EDIT_FRAME* aParent ) :
        WX_PANEL( aParent )
{
    wxASSERT( dynamic_cast<SCH_EDIT_FRAME*>( aParent ) );

    m_frame = aParent;

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( sizer );
    m_tree = new HIERARCHY_TREE( this );

    wxVector<wxBitmapBundle> images;
    images.push_back( KiBitmapBundle( BITMAPS::tree_nosel ) );
    images.push_back( KiBitmapBundle( BITMAPS::tree_sel ) );
    m_tree->SetImages( images );

    sizer->Add( m_tree, 1, wxEXPAND, wxBORDER_NONE );

    m_events_bound = false;

    PROJECT_LOCAL_SETTINGS& localSettings = m_frame->Prj().GetLocalSettings();

    for( const wxString& path : localSettings.m_SchHierarchyCollapsed )
        m_collapsedPaths.insert( path );

    UpdateHierarchyTree();

    // Enable selection events
    Bind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_PANE::onSelectSheetPath, this );
    Bind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_PANE::onSelectSheetPath, this );
    Bind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_PANE::onTreeItemRightClick, this );
    Bind( wxEVT_CHAR_HOOK, &HIERARCHY_PANE::onCharHook, this );
    m_tree->Bind( wxEVT_TREE_END_LABEL_EDIT, &HIERARCHY_PANE::onTreeEditFinished, this );
    m_tree->Bind( wxEVT_CONTEXT_MENU, &HIERARCHY_PANE::onContextMenu, this );
    m_events_bound = true;
}


HIERARCHY_PANE::~HIERARCHY_PANE()
{
    Unbind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_PANE::onSelectSheetPath, this );
    Unbind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_PANE::onSelectSheetPath, this );
    Unbind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_PANE::onTreeItemRightClick, this );
    Unbind( wxEVT_CHAR_HOOK, &HIERARCHY_PANE::onCharHook, this );
    m_tree->Unbind( wxEVT_TREE_END_LABEL_EDIT, &HIERARCHY_PANE::onTreeEditFinished, this );
    m_tree->Unbind( wxEVT_CONTEXT_MENU, &HIERARCHY_PANE::onContextMenu, this );
}


void HIERARCHY_PANE::buildHierarchyTree( SCH_SHEET_PATH* aList, const wxTreeItemId& aParent )
{
    std::vector<SCH_ITEM*> sheetChildren;
    aList->LastScreen()->GetSheets( &sheetChildren );

    for( SCH_ITEM* aItem : sheetChildren )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( aItem );
        aList->push_back( sheet );

        wxString     sheetNameBase = sheet->GetField( FIELD_T::SHEET_NAME )->GetShownText( false );

        // If the sheet name is empty, use the filename (without extension) as fallback
        if( sheetNameBase.IsEmpty() )
        {
            wxFileName fn( sheet->GetFileName() );
            sheetNameBase = fn.GetName();
        }

        wxString     sheetName = formatPageString( sheetNameBase, aList->GetPageNumber() );
        wxTreeItemId child = m_tree->AppendItem( aParent, sheetName, 0, 1 );
        m_tree->SetItemData( child, new TREE_ITEM_DATA( *aList ) );

        buildHierarchyTree( aList, child );
        aList->pop_back();
    }

    m_tree->SortChildren( aParent );
}


void HIERARCHY_PANE::UpdateHierarchySelection()
{
    bool eventsWereBound = m_events_bound;

    if( eventsWereBound )
    {
        // Disable selection events
        Unbind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_PANE::onSelectSheetPath, this );
        Unbind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_PANE::onSelectSheetPath, this );
        Unbind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_PANE::onTreeItemRightClick, this );
        m_tree->Unbind( wxEVT_CONTEXT_MENU, &HIERARCHY_PANE::onContextMenu, this );

        m_events_bound = false;
    }

    std::function<void( const wxTreeItemId& )> recursiveDescent =
            [&]( const wxTreeItemId& id )
            {
                wxCHECK_RET( id.IsOk(), wxT( "Invalid tree item" ) );

                TREE_ITEM_DATA* itemData = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( id ) );

                // Skip items without data (e.g., project root node)
                if( !itemData )
                {
                    wxTreeItemIdValue cookie;
                    wxTreeItemId      child = m_tree->GetFirstChild( id, cookie );

                    while( child.IsOk() )
                    {
                        recursiveDescent( child );
                        child = m_tree->GetNextChild( id, cookie );
                    }

                    return;
                }

                if( itemData->m_SheetPath == m_frame->GetCurrentSheet() )
                {
                    wxTreeItemId parent = m_tree->GetItemParent( id );

                    if( parent.IsOk() )
                    {
                        // AT least on MSW, wxTreeCtrl::IsExpanded(item) and wxTreeCtrl::Expand(item)
                        // can be called only if item is visible.
                        // Otherwise wxWidgets alerts are thrown and Expand() say the item is invisible
                        if( m_tree->IsVisible( parent ) && !m_tree->IsExpanded( parent ) )
                            m_tree->Expand( parent );
                    }

                    if( !m_tree->IsVisible( id ) )
                        m_tree->EnsureVisible( id );

                    m_tree->SetItemBold( id, true );
                    m_tree->SetFocusedItem( id );
                }
                else
                {
                    m_tree->SetItemBold( id, false );
                }

                wxTreeItemIdValue cookie;
                wxTreeItemId      child = m_tree->GetFirstChild( id, cookie );

                while( child.IsOk() )
                {
                    recursiveDescent( child );
                    child = m_tree->GetNextChild( id, cookie );
                }
            };

    recursiveDescent( m_tree->GetRootItem() );

    if( eventsWereBound )
    {
        // Enable selection events
        Bind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_PANE::onSelectSheetPath, this );
        Bind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_PANE::onSelectSheetPath, this );
        Bind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_PANE::onTreeItemRightClick, this );
        m_tree->Bind( wxEVT_CONTEXT_MENU, &HIERARCHY_PANE::onContextMenu, this );

        m_events_bound = true;
    }
}


void HIERARCHY_PANE::UpdateHierarchyTree( bool aClear )
{
    wxWindowUpdateLocker updateLock( this );

    // If hierarchy hasn't been built yet (e.g., during frame construction before schematic
    // is loaded), just return. The tree will be updated later when the schematic is loaded.
    if( !m_frame->Schematic().HasHierarchy() )
        return;

    bool eventsWereBound = m_events_bound;

    if( eventsWereBound )
    {
        // Disable selection events
        Unbind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_PANE::onSelectSheetPath, this );
        Unbind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_PANE::onSelectSheetPath, this );
        Unbind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_PANE::onTreeItemRightClick, this );
        m_tree->Unbind( wxEVT_CONTEXT_MENU, &HIERARCHY_PANE::onContextMenu, this );

        m_events_bound = false;
    }

    SCH_SHEET_LIST hierarchy = m_frame->Schematic().Hierarchy();
    std::set<wxString> collapsedNodes = m_collapsedPaths;

    std::function<void( const wxTreeItemId& )> getCollapsedNodes =
            [&]( const wxTreeItemId& id )
            {
                wxCHECK_RET( id.IsOk(), wxT( "Invalid tree item" ) );

                TREE_ITEM_DATA* itemData = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( id ) );

                // Skip items without data (e.g., project root node)
                if( itemData && m_tree->ItemHasChildren( id ) && !m_tree->IsExpanded( id )
                    && hierarchy.HasPath( itemData->m_SheetPath.Path() ) )
                {
                    collapsedNodes.emplace( itemData->m_SheetPath.PathAsString() );
                    return;
                }

                wxTreeItemIdValue cookie;
                wxTreeItemId      child = m_tree->GetFirstChild( id, cookie );

                while( child.IsOk() )
                {
                    getCollapsedNodes( child );
                    child = m_tree->GetNextChild( id, cookie );
                }
            };

    // If we are clearing the tree, don't try to get collapsed nodes as they
    // might be deleted
    if( !aClear && !m_tree->IsEmpty() )
    {
        collapsedNodes.clear();
        getCollapsedNodes( m_tree->GetRootItem() );
    }

    m_tree->DeleteAllItems();

    // Create project root node (not associated with virtual root sheet)
    wxTreeItemId projectRoot = m_tree->AddRoot( getRootString(), 0, 1 );
    // Don't set item data for the project root - it doesn't correspond to a real sheet

    // Get all top-level sheets
    std::vector<SCH_SHEET*> topLevelSheets = m_frame->Schematic().GetTopLevelSheets();

    // For each top-level sheet, build its hierarchy under the project root
    for( SCH_SHEET* sheet : topLevelSheets )
    {
        if( sheet )
        {
            m_list.clear();
            m_list.push_back( sheet );

            wxString sheetNameBase = sheet->GetShownName( false );

            // If the sheet name is empty, use the filename (without extension) as fallback
            if( sheetNameBase.IsEmpty() && sheet->GetScreen() )
            {
                wxFileName fn( sheet->GetScreen()->GetFileName() );
                sheetNameBase = fn.GetName();
            }

            // Create tree item for this top-level sheet
            wxString sheetName = formatPageString( sheetNameBase, m_list.GetPageNumber() );
            wxTreeItemId topLevelItem = m_tree->AppendItem( projectRoot, sheetName, 0, 1 );
            m_tree->SetItemData( topLevelItem, new TREE_ITEM_DATA( m_list ) );

            // Build hierarchy for this top-level sheet
            buildHierarchyTree( &m_list, topLevelItem );
        }
    }

    UpdateHierarchySelection();

    m_tree->ExpandAll();

    std::function<void( const wxTreeItemId& )> collapseNodes =
            [&]( const wxTreeItemId& id )
            {
                wxCHECK_RET( id.IsOk(), wxT( "Invalid tree item" ) );

                TREE_ITEM_DATA* itemData =
                        static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( id ) );

                if( id != projectRoot && itemData &&
                    collapsedNodes.find( itemData->m_SheetPath.PathAsString() ) != collapsedNodes.end() )
                {
                    m_tree->Collapse( id );
                    return;
                }

                wxTreeItemIdValue cookie;
                wxTreeItemId      child = m_tree->GetFirstChild( id, cookie );

                while( child.IsOk() )
                {
                    collapseNodes( child );
                    child = m_tree->GetNextChild( id, cookie );
                }
            };

    collapseNodes( projectRoot );
    m_collapsedPaths = std::move( collapsedNodes );

    if( eventsWereBound )
    {
        // Enable selection events
        Bind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_PANE::onSelectSheetPath, this );
        Bind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_PANE::onSelectSheetPath, this );
        Bind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_PANE::onTreeItemRightClick, this );
        m_tree->Bind( wxEVT_CONTEXT_MENU, &HIERARCHY_PANE::onContextMenu, this );

        m_events_bound = true;
    }
}


void HIERARCHY_PANE::onSelectSheetPath( wxTreeEvent& aEvent )
{
    wxTreeItemId  itemSel = m_tree->GetSelection();

    if( !itemSel.IsOk() )
        return;

    TREE_ITEM_DATA* itemData = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( itemSel ) );

    if( !itemData )
        return;

    SetCursor( wxCURSOR_ARROWWAIT );
    m_frame->GetToolManager()->RunAction<SCH_SHEET_PATH*>( SCH_ACTIONS::changeSheet, &itemData->m_SheetPath );
    SetCursor( wxCURSOR_ARROW );
}


void HIERARCHY_PANE::UpdateLabelsHierarchyTree()
{
    // Update the labels of the hierarchical tree of the schematic.
    // Must be called only for an up to date tree, to update displayed labels after
    // a sheet name or a sheet number change.

    std::function<void( const wxTreeItemId& )> updateLabel =
            [&]( const wxTreeItemId& id )
            {
                TREE_ITEM_DATA* itemData = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( id ) );

                if( !itemData )     // happens if not shown in wxTreeCtrl m_tree (virtual sheet)
                    return;

                SCH_SHEET* sheet = itemData->m_SheetPath.Last();
                wxString   sheetNameBase = sheet->GetField( FIELD_T::SHEET_NAME )->GetShownText( false );
                wxString   sheetName = formatPageString( sheetNameBase,
                                                         itemData->m_SheetPath.GetPageNumber() );

                if( m_tree->GetItemText( id ) != sheetName )
                    m_tree->SetItemText( id, sheetName );
            };

    wxTreeItemId rootId  = m_tree->GetRootItem();
    updateLabel( rootId );

    std::function<void( const wxTreeItemId& )> recursiveDescent =
            [&]( const wxTreeItemId& id )
            {
                wxCHECK_RET( id.IsOk(), wxT( "Invalid tree item" ) );
                wxTreeItemIdValue cookie;
                wxTreeItemId      child = m_tree->GetFirstChild( id, cookie );

                while( child.IsOk() )
                {
                    updateLabel( child );
                    recursiveDescent( child );
                    child = m_tree->GetNextChild( id, cookie );
                }
            };

    recursiveDescent( rootId );
}


std::vector<wxString> HIERARCHY_PANE::GetCollapsedPaths() const
{
    std::vector<wxString> collapsed;

    if( m_tree->IsEmpty() )
        return collapsed;

    std::function<void( const wxTreeItemId& )> collect =
            [&]( const wxTreeItemId& id )
            {
                wxCHECK_RET( id.IsOk(), wxT( "Invalid tree item" ) );

                TREE_ITEM_DATA* itemData = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( id ) );

                if( id != m_tree->GetRootItem() && m_tree->ItemHasChildren( id )
                    && !m_tree->IsExpanded( id ) )
                {
                    collapsed.push_back( itemData->m_SheetPath.PathAsString() );
                    return;
                }

                wxTreeItemIdValue cookie;
                wxTreeItemId      child = m_tree->GetFirstChild( id, cookie );

                while( child.IsOk() )
                {
                    collect( child );
                    child = m_tree->GetNextChild( id, cookie );
                }
            };

    collect( m_tree->GetRootItem() );
    return collapsed;
}


void HIERARCHY_PANE::onTreeItemRightClick( wxTreeEvent& aEvent )
{
    onRightClick( aEvent.GetItem() );
}


void HIERARCHY_PANE::onContextMenu( wxContextMenuEvent& aEvent )
{
    // Handle right-click in empty space - treat as if clicking on an invalid item
    onRightClick( wxTreeItemId() );
}


void HIERARCHY_PANE::onRightClick( wxTreeItemId aItem )
{
    wxMenu          ctxMenu;
    TREE_ITEM_DATA* itemData = nullptr;
    bool            isProjectRoot = false;

    if( !aItem.IsOk() )
        aItem = m_tree->GetSelection();

    if( aItem.IsOk() )
    {
        itemData = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( aItem ) );
        isProjectRoot = ( m_tree->GetRootItem() == aItem.GetID() );
    }

    if( itemData )
    {
        ctxMenu.Append( EDIT_PAGE_NUMBER, _( "Edit Page Number" ) );
        // The root item cannot be renamed
        if( !isProjectRoot )
        {
            ctxMenu.Append( RENAME, _( "Rename" ), _( "Change name of this sheet" ) );

            // Allow deleting top-level sheets (but not sub-sheets)
            if( itemData->m_SheetPath.size() == 1 )
            {
                ctxMenu.Append( DELETE_TOP_LEVEL_SHEET, _( "Delete Top-Level Sheet" ),
                                _( "Remove this top-level sheet from the project" ) );
            }
        }

        ctxMenu.AppendSeparator();
    }

    // Always allow creating new top-level sheets (root is hidden with wxTR_HIDE_ROOT)
    ctxMenu.Append( NEW_TOP_LEVEL_SHEET, _( "New Top-Level Sheet" ), _( "Create a new top-level sheet" ) );
    ctxMenu.AppendSeparator();

    ctxMenu.Append( EXPAND_ALL, ACTIONS::expandAll.GetMenuItem() );
    ctxMenu.Append( COLLAPSE_ALL, ACTIONS::collapseAll.GetMenuItem() );


    int selected = GetPopupMenuSelectionFromUser( ctxMenu );

    switch( selected )
    {
    case NEW_TOP_LEVEL_SHEET:
    {
        // Create a new top-level sheet
        wxTextEntryDialog dlg( m_frame, _( "Enter name for new top-level sheet:" ),
                                        _( "New Top-Level Sheet" ),
                                        _( "Untitled" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString newName = dlg.GetValue();

            if( !newName.IsEmpty() )
            {
                SCH_COMMIT commit( m_frame );

                // Create new sheet and screen
                SCH_SHEET* newSheet = new SCH_SHEET( &m_frame->Schematic() );
                SCH_SCREEN* newScreen = new SCH_SCREEN( &m_frame->Schematic() );

                newSheet->SetScreen( newScreen );
                newSheet->GetField( FIELD_T::SHEET_NAME )->SetText( newName );

                // Generate a unique filename
                wxString filename = newName;
                filename.Replace( " ", "_" );
                filename = filename.Lower();

                if( !filename.EndsWith( ".kicad_sch" ) )
                    filename += ".kicad_sch";

                newScreen->SetFileName( filename );

                // Calculate the next page number
                int nextPage = m_frame->Schematic().GetTopLevelSheets().size() + 1;
                newScreen->SetPageNumber( wxString::Format( "%d", nextPage ) );

                // Add to schematic
                m_frame->Schematic().AddTopLevelSheet( newSheet );

                commit.Push( _( "Add new top-level sheet" ) );

                // Refresh the hierarchy tree
                UpdateHierarchyTree();
            }
        }
        break;
    }

    case DELETE_TOP_LEVEL_SHEET:
    {
        if( itemData && itemData->m_SheetPath.size() == 1 )
        {
            SCH_SHEET* sheet = itemData->m_SheetPath.Last();

            // Confirm deletion
            wxString msg = wxString::Format( _( "Delete top-level sheet '%s'?\n\nThis cannot be undone." ),
                                             sheet->GetName() );

            if( wxMessageBox( msg, _( "Delete Top-Level Sheet" ), wxYES_NO | wxICON_QUESTION, m_frame ) == wxYES )
            {
                // Don't allow deleting the last top-level sheet
                if( m_frame->Schematic().GetTopLevelSheets().size() <= 1 )
                {
                    wxMessageBox( _( "Cannot delete the last top-level sheet." ), _( "Delete Top-Level Sheet" ),
                                  wxOK | wxICON_ERROR, m_frame );
                    break;
                }

                SCH_COMMIT commit( m_frame );

                // Remove from schematic
                if( m_frame->Schematic().RemoveTopLevelSheet( sheet ) )
                {
                    commit.Push( _( "Delete top-level sheet" ) );

                    // Refresh the hierarchy tree
                    UpdateHierarchyTree();
                }
            }
        }
        break;
    }

    case EDIT_PAGE_NUMBER:
    {
        wxString msg;
        wxString sheetPath = itemData->m_SheetPath.PathHumanReadable( false, true );
        wxString pageNumber = itemData->m_SheetPath.GetPageNumber();

        msg.Printf( _( "Enter page number for sheet path %s" ),
                    ( sheetPath.Length() > 20 ) ? wxS( "   \n" ) + sheetPath + wxT( ":   " )
                                                : sheetPath + wxT( ":   " ) );

        wxTextEntryDialog dlg( m_frame, msg, _( "Edit Sheet Page Number" ), pageNumber );

        dlg.SetTextValidator( wxFILTER_ALPHANUMERIC );  // No white space.

        if( dlg.ShowModal() == wxID_OK && dlg.GetValue() != itemData->m_SheetPath.GetPageNumber() )
        {
            SCH_COMMIT commit( m_frame );
            SCH_SHEET_PATH parentPath = itemData->m_SheetPath;
            parentPath.pop_back();

            commit.Modify( itemData->m_SheetPath.Last(), parentPath.LastScreen() );

            itemData->m_SheetPath.SetPageNumber( dlg.GetValue() );

            if( itemData->m_SheetPath == m_frame->GetCurrentSheet() )
            {
                if( m_frame->GetScreen() )
                {
                    m_frame->GetScreen()->SetPageNumber( dlg.GetValue() );
                    m_frame->OnPageSettingsChange();
                }
            }

            commit.Push( wxS( "Change sheet page number." ) );

            UpdateLabelsHierarchyTree();
        }

        break;
    }
    case EXPAND_ALL:
        m_tree->ExpandAll();
        break;
    case COLLAPSE_ALL:
        m_tree->CollapseAll();
        break;
    case RENAME:
        m_tree->SetItemText( aItem, itemData->m_SheetPath.Last()->GetName() );
        m_tree->EditLabel( aItem );
        setIdenticalSheetsHighlighted( itemData->m_SheetPath );
        break;
    }
}


void HIERARCHY_PANE::onTreeEditFinished( wxTreeEvent& event )
{
    TREE_ITEM_DATA* data = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( event.GetItem() ) );
    wxString        newName = event.GetLabel();

    if( data && data->m_SheetPath.Last() )
    {
        if( !newName.IsEmpty() )
        {
            // The editor holds only the page name as a text, while normally
            // the tree items displaying it suffixed with the page number
            if( data->m_SheetPath.Last()->GetName() != newName )
            {
                SCH_COMMIT commit( m_frame );
                SCH_SCREEN* modifyScreen = nullptr;

                // For top-level sheets (size == 1), modify on the virtual root's screen
                // For sub-sheets, modify on the parent sheet's screen
                if( data->m_SheetPath.size() == 1 )
                {
                    modifyScreen = m_frame->Schematic().Root().GetScreen();
                }
                else
                {
                    const SCH_SHEET* parentSheet = data->m_SheetPath.GetSheet( data->m_SheetPath.size() - 2 );
                    if( parentSheet )
                        modifyScreen = parentSheet->GetScreen();
                }

                if( modifyScreen )
                {
                    commit.Modify( data->m_SheetPath.Last()->GetField( FIELD_T::SHEET_NAME ),
                                   modifyScreen );

                    data->m_SheetPath.Last()->SetName( newName );

                    renameIdenticalSheets( data->m_SheetPath, newName, &commit );

                    if( !commit.Empty() )
                        commit.Push( _( "Renaming sheet" ) );

                    if( data->m_SheetPath == m_frame->GetCurrentSheet() )
                    {
                        m_frame->OnPageSettingsChange();
                    }
                }
            }
        }

        m_tree->SetItemText( event.GetItem(), formatPageString( data->m_SheetPath.Last()->GetName(),
                                                                data->m_SheetPath.GetPageNumber() ) );
        setIdenticalSheetsHighlighted( data->m_SheetPath, false );
        // The event needs to be rejected otherwise the SetItemText call above
        // will be ineffective (the treeview item will hold the editor's content)
        event.Veto();
    }
}


void HIERARCHY_PANE::onCharHook( wxKeyEvent& aKeyStroke )
{
    int hotkey = aKeyStroke.GetKeyCode();

    int mods = aKeyStroke.GetModifiers();

    // the flag wxMOD_ALTGR is defined in wxWidgets as wxMOD_CONTROL|wxMOD_ALT
    // So AltGr key cannot used as modifier key because it is the same as Alt key + Ctrl key.
#if CAN_USE_ALTGR_KEY
    if( wxmods & wxMOD_ALTGR )
        mods |= MD_ALTGR;
    else
#endif
    {
        if( mods & wxMOD_CONTROL )
            hotkey += MD_CTRL;

        if( mods & wxMOD_ALT )
            hotkey += MD_ALT;
    }

    if( mods & wxMOD_SHIFT )
        hotkey += MD_SHIFT;

#ifdef wxMOD_META
    if( mods & wxMOD_META )
        hotkey += MD_META;
#endif

#ifdef wxMOD_WIN
    if( mods & wxMOD_WIN )
        hotkey += MD_SUPER;
#endif

    if( hotkey == ACTIONS::expandAll.GetHotKey()
        || hotkey == ACTIONS::expandAll.GetHotKeyAlt() )
    {
        m_tree->ExpandAll();
        return;
    }
    else if( hotkey == ACTIONS::collapseAll.GetHotKey()
             || hotkey == ACTIONS::collapseAll.GetHotKeyAlt() )
    {
        m_tree->CollapseAll();
        return;
    }
    else
    {
        aKeyStroke.Skip();
    }
}


wxString HIERARCHY_PANE::getRootString()
{
    // Pane may be repainting while schematic is in flux
    if ( !m_frame->Schematic().IsValid() )
        return _( "Schematic" );

    // Return the project name for the root node
    wxString projectName = m_frame->Schematic().Project().GetProjectName();

    if( projectName.IsEmpty() )
        projectName = _( "Schematic" );

    return projectName;
}


wxString HIERARCHY_PANE::formatPageString( const wxString& aName, const wxString& aPage )
{
    return aName + wxT( " " ) + wxString::Format( _( "(page %s)" ), aPage );
}

void HIERARCHY_PANE::setIdenticalSheetsHighlighted( const SCH_SHEET_PATH& path, bool highLighted )
{
    std::function<void( const wxTreeItemId& )> recursiveDescent = [&]( const wxTreeItemId& id )
    {
        wxCHECK_RET( id.IsOk(), wxT( "Invalid tree item" ) );

        TREE_ITEM_DATA* itemData = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( id ) );

        // Skip items without data (e.g., project root node)
        if( itemData && itemData->m_SheetPath.Cmp( path ) != 0 && itemData->m_SheetPath.Last() == path.Last() )
        {
            wxFont font = m_tree->GetItemFont( id );
            font.SetUnderlined( highLighted );
            m_tree->SetItemFont( id, font );
        }

        wxTreeItemIdValue cookie;
        wxTreeItemId      child = m_tree->GetFirstChild( id, cookie );

        while( child.IsOk() )
        {
            recursiveDescent( child );
            child = m_tree->GetNextChild( id, cookie );
        }
    };

    recursiveDescent( m_tree->GetRootItem() );
}

void HIERARCHY_PANE::renameIdenticalSheets( const SCH_SHEET_PATH& renamedSheet,
                                            const wxString newName, SCH_COMMIT* commit )
{
    std::function<void( const wxTreeItemId& )> recursiveDescent = [&]( const wxTreeItemId& id )
    {
        wxCHECK_RET( id.IsOk(), wxT( "Invalid tree item" ) );

        TREE_ITEM_DATA* data = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( id ) );

        // Skip items without data (e.g., project root node)
        if( !data )
        {
            wxTreeItemIdValue cookie;
            wxTreeItemId      child = m_tree->GetFirstChild( id, cookie );

            while( child.IsOk() )
            {
                recursiveDescent( child );
                child = m_tree->GetNextChild( id, cookie );
            }

            return;
        }

        // Check if this is an identical sheet that needs renaming (but not the renamed sheet itself)
        if( data->m_SheetPath.Cmp( renamedSheet ) != 0
            && data->m_SheetPath.Last() == renamedSheet.Last() )
        {
            SCH_SCREEN* modifyScreen = nullptr;

            // For top-level sheets (size == 1), modify on the virtual root's screen
            // For sub-sheets, modify on the parent sheet's screen
            if( data->m_SheetPath.size() == 1 )
            {
                modifyScreen = m_frame->Schematic().Root().GetScreen();
            }
            else
            {
                const SCH_SHEET* parentSheet = data->m_SheetPath.GetSheet( data->m_SheetPath.size() - 2 );
                if( parentSheet )
                    modifyScreen = parentSheet->GetScreen();
            }

            if( modifyScreen )
            {
                commit->Modify( data->m_SheetPath.Last()->GetField( FIELD_T::SHEET_NAME ),
                                modifyScreen );

                data->m_SheetPath.Last()->SetName( newName );

                if( data->m_SheetPath == m_frame->GetCurrentSheet() )
                {
                    m_frame->OnPageSettingsChange();
                }

                m_tree->SetItemText( id, formatPageString( data->m_SheetPath.Last()->GetName(),
                                                           data->m_SheetPath.GetPageNumber() ) );
            }
        }

        wxTreeItemIdValue cookie;
        wxTreeItemId      child = m_tree->GetFirstChild( id, cookie );

        while( child.IsOk() )
        {
            recursiveDescent( child );
            child = m_tree->GetNextChild( id, cookie );
        }
    };

    recursiveDescent( m_tree->GetRootItem() );
}
