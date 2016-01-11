/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
#include <schframe.h>

#include <general.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>

#include <wx/imaglist.h>
#include <wx/treectrl.h>


enum
{
    ID_TREECTRL_HIERARCHY = 1600
};


class HIERARCHY_NAVIG_DLG;

/* This class derived from wxTreeItemData stores the SCH_SHEET_PATH of each
 * sheet in hierarchy in each TreeItem, in its associated data buffer
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

/* Class to handle hierarchy tree. */
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


class HIERARCHY_NAVIG_DLG : public wxDialog
{
public:
    SCH_EDIT_FRAME* m_Parent;
    HIERARCHY_TREE* m_Tree;
    int             m_nbsheets;

private:
    wxSize m_TreeSize;
    int    maxposx;

public:
    HIERARCHY_NAVIG_DLG( SCH_EDIT_FRAME* aParent, const wxPoint& aPos );
    void BuildSheetsTree( SCH_SHEET_PATH* list, wxTreeItemId* previousmenu );

    ~HIERARCHY_NAVIG_DLG();

    void OnSelect( wxTreeEvent& event );

private:
    void OnQuit( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( HIERARCHY_NAVIG_DLG, wxDialog )
    EVT_TREE_ITEM_ACTIVATED( ID_TREECTRL_HIERARCHY, HIERARCHY_NAVIG_DLG::OnSelect )
END_EVENT_TABLE()


void SCH_EDIT_FRAME::InstallHierarchyFrame( wxPoint& pos )
{
    HIERARCHY_NAVIG_DLG* treeframe = new HIERARCHY_NAVIG_DLG( this, pos );

    treeframe->ShowModal();
    treeframe->Destroy();
}


HIERARCHY_NAVIG_DLG::HIERARCHY_NAVIG_DLG( SCH_EDIT_FRAME* aParent, const wxPoint& aPos ) :
    wxDialog( aParent, wxID_ANY, _( "Navigator" ), aPos, wxSize( 110, 50 ),
    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    wxTreeItemId cellule;

    m_Parent = aParent;
    m_Tree = new HIERARCHY_TREE( this );

    m_nbsheets = 1;

    cellule = m_Tree->AddRoot( _( "Root" ), 0, 1 );
    m_Tree->SetItemBold( cellule, true );

    SCH_SHEET_PATH list;
    list.Push( g_RootSheet );
    m_Tree->SetItemData( cellule, new TreeItemData( list ) );

    if( m_Parent->GetCurrentSheet().Last() == g_RootSheet )
        m_Tree->SelectItem( cellule ); //root.

    maxposx = 15;
    BuildSheetsTree( &list, &cellule );

    m_Tree->Expand( cellule );

    wxRect itemrect;
    m_Tree->GetBoundingRect( cellule, itemrect );

    // Set dialog window size to be large enough
    m_TreeSize.x = itemrect.GetWidth() + 20;
    m_TreeSize.x = std::max( m_TreeSize.x, 250 );

    // Readjust the size of the frame to an optimal value.
    m_TreeSize.y = m_nbsheets * itemrect.GetHeight();
    m_TreeSize.y += 10;

    SetClientSize( m_TreeSize );
}


HIERARCHY_NAVIG_DLG::~HIERARCHY_NAVIG_DLG()
{
}


void HIERARCHY_NAVIG_DLG::OnQuit( wxCommandEvent& event )
{
    // true is to force the frame to close
    Close( true );
}


/* Routine to create the hierarchical tree of the schematic
 * This routine is re-entrant!
 */
void HIERARCHY_NAVIG_DLG::BuildSheetsTree( SCH_SHEET_PATH* list, wxTreeItemId*  previousmenu )

{
    wxTreeItemId menu;

    if( m_nbsheets > NB_MAX_SHEET )
    {
        if( m_nbsheets == (NB_MAX_SHEET + 1) )
        {
            wxString msg;
            msg << wxT( "BuildSheetsTree: Error: nbsheets > " ) << NB_MAX_SHEET;
            DisplayError( this, msg );
            m_nbsheets++;
        }

        return;
    }

    maxposx += m_Tree->GetIndent();
    SCH_ITEM* schitem = list->LastDrawList();

    while( schitem && m_nbsheets < NB_MAX_SHEET )
    {
        if( schitem->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) schitem;
            m_nbsheets++;
            menu = m_Tree->AppendItem( *previousmenu, sheet->GetName(), 0, 1 );
            list->Push( sheet );
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

            if( *list == m_Parent->GetCurrentSheet() )
            {
                m_Tree->EnsureVisible( menu );
                m_Tree->SelectItem( menu );
            }

            BuildSheetsTree( list, &menu );
            m_Tree->Expand( menu );
            list->Pop();
        }

        schitem = schitem->Next();
    }

    maxposx -= m_Tree->GetIndent();
}


/* Called on a double-click on a tree item:
 * Open the selected sheet, and display the corresponding screen
 */
void HIERARCHY_NAVIG_DLG::OnSelect( wxTreeEvent& event )

{
    wxTreeItemId ItemSel = m_Tree->GetSelection();

    m_Parent->SetCurrentSheet(( (TreeItemData*) m_Tree->GetItemData( ItemSel ) )->m_SheetPath );
    m_Parent->DisplayCurrentSheet();
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
    m_CurrentSheet->Last()->UpdateAllScreenReferences();
    SetSheetNumberAndCount();
    m_canvas->SetCanStartBlock( -1 );

    if( screen->m_FirstRedraw )
    {
        Zoom_Automatique( false );
        screen->m_FirstRedraw = false;
        SetCrossHairPosition( GetScrollCenterPosition() );
        m_canvas->MoveCursorToCrossHair();
        screen->SchematicCleanUp( GetCanvas(), NULL );
    }
    else
    {
        RedrawScreen( GetScrollCenterPosition(), true );
    }

    // Now refresh m_canvas. Should be not necessary, but because screen has changed
    // the previous refresh has set all new draw parameters (scroll position ..)
    // but most of time there were some inconsitencies about cursor parameters
    // ( previous position of cursor ...) and artefacts can happen
    // mainly when sheet size has changed
    // This second refresh clears artefacts because at this point,
    // all parameters are now updated
    m_canvas->Refresh();
}
