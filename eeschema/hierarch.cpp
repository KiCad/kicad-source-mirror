/******************/
/*  hierarch.cpp  */
/******************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "bitmaps.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "sch_sheet.h"
#include "sch_sheet_path.h"

#include "wx/imaglist.h"
#include "wx/treectrl.h"


static bool UpdateScreenFromSheet( SCH_EDIT_FRAME* frame );

enum
{
    ID_TREECTRL_HIERARCHY = 1600
};


class WinEDA_HierFrame;

/* This class derived from wxTreeItemData stores the SCH_SHEET_PATH of each
 * sheet in hierarchy in each TreeItem, in its associated data buffer
*/
class TreeItemData : public wxTreeItemData
{
public:
    SCH_SHEET_PATH m_SheetPath;
    TreeItemData( SCH_SHEET_PATH sheet ) : wxTreeItemData()
    {
        m_SheetPath = sheet;
    }
};

/* Class to handle hierarchy tree. */
class WinEDA_Tree : public wxTreeCtrl
{
private:
    WinEDA_HierFrame* m_Parent;
    wxImageList*      imageList;

public:
    WinEDA_Tree() { }
    WinEDA_Tree( WinEDA_HierFrame* parent );

    DECLARE_DYNAMIC_CLASS( WinEDA_Tree )
};

IMPLEMENT_DYNAMIC_CLASS( WinEDA_Tree, wxTreeCtrl )


WinEDA_Tree::WinEDA_Tree( WinEDA_HierFrame* parent ) :
    wxTreeCtrl( (wxWindow*)parent, ID_TREECTRL_HIERARCHY,
                wxDefaultPosition, wxDefaultSize,
                wxTR_HAS_BUTTONS, wxDefaultValidator, wxT( "HierachyTreeCtrl" ) )
{
    m_Parent = parent;

    // Make an image list containing small icons
    imageList = new wxImageList( 16, 15, TRUE, 2 );

    imageList->Add( wxBitmap( tree_nosel_xpm ) );
    imageList->Add( wxBitmap( tree_sel_xpm ) );

    AssignImageList( imageList );
}


class WinEDA_HierFrame : public wxDialog
{
public:
    SCH_EDIT_FRAME* m_Parent;
    WinEDA_Tree*    m_Tree;
    int             m_nbsheets;
    wxDC*           m_DC;

private:
    wxSize m_TreeSize;
    int    maxposx;

public:
    WinEDA_HierFrame( SCH_EDIT_FRAME* parent, wxDC* DC, const wxPoint& pos );
    void BuildSheetsTree( SCH_SHEET_PATH* list, wxTreeItemId* previousmenu );

    ~WinEDA_HierFrame();

