	/******************************************************************/
	/* toolbars.cpp - fonctions des classes du type WinEDA_ttolbar */
	/******************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"


#include "id.h"

	/*************************/
	/* class WinEDA_HToolbar */
	/*************************/

WinEDA_Toolbar::WinEDA_Toolbar(id_toolbar type, wxWindow * parent,
				wxWindowID id, bool horizontal):
			wxToolBar(parent,id, wxPoint(-1,-1), wxSize(-1,-1),
						horizontal ? wxTB_HORIZONTAL : wxTB_VERTICAL)
{
	m_Parent = parent;
	Pnext = NULL;
	m_Ident = type;
	m_Horizontal = horizontal;
	m_Size = 24;

	SetToolBitmapSize(wxSize(16,16));
	SetMargins(0,0);
	SetToolSeparation(1);
	SetToolPacking(1);
}


