/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_paste_special.h>


static PASTE_MODE g_paste_mode = PASTE_MODE::UNIQUE_ANNOTATIONS;


DIALOG_PASTE_SPECIAL::DIALOG_PASTE_SPECIAL( wxWindow* aParent,
                                            PASTE_MODE* aMode,
                                            wxString aReplacement ) :
    DIALOG_PASTE_SPECIAL_BASE( aParent ),
    m_mode( aMode )
{
    m_pasteOptions->SetItemToolTip( static_cast<int>( PASTE_MODE::UNIQUE_ANNOTATIONS ),
                                    _( "Finds the next available reference designator for "
                                       "any designators that already exist in the design." ) );

    m_pasteOptions->SetItemToolTip( static_cast<int>( PASTE_MODE::KEEP_ANNOTATIONS ),
                                    wxT( "" ) ); // Self explanatory

    m_pasteOptions->SetItemToolTip( static_cast<int>( PASTE_MODE::REMOVE_ANNOTATIONS ),
                                    wxString::Format( _( "Replaces reference designators "
                                                         "with '%s'." ),
                                                      aReplacement ) );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_PASTE_SPECIAL::TransferDataToWindow()
{
    m_pasteOptions->SetSelection( static_cast<int>( g_paste_mode ) );
    return true;
}


bool DIALOG_PASTE_SPECIAL::TransferDataFromWindow()
{
    g_paste_mode = *m_mode = static_cast<PASTE_MODE>( m_pasteOptions->GetSelection() );
    return true;
}
