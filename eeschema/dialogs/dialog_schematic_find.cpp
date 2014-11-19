/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2010-2011 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file dialog_schematic_find.cpp
 * @brief Schematic find and replace dialog implementation.
 */

#include <dialog_schematic_find.h>


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
        SetTitle( _( "Find and Replace" ) );
        m_buttonReplace->Show( true );
        m_buttonReplaceAll->Show( true );
        m_staticReplace->Show( true );
        m_comboReplace->Show( true );
        m_checkReplaceReferences->Show( true );
        m_checkWildcardMatch->Show( false );  // Wildcard replace is not implemented.
    }

    int flags = m_findReplaceData->GetFlags();
    m_radioForward->SetValue( flags & wxFR_DOWN );
    m_radioBackward->SetValue( ( flags & wxFR_DOWN ) == 0 );
    m_checkMatchCase->SetValue( flags & wxFR_MATCHCASE );
    m_checkWholeWord->SetValue( flags & wxFR_WHOLEWORD );
    m_checkNoWarpCursor->SetValue( flags & FR_NO_WARP_CURSOR );

    /* Whole word and wild card searches are mutually exclusive. */
    if( !( flags & wxFR_WHOLEWORD ) )
        m_checkWildcardMatch->SetValue( flags & FR_MATCH_WILDCARD );

    m_checkAllFields->SetValue( flags & FR_SEARCH_ALL_FIELDS );
    m_checkReplaceReferences->SetValue( flags & FR_REPLACE_REFERENCES );
    m_checkAllPins->SetValue( flags & FR_SEARCH_ALL_PINS );
    m_checkWrap->SetValue( flags & FR_SEARCH_WRAP );
    m_checkCurrentSheetOnly->SetValue( flags & FR_CURRENT_SHEET_ONLY );

    m_buttonFind->SetDefault();
    m_comboFind->SetFocus();
    SetPosition( aPosition );

    // Adjust the height of the dialog to prevent controls from being hidden when
    // switching between the find and find/replace modes of the dialog.  This ignores
    // the users preferred height if any of the controls would be hidden.
    GetSizer()->SetSizeHints( this );
    wxSize size = aSize;

    if( aSize != wxDefaultSize )
    {
        wxSize bestSize = GetBestSize();

        if( size.GetHeight() != bestSize.GetHeight() )
            size.SetHeight( bestSize.GetHeight() );
    }

    SetSize( size );

    GetSizer()->Fit( this ); // Needed on Ubuntu/Unity to display the dialog
}


void DIALOG_SCH_FIND::OnClose( wxCloseEvent& aEvent )
{
    SendEvent( wxEVT_COMMAND_FIND_CLOSE );
}


void DIALOG_SCH_FIND::OnUpdateFindUI( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( !m_comboFind->GetValue().empty() );
}


void DIALOG_SCH_FIND::OnUpdateReplaceUI( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( HasFlag( wxFR_REPLACEDIALOG ) && !m_comboFind->GetValue().empty() &&
                   (m_findReplaceData->GetFlags() & FR_REPLACE_ITEM_FOUND) );
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


void DIALOG_SCH_FIND::OnReplace( wxCommandEvent& aEvent )
{
    int index = m_comboReplace->FindString( m_comboReplace->GetValue(), true );

    if( index == wxNOT_FOUND )
    {
        m_comboReplace->Insert( m_comboReplace->GetValue(), 0 );
    }
    else if( index != 0 )
    {
        /* Move the search string to the top of the list if it isn't already there. */
        wxString tmp = m_comboReplace->GetValue();
        m_comboReplace->Delete( index );
        m_comboReplace->Insert( tmp, 0 );
        m_comboReplace->SetSelection( 0 );
    }

    if( aEvent.GetId() == wxID_REPLACE )
        SendEvent( wxEVT_COMMAND_FIND_REPLACE );
    else if( aEvent.GetId() == wxID_REPLACE_ALL )
        SendEvent( wxEVT_COMMAND_FIND_REPLACE_ALL );
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

    int flags = 0;

    if ( HasFlag( wxFR_REPLACEDIALOG ) )
    {
        event.SetReplaceString( m_comboReplace->GetValue() );
        flags |= FR_SEARCH_REPLACE;
    }

    if( m_checkReplaceReferences->GetValue() )
        flags |= FR_REPLACE_REFERENCES;

    if( m_radioForward->GetValue() )
        flags |= wxFR_DOWN;

    if( m_checkMatchCase->GetValue() )
        flags |= wxFR_MATCHCASE;

    if( m_checkWholeWord->GetValue() )
        flags |= wxFR_WHOLEWORD;

    if( m_checkWildcardMatch->IsShown() && m_checkWildcardMatch->GetValue() )
        flags |= FR_MATCH_WILDCARD;

    if( m_checkAllFields->GetValue() )
        flags |= FR_SEARCH_ALL_FIELDS;

    if( m_checkAllPins->GetValue() )
        flags |= FR_SEARCH_ALL_PINS;

    if( m_checkWrap->GetValue() )
        flags |= FR_SEARCH_WRAP;

    if( m_checkCurrentSheetOnly->GetValue() )
        flags |= FR_CURRENT_SHEET_ONLY;

    if( m_checkNoWarpCursor->GetValue() )
        flags |= FR_NO_WARP_CURSOR;

    m_findReplaceData->SetFindString( event.GetFindString() );

    if( HasFlag( wxFR_REPLACEDIALOG )
        && ( event.GetEventType() == wxEVT_COMMAND_FIND_REPLACE
             || event.GetEventType() == wxEVT_COMMAND_FIND_REPLACE_ALL ) )
    {
        m_findReplaceData->SetReplaceString( event.GetReplaceString() );
    }

    event.SetFlags( flags );

    m_findReplaceData->SetFlags( event.GetFlags() );

    // when we are no using the find/replace (just find)
    // FR_REPLACE_REFERENCES flag bit is always set to 1 in event flags
    // but not set in m_findReplaceData
    if ( ! HasFlag( wxFR_REPLACEDIALOG ) )
    {
        flags |= FR_REPLACE_REFERENCES;
        event.SetFlags( flags );
    }

    if( !GetEventHandler()->ProcessEvent( event ) )
    {
        GetParent()->GetEventHandler()->ProcessEvent( event );
    }

    if( event.GetFlags() != flags )
        m_findReplaceData->SetFlags( event.GetFlags() );
}


wxArrayString DIALOG_SCH_FIND::GetFindEntries() const
{
    return m_comboFind->GetStrings();
}


void DIALOG_SCH_FIND::SetFindEntries( const wxArrayString& aEntries )
{
    m_comboFind->Append( aEntries );

    if( m_comboFind->GetCount() )
        m_comboFind->SetSelection( 0 );
}


void DIALOG_SCH_FIND::SetReplaceEntries( const wxArrayString& aEntries )
{
    m_comboReplace->Append( aEntries );

    if( m_comboReplace->GetCount() )
        m_comboReplace->SetSelection( 0 );
}
