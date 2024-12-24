/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_commit.h>
#include <schematic.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>

#include <hierarchy_pane.h>
#include <kiface_base.h>
#include <eeschema_settings.h>

#include <wx/object.h>
#include <wx/generic/textdlgg.h>
#include <wx/menu.h>

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

#ifdef __WXMAC__
    // HiDPI-aware API; will be generally available in wxWidgets 3.4
    wxVector<wxBitmapBundle> images;
    images.push_back( KiBitmapBundle( BITMAPS::tree_nosel ) );
    images.push_back( KiBitmapBundle( BITMAPS::tree_sel ) );
    m_tree->SetImages( images );
#else
    // Make an image list containing small icons
    // All icons are expected having the same size.
    wxBitmap tree_nosel_bm( KiBitmap( BITMAPS::tree_nosel ) );
    wxImageList* imageList = new wxImageList( tree_nosel_bm.GetWidth(), tree_nosel_bm.GetHeight(),
                                              true, 2 );

    imageList->Add( tree_nosel_bm );
    imageList->Add( KiBitmap( BITMAPS::tree_sel ) );

    m_tree->AssignImageList( imageList );
#endif

    sizer->Add( m_tree, 1, wxEXPAND, wxBORDER_NONE );

    m_events_bound = false;

    UpdateHierarchyTree();

    // Enable selection events
    Bind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_PANE::onSelectSheetPath, this );
    Bind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_PANE::onSelectSheetPath, this );
    Bind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_PANE::onTreeItemRightClick, this );
    Bind( wxEVT_CHAR_HOOK, &HIERARCHY_PANE::onCharHook, this );
    m_tree->Bind( wxEVT_TREE_END_LABEL_EDIT, &HIERARCHY_PANE::onTreeEditFinished, this );
    m_events_bound = true;
}


HIERARCHY_PANE::~HIERARCHY_PANE()
{
    Unbind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_PANE::onSelectSheetPath, this );
    Unbind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_PANE::onSelectSheetPath, this );
    Unbind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_PANE::onTreeItemRightClick, this );
    Unbind( wxEVT_CHAR_HOOK, &HIERARCHY_PANE::onCharHook, this );
    m_tree->Unbind( wxEVT_TREE_END_LABEL_EDIT, &HIERARCHY_PANE::onTreeEditFinished, this );
}


void HIERARCHY_PANE::buildHierarchyTree( SCH_SHEET_PATH* aList, const wxTreeItemId& aParent )
{
    std::vector<SCH_ITEM*> sheetChildren;
    aList->LastScreen()->GetSheets( &sheetChildren );

    for( SCH_ITEM* aItem : sheetChildren )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( aItem );
        aList->push_back( sheet );

        wxString     sheetNameBase = sheet->GetFields()[SHEETNAME].GetShownText( false );
        wxString     sheetName = formatPageString( sheetNameBase, aList->GetPageNumber() );
        wxString     sheetNumber = aList->GetPageNumber();
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

        m_events_bound = false;
    }

    std::function<void( const wxTreeItemId& )> recursiveDescent =
            [&]( const wxTreeItemId& id )
            {
                wxCHECK_RET( id.IsOk(), wxT( "Invalid tree item" ) );

                TREE_ITEM_DATA* itemData = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( id ) );

                if( itemData->m_SheetPath == m_frame->GetCurrentSheet() )
                {
                    wxTreeItemId parent = m_tree->GetItemParent( id );

                    if( parent.IsOk() && !m_tree->IsExpanded( parent ) )
                        m_tree->Expand( parent );

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

        m_events_bound = true;
    }
}


void HIERARCHY_PANE::UpdateHierarchyTree()
{
    Freeze();

    bool eventsWereBound = m_events_bound;

    if( eventsWereBound )
    {
        // Disable selection events
        Unbind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_PANE::onSelectSheetPath, this );
        Unbind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_PANE::onSelectSheetPath, this );
        Unbind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_PANE::onTreeItemRightClick, this );

        m_events_bound = false;
    }

    SCH_SHEET_LIST hierarchy = m_frame->Schematic().Hierarchy();
    std::set<SCH_SHEET_PATH> expandedNodes;

    std::function<void( const wxTreeItemId& )> getExpandedNodes =
            [&]( const wxTreeItemId& id )
            {
                wxCHECK_RET( id.IsOk(), wxT( "Invalid tree item" ) );

                TREE_ITEM_DATA* itemData = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( id ) );

                if( m_tree->IsExpanded( id ) && hierarchy.HasPath( itemData->m_SheetPath.Path() ) )
                    expandedNodes.emplace( itemData->m_SheetPath );

                wxTreeItemIdValue cookie;
                wxTreeItemId      child = m_tree->GetFirstChild( id, cookie );

                while( child.IsOk() )
                {
                    getExpandedNodes( child );
                    child = m_tree->GetNextChild( id, cookie );
                }
            };

    if( !m_tree->IsEmpty() )
        getExpandedNodes( m_tree->GetRootItem() );

    m_list.clear();
    m_list.push_back( &m_frame->Schematic().Root() );

    m_tree->DeleteAllItems();

    wxTreeItemId root = m_tree->AddRoot( getRootString(), 0, 1 );
    m_tree->SetItemData( root, new TREE_ITEM_DATA( m_list ) );

    buildHierarchyTree( &m_list, root );
    UpdateHierarchySelection();

    if( !expandedNodes.empty() )
    {
        std::function<void( const wxTreeItemId& )> expandNodes =
                [&]( const wxTreeItemId& id )
                {
                    wxCHECK_RET( id.IsOk(), wxT( "Invalid tree item" ) );

                    TREE_ITEM_DATA* itemData =
                            static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( id ) );

                    if( expandedNodes.find( itemData->m_SheetPath ) != expandedNodes.end() )
                        m_tree->Expand( id );

                    wxTreeItemIdValue cookie;
                    wxTreeItemId      child = m_tree->GetFirstChild( id, cookie );

                    while( child.IsOk() )
                    {
                        expandNodes( child );
                        child = m_tree->GetNextChild( id, cookie );
                    }
                };

        expandNodes( m_tree->GetRootItem() );
    }
    else if( m_tree->ItemHasChildren( root ) )
    {
        m_tree->Expand( root );
    }

    if( eventsWereBound )
    {
        // Enable selection events
        Bind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_PANE::onSelectSheetPath, this );
        Bind( wxEVT_TREE_SEL_CHANGED, &HIERARCHY_PANE::onSelectSheetPath, this );
        Bind( wxEVT_TREE_ITEM_RIGHT_CLICK, &HIERARCHY_PANE::onTreeItemRightClick, this );

        m_events_bound = true;
    }

    Thaw();
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
    m_frame->GetToolManager()->RunAction<SCH_SHEET_PATH*>( EE_ACTIONS::changeSheet,
                                                           &itemData->m_SheetPath );
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
                SCH_SHEET*      sheet = itemData->m_SheetPath.Last();
                wxString        sheetNameLabel =
                        formatPageString( sheet->GetFields()[SHEETNAME].GetShownText( false ),
                                          itemData->m_SheetPath.GetPageNumber() );

                if( m_tree->GetItemText( id ) != sheetNameLabel )
                    m_tree->SetItemText( id, sheetNameLabel );
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


