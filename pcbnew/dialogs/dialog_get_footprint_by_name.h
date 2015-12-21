/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file dialog_get_footprint_by_name.h
 */

#ifndef DIALOG_GET_FOOTPRINT_BY_NAME_H
#define DIALOG_GET_FOOTPRINT_BY_NAME_H

#include <dialog_get_footprint_by_name_base.h>

/**
 * DIALOG_GET_FOOTPRINT_BY_NAME is a helper dialog to select a footprint by its reference
 * One can enter any reference or select it in the list of fp references
 * Get a footprint ref from user and return a pointer to the corresponding footprint
 */
class DIALOG_GET_FOOTPRINT_BY_NAME: public DIALOG_GET_FOOTPRINT_BY_NAME_BASE
{
public:
    DIALOG_GET_FOOTPRINT_BY_NAME( PCB_BASE_FRAME* aParent, wxArrayString& aFpList ) :
        DIALOG_GET_FOOTPRINT_BY_NAME_BASE( aParent )
    {
        m_sdbSizerOK->SetDefault();
        m_choiceFpList->Append( aFpList );

        // Dialog should not shrink beyond it's minimal size.
        GetSizer()->SetSizeHints( this );
    }

    // returns the selected text (fp reference)
    const wxString GetValue()
    {
        return m_SearchTextCtrl->GetValue();
    }

private:
    // Called when selecting an item from the item list
	void OnSelectFootprint( wxCommandEvent& aEvent )
    {
        if( m_choiceFpList->GetSelection() >= 0 )
            m_SearchTextCtrl->SetValue(
                m_choiceFpList->GetString( m_choiceFpList->GetSelection() ).BeforeFirst( ' ' ) );
    }
};

#endif  // #ifndef DIALOG_GET_FOOTPRINT_BY_NAME_H
