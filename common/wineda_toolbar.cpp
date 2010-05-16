/****************/
/* toolbars.cpp */
/****************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "wxstruct.h"


WinEDA_Toolbar::WinEDA_Toolbar( id_toolbar type, wxWindow * parent,
                                wxWindowID id, bool horizontal ):
    wxAuiToolBar( parent, id, wxDefaultPosition, wxDefaultSize,
                  wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_OVERFLOW | ( ( horizontal ) ?
                                             wxAUI_TB_HORZ_LAYOUT :
                                             wxAUI_TB_VERTICAL ) )
{
    m_Parent = parent;
    m_Ident = type;
    m_Horizontal = horizontal;

    SetToolBitmapSize(wxSize(16,16));
    SetMargins(0,0);
    SetToolSeparation(1);
    SetToolPacking(1);
}


int WinEDA_Toolbar::GetDimension( )
{
    if( m_Horizontal )
        return GetSize().y;
    else
        return GetSize().x;
}
