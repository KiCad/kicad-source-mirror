/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kidialog.h>
#include <unordered_map>


// Set of dialogs that have been chosen not to be shown again
static std::unordered_map<unsigned long, int> g_doNotShowAgainDlgs;


KIDIALOG::KIDIALOG( wxWindow* aParent, const wxString& aMessage, const wxString& aCaption, long aStyle ) :
        KIDIALOG_BASE( aParent, aMessage, aCaption, aStyle | wxCENTRE | wxSTAY_ON_TOP ),
        m_hash( 0 ),
        m_cancelMeansCancel( true )
{
}


KIDIALOG::KIDIALOG( wxWindow* aParent, const wxString& aMessage, KD_TYPE aType, const wxString& aCaption ) :
        KIDIALOG_BASE( aParent, aMessage, getCaption( aType, aCaption ), getStyle( aType ) ),
        m_hash( 0 ),
        m_cancelMeansCancel( true )
{
}


void KIDIALOG::ClearDoNotShowAgainDialogs()
{
    g_doNotShowAgainDlgs = {};
}


void KIDIALOG::DoNotShowCheckbox( wxString aUniqueId, int line )
{
    ShowCheckBox( _( "Do not show again" ), false );

    m_hash = std::hash<wxString>{}( aUniqueId ) + line;
}


bool KIDIALOG::DoNotShowAgain() const
{
    return g_doNotShowAgainDlgs.count( m_hash ) > 0;
}


bool KIDIALOG::Show( bool aShow )
{
    // We should check the do-not-show-again setting only when the dialog is displayed
    if( aShow )
    {
        // Check if this dialog should be shown to the user
        auto it = g_doNotShowAgainDlgs.find( m_hash );

        if( it != g_doNotShowAgainDlgs.end() )
            return it->second;
    }

    int ret = KIDIALOG_BASE::Show( aShow );

    // Has the user asked not to show the dialog again?
    // Note that we don't save a Cancel value unless the Cancel button is being used for some
    // other function (which is actually more common than it being used for Cancel).
    if( IsCheckBoxChecked() && (!m_cancelMeansCancel || ret != wxID_CANCEL ) )
        g_doNotShowAgainDlgs[m_hash] = ret;

    return ret;
}


int KIDIALOG::ShowModal()
{
    // Check if this dialog should be shown to the user
    auto it = g_doNotShowAgainDlgs.find( m_hash );

    if( it != g_doNotShowAgainDlgs.end() )
        return it->second;

    int ret = KIDIALOG_BASE::ShowModal();

    // Has the user asked not to show the dialog again?
    // Note that we don't save a Cancel value unless the Cancel button is being used for some
    // other function (which is actually more common than it being used for Cancel).
    if( IsCheckBoxChecked() && (!m_cancelMeansCancel || ret != wxID_CANCEL ) )
        g_doNotShowAgainDlgs[m_hash] = ret;

    return ret;
}


wxString KIDIALOG::getCaption( KD_TYPE aType, const wxString& aCaption )
{
    if( !aCaption.IsEmpty() )
        return aCaption;

    switch( aType )
    {
    case KD_NONE:       /* fall through */
    case KD_INFO:       return _( "Message" );
    case KD_QUESTION:   return _( "Question" );
    case KD_WARNING:    return _( "Warning" );
    case KD_ERROR:      return _( "Error" );
    }

    return wxEmptyString;
}


long KIDIALOG::getStyle( KD_TYPE aType )
{
    long style = wxOK | wxCENTRE | wxSTAY_ON_TOP;

    switch( aType )
    {
    case KD_NONE:       break;
    case KD_INFO:       style |= wxICON_INFORMATION; break;
    case KD_QUESTION:   style |= wxICON_QUESTION;    break;
    case KD_WARNING:    style |= wxICON_WARNING;     break;
    case KD_ERROR:      style |= wxICON_ERROR;       break;
    }

    return style;
}

