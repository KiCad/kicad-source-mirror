/*********************************/
/*  dialog_get_component.cpp     */
/*********************************/

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <draw_frame.h>
#include <dialog_get_component.h>


/****************************************************************************/
/* Show a dialog frame to choose a name from an history list, or a new name */
/* to select a component or a module                                        */
/****************************************************************************/

static unsigned s_HistoryMaxCount = 8;  // Max number of items displayed in history list


/*
 * Dialog frame to choose a component or a footprint
 *   This dialog shows an history of last selected items
 */
DIALOG_GET_COMPONENT::DIALOG_GET_COMPONENT( EDA_DRAW_FRAME* parent,
                                           wxArrayString&  HistoryList,
                                            const wxString& Title,
                                            bool            show_extra_tool ) :
    DIALOG_GET_COMPONENT_BASE( parent, -1, Title )
{

#ifdef __WXMAC__
    m_auxToolSelector = false;
#else
    m_auxToolSelector = show_extra_tool;
#endif
    initDialog( HistoryList );

    m_textCmpNameCtrl->SetFocus();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}

void DIALOG_GET_COMPONENT::initDialog( wxArrayString& aHistoryList )
{
    SetFocus();
    m_GetExtraFunction = false;
    m_selectionIsKeyword = false;
    m_historyList->Append( aHistoryList );
    if( !m_auxToolSelector )
    {
        m_buttonBrowse->Show( false );
        m_buttonBrowse->Enable( false );
    }
}


void DIALOG_GET_COMPONENT::OnCancel( wxCommandEvent& event )
{
    m_Text = wxEmptyString;
    EndModal( wxID_CANCEL );
}

void DIALOG_GET_COMPONENT::Accept( wxCommandEvent& event )
{
    m_selectionIsKeyword = false;
    switch( event.GetId() )
    {
    case ID_SEL_BY_LISTBOX:
        m_Text = m_historyList->GetStringSelection();
        break;

    case wxID_OK:
        m_Text = m_textCmpNameCtrl->GetValue();
        break;

    case ID_ACCEPT_KEYWORD:
        m_selectionIsKeyword = true;
        m_Text = m_textCmpNameCtrl->GetValue();
        break;

    case ID_LIST_ALL:
        m_Text = wxT( "*" );
        break;
    }

    m_Text.Trim( false );      // Remove blanks at beginning
    m_Text.Trim( true );       // Remove blanks at end

    EndModal( wxID_OK );
}


/* Get the component name by the extra function */
void DIALOG_GET_COMPONENT::GetExtraSelection( wxCommandEvent& event )
{
    m_GetExtraFunction = true;
    EndModal( wxID_OK );
}


// Return the component name selected by the dialog
wxString DIALOG_GET_COMPONENT::GetComponentName( void )
{
    return m_Text;
}


/* Initialize the default component name default choice
*/
void DIALOG_GET_COMPONENT::SetComponentName( const wxString& name )
{
    if( m_textCmpNameCtrl )
    {
        m_textCmpNameCtrl->SetValue( name );
        m_textCmpNameCtrl->SetSelection(-1, -1);
    }
}


/*
 * Add the string "aName" to the history list aHistoryList
 */
void AddHistoryComponentName( wxArrayString& aHistoryList, const wxString& aName )
{
    if( ( aHistoryList.GetCount() > 0 ) && ( aName == aHistoryList[0] ) )
        return;

    /* remove an old identical name if exists */
    for( unsigned ii = 1; ii < aHistoryList.GetCount(); ii++ )
    {
        if( aName == aHistoryList[ii] )
        {
            aHistoryList.RemoveAt( ii );
            ii--;
        }
    }

    // Add the new name at the beginning of the history list
    aHistoryList.Insert(aName, 0);

    // Remove extra names
    while( aHistoryList.GetCount() >= s_HistoryMaxCount )
        aHistoryList.RemoveAt( aHistoryList.GetCount()-1 );
}
