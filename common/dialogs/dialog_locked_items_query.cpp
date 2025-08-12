/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <dialogs/dialog_locked_items_query.h>
#include <bitmaps.h>


DIALOG_LOCKED_ITEMS_QUERY::DIALOG_LOCKED_ITEMS_QUERY( wxWindow* aParent, std::size_t aLockedItemCount,
                                                      PCBNEW_SETTINGS::LOCKING_OPTIONS& aLockingOptions ) :
        DIALOG_LOCKED_ITEMS_QUERY_BASE( aParent ),
        m_lockingOptions( aLockingOptions )
{
    // Potentially dangerous to save the state of the Do Not Show Again button between sessions.
    OptOut( m_doNotShowBtn );

    m_icon->SetBitmap( KiBitmapBundle( BITMAPS::locked ) );

    m_messageLine1->SetLabel( wxString::Format( m_messageLine1->GetLabel(), aLockedItemCount ) );

    SetupStandardButtons( { { wxID_OK, _( "Skip Locked Items" ) } } );
    m_sdbSizerOK->SetToolTip( _( "Remove locked items from the selection and only apply the "
                                 "operation to the unlocked items (if any)." ) );

    m_doNotShowBtn->SetToolTip( _( "Do not show this dialog again until KiCad restarts. "
                                   "You can re-enable this dialog in Pcbnew preferences." ) );

    // While this dialog can get called for a lot of different use-cases, we'll assume that it
    // does make sense to store state *between* the use-cases.  So we don't assign a separate
    // hash key for each use case.

    SetInitialFocus( m_sdbSizerOK );

    Layout();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_LOCKED_ITEMS_QUERY::onOverrideLocks( wxCommandEvent& event )
{
    // This will choose the correct way to end the dialog no matter how is was shown.
    EndDialog( wxID_APPLY );
}


int DIALOG_LOCKED_ITEMS_QUERY::ShowModal()
{
    static int doNotShowValue = wxID_ANY;

    if( doNotShowValue != wxID_ANY && m_lockingOptions.m_sessionSkipPrompts )
        return doNotShowValue;

    int ret = DIALOG_SHIM::ShowModal();

    // Has the user asked not to show the dialog again this session?
    if( m_doNotShowBtn->IsChecked() && ret != wxID_CANCEL )
    {
        doNotShowValue = ret;
        m_lockingOptions.m_sessionSkipPrompts = true;
    }

    return ret;
}
