/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <draw_frame.h>
#include <dialogs/dialog_get_footprint.h>
#include <lib_id.h>
#include <pcb_base_frame.h>
#include <lib_table_base.h>


/****************************************************************************/
/* Show a dialog frame to choose a name from an history list, or a new name */
/* to select a module                                                       */
/****************************************************************************/

static wxArrayString s_HistoryList;
static unsigned      s_HistoryMaxCount = 8;  // Max number of items displayed in history list


DIALOG_GET_FOOTPRINT::DIALOG_GET_FOOTPRINT( PCB_BASE_FRAME* parent, bool aShowBrowseButton ) :
    DIALOG_GET_FOOTPRINT_BASE( parent, -1 ),
    m_frame( parent )
{

    m_Text = wxEmptyString;
    m_selectByBrowser = false;
    m_selectionIsKeyword = false;

    for( size_t ii = 0; ii < s_HistoryList.size(); ++ii )
    {
        LIB_ID fpid( s_HistoryList[ ii ] );
        if( m_frame->CheckFootprint( fpid ) )
            m_historyList->Append( s_HistoryList[ ii ] );
    }

    m_buttonBrowse->Show( aShowBrowseButton );
    m_buttonBrowse->Enable( aShowBrowseButton );

    m_sdbSizerOK->SetDefault();

    m_textCmpNameCtrl->SetFocus();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void DIALOG_GET_FOOTPRINT::OnHistoryClick( wxCommandEvent& aEvent )
{
    m_textCmpNameCtrl->SetValue( m_historyList->GetStringSelection() );
}


void DIALOG_GET_FOOTPRINT::Accept( wxCommandEvent& aEvent )
{
    m_selectionIsKeyword = false;
    switch( aEvent.GetId() )
    {
    case ID_SEL_BY_LISTBOX:
        m_Text = m_historyList->GetStringSelection();
        break;

    case wxID_OK:
        if( m_historyList->HasFocus() )
            m_Text = m_historyList->GetStringSelection();
        else
            m_Text = m_textCmpNameCtrl->GetValue();
        break;

    case ID_ACCEPT_KEYWORD:
        m_selectionIsKeyword = true;
        m_Text = m_textCmpNameCtrl->GetValue();
        break;

    case ID_LIST_ALL:
        m_Text = wxT( "*" );
        break;

    case ID_BROWSE:
        m_Text = wxEmptyString;
        m_selectByBrowser = true;
        break;
    }

    m_Text.Trim( false );      // Remove blanks at beginning
    m_Text.Trim( true );       // Remove blanks at end

    // Put an wxID_OK event through the dialog infrastrucutre
    aEvent.SetEventType( wxEVT_COMMAND_BUTTON_CLICKED );
    aEvent.SetId( wxID_OK );
    aEvent.Skip();
}


// Return the component name selected by the dialog
wxString DIALOG_GET_FOOTPRINT::GetComponentName( void )
{
    return m_Text;
}


/* Initialize the default component name default choice
*/
void DIALOG_GET_FOOTPRINT::SetComponentName( const wxString& name )
{
    if( m_textCmpNameCtrl )
    {
        m_textCmpNameCtrl->SetValue( name );
        m_textCmpNameCtrl->SetSelection( -1, -1 );
    }
}


/*
 * Add the string "aName" to the history list aHistoryList
 */
void AddHistoryComponentName( const wxString& aName )
{
    // Remove duplicates
    for( int ii = s_HistoryList.GetCount() - 1; ii >= 0; --ii )
    {
        if( s_HistoryList[ ii ] == aName )
            s_HistoryList.RemoveAt( (size_t) ii );
    }

    // Add the new name at the beginning of the history list
    s_HistoryList.Insert( aName, 0 );

    // Remove extra names
    while( s_HistoryList.GetCount() >= s_HistoryMaxCount )
        s_HistoryList.RemoveAt( s_HistoryList.GetCount() - 1 );
}
