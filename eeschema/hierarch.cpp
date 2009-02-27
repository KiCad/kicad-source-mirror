/************************************************************************/
/* hierarch.cpp: Gestion  de la hierarchie: navigation dans les feuilles */
/************************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

//#include "protos.h"

#include "wx/image.h"
#include "wx/imaglist.h"
#include "wx/treectrl.h"


#include "bitmaps.h"


static bool UpdateScreenFromSheet( WinEDA_SchematicFrame* frame );

enum {
    ID_TREECTRL_HIERARCHY = 1600
};


class WinEDA_HierFrame;

/* This class derived from wxTreeItemData stores the DrawSheetPath of each sheet in hierarcy
 * in each TreeItem, in its associated data buffer
*/
class TreeItemData : public wxTreeItemData
{
public:
    DrawSheetPath m_SheetPath;
    TreeItemData( DrawSheetPath sheet ) : wxTreeItemData()
    {
        m_SheetPath = sheet;
    }
};

/* Classe de l'arbre de hierarchie */
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


/* Classe definissant la fenetre d'affichage de la hierarchie */
class WinEDA_HierFrame : public wxDialog
{
public:
    WinEDA_SchematicFrame* m_Parent;
    WinEDA_Tree*           m_Tree;
    int    m_nbsheets;
    wxDC*  m_DC;

private:
    wxSize m_TreeSize;  // Taille de l'arbre de hierarchie
    int    maxposx;

public:
    WinEDA_HierFrame( WinEDA_SchematicFrame* parent, wxDC* DC, const wxPoint& pos );
    void BuildSheetsTree( DrawSheetPath* list, wxTreeItemId* previousmenu );

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


/*************************************************************************/
void WinEDA_SchematicFrame::InstallHierarchyFrame( wxDC* DC, wxPoint& pos )
/*************************************************************************/
{
    WinEDA_HierFrame* treeframe = new WinEDA_HierFrame( this, DC, pos );

    treeframe->ShowModal(); treeframe->Destroy();
}


WinEDA_HierFrame::WinEDA_HierFrame( WinEDA_SchematicFrame* parent, wxDC* DC,
                                    const wxPoint& pos ) :
    wxDialog( parent, -1, _( "Navigator" ), pos, wxSize( 110, 50 ),
              DIALOG_STYLE )
{
    wxTreeItemId cellule;

    m_Parent = parent;
    m_DC   = DC;
    m_Tree = new WinEDA_Tree( this );

    m_nbsheets = 1;

    cellule = m_Tree->AddRoot( _( "Root" ), 0, 1 );
    m_Tree->SetItemBold( cellule, TRUE );
    DrawSheetPath list;
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

        // Reajustage de la taille de la frame a une valeur optimale
        m_TreeSize.y += m_nbsheets * itemrect.GetHeight();
        m_TreeSize.x  = MIN( m_TreeSize.x, 250 );
        m_TreeSize.y  = MIN( m_TreeSize.y, 350 );
        SetClientSize( m_TreeSize );
    }
}


WinEDA_HierFrame::~WinEDA_HierFrame()
{
}


/************************************************************************/
void WinEDA_HierFrame::OnQuit( wxCommandEvent& WXUNUSED (event) )
/************************************************************************/
{
    // true is to force the frame to close
    Close( true );
}


/********************************************************************/
void WinEDA_HierFrame::BuildSheetsTree( DrawSheetPath* list,
                                       wxTreeItemId*  previousmenu )
/********************************************************************/

/* Routine de creation de l'arbre de navigation dans la hierarchy
  * schematique
  * Cette routine est Reentrante !
 */
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
        if( schitem->Type() == DRAW_SHEET_STRUCT_TYPE )
        {
            DrawSheetStruct* sheet = (DrawSheetStruct*) schitem;
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


/***************************************************/
void WinEDA_HierFrame::OnSelect( wxTreeEvent& event )
/***************************************************/

/* Called on a double-click on a tree item:
  * Open the selected sheet, and display the corresponding screen
 */
{
    wxTreeItemId ItemSel = m_Tree->GetSelection();

    *(m_Parent->m_CurrentSheet) =
        ( (TreeItemData*) m_Tree->GetItemData( ItemSel ) )->m_SheetPath;
    UpdateScreenFromSheet( m_Parent );
    Close( TRUE );
}


/******************************************************/
void WinEDA_SchematicFrame::InstallPreviousSheet()
/******************************************************/

/* Set the current screen to display the parent sheet of the current displayed sheet
 */
{
    if( m_CurrentSheet->Last() == g_RootSheet )
        return;

    g_ItemToRepeat = NULL;
    MsgPanel->EraseMsgBox();

    //make a copy for testing purposes.
    DrawSheetPath listtemp = *m_CurrentSheet;
    listtemp.Pop();
    if( listtemp.LastScreen() == NULL )
    {
        DisplayError( this, wxT( "InstallPreviousScreen() Error: Sheet not found" ) );
        return;
    }
    m_CurrentSheet->Pop();
    UpdateScreenFromSheet( this );
}


/*********************************************************************/
void WinEDA_SchematicFrame::InstallNextScreen( DrawSheetStruct* Sheet )
/*********************************************************************/

/* Routine d'installation de l'ecran correspondant au symbole Sheet pointe
  * par la souris
  * have to be careful here because the DrawSheetStructs within the EEDrawList
  * don't actually have a valid m_AssociatedScreen (on purpose -- you need the m_SubSheet hierarchy
  * to maintain path info (well, this is but one way to maintain path info..)
 */
{
    if( Sheet == NULL )
    {
        DisplayError( this, wxT( "InstallNextScreen() error" ) ); return;
    }
    m_CurrentSheet->Push( Sheet );
    g_ItemToRepeat = NULL;
    MsgPanel->EraseMsgBox();
    UpdateScreenFromSheet( this );
}


/**************************************************************/
static bool UpdateScreenFromSheet( WinEDA_SchematicFrame* frame )
/**************************************************************/

/* Recherche et installe de l'ecran relatif au sheet symbole Sheet.
  * Si Sheet == NULL installation de l'ecran de base ( Root ).
 */

{
    SCH_SCREEN* NewScreen;

    NewScreen = frame->m_CurrentSheet->LastScreen();
    if( !NewScreen )
    {
        DisplayError( frame, wxT( "Screen not found for this sheet" ) );
        return false;
    }

    // Reinit des parametres d'affichage du nouvel ecran
    // assumes m_CurrentSheet has already been updated.
    frame->MsgPanel->EraseMsgBox();
    frame->DrawPanel->SetScrollbars( NewScreen->m_ZoomScalar,
                                     NewScreen->m_ZoomScalar,
                                     NewScreen->m_ScrollbarNumber.x,
                                     NewScreen->m_ScrollbarNumber.y,
                                     NewScreen->m_ScrollbarPos.x,
                                     NewScreen->m_ScrollbarPos.y, TRUE );

    //update the References
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
