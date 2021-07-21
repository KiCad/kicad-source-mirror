/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


DIALOG_LOCKED_ITEMS_QUERY::DIALOG_LOCKED_ITEMS_QUERY( wxWindow* aParent, int aLockedItemCount ) :
    DIALOG_LOCKED_ITEMS_QUERY_BASE( aParent )
{
    m_icon->SetBitmap( KiBitmap( BITMAPS::locked ) );

    m_messageLine1->SetLabel( wxString::Format( m_messageLine1->GetLabel(), aLockedItemCount ) );

    m_sdbSizerOK->SetLabel( _( "Skip Locked Items" ) );
    m_sdbSizerOK->SetToolTip( _( "Remove locked items from the selection and only apply the "
                                 "operation to the unlocked items (if any)." ) );
    m_sdbSizerOK->SetDefault();
    m_sdbSizerOK->SetFocus();

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

    if( doNotShowValue != wxID_ANY )
        return doNotShowValue;

    int ret = DIALOG_SHIM::ShowModal();

    // Has the user asked not to show the dialog again this session?
    if( m_doNotShowBtn->IsChecked() && ret != wxID_CANCEL )
        doNotShowValue = ret;

    return ret;
}


