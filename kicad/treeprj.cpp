/***************/
/* treeprj.cpp */
/***************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"

#include "kicad.h"
#include "protos.h"

#include "wx/image.h"
#include "wx/imaglist.h"
#include "wx/treectrl.h"

#include "bitmaps.h"

#include "id.h"

enum {
	TREE_PROJECT = 1,
	TREE_SCHEMA,
	TREE_PCB
};

/***********************************************************/
/* Classes pour l'arbre de hierarchie de gestion du projet */
/***********************************************************/
class TreePrjItemData: public wxTreeItemData
{
public:
	int m_Id;
	wxString m_FileName;

public:
	TreePrjItemData(int tree_id, const wxString & data) :wxTreeItemData()
	{
		m_Id = tree_id;
		if ( data ) m_FileName = data;
	}
};



/**********************************************************************/
/* Methodes de la Frame de l'arbre de hierarchie de gestion du projet */
/**********************************************************************/

WinEDA_PrjFrame::WinEDA_PrjFrame(WinEDA_MainFrame * parent,
				const wxPoint & pos,
				const wxSize & size ) :
			 wxSashLayoutWindow(parent, ID_LEFT_FRAME, pos, size,
				 wxNO_BORDER|wxSW_3D)
{
	m_Parent = parent;
	m_TreeProject = NULL;
	ReCreateTreePrj();
}

BEGIN_EVENT_TABLE(WinEDA_PrjFrame, wxSashLayoutWindow)
	EVT_TREE_ITEM_ACTIVATED(ID_PROJECT_TREE, WinEDA_PrjFrame::OnSelect)
END_EVENT_TABLE()


/******************************************/
void WinEDA_PrjFrame::ReCreateTreePrj(void)
/******************************************/
/* Create or modify the tree showing root schematic and pcb file names
*/
{
wxTreeItemId rootcellule;
wxTreeItemId cellule;
wxString Text;
int id;

	if ( ! m_TreeProject ) m_TreeProject = new WinEDA_TreePrj(this);
	else m_TreeProject->DeleteAllItems();

	m_TreeProject->SetFont(* g_StdFont);
	if (m_Parent->m_PrjFileName.IsEmpty() ) Text = wxT("noname");
	else Text = wxFileNameFromPath(m_Parent->m_PrjFileName);

	id = 0;
	rootcellule = m_TreeProject->AddRoot(Text, id, id);
	m_TreeProject->SetItemBold(rootcellule, TRUE);
	m_TreeProject->SetItemData( rootcellule, new TreePrjItemData(TREE_PROJECT, wxEmptyString) );
	m_TreeProject->SetItemFont(rootcellule, *g_StdFont);

	ChangeFileNameExt(Text, wxEmptyString);

	id++;
	cellule = m_TreeProject->AppendItem(rootcellule,Text + g_SchExtBuffer, id, id);
	m_TreeProject->SetItemData( cellule, new TreePrjItemData(TREE_SCHEMA, wxEmptyString) );
	m_TreeProject->SetItemFont(cellule, *g_StdFont);
	g_SchematicRootFileName = m_TreeProject->GetItemText(cellule);

	id++;
	cellule = m_TreeProject->AppendItem(rootcellule,Text + g_BoardExtBuffer, id, id);
	m_TreeProject->SetItemData( cellule, new TreePrjItemData(TREE_PCB, wxEmptyString) );
	m_TreeProject->SetItemFont(cellule, *g_StdFont);
	g_BoardFileName = m_TreeProject->GetItemText(cellule);

	m_TreeProject->Expand(rootcellule);
}

/**************************************************/
void WinEDA_PrjFrame::OnSelect(wxTreeEvent & Event)
/**************************************************/
{
int tree_id;
TreePrjItemData * tree_data;
wxString FullFileName;

	tree_data = (TreePrjItemData*)
		m_TreeProject->GetItemData(m_TreeProject->GetSelection());

	tree_id = tree_data->m_Id;
	FullFileName = m_Parent->m_PrjFileName;

	switch ( tree_id )
		{
		case TREE_PROJECT:
			break;

		case TREE_SCHEMA:
			{
			ChangeFileNameExt(FullFileName, g_SchExtBuffer);
			AddDelimiterString( FullFileName );
			ExecuteFile(this, EESCHEMA_EXE, FullFileName);
			break;
			}

		case TREE_PCB:
			ChangeFileNameExt(FullFileName, g_BoardExtBuffer);
			AddDelimiterString( FullFileName );
			ExecuteFile(this, PCBNEW_EXE, FullFileName);
			break;
		}
}


/********************************************/
/* Methodes pour l'arbre  gestion du projet */
/********************************************/

WinEDA_TreePrj::WinEDA_TreePrj(WinEDA_PrjFrame * parent) :
	wxTreeCtrl(parent, ID_PROJECT_TREE,
				wxDefaultPosition, wxDefaultSize,
                wxTR_HAS_BUTTONS, wxDefaultValidator, wxT("EDATreeCtrl"))
{
	m_Parent = parent;
	// Make an image list containing small icons
	m_ImageList = new wxImageList(16, 16, TRUE, 2);

	m_ImageList->Add(wxBitmap(kicad_icon_small_xpm));
	m_ImageList->Add(wxBitmap(eeschema_xpm));
	m_ImageList->Add(wxBitmap(pcbnew_xpm));

	SetImageList(m_ImageList);

}

/********************************/
WinEDA_TreePrj::~WinEDA_TreePrj()
/********************************/
{
}


