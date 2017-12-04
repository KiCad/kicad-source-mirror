/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2017 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file hierarch.cpp
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <id.h>
#include <bitmaps.h>
#include <dialog_shim.h>

#include <schframe.h>
#include <general.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>

#include <wx/imaglist.h>
#include <wx/treectrl.h>

#include <class_netlist_object.h>
#include <sch_sheet_path.h>


enum
{
    ID_TREECTRL_HIERARCHY = 1600
};


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


/**
 * Handle hierarchy tree control.
 */
class HIERARCHY_TREE : public wxTreeCtrl
{
private:
    HIERARCHY_NAVIG_DLG* m_Parent;
    wxImageList*      imageList;

public:
    HIERARCHY_TREE()
    {
        m_Parent = NULL;
        imageList = NULL;
    }

    HIERARCHY_TREE( HIERARCHY_NAVIG_DLG* parent );

    DECLARE_DYNAMIC_CLASS( HIERARCHY_TREE )
};


IMPLEMENT_DYNAMIC_CLASS( HIERARCHY_TREE, wxTreeCtrl )


HIERARCHY_TREE::HIERARCHY_TREE( HIERARCHY_NAVIG_DLG* parent ) :
    wxTreeCtrl( (wxWindow*)parent, ID_TREECTRL_HIERARCHY, wxDefaultPosition, wxDefaultSize,
                wxTR_HAS_BUTTONS, wxDefaultValidator, wxT( "HierachyTreeCtrl" ) )
{
    m_Parent = parent;

    // Make an image list containing small icons
    // All icons are expected having the same size.
    wxBitmap tree_nosel_bm( KiBitmap( tree_nosel_xpm ) );
    imageList = new wxImageList( tree_nosel_bm.GetWidth(),
                                 tree_nosel_bm.GetHeight(), true, 2 );

    imageList->Add( tree_nosel_bm );
    imageList->Add( KiBitmap( tree_sel_xpm ) );

    AssignImageList( imageList );
}


class HIERARCHY_NAVIG_DLG : public DIALOG_SHIM
{
public:
    HIERARCHY_TREE* m_Tree;
    int             m_nbsheets;

private:
    wxSize m_TreeSize;
    int    maxposx;

public:
    HIERARCHY_NAVIG_DLG( SCH_EDIT_FRAME* aParent, const wxPoint& aPos );

    ~HIERARCHY_NAVIG_DLG();

    /**
     * Create the hierarchical tree of the schematic.
     *
     * This routine is re-entrant!
     */
    void BuildTree( SCH_SHEET_PATH* list, wxTreeItemId* previousmenu );

    /**
     * Open the selected sheet and display the corresponding screen when a tree item is
     * selected.
     */
    void OnSelect( wxTreeEvent& event );

private:
    void OnQuit( wxCommandEvent& event );
};


void SCH_EDIT_FRAME::InstallHierarchyFrame( wxPoint& pos )
{
    HIERARCHY_NAVIG_DLG* treeframe = new HIERARCHY_NAVIG_DLG( this, pos );

    treeframe->ShowQuasiModal();
    treeframe->Destroy();
}


HIERARCHY_NAVIG_DLG::HIERARCHY_NAVIG_DLG( SCH_EDIT_FRAME* aParent, const wxPoint& aPos ) :
    DIALOG_SHIM( aParent, wxID_ANY, _( "Navigator" ), wxDefaultPosition, wxDefaultSize,
                 wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    wxASSERT( dynamic_cast< SCH_EDIT_FRAME* >( aParent ) );

    wxTreeItemId root;

    m_Tree = new HIERARCHY_TREE( this );

    m_nbsheets = 1;

    root = m_Tree->AddRoot( _( "Root" ), 0, 1 );
    m_Tree->SetItemBold( root, true );

    SCH_SHEET_PATH list;
    list.push_back( g_RootSheet );
    m_Tree->SetItemData( root, new TreeItemData( list ) );

    if( dynamic_cast< SCH_EDIT_FRAME* >( aParent )->GetCurrentSheet().Last() == g_RootSheet )
        m_Tree->SelectItem( root ); //root.

    maxposx = 15;
    BuildTree( &list, &root );

    m_Tree->Expand( root );

    wxRect itemrect;
    m_Tree->GetBoundingRect( root, itemrect );

    // Set dialog window size to be large enough
    m_TreeSize.x = itemrect.GetWidth() + 20;
    m_TreeSize.x = std::max( m_TreeSize.x, 250 );

    // Readjust the size of the frame to an optimal value.
    m_TreeSize.y = m_nbsheets * itemrect.GetHeight();
    m_TreeSize.y += 10;

    SetClientSize( m_TreeSize );

    Bind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_NAVIG_DLG::OnSelect, this );
}


