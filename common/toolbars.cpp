/******************************************************************/
/* toolbars.cpp - fonctions des classes du type WinEDA_ttolbar */
/******************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "wxstruct.h"

/*************************/
/* class WinEDA_HToolbar */
/*************************/

WinEDA_Toolbar::WinEDA_Toolbar( id_toolbar type, wxWindow * parent,
                                wxWindowID id, bool horizontal ):
#if defined(KICAD_AUITOOLBAR)
    wxAuiToolBar( parent, id, wxDefaultPosition, wxDefaultSize,
               wxAUI_TB_DEFAULT_STYLE | (horizontal ? wxAUI_TB_HORZ_LAYOUT : wxAUI_TB_VERTICAL))
#else
    wxToolBar( parent, id, wxPoint( -1,-1 ), wxSize( -1,-1 ),
               horizontal ? wxTB_HORIZONTAL : wxTB_VERTICAL )
#endif
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
