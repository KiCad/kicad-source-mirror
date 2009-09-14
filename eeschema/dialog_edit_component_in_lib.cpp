/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_edit_component_in_lib.cpp
// Author:      jean-pierre Charras
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "dialog_edit_component_in_lib.h"

#include "protos.h"

DIALOG_EDIT_COMPONENT_IN_LIBRARY::DIALOG_EDIT_COMPONENT_IN_LIBRARY( WinEDA_LibeditFrame* aParent):
    DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE(aParent)
{
	m_Parent = aParent;
	m_RecreateToolbar = false;
	m_AliasLocation = -1;

	Init();

    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
}


DIALOG_EDIT_COMPONENT_IN_LIBRARY::~DIALOG_EDIT_COMPONENT_IN_LIBRARY()
{
}

/* Initialize state of check boxes and texts
*/
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::Init( )
{
    SetFocus();
    m_AliasLocation = -1;

    if( CurrentLibEntry == NULL )
    {
        SetTitle( _( "Library Component Properties" ) );
        return;
    }

    wxString title = _( "Properties for " );

    if( !CurrentAliasName.IsEmpty() )
    {
        title += CurrentAliasName + _( " (alias of " ) +
            wxString( CurrentLibEntry->m_Name.m_Text )+ wxT( ")" );
    }
    else
    {
        title += CurrentLibEntry->m_Name.m_Text;
        CurrentAliasName.Empty();
    }

    SetTitle( title );

    InitPanelDoc();
    InitBasicPanel();

    if( !CurrentAliasName.IsEmpty() )
        m_ButtonDeleteAllAlias->Enable( false );

    /* Place list of alias names in listbox */
    if( CurrentLibEntry )
    {
        m_PartAliasList->Append( CurrentLibEntry->m_AliasList );
    }

    if( ( CurrentLibEntry == NULL )
        || ( CurrentLibEntry->m_AliasList.GetCount() == 0 ) )
    {
        m_ButtonDeleteAllAlias->Enable( false );
        m_ButtonDeleteOneAlias->Enable( false );
    }

    /* Read the Footprint Filter list */
    if( CurrentLibEntry )
    {
        m_FootprintFilterListBox->Append( CurrentLibEntry->m_FootprintList );
    }

    if( ( CurrentLibEntry == NULL )
        || ( CurrentLibEntry->m_FootprintList.GetCount() == 0 ) )
    {
        m_ButtonDeleteAllFootprintFilter->Enable( false );
        m_ButtonDeleteOneFootprintFilter->Enable( false );
    }
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnCancelClick( wxCommandEvent& event )
{
	EndModal( wxID_CANCEL );
}