HIERARCHY_NAVIG_DLG::~HIERARCHY_NAVIG_DLG()
{
    Unbind( wxEVT_TREE_ITEM_ACTIVATED, &HIERARCHY_NAVIG_DLG::OnSelect, this );
}


void HIERARCHY_NAVIG_DLG::OnQuit( wxCommandEvent& event )
{
    // true is to force the frame to close
    Close( true );
}


void HIERARCHY_NAVIG_DLG::BuildTree( SCH_SHEET_PATH* list, wxTreeItemId*  previousmenu )

{
    wxTreeItemId menu;

    wxCHECK_RET( m_nbsheets < NB_MAX_SHEET, "Maximum number of sheets exceeded." );

    maxposx += m_Tree->GetIndent();
    SCH_ITEM* schitem = list->LastDrawList();

    while( schitem && m_nbsheets < NB_MAX_SHEET )
    {
        if( schitem->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) schitem;
            m_nbsheets++;
            menu = m_Tree->AppendItem( *previousmenu, sheet->GetName(), 0, 1 );
            list->push_back( sheet );
            m_Tree->SetItemData( menu, new TreeItemData( *list ) );
            int ll = m_Tree->GetItemText( menu ).Len();

#ifdef __WINDOWS__
            ll *= 9;    //  * char width
#else
            ll *= 12;   //  * char width
#endif
            ll += maxposx + 20;
            m_TreeSize.x  = std::max( m_TreeSize.x, ll );
            m_TreeSize.y += 1;

            SCH_EDIT_FRAME* parent = dynamic_cast< SCH_EDIT_FRAME* >( GetParent() );

            if( parent && *list == parent->GetCurrentSheet() )
            {
                m_Tree->EnsureVisible( menu );
                m_Tree->SelectItem( menu );
            }

            BuildTree( list, &menu );
            m_Tree->Expand( menu );
            list->pop_back();
        }

        schitem = schitem->Next();
    }

    maxposx -= m_Tree->GetIndent();
}


void HIERARCHY_NAVIG_DLG::OnSelect( wxTreeEvent& event )

{
    wxTreeItemId ItemSel = m_Tree->GetSelection();
    SCH_EDIT_FRAME* parent = dynamic_cast< SCH_EDIT_FRAME* >( GetParent() );

    wxCHECK2_MSG( parent, Close( true ),
                  "Parent window of hierarchy dialog is not SCH_EDIT_FRAME." );

    parent->SetCurrentSheet(( (TreeItemData*) m_Tree->GetItemData( ItemSel ) )->m_SheetPath );
    parent->DisplayCurrentSheet();
    Close( true );
}


void SCH_EDIT_FRAME::DisplayCurrentSheet()
{
    SetRepeatItem( NULL );
    ClearMsgPanel();

    SCH_SCREEN* screen = m_CurrentSheet->LastScreen();

    // Switch to current sheet,
    // and update the grid size, because it can be modified in latest screen
    SetScreen( screen );
    GetScreen()->SetGrid( m_LastGridSizeId + ID_POPUP_GRID_LEVEL_1000 );

    // update the References
    m_CurrentSheet->UpdateAllScreenReferences();
    SetSheetNumberAndCount();
    m_canvas->SetCanStartBlock( -1 );

    if( screen->m_FirstRedraw )
    {
        Zoom_Automatique( false );
        screen->m_FirstRedraw = false;
        SetCrossHairPosition( GetScrollCenterPosition() );
        m_canvas->MoveCursorToCrossHair();

        // Ensure the schematic is fully segmented on first display
        BreakSegmentsOnJunctions();
        SchematicCleanUp( true );
        screen->ClearUndoORRedoList( screen->m_UndoList, 1 );

        screen->TestDanglingEnds();
    }
    else
    {
        RedrawScreen( GetScrollCenterPosition(), true );
    }

    // Some items (wires, labels) can be highlighted. So prepare the highlight flag:
    SetCurrentSheetHighlightFlags();

    // Now refresh m_canvas. Should be not necessary, but because screen has changed
    // the previous refresh has set all new draw parameters (scroll position ..)
    // but most of time there were some inconsitencies about cursor parameters
    // ( previous position of cursor ...) and artefacts can happen
    // mainly when sheet size has changed
    // This second refresh clears artefacts because at this point,
    // all parameters are now updated
    m_canvas->Refresh();
}
