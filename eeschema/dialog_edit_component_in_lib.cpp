/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_edit_component_in_lib.cpp
// Author:      jean-pierre Charras
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "libeditfrm.h"
#include "class_libentry.h"

#include "dialog_edit_component_in_lib.h"


DIALOG_EDIT_COMPONENT_IN_LIBRARY::DIALOG_EDIT_COMPONENT_IN_LIBRARY( WinEDA_LibeditFrame* aParent):
    DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE(aParent)
{
	m_Parent = aParent;
	m_RecreateToolbar = false;

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

    LIB_COMPONENT* component = m_Parent->GetComponent();

    if( component == NULL )
    {
        SetTitle( _( "Library Component Properties" ) );
        return;
    }

    wxString title = _( "Properties for " );

    if( !m_Parent->GetAliasName().IsEmpty() )
    {
        title += m_Parent->GetAliasName() + _( " (alias of " ) +
            component->GetName() + wxT( ")" );
    }
    else
    {
        title += component->GetName();
        m_Parent->GetAliasName().Empty();
    }

    SetTitle( title );
    InitPanelDoc();
    InitBasicPanel();

    if( !m_Parent->GetAliasName().IsEmpty() )
        m_ButtonDeleteAllAlias->Enable( false );

    /* Place list of alias names in listbox */
    m_PartAliasList->Append( component->m_AliasList );

    if( component->m_AliasList.GetCount() == 0 )
    {
        m_ButtonDeleteAllAlias->Enable( false );
        m_ButtonDeleteOneAlias->Enable( false );
    }

    /* Read the Footprint Filter list */
    m_FootprintFilterListBox->Append( component->m_FootprintList );

    if( component->m_FootprintList.GetCount() == 0 )
    {
        m_ButtonDeleteAllFootprintFilter->Enable( false );
        m_ButtonDeleteOneFootprintFilter->Enable( false );
    }
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnCancelClick( wxCommandEvent& event )
{
	EndModal( wxID_CANCEL );
}

