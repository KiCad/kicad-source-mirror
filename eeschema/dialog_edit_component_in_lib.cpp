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
    m_PartAliasListCtrl->Append( component->GetAliasList() );

    if( component->GetAliasList().GetCount() == 0 )
    {
        m_ButtonDeleteAllAlias->Enable( false );
        m_ButtonDeleteOneAlias->Enable( false );
    }

    /* Read the Footprint Filter list */
    m_FootprintFilterListBox->Append( component->GetFootPrints() );

    if( component->GetFootPrints().GetCount() == 0 )
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
    LIB_COMPONENT* component = m_Parent->GetComponent();

    if( component == NULL )
        return;

    wxString aliasname = m_Parent->GetAliasName();
    if( aliasname.IsEmpty() )       // The root component is selected
    {
        m_DocCtrl->SetValue( component->GetDescription() );
        m_KeywordsCtrl->SetValue( component->GetKeyWords() );
        m_DocfileCtrl->SetValue( component->GetDocFileName() );
    }
    else    // An alias is currently selected
    {
        m_DocCtrl->SetValue( component->GetAliasDataDoc( aliasname ) );
        m_KeywordsCtrl->SetValue( component->GetAliasDataKeyWords( aliasname ) );
        m_DocfileCtrl->SetValue( component->GetAliasDataDocFileName( aliasname ) );
    }
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

    m_ShowPinNumButt->SetValue( component->ShowPinNumbers() );
    m_ShowPinNameButt->SetValue( component->ShowPinNames() );
    m_PinsNameInsideButt->SetValue( component->GetPinNameOffset() != 0 );
    m_SelNumberOfUnits->SetValue( component->GetPartCount() );
    m_SetSkew->SetValue( component->GetPinNameOffset() );
    m_OptionPower->SetValue( component->isPower() );
    m_OptionPartsLocked->SetValue( component->UnitsLocked() );
}