void HIERARCHY_PANE::onTreeItemRightClick( wxTreeEvent& aEvent )
{
    onRightClick( aEvent.GetItem() );
}


void HIERARCHY_PANE::onRightClick( wxTreeItemId aItem )
{
    wxMenu          ctxMenu;
    TREE_ITEM_DATA* itemData = nullptr;

    if( !aItem.IsOk() )
        aItem = m_tree->GetSelection();

    if( aItem.IsOk() )
        itemData = static_cast<TREE_ITEM_DATA*>( m_tree->GetItemData( aItem ) );

    if( itemData )
    {
        ctxMenu.Append( EDIT_PAGE_NUMBER, _( "Edit Page Number" ) );
        // The root item cannot be renamed
        if( m_tree->GetRootItem() != aItem.GetID() )
        {
            ctxMenu.Append( RENAME, _( "Rename" ), _( "Change name of this sheet" ) );
        }

        ctxMenu.AppendSeparator();
    }
    ctxMenu.Append( EXPAND_ALL, ACTIONS::expandAll.GetMenuItem() );
    ctxMenu.Append( COLLAPSE_ALL, ACTIONS::collapseAll.GetMenuItem() );


    int selected = GetPopupMenuSelectionFromUser( ctxMenu );

    switch( selected )
    {
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
                m_frame->GetScreen()->SetPageNumber( dlg.GetValue() );
                m_frame->OnPageSettingsChange();
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

    if( !newName.IsEmpty() )
    {
        // The editor holds only the page name as a text, while normally
        // the tree items displaying it suffixed with the page number
        if( data->m_SheetPath.Last()->GetName() != newName )
        {
            const SCH_SHEET* parentSheet =
                    data->m_SheetPath.GetSheet( data->m_SheetPath.size() - 2 );

            if( parentSheet )
            {
                SCH_COMMIT commit( m_frame );
                commit.Modify( &data->m_SheetPath.Last()->GetFields()[SHEETNAME],
                               parentSheet->GetScreen() );

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


void HIERARCHY_PANE::onCharHook( wxKeyEvent& aKeyStroke )
{
    int hotkey = aKeyStroke.GetKeyCode();

    if( aKeyStroke.GetModifiers() & wxMOD_CONTROL )
        hotkey += MD_CTRL;

    if( aKeyStroke.GetModifiers() & wxMOD_ALT )
        hotkey += MD_ALT;

    if( aKeyStroke.GetModifiers() & wxMOD_SHIFT )
        hotkey += MD_SHIFT;

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
    SCH_SHEET*     rootSheet = &m_frame->Schematic().Root();
    SCH_SHEET_PATH rootPath;
    rootPath.push_back( rootSheet );

    return formatPageString( rootSheet->GetShownName( false ), rootPath.GetPageNumber() );
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

        if( itemData->m_SheetPath.Cmp( path ) != 0 && itemData->m_SheetPath.Last() == path.Last() )
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

        const SCH_SHEET* parentSheet = data->m_SheetPath.GetSheet( data->m_SheetPath.size() - 2 );

        if( parentSheet && data->m_SheetPath.Cmp( renamedSheet ) != 0
            && data->m_SheetPath.Last() == renamedSheet.Last() )
        {
            commit->Modify( &data->m_SheetPath.Last()->GetFields()[SHEETNAME],
                            parentSheet->GetScreen() );

            data->m_SheetPath.Last()->SetName( newName );

            if( data->m_SheetPath == m_frame->GetCurrentSheet() )
            {
                m_frame->OnPageSettingsChange();
            }

            m_tree->SetItemText( id, formatPageString( data->m_SheetPath.Last()->GetName(),
                                                       data->m_SheetPath.GetPageNumber() ) );
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
