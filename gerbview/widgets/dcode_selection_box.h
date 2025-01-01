/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

// file class_DCodeSelectionbox.h

#ifndef DCODESELECTIONBOX_H
#define DCODESELECTIONBOX_H

#include <wx/combobox.h>

// Define event type for DCODE_SELECTION_BOX
#define EVT_SELECT_DCODE EVT_COMBOBOX


/**
 * Helper to display a DCode list and select a DCode id.
 */
class DCODE_SELECTION_BOX : public wxComboBox
{
public:
    DCODE_SELECTION_BOX( wxAuiToolBar* aParent, wxWindowID aId,
                         const wxPoint& aLocation, const wxSize& aSize,
                         const wxArrayString* aChoices = nullptr );
    ~DCODE_SELECTION_BOX();

    /**
     * @return the current selected DCode Id or 0 if no dcode
     */
    int GetSelectedDCodeId();

    /**
     * @param aDCodeId is the DCode Id to select or <= 0 to select "no dcode".
     */
    void SetDCodeSelection( int aDCodeId );

    /**
     * @param aChoices is the DCode Id list to add to the combo box.
     */
    void AppendDCodeList( const wxArrayString& aChoices );
};

#endif //DCODESELECTIONBOX_H
