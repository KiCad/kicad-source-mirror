/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <pgm_base.h>
#include <wxstruct.h>
#include <class_drawpanel.h>
#include <gerbview.h>
#include <dcode.h>

#include <class_DCodeSelectionbox.h>

/*******************************************/
/* Helper class for displaying DCodes list */
/*******************************************/

DCODE_SELECTION_BOX::DCODE_SELECTION_BOX( wxAuiToolBar* aParent, wxWindowID aId,
                                          const wxPoint& aLocation, const wxSize& aSize,
                                          const wxArrayString& aChoices  ) :
    wxComboBox( aParent, aId, wxEmptyString, aLocation, aSize, 0, NULL, wxCB_READONLY )
{
    m_dcodeList  = &aChoices;
    // Append aChoices here is by far faster than use aChoices inside
    // the wxComboBox constructor
    Append(aChoices);
}


DCODE_SELECTION_BOX::~DCODE_SELECTION_BOX()
{
}


int DCODE_SELECTION_BOX::GetSelectedDCodeId()
{
    int ii = GetSelection();

    if( ii > 0 )
    {
        wxString msg = (*m_dcodeList)[ii].AfterFirst( wxChar( ' ' ) );
        long id;
        msg.ToLong(&id);
        return id;
    }

    return -1;
}


/* SetDCodeSelection
 * aDCodeId = the DCode Id to select or -1 to select "no dcode"
 */
void DCODE_SELECTION_BOX::SetDCodeSelection( int aDCodeId )
{
    if( aDCodeId > LAST_DCODE )
        aDCodeId = LAST_DCODE;

    int index = 0;
    if( aDCodeId >= FIRST_DCODE )
        index = aDCodeId - FIRST_DCODE + 1;

    SetSelection(index);
}