    void OnSelect( wxTreeEvent& event );

private:
    void OnQuit( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_HierFrame, wxDialog )
    EVT_TREE_ITEM_ACTIVATED( ID_TREECTRL_HIERARCHY,
                             WinEDA_HierFrame::OnSelect )
END_EVENT_TABLE()


void SCH_EDIT_FRAME::InstallHierarchyFrame( wxDC* DC, wxPoint& pos )
{
    WinEDA_HierFrame* treeframe = new WinEDA_HierFrame( this, DC, pos );

    treeframe->ShowModal();
    treeframe->Destroy();
}


WinEDA_HierFrame::WinEDA_HierFrame( SCH_EDIT_FRAME* parent, wxDC* DC, const wxPoint& pos ) :
    wxDialog( parent, -1, _( "Navigator" ), pos, wxSize( 110, 50 ), DIALOG_STYLE )
{
    wxTreeItemId cellule;

    m_Parent = parent;
    m_DC   = DC;
    m_Tree = new WinEDA_Tree( this );

    m_nbsheets = 1;

    cellule = m_Tree->AddRoot( _( "Root" ), 0, 1 );
    m_Tree->SetItemBold( cellule, TRUE );
    SCH_SHEET_PATH list;
    list.Push( g_RootSheet );
    m_Tree->SetItemData( cellule, new TreeItemData( list ) );

    wxRect itemrect;
#ifdef __UNIX__
    itemrect.SetWidth( 100 );
    itemrect.SetHeight( 20 );
#else
    m_Tree->GetBoundingRect( cellule, itemrect );
#endif
    m_TreeSize.x = itemrect.GetWidth() + 10;
    m_TreeSize.y = 20;

    if( m_Parent->GetSheet()->Last() == g_RootSheet )
        m_Tree->SelectItem( cellule ); //root.

    maxposx = 15;
    BuildSheetsTree( &list, &cellule );

    if( m_nbsheets > 1 )
    {
        m_Tree->Expand( cellule );

        // Readjust the size of the frame to an optimal value.
        m_TreeSize.y += m_nbsheets * itemrect.GetHeight();
        m_TreeSize.x  = MIN( m_TreeSize.x, 250 );
        m_TreeSize.y  = MIN( m_TreeSize.y, 350 );
        SetClientSize( m_TreeSize );
    }
}


WinEDA_HierFrame::~WinEDA_HierFrame()
{
}


void WinEDA_HierFrame::OnQuit( wxCommandEvent& WXUNUSED (event) )
{
    // true is to force the frame to close
    Close( true );
}


/* Routine to create the tree in the navigation hierarchy
 * Schematic
 * This routine is re-entrant!
 */
void WinEDA_HierFrame::BuildSheetsTree( SCH_SHEET_PATH* list,
                                        wxTreeItemId*  previousmenu )

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
            menu = m_Tree->AppendItem( *previousmenu, sheet->m_SheetName, 0, 1 );
            list->Push( sheet );
            m_Tree->SetItemData( menu, new TreeItemData( *list ) );
            int ll = m_Tree->GetItemText( menu ).Len();
#ifdef __WINDOWS__
            ll *= 9;    //  * char width
#else
            ll *= 12;   //  * char width
#endif
            ll += maxposx + 20;
            m_TreeSize.x  = MAX( m_TreeSize.x, ll );
            m_TreeSize.y += 1;
            if( *list == *( m_Parent->GetSheet() ) )
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
void WinEDA_HierFrame::OnSelect( wxTreeEvent& event )

{
    wxTreeItemId ItemSel = m_Tree->GetSelection();

    *(m_Parent->m_CurrentSheet) =
        ( (TreeItemData*) m_Tree->GetItemData( ItemSel ) )->m_SheetPath;
    UpdateScreenFromSheet( m_Parent );
    Close( TRUE );
}


/* Set the current screen to display the parent sheet of the current
 * displayed sheet
 */
void SCH_EDIT_FRAME::InstallPreviousSheet()
{
    if( m_CurrentSheet->Last() == g_RootSheet )
        return;

    g_ItemToRepeat = NULL;
    ClearMsgPanel();

    //make a copy for testing purposes.
    SCH_SHEET_PATH listtemp = *m_CurrentSheet;
    listtemp.Pop();

    if( listtemp.LastScreen() == NULL )
    {
        DisplayError( this, wxT( "InstallPreviousScreen() Error: Sheet not found" ) );
        return;
    }

    m_CurrentSheet->Pop();
    UpdateScreenFromSheet( this );
}


/* Routine installation of the screen corresponding to the symbol edge Sheet
 * Be careful here because the SCH_SHEETs within the GetDrawItems()
 * don't actually have a valid m_AssociatedScreen (on purpose -- you need the
 * m_SubSheet hierarchy to maintain path info (well, this is but one way to
 * maintain path info..)
 */
void SCH_EDIT_FRAME::InstallNextScreen( SCH_SHEET* Sheet )
{
    if( Sheet == NULL )
    {
        DisplayError( this, wxT( "InstallNextScreen() error" ) ); return;
    }
    m_CurrentSheet->Push( Sheet );
    g_ItemToRepeat = NULL;
    ClearMsgPanel();
    UpdateScreenFromSheet( this );
}


/* Find and install the screen on the sheet symbol Sheet.
 * If Sheet == NULL installation of the screen base (Root).
 */
static bool UpdateScreenFromSheet( SCH_EDIT_FRAME* frame )
{
    SCH_SCREEN* NewScreen;

    NewScreen = frame->m_CurrentSheet->LastScreen();
    if( !NewScreen )
    {
        DisplayError( frame, wxT( "Screen not found for this sheet" ) );
        return false;
    }

    // Reset display settings of the new screen
    // Assumes m_CurrentSheet has already been updated.
    frame->ClearMsgPanel();
    frame->DrawPanel->SetScrollbars( NewScreen->m_ScrollPixelsPerUnitX,
                                     NewScreen->m_ScrollPixelsPerUnitY,
                                     NewScreen->m_ScrollbarNumber.x,
                                     NewScreen->m_ScrollbarNumber.y,
                                     NewScreen->m_ScrollbarPos.x,
                                     NewScreen->m_ScrollbarPos.y, TRUE );

    // update the References
    frame->m_CurrentSheet->UpdateAllScreenReferences();
    frame->SetSheetNumberAndCount();
    frame->DrawPanel->m_CanStartBlock = -1;
    ActiveScreen = frame->m_CurrentSheet->LastScreen();

    if( NewScreen->m_FirstRedraw )
    {
        NewScreen->m_FirstRedraw = FALSE;
        frame->Zoom_Automatique( TRUE );
    }
    else
    {
        frame->DrawPanel->MouseToCursorSchema();
    }

    frame->DrawPanel->Refresh();
    return true;
}
