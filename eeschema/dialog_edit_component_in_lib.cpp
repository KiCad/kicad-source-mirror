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
#include "libeditframe.h"
#include "class_library.h"
//#include "class_libentry.h"

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
    m_PartAliasListCtrl->Append( component->m_AliasList );

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



void DIALOG_EDIT_COMPONENT_IN_LIBRARY::InitPanelDoc()
{
    CMP_LIB_ENTRY* entry;
    LIB_COMPONENT* component = m_Parent->GetComponent();
    CMP_LIBRARY* library = m_Parent->GetLibrary();

    if( component == NULL )
        return;

    if( m_Parent->GetAliasName().IsEmpty() )
    {
        entry = component;
    }
    else
    {
        entry =
            ( CMP_LIB_ENTRY* ) library->FindAlias( m_Parent->GetAliasName() );

        if( entry == NULL )
            return;
    }

    m_DocCtrl->SetValue( entry->GetDescription() );
    m_KeywordsCtrl->SetValue( entry->GetKeyWords() );
    m_DocfileCtrl->SetValue( entry->GetDocFileName() );
}


/*
 * create the basic panel for component properties editing
 */
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::InitBasicPanel()
{
    LIB_COMPONENT* component = m_Parent->GetComponent();

    if( m_Parent->GetShowDeMorgan() )
        m_AsConvertButt->SetValue( true );

    /* Default values for a new component. */
    if( component == NULL )
    {
        m_ShowPinNumButt->SetValue( true );
        m_ShowPinNameButt->SetValue( true );
        m_PinsNameInsideButt->SetValue( true );
        m_SelNumberOfUnits->SetValue( 1 );
        m_SetSkew->SetValue( 40 );
        m_OptionPower->SetValue( false );
        m_OptionPartsLocked->SetValue( false );
        return;
    }

    m_ShowPinNumButt->SetValue( component->m_DrawPinNum );
    m_ShowPinNameButt->SetValue( component->m_DrawPinName );
    m_PinsNameInsideButt->SetValue( component->m_TextInside != 0 );
    m_SelNumberOfUnits->SetValue( component->GetPartCount() );
    m_SetSkew->SetValue( component->m_TextInside );
    m_OptionPower->SetValue( component->isPower() );
    m_OptionPartsLocked->SetValue( component->m_UnitSelectionLocked );
}
