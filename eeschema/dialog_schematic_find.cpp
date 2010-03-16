#include "dialog_schematic_find.h"


DEFINE_EVENT_TYPE( EVT_COMMAND_FIND_DRC_MARKER )
DEFINE_EVENT_TYPE( EVT_COMMAND_FIND_COMPONENT_IN_LIB )


DIALOG_SCH_FIND::DIALOG_SCH_FIND( wxWindow* aParent, wxFindReplaceData* aData,
                                  const wxPoint& aPosition, const wxSize& aSize, int aStyle ) :
    DIALOG_SCH_FIND_BASE( aParent, wxID_ANY, _( "Find" ), aPosition, aSize,
                          wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | aStyle )
{
    SetData( aData );

    wxASSERT_MSG( m_findReplaceData, wxT( "can't create find dialog without data" ) );

    if( aStyle & wxFR_REPLACEDIALOG )
    {
        m_staticReplace->Show( true );
        m_comboReplace->Show( true );
    }

    int flags = m_findReplaceData->GetFlags();
    m_radioForward->SetValue( flags & wxFR_DOWN );
    m_radioBackward->SetValue( ( flags & wxFR_DOWN ) == 0 );
    m_checkMatchCase->SetValue( flags & wxFR_MATCHCASE );
    m_checkWholeWord->SetValue( flags & wxFR_WHOLEWORD );

    /* Whole work and wild card searches are mutually exclusive. */
    if( !( flags & wxFR_WHOLEWORD ) )
        m_checkWildcardMatch->SetValue( flags & FR_MATCH_WILDCARD );

    m_checkAllFields->SetValue( flags & FR_SEARCH_ALL_FIELDS );
    m_checkWrap->SetValue( flags & FR_SEARCH_WRAP );
    m_checkCurrentSheetOnly->SetValue( flags & FR_CURRENT_SHEET_ONLY );

    m_comboFind->SetFocus();

    SetPosition( aPosition );
    SetSize( aSize );
}


void DIALOG_SCH_FIND::OnClose( wxCloseEvent& aEvent )
{
    SendEvent( wxEVT_COMMAND_FIND_CLOSE );
}


void DIALOG_SCH_FIND::OnUpdateFindUI( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( !m_comboFind->GetValue().empty() );
}


void DIALOG_SCH_FIND::OnUpdateWholeWordUI( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( !m_checkWildcardMatch->GetValue() );
}


void DIALOG_SCH_FIND::OnUpdateWildcardUI( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( !m_checkWholeWord->GetValue() );
}


void DIALOG_SCH_FIND::OnFind( wxCommandEvent& aEvent )
{
    int index = m_comboFind->FindString( m_comboFind->GetValue(), true );

    if( index == wxNOT_FOUND )
    {
        m_comboFind->Insert( m_comboFind->GetValue(), 0 );
    }
    else if( index != 0 )
    {
        /* Move the search string to the top of the list if it isn't already there. */
        wxString tmp = m_comboFind->GetValue();
        m_comboFind->Delete( index );
        m_comboFind->Insert( tmp, 0 );
        m_comboFind->SetSelection( 0 );
    }

    SendEvent( wxEVT_COMMAND_FIND );
}


void DIALOG_SCH_FIND::OnCancel( wxCommandEvent& aEvent )
{
    SendEvent( wxEVT_COMMAND_FIND_CLOSE );
    Show( false );
}


void DIALOG_SCH_FIND::SendEvent( const wxEventType& aEventType )
{
    wxFindDialogEvent event( aEventType, GetId() );
    event.SetEventObject( this );
    event.SetFindString( m_comboFind->GetValue() );

    if ( HasFlag( wxFR_REPLACEDIALOG ) )
    {
        event.SetReplaceString( m_comboReplace->GetValue() );
    }

    int flags = 0;

    if( m_radioForward->GetValue() )
        flags |= wxFR_DOWN;

    if( m_checkMatchCase->GetValue() )
        flags |= wxFR_MATCHCASE;

    if( m_checkWholeWord->GetValue() )
        flags |= wxFR_WHOLEWORD;

    if( m_checkWildcardMatch->GetValue() )
        flags |= FR_MATCH_WILDCARD;

    if( m_checkAllFields->GetValue() )
        flags |= FR_SEARCH_ALL_FIELDS;

    if( m_checkWrap->GetValue() )
        flags |= FR_SEARCH_WRAP;

    if( m_checkCurrentSheetOnly->GetValue() )
        flags |= FR_CURRENT_SHEET_ONLY;

    m_findReplaceData->SetFindString( event.GetFindString() );

    if( HasFlag( wxFR_REPLACEDIALOG )
        && ( event.GetEventType() == wxEVT_COMMAND_FIND_REPLACE
             || event.GetEventType() == wxEVT_COMMAND_FIND_REPLACE_ALL ) )
    {
        m_findReplaceData->SetReplaceString( event.GetReplaceString() );
    }

    event.SetFlags( flags );

    m_findReplaceData->SetFlags( event.GetFlags() );

    if( !GetEventHandler()->ProcessEvent( event ) )
    {
        GetParent()->GetEventHandler()->ProcessEvent( event );
    }
}


wxArrayString DIALOG_SCH_FIND::GetFindEntries() const
{
    return m_comboFind->GetStrings();
}


void DIALOG_SCH_FIND::SetFindEntries( const wxArrayString& aEntries )
{
    m_comboFind->Append( aEntries );

    if( !m_comboFind->IsEmpty() )
        m_comboFind->SetSelection( 0 );
}


void DIALOG_SCH_FIND::SetReplaceEntries( const wxArrayString& aEntries )
{
    m_comboReplace->Append( aEntries );

    if( !m_comboReplace->IsEmpty() )
        m_comboReplace->SetSelection( 0 );
}
